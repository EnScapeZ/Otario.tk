#include "ESP.h"
#include "..\Utils\Utils.h"
#include "..\SDK\IVEngineClient.h"
#include "..\SDK\PlayerInfo.h"
#include "..\Globals.h"

ESP g_ESP;

void ESP::set_hitmarker_time(float time)
{
	Globals::flHurtTime = time;
}

void set_dormant_time(float time)
{
	Globals::dormanttime = time;
}

/*

bool IsFakeDucking(C_BaseEntity* entity)
{
	int storedTick = 0;
	int crouchedTicks[64];
	float duckamount = entity->m_flDuckAmount();
	if (!duckamount) return false;
	float duckspeed = entity->m_flDuckSpeed();
	if (!duckspeed) return false;

	if (storedTick != g_pGlobalVars->tickcount)
	{
		crouchedTicks[entity->EntIndex()]++;
		storedTick = g_pGlobalVars->tickcount;
	}
	if (duckspeed == 8 && duckamount <= 0.9 && duckamount > 0.01 && (entity->GetFlags() & FL_ONGROUND) && (crouchedTicks[entity->EntIndex()] >= 5))
		return true;
	else
		return false;
}

*/

void ESP::DrawHitmarker()
{
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	static int lineSize = 4.25;

	static float alpha = 0;
	float step = 255.f / 0.3f *g_pGlobalVars->frametime;

	if (g_pGlobalVars->realtime - Globals::flHurtTime < .3f)
		alpha = 255.f;
	else
		alpha -= step;

	if (alpha > 0) 
	{
		int screenSizeX, screenCenterX;
		int screenSizeY, screenCenterY;
		g_pEngine->GetScreenSize(screenSizeX, screenSizeY);

		screenCenterX = screenSizeX / 2;
		screenCenterY = screenSizeY / 2;
		Color col = Color(240, 240, 240, alpha);
		g_Render.Line(screenCenterX - lineSize * 2, screenCenterY - lineSize * 2, screenCenterX - (lineSize), screenCenterY - (lineSize), col);
		g_Render.Line(screenCenterX - lineSize * 2, screenCenterY + lineSize * 2, screenCenterX - (lineSize), screenCenterY + (lineSize), col);
		g_Render.Line(screenCenterX + lineSize * 2, screenCenterY + lineSize * 2, screenCenterX + (lineSize), screenCenterY + (lineSize), col);
		g_Render.Line(screenCenterX + lineSize * 2, screenCenterY - lineSize * 2, screenCenterX + (lineSize), screenCenterY - (lineSize), col);
	}
}

void ESP::SpectatorList()
{
	int specs = 0;
	int modes = 0;
	std::string spect = "";
	std::string mode = "";

	int screenx;
	int screeny;
	g_pEngine->GetScreenSize(screenx, screeny);

	int centerx = screenx / 2;
	int centery = screeny / 2;

	if (g_pEngine->IsInGame() && g_pEngine->IsConnected())
	{
		int localIndex = g_pEngine->GetLocalPlayer();
		if (g::pLocalEntity)
		{
			for (int i = 0; i < g_pEngine->GetMaxClients(); i++)
			{
				C_BaseEntity *pPlayerEntity = g_pEntityList->GetClientEntity(i);
				if (!pPlayerEntity)
					continue;
				if (pPlayerEntity->GetHealth() > 0)
					continue;
				if (pPlayerEntity == g::pLocalEntity)
					continue;
				if (pPlayerEntity->IsDormant())
					continue;
				if (pPlayerEntity->getObserverTarget() != g::pLocalEntity)
					continue;

				PlayerInfo_t pInfo;
				g_pEngine->GetPlayerInfo(pPlayerEntity->EntIndex(), &pInfo);
				if (pInfo.ishltv)
					continue;

				spect += pInfo.szName;
				spect += "\n";
				specs++;

				g_Render.String(screenx, screeny, 0, Color(255, 255, 255, 255), g_Fonts.pFontSmallestPixel7.get(), spect.c_str());
			}
		}
	}
}

