#include "betaAutoWall.h"

void ClipTraceToPlayers2(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter* filter, trace_t* tr)
{
	static DWORD dwAddress = Utils::FindSignature("client_panorama.dll", "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC ? ? ? ? 8B 43 10");

	if (!dwAddress)
		return;

	_asm
	{
		MOV		EAX, filter
		LEA		ECX, tr
		PUSH	ECX
		PUSH	EAX
		PUSH	mask
		LEA		EDX, vecAbsEnd
		LEA		ECX, vecAbsStart
		CALL	dwAddress
		ADD		ESP, 0xC
	}
}

bool IsTaser(void* weapon)
{
	if (weapon == nullptr) return false;
	C_BaseEntity* weaponEnt = (C_BaseEntity*)weapon;
	ClientClass* pWeaponClass = weaponEnt->GetClientClass();

	if (pWeaponClass->m_ClassID == (int)EClassIds::CWeaponTaser)
		return true;
	else
		return false;
}

namespace FEATURES
{
	namespace RAGEBOT
	{
		Autowall betaautowall;

		Autowall::Autowall_Return_Info Autowall::CalculateDamage(Vector start, Vector end, C_BaseEntity* from_entity, C_BaseEntity* to_entity, int specific_hitgroup)
		{
			// default values for return info, in case we need to return abruptly
			Autowall_Return_Info return_info;
			return_info.damage = -1;
			return_info.hitgroup = -1;
			return_info.hit_entity = nullptr;
			return_info.penetration_count = 4;
			return_info.thickness = 0.f;
			return_info.did_penetrate_wall = false;

			Autowall_Info autowall_info;
			autowall_info.penetration_count = 4;
			autowall_info.start = start;
			autowall_info.end = end;
			autowall_info.current_position = start;
			autowall_info.thickness = 0.f;

			// direction 
			Utils::AngleVectors(Utils::CalcAngle(start, end), &autowall_info.direction);

			// attacking entity
			if (!from_entity)
				from_entity = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
			if (!from_entity)
				return return_info;

			auto filter_player = CTraceFilterOneEntity();
			filter_player.pEntity = to_entity;
			auto filter_local = CTraceFilter();
			filter_local.pSkip = from_entity;

			// setup filters
			if (to_entity)
				autowall_info.filter = &filter_player;
			else
				autowall_info.filter = &filter_player;

			// weapon
			C_BaseCombatWeapon* weapon = g::pLocalEntity->GetActiveWeapon();
			if (!weapon)
				return return_info;

			// weapon data
			auto weapon_info = weapon->GetCSWpnData();
			if (!weapon_info)
				return return_info;

			// client class
			auto weapon_client_class = reinterpret_cast<C_BaseEntity*>(weapon)->GetClientClass();
			if (!weapon_client_class)
				return return_info;

			Vector eyePosition = g::pLocalEntity->GetEyeOrigin();

			
			// weapon range
			float range = min(weapon_info->flRange, (start - end).Length());
			end = start + (autowall_info.direction * range);
			autowall_info.current_damage = weapon_info->iDamage;

			while (autowall_info.current_damage > 0 && autowall_info.penetration_count > 0)
			{
				return_info.penetration_count = autowall_info.penetration_count;

				UTIL_TraceLine(start, end, 0x4600400B, from_entity, &autowall_info.enter_trace);
				ClipTraceToPlayers2(start, end * 40.f, 0x4600400B, autowall_info.filter, &autowall_info.enter_trace);

				const float distance_traced = (autowall_info.enter_trace.endpos - start).Length();
				autowall_info.current_damage *= pow(weapon_info->flRangeModifier, (distance_traced / 500.f));

				/// if reached the end
				if (autowall_info.enter_trace.fraction == 1.f)
				{
					if (to_entity && specific_hitgroup != -1)
					{
						ScaleDamage(to_entity, weapon_info, specific_hitgroup, autowall_info.current_damage);

						return_info.damage = autowall_info.current_damage;
						return_info.hitgroup = specific_hitgroup;
						return_info.end = autowall_info.enter_trace.endpos;
						return_info.hit_entity = to_entity;
					}
					else
					{
						return_info.damage = autowall_info.current_damage;
						return_info.hitgroup = -1;
						return_info.end = autowall_info.enter_trace.endpos;
						return_info.hit_entity = nullptr;
					}

					break;
				}
				// if hit an entity
				if (autowall_info.enter_trace.hitgroup > 0 && autowall_info.enter_trace.hitgroup <= 7 && autowall_info.enter_trace.m_pEnt)
				{
					// checkles gg
					if ((to_entity && autowall_info.enter_trace.m_pEnt != to_entity) ||
						(autowall_info.enter_trace.m_pEnt->GetTeam() == from_entity->GetTeam()))
					{
						return_info.damage = -1;
						return return_info;
					}

					if (specific_hitgroup != -1)
						ScaleDamage(autowall_info.enter_trace.m_pEnt, weapon_info, specific_hitgroup, autowall_info.current_damage);
					else
						ScaleDamage(autowall_info.enter_trace.m_pEnt, weapon_info, autowall_info.enter_trace.hitgroup, autowall_info.current_damage);

					// fill the return info
					return_info.damage = autowall_info.current_damage;
					return_info.hitgroup = autowall_info.enter_trace.hitgroup;
					return_info.end = autowall_info.enter_trace.endpos;
					return_info.hit_entity = autowall_info.enter_trace.m_pEnt;

					break;
				}

				// break out of the loop retard
				//if (!CanPenetrate(from_entity, autowall_info, weapon_info));
					break;

				return_info.did_penetrate_wall = true;
			}

			return_info.penetration_count = autowall_info.penetration_count;
		//	Utils::Log("dmg %", return_info.damage);
			
			return return_info;
		}

