#pragma once
#include "..\SDK\CInput.h"
#include "..\SDK\CEntity.h"
#include "..\Utils\GlobalVars.h"
#include "..\SDK\ClientClass.h"
#include "..\SDK\ConVar.h"
#include "..\Settings.h"
#include "..\SDK\CTrace.h"
#include "betaAutoWall.h"
#include "..\SDK\Materials.h"
#include "..\Globals.h"
#include "FakeLag.h"
#include "LBacktrack.h"

namespace FEATURES
{
	namespace RAGEBOT
	{
		class Aimbot
		{
		public:
			void Do(CUserCmd* cmd);

		private:
			struct PlayerAimbotInfo
			{
				int head_damage = 0, hitscan_damage = 0, tick, extrapolted_ticks;
				Vector head_position, hitscan_position;
				FEATURES::RAGEBOT::Backtracking_Record backtrack_record;
			};

		private:
			PlayerAimbotInfo player_aimbot_info[64];

		public:
			PlayerAimbotInfo GetPlayerAimbotInfo(C_BaseEntity* entity)
			{
				return player_aimbot_info[entity->EntIndex()];
			}

		private:
			bool DoHitscan(C_BaseEntity* entity, Vector& final_position, int& final_damage);
			bool DoHeadAim(C_BaseEntity* entity, Vector& final_position, int& final_damage);

			bool IsAccurate(C_BaseEntity* entity, Vector position);

			/// helper functions
			void GetMultipointPositions(C_BaseEntity* entity, std::vector<Vector>& positions, int hitbox_index, float pointscale);
			Vector GetHitboxPosition(C_BaseEntity* entity, int hitbox_index);
			static mstudiobbox_t* GetHitbox(C_BaseEntity* entity, int hitbox_index);
		};

		extern Aimbot aimbot;
	}
}