void ESP::RenderBox(C_BaseEntity* pEnt)
{
    Vector vecScreenOrigin, vecOrigin = pEnt->GetRenderOrigin();
    if (!Utils::WorldToScreen(vecOrigin, vecScreenOrigin))
        return;

    Vector vecScreenBottom, vecBottom = vecOrigin;
    vecBottom.z += (pEnt->GetFlags() & FL_DUCKING) ? 54.f : 72.f;
    if (!Utils::WorldToScreen(vecBottom, vecScreenBottom))
        return;

    const auto sx = int(std::roundf(vecScreenOrigin.x)),
               sy = int(std::roundf(vecScreenOrigin.y)),
               h  = int(std::roundf(vecScreenBottom.y - vecScreenOrigin.y)),
               w  = int(std::roundf(h * 0.25f));

    /* Draw rect around the entity */
    g_Render.Rect(sx - w, sy, sx + w, sy + h, pEnt->GetTeam() == localTeam ? teamColor : pEnt->IsDormant() ? g_Settings.boxcolor(100) : g_Settings.boxcolor);

    /* Draw rect outline */
    g_Render.Rect(sx - w - 1, sy - 1, sx + w + 1, sy + h + 1, pEnt->IsDormant() ? Color(0, 0, 0, 65) : Color(0, 0, 0, 100));
    g_Render.Rect(sx - w + 1, sy + 1, sx + w - 1, sy + h - 1, pEnt->IsDormant() ? Color(0, 0, 0, 65) : Color(0, 0, 0, 100));
}

void ESP::RenderHealth(C_BaseEntity* pEnt)
{
	Vector vecScreenOrigin, vecOrigin = pEnt->GetRenderOrigin();
	if (!Utils::WorldToScreen(vecOrigin, vecScreenOrigin))
		return;

	Vector vecScreenBottom, vecBottom = vecOrigin;
	vecBottom.z += (pEnt->GetFlags() & FL_DUCKING) ? 54.f : 72.f;
	if (!Utils::WorldToScreen(vecBottom, vecScreenBottom))
		return;

	const auto sx = int(std::roundf(vecScreenOrigin.x)),
		sy = int(std::roundf(vecScreenOrigin.y)),
		h = int(std::roundf(vecScreenBottom.y - vecScreenOrigin.y)),
		w = int(std::roundf(h * 0.25f));

	int HPEnemy = 100;
	HPEnemy = pEnt->GetHealth();
	char nameBuffer[512];
	sprintf_s(nameBuffer, "%d", HPEnemy);
	if (HPEnemy > 100)
		HPEnemy = 100;

	if (HPEnemy >= 100)
	{
		g_Render.String(sx + w - 8.5, sy + h, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
			(localTeam == pEnt->GetTeam()) ? teamColor : enemyColor,
			g_Fonts.pFontSmallestPixel7.get(), nameBuffer);
	}
	else if (HPEnemy < 100)
	{
		g_Render.String(sx + w - 6.5, sy + h, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
			(localTeam == pEnt->GetTeam()) ? teamColor : enemyColor,
			g_Fonts.pFontSmallestPixel7.get(), nameBuffer);
	}
	else if (HPEnemy < 10)
	{
		g_Render.String(sx + w, sy + h, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
			(localTeam == pEnt->GetTeam()) ? teamColor : enemyColor,
			g_Fonts.pFontSmallestPixel7.get(), nameBuffer);
	}
}

void ESP::RenderHealthBar(C_BaseEntity* pEnt)
{
	Vector vecScreenOrigin, vecOrigin = pEnt->GetRenderOrigin();
	if (!Utils::WorldToScreen(vecOrigin, vecScreenOrigin))
		return;

	Vector vecScreenBottom, vecBottom = vecOrigin;
	vecBottom.z += (pEnt->GetFlags() & FL_DUCKING) ? 54.f : 72.f;
	if (!Utils::WorldToScreen(vecBottom, vecScreenBottom))
		return;

	const auto sx = int(std::roundf(vecScreenOrigin.x)),
		sy = int(std::roundf(vecScreenOrigin.y)),
		h = int(std::roundf(vecScreenBottom.y - vecScreenOrigin.y)),
		w = int(std::roundf(h * 0.25f));

	int enemyHp = pEnt->GetHealth(),
		hpRed = 255 - (enemyHp * 2.55),
		hpGreen = enemyHp * 2.55;

	if (enemyHp > 100)
		enemyHp = 100;

	UINT hp = h - (UINT)((h * enemyHp) / 100);
	
	g_Render.RectFilled(sx + w - 3, sy + 2, sx + w - 7, sy + h - 2, Color(0, 0, 0, pEnt->IsDormant() ? 65 : 100));
	g_Render.Rect(sx + w - 5, sy, sx + w - 5, sy + h - hp, Color(hpRed, hpGreen, 1, pEnt->IsDormant() ? 100 : 255));

	std::string s = std::to_string(enemyHp);
	char const *pchar = s.c_str();

	if (pEnt->GetHealth() < 100)
	{
		g_Render.String(sx + w - 5, sy + h - hp, CD3DFONT_CENTERED_Y | CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW, (localTeam == pEnt->GetTeam()) ? teamColor : enemyColor, g_Fonts.pFontSmallestPixel7.get(), pchar);
	}
}