		float Autowall::GetThickness(Vector start, Vector end)
		{
			float current_thickness = 0.f;

			Vector direction;
			Utils::AngleVectors(Utils::CalcAngle(start, end), &direction);

			CTraceWorldOnly filter;
			trace_t enter_trace;
			trace_t exit_trace;
			Ray_t ray;

			int pen = 0;
			while (pen < 4)
			{
				ray.Init(start, end);

				g_pTrace->TraceRay(ray, MASK_ALL, &filter, &enter_trace);
				if (enter_trace.fraction == 1.f)
					return current_thickness;

				start = enter_trace.endpos;
				if (!TraceToExit(enter_trace, exit_trace, start, direction))
					return current_thickness + 90.f;

				start = exit_trace.endpos;
				current_thickness += (start - exit_trace.endpos).Length();

				pen++;
			}

			return current_thickness;
		}

		bool Autowall::CanPenetrate(C_BaseEntity* attacker, Autowall_Info& info, WeaponInfo_t* weapon_data)
		{
			//typedef bool(__thiscall* HandleBulletPenetrationFn)(SDK::CBaseEntity*, float&, int&, int*, SDK::trace_t*, Vector*, float, float, float, int, int, float, int*, Vector*, float, float, float*);
			//CBaseEntity *pLocalPlayer, float *flPenetration, int *SurfaceMaterial, char *IsSolid, trace_t *ray, Vector *vecDir, int unused, float flPenetrationModifier, float flDamageModifier, int unused2, int weaponmask, float flPenetration2, int *hitsleft, Vector *ResultPos, int unused3, int unused4, float *damage)
			//typedef bool(__thiscall* HandleBulletPenetrationFn)(C_BaseEntity*, float*, int*, char*, trace_t*, Vector*, int, float, float, int, int, float, int*, Vector*, int, int, float*);
			//static HandleBulletPenetrationFn HBP = reinterpret_cast<HandleBulletPenetrationFn>(Utils::FindSignature("client_panorama.dll",
			//	"53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC ? ? ? ? 56 8B 73 2C"));

			C_BaseCombatWeapon* weapon = g::pLocalEntity->GetActiveWeapon();
			if (!weapon)
				return false;

			auto enter_surface_data = g_pPhysProps->GetSurfaceData(info.enter_trace.surface.surfaceProps);
			if (!enter_surface_data)
				return true;

			char is_solid = 0;
			int material = enter_surface_data->game.material;
			int mask = IsTaser(weapon) ? 0x1100 : 0x1002;

			// glass and shit gg
			if (info.enter_trace.m_pEnt && !strcmp("CBreakableSurface",
				info.enter_trace.m_pEnt->GetClientClass()->pNetworkName))
				*reinterpret_cast<byte*>(uintptr_t(info.enter_trace.m_pEnt + 0x27C)) = 2;

			is_autowalling = true;
			bool return_value = !HandleBulletPenetration2(weapon_data, info);
			is_autowalling = false;
			return return_value;
		}

