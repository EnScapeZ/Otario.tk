#pragma once
#include "GUI\GUI.h"
#include <filesystem>
using namespace ui;
namespace fs = std::experimental::filesystem;

class Settings
{
public:
    void Initialize(MenuMain* pMenuObj);

    void SaveSettings(const std::string& strFileName, MenuMain* pMenuObj);
    void LoadSettings(const std::string& strFileName, MenuMain* pMenuObj);

private:
    void UpdateDirectoryContent(const fs::path& fsPath);
    inline fs::path GetFolderPath();

    fs::path                 fsPath{};
    std::vector<std::string> vecFileNames{};

public:
    /* All our settings variables will be here  *
    * The best would be if they'd get          *
    * initialized in the class itself.         */

	Color chamsvisible = { 150, 200, 60, 255 };
	Color chamshidden = { 60, 120, 180, 255 };
	float chamslocal[3];

	Color glowcolorlocal = Color(255, 255, 255, 255);
	Color glowcolorother = Color(255, 255, 255, 255);

	Color boxcolor = Color(230, 230, 230, 255);
	Color namecolor = Color(230, 230, 230, 255);
	Color skeletoncolor = Color(230, 230, 230, 255);
	Color ammocolor = Color(230, 230, 230, 255);
	Color projectilecolor = Color(230, 230, 230, 255);
	Color projectiledroppedcolor = Color(230, 230, 230, 255);
	Color bullettracer = Color(238, 130, 238, 255);
	Color droppedweaponname = Color(230, 230, 230, 255);
	Color GunIconCustom = Color(230, 230, 230, 255);
	//int weapon_skin[skin_changer.NUM_WEAPONS];
	//int weapon_quality[skin_changer.NUM_WEAPONS];
//	float weapon_wear[skin_changer.NUM_WEAPONS];
//	float weapon_seed[skin_changer.NUM_WEAPONS];
	//bool weapon_stat_trak_enabled[skin_changer.NUM_WEAPONS];
//	float weapon_stat_trak_kills[skin_changer.NUM_WEAPONS];
	//char weapon_custom_name[skin_changer.NUM_WEAPONS][PPGUI::PPGUI_MAX_STRING_LENGTH];

    bool  bCheatActive = true;
    bool  bMenuOpened  = false;
    bool  bShowBoxes   = true;
    bool  bShowNames   = true;
	bool  bShowHealth = true;
	bool  bShowChams   = true;
	bool  bShowChamsXQZ = true;
	bool  bShowLocalChams = false;
	bool  bShowLocalChamsXQZ = false;
	bool  bShowGlow    = true;
	bool  bShowAmmo = true;
	bool  bShowSkeleton = false;
	bool  bShowTeammates = false;
	bool  bShowWeaponName = true;
	bool  bShowWeaponIcon = true;
	bool  bShowProjectiles = true;
	bool  bShowDormant = false;
	int   iHitSound = 4;

	bool bHitboxSelection[7];
	bool bMultiPointSelection[7];
	bool bFakeLagFlags[4];	
	bool bDroppedItems[2];
	bool bItemGlow[2];
	bool bBulletTracers = true;
	bool bBulletImpacts = true;
	bool bKnifeLeft = false;
	bool bFixLegMovement = true;
	bool bSlideWalk = false;
	bool bSilentAim = true;
	bool bAutoFire  = true;
	bool bFriendlyFire = false;
	bool bAimBotEnabled = true;
	bool bForceUpdate;
	bool bPreferBodyAim = false;
	bool bAutoWallEnabled = false;
	bool bRemoveRecoil = true;
	bool bBhopEnabled = true;
	bool bAutomaticWeapons = true;
	bool bAutomaticRevolver = true;
	bool bNoFlash = true;
	bool bEngineCrosshair = true;
	bool bNoSmoke = true;
	bool bAirStrafer = true;
	bool bAntiAim = true;
	bool AntiUT = true;
	bool bThirdPersonDead = true;
	bool bPreserveKillFeed = false;
	float flTransparentWorld = 100.f;
	float flTransparentProps = 100.f;
	bool bKillSay = false;
	bool bClantag = false;
	bool bDamageLogs = true;
	bool bSkinChangerWindow = false;
	bool bScopeNoZoom = true;
	bool bHitmaker = true;
	bool bUselessInfo = false;
	int  iMinSpeedAmmount = 0;
	bool bFakeLagShooting = false;
	bool bFakeLagBreak = true;
	bool bAutoStop = false;
	bool bAutoRevolver = true;
	int iDelayShot = 0;
	int iBodyAimIfxDamage = 0;
	int iChamsMode = 1;
	int iImpactTime = 4;
	int iChamsModeTeam = 0;	
	int iLocalChamsMode = 0;
	int iGlowStyle = 1;
	int iLocalGlowStyle = 0;
	bool bNightMode = false;
	bool bNoSky = false;
	int iMinDamage = 30;
	int iMinAutoWallDamage = 0;
	int iMaxFov = 0;
	int iHitChance = 60;
	int iHitScanMode = 1;
	int iMultiPointAmount = 0;
	int iBodyAimOptions = 0;
	int iHitChanceType = 1;
	int iThirdPersonKey = 1;
	int iThirdPersonDistance = 150;
	int iScopedBlend = 65;
	int iFakeLagType = 2;
	int iFakeLagChoke = 14;
	int iPitch = 1;
	int iYaw = 2;
	bool bYawAtTargets = true;

	int iPointHeadScale;
	int iPointBodyScale;

	int iHeadScale = 90.f;
	int iNeckScale = 50.f;
	int iChestScale = 50.f;
	int iStomachScale = 50.f;
	int iPelvisScale = 50.f;
	int iArmsScale = 50.f;
	int iLegsScale = 50.f;

	
	bool bLegitBotMaster = true;
	bool bLegitNoRecoil = false;
	bool bLegitAutoFire = false;
	bool bLegitAimAccBoost = false;
	bool bLegitOnShot = false;
	bool bLegitSilentAim = false;
	float flFovLegit = .5f;
	int iLegitHitBox = 0;
	bool bLegitBacktrack = true;
	int iWeaponType;


	int iKnifeModel = 5;
	bool bKnifeChanger = true;
	bool bSkinChanger = true;
};

extern Settings g_Settings;

