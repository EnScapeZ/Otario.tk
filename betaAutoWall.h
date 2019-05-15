#pragma once
#include "..\SDK\CInput.h"
#include "..\SDK\CEntity.h"
#include "..\Utils\GlobalVars.h"
#include "..\SDK\ClientClass.h"
#include "..\SDK\CTrace.h"
#include "..\SDK\Definitions.h"
#include "..\SDK\ISurfaceData.h"
#include "..\SDK\ConVar.h"
#include "..\SDK\ConVar.h"
namespace FEATURES
{
	namespace RAGEBOT
	{
		class Autowall
		{
		public:
			struct Autowall_Return_Info
			{
				int damage;
				int hitgroup;
				int penetration_count;
				bool did_penetrate_wall;
				float thickness;
				Vector end;
				C_BaseEntity* hit_entity;
			};

			Autowall_Return_Info CalculateDamage(Vector start, Vector end, C_BaseEntity* from_entity = nullptr, C_BaseEntity* to_entity = nullptr, int specific_hitgroup = -1);
			float GetThickness(Vector start, Vector end);

			inline bool IsAutowalling() const
			{
				return is_autowalling;
			}

		private:
			bool is_autowalling = false;

			struct Autowall_Info
			{
				Vector start;
				Vector end;
				Vector current_position;
				Vector direction;

				ITraceFilter* filter;
				trace_t enter_trace;

				float thickness;
				float current_damage;
				int penetration_count;
			};

			bool CanPenetrate(C_BaseEntity* attacker, Autowall_Info& info, WeaponInfo_t* weapon_data);

			void ScaleDamage(C_BaseEntity* entity, WeaponInfo_t* weapon_info, int hitgroup, float& current_damage);

			void UTIL_TraceLine(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, C_BaseEntity* ignore, trace_t* ptr)
			{
				Ray_t ray;
				ray.Init(vecAbsStart, vecAbsEnd);
				CTraceFilter filter;
				filter.pSkip = ignore;

				g_pTrace->TraceRay(ray, mask, &filter, ptr);
			}

			void GetBulletTypeParameters(float& maxRange, float& maxDistance)
			{
				maxRange = 35.0;
				maxDistance = 3000.0;
			}

			bool IsBreakableEntity(C_BaseEntity* entity)
			{
				if (!entity || !entity->EntIndex())
					return false;

				//m_takeDamage isn't properly set when using the signature.
				//Back it up, set it to true, then restore.
				int takeDamageBackup = *reinterpret_cast<byte*>(uintptr_t(entity) + 0x27C);

				auto pClass = entity->GetClientClass();
				if (!pClass)
					return false;

				//				 '       ''     '      '   '
				//			    01234567890123456     012345678
				//Check against CBreakableSurface and CBaseDoor

				//Windows etc. are CBrekableSurface
				//Large garage door in Office is CBaseDoor and it get's reported as a breakable when it is not one
				//This is seperate from "CPropDoorRotating", which is a normal door.
				//Normally you would also check for "CFuncBrush" but it was acting oddly so I removed it. It's below if interested.
				//((clientClass->m_pNetworkName[1]) != 'F' || (clientClass->m_pNetworkName[4]) != 'c' || (clientClass->m_pNetworkName[5]) != 'B' || (clientClass->m_pNetworkName[9]) != 'h')

				if ((pClass->pNetworkName[1] == 'B' && pClass->pNetworkName[9] == 'e' && pClass->pNetworkName[10] == 'S' && pClass->pNetworkName[16] == 'e')
					|| (pClass->pNetworkName[1] != 'B' || pClass->pNetworkName[5] != 'D'))
					*reinterpret_cast<byte*>(uintptr_t(entity) + 0x27C) = 2;

				typedef bool(__thiscall* IsBreakbaleEntity_Fn)(C_BaseEntity*);
				static IsBreakbaleEntity_Fn is_breakable_entity = (IsBreakbaleEntity_Fn)Utils::FindSignature("client_panorama.dll", "55 8B EC 51 56 8B F1 85 F6 74 68");

				bool breakable = is_breakable_entity(entity);
				*reinterpret_cast<byte*>(uintptr_t(entity) + 0x27C) = takeDamageBackup;

				return breakable;
			}

			bool TraceToExit(trace_t& enterTrace, trace_t& exitTrace, Vector startPosition, Vector direction);
			bool HandleBulletPenetration2(WeaponInfo_t * wpn_data, Autowall_Info & data);
			bool HandleBulletPenetration(WeaponInfo_t * weaponData, trace_t & enterTrace, Vector & eyePosition, Vector direction, int & possibleHitsRemaining, float & currentDamage, float penetrationPower, bool sv_penetration_type, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration);
		};

		extern Autowall betaautowall;
	}
}