		void Autowall::ScaleDamage(C_BaseEntity* entity, WeaponInfo_t* weapon_info, int hitgroup, float& current_damage)
		{
			//Cred. to N0xius for reversing this.
			//TODO: _xAE^; look into reversing this yourself sometime

			bool hasHeavyArmor = false;
			int armorValue = entity->GetArmor();

			//Fuck making a new function, lambda beste. ~ Does the person have armor on for the hitbox checked?
			auto IsArmored = [&entity, &hitgroup]()-> bool
			{
				C_BaseEntity* targetEntity = entity;
				switch (hitgroup)
				{
				case HITGROUP_HEAD:
					return targetEntity->HasHelmet();
				case HITGROUP_GENERIC:
				case HITGROUP_CHEST:
				case HITGROUP_STOMACH:
				case HITGROUP_LEFTARM:
				case HITGROUP_RIGHTARM:
					return true;
				default:
					return false;
				}
			};

			switch (hitgroup)
			{
			case HITGROUP_HEAD:
				current_damage *= hasHeavyArmor ? 2.f : 4.f; //Heavy Armor does 1/2 damage
				break;
			case HITGROUP_STOMACH:
				current_damage *= 1.25f;
				break;
			case HITGROUP_LEFTLEG:
			case HITGROUP_RIGHTLEG:
				current_damage *= 0.75f;
				break;
			default:
				break;
			}

			if (armorValue > 0 && IsArmored())
			{
				float bonusValue = 1.f, armorBonusRatio = 0.5f, armorRatio = weapon_info->flArmorRatio / 2.f;

				//Damage gets modified for heavy armor users
				if (hasHeavyArmor)
				{
					armorBonusRatio = 0.33f;
					armorRatio *= 0.5f;
					bonusValue = 0.33f;
				}

				auto NewDamage = current_damage * armorRatio;

				if (hasHeavyArmor)
					NewDamage *= 0.85f;

				if (((current_damage - (current_damage * armorRatio)) * (bonusValue * armorBonusRatio)) > armorValue)
					NewDamage = current_damage - (armorValue / armorBonusRatio);

				current_damage = NewDamage;
			}
		}


		bool TraceToExit2(Vector& end, trace_t& tr, Vector start, Vector vEnd, trace_t* trace)
		{
			typedef bool(__fastcall* TraceToExitFn)(Vector&, trace_t&, float, float, float, float, float, float, trace_t*);
			static DWORD TraceToExit = Utils::FindSignature("client_panorama.dll", "55 8B EC 83 EC 30 F3 0F 10 75");

			if (!TraceToExit)
				return false;

			float start_y = start.y, start_z = start.z, start_x = start.x, dir_y = vEnd.y, dir_x = vEnd.x, dir_z = vEnd.z;

			_asm
			{
				push trace
				push dir_z
				push dir_y
				push dir_x
				push start_z
				push start_y
				push start_x
				mov edx, tr
				mov ecx, end
				call TraceToExit
				add esp, 0x1C
			}
		}

