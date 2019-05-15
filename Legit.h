#pragma once
#include "..\SDK\CInput.h"
#include "..\SDK\CEntity.h"
#include "..\Utils\GlobalVars.h"
#include "..\SDK\ClientClass.h"
#include "Aimbot.h"
#include "LegitBacktracking.h"

struct LAimbotData_t
{
	LAimbotData_t(C_BaseEntity* player, const int& idx)
	{
		this->pPlayer = player;
		this->index = idx;
	}
	C_BaseEntity*	pPlayer;
	int					index;
};

class CLegitBot
{
public:
	std::vector<LAimbotData_t>	Entities;
	void Move(CUserCmd * pCmd);
	void SelectTarget();
	float FovToPlayer(Vector ViewOffSet, Vector View, C_BaseEntity * pEntity, int aHitBox);
	void DoAimbot(CUserCmd * pCmd);
};

extern CLegitBot* laimbot;