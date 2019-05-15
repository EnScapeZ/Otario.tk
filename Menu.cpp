#include "GUI\GUI.h"
#include "Settings.h"
#include "SDK\ConVar.h"

void Detach() 
{
	g_Settings.bCheatActive = false; 
}


template<class T>
static T* FindHudElement(const char* name)
{
	static auto pThis = *reinterpret_cast<DWORD**> (Utils::FindSignature("client_panorama.dll", "B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08") + 1); //(Utilities::Memory::FindPatternV2("client_panorama.dll", "B9 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 46 24") + 1);

	static auto find_hud_element = reinterpret_cast<DWORD(__thiscall*)(void*, const char*)>(Utils::FindSignature("client_panorama.dll", "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28"));
	return (T*)find_hud_element(pThis, name);
}

struct hud_weapons_t
{
	std::int32_t* get_weapon_count()
	{
		return reinterpret_cast<std::int32_t*>(std::uintptr_t(this) + 0x80);
	}
};

void KnifeApplyCallbk()
{
	static auto clear_hud_weapon_icon_fn = reinterpret_cast<std::int32_t(__thiscall*)(void*, std::int32_t)>(Utils::FindSignature(("client_panorama.dll"), ("55 8B EC 51 53 56 8B 75 08 8B D9 57 6B FE 2C")));

	auto element = FindHudElement<std::uintptr_t*>(("CCSGO_HudWeaponSelection"));

	auto hud_weapons = reinterpret_cast<hud_weapons_t*>(std::uintptr_t(element) - 0xA0);
	if (hud_weapons == nullptr)
		return;

	if (!*hud_weapons->get_weapon_count())
		return;

	for (std::int32_t i = 0; i < *hud_weapons->get_weapon_count(); i++)
		i = clear_hud_weapon_icon_fn(hud_weapons, i);

	g_pClientState->ForceFullUpdate();
}