		bool Autowall::TraceToExit(trace_t& enterTrace, trace_t& exitTrace, Vector startPosition, Vector direction)
		{
			/*
			Masks used:
			MASK_SHOT_HULL					 = 0x600400B
			CONTENTS_HITBOX					 = 0x40000000
			MASK_SHOT_HULL | CONTENTS_HITBOX = 0x4600400B
			*/

			Vector start, end;
			float maxDistance = 90.f, rayExtension = 4.f, currentDistance = 0;
			int firstContents = 0;

			while (currentDistance <= maxDistance)
			{
				//Add extra distance to our ray
				currentDistance += rayExtension;

				//Multiply the direction vector to the distance so we go outwards, add our position to it.
				start = startPosition + direction * currentDistance;

				if (!firstContents)
					firstContents = g_pTrace->GetPointContents(start, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr); /*0x4600400B*/
				int pointContents = g_pTrace->GetPointContents(start, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr);

				if (!(pointContents & MASK_SHOT_HULL) || pointContents & CONTENTS_HITBOX && pointContents != firstContents) /*0x600400B, *0x40000000*/
				{
					//Let's setup our end position by deducting the direction by the extra added distance
					end = start - (direction * rayExtension);

					//Let's cast a ray from our start pos to the end pos
					UTIL_TraceLine(start, end, MASK_SHOT_HULL | CONTENTS_HITBOX, nullptr, &exitTrace);

					//Let's check if a hitbox is in-front of our enemy and if they are behind of a solid wall
					if (exitTrace.startsolid && exitTrace.surface.flags & SURF_HITBOX)
					{
						UTIL_TraceLine(start, startPosition, MASK_SHOT_HULL, exitTrace.m_pEnt, &exitTrace);

						if (exitTrace.DidHit() && !exitTrace.startsolid)
						{
							start = exitTrace.endpos;
							return true;
						}

						continue;
					}

					//Can we hit? Is the wall solid?
					if (exitTrace.DidHit() && !exitTrace.startsolid)
					{
						//Is the wall a breakable? If so, let's shoot through it.
						if (Autowall::IsBreakableEntity(enterTrace.m_pEnt) && Autowall::IsBreakableEntity(exitTrace.m_pEnt))
							return true;

						if (enterTrace.surface.flags & SURF_NODRAW || !(exitTrace.surface.flags & SURF_NODRAW) && (exitTrace.plane.normal.Dot(direction) <= 1.f))
						{
							float multAmount = exitTrace.fraction * 4.f;
							start -= direction * multAmount;
							return true;
						}

						continue;
					}

					if (!exitTrace.DidHit() || exitTrace.startsolid)
					{
						if (enterTrace.DidHitNonWorldEntity2() && Autowall::IsBreakableEntity(enterTrace.m_pEnt))
						{
							exitTrace = enterTrace;
							exitTrace.endpos = start + direction;
							return true;
						}
					}
				}
			}
			return false;
		}

