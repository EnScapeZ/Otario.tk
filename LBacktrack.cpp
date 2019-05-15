#include "LBacktrack.h"

float GetCurtimebetab()
{
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
	if (!local_player)
		return 0.f;

	return local_player->GetTickBase() * g_pGlobalVars->intervalPerTick;
}


template <class T>
constexpr const T& Clampbetab(const T& v, const T& lo, const T& hi)
{
	return (v >= lo && v <= hi) ? v : (v < lo ? lo : hi);
}

template <class T>
constexpr const T& Minbetab(const T& x, const T& y)
{
	return (x > y) ? y : x;
}

namespace FEATURES
{
	namespace RAGEBOT
	{
		CBacktracking backtracking;

		Backtracking_Record::Backtracking_Record(C_BaseEntity* ent)
		{
			vec_origin = ent->GetVecOrigin();
			abs_angles = ent->GetAbsAngles();
		//	eye_angles = ent->GetEyeAngles();
			render_angles = ent->GetRenderAngles();
			velocity = ent->GetVelocity();
			simulation_time = ent->m_flSimulationTime();
			flags = ent->GetFlags();
			animstate = *ent->GetAnimState();
			curtime = g_pGlobalVars->curtime;
			is_exploiting = false;

			for (int i = 0; i < 15; i++)
				anim_layers[i] = ent->GetAnimOverlay(i);
			for (int i = 0; i < 128; i++)
				bone_matrix[i] = ent->GetBoneMatrix(i);

			memcpy(pose_params, ent->GetPoseParamaters(), 96);

			auto collideable = ent->GetCollideable();
			if (!collideable)
				return;

			bbmin = collideable->OBBMins();
			bbmax = collideable->OBBMaxs();

			player = ent;
		}

		Backtracking_Record* CBacktracking::GetCurrentRecord(C_BaseEntity* entity)
		{
			if (!player_records[entity->EntIndex()].size())
				return nullptr;

			return &player_records[entity->EntIndex()][0];
		}

		void CBacktracking::Store()
		{
			auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
			if (!local_player || local_player->GetHealth() <= 0)
				return;

			for (int i = 0; i < 64; i++)
			{
				auto entity = g_pEntityList->GetClientEntity(i);
				if (!entity || entity->GetHealth() <= 0 || entity->GetTeam() == local_player->GetTeam())
				{
					player_records[i].clear();
					continue;
				}

				if (entity->IsDormant() || entity->IsImmune())
					continue;

				SetInterpolationFlags(entity, DISABLE_INTERPOLATION);
				entity->UpdateClientSideAnimation();

				if (!player_records[i].size())
				{
					player_records[i].insert(player_records[i].begin(), Backtracking_Record(entity));
					continue;
				}

				// only add if simtime updates, so when player sends data
				if (player_records[i][0].simulation_time != entity->m_flSimulationTime())
				{
					auto record = Backtracking_Record(entity);

					record.previous_vec_origin = player_records[i][0].vec_origin;
					record.previous_simulation_time = player_records[i][0].simulation_time;
					record.previous_curtime = player_records[i][0].curtime;

					player_records[i].insert(player_records[i].begin(), record);
				}

				// if more than 1 second worth of records pop back the last record
				if (g_pGlobalVars->curtime - player_records[i].back().curtime > MAX_UNLAG)
					player_records[i].pop_back();
			}
		}