void MenuMain::Initialize()
{
	/* Create our main window (Could have multiple if you'd create vec. for it) */
	auto mainWindow = std::make_shared<Window>("lowercase", SSize(521, 611), g_Fonts.pFontTahoma8, g_Fonts.pFontTahoma8);
	{
		///TODO: window->AddTab()
		auto tab1 = std::make_shared<Tab>("legit", 2, mainWindow);
		{
			/* Create sections for it */
			auto sectMain = tab1->AddSection("LegitMainSect", 1.5f);
			{
				sectMain->AddCheckBox("enabled", &g_Settings.bLegitBotMaster);
				sectMain->AddCheckBox("on shot", &g_Settings.bLegitOnShot);
				sectMain->AddCheckBox("auto fire", &g_Settings.bLegitAutoFire);
				sectMain->AddCheckBox("silent", &g_Settings.bLegitSilentAim);
				sectMain->AddCheckBox("remove recoil", &g_Settings.bLegitNoRecoil);
				sectMain->AddSlider("minimum fov", &g_Settings.flFovLegit, 0.f, 180.f);
				sectMain->AddCombo("hitbox", &g_Settings.iLegitHitBox, std::vector<std::string>{ "head", "neck", "stomach", "chest", "pelvis" });
				sectMain->AddCheckBox("accuracy boost", &g_Settings.bLegitBacktrack);
				sectMain->AddCheckBoxUntrusted("aim at accuracy boost", &g_Settings.bLegitAimAccBoost);
			}

			auto sectMain2 = tab1->AddSection("Legit2Sect", 1.f);
			{
				sectMain2->AddCombo("weapon filter", &g_Settings.iWeaponType, std::vector<std::string>{ "awp", "scout", "auto", "rifle", "heavy", "pistol" });
			}

		} mainWindow->AddChild(tab1);

		auto tab2 = std::make_shared<Tab>("rage", 2, mainWindow);
		{
			/* Create sections for it */
			auto sectMain = tab2->AddSection("RageMainSect", 1.5f);
			{
				/* Add controls within section */
				sectMain->AddCheckBox("enabled", &g_Settings.bAimBotEnabled);
				sectMain->AddCheckBox("automatic fire", &g_Settings.bAutoFire);
				sectMain->AddCheckBox("silent aimbot", &g_Settings.bSilentAim);
				sectMain->AddCombo("hitchance type", &g_Settings.iHitChanceType, std::vector<std::string>{ "off", "hitchance", "spread limit" });
				sectMain->AddSlider("hitchance", &g_Settings.iHitChance, 0, 100);
				sectMain->AddSlider("minimum damage", &g_Settings.iMinDamage, 1, 100);
				sectMain->AddText("hitbox selection");
				sectMain->AddMulti("hitbox selection", g_Settings.bHitboxSelection, std::vector<std::string>
				{
						"head",
						"neck",
						"chest",
						"stomach",
						"pelvis",
						"arms",
						"legs"
				});
				sectMain->AddText("multipoint selection");
				sectMain->AddMulti("multipoint selection", g_Settings.bMultiPointSelection, std::vector<std::string>
				{
						"head",
						"neck", 
						"chest",
						"stomach",
						"pelvis",
						"arms",
						"legs"
				});
				sectMain->AddSlider("head scale", &g_Settings.iHeadScale, 0, 100);
				sectMain->AddSlider("neck scale", &g_Settings.iNeckScale, 0, 100);
				sectMain->AddSlider("chest scale", &g_Settings.iChestScale, 0, 100);
				sectMain->AddSlider("stomach scale", &g_Settings.iStomachScale, 0, 100);
				sectMain->AddSlider("pelvis scale", &g_Settings.iPelvisScale, 0, 100);
				sectMain->AddSlider("arms scale", &g_Settings.iArmsScale, 0, 100);
				sectMain->AddSlider("legs scale", &g_Settings.iLegsScale, 0, 100);
			}

			auto sectMain2 = tab2->AddSection("Rage2Sect", 1.f);
			{
				sectMain2->AddSlider("body aim after x shots", &g_Settings.iBodyAimIfxDamage, 0, 10);
				sectMain2->AddCombo("body aim", &g_Settings.iBodyAimOptions, std::vector<std::string>{ "off", "inaccurate", "always" });
				sectMain2->AddCheckBox("auto stop", &g_Settings.bAutoStop);
				sectMain2->AddCheckBox("auto revolver", &g_Settings.bAutoRevolver);
			}

		} mainWindow->AddChild(tab2);

		auto tab3 = std::make_shared<Tab>("anti hit", 2, mainWindow);
		{
			/* Create sections for it */
			auto sectMain = tab3->AddSection("AntiHitMainSect", 1.5f);
			{
				/* Add controls within section */
				sectMain->AddCheckBox("enabled", &g_Settings.bAntiAim);
				sectMain->AddCombo("pitch", &g_Settings.iPitch, std::vector<std::string>{ "off", "default", "down", "up", "zero", "random" });
				sectMain->AddCombo("yaw", &g_Settings.iYaw, std::vector<std::string>{ "off", "backwards", "freestanding" });
				sectMain->AddCheckBox("at targets", &g_Settings.bYawAtTargets);
			}
			 
			auto sectMain2 = tab3->AddSection("AntiHit2Sect", 1.f);
			{
				sectMain2->AddCombo("fakelag", &g_Settings.iFakeLagType, std::vector<std::string>{ "off", "always", "dynamic", "switch" });
				sectMain2->AddSlider("fakelag choke", &g_Settings.iFakeLagChoke, 0, 14);
				sectMain2->AddText("flags");
				sectMain2->AddMulti("flags", g_Settings.bFakeLagFlags, std::vector<std::string>{
						"while standing",
						"on accelerate",
						"on high speed",
						"on peek"
				});
				sectMain2->AddCheckBox("while shooting", &g_Settings.bFakeLagShooting);
				sectMain2->AddCheckBoxUntrusted("fix leg movement", &g_Settings.bFixLegMovement);
				sectMain2->AddCheckBoxUntrusted("slide walk", &g_Settings.bSlideWalk);
			}

		} mainWindow->AddChild(tab3);

		auto tab4 = std::make_shared<Tab>("players", 2, mainWindow);
		{
			/* Create sections for it */
			auto sectMain = tab4->AddSection("PlayersMainSect", 1.5f);
			{
				
			}

			auto sectMain2 = tab4->AddSection("Players2Sect", 1.f);
			{
				
			}

		} mainWindow->AddChild(tab4);

		auto tab5 = std::make_shared<Tab>("visuals", 2, mainWindow);
		{
			auto sectMain = tab5->AddSection("VisualsMainSect", 1.f);
			{
				/* Add controls within section */
				sectMain->AddCheckBox("teammates", &g_Settings.bShowTeammates);
				sectMain->AddCheckBox("dormant", &g_Settings.bShowDormant);
				sectMain->AddColor("box color", &g_Settings.boxcolor);
				sectMain->AddCheckBox("bounding box", &g_Settings.bShowBoxes);
				sectMain->AddColor("skeleton color", &g_Settings.skeletoncolor);
				sectMain->AddCheckBox("skeleton", &g_Settings.bShowSkeleton);
				sectMain->AddColor("name color", &g_Settings.namecolor);
				sectMain->AddCheckBox("names", &g_Settings.bShowNames);
				sectMain->AddCheckBox("health bar", &g_Settings.bShowHealth);
				sectMain->AddColor("ammo color", &g_Settings.ammocolor);
				sectMain->AddCheckBox("weapon ammo", &g_Settings.bShowAmmo);
				sectMain->AddCheckBox("weapon name", &g_Settings.bShowWeaponName);
				sectMain->AddCheckBox("weapon icon", &g_Settings.bShowWeaponIcon);
				sectMain->AddColor("dropped name", &g_Settings.droppedweaponname);
				sectMain->AddText("dropped weapons");
				sectMain->AddMulti("dropped weapons", g_Settings.bDroppedItems, std::vector<std::string>{
					"name",
					"ammo"
				});
				sectMain->AddColor("projectile color", &g_Settings.projectilecolor);
				sectMain->AddCheckBox("projectiles", &g_Settings.bShowProjectiles);
				sectMain->AddColor("projectile color", &g_Settings.projectiledroppedcolor);
				sectMain->AddText("item glow");
				sectMain->AddMulti("item glow", g_Settings.bItemGlow, std::vector<std::string>{
						"dropped weapons",
						"projectiles"
				});
				sectMain->AddCheckBox("night mode", &g_Settings.bNightMode);
				sectMain->AddCheckBox("remove skybox", &g_Settings.bNoSky);
				sectMain->AddSlider("world transparency", &g_Settings.flTransparentWorld, 0.f, 100.f);
				sectMain->AddSlider("prop transparency", &g_Settings.flTransparentProps, 0.f, 100.f);
				sectMain->AddCheckBox("player model", &g_Settings.bShowChams);
				sectMain->AddCheckBox("player model behind walls", &g_Settings.bShowChamsXQZ);
				sectMain->AddCombo("type", &g_Settings.iChamsMode, std::vector<std::string>{ "off", "normal", "flat", "wireframe" });
				sectMain->AddCombo("player glow", &g_Settings.iGlowStyle, std::vector<std::string>{ "off", "outline", "pulse" });
			}

			auto sectMain2 = tab5->AddSection("Visuals2Sect", .5f);
			{
				sectMain2->AddCheckBox("knife left side", &g_Settings.bKnifeLeft);
				sectMain2->AddSlider("scope blend", &g_Settings.iScopedBlend, 0, 100);
				sectMain2->AddCheckBox("no scope zoom", &g_Settings.bScopeNoZoom);
				sectMain2->AddCheckBox("no flash", &g_Settings.bNoSmoke);
				sectMain2->AddCheckBox("no smoke", &g_Settings.bNoFlash);
				sectMain2->AddCheckBoxUntrusted("engine crosshair", &g_Settings.bEngineCrosshair);
				sectMain2->AddCheckBox("hitmarker", &g_Settings.bHitmaker);
				sectMain2->AddCombo("hitsound", &g_Settings.iHitSound, std::vector<std::string>{ "off", "arena_switch", "bubble", "quake", "cod" });
				sectMain2->AddCheckBox("bullet tracers", &g_Settings.bBulletTracers);
				sectMain2->AddColor("bullet tracer", &g_Settings.bullettracer);
				sectMain2->AddCheckBox("bullet impacts", &g_Settings.bBulletImpacts);
			}

			auto sectMain3 = tab5->AddSection("Visuals3Sect", .5f);
			{
				sectMain3->AddCheckBox("local player model", &g_Settings.bShowLocalChams);
				sectMain3->AddCheckBox("local player model behind walls", &g_Settings.bShowLocalChamsXQZ);
				sectMain3->AddCombo("type", &g_Settings.iLocalChamsMode, std::vector<std::string>{ "off", "normal", "flat", "original" });
				sectMain3->AddCombo("local glow", &g_Settings.iLocalGlowStyle, std::vector<std::string>{ "off", "outline", "pulse" });
			}

		} mainWindow->AddChild(tab5);

		auto tab6 = std::make_shared<Tab>("skins", 2, mainWindow);
		{
			/* Create sections for it */
			auto sectMain = tab6->AddSection("SkinsMainSect", 1.5f);
			{
				sectMain->AddButton("full update", KnifeApplyCallbk);
				sectMain->AddCombo("knife", &g_Settings.iKnifeModel, std::vector<std::string>
				{ "off", "bayonet", "flip", "gut", "karambit",
				  "m9 bayonet", "huntsman", "falchion", "bowie",
				  "butterfly", "shadow daggers", "ursus", "navaja",
				  "stiletto", "talon" });
			}

			auto sectMain2 = tab6->AddSection("Skins2Sect", 1.f);
			{

			}

		} mainWindow->AddChild(tab6);

		auto tab7 = std::make_shared<Tab>("misc", 2, mainWindow);
		{
			auto sectMain = tab7->AddSection("MiscMainSect", 1.f);
			{
				sectMain->AddCheckBox("bunnyhop", &g_Settings.bBhopEnabled);
				sectMain->AddCheckBox("airstrafe", &g_Settings.bAirStrafer);
				sectMain->AddCombo("thirdperson", &g_Settings.iThirdPersonKey, std::vector<std::string>{ "off", "t", "mouse3", "mouse4" });
				sectMain->AddSlider("thirdperson distance", &g_Settings.iThirdPersonDistance, 0, 300);
				sectMain->AddCheckBox("thirdperson (dead)", &g_Settings.bThirdPersonDead);
				sectMain->AddCheckBox("preserve killfeed", &g_Settings.bPreserveKillFeed);
				sectMain->AddCheckBox("log damage", &g_Settings.bDamageLogs);
				sectMain->AddCheckBox("useless info", &g_Settings.bUselessInfo);
				sectMain->AddCheckBox("clantag spammer", &g_Settings.bClantag);
				sectMain->AddCheckBox("Bored kill say", &g_Settings.bKillSay);
			}

			auto sectMain2 = tab7->AddSection("Misc2Sect", 1.f);
			{
				sectMain2->AddButton("shutdown", Detach);
				sectMain2->AddCheckBox("skinchanger", &g_Settings.bSkinChangerWindow);
				sectMain2->AddCheckBox("automatic weapons", &g_Settings.bAutomaticWeapons);
				sectMain2->AddCheckBox("automatic revolver", &g_Settings.bAutomaticRevolver);
				sectMain2->AddDummy();
				sectMain2->AddDummy();
				sectMain2->AddDummy();
				sectMain2->AddDummy();
				sectMain2->AddCheckBoxUntrusted("anti untrusted", &g_Settings.AntiUT);
			}

		} mainWindow->AddChild(tab7);
	}
	this->vecChildren.push_back(mainWindow);

	auto tab6 = std::make_shared<Tab>("config", 2, mainWindow);
	{
		/* Create sections for it */
		auto sectMain = tab6->AddSection("ConfigMainSect", 1.5f);
		{

		}

		auto sectMain2 = tab6->AddSection("Config2Sect", 1.f);
		{

		}

	} mainWindow->AddChild(tab6);


	/*auto skinWindow = std::make_shared<Window>("skinchanger", SSize(280, 370), g_Fonts.pFontTahoma8, g_Fonts.pFontTahoma8);
	{
		auto tab1 = std::make_shared<Tab>("weapons", 1, skinWindow);
		{
			static int selected_weapon = 0;
			auto sectMainWeap = tab1->AddSection("MiscMainSect", 1.f);
			{
				static int selected_weapon = 0;
			//	sectMainWeap->AddCombo("Weapons", &selected_weapon, std::vector<std::string>{ skin_changer.NUM_WEAPONS });
			}
		} skinWindow->AddChild(tab1);

		auto tab2 = std::make_shared<Tab>("knife", 1, skinWindow);
		{

		} skinWindow->AddChild(tab2);

		auto tab3 = std::make_shared<Tab>("gloves", 1, skinWindow);
		{

		} skinWindow->AddChild(tab3);

		auto tab4 = std::make_shared<Tab>("custom", 1, skinWindow);
		{

		} skinWindow->AddChild(tab4);
	}
	this->vecChildren.push_back(skinWindow);*/

	/* Create our mouse cursor (one instance only) */
	mouseCursor = std::make_unique<MouseCursor>();

	/* Do the first init run through all of the objects */
	for (auto& it : vecChildren)
		it->Initialize();
}