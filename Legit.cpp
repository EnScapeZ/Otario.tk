#include "Legit.h"


void CLegitBot::Move(CUserCmd * pCmd)
{
	Entities.clear();
	SelectTarget();
	DoAimbot(pCmd);
}

void CLegitBot::SelectTarget()
{
	if (!g::pLocalEntity)
		return;

	for (int index = 1; index <= g_pGlobalVars->maxClients; index++)
	{
		C_BaseEntity* entity = g_pEntityList->GetClientEntity(index);

		if (!entity)
			continue;

		bool is_local_player = entity == g::pLocalEntity;
		bool is_teammate = g::pLocalEntity->GetTeam() == entity->GetTeam() && !is_local_player;

		if (is_local_player)
			continue;

		if (is_teammate)
			continue;

		if (entity->GetHealth() <= 0)
			continue;

		if (!entity->IsAlive())
			continue;

		auto class_id = entity->GetClientClass()->m_ClassID;

		if (class_id != 40)
			continue;

		if (entity->GetVecOrigin() == Vector(0, 0, 0))
			continue;

		if (entity->IsImmune())
			continue;

		if (entity->IsDormant())
			continue;

		LAimbotData_t data = LAimbotData_t(entity, index);

		Entities.push_back(data);
	}
}

float CLegitBot::FovToPlayer(Vector ViewOffSet, Vector View, C_BaseEntity* pEntity, int aHitBox)
{
	Vector out[9];

	// Anything past 180 degrees is just going to wrap around
	CONST FLOAT MaxDegrees = 180.0f;

	// Get local angles
	Vector Angles = View;

	// Get local view / eye position
	Vector Origin = ViewOffSet;

	// Create and intiialize vectors for calculations below
	Vector Delta(0, 0, 0);
	//Vector Origin(0, 0, 0);
	Vector Forward(0, 0, 0);

	// Convert angles to normalized directional forward vector
	Utils::AngleVectors(Angles, &Forward);
	Vector AimPos = aimbot->GetHitboxPosition(pEntity, aHitBox);
	// Get delta vector between our local eye position and passed vector
	VectorSubtract(AimPos, Origin, Delta);
	//Delta = AimPos - Origin;

	// Normalize our delta vector
	Utils::Normalize(Delta, Delta);

	// Get dot product between delta position and directional forward vectors
	FLOAT DotProduct = Forward.Dot(Delta);

	// Time to calculate the field of view
	return (acos(DotProduct) * (MaxDegrees / PI));
}

bool IsAuto(C_BaseCombatWeapon* pWeapon)
{
	if (pWeapon == nullptr) return false;
	int id = pWeapon->sGetItemDefinitionIndex();
	static const std::vector<int> v = { WEAPON_SCAR20, WEAPON_G3SG1 };
	return (std::find(v.begin(), v.end(), id) != v.end());
}

bool IsRifle(C_BaseCombatWeapon* pWeapon)
{
	if (pWeapon == nullptr) return false;
	int id = pWeapon->sGetItemDefinitionIndex();
	static const std::vector<int> v = { WEAPON_FAMAS, WEAPON_GALILAR, WEAPON_M4A1, WEAPON_M4A1_SILENCER, WEAPON_AK47, WEAPON_AUG, WEAPON_SG556 };
	return (std::find(v.begin(), v.end(), id) != v.end());
}

bool IsSmg(C_BaseCombatWeapon* pWeapon)
{
	if (pWeapon == nullptr) return false;
	int id = pWeapon->sGetItemDefinitionIndex();
	static const std::vector<int> v = { WEAPON_MP9, WEAPON_MAC10, WEAPON_BIZON, WEAPON_MP7, WEAPON_UMP45, WEAPON_P90, WEAPON_MP5SD };
	return (std::find(v.begin(), v.end(), id) != v.end());
}

bool IsHeavy(C_BaseCombatWeapon* pWeapon)
{
	if (pWeapon == nullptr) return false;
	int id = pWeapon->sGetItemDefinitionIndex();
	static const std::vector<int> v = { WEAPON_NOVA, WEAPON_XM1014, WEAPON_MAG7, WEAPON_SAWEDOFF, WEAPON_M249, WEAPON_NEGEV };
	return (std::find(v.begin(), v.end(), id) != v.end());
}

bool IsPistol(C_BaseCombatWeapon* pWeapon)
{
	if (pWeapon == nullptr) return false;
	int id = pWeapon->sGetItemDefinitionIndex();
	static const std::vector<int> v = { WEAPON_HKP2000, WEAPON_USP_SILENCER, WEAPON_GLOCK, WEAPON_P250, WEAPON_FIVESEVEN, WEAPON_TEC9, WEAPON_CZ75A, WEAPON_ELITE, WEAPON_DEAGLE, WEAPON_REVOLVER, WEAPON_TASER };
	return (std::find(v.begin(), v.end(), id) != v.end());
}

float bestFov = FLT_MAX;
int aimtick = -1;