		void CBacktracking::Restore(C_BaseEntity* entity, Backtracking_Record rec, int extrapolate_ticks)
		{
			needs_to_restore[entity->EntIndex()] = true;

			auto extrapolation_amount = Vector(0, 0, 0);
			if (extrapolate_ticks != 0)
			{
				const auto acceleration = Vector(0, 0, 0);
				const float delta_time = (rec.curtime - rec.previous_curtime);
				auto velocity = (rec.vec_origin - rec.previous_vec_origin) / (delta_time > 0.f ? delta_time : 1.f);

				while (extrapolate_ticks > 0)
				{
					extrapolation_amount += velocity * g_pGlobalVars->intervalPerTick;
					velocity += acceleration * g_pGlobalVars->intervalPerTick;

					extrapolate_ticks--;
				}
				while (extrapolate_ticks < 0)
				{
					extrapolation_amount += velocity * g_pGlobalVars->intervalPerTick;
					velocity += acceleration * g_pGlobalVars->intervalPerTick;

					extrapolate_ticks++;
				}
			}

			memcpy(entity->GetPoseParamaters(), rec.pose_params, 96);
			memcpy(entity->GetAnimState(), &rec.animstate, sizeof(CBaseAnimState));

			for (int i = 0; i < 15; i++)
				entity->SetAnimOverlay(i, rec.anim_layers[i]);

			auto collideable = entity->GetCollideable();
			if (!collideable)
				return;

			//*reinterpret_cast<Vector*>(uintptr_t(entity) + OFFSETS::m_angRotation) = Vector(0, 0, 0);
			//            DT_BaseAnimating m_bClientSideAnimation -------------------------------------------------------------------------- 0x288c
			//			  DT_BaseAnimating m_bClientSideFrameReset

			entity->SetEyeAngles(rec.eye_angles);
			entity->SetAbsAngles(QAngle(0, rec.abs_angles.y, 0));
			entity->SetRenderAngles(rec.render_angles);
			entity->SetAbsOrigin(rec.vec_origin + extrapolation_amount);
			entity->SetVelocity(rec.velocity);
			//entity->SetFlags(rec.flags);

			collideable->OBBMins() = rec.bbmin;
			collideable->OBBMaxs() = rec.bbmax;
		}

		void CBacktracking::ApplyRestore(C_BaseEntity* entity, float curtime)
		{
			/// pvs fix
			*reinterpret_cast<int*>(uintptr_t(entity) + 0xA30) = 0;
			*reinterpret_cast<int*>(uintptr_t(entity) + 0xA28) = 0;

			InvalidateBoneCache(entity);

			entity->UpdateClientSideAnimation();

			entity->SetupBones2(NULL, 128, BONE_USED_BY_ANYTHING, curtime); 
		}
		
		void CBacktracking::RestoreToCurrentRecord(C_BaseEntity* entity)
		{
			if (!player_records[entity->EntIndex()].size() || !needs_to_restore[entity->EntIndex()])
				return;

			auto record = player_records[entity->EntIndex()][0];

			Restore(entity, record);
			ApplyRestore(entity, g_pGlobalVars->curtime);

			needs_to_restore[entity->EntIndex()] = false;
		}

		int CBacktracking::GetTickCount(Backtracking_Record record)
		{
			int ideal_tick = TIME_TO_TICKS(record.simulation_time) + GetLerpTicks();

			return ideal_tick;
		}

		void CBacktracking::SetInterpolationFlags(C_BaseEntity * entity, int flag)
		{
			auto VarMap = reinterpret_cast<uintptr_t>(entity) + 36; // tf2 = 20
			auto VarMapSize = *reinterpret_cast<int*>(VarMap + 20);

			for (auto index = 0; index < VarMapSize; index++)
				*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(VarMap) + index * 12) = flag;
		}

