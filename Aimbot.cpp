#include "Aimbot.h"

void CAimbot::Init()
{
	IsLocked = false;
	TargetID = -1;
}

C_BaseEntity* entCopy;
trace_t Trace;

void CAimbot::Move(CUserCmd * pCmd)
{
	Entities.clear();
	SelectTarget();
	DoAimbot(pCmd);
}

void CAimbot::SelectTarget()
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

		AimbotData_t data = AimbotData_t(entity, index);

		Entities.push_back(data);
	}
}
Vector BestPoint(C_BaseEntity *targetPlayer, Vector &final)
{
	trace_t tr;
	Ray_t ray;
	CTraceFilter filter;

	filter.pSkip = targetPlayer;
	ray.Init(final + Vector(0, 0, 10), final);
	g_pTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);

	final = tr.endpos;
	return final;

}

mstudiobbox_t* CAimbot::GetHitbox(C_BaseEntity* entity, int hitbox_index)
{
	if (entity->IsDormant() || entity->GetHealth() <= 0)
		return nullptr;

	const auto pModel = entity->GetModel();
	if (!pModel)
		return nullptr;

	auto pStudioHdr = g_pModelInfo->GetStudiomodel(pModel);
	if (!pStudioHdr)
		return nullptr;

	auto pSet = pStudioHdr->pHitboxSet(0);
	if (!pSet)
		return nullptr;

	if (hitbox_index >= pSet->numhitboxes || hitbox_index < 0)
		return nullptr;

	return pSet->GetHitbox(hitbox_index);
}

Vector MultipointVectors[] = 
{
	Vector(0 ,0 ,1 ), Vector(0 ,0 ,2 ), Vector(0 ,0, 3), Vector(0 ,0 ,4 ), 
	Vector(0 ,0 ,5 ), Vector(0,0,6) , Vector(0,0,7) , Vector(0,0,8),
	Vector(0,0, 9), Vector(0,0,10)
};

bool CAimbot::IsVisible(C_BaseEntity* pLocal, C_BaseEntity* pEntity, int BoneID)
{
	if (BoneID < 0) return false;

	entCopy = pEntity;
	Vector start = pLocal->GetEyeOrigin();
	Vector end = aimbot->GetHitboxPosition(pEntity, BoneID);//pEntity->GetBonePos(BoneID);

	Ray_t ray;
	trace_t tr;
	ray.Init(start, end);
	CTraceFilterSkipTwoEntities filter(pLocal, pEntity);

	g_pTrace->TraceRay(ray, MASK_SOLID, &filter, &Trace);

	if (Trace.m_pEnt == entCopy)
	{
		return true;
	}

	if (Trace.fraction == 1.0f)
	{
		return true;
	}
	return false;
}

Vector MultipointVectors2[] = { Vector(0,0,0), Vector(0,0,1.5), Vector(0,0,3), Vector(0,0,4), Vector(0,0,-2), Vector(0,0,-4), Vector(0,0,4.8), Vector(0,0,5), Vector(0,0,5.4), Vector(0,3,0), Vector(3,0,0), Vector(-3,0,0),Vector(0,-3,0), Vector(0,2,4.2), Vector(0,-2,4.2), Vector(2,0,4.2), Vector(-2,0,4.2),  Vector(3.8,0,0), Vector(-3.8,0,0),Vector(0,3.6,0), Vector(0,-3.6,0),  Vector(0,1.2,3.2), Vector(0,0.6,1.4), Vector(0,3.1,5.1), Vector(0,0,6.2), Vector(0,2.5,0), Vector(2.1,2.1,2.1) };

Vector CAimbot::GetHitboxPosition(C_BaseEntity* pEntity, int Hitbox)
{
	auto hitbox = GetHitbox(pEntity, Hitbox);
	if (!hitbox)
		return Vector(0, 0, 0);

	auto bone_matrix = pEntity->GetBoneMatrix(hitbox->bone);

	Vector bbmin, bbmax, vCenter;
	Utils::VectorTransform(hitbox->bbmin, bone_matrix, bbmin);
	Utils::VectorTransform(hitbox->bbmax, bone_matrix, bbmax);

	vCenter = (bbmin + bbmax) * 0.5f;

	return vCenter;
}

bool  CAimbot::IsBallisticWeapon(void* weapon)
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

bool IsPistol(void* weapon)
{
	if (weapon == nullptr) return false;
	C_BaseEntity* weaponEnt = (C_BaseEntity*)weapon;
	ClientClass* pWeaponClass = weaponEnt->GetClientClass();

	if (pWeaponClass->m_ClassID == (int)EClassIds::CDEagle || pWeaponClass->m_ClassID == (int)EClassIds::CWeaponElite || pWeaponClass->m_ClassID == (int)EClassIds::CWeaponFiveSeven || pWeaponClass->m_ClassID == (int)EClassIds::CWeaponGlock || pWeaponClass->m_ClassID == (int)EClassIds::CWeaponHKP2000 || pWeaponClass->m_ClassID == (int)EClassIds::CWeaponP250 || pWeaponClass->m_ClassID == (int)EClassIds::CWeaponP228 || pWeaponClass->m_ClassID == (int)EClassIds::CWeaponTec9 || pWeaponClass->m_ClassID == (int)EClassIds::CWeaponUSP)
		return true;
	else
		return false;
}