bool IsVisiblePoint(C_BaseEntity* pLocal, Vector Point)
{

	Vector start = pLocal->GetEyeOrigin();
	trace_t Trace;
	autowall->UTIL_TraceLine(start, Point, MASK_SOLID, pLocal, &Trace);

	if (Trace.fraction > 0.9f)  //i know this is bad af....
	{
		return true;
	}

	return false;
}

void CLegitBot::DoAimbot(CUserCmd *pCmd)
{
	if (!g::pLocalEntity)
		return;

	if (g::pLocalEntity->GetHealth() <= 0)
		return;

	if (g::pLocalEntity->GetFlags() & FL_ATCONTROLS)
		return;

	C_BaseCombatWeapon* pWeapon = g::pLocalEntity->GetActiveWeapon();
	if (!pWeapon)
		return;

	if (Globals::dontdoaa)
		return;

	if (aimbot->IsKnife31(pWeapon) || aimbot->IsWeaponGrenade2(pWeapon) || pWeapon->GetAmmo() == 0 || !aimbot->IsBallisticWeapon(pWeapon))
	{
		Globals::shouldstop = false;
		return;
	}

	if (!aimbot->can_shoot(pCmd))
	{
		Globals::shouldstop = false;
		return;
	}

	for (auto players : Entities)
	{
		auto entity = players.pPlayer;

		if (!entity)
			continue;

		bool is_local_player = entity == g::pLocalEntity;
		bool is_teammate = g::pLocalEntity->GetTeam() == entity->GetTeam() && !is_local_player;

		if (is_local_player)
			continue;

		if (is_teammate)
			continue;

		if (entity->GetHealth() <= 0)
			continue;

		if (!entity->IsAlive())
			continue;

		auto class_id = entity->GetClientClass()->m_ClassID;

		if (class_id != 40)
			continue;

		if (entity->GetVecOrigin() == Vector(0, 0, 0))
			continue;

		if (entity->IsImmune())
			continue;

		if (entity->IsDormant())
			continue;

		Vector ViewOffset = g::pLocalEntity->GetEyeOrigin();
		Vector View; g_pEngine->GetViewAngles(View);
		View += g::pLocalEntity->GetPunchAngles() * 2.f;

		float currentfov = FovToPlayer(ViewOffset, View, entity, 0);
		if (currentfov > g_Settings.flFovLegit)
			continue;

		int hitbox;

		/*switch (g_Settings.iWeaponType)
		{
		case 0:
			if (pWeapon->GetItemDefinitionIndex() != ItemDefinitionIndex::WEAPON_AWP)
				continue;

			break;
		case 1:
			if (pWeapon->GetItemDefinitionIndex() != ItemDefinitionIndex::WEAPON_SSG08)
				continue;

			break;
		case 2:
			if (!IsAuto(pWeapon))
				continue;

			break;
		case 3:
			if (!IsRifle(pWeapon))
				continue;

			break;
		case 4:
			if (!IsHeavy(pWeapon))
				continue;

			break;
		case 5:
			if (IsPistol(pWeapon))
				continue;

		}*/

		
		
		switch (g_Settings.iLegitHitBox)
		{
		case 0:
			hitbox = HitboxList::Head;
			break;
		case 1:
			hitbox = HitboxList::Neck;
			break;
		case 2:
			hitbox = HitboxList::Stomach;
			break;
		case 3:
			hitbox = HitboxList::Chest;
			break;
		case 4:
			hitbox = HitboxList::Pelvis;
			break;
		}
		
		Vector shoot_here = aimbot->GetHitboxPosition(entity, hitbox);

		Vector AimPoint;
		
		Vector AimAngle;
	
		AimAngle = Utils::NormalizeAngle(Utils::CalcAngle(g::pLocalEntity->GetEyeOrigin(), entity->GetPredicted(shoot_here)));
		
		if (AimAngle == Vector(0, 0, 0))
			continue;

		if (shoot_here == Vector(0, 0, 0))
			continue;

		if (AimPoint == Vector(0, 0, 0))
			continue;

		if (g_Settings.bLegitAutoFire)
			pCmd->buttons |= IN_ATTACK;

		if (pCmd->buttons & IN_ATTACK && g_Settings.bLegitOnShot)
		{
			if (!g_Settings.bLegitSilentAim)
			{
				g_pEngine->SetViewAngles(AimAngle);
				pCmd->viewangles = AimAngle;
			}
			else
				pCmd->viewangles = AimAngle;
		}
		else if (!g_Settings.bLegitOnShot)
		{
			if (!g_Settings.bLegitSilentAim)
			{
				g_pEngine->SetViewAngles(AimAngle);
				pCmd->viewangles = AimAngle;
			}
			else
				pCmd->viewangles = AimAngle;
		}
		
		if (g_Settings.bLegitNoRecoil)
			pCmd->viewangles = AimAngle - (g::pLocalEntity->GetPunchAngles() * 2.f);
	}
}

CLegitBot* laimbot = new CLegitBot();