		std::vector<Backtracking_Record> CBacktracking::GetPriorityRecords(C_BaseEntity* entity)
		{
			std::vector<Backtracking_Record> priority_records;
			const int player_index = entity->EntIndex();

			auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
			if (!local_player || local_player->GetHealth() <= 0 || player_records[player_index].size() <= 0)
				return priority_records;

			/// try to find all the records where they're resolved		
			std::vector<Backtracking_Record> resolved_records;
			for (const auto& record : player_records[player_index])
			{
				if (GetDeltaTime(record) >= MAX_BACKTRACK_RANGE && !record.is_exploiting)
					continue;
			}

			/// no resolved records found :(
			if (!resolved_records.size())
			{
				for (const auto& record : player_records[player_index])
				{
					if (GetDeltaTime(record) >= MAX_BACKTRACK_RANGE)
						continue;
				}

				for (const auto& record : player_records[player_index])
				{
					if (GetDeltaTime(record) >= MAX_BACKTRACK_RANGE)
						continue;

					priority_records.push_back(record);
					break;
				}

				return priority_records;
			}

			/// sort through the resolved records (the could be many if they're moving) and get 2 good ones

			/// not enough records to need sorting
			if (resolved_records.size() <= 2)
				return resolved_records;

			const auto left_yaw = Utils::CalcAngle(local_player->GetVecOrigin(), entity->GetVecOrigin()).y + 90.f,
				right_yaw = Utils::CalcAngle(local_player->GetVecOrigin(), entity->GetVecOrigin()).y - 90.f;

			/// try to find one where they're yaw is sideways to us (easier to hit fam)
			Backtracking_Record sideways_rec;
			float lowest_sideways_delta = 180.f;
			for (const auto& record : resolved_records)
			{
				const float current_delta = Minbetab<float>(fabs(Utils::NormalizeYaw(record.eye_angles.y - left_yaw)), fabs(Utils::NormalizeYaw(record.eye_angles.y - right_yaw)));
				if (current_delta < lowest_sideways_delta)
				{
					lowest_sideways_delta = current_delta;
					sideways_rec = record;
				}
			}

			/// find the farthest away record from the sideways_rec (for diversity, since the sideways_rec could be behind a wall or smthn)
			Backtracking_Record opposite_rec;
			float highest_delta = 0.f;
			for (const auto& record : resolved_records)
			{
				const float current_delta = fabs(Utils::NormalizeYaw(record.eye_angles.y - sideways_rec.eye_angles.y));
				if (current_delta > highest_delta)
				{
					highest_delta = current_delta;
					opposite_rec = record;
				}
			}

			if (highest_delta > 45.f)
			{
				priority_records.push_back(opposite_rec);
				priority_records.push_back(sideways_rec);
			}
			else /// return the first record and the slowest record
			{
				Backtracking_Record slowest_record = resolved_records.back();
				float slowest_speed = slowest_record.velocity.Length2D();
				for (const auto& record : resolved_records)
				{
					const float speed = record.velocity.Length2D();
					if (speed < slowest_speed)
					{
						slowest_speed = speed;
						slowest_record = record;
					}
				}

				if (fabs(slowest_speed - resolved_records.back().velocity.Length2D()) < 30.f)
					slowest_record = resolved_records.back();

				priority_records.push_back(resolved_records.front());
				priority_records.push_back(slowest_record);
			}

			return priority_records;
		}

		std::vector<Backtracking_Record> CBacktracking::GetValidRecords(C_BaseEntity* entity)
		{
			std::vector<Backtracking_Record> valid_records;

			for (const auto& record : player_records[entity->EntIndex()])
			{
				if (GetDeltaTime(record) < MAX_BACKTRACK_RANGE)
					valid_records.push_back(record);
			}

			return valid_records;
		}

		std::vector<Backtracking_Record> CBacktracking::GetRecords(C_BaseEntity* entity)
		{
			return player_records[entity->EntIndex()];
		}

		bool CBacktracking::GetExtrapolatedRecord(C_BaseEntity* entity, Backtracking_Record& record)
		{
			const int player_index = entity->EntIndex();

			if (!player_records[player_index].size())
				return false;

			record = player_records[player_index][0];

			float time_to_extrapolate_to;
			if (GetDeltaTime(record) > MAX_BACKTRACK_RANGE)
				time_to_extrapolate_to = g_pGlobalVars->curtime;
			else /// breaking lag compensation
				return false;

			if (!Extrapolate(entity, record, time_to_extrapolate_to))
				return false;

			return true;
		}