bool IsWeaponGrenade5(C_BaseCombatWeapon* weapon)
{
	if (weapon == nullptr) return false;
	int id = weapon->sGetItemDefinitionIndex();
	static const std::vector<int> v = { WEAPON_FLASHBANG,WEAPON_HEGRENADE,WEAPON_SMOKEGRENADE,WEAPON_MOLOTOV,WEAPON_DECOY,WEAPON_INCGRENADE };
	return (std::find(v.begin(), v.end(), id) != v.end());
}

bool IsKnife4(C_BaseCombatWeapon* pWeapon)
{
	if (pWeapon == nullptr) return false;
	int id = pWeapon->sGetItemDefinitionIndex();
	static const std::vector<int> v = { WEAPON_KNIFE_BAYONET, WEAPON_KNIFE_SURVIVAL_BOWIE, WEAPON_KNIFE_BUTTERFLY, WEAPON_KNIFE_FALCHION, WEAPON_KNIFE_FLIP, WEAPON_KNIFE_GUT, WEAPON_KNIFE_KARAMBIT, WEAPON_KNIFE_M9_BAYONET, WEAPON_KNIFE_PUSH, WEAPON_KNIFE_TACTICAL , WEAPON_KNIFE, WEAPON_KNIFE_T };
	return (std::find(v.begin(), v.end(), id) != v.end());
}

void ESP::RenderAmmo(C_BaseEntity* pEnt)
{
	Vector vecScreenPos, vecOrigin = pEnt->GetRenderOrigin();
	if (!Utils::WorldToScreen(vecOrigin, vecScreenPos))
		return;

	Vector vecScreenBottom, vecBottom = vecOrigin;
	vecBottom.z += (pEnt->GetFlags() & FL_DUCKING) ? 54.f : 72.f;
	if (!Utils::WorldToScreen(vecBottom, vecScreenBottom))
		return;

	const auto sx = int(std::roundf(vecScreenPos.x)),
		sy = int(std::roundf(vecScreenPos.y)),
		h = int(std::roundf(vecScreenBottom.y - vecScreenPos.y)),
		w = int(std::roundf(h * 0.25f));

	C_BaseCombatWeapon* weapon = pEnt->GetActiveWeapon();
	if (!weapon)
		return;

	auto animLayer = pEnt->GetAnimOverlay(1);
	if (!animLayer.m_pOwner)
		return;

	auto activity = pEnt->GetSequenceActivity(animLayer.m_nSequence);

	if (!activity)
		return;

	int ammo1 = weapon->GetAmmo();

//	if (!ammo1)
//		return;

	if (ammo1 < 0)
		return;

	int max_clip = weapon->GetCSWpnData()->iMaxClip1;

	int ammo = 0;

	ammo = w - (int)((w * ammo1) / max_clip);

	std::string s = std::to_string(ammo1);
	char const *pchar = s.c_str();

	int visibleammo = max_clip / 4;

	if (activity == 967 && animLayer.m_flWeight != 0.f)
	{
		float cycle = animLayer.m_flCycle; // 1 = finished 0 = just started
		ammo = w - (int)((w* cycle) / 1.f);
	}
	else
	{
		if (ammo1 <= visibleammo)
			g_Render.String(sx - w - 1 + ammo * 2, sy + 10, CD3DFONT_CENTERED_Y | CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW, localTeam == pEnt->GetTeam() ? teamColor : enemyColor, g_Fonts.pFontSmallestPixel7.get(), pchar);
	}

	g_Render.RectFilled(sx + w - 2, sy + 3, sx - w + 2, sy + 7, pEnt->IsDormant() ? Color(0, 0, 0, 65) : Color(0, 0, 0, 100));
	g_Render.Rect(sx + w - 1, sy + 4, sx - w + 1 + ammo * 2, sy + 6, pEnt->GetTeam() == localTeam ? teamColor : pEnt->IsDormant() ? g_Settings.ammocolor(100) : g_Settings.ammocolor);

	
	//sx + w - 5, sy, sx + w - 5, sy + h - hp, Color(hpRed, hpGreen, 1, pEnt->IsDormant() ? 100 : 255));
	/*
	std::string am = "(" + std::to_string(ammo1) + " / " + std::to_string(ammo2) + ")";

	if (!g_Settings.bShowWeaponName && !g_Settings.bShowWeaponIcon)
	{
		g_Render.String(vecScreenPos.x, vecScreenPos.y + 2, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
			(localTeam == pEnt->GetTeam()) ? teamColor : enemyColor,
			g_Fonts.pFontRando.get(), am.c_str());
	}
	else if (g_Settings.bShowWeaponName && !g_Settings.bShowWeaponIcon)
	{
		g_Render.String(vecScreenPos.x, vecScreenPos.y + 12, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
			(localTeam == pEnt->GetTeam()) ? teamColor : enemyColor,
			g_Fonts.pFontRando.get(), am.c_str());
	}
	else if (g_Settings.bShowWeaponName && g_Settings.bShowWeaponIcon)
	{
		g_Render.String(vecScreenPos.x, vecScreenPos.y + 22, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
			(localTeam == pEnt->GetTeam()) ? teamColor : enemyColor,
			g_Fonts.pFontRando.get(), am.c_str());
	}
	else if (!g_Settings.bShowWeaponName && g_Settings.bShowWeaponIcon)
	{
		g_Render.String(vecScreenPos.x, vecScreenPos.y + 14, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
			(localTeam == pEnt->GetTeam()) ? teamColor : enemyColor,
			g_Fonts.pFontRando.get(), am.c_str());
	}*/
}

