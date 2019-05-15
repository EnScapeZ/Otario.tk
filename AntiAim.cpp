#include "AntiAim.h"
#include "Aimbot.h"


bool IsWeaponGrenade4(C_BaseCombatWeapon* weapon)
{
	if (weapon == nullptr) return false;
	int id = weapon->sGetItemDefinitionIndex();
	static const std::vector<int> v = { WEAPON_FLASHBANG,WEAPON_HEGRENADE,WEAPON_SMOKEGRENADE,WEAPON_MOLOTOV,WEAPON_DECOY,WEAPON_INCGRENADE };
	return (std::find(v.begin(), v.end(), id) != v.end());
}

bool IsBallisticWeapon4(void* weapon)
{
	if (weapon == nullptr) return false;
	C_BaseEntity* weaponEnt = (C_BaseEntity*)weapon;
	ClientClass* pWeaponClass = weaponEnt->GetClientClass();

	if (pWeaponClass->m_ClassID == (int)EClassIds::CKnife || pWeaponClass->m_ClassID == (int)EClassIds::CHEGrenade ||
		pWeaponClass->m_ClassID == (int)EClassIds::CDecoyGrenade || pWeaponClass->m_ClassID == (int)EClassIds::CIncendiaryGrenade ||
		pWeaponClass->m_ClassID == (int)EClassIds::CSmokeGrenade || pWeaponClass->m_ClassID == (int)EClassIds::CC4 ||
		pWeaponClass->m_ClassID == (int)EClassIds::CMolotovGrenade || pWeaponClass->m_ClassID == (int)EClassIds::CFlashbang)
		return false;
	else
		return true;
}


void NormalizeNum(Vector &vIn, Vector &vOut)
{
	float flLen = vIn.Length();
	if (flLen == 0) {
		vOut.Init(0, 0, 1);
		return;
	}
	flLen = 1 / flLen;
	vOut.Init(vIn.x * flLen, vIn.y * flLen, vIn.z * flLen);
}

float fov_player(Vector ViewOffSet, Vector View, C_BaseEntity* entity, int hitbox)
{
	const float MaxDegrees = 180.0f;
	Vector Angles = View, Origin = ViewOffSet;
	Vector Delta(0, 0, 0), Forward(0, 0, 0);
	Vector AimPos = aimbot->GetHitboxPosition(entity, hitbox);

	Utils::AngleVectors(Angles, &Forward);
	VectorSubtract(AimPos, Origin, Delta);
	NormalizeNum(Delta, Delta);

	float DotProduct = Forward.Dot(Delta);
	return (acos(DotProduct) * (MaxDegrees / M_PI));
}

int closest_to_crosshair()
{
	int index = -1;
	float lowest_fov = INT_MAX;

	C_BaseEntity* local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	if (!local_player)
		return -1;

	Vector local_position = local_player->GetEyeOrigin();

	Vector angles;
	g_pEngine->GetViewAngles(angles);

	for (int i = 1; i <= g_pGlobalVars->maxClients; i++)
	{
		C_BaseEntity *entity = g_pEntityList->GetClientEntity(i);

		if (!entity || entity->GetHealth() <= 0 || entity->GetTeam() == local_player->GetTeam() || entity->IsDormant() || entity == local_player)
			continue;

		float fov = fov_player(local_position, angles, entity, 0);

		if (fov < lowest_fov)
		{
			lowest_fov = fov;
			index = i;
		}
	}

	return index;
}

float get_curtime(CUserCmd* ucmd) 
{
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
	if (!local_player)
		return 0;

	int g_tick = 0;
	CUserCmd* g_pLastCmd = nullptr;
	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = (float)local_player->GetTickBase();
	}
	else {
		++g_tick;
	}
	g_pLastCmd = ucmd;
	float curtime = g_tick * g_pGlobalVars->intervalPerTick;
	return curtime;
}


bool next_lby_update(CUserCmd* cmd)
{
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
	if (!local_player)
		return false;

	static float next_lby_update_time = 0;
	float curtime = get_curtime(cmd);

	auto animstate = local_player->GetAnimState();
	if (!animstate)
		return false;

	auto net_channel = g_pEngine->GetNetChannel();

	if (!net_channel || net_channel->m_nChokedPackets)
		return false;

	if (!(local_player->GetFlags() & FL_ONGROUND))
		return false;

	if (animstate->speed_2d > 0.1)
		next_lby_update_time = curtime + 0.22f;

	if (next_lby_update_time < curtime)
	{
		next_lby_update_time = curtime + 1.1f;
		return true;
	}
	return false;
}