bool IsSniper(void* weapon)
{
	if (weapon == nullptr) return false;
	C_BaseEntity* weaponEnt = (C_BaseEntity*)weapon;
	ClientClass* pWeaponClass = weaponEnt->GetClientClass();

	if (pWeaponClass->m_ClassID == (int)EClassIds::CWeaponAWP || pWeaponClass->m_ClassID == (int)EClassIds::CWeaponSSG08 || pWeaponClass->m_ClassID == (int)EClassIds::CWeaponSCAR20 || pWeaponClass->m_ClassID == (int)EClassIds::CWeaponG3SG1)
		return true;
	else
		return false;
}


void Normalize(Vector &vIn, Vector &vOut)
{
	float flLen = vIn.Length();
	if (flLen == 0) {
		vOut.Init(0, 0, 1);
		return;
	}
	flLen = 1 / flLen;
	vOut.Init(vIn.x * flLen, vIn.y * flLen, vIn.z * flLen);
}

void CAimbot::minspeed(C_BaseEntity* local, CUserCmd* cmd)
{
	C_BaseCombatWeapon* weapon = local->GetActiveWeapon();
	if (!weapon)
		return;

	Vector velocity = local->GetVelocity();
	Vector direction;

	Utils::VectorAngles(velocity, direction);

	float speed = velocity.Length2D();

	direction.y = cmd->viewangles.y - direction.y;

	Vector forward;

	Utils::AngleVectors(direction, &forward);

	Vector source = forward * -speed;

	float maxspeed = weapon->GetCSWpnData()->flMaxPlayerSpeed;
	float maxspeedalt = weapon->GetCSWpnData()->flMaxPlayerSpeedAlt;

	if (maxspeed = maxspeedalt)
	{
		if (speed >= (maxspeed * .34))
		{
			cmd->forwardmove = source.x;
			cmd->sidemove = source.y;
		}
	}
	else
	{
		if (speed >= (maxspeedalt * .34))
		{
			cmd->forwardmove = source.x;
			cmd->sidemove = source.y;
		}
	}
}

void CAimbot::stopmovement(C_BaseEntity* local_player, CUserCmd* cmd)
{
	Vector velocity = local_player->GetVelocity();
	Vector direction;

	Utils::VectorAngles(velocity, direction);

	float speed = velocity.Length2D();

	direction.y = cmd->viewangles.y - direction.y;

	Vector forward;

	Utils::AngleVectors(direction, &forward);

	Vector source = forward * -speed;

	cmd->forwardmove = source.x;
	cmd->sidemove = source.y;
}

bool CAimbot::TargetMeetsRequirements(C_BaseEntity* pEntity)
{
	// Is a valid player
	if (pEntity && pEntity->IsDormant() == false && pEntity->IsAlive() && pEntity->EntIndex() != g::pLocalEntity->EntIndex())
	{
		// Entity Type checks
		ClientClass *pClientClass = pEntity->GetClientClass();
		PlayerInfo_t pinfo;
		if (pClientClass->m_ClassID == (int)EClassIds::CCSPlayer && g_pEngine->GetPlayerInfo(pEntity->EntIndex(), &pinfo))
		{
			// Team Check
			if (pEntity->GetTeam() != g::pLocalEntity->GetTeam() || g_Settings.bFriendlyFire)
			{
				// Spawn Check
				if (!pEntity->IsImmune())
				{
					return true;
				}
			}
		}
	}

	// They must have failed a requirement
	return false;
}

float CAimbot::FovToPlayer(Vector ViewOffSet, Vector View, C_BaseEntity* pEntity, int aHitBox)
{
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
	Vector AimPos = GetHitboxPosition(pEntity, aHitBox);
	// Get delta vector between our local eye position and passed vector
	VectorSubtract(AimPos, Origin, Delta);
	//Delta = AimPos - Origin;

	// Normalize our delta vector
	Normalize(Delta, Delta);

	// Get dot product between delta position and directional forward vectors
	FLOAT DotProduct = Forward.Dot(Delta);

	// Time to calculate the field of view
	return (acos(DotProduct) * (MaxDegrees / PI));
}