		bool Autowall::HandleBulletPenetration2(WeaponInfo_t *wpn_data, Autowall_Info &data)
		{
			bool bSolidSurf = ((data.enter_trace.contents >> 3) & CONTENTS_SOLID);
			bool bLightSurf = (data.enter_trace.surface.flags >> 7) & SURF_LIGHT;

			surfacedata_t *enter_surface_data = g_pPhysProps->GetSurfaceData(data.enter_trace.surface.surfaceProps);
			unsigned short enter_material = enter_surface_data->game.material;
			float enter_surf_penetration_mod = enter_surface_data->game.flPenetrationModifier;

			if (!data.penetration_count && !bLightSurf && !bSolidSurf && enter_material != 89)
			{
				if (enter_material != 71)
					return false;
			}

			if (data.penetration_count <= 0 || wpn_data->flPenetration <= 0.f)
				return false;

			Vector dummy;
			trace_t trace_exit;

			if (!TraceToExit2(dummy, data.enter_trace, data.enter_trace.endpos, data.direction, &trace_exit))
			{
				if (!(g_pTrace->GetPointContents(dummy, MASK_SHOT_HULL) & MASK_SHOT_HULL))
					return false;
			}

			surfacedata_t *exit_surface_data = g_pPhysProps->GetSurfaceData(trace_exit.surface.surfaceProps);
			unsigned short exit_material = exit_surface_data->game.material;

			float exit_surf_penetration_mod = exit_surface_data->game.flPenetrationModifier;
			float exit_surf_damage_mod = exit_surface_data->game.flDamageModifier;

			float final_damage_modifier = 0.16f;
			float combined_penetration_modifier = 0.0f;

			if (enter_material == 89 || enter_material == 71)
			{
				combined_penetration_modifier = 3.0f;
				final_damage_modifier = 0.05f;
			}
			else if (bSolidSurf || bLightSurf)
			{
				combined_penetration_modifier = 1.0f;
				final_damage_modifier = 0.16f;
			}
			else
			{
				combined_penetration_modifier = (enter_surf_penetration_mod + exit_surf_penetration_mod) * 0.5f;
			}

			if (enter_material == exit_material)
			{
				if (exit_material == 87 || exit_material == 85)
					combined_penetration_modifier = 3.0f;
				else if (exit_material == 76)
					combined_penetration_modifier = 2.0f;
			}

			float modifier = fmaxf(0.0f, 1.0f / combined_penetration_modifier);
			float thickness = (trace_exit.endpos - data.enter_trace.endpos).LengthSqr();
			float taken_damage = ((modifier * 3.0f) * fmaxf(0.0f, (3.0f / wpn_data->flPenetration) * 1.25f) + (data.current_damage * final_damage_modifier)) + ((thickness * modifier) / 24.0f);

			float lost_damage = fmaxf(0.0f, taken_damage);

			if (lost_damage > data.current_damage)
				return false;

			if (lost_damage > 0.0f)
				data.current_damage -= lost_damage;

			if (data.current_damage < 1.0f)
				return false;

			data.start = trace_exit.endpos;
			data.penetration_count--;

			return true;
		}


