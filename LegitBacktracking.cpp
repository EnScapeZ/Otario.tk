#include "LegitBacktracking.h"

void BackTrack::legitBackTrack(CUserCmd* cmd, C_BaseEntity* pLocal)
{
	if (g_Settings.bLegitBacktrack && g_Settings.bLegitBotMaster)
	{
		int bestTargetIndex = -1;
		float bestFov = FLT_MAX;
		PlayerInfo_t info;
		if (!pLocal->IsAlive())
			return;

		for (int i = 0; i < g_pEngine->GetMaxClients(); i++)
		{
			C_BaseEntity* entity = g_pEntityList->GetClientEntity(i);

			if (!entity || !pLocal)
				continue;

			if (entity == pLocal)
				continue;

			if (!g_pEngine->GetPlayerInfo(i, &info))
				continue;

			if (entity->IsDormant())
				continue;

			if (entity->GetTeam() == pLocal->GetTeam())
				continue;

			if (entity->IsAlive())
			{
				simtime = entity->m_flSimulationTime();
				hitboxPos = aimbot->GetHitboxPosition(entity, g_Settings.iLegitHitBox);

				headPositions[i][cmd->command_number % 13] = backtrackData{ simtime, hitboxPos };
				Vector ViewDir = AngleVector(cmd->viewangles + (pLocal->GetPunchAngles() * 2.f));
				float FOVDistance = DistPointToLine(hitboxPos, pLocal->GetEyeOrigin(), ViewDir);

				if (bestFov > FOVDistance)
				{
					bestFov = FOVDistance;
					bestTargetIndex = i;
				}
			}
		}

		float bestTargetSimTime;
		if (bestTargetIndex != -1)
		{
			float tempFloat = FLT_MAX;
			Vector ViewDir = AngleVector(cmd->viewangles + (pLocal->GetPunchAngles() * 2.f));
			for (int t = 0; t < 13; ++t)
			{
				float tempFOVDistance = DistPointToLine(headPositions[bestTargetIndex][t].hitboxPos, pLocal->GetEyeOrigin(), ViewDir);
				if (tempFloat > tempFOVDistance && headPositions[bestTargetIndex][t].simtime > pLocal->m_flSimulationTime() - 1)
				{
					tempFloat = tempFOVDistance;
					bestTargetSimTime = headPositions[bestTargetIndex][t].simtime;
				}
			}
			
			if (cmd->buttons & IN_ATTACK)
			{
				cmd->tick_count = TIME_TO_TICKS(bestTargetSimTime);
			}
		}
	}
}

BackTrack* backtracking = new BackTrack();
backtrackData headPositions[64][12];