void ESP::RenderName(C_BaseEntity* pEnt, int iterator)
{
    Vector vecScreenOrigin, vecOrigin = pEnt->GetRenderOrigin();
    if (!Utils::WorldToScreen(vecOrigin, vecScreenOrigin))
        return;

    Vector vecScreenBottom, vecBottom = vecOrigin;
    vecBottom.z += (pEnt->GetFlags() & FL_DUCKING) ? 54.f : 72.f;
    if (!Utils::WorldToScreen(vecBottom, vecScreenBottom))
        return;


    PlayerInfo_t pInfo;
    g_pEngine->GetPlayerInfo(iterator, &pInfo);

   auto sx = int(std::roundf(vecScreenOrigin.x)),
        sy = int(std::roundf(vecScreenOrigin.y)),
        h  = int(std::roundf(vecScreenBottom.y - vecScreenOrigin.y));

    g_Render.String(sx, sy + h - 12, CD3DFONT_CENTERED_X, pEnt->GetTeam() == localTeam ? teamColor : pEnt->IsDormant() ? g_Settings.namecolor(100) : g_Settings.namecolor, g_Fonts.pFontInfo.get(), pInfo.szName);
}

void ESP::RenderWeaponName(C_BaseEntity* pEnt)
{
	Vector vecScreenOrigin, vecOrigin = pEnt->GetRenderOrigin();
	if (!Utils::WorldToScreen(vecOrigin, vecScreenOrigin))
        return;

	C_BaseCombatWeapon* weapon = pEnt->GetActiveWeapon();
	if (!weapon)
		return;

	int ammo = weapon->GetAmmo();
	bool ya = false;
	if (ammo < 0)
		ya = true;

	if (g_Settings.bShowWeaponName && !g_Settings.bShowWeaponIcon)
	{
		if (ya)
		{
			g_Render.String(int(vecScreenOrigin.x), int(vecScreenOrigin.y) + 1, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
				(pEnt->GetTeam() == localTeam ? teamColor : pEnt->IsDormant() ? enemyColor(100) : enemyColor(255)),
				g_Fonts.pFontSmallestPixel7.get(), weapon->GetWeaponName());
		}
		else
		{
			g_Render.String(int(vecScreenOrigin.x), int(vecScreenOrigin.y) + 6, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
				(pEnt->GetTeam() == localTeam ? teamColor : pEnt->IsDormant() ? enemyColor(100) : enemyColor(255)),
				g_Fonts.pFontSmallestPixel7.get(), weapon->GetWeaponName());
		}
	}
	else if (g_Settings.bShowWeaponIcon && !g_Settings.bShowWeaponName)
	{
		g_Render.String(int(vecScreenOrigin.x), int(vecScreenOrigin.y) + 13, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
			(pEnt->GetTeam() == localTeam ? teamColor : pEnt->IsDormant() ? enemyColor(100) : enemyColor(255)),
			g_Fonts.pFontIcons.get(), weapon->GunIconCustom());
	}
	else if (g_Settings.bShowWeaponName && g_Settings.bShowWeaponIcon)
	{
		if (ya)
		{
			g_Render.String(int(vecScreenOrigin.x), int(vecScreenOrigin.y) + 1, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
				(pEnt->GetTeam() == localTeam ? teamColor : pEnt->IsDormant() ? enemyColor(100) : enemyColor(255)),
				g_Fonts.pFontSmallestPixel7.get(), weapon->GetWeaponName());

			g_Render.String(int(vecScreenOrigin.x), int(vecScreenOrigin.y) + 16, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
				(pEnt->GetTeam() == localTeam ? teamColor : pEnt->IsDormant() ? enemyColor(100) : enemyColor(255)),
				g_Fonts.pFontIcons.get(), weapon->GunIconCustom());
		}
		else
		{
			g_Render.String(int(vecScreenOrigin.x), int(vecScreenOrigin.y) + 6, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
				(pEnt->GetTeam() == localTeam ? teamColor : pEnt->IsDormant() ? enemyColor(100) : enemyColor(255)),
				g_Fonts.pFontSmallestPixel7.get(), weapon->GetWeaponName());

			g_Render.String(int(vecScreenOrigin.x), int(vecScreenOrigin.y) + 19, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW,
				(pEnt->GetTeam() == localTeam ? teamColor : pEnt->IsDormant() ? enemyColor(100) : enemyColor(255)),
				g_Fonts.pFontIcons.get(), weapon->GunIconCustom());
		}		
	}
}