		bool CBacktracking::Extrapolate(C_BaseEntity* entity, Backtracking_Record& record, float time)
		{
			const int player_index = entity->EntIndex();
			const float time_delta = time - record.curtime;

			static auto sv_gravity = g_pConVar->FindVar("sv_gravity");
			static auto sv_jump_impulse = g_pConVar->FindVar("sv_jump_impulse");

			/// need 3 records to extrapolate
			if (player_records[player_index].size() < 3)
				return false;

			/// to check if ground is below the nigger, and speed is per second
			auto IsObjectInWay = [](Vector origin, Vector velocity, Vector& end) -> bool
			{
				trace_t trace;
				CTraceWorldOnly filter;
				Ray_t ray;
				ray.Init(origin, origin + (velocity * g_pGlobalVars->intervalPerTick));

				g_pTrace->TraceRay(ray, MASK_ALL, &filter, &trace);

				end = trace.endpos;

				return trace.fraction < 1.f;
			};

			auto record_1 = player_records[player_index][0], record_2 = player_records[player_index][1], record_3 = player_records[player_index][2];
			record = record_1;

			/// velocity and acceleration are per second, not per tick
			Vector velocity = record.velocity;
			Vector acceleration = ((record_1.velocity - record_2.velocity) + (record_2.velocity - record_3.velocity)) / (record_1.simulation_time - record_2.simulation_time);
			acceleration.z = -sv_gravity->GetFloat();

			bool was_object_in_way_last_tick = false;
			float curtime = record.simulation_time;
			while (curtime < time)
			{
				Vector vel_change = velocity * g_pGlobalVars->intervalPerTick;

				record.vec_origin += vel_change;
				record.bbmax += vel_change;
				record.bbmin += vel_change;

				velocity += acceleration * g_pGlobalVars->intervalPerTick;

				Vector end;
				if (IsObjectInWay(record.vec_origin, velocity, end))
				{
					record.vec_origin = end;

					if (!was_object_in_way_last_tick)
						velocity.z = sv_jump_impulse->GetFloat();
					else
						break;

					was_object_in_way_last_tick = true;
				}
				else
					was_object_in_way_last_tick = false;

				curtime += g_pGlobalVars->intervalPerTick;
			}

			return true;
		}

		void CBacktracking::InvalidateBoneCache(C_BaseEntity* entity)
		{
			static uintptr_t InvalidateBoneCache = Utils::FindSignature("client_panorama.dll", "80 3D ? ? ? ? ? 74 16 A1 ? ? ? ? 48 C7 81");
			static uintptr_t g_iModelBoneCounter = **(uintptr_t**)(InvalidateBoneCache + 10); //	Offsets::InvalidateBoneCache = FindPatternIDA("client.dll", "80 3D ? ? ? ? 00 74 16 A1");
		//	*(int*)((uintptr_t)entity + entity->ForceBone() + 0x20) = 0; //m_nForceBone + 0x20
			*(uintptr_t*)((uintptr_t)entity + 0x2924) = 0xFF7FFFFF; // m_flLastBoneSetupTime = -FLT_MAX;
			*(uintptr_t*)((uintptr_t)entity + 0x2690) = (g_iModelBoneCounter - 1); // m_iMostRecentModelBoneCounter = g_iModelBoneCounter - 1;
		}

		float CBacktracking::GetDeltaTime(Backtracking_Record record)
		{
			auto nci = g_pEngine->GetNetChannelInfo();
			if (!nci)
				return FLT_MAX;

			float correct = 0.f;
			correct += nci->GetLatency(FLOW_OUTGOING);
			correct += nci->GetLatency(FLOW_INCOMING);
			correct += TICKS_TO_TIME(GetLerpTicks());
			correct = Clampbetab<float>(correct, 0.f, 1.f);

			return fabs(correct - (GetCurtimebetab() - record.simulation_time));
		}

		int CBacktracking::GetLerpTicks()
		{
			static const auto cl_interp_ratio = g_pConVar->FindVar("cl_interp_ratio");
			static const auto cl_updaterate = g_pConVar->FindVar("cl_updaterate");
			static const auto cl_interp = g_pConVar->FindVar("cl_interp");

			return TIME_TO_TICKS(max(cl_interp->GetFloat(), cl_interp_ratio->GetFloat() / cl_updaterate->GetFloat()));
		}
	}
}