void autoDirection(CUserCmd* cmd)
{
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	if (!local_player)
		return;

	static bool shark = false;
	shark = !shark;
	static float last_real;
	bool no_active = true;
	int hold = 0;
	float bestrotation = 0.f;
	float highestthickness = 0.f;
	Vector besthead;

	auto leyepos = local_player->GetEyeOrigin();
	auto headpos = aimbot->GetHitboxPosition(local_player, 0);
	auto origin = local_player->GetAbsOrigin();

	auto checkWallThickness = [&](C_BaseEntity* pPlayer, Vector newhead) -> float
	{
		Vector endpos1, endpos2;
		Vector eyepos = pPlayer->GetEyePosition();
		Ray_t ray;
		ray.Init(newhead, eyepos);
		CTraceFilterSkipTwoEntities filter(pPlayer, g::pLocalEntity);
		trace_t trace1, trace2;
		g_pTrace->TraceRay(ray, MASK_SHOT_BRUSHONLY, &filter, &trace1);

		if (trace1.DidHit())
			endpos1 = trace1.endpos;
		else
			return 0.f;

		ray.Init(eyepos, newhead);
		g_pTrace->TraceRay(ray, MASK_SHOT_BRUSHONLY, &filter, &trace2);

		if (trace2.DidHit())
			endpos2 = trace2.endpos;

		float add = newhead.DistTo(eyepos) - leyepos.DistTo(eyepos) + 3.f;
		return endpos1.DistTo(endpos2) + add / 3;
	};

	int index = closest_to_crosshair();
	auto entity = g_pEntityList->GetClientEntity(index);

	if (!local_player->IsAlive())
		hold = 0.f;

	if (entity == nullptr)
	{
		if (hold != 0.f)
			g::pCmd->viewangles.y = hold;
		else
		{
			g::pCmd->viewangles.y -= 180.f;
			return;
		}
	}

	float step = (2 * M_PI) / 18.f;
	float radius = Vector(headpos - origin).Length2D();

	if (index == -1)
	{
		no_active = true;
	}
	else
	{
		for (float besthead = 0; besthead < 6; besthead += 0.1)
		{
			Vector newhead(radius * cos(besthead) + leyepos.x, radius * sin(besthead) + leyepos.y, leyepos.z);
			float totalthickness = 0.f;
			no_active = false;
			totalthickness += checkWallThickness(entity, newhead);
			if (totalthickness > highestthickness)
			{
				highestthickness = totalthickness;

				hold = bestrotation;

				bestrotation = besthead;
			}
		}
	}
	if (no_active)
	{
		g::pCmd->viewangles.y -= 180.f;
	}
	else if(bestrotation > 0)
		cmd->viewangles.y = RAD2DEG(bestrotation);
	else if(bestrotation <= 0)
	{
		cmd->viewangles.y += shark ? 175 : -175;
	}
}

float MaxDesyncDelta()
{
	auto animstate = uintptr_t(g::pLocalEntity->GetAnimState());

	float duckammount = *(float *)(animstate + 0xA4);
	float speedfraction = max(0, min(*reinterpret_cast<float*>(animstate + 0xF8), 1));

	float speedfactor = max(0, min(1, *reinterpret_cast<float*> (animstate + 0xFC)));

	float unk1 = ((*reinterpret_cast<float*> (animstate + 0x11C) * -0.30000001) - 0.19999999) * speedfraction;
	float unk2 = unk1 + 1.f;
	float unk3;

	if (duckammount > 0) {

		unk2 += ((duckammount * speedfactor) * (0.5f - unk2));
	}

	unk3 = *(float *)(animstate + 0x334) * unk2;

	return unk3;
}


float DesyncClamp(float Yaw)
{
	if (Yaw > 180)
	{
		Yaw -= (round(Yaw / 360) * 360.f);
	}
	else if (Yaw < -180)
	{
		Yaw += (round(Yaw / 360) * -360.f);
	}
	return Yaw;
}