int CAimbot::HitScan(C_BaseEntity* pEntity, CUserCmd* pCmd)
{
	std::vector<int> HitBoxesToScan;

		switch (g_Settings.iHitScanMode)
		{
		case 0:
			HitBoxesToScan.push_back((int)HitboxList::Head);
			break;
		case 1:
			HitBoxesToScan.push_back((int)HitboxList::Pelvis);
			HitBoxesToScan.push_back((int)HitboxList::Stomach);
			HitBoxesToScan.push_back((int)HitboxList::Chest);
			HitBoxesToScan.push_back((int)HitboxList::UpperChest);
			HitBoxesToScan.push_back((int)HitboxList::LeftThigh);
			HitBoxesToScan.push_back((int)HitboxList::RightThigh);
			HitBoxesToScan.push_back((int)HitboxList::LeftUpperArm);
			HitBoxesToScan.push_back((int)HitboxList::RightUpperArm);
			HitBoxesToScan.push_back((int)HitboxList::LeftLowerArm);
			HitBoxesToScan.push_back((int)HitboxList::RightLowerArm);
			HitBoxesToScan.push_back((int)HitboxList::LeftFoot);
			HitBoxesToScan.push_back((int)HitboxList::RightFoot);
			HitBoxesToScan.push_back((int)HitboxList::LeftShin);
			HitBoxesToScan.push_back((int)HitboxList::RightShin);
			HitBoxesToScan.push_back((int)HitboxList::Head);
			HitBoxesToScan.push_back((int)HitboxList::Neck);
			break;
		case 2:
			HitBoxesToScan.push_back((int)HitboxList::Head);
			HitBoxesToScan.push_back((int)HitboxList::Neck);
			HitBoxesToScan.push_back((int)HitboxList::UpperChest);
			HitBoxesToScan.push_back((int)HitboxList::Chest);
			HitBoxesToScan.push_back((int)HitboxList::Stomach);
			HitBoxesToScan.push_back((int)HitboxList::Pelvis);
			HitBoxesToScan.push_back((int)HitboxList::LeftUpperArm);
			HitBoxesToScan.push_back((int)HitboxList::RightUpperArm);
			HitBoxesToScan.push_back((int)HitboxList::LeftThigh);
			HitBoxesToScan.push_back((int)HitboxList::RightThigh);
			HitBoxesToScan.push_back((int)HitboxList::LeftFoot);
			HitBoxesToScan.push_back((int)HitboxList::RightFoot);
			HitBoxesToScan.push_back((int)HitboxList::LeftShin);
			HitBoxesToScan.push_back((int)HitboxList::RightShin);
			HitBoxesToScan.push_back((int)HitboxList::LeftLowerArm);
			HitBoxesToScan.push_back((int)HitboxList::RightLowerArm);
			break;
		}

	// check hits
	for (int HitBoxID : HitBoxesToScan)
	{
			Vector Point = GetHitboxPosition(pEntity, HitBoxID);
			float Damage = 0.f;

			if (autowall->CanHit(Point, &Damage, pEntity, HitBoxID))
			{	
				if (Damage >= g_Settings.iMinDamage || Damage > pEntity->GetHealth())
				{
					return HitBoxID;
				}
			}
	}
	return -1;
}