		bool Autowall::HandleBulletPenetration(WeaponInfo_t* weaponData, trace_t& enterTrace, Vector& eyePosition, Vector direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, bool sv_penetration_type, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration)
		{
			//Because there's been issues regarding this- putting this here.
			if (&currentDamage == nullptr)
				throw std::invalid_argument("currentDamage is null!");

			auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
			if (!local_player)
				return false;

			trace_t exitTrace;
			C_BaseEntity* pEnemy = enterTrace.m_pEnt;
			surfacedata_t* enterSurfaceData = g_pPhysProps->GetSurfaceData(enterTrace.surface.surfaceProps);
			int enterMaterial = enterSurfaceData->game.material;

			float enterSurfPenetrationModifier = enterSurfaceData->game.flPenetrationModifier;
			float enterDamageModifier = enterSurfaceData->game.flDamageModifier;
			float thickness, modifier, lostDamage, finalDamageModifier, combinedPenetrationModifier;
			bool isSolidSurf = ((enterTrace.contents >> 3) & CONTENTS_SOLID);
			bool isLightSurf = ((enterTrace.surface.flags >> 7) & SURF_LIGHT);

			if (possibleHitsRemaining <= 0
				|| (!possibleHitsRemaining && !isLightSurf && !isSolidSurf && enterMaterial != CHAR_TEX_GRATE && enterMaterial != CHAR_TEX_GLASS)
				|| weaponData->flPenetration <= 0.f
				|| !TraceToExit(enterTrace, exitTrace, enterTrace.endpos, direction)
				&& !(g_pTrace->GetPointContents(enterTrace.endpos, MASK_SHOT_HULL, nullptr) & MASK_SHOT_HULL))
				return false;

			surfacedata_t* exitSurfaceData = g_pPhysProps->GetSurfaceData(exitTrace.surface.surfaceProps);
			int exitMaterial = exitSurfaceData->game.material;
			float exitSurfPenetrationModifier = exitSurfaceData->game.flPenetrationModifier;
			float exitDamageModifier = exitSurfaceData->game.flDamageModifier;

			//Are we using the newer penetration system?
			if (sv_penetration_type)
			{
				if (enterMaterial == CHAR_TEX_GRATE || enterMaterial == CHAR_TEX_GLASS)
				{
					combinedPenetrationModifier = 3.f;
					finalDamageModifier = 0.05f;
				}
				else if (isSolidSurf || isLightSurf)
				{
					combinedPenetrationModifier = 1.f;
					finalDamageModifier = 0.16f;
				}
				else if (enterMaterial == CHAR_TEX_FLESH && (local_player->GetTeam() == pEnemy->GetTeam() && ff_damage_reduction_bullets == 0.f)) //TODO: Team check config
				{
					//Look's like you aren't shooting through your teammate today
					if (ff_damage_bullet_penetration == 0.f)
						return false;

					//Let's shoot through teammates and get kicked for teamdmg! Whatever, atleast we did damage to the enemy. I call that a win.
					combinedPenetrationModifier = ff_damage_bullet_penetration;
					finalDamageModifier = 0.16f;
				}
				else
				{
					combinedPenetrationModifier = (enterSurfPenetrationModifier + exitSurfPenetrationModifier) / 2.f;
					finalDamageModifier = 0.16f;
				}

				//Do our materials line up?
				if (enterMaterial == exitMaterial)
				{
					if (exitMaterial == CHAR_TEX_CARDBOARD || exitMaterial == CHAR_TEX_WOOD)
						combinedPenetrationModifier = 3.f;
					else if (exitMaterial == CHAR_TEX_PLASTIC)
						combinedPenetrationModifier = 2.f;
				}

				//Calculate thickness of the wall by getting the length of the range of the trace and squaring
				thickness = (exitTrace.endpos - enterTrace.endpos).LengthSqr();
				modifier = fmaxf(1.f / combinedPenetrationModifier, 0.f);

				//This calculates how much damage we've lost depending on thickness of the wall, our penetration, damage, and the modifiers set earlier
				lostDamage = fmaxf(
					((modifier * thickness) / 24.f)
					+ ((currentDamage * finalDamageModifier)
						+ (fmaxf(3.75f / penetrationPower, 0.f) * 3.f * modifier)), 0.f);

				//Did we loose too much damage?
				if (lostDamage > currentDamage)
					return false;

				//We can't use any of the damage that we've lost
				if (lostDamage > 0.f)
					currentDamage -= lostDamage;

				//Do we still have enough damage to deal?
				if (currentDamage < 1.f)
					return false;

				eyePosition = exitTrace.endpos;
				--possibleHitsRemaining;

				return true;
			}
			else //Legacy penetration system
			{
				combinedPenetrationModifier = 1.f;

				if (isSolidSurf || isLightSurf)
					finalDamageModifier = 0.99f; //Good meme :^)
				else
				{
					finalDamageModifier = fminf(enterDamageModifier, exitDamageModifier);
					combinedPenetrationModifier = fminf(enterSurfPenetrationModifier, exitSurfPenetrationModifier);
				}

				if (enterMaterial == exitMaterial && (exitMaterial == CHAR_TEX_METAL || exitMaterial == CHAR_TEX_WOOD))
					combinedPenetrationModifier += combinedPenetrationModifier;

				thickness = (exitTrace.endpos - enterTrace.endpos).LengthSqr();

				if (sqrt(thickness) <= combinedPenetrationModifier * penetrationPower)
				{
					currentDamage *= finalDamageModifier;
					eyePosition = exitTrace.endpos;
					--possibleHitsRemaining;

					return true;
				}

				return false;
			}
		}
	}
}