void aimAtPlayer(CUserCmd *pCmd)
{
	if (!g_Settings.bYawAtTargets)
		return;

	if (!g_Settings.iYaw == 1)
		return;

	C_BaseCombatWeapon* pWeapon = g::pLocalEntity->GetActiveWeapon();

	if (!g::pLocalEntity || !pWeapon)
		return;

	Vector eye_position = g::pLocalEntity->GetEyeOrigin();

	float best_dist = pWeapon->GetCSWpnData()->flRange;

	C_BaseEntity* entity = nullptr;

	for (int i = 0; i < g_pEngine->GetMaxClients(); i++)
	{
		C_BaseEntity *pEntity = g_pEntityList->GetClientEntity(i);
		if (aimbot->TargetMeetsRequirements(pEntity))
		{
			int index = closest_to_crosshair();
			entity = g_pEntityList->GetClientEntity(index);

			Vector target_position = entity->GetEyeOrigin();

			Utils::CalcAngle(eye_position, target_position, pCmd->viewangles);
		}
	}
}


void Antiaim::Do(CUserCmd * pCmd)
{
	if (!g_Settings.bAntiAim)
		return;

	if (g_Settings.iPitch > 0 || g_Settings.iYaw > 0)
	{
		static QAngle angles;

		auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
		if (!local_player)
			return;

		if (!local_player->IsAlive())
			return;

		if (g::pLocalEntity->GetFlags() & FL_ATCONTROLS)
			return;

		C_BaseCombatWeapon* weapon = g::pLocalEntity->GetActiveWeapon();

		if (!weapon)
			return;

		if ((pCmd->buttons & IN_USE && !local_player->IsDefusing()) || local_player->GetMoveType() == MOVETYPE_LADDER && local_player->GetVelocity().Length2D() > 0 || local_player->GetMoveType() == MOVETYPE_NOCLIP)
			return;

		if (Globals::dontdoaa)
			return;

		if (IsWeaponGrenade4(weapon))
		{
			if (!weapon->IsPinPulled() || (pCmd->buttons & IN_ATTACK) || (pCmd->buttons & IN_ATTACK2))
			{
				float throwTime = weapon->GetThrowTime();

				if (throwTime > 0)
					return;
			}
		}
		else if (pCmd->buttons & IN_ATTACK && aimbot->can_shoot(pCmd) && !weapon->GetAmmo() == 0)
		{
			if (weapon->GetItemDefinitionIndex() != ItemDefinitionIndex::WEAPON_C4)
				return;
		}
		else if (pCmd->buttons & IN_ATTACK2 && aimbot->can_shoot(pCmd) && !IsBallisticWeapon4(weapon))
		{
			if (weapon->GetItemDefinitionIndex() != ItemDefinitionIndex::WEAPON_C4)
				return;
		}

		aimAtPlayer(pCmd);

		//Utils::Log("desync max: %", maxDesyncDelta);

		// Anti-Aim Pitch
		switch (g_Settings.iPitch)
		{
		case 0:
			break;
		case 1:
			pCmd->viewangles.x = 89.f;
			break;
		case 2:
			g_Settings.AntiUT ? pCmd->viewangles.x = 89.f : pCmd->viewangles.x = -180.f;
			break;
		case 3:
			g_Settings.AntiUT ? pCmd->viewangles.x = -89.f : pCmd->viewangles.x = 180.f;
			break;
		case 4:
			g_Settings.AntiUT ? pCmd->viewangles.x = 0.f : pCmd->viewangles.x = -180540.f;
			break;
		case 5:
			g_Settings.AntiUT ? pCmd->viewangles.x = rand() % 89 : pCmd->viewangles.x = rand() % 180000;
			break;
		}	

		float maxDesyncDelta = MaxDesyncDelta();

		switch (g_Settings.iYaw)
		{
		case 0:
			break;
		case 1:
		//	If command_number % 3 isn't true you set your yaw to the desired angle. - your real angle
		//	If command_number % 3 is true you can run your desync you can run the following two steps :

		//	If your YawFeetDelta is less than MaxRotation you can add 180 to your yaw
		//		Otherwise you must add or subtract MaxRotation doesn't really matter.

		//		- its up to you to calculate these things.
		//		hint : SetupVelocity @ server.dll

			pCmd->viewangles.y += 180.f;

			if (pCmd->command_number % 3)
			{
				pCmd->viewangles.y -= maxDesyncDelta;
				
				if (next_lby_update(pCmd))
					pCmd->viewangles.y -= 180.f;
			}
			break;
		case 2:
				autoDirection(pCmd);
			break;
		}
	}	
}


void Antiaim::Desync(CUserCmd* pCmd)
{
}

Antiaim* antiaim = new Antiaim();