std::vector<Vector> CAimbot::GetMultiplePointsForHitbox(C_BaseEntity* local, C_BaseEntity* entity, int iHitbox, matrix3x4_t BoneMatrix[128])
{
	studiohdr_t* pStudioModel = g_pModelInfo->GetStudiomodel(entity->GetModel());
	mstudiohitboxset_t* set = pStudioModel->pHitboxSet(0);
	mstudiobbox_t *hitbox = set->GetHitbox(iHitbox);

	std::vector<Vector> vecArray;

	Vector max;
	Vector min;
	Utils::VectorTransform(hitbox->bbmax, BoneMatrix[hitbox->bone], max);
	Utils::VectorTransform(hitbox->bbmin, BoneMatrix[hitbox->bone], min);

	auto center = (min + max) * 0.5f;

	Vector CurrentAngles = Utils::CalcAngle(center, local->GetEyeOrigin());

	Vector Forward;
	Utils::AngleVectors(CurrentAngles, &Forward);

	Vector Right = Forward.Cross(Vector(0, 0, 1));
	Vector Left = Vector(-Right.x, -Right.y, Right.z);

	Vector Top = Vector(0, 0, 1);
	Vector Bot = Vector(0, 0, -1);

	switch (iHitbox) 
	{
	case 0:
		if (g_Settings.bMultiPointSelection[0])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Top * (hitbox->radius* (g_Settings.iHeadScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	case 1:
		if (g_Settings.bMultiPointSelection[1])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Left * (hitbox->radius* (g_Settings.iNeckScale / 100.f));
			vecArray[2] += Right * (hitbox->radius* (g_Settings.iNeckScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	case 2:
		if (g_Settings.bMultiPointSelection[2])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Left * (hitbox->radius* (g_Settings.iPelvisScale / 100.f));
			vecArray[2] += Right * (hitbox->radius* (g_Settings.iPelvisScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	case 3:
		if (g_Settings.bMultiPointSelection[4])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Left * (hitbox->radius* (g_Settings.iStomachScale / 100.f));
			vecArray[2] += Right * (hitbox->radius* (g_Settings.iStomachScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	case 4:
		if (g_Settings.bMultiPointSelection[3])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Left * (hitbox->radius* (g_Settings.iChestScale / 100.f));
			vecArray[2] += Right * (hitbox->radius* (g_Settings.iChestScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	case 5:
		if (g_Settings.bMultiPointSelection[3])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Left * (hitbox->radius* (g_Settings.iChestScale / 100.f));
			vecArray[2] += Right * (hitbox->radius* (g_Settings.iChestScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	case 6:
		if (g_Settings.bMultiPointSelection[3])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Left * (hitbox->radius* (g_Settings.iChestScale / 100.f));
			vecArray[2] += Right * (hitbox->radius* (g_Settings.iChestScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	case 7:
		if (g_Settings.bMultiPointSelection[7])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Left * (hitbox->radius* (g_Settings.iLegsScale / 100.f));
			vecArray[2] += Right * (hitbox->radius* (g_Settings.iLegsScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	case 8:
		if (g_Settings.bMultiPointSelection[7])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Left * (hitbox->radius* (g_Settings.iLegsScale / 100.f));
			vecArray[2] += Right * (hitbox->radius* (g_Settings.iLegsScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	case 9:
		if (g_Settings.bMultiPointSelection[7])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Left * (hitbox->radius* (g_Settings.iLegsScale / 100.f));
			vecArray[2] += Right * (hitbox->radius* (g_Settings.iLegsScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	case 10:
		if (g_Settings.bMultiPointSelection[7])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Left * (hitbox->radius* (g_Settings.iLegsScale / 100.f));
			vecArray[2] += Right * (hitbox->radius* (g_Settings.iLegsScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	case 11:
		if (g_Settings.bMultiPointSelection[7])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Left * (hitbox->radius* (g_Settings.iLegsScale / 100.f));
			vecArray[2] += Right * (hitbox->radius* (g_Settings.iLegsScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	case 12:
		if (g_Settings.bMultiPointSelection[7])
		{
			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Left * (hitbox->radius* (g_Settings.iLegsScale / 100.f));
			vecArray[2] += Right * (hitbox->radius* (g_Settings.iLegsScale / 100.f));
		}
		else
		{
			for (auto i = 0; i < 1; ++i)
			{
				vecArray.emplace_back(center);
			}
		}
		break;
	}
	return vecArray;
}

Vector CAimbot::hitscan_mp(C_BaseEntity* entity)
{
	if (!g::pLocalEntity)
		return Vector(0, 0, 0);

	Vector vector_best_point = Vector(0, 0, 0);
	int mostDamage = -1;
	mostDamage = g_Settings.iMinDamage;

	matrix3x4_t matrix[128];

	if (!entity->SetupBones(matrix, 128, 256, 0))
		return Vector(0, 0, 0);

	std::vector<int> HitBoxesToScan;

	if (g_Settings.bHitboxSelection[4])
	{
		HitBoxesToScan.push_back((int)HitboxList::Pelvis);
	}

	if (g_Settings.bHitboxSelection[3])
	{
		HitBoxesToScan.push_back((int)HitboxList::Stomach);
	}

	if (g_Settings.bHitboxSelection[2])
	{
		HitBoxesToScan.push_back((int)HitboxList::Chest);
		HitBoxesToScan.push_back((int)HitboxList::LowerChest);
		HitBoxesToScan.push_back((int)HitboxList::UpperChest);
	}

	if (g_Settings.bHitboxSelection[0])
	{
		HitBoxesToScan.push_back((int)HitboxList::Head);
	}


	if (g_Settings.bHitboxSelection[1])
	{
		HitBoxesToScan.push_back((int)HitboxList::Neck);
	}
	
	if (g_Settings.bHitboxSelection[5])
	{
		HitBoxesToScan.push_back((int)HitboxList::LeftUpperArm);
		HitBoxesToScan.push_back((int)HitboxList::RightUpperArm);
		HitBoxesToScan.push_back((int)HitboxList::LeftLowerArm);
		HitBoxesToScan.push_back((int)HitboxList::RightLowerArm);
	}
	if (g_Settings.bHitboxSelection[6])
	{
		HitBoxesToScan.push_back((int)HitboxList::LeftThigh);
		HitBoxesToScan.push_back((int)HitboxList::RightThigh);
		HitBoxesToScan.push_back((int)HitboxList::LeftShin);
		HitBoxesToScan.push_back((int)HitboxList::RightShin);
		HitBoxesToScan.push_back((int)HitboxList::LeftFoot);
		HitBoxesToScan.push_back((int)HitboxList::RightFoot);
	}

	for (auto HitBoxID : HitBoxesToScan)
	{
		for (auto point : GetMultiplePointsForHitbox(g::pLocalEntity, entity, HitBoxID, matrix))
		{
			float damage = 0.f;

			if (autowall->CanHit2(point, &damage))
			{
				if (damage > mostDamage)
				{
					mostDamage = damage;
					vector_best_point = point;

					if (mostDamage >= entity->GetHealth())
						return vector_best_point;
				}
			}
		}
	}	
	return vector_best_point;
}

Vector CAimbot::body_multipoint(C_BaseEntity* entity)
{
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	if (!local_player)
		return Vector(0, 0, 0);

	Vector local_position = local_player->GetEyeOrigin();

	Vector vector_best_point = Vector(0, 0, 0);
	int mostDamage = -1;
	mostDamage = g_Settings.iMinDamage;

	matrix3x4_t matrix[128];

	if (!entity->SetupBones(matrix, 128, 256, 0))
		return Vector(0, 0, 0);

	int hitboxes[] =
	{
		HitboxList::Pelvis,
		HitboxList::Stomach,
		HitboxList::Chest,
		HitboxList::LeftLowerArm,
		HitboxList::LeftUpperArm,
		HitboxList::RightUpperArm,
		HitboxList::RightLowerArm,
		HitboxList::LowerChest,
		HitboxList::Head,
		HitboxList::LeftShin,
		HitboxList::RightShin,
		HitboxList::LeftFoot,
		HitboxList::RightFoot
	};

	for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
	{
		for (auto point : GetMultiplePointsForHitbox(local_player, entity, hitboxes[i], matrix))
		{
			float damage = 0.f;

			if (autowall->CanHit2(point, &damage))
			{
				if (damage > mostDamage)
				{
					mostDamage = damage;
					vector_best_point = point;

					if (mostDamage >= entity->GetHealth() || damage >= entity->GetHealth())
						return vector_best_point;
				}
			}
		}
	}
	return vector_best_point;
}

Vector CAimbot::full_multipoint(C_BaseEntity* entity)
{
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	if (!local_player)
		return Vector(0, 0, 0);

	Vector local_position = local_player->GetEyeOrigin();

	Vector vector_best_point = Vector(0, 0, 0);
	int mostDamage = 0;
	mostDamage = g_Settings.iMinDamage;

	matrix3x4_t matrix[128];

	if (!entity->SetupBones(matrix, 128, 256, 0))
		return Vector(0, 0, 0);

	int hitboxes[] =
	{
		HitboxList::Head,
		HitboxList::Pelvis,
		HitboxList::Stomach,
		HitboxList::Chest,
		HitboxList::LowerChest,
		HitboxList::LeftLowerArm,
		HitboxList::LeftUpperArm,
		HitboxList::RightUpperArm,
		HitboxList::RightLowerArm,
		HitboxList::LeftShin,
		HitboxList::RightShin,
		HitboxList::LeftFoot,
		HitboxList::RightFoot
	};

	for (int i = 0; i < ARRAYSIZE(hitboxes); i++)
	{
		for (auto point : GetMultiplePointsForHitbox(local_player, entity, hitboxes[i], matrix))
		{
			float damage = 0.f;

			if (autowall->CanHit2(point, &damage))
			{
				if (damage > mostDamage)
				{
					mostDamage = damage;
					vector_best_point = point;

					if (mostDamage >= entity->GetHealth() || damage >= entity->GetHealth())
						return vector_best_point;
				}
			}
		}
	}
	return vector_best_point;
}

Vector CAimbot::taser_multipoint(C_BaseEntity* entity)
{
	if (!g::pLocalEntity)
		return Vector(0, 0, 0);

	Vector vector_best_point = Vector(0, 0, 0);
	float mostDamage = -1.f;
	mostDamage = 100.f;

	matrix3x4_t matrix[128];

	if (!entity->SetupBones(matrix, 128, 256, 0))
		return Vector(0, 0, 0);

	for (auto point : GetMultiplePointsForHitbox(g::pLocalEntity, entity, 2, matrix))
	{
		float damage = 0.f;

		if (autowall->CanHit2(point, &damage))
		{
			if (damage > mostDamage)
			{
				mostDamage = damage;
				vector_best_point = point;

				if (mostDamage >= entity->GetHealth())
					return vector_best_point;
			}
		}
	}
	return vector_best_point;
}

int CAimbot::GetTargetHealth(CUserCmd* pCmd)
{
	// Target selection
	int target = -1;
	int minHealth = 101;
	
	for (int i = 0; i < g_pEntityList->GetHighestEntityIndex(); i++)
	{
		C_BaseEntity *pEntity = g_pEntityList->GetClientEntity(i);
		
		if (aimbot->TargetMeetsRequirements(pEntity))
		{
			int Health = pEntity->GetHealth();

			if (Health < minHealth)
			{
				minHealth = Health;
				target = i;
			}
		}		
	}
	return target;
}

void CAimbot::GetMultipointPositions(C_BaseEntity* entity, std::vector<Vector>& positions, int hitbox_index, float pointscale)
{
	const auto hitbox = aimbot->GetHitbox(entity, hitbox_index);
	if (!hitbox)
		return;

	const float hitbox_radius = hitbox->radius * pointscale;

	// 2 spheres
	Vector Min = hitbox->bbmin, Max = hitbox->bbmax;

	if (hitbox->radius == -1.f)
	{
		const auto center = (Min + Max) * 0.5f;

		positions.emplace_back();
	}
	else
	{
		// Points 0 - 5 = min
		// Points 6 - 11 = max
		Vector P[12];
		for (int j = 0; j < 6; j++) { P[j] = Min; }
		for (int j = 7; j < 12; j++) { P[j] = Max; }

		P[1].x += hitbox_radius;
		P[2].x -= hitbox_radius;
		P[3].y += hitbox_radius;
		P[4].y -= hitbox_radius;
		P[5].z += hitbox_radius;

		P[6].x += hitbox_radius;
		P[7].x -= hitbox_radius;
		P[8].y += hitbox_radius;
		P[9].y -= hitbox_radius;
		P[10].z += hitbox_radius;
		P[11].z -= hitbox_radius;

		for (int j = 0; j < 12; j++)
		{
			Utils::VectorTransform(P[j], entity->GetBoneMatrix(hitbox->bone), P[j]);
			//Points[j] += OriginOffset;
			positions.push_back(P[j]);
		}
	}
}

bool CAimbot::accepted_inaccuracy(C_BaseCombatWeapon* weapon, C_BaseEntity* entity, Vector position)
{
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	if (!local_player)
		return false;

	if (!weapon)
		return false;

//	if (weapon->sGetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_TASER)
//		return true;

	switch (g_Settings.iHitChanceType)
	{
	case 0: /// none
		return true;
		break;
	case 1: /// hitchance
	{
		auto RandomFloat = [](float min, float max) -> float
		{
			typedef float(*RandomFloat_t)(float, float);
			static RandomFloat_t m_RandomFloat = (RandomFloat_t)GetProcAddress(GetModuleHandleA("vstdlib.dll"), "RandomFloat");
			return m_RandomFloat(min, max);
		};

		Vector angle = Utils::NormalizeAngle(Utils::CalcAngle(local_player->GetEyeOrigin(), position));

		Vector forward, right, up;
		Utils::AngleVectorsbeta(angle, &forward, &right, &up);

		int iHit = 0;
		weapon->UpdateAccuracyPenalty();
		for (int i = 0; i < 256; i++)
		{
			float RandomA = RandomFloat(0.0f, 1.0f);
			float RandomB = 1.0f - RandomA * RandomA;

			RandomA = RandomFloat(0.0f, M_PI_F * 2.0f);
			RandomB *= weapon->GetSpreadCone();

			float SpreadX1 = (cos(RandomA) * RandomB);
			float SpreadY1 = (sin(RandomA) * RandomB);

			float RandomC = RandomFloat(0.0f, 1.0f);
			float RandomF = 1.0f - RandomC * RandomC;

			RandomC = RandomFloat(0.0f, M_PI_F * 2.0f);
			RandomF *= weapon->GetInaccuracy();

			float SpreadX2 = (cos(RandomC) * RandomF);
			float SpreadY2 = (sin(RandomC) * RandomF);

			float fSpreadX = SpreadX1 + SpreadX2;
			float fSpreadY = SpreadY1 + SpreadY2;

			Vector vSpreadForward;
			vSpreadForward[0] = forward[0] + (fSpreadX * right[0]) + (fSpreadY * up[0]);
			vSpreadForward[1] = forward[1] + (fSpreadX * right[1]) + (fSpreadY * up[1]);
			vSpreadForward[2] = forward[2] + (fSpreadX * right[2]) + (fSpreadY * up[2]);
			vSpreadForward.NormalizeInPlace();

			Vector qaNewAngle;
			Utils::VectorAngles(vSpreadForward, qaNewAngle);
			qaNewAngle = Utils::NormalizeAngle(qaNewAngle);

			Vector vEnd;
			Utils::AngleVectors(qaNewAngle, &vEnd);
			vEnd = local_player->GetEyeOrigin() + (vEnd * 8192.f);

			trace_t trace;
			CTraceFilterOneEntity2 filter;
			filter.pEntity = entity;
			Ray_t ray;
			ray.Init(local_player->GetVecOrigin() + local_player->GetViewOffset(), vEnd);

			g_pTrace->TraceRay(ray, MASK_ALL, &filter, &trace);
			if (trace.m_pEnt == entity)
				iHit++;

			if (iHit / 256.f >= g_Settings.iHitChance / 100.f)
				return true;
		}
		return false;
	}
	break;
	case 2: /// spread limit
	{
		float hitchance = 101; //lol idk why, its pasted anyway so w/e
		float inaccuracy = weapon->GetInaccuracy();

		if (inaccuracy == 0)
			inaccuracy = 0.0000001;

		inaccuracy = 1 / inaccuracy;
		hitchance = inaccuracy;

		return hitchance >= g_Settings.iHitChance;
	}
	break;
	}
	return false;
}
bool CAimbot::AimAtPoint(C_BaseEntity* pLocal, Vector point, CUserCmd *pCmd)
{
	Vector angles;
	Vector src = pLocal->GetEyeOrigin();

	Utils::CalcAngle(src, point, angles); 
	Utils::NormaliseViewAngle(angles);

	g_pEngine->SetViewAngles(angles);
	
		//Globals::bsendpacket = true;
		pCmd->viewangles = angles;

	return true;
}

float GetCurtime(CUserCmd* ucmd)
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

bool  CAimbot::IsWeaponGrenade2(C_BaseCombatWeapon* weapon)
{
	if (weapon == nullptr) return false;
	int id = weapon->sGetItemDefinitionIndex();
	static const std::vector<int> v = { WEAPON_FLASHBANG,WEAPON_HEGRENADE,WEAPON_SMOKEGRENADE,WEAPON_MOLOTOV,WEAPON_DECOY,WEAPON_INCGRENADE };
	return (std::find(v.begin(), v.end(), id) != v.end());
}

bool CAimbot::can_shoot(CUserCmd* cmd)
{
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
	if (!local_player || local_player->GetHealth() <= 0 || local_player->GetNextAttack() > GetCurtime(cmd))
		return false;

	C_BaseCombatWeapon* weapon = g::pLocalEntity->GetActiveWeapon();
	if (!weapon)
		return false;

	if (IsWeaponGrenade2(weapon))
	{
		if (weapon->IsPinPulled())
			return false;

		if (weapon->GetThrowTime() <= 0 || weapon->GetThrowTime() > GetCurtime(cmd))
			return false;

		return true;
	}

		if (weapon->GetNextPrimaryAttack() > GetCurtime(cmd))
			return false;

		/// revolver
		if (weapon->GetItemDefinitionIndex() == WEAPON_REVOLVER && weapon->GetPostponeFireReadyTime() > GetCurtime(cmd))
			return false;
	

	return true;
}

inline Vector ExtrapolateTick(Vector p0, Vector v0)
{
	return p0 + (v0 * g_pGlobalVars->intervalPerTick);
}

Vector C_BaseEntity::GetPredicted(Vector p0)
{
	return ExtrapolateTick(p0, this->GetVelocity());
}

bool CAimbot::IsKnife31(C_BaseCombatWeapon* pWeapon)
{
	if (pWeapon == nullptr) return false;
	int id = pWeapon->sGetItemDefinitionIndex();
	static const std::vector<int> v = { WEAPON_KNIFE_BAYONET, WEAPON_KNIFE_SURVIVAL_BOWIE, WEAPON_KNIFE_BUTTERFLY, WEAPON_KNIFE_FALCHION, WEAPON_KNIFE_FLIP, WEAPON_KNIFE_GUT, WEAPON_KNIFE_KARAMBIT, WEAPON_KNIFE_M9_BAYONET, WEAPON_KNIFE_PUSH, WEAPON_KNIFE_TACTICAL , WEAPON_KNIFE, WEAPON_KNIFE_T, WEAPON_KNIFEGG };
	return (std::find(v.begin(), v.end(), id) != v.end());
}


template<class T, class U>
static T clamp(T in, U low, U high) {
	if (in <= low)
		return low;

	if (in >= high)
		return high;

	return in;

}

float GetLerpTime()
{
	int ud_rate = g_pConVar->FindVar("cl_updaterate")->GetInt();
	ConVar *min_ud_rate = g_pConVar->FindVar("sv_minupdaterate");
	ConVar *max_ud_rate = g_pConVar->FindVar("sv_maxupdaterate");

	if (min_ud_rate && max_ud_rate)
		ud_rate = max_ud_rate->GetInt();

	float ratio = g_pConVar->FindVar("cl_interp_ratio")->GetFloat();

	if (ratio == 0)
		ratio = 1.0f;

	float lerp = g_pConVar->FindVar("cl_interp")->GetFloat();
	ConVar *c_min_ratio = g_pConVar->FindVar("sv_client_min_interp_ratio");
	ConVar *c_max_ratio = g_pConVar->FindVar("sv_client_max_interp_ratio");

	if (c_min_ratio && c_max_ratio && c_min_ratio->GetFloat() != 1)
		ratio = clamp(ratio, c_min_ratio->GetFloat(), c_max_ratio->GetFloat());

	return max(lerp, (ratio / ud_rate));
}

/*	if (g_Settings.bLegitAimAccBoost && g_Settings.bLegitBacktrack)
		{
			/*for (int t = 0; t < 13; ++t)  //going trough all recorded ticks.
			{
				Vector ViewDir = backtracking->AngleVector(pCmd->viewangles + (g::pLocalEntity->GetPunchAngles() * 2.f));
				Vector hitboxPos = headPositions[i][t].hitboxPos;
				float FOVDistance = backtracking->DistPointToLine(hitboxPos, g::pLocalEntity->GetEyeOrigin(), ViewDir); //getting fov to each tick
				if (bestFov > FOVDistance) //comparing fovs to get closest
				{
					bestFov = FOVDistance;
					aimtick = t;
				}
			}
			AimPoint = headPositions[i][aimtick].hitboxPos; //get vector of enemys hitbox
			if (!IsVisiblePoint(g::pLocalEntity, AimPoint)) //check if that point is visible
			{
				continue;
			}
			AimAngle = Utils::NormalizeAngle(Utils::CalcAngle(g::pLocalEntity->GetEyeOrigin(), entity->GetPredicted(AimPoint)));
		}*/

void CAimbot::DoAimbot(CUserCmd *pCmd)
{
	if (!g::pLocalEntity)
		return;

	if (g::pLocalEntity->GetHealth() <= 0)
		return;

	//if (g::pLocalEntity->GetFlags() & FL_ATCONTROLS)
	//	return;

	C_BaseCombatWeapon* pWeapon = g::pLocalEntity->GetActiveWeapon();
	if (!pWeapon)
		return;

	if (Globals::dontdoaa)
		return;

	if (IsKnife31(pWeapon) || IsWeaponGrenade2(pWeapon) || pWeapon->GetAmmo() == 0 || !IsBallisticWeapon(pWeapon))
	{
		Globals::shouldstop = false;
		return;
	}

	if (!can_shoot(pCmd))
	{
		pCmd->buttons &= ~IN_ATTACK;
		Globals::shouldstop = false;
		return;
	}

	if (pWeapon->sGetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_REVOLVER)
	{
		pCmd->buttons |= IN_ATTACK;

		float flPostponeFireReady = pWeapon->GetPostponeFireReadyTime();
		if (flPostponeFireReady > 0 && flPostponeFireReady < g_pGlobalVars->curtime)
		{
			pCmd->buttons &= ~IN_ATTACK;
		}
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

		if (entity->GetMoveType() == MOVETYPE_NOCLIP)
			continue;

		Vector shoot_here;

		Vector AimAngle;

		if (pWeapon->sGetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_TASER)
		{
			shoot_here = taser_multipoint(entity);
			AimAngle = Utils::NormalizeAngle(Utils::CalcAngle(g::pLocalEntity->GetEyeOrigin(), entity->GetPredicted(shoot_here)));
		}
		else
		{
			shoot_here = hitscan_mp(entity);
			AimAngle = Utils::NormalizeAngle(Utils::CalcAngle(g::pLocalEntity->GetEyeOrigin(), entity->GetPredicted(shoot_here)));
		}
			
		if (shoot_here == Vector(0, 0, 0))
			continue;

		if (AimAngle == Vector(0, 0, 0))
			continue;
		
		if (accepted_inaccuracy(pWeapon, entity, shoot_here))
		{
			if (g_Settings.bAutoFire)
				pCmd->buttons |= IN_ATTACK;

			if (!g_Settings.bSilentAim)
				g_pEngine->SetViewAngles(AimAngle);

			pCmd->viewangles = AimAngle - (g::pLocalEntity->GetPunchAngles() * 2.f);

			pCmd->tick_count = TIME_TO_TICKS(entity->m_flSimulationTime() + GetLerpTime());

			if (pCmd->buttons & IN_ATTACK)
			{
				Globals::shots_fired[entity->EntIndex()]++;
				Globals::shot[entity->EntIndex()] = true;
			}
		}
		else
		{
			Globals::shouldstop = true;
			if (!g::pLocalEntity->IsScoped() && g::pLocalEntity->GetFlags() & FL_ONGROUND)
				pCmd->buttons |= IN_ZOOM;
		}
	}
	Globals::shouldstop = false;
}

void CAimbot::fix_recoil(CUserCmd* cmd)
{
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	if (!local_player)
		return;

	if (cmd->buttons & IN_ATTACK)
	{
		local_player->GetPunchAngles() * 2.f;
	}
}
	
CAimbot* aimbot = new CAimbot();