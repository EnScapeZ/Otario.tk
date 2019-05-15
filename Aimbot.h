#pragma once
#include "..\SDK\CInput.h"
#include "..\SDK\CEntity.h"
#include "..\Utils\GlobalVars.h"
#include "..\SDK\ClientClass.h"
#include "..\SDK\ConVar.h"
#include "..\Settings.h"
#include "..\SDK\CTrace.h"
#include "AutoWall.h"
#include "..\SDK\Materials.h"
#include "..\Globals.h"
#include "FakeLag.h"

enum HitboxList
{
	Head = 0,
	Neck,
	Pelvis,
	Stomach,
	LowerChest,
	Chest,
	UpperChest,
	RightThigh,
	LeftThigh,
	RightShin,
	LeftShin,
	RightFoot,
	LeftFoot,
	RightHand,
	LeftHand,
	RightUpperArm,
	RightLowerArm,
	LeftUpperArm,
	LeftLowerArm
};

#define PI 3.14159265358979323846f
#define RADPI 57.295779513082f
#define PI_F	((float)(PI)) 


struct AimbotData_t
{
	AimbotData_t(C_BaseEntity* player, const int& idx)
	{
		this->pPlayer = player;
		this->index = idx;
	}
	C_BaseEntity*	pPlayer;
	int					index;
};

class CAimbot
{
public:
	void minspeed(C_BaseEntity * local, CUserCmd * cmd);
	void stopmovement(C_BaseEntity * local_player, CUserCmd * cmd);
	bool TargetMeetsRequirements(C_BaseEntity * pEntity);
	float FovToPlayer(Vector ViewOffSet, Vector View, C_BaseEntity * pEntity, int aHitBox);
	std::vector<Vector> GetMultiplePointsForHitbox(C_BaseEntity * local, C_BaseEntity * entity, int iHitbox, VMatrix BoneMatrix[128]);
	int HitScan(C_BaseEntity * pEntity, CUserCmd * pCmd);
	std::vector<Vector> GetMultiplePointsForHitbox(C_BaseEntity * local, C_BaseEntity * entity, int iHitbox, matrix3x4_t BoneMatrix[128]);
	Vector hitscan_mp(C_BaseEntity * entity);
	Vector multipoint(C_BaseEntity * entity);
	Vector body_multipoint(C_BaseEntity * entity);
	Vector full_multipoint(C_BaseEntity * entity);
	Vector taser_multipoint(C_BaseEntity * entity);
	int GetTargetHealth(CUserCmd * pCmd);
	void GetMultipointPositions(C_BaseEntity * entity, std::vector<Vector>& positions, int hitbox_index, float pointscale);
//	int HitScan(C_BaseEntity * pEntity);
//	int GetTargetHealth();
	bool accepted_inaccuracy(C_BaseCombatWeapon * weapon, C_BaseEntity * entity, Vector position);
	bool AimAtPoint(C_BaseEntity * pLocal, Vector point, CUserCmd * pCmd);
	bool IsWeaponGrenade2(C_BaseCombatWeapon * weapon);
	bool can_shoot(CUserCmd * cmd);
	bool IsKnife31(C_BaseCombatWeapon * pWeapon);
	//bool AimAtPoint(C_BaseEntity * pLocal, Vector point, CUserCmd * pCmd);
	void DoAimbot(CUserCmd * pCmd);
	void fix_recoil(CUserCmd* cmd);
	void Init();
	void Move(CUserCmd * pCmd);
	void SelectTarget();
	mstudiobbox_t * GetHitbox(C_BaseEntity * entity, int hitbox_index);
	Vector GetHitboxPosition(C_BaseEntity * pEntity, int Hitbox);
	bool IsBallisticWeapon(void * weapon);
	Vector GetHitboxPosition2(int Hitbox, C_BaseEntity * pBaseEntity);
	bool IsVisible(C_BaseEntity * pLocal, C_BaseEntity * pEntity, int BoneID);
	Vector GetHitboxPosition2(C_BaseEntity * pEntity, int Hitbox);
	Vector GetHitboxPosition(int Hitbox, C_BaseEntity * pBaseEntity);
	int HitBox;
	//C_BaseEntity* pTarget = nullptr;
	bool IsLocked;
	int TargetID;
	std::vector<AimbotData_t>	Entities;
};

extern CAimbot* aimbot;