void ESP::DrawSkeleton(C_BaseEntity* pEntity)
{
	studiohdr_t* pStudioHdr = g_pModelInfo->GetStudiomodel(pEntity->GetModel());

	if (!pStudioHdr)
		return;

	Vector vParent, vChild, sParent, sChild;

	for (int j = 0; j < pStudioHdr->numbones; j++)
	{
		mstudiobone_t* pBone = pStudioHdr->pBone(j);

		if (!pBone)
			return;

		if (pBone && (pBone->flags & BONE_USED_BY_HITBOX) && (pBone->parent != -1))
		{
			vChild = pEntity->GetBonePosition(j);
			vParent = pEntity->GetBonePosition(pBone->parent);

			if (vChild == Vector(0,0,0))
				return;

			if (vParent == Vector(0, 0, 0))
				return;

			if (Utils::WorldToScreen(vParent, sParent) && Utils::WorldToScreen(vChild, sChild))
			{
				g_Render.Line(sParent[0], sParent[1], sChild[0], sChild[1], g_Settings.skeletoncolor);
			}
		}
	}
}

void ESP::DroppedWeapons(C_BaseEntity * entity)
{
	Vector vecScreenPos, vecOrigin = entity->GetRenderOrigin();
	if (!Utils::WorldToScreen(vecOrigin, vecScreenPos))
		return;

	Vector vecScreenBottom, vecBottom = vecOrigin;
	vecBottom.z += (entity->GetFlags() & FL_DUCKING) ? 54.f : 72.f;
	if (!Utils::WorldToScreen(vecBottom, vecScreenBottom))
		return;

	const auto sx = int(std::roundf(vecScreenPos.x)),
		sy = int(std::roundf(vecScreenPos.y)),
		h = int(std::roundf(vecScreenBottom.y - vecScreenPos.y)),
		w = int(std::roundf(h * 0.25f));

	auto * weapon = reinterpret_cast<C_BaseCombatWeapon*>(entity);

	if (!weapon)
		return;

	if (weapon->IsDormant())
		return;

	auto class_id = entity->GetClientClass()->m_ClassID;

	if (class_id == (int)EClassIds::CBaseCSGrenadeProjectile || class_id == (int)EClassIds::CMolotovProjectile 
		|| class_id == (int)EClassIds::CDecoyProjectile || class_id == (int)EClassIds::CSmokeGrenadeProjectile
		|| class_id == (int)EClassIds::CSensorGrenadeProjectile || class_id == (int)EClassIds::CBreachChargeProjectile)
		return;

	if (weapon && !(entity->GetVecOrigin().x == 0 && entity->GetVecOrigin().y == 0 && entity->GetVecOrigin().z == 0))
	{
		WeaponInfo_t* weapon_data = weapon->GetCSWpnData();

		if (!weapon_data)
			return;

		std::string weaponName = weapon->GetWeaponName();

		int ammo1 = weapon->GetAmmo();

		if (ammo1 < 0)
			return;

		int max_clip = weapon->GetCSWpnData()->iMaxClip1;

		int ammo = 0;

		ammo = w - (int)((w * ammo1) / max_clip);

		if(g_Settings.bDroppedItems[0])
		g_Render.String(sx, sy + 9, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW, g_Settings.droppedweaponname, g_Fonts.pFontSmallestPixel7.get(), weaponName.c_str());

		if (g_Settings.bDroppedItems[1])
		{
			g_Render.RectFilled(sx + w - 2, sy + 7, sx - w + 1, sy + 11, Color(0, 0, 0, 100));
			g_Render.Rect(sx + w - 1, sy + 8, sx - w + ammo * 2, sy + 10, g_Settings.ammocolor);
		}
	}
}

void ESP::DrawProjectiles(C_BaseEntity * entity)
{
	const model_t* model = entity->GetModel();

	if (!model)
		return;

	if (entity->IsDormant())
		return;

	const studiohdr_t* hdr = g_pModelInfo->GetStudiomodel(model);

	if (!hdr)
		return;

	const auto client_class = entity->GetClientClass();

	if (!client_class)
		return;

	std::string to_render = "error";

	switch (client_class->m_ClassID)
	{
	case (int)EClassIds::CMolotovProjectile:
		to_render = "l";
		break;
	case (int)EClassIds::CSmokeGrenadeProjectile:
		to_render = "k";
		break;
	case (int)EClassIds::CDecoyProjectile:
		to_render = "m";
		break;
	case (int)EClassIds::CFlashbang:
		to_render = "i";
		break;
	case (int)EClassIds::CBaseCSGrenadeProjectile:
		if (hdr->name[16] == 'j')
			to_render = "j";
		else
			to_render = "j";
		break;
	default:
		break;
	}

	Vector screen;

	if (Utils::WorldToScreen(entity->GetAbsOrigin(), screen))
	{
		g_Render.String(screen.x, screen.y + 5, CD3DFONT_CENTERED_X | CD3DFONT_DROPSHADOW, g_Settings.projectilecolor, g_Fonts.pFontIcons13.get(), to_render.c_str());
	}
}
void ESP::Render()
{
    if (!g::pLocalEntity || !g_pEngine->IsInGame())
        return;

    localTeam = g::pLocalEntity->GetTeam();

    for (int it = 1; it <= g_pEngine->GetMaxClients(); ++it)
    {
        C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(it);

        if (!pPlayerEntity
            || !pPlayerEntity->IsAlive()
			|| pPlayerEntity == g::pLocalEntity)
            continue;

		if (!g_Settings.bShowDormant && pPlayerEntity->IsDormant())
		{
			continue;
		}
		else
			enemyColor = Color(230, 230, 230, 100);

		teamColor = Color (150, 200, 60, 255);
		enemyColor = Color(230, 230, 230, 255);
		
		if (pPlayerEntity->GetTeam() == g::pLocalEntity->GetTeam() && !g_Settings.bShowTeammates)
			continue;

        if (g_Settings.bShowBoxes)
            this->RenderBox(pPlayerEntity);

        if (g_Settings.bShowNames)
            this->RenderName(pPlayerEntity, it);	

		if (g_Settings.bShowHealth)
			this->RenderHealthBar(pPlayerEntity);

		if(g_Settings.bShowAmmo)
			this->RenderAmmo(pPlayerEntity);

		this->RenderWeaponName(pPlayerEntity);

		if (g_Settings.bShowSkeleton)
			this->DrawSkeleton(pPlayerEntity);
    }
	for (auto i = 0; i < g_pEntityList->GetHighestEntityIndex(); i++)
	{
		C_BaseEntity* entity = g_pEntityList->GetClientEntity(i);

		if (!entity || !g::pLocalEntity)
			continue;

		const auto class_id = entity->GetClientClass();

		if((strstr(class_id->pNetworkName, "Weapon") || class_id->m_ClassID == (int)EClassIds::CDEagle || class_id->m_ClassID == (int)EClassIds::CAK47))
		this->DroppedWeapons(entity);

		if (g_Settings.bShowProjectiles && class_id->m_ClassID == (int)EClassIds::CMolotovProjectile || class_id->m_ClassID == (int)EClassIds::CSmokeGrenadeProjectile
			|| class_id->m_ClassID == (int)EClassIds::CDecoyProjectile || class_id->m_ClassID == (int)EClassIds::CBaseCSGrenadeProjectile)
		DrawProjectiles(entity);
	}

	//SpectatorList();

}
