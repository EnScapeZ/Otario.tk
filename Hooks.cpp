#include <thread>
#include "Hooks.h"
#include "Utils\Utils.h"
#include "Features\Features.h"

Misc     g_Misc;
Hooks    g_Hooks;
Settings g_Settings;
static CFixMove *FixMoveManager = new CFixMove();
Vector AngleReal = Vector(0, 0, 0);
float  AngleRealY = 0.f;

const unsigned short INVALID_STRING_INDEX = (unsigned short)-1;

bool PrecacheModel(const char* szModelName)
{
	INetworkStringTable* m_pModelPrecacheTable = g_pNetworkString->FindTable("modelprecache");

	if (m_pModelPrecacheTable)
	{
		g_pModelInfo->FindOrLoadModel(szModelName);
		int idx = m_pModelPrecacheTable->AddString(false, szModelName);
		if (idx == INVALID_STRING_INDEX)
			return false;
	}
	return true;
}


void PrecacheModels()
{
	PrecacheModel("models/weapons/v_ak47beast.mdl");
	//PrecacheModel("models/weapons/v_minecraft_pickaxe.mdl");
	PrecacheModel("models/weapons/v_cod9_ballista.mdl");
}

void LogEvents()
{	
	static bool convar_performed = false, convar_lastsetting;
	//--- Log Events ---//
	static auto developer = g_pConVar->FindVar("developer");
	developer->SetValue("1");
	static auto con_filter_text_out = g_pConVar->FindVar("con_filter_text_out");
	static auto con_filter_enable = g_pConVar->FindVar("con_filter_enable");
	static auto con_filter_text = g_pConVar->FindVar("con_filter_text");

	con_filter_text->SetValue("    ");
	con_filter_text_out->SetValue("");
	con_filter_enable->SetValue("2");
	convar_performed = true;
}

void EngineCrosshair()
{
	static ConVar* weapon_debug_spread_show = g_pConVar->FindVar("weapon_debug_spread_show");

	if (g_Settings.bEngineCrosshair)
	{
		weapon_debug_spread_show->SetValue("3");
	}
	else
	{
		
		weapon_debug_spread_show->SetValue("0");
	}
}

void NoScopeOverlay()
{
	g::pLocalEntity = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
	if (!g::pLocalEntity)
		return; 

	C_BaseCombatWeapon* pWeapon = g::pLocalEntity->GetActiveWeapon();
	if (!pWeapon)
		return;

	int screenx;
	int screeny;
	g_pEngine->GetScreenSize(screenx, screeny);

	int centerx = screenx / 2;
	int centery = screeny / 2;

	float spread = pWeapon->GetInaccuracy() * 300;
	int height = std::clamp(spread, 1.f, 25.f);
	int alpha = 255 - (height * 7.5f);

	if (g::pLocalEntity->IsScoped())
	{
		g_Render.RectFilled(0, centery - (height / 2), screenx, centery + (height / 2), Color(0, 0, 0, alpha));
		g_Render.RectFilled(centerx + (height / 2), 0, centerx - (height / 2), screeny, Color(0, 0, 0, alpha));

		g_Render.Rect(0, centery - (height / 2), screenx, centery + (height / 2), Color(0, 0, 0, alpha));
		g_Render.Rect(centerx + (height / 2), 0, centerx - (height / 2), screeny, Color(0, 0, 0, alpha));
	//	csgo->surface()->filled_rect(0, center.y - (height / 2), screen.x, height, color(0, 0, 0, alpha));
	//	csgo->surface()->filled_rect(center.x - (height / 2), 0, height, screen.y, color(0, 0, 0, alpha));
	}
}


void watermark(int Width, int Height, INetChannelInfo* nci)
{
	int tickrate = 1.f / g_pGlobalVars->intervalPerTick;
	SIZE sz;
	time_t current = time(0);
	std::string nomezao = (" BORED | ");
	std::string usernaimezao = ("admin | ");
	std::string fpszao = "fps: " + std::to_string(static_cast<int>(1.f / g_pGlobalVars->frametime)) + " | ";
	std::string pingzao = "ms: " + std::to_string((int)(nci->GetAvgLatency(FLOW_INCOMING) + nci->GetAvgLatency(FLOW_OUTGOING) * 1000)) + " | ";
	std::string rate = "rt: " + std::to_string(tickrate);

	std::string infozao = nomezao + usernaimezao + fpszao.c_str() + pingzao.c_str() + rate.c_str();

	g_Fonts.pFontRando->GetTextExtent(infozao.c_str(), &sz);

	g_Render.RectFilled(Width - sz.cx - 15, 7, Width - 7, 28, Color(255, 0, 0, 255));
	g_Render.String(Width - sz.cx - 13, 11, CD3DFONT_DROPSHADOW, Color(255, 255, 255, 180), g_Fonts.pFontRando.get(), infozao.c_str());
	g_Render.Rect(Width - sz.cx - 15, 7, Width - 7, 28, Color(0, 0, 0, 40));
	g_Render.Rect(Width - sz.cx - 16, 6, Width - 6, 29, Color(0, 0, 0, 40));
}

void damagelogscreen(int centerX, int CenterY)
{
	static float alpha = 0;
	float step = 255.f / 0.3f *g_pGlobalVars->frametime;

	if (g_pGlobalVars->realtime - Globals::flHurtTime < .3f)
		alpha = 255.f;
	else
		alpha -= step;

	if (alpha > 0)
	{
		std::string damage2 = "-" + std::to_string(Globals::damage) + " hp";
		SIZE sz;
		g_Fonts.pFontRando->GetTextExtent(damage2.c_str(), &sz);

		g_Render.String(centerX - sz.cx / 2, CenterY - 30, CD3DFONT_DROPSHADOW, Color(255, 255, 255, alpha), g_Fonts.pFontRando.get(), damage2.c_str());
	}
}
void DrawUselessInfo()
{
	g::pLocalEntity = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
	if (!g::pLocalEntity)
		return;

	if (!g::pLocalEntity->IsAlive())
		return;

	int Width;
	int Height;
	g_pEngine->GetScreenSize(Width, Height);

	int centerX = Width / 2;
 	int CenterY = Height / 2;
			
	auto nci = g_pEngine->GetNetChannelInfo(); 


	if (g_Settings.bUselessInfo)
	{
		int fps = 1.f / g_pGlobalVars->frametime;
		int ping = nci->GetAvgLatency(FLOW_INCOMING) + nci->GetAvgLatency(FLOW_OUTGOING) * 1000;

		if (1.f / g_pGlobalVars->frametime > 1.f / TICKS_TO_TIME(1))
			Globals::playable = true;

		if (g::pLocalEntity->GetVelocity().Length2D() >= 255.f && g::pLocalEntity->GetVelocity().Length2D() < 290.f)
			g_Render.String(5, CenterY + 15, CD3DFONT_DROPSHADOW, Color(255, 0, 0, 255), g_Fonts.pFontInfo.get(), "LC");
		else if (g::pLocalEntity->GetVelocity().Length2D() >= 290.f)
			g_Render.String(5, CenterY + 15, CD3DFONT_DROPSHADOW, Color(0, 255, 0, 255), g_Fonts.pFontInfo.get(), "LC");

		if (g_Settings.iYaw == 2 && g_Settings.bAntiAim)
			g_Render.String(5, CenterY, CD3DFONT_DROPSHADOW, Color(0, 255, 0, 255), g_Fonts.pFontInfo.get(), "AUTO");

		if (g_Settings.bAntiAim)
		{
			g_Render.String(5, CenterY - 15, CD3DFONT_DROPSHADOW, !Globals::bsendpacket ? Color(0, 255, 0, 255) : Color(255, 0, 0, 255), g_Fonts.pFontInfo.get(), "FLAG");

			g_Render.String(5, CenterY - 30, CD3DFONT_DROPSHADOW, Globals::lbybroken ? Color(0, 255, 0, 255) : Color(255, 0, 0, 255), g_Fonts.pFontInfo.get(), "LBY");
		}

		if (fps <= 40)
			g_Render.String(5, CenterY + 30, CD3DFONT_DROPSHADOW, Color(255, 0, 0, 255), g_Fonts.pFontInfo.get(), "FPS");
		if (ping >= 60 && ping < 80)
			g_Render.String(5, CenterY + 45, CD3DFONT_DROPSHADOW, Color(0, 255, 0, 255), g_Fonts.pFontInfo.get(), "PING");
		else if (ping >= 80)
			g_Render.String(5, CenterY + 45, CD3DFONT_DROPSHADOW, Color(255, 0, 0, 255), g_Fonts.pFontInfo.get(), "PING");
	}

	watermark(Width, Height, nci);
	damagelogscreen(centerX, CenterY);
	//g_Render.Rect/*FilledGent*/(Width - 310, 10, Width - 30, 50, Color(10, 9, 10, 255));
	//g_Render.Rect/*FilledGent*/(Width - 309, 11, Width - 31, 49, Color(80, 80, 80, 255));
	//g_Render.Rect/*aaaaaafaat*/(Width - 308, 12, Width - 32, 48, Color(40, 40, 40, 255));
	//g_Render.RectFilled/*aaat*/(Width - 308, 12, Width - 32, 48, Color(40, 40, 40, 255));
	//g_Render.Rect/*Feaaaaaaaa*/(Width - 302, 18, Width - 37, 43, Color(80, 80, 80, 255));
	//g_Render.Rect/*Feaaaaaaaa*/(Width - 301, 19, Width - 38, 42, Color(10, 9, 10, 255));
//	g_Render.String(Width - 245, 15, CD3DFONT_DROPSHADOW, Color(255, 255, 255, 180), g_Fonts.pFontRando.get(), ("ms: " + pingzao).c_str());
}

void Hooks::Init()
{
    // Get window handle
    while (!(g_Hooks.hCSGOWindow = FindWindowA("Valve001", nullptr)))
    {
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(50ms);
    }

    interfaces::Init();                         // Get interfaces
    g_pNetvars = std::make_unique<NetvarTree>();// Get netvars after getting interfaces as we use them

    Utils::Log("Hooking in progress...");

    // D3D Device pointer
    const uintptr_t d3dDevice = **reinterpret_cast<uintptr_t**>(Utils::FindSignature("shaderapidx9.dll", "A1 ? ? ? ? 50 8B 08 FF 51 0C") + 1);

    if (g_Hooks.hCSGOWindow)        // Hook WNDProc to capture mouse / keyboard input
        g_Hooks.pOriginalWNDProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_Hooks.hCSGOWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Hooks::WndProc)));



    // VMTHooks
    g_Hooks.pD3DDevice9Hook = std::make_unique<VMTHook>(reinterpret_cast<void*>(d3dDevice));
    g_Hooks.pClientModeHook = std::make_unique<VMTHook>(g_pClientMode);
    g_Hooks.pSurfaceHook	= std::make_unique<VMTHook>(g_pSurface);
	g_Hooks.pRenderViewHook = std::make_unique<VMTHook>(g_pRenderView);
	g_Hooks.pModelRenderHook = std::make_unique<VMTHook>(g_pModelRender);
	g_Hooks.pClientDllHook  = std::make_unique<VMTHook>(g_pClientDll);
	g_Hooks.pPanelHook		= std::make_unique<VMTHook>(g_pPanel);
	//g_Hooks.pClientModeHook = std::make_unique<VMTHook>(sv_cheats);

    // Hook the table functions
    g_Hooks.pD3DDevice9Hook->Hook(vtable_indexes::reset,      Hooks::Reset);
    g_Hooks.pD3DDevice9Hook->Hook(vtable_indexes::present,    Hooks::Present);
    g_Hooks.pClientModeHook->Hook(vtable_indexes::createMove, Hooks::CreateMove);
    g_Hooks.pSurfaceHook   ->Hook(vtable_indexes::lockCursor, Hooks::LockCursor);
	g_Hooks.pRenderViewHook->Hook(vtable_indexes::sceneend,   Hooks::SceneEnd);
	g_Hooks.pClientModeHook->Hook(vtable_indexes::postscreenfx, Hooks::DoPostScreenEffects);
	g_Hooks.pClientDllHook->Hook(vtable_indexes::framestage, Hooks::FrameStageNotify);
	g_Hooks.pClientModeHook->Hook(vtable_indexes::overrideview, Hooks::OverrideView);
	g_Hooks.pPanelHook->Hook(vtable_indexes::painttraverse, Hooks::PaintTraverse);
	//g_Hooks.pClientModeHook->Hook(vtable_indexes::getbool, Hooks::HookedGetBool);
	//g_Hooks.pClientModeHook->Hook(vtable_indexes::datagram,   Hooks::SendDatagram);
	g_Hooks.pModelRenderHook->Hook(vtable_indexes::dmeidx, Hooks::DrawModelExecute);

    // Create event listener, no need for it now so it will remain commented.
	const std::vector<const char*> vecEventNames = { "bullet_impact", "player_hurt", "round_prestart", "round_freeze_end", "item_purchase" };
    g_Hooks.pEventListener = std::make_unique<EventListener>(vecEventNames);
	//NetvarHook();
    Utils::Log("Hooking completed!");
}

void Hooks::Restore()
{
	ResetWorld();
	Utils::Log("Unhooking in progress...");
    {   // Unhook every function we hooked and restore original one
        g_Hooks.pD3DDevice9Hook->Unhook(vtable_indexes::reset);
        g_Hooks.pD3DDevice9Hook->Unhook(vtable_indexes::present);
        g_Hooks.pClientModeHook->Unhook(vtable_indexes::createMove);
        g_Hooks.pSurfaceHook->Unhook(vtable_indexes::lockCursor);
		g_Hooks.pRenderViewHook->Unhook(vtable_indexes::sceneend);
		g_Hooks.pClientModeHook->Unhook(vtable_indexes::postscreenfx);
		g_Hooks.pClientDllHook->Unhook(vtable_indexes::framestage);
		g_Hooks.pClientModeHook->Unhook(vtable_indexes::overrideview);
		g_Hooks.pPanelHook->Unhook(vtable_indexes::painttraverse);
		//g_Hooks.pClientModeHook->Unhook(vtable_indexes::getbool);
		//g_Hooks.pClientModeHook->Unhook(vtable_indexes::datagram);
		g_Hooks.pModelRenderHook->Unhook(vtable_indexes::dmeidx);
        SetWindowLongPtr(g_Hooks.hCSGOWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_Hooks.pOriginalWNDProc));
        g_pNetvars.reset();   /* Need to reset by-hand, global pointer so doesnt go out-of-scope */
    }
    Utils::Log("Unhooking succeded!");

	
    // Destroy fonts and all textures we created
    g_Render.InvalidateDeviceObjects();
    g_Fonts.DeleteDeviceObjects();
}

bool IsKnife(C_BaseCombatWeapon* pWeapon)
{
	if (pWeapon == nullptr) return false;
	int id = pWeapon->sGetItemDefinitionIndex();
	static const std::vector<int> v = { WEAPON_KNIFE_BAYONET, WEAPON_KNIFE_SURVIVAL_BOWIE, WEAPON_KNIFE_BUTTERFLY, WEAPON_KNIFE_FALCHION, WEAPON_KNIFE_FLIP, WEAPON_KNIFE_GUT, WEAPON_KNIFE_KARAMBIT, WEAPON_KNIFE_M9_BAYONET, WEAPON_KNIFE_PUSH, WEAPON_KNIFE_TACTICAL , WEAPON_KNIFE, WEAPON_KNIFE_T };
	return (std::find(v.begin(), v.end(), id) != v.end());
}

void knifeleftside(C_BaseEntity* local)
{
	C_BaseCombatWeapon* pWeapon = local->GetActiveWeapon();
	if (!pWeapon)
		return;

	static ConVar* cl_righthand = g_pConVar->FindVar("cl_righthand");

	if(IsKnife(pWeapon))
		cl_righthand->SetValue("0");
	else
		cl_righthand->SetValue("1");
}

bool IsBallisticWeapon6(C_BaseEntity* local)
{
	C_BaseCombatWeapon* pWeapon = local->GetActiveWeapon();
	if (!pWeapon)
		return false;
	ClientClass* pWeaponClass = pWeapon->GetClientClass();

	if (pWeaponClass->m_ClassID == (int)EClassIds::CKnife || pWeaponClass->m_ClassID == (int)EClassIds::CHEGrenade ||
		pWeaponClass->m_ClassID == (int)EClassIds::CDecoyGrenade || pWeaponClass->m_ClassID == (int)EClassIds::CIncendiaryGrenade ||
		pWeaponClass->m_ClassID == (int)EClassIds::CSmokeGrenade || pWeaponClass->m_ClassID == (int)EClassIds::CC4 ||
		pWeaponClass->m_ClassID == (int)EClassIds::CMolotovGrenade || pWeaponClass->m_ClassID == (int)EClassIds::CFlashbang)
		return false;
	else
		return true;
}

static auto linegoesthrusmoke = Utils::FindSignature("client_panorama.dll", "55 8B EC 83 EC 08 8B 15 ?? ?? ?? ?? 0F 57 C0");
static auto smokecout = *(DWORD*)(linegoesthrusmoke + 0x8);

auto clantag = [](const char * name) -> void {
	using Fn = int(__fastcall *)(const char *, const char *);
	static auto fn = reinterpret_cast<Fn>(Utils::FindSignature("engine.dll", "53 56 57 8B DA 8B F9 FF 15"));
	fn(name, name);
};

static int ground_tick;

//--- Tick Counting ---//
void ground_ticks()
{
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetFlags() & FL_ONGROUND)
		ground_tick++;
	else
		ground_tick = 0;
}

void UpdateAngleAA(CUserCmd* pCmd)
{
	Globals::AngleAA = pCmd->viewangles;
}

void Marquee(std::string& clantag)
{
	std::string temp = clantag;
	clantag.erase(0, 1);
	clantag += temp[0];
}

void ClantagCM()
{
	static std::string text = " BoredGang       ";

	static float LastChangeTime = 0.f;
	if (g_pGlobalVars->realtime - LastChangeTime < .45f)
	{
		return;
	}

	LastChangeTime = g_pGlobalVars->realtime;
	Marquee(text);

	clantag(text.data());
}

void SlideWalk(CUserCmd* pCmd)
{
	if (!g_Settings.bSlideWalk)
		return;

	if (pCmd->forwardmove > 0)
	{
		pCmd->buttons |= IN_BACK;
		pCmd->buttons &= ~IN_FORWARD;
	}

	if (pCmd->forwardmove < 0)
	{
		pCmd->buttons |= IN_FORWARD;
		pCmd->buttons &= ~IN_BACK;
	}

	if (pCmd->sidemove < 0)
	{
		pCmd->buttons |= IN_MOVERIGHT;
		pCmd->buttons &= ~IN_MOVELEFT;
	}

	if (pCmd->sidemove > 0)
	{
		pCmd->buttons |= IN_MOVELEFT;
		pCmd->buttons &= ~IN_MOVERIGHT;
	}
}

void FixWalk(CUserCmd* pCmd)
{
	if (!g_Settings.bFixLegMovement)
		return;

	if (pCmd->buttons & IN_FORWARD)
	{
		pCmd->forwardmove = 450;
		pCmd->buttons &= ~IN_FORWARD;
	}
	else if (pCmd->buttons & IN_BACK)
	{
		pCmd->forwardmove = -450;
		pCmd->buttons &= ~IN_BACK;
	}
	if (pCmd->buttons & IN_MOVELEFT)
	{
		pCmd->sidemove = -450;
		pCmd->buttons &= ~IN_MOVELEFT;
	}
	else if (pCmd->buttons & IN_MOVERIGHT)
	{
		pCmd->sidemove = 450;
		pCmd->buttons &= ~IN_MOVERIGHT;
	}
}

void AutomaticWeapons(CUserCmd* pCmd)
{
	C_BaseCombatWeapon* pWeapon = g::pLocalEntity->GetActiveWeapon();
	if (!pWeapon)
		return;

	if (pWeapon->GetAmmo() == 0)
		return;

	static bool WasFiring = false;
	WeaponInfo_t* WeaponInfo = pWeapon->GetCSWpnData();
	if (!WeaponInfo->bFullAuto)
	{
		if (pCmd->buttons & IN_ATTACK)
		{
			if (WasFiring)
			{
				pCmd->buttons &= ~IN_ATTACK;
			}
		}
		WasFiring = pCmd->buttons & IN_ATTACK ? true : false;
	}
}

void AutoRevolver(CUserCmd* cmd, C_BaseEntity* local_player)
{
	if (!g_Settings.bAutoRevolver)
		return;

	C_BaseCombatWeapon* weapon = local_player->GetActiveWeapon();
	if (!weapon || weapon->GetItemDefinitionIndex() != ItemDefinitionIndex::WEAPON_REVOLVER)
		return;

	static float delay = 0.f;
	if (delay < 0.15f)
	{
		delay += g_pGlobalVars->intervalPerTick;
		cmd->buttons |= IN_ATTACK;
	}
	else
		delay = 0.f;
}

int GetTickbase(CUserCmd* ucmd) {

	static int g_tick = 0;
	static CUserCmd* g_pLastCmd = nullptr;

	if (!ucmd)
		return g_tick;

	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = g::pLocalEntity->GetTickBase();
	}
	else {
		// Required because prediction only runs on frames, not ticks
		// So if your framerate goes below tickrate, m_nTickBase won't update every tick
		++g_tick;
	}

	g_pLastCmd = ucmd;
	return g_tick;
}

bool __fastcall Hooks::CreateMove(IClientMode* thisptr, void* edx, float sample_frametime, CUserCmd* pCmd)
{
	// Call original createmove before we start screwing with it
	static auto oCreateMove = g_Hooks.pClientModeHook->GetOriginal<CreateMove_t>(24);
	oCreateMove(thisptr, edx, sample_frametime, pCmd);

	if (!pCmd || !pCmd->command_number)
		return oCreateMove;

	// Get globals
	g::pCmd = pCmd;
	g::pLocalEntity = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	if (!g::pLocalEntity)
		return oCreateMove;

	GetTickbase(pCmd);

	uintptr_t *framePtr;
	__asm mov framePtr, ebp;

	Globals::bsendpacket = true;

	if (g_Settings.bSkinChangerWindow)
		PrecacheModels();

	if(g_Settings.bClantag)
		ClantagCM();

	if (g::pLocalEntity->IsAlive())
	{
		pCmd->buttons |= IN_BULLRUSH;

		g_Misc.OnCreateMove();		

		if (g_Settings.bAutomaticRevolver)
			AutoRevolver(pCmd, g::pLocalEntity);

		if (g_Settings.bKnifeLeft)
			knifeleftside(g::pLocalEntity);

		if (IsBallisticWeapon6(g::pLocalEntity) && g::pLocalEntity->GetFlags() & FL_ONGROUND && g_Settings.bAutoStop && Globals::shouldstop)
			aimbot->stopmovement(g::pLocalEntity, pCmd);

		if (g::pLocalEntity->GetFlags() & FL_ONGROUND && pCmd->buttons & IN_SPEED)
			aimbot->minspeed(g::pLocalEntity, pCmd);

		FixMoveManager->Start(pCmd);
		
		FixWalk(pCmd);

		KnifeBot::Run(g::pLocalEntity, pCmd);

		backtracking->legitBackTrack(pCmd, g::pLocalEntity);

		engine_prediction::RunEnginePred();

		fakelag->fakeduck2(pCmd, g::pLocalEntity);

		fakelag->do_fakelag(pCmd, g::pLocalEntity);

		if (g_Settings.bAutomaticWeapons && IsBallisticWeapon6(g::pLocalEntity))
			AutomaticWeapons(pCmd);
		
		if (g_Settings.bLegitBotMaster)
			laimbot->Move(pCmd);
	
		if (g_Settings.bAimBotEnabled)
			aimbot->Move(pCmd);

		antiaim->Do(pCmd);

		engine_prediction::EndEnginePred();

		FixMoveManager->Stop(pCmd);

		SlideWalk(pCmd);
	}

	if(g_Settings.AntiUT)
		pCmd->viewangles = Utils::NormalizeAngle(pCmd->viewangles);

	*(bool*)(*framePtr - 0x1C) = Globals::bsendpacket;

	UpdateAngleAA(pCmd);

    return false;
}


void __fastcall Hooks::LockCursor(ISurface* thisptr, void* edx)
{
    static auto oLockCursor = g_Hooks.pSurfaceHook->GetOriginal<LockCursor_t>(vtable_indexes::lockCursor);

    if (!g_Settings.bMenuOpened)
        return oLockCursor(thisptr, edx);

    g_pSurface->UnlockCursor();
}


HRESULT __stdcall Hooks::Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    static auto oReset = g_Hooks.pD3DDevice9Hook->GetOriginal<Reset_t>(16);

    if (g_Hooks.bInitializedDrawManager)
    {
        Utils::Log("Reseting draw manager.");
        g_Render.InvalidateDeviceObjects();
        HRESULT hr = oReset(pDevice, pPresentationParameters);
        g_Render.RestoreDeviceObjects(pDevice);
        Utils::Log("DrawManager reset succeded.");
        return hr;
    }	

    return oReset(pDevice, pPresentationParameters);
}


HRESULT __stdcall Hooks::Present(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect, 
                                 HWND hDestWindowOverride,  const RGNDATA* pDirtyRegion)
{
	static auto oPresent = g_Hooks.pD3DDevice9Hook->GetOriginal<Present_t>(17);

	IDirect3DStateBlock9* stateBlock = nullptr;
	IDirect3DVertexDeclaration9* vertDec = nullptr;

	pDevice->GetVertexDeclaration(&vertDec);
    pDevice->CreateStateBlock(D3DSBT_PIXELSTATE, &stateBlock);

    [pDevice]()
    {
        if (!g_Hooks.bInitializedDrawManager)
        {
            Utils::Log("Initializing Draw manager");
            g_Render.InitDeviceObjects(pDevice);
            g_Hooks.nMenu.Initialize();
            g_Hooks.bInitializedDrawManager = true;
            Utils::Log("Draw manager initialized");
        }
        else
        {
            g_Render.SetupRenderStates(); // Sets up proper render states for our state block

            //static std::string szWatermark = "lowercase";
            //g_Render.String(8, 5, CD3DFONT_DROPSHADOW, Color(250, 255, 255, 255), g_Fonts.pFontTahoma8.get(), szWatermark.c_str());
            // Put your draw calls here
            g_ESP.Render();

			EngineCrosshair();

			if(g_Settings.bDamageLogs)
				LogEvents();
			if (g_Settings.bScopeNoZoom)
				NoScopeOverlay();

			//clantag("otario.cc");
			DrawUselessInfo();

			DoNightMode();

			DoSkyBox();

			DoAsusWalls();

			if(g_Settings.bHitmaker)
				g_ESP.DrawHitmarker(); 	

			//otheresp::get().hitmarkerdynamic_paint();
			//hitmarkerdynamic_paint();
			if (g_Settings.bMenuOpened)
			{
				g_Hooks.nMenu.Render();             // Render our menu
				g_Hooks.nMenu.mouseCursor->Render();// Render mouse cursor in the end so its not overlapped
			}
        }
    }();

    stateBlock->Apply();
    stateBlock->Release();
	pDevice->SetVertexDeclaration(vertDec);
    
    return oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

LRESULT Hooks::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // for now as a lambda, to be transfered somewhere
    // Thanks uc/WasserEsser for pointing out my mistake!
    // Working when you HOLD th button, not when you press it.
    const auto getButtonHeld = [uMsg, wParam](bool& bButton, int vKey)
    {
		if (wParam != vKey) return;

        if (uMsg == WM_KEYDOWN)
            bButton = true;
        else if (uMsg == WM_KEYUP)
            bButton = false;
    };

	const auto getButtonToggle = [uMsg, wParam](bool& bButton, int vKey)
	{
		if (wParam != vKey) return;

		if (uMsg == WM_KEYUP)
			bButton = !bButton;
	};

	getButtonToggle(g_Settings.bMenuOpened, VK_INSERT);

    if (g_Hooks.bInitializedDrawManager)
    {
        // our wndproc capture fn
        if (g_Settings.bMenuOpened)
        {
            g_Hooks.nMenu.MsgProc(uMsg, wParam, lParam);
            return true;
        }
    }

    // Call original wndproc to make game use input again
    return CallWindowProcA(g_Hooks.pOriginalWNDProc, hWnd, uMsg, wParam, lParam);
}

void __fastcall Hooks::SceneEnd(void * ecx, void * edx)
{
	static auto oSceneEnd = g_Hooks.pRenderViewHook->GetOriginal<SceneEnd_t>(9);
	oSceneEnd(ecx);
	
	g::pLocalEntity = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
	if (!g::pLocalEntity)
		return;

	float flColor[3] = { 0.f };
	float flColor2[3] = { 0.f };
	float flColor3[3] = { 0.f };

	flColor[0] = 60.f / 255.f;
	flColor[1] = 120.f / 255.f;
	flColor[2] = 180.f / 255.f;

	flColor2[0] = 150.f / 255.f;
	flColor2[1] = 200.f / 255.f;
	flColor2[2] = 60.f / 255.f;

	flColor3[0] = 255.f / 255.f;
	flColor3[1] = 255.f / 255.f;
	flColor3[2] = 255.f / 255.f;

	Color2 LocalColor = Color2(60, 120, 180, 255);
	Color2 EnemyColor = Color2(60, 120, 180, 255);

	Color2 LocalColorXqz = Color2(150, 200, 60, 255);
	Color2 EnemyColorXqz = Color2(150, 200, 60, 255);

	if (!g::pLocalEntity || !g_pEngine->IsInGame())
		return;

	if (!g_pEngine->IsInGame() && !g_pEngine->IsConnected())
		return;

	for (int it = 1; it <= g_pEngine->GetMaxClients(); ++it)
	{
		C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(it);

		if (!pPlayerEntity
			|| !pPlayerEntity->IsAlive()
			|| pPlayerEntity->IsDormant())
			continue;

		bool IsLocal = pPlayerEntity == g::pLocalEntity;
		bool IsTeam = pPlayerEntity->GetTeam() == g::pLocalEntity->GetTeam();

		bool normal = false;
		bool flat = false;
		bool wireframe = false;
		bool glass = false;
		bool metallic = false;
		bool xqz = false;
		bool flat_xqz = false;
		bool wireframe_xqz = false;
		bool onetapsu = false;

		int mode = IsLocal ? g_Settings.iLocalChamsMode : g_Settings.iChamsMode;

		if (IsLocal && !g_Settings.bShowLocalChams)
		{
			continue;
		}
		if ((IsTeam && !IsLocal) && !g_Settings.bShowTeammates)
		{
			continue;
		}
		if (!IsTeam && !g_Settings.bShowChams)
		{
			continue;
		}

		Color2 clr = IsLocal ? LocalColor : EnemyColor;
		Color2 clr2 = IsLocal ? LocalColorXqz : EnemyColorXqz;

	
			switch (mode)
			{
			case 1:
				xqz = true;
				break;
			case 2:
				flat_xqz = true;
				break;
			case 3:
				wireframe_xqz = true;
				break;
			}
		
			switch (mode)
			{
			case 1:
				normal = true;
				break;
			case 2:
				flat = true;
				break;
			case 3:
				wireframe = true;
				break;
			}
		
		MaterialManager::get().OverrideMaterial(xqz || flat_xqz || wireframe_xqz, flat, wireframe, glass, metallic, onetapsu);
			g_pRenderView->SetColorModulation(clr.r() / 255.f, clr.g() / 255.f, clr.b() / 255.f);
			pPlayerEntity->GetClientRenderable()->DrawModel(0x1, 255);
			if (xqz || flat_xqz || wireframe_xqz)
			{
				MaterialManager::get().OverrideMaterial(false, flat, wireframe, glass, metallic, onetapsu);
				g_pRenderView->SetColorModulation(clr2.r() / 255.f, clr2.g() / 255.f, clr2.b() / 255.f);
				pPlayerEntity->GetClientRenderable()->DrawModel(0x1, 255);
			}
			g_pModelRender->ForcedMaterialOverride(nullptr);
		}
	g_pModelRender->ForcedMaterialOverride(nullptr);

		/*
		if ((g::pLocalEntity != pPlayerEntity) && g::pLocalEntity->GetTeam() != pPlayerEntity->GetTeam())
		{
			if (g_Settings.iChamsMode == 1)
			{
				g_pRenderView->SetColorModulation(flColor2);
				pPlayerEntity->DrawModel(0x1, 255);
			}
			else if (g_Settings.iChamsMode == 2)
			{
				g_pRenderView->SetColorModulation(flColor);
				pPlayerEntity->DrawModel(0x1, 255);

				g_pRenderView->SetColorModulation(flColor2);
				pPlayerEntity->DrawModel(0x1, 255);
			}
		}
		else if ((g::pLocalEntity != pPlayerEntity) && g_Settings.bShowTeammates && g::pLocalEntity->GetTeam() == pPlayerEntity->GetTeam())
		{
			if (g_Settings.iChamsMode == 1)
			{
				g_pRenderView->SetColorModulation(flColor2);
				pPlayerEntity->DrawModel(0x1, 255);
			}
			else if (g_Settings.iChamsMode == 2)
			{
				g_pRenderView->SetColorModulation(flColor);
				pPlayerEntity->DrawModel(0x1, 255);

				g_pRenderView->SetColorModulation(flColor2);
				pPlayerEntity->DrawModel(0x1, 255);
			}
		}
		else if (g::pLocalEntity == pPlayerEntity && g_pInput->m_fCameraInThirdPerson && !g::pLocalEntity->IsScoped() && !g_Settings.iScopedBlend < 100)
		{
			switch (g_Settings.iLocalChamsMode)
			{
			case 1:
				g_pRenderView->SetColorModulation(flColor3);
				pPlayerEntity->DrawModel(0x1, 255);
				break;
			case 2:
				g_pRenderView->SetColorModulation(flColor3);
				pPlayerEntity->DrawModel(0x1, 255);
				break;
			case 3:
				g_pRenderView->SetColorModulation(flColor3);
				pPlayerEntity->DrawModel(0x1, 255);
				break;
			}
		}
		g_pModelRender->ForcedMaterialOverride(nullptr);
	}
	g_pModelRender->ForcedMaterialOverride(nullptr);*/
}

int __fastcall Hooks::DoPostScreenEffects(void * ecx, void * edx, int a1)
{
	static auto oDoPostScreenEffects = g_Hooks.pClientModeHook->GetOriginal<DoPostScreenEffects_t>(44);

	if (g_Settings.bShowGlow)
		DoGlow->DrawGlow();

	return oDoPostScreenEffects(ecx, edx, a1);
}

void animation_fix33(C_BaseEntity* entity)
{
			if (!entity->GetAnimState())
					return;
	/*
				auto OldCurtime = g_pGlobalVars->curtime;
				float OldFraction = entity->GetAnimState()->m_flUnknownFraction;
				auto OldFrametime = g_pGlobalVars->frametime;
				g_pGlobalVars->curtime = entity->m_flSimulationTime();

			//	if(!entity->GetFlags() & FL_DUCKING && entity->GetVelocity().Length2D() <= 0.5f && entity->GetVelocity().Length2D() > 10.f)
			//	OldFraction = entity->GetAnimState()->m_flUnknownFraction = 0;
			//	entity->GetAnimState()->m_bOnGround = false;
				g_pGlobalVars->frametime =
					g_pGlobalVars->intervalPerTick *
					g_pConVar->FindVar("host_timescale")->GetFloat();

				CAnimationLayer Layers[15];
				std::memcpy(Layers, entity->AnimOverlays(), (sizeof(CAnimationLayer) * entity->GetNumAnimOverlays()));

					entity->GetAnimState()->m_iLastClientSideAnimationUpdateFramecount = g_pGlobalVars->framecount - 1;

				entity->ClientSideAnimation() = true;
				entity->UpdateClientSideAnimation();
				entity->ClientSideAnimation() = false;
				//end
				std::memcpy(entity->AnimOverlays(), Layers, (sizeof(CAnimationLayer) * entity->GetNumAnimOverlays()));

				g_pGlobalVars->curtime = OldCurtime;
				g_pGlobalVars->frametime = OldFrametime;
				//if (!entity->GetFlags() & FL_DUCKING)
				//if (!entity->GetFlags() & FL_DUCKING && entity->GetVelocity().Length2D() <= 0.5f && entity->GetVelocity().Length2D() > 1.f)
				//entity->GetAnimState()->m_flUnknownFraction = OldFraction;

				entity->SetAbsAngles(QAngle(0, entity->GetAnimState()->m_flGoalFeetYaw, 0));*/


				//start
		//	auto OldCurtime = g_pGlobalVars->curtime;
			auto OldFrametime = g_pGlobalVars->frametime;
			//float OldFraction;
			//if (!entity->GetFlags() & FL_DUCKING && entity->GetVelocity().Length2D() <= 0.5f && entity->GetVelocity().Length2D() > 1.f)
			//	OldFraction = entity->GetAnimState()->m_flUnknownFraction = 0;

			//g_pGlobalVars->curtime = entity->m_flSimulationTime();
			g_pGlobalVars->frametime =
				g_pGlobalVars->intervalPerTick *
				g_pConVar->FindVar("host_timescale")->GetFloat();

			CAnimationLayer Layers[15];
			std::memcpy(Layers, entity->AnimOverlays(), (sizeof(CAnimationLayer) * entity->GetNumAnimOverlays()));

			if (entity->GetAnimState())
				entity->GetAnimState()->m_iLastClientSideAnimationUpdateFramecount = g_pGlobalVars->framecount - 1;

			entity->ClientSideAnimation() = true;
			entity->UpdateClientSideAnimation();
			entity->ClientSideAnimation() = false;

			//end
			std::memcpy(entity->AnimOverlays(), Layers, (sizeof(CAnimationLayer) * entity->GetNumAnimOverlays()));

		//	g_pGlobalVars->curtime = OldCurtime;
			g_pGlobalVars->frametime = OldFrametime;

			
		//	entity->GetAnimState()->m_flUnknownFraction = OldFraction;
}

void animation_fix(C_BaseEntity * e) 
{
	CBaseAnimState * animation_state = e->GetAnimState();

	if (!animation_state)
		return;

 //float frameTime;
	float curtime;
	float simtime;
	////float oldFraction;
	simtime = e->m_flSimulationTime();

	//e->ClientSideAnimation() = true;

	////oldFraction = animation_state->m_flUnknownFraction = 0.f;
	curtime = g_pGlobalVars->curtime;
	g_pGlobalVars->curtime = simtime;
//frameTime = g_pGlobalVars->frametime;
	g_pGlobalVars->frametime = g_pGlobalVars->intervalPerTick;
	//
		e->UpdateClientSideAnimation();
	//		
	//	//if (e->GetVelocity().Length2D() < 100)
	//	//	animation_state->m_flUnknownFraction = oldFraction;

	//	g_pGlobalVars->curtime = curtime;
	//	g_pGlobalVars->frametime = frameTime;

	//	//e->SetAbsAngles(QAngle(0, animation_state->m_flGoalFeetYaw, 0));

	////e->ClientSideAnimation() = false;	

	auto curtime_restore = g_pGlobalVars->curtime;
	auto frametime_restore = g_pGlobalVars->frametime;

	g_pGlobalVars->frametime = g_pGlobalVars->intervalPerTick;
	
	//g_pGlobalVars->curtime = e->m_flSimulationTime(); // entity 

	animation_state->m_iLastClientSideAnimationUpdateFramecount = g_pGlobalVars->framecount - 1; // last framecount from the animation update

	e->UpdateClientSideAnimation(); // Update client_side_animation index 219
	
	//g_pGlobalVars->curtime = curtime_restore; // restore old
	g_pGlobalVars->frametime = frametime_restore; // restore old 

	e->SetAbsAngles(QAngle(0.f, animation_state->m_flGoalFeetYaw, 0.f)); // Returns 0 for me

	e->ClientSideAnimation() = false;
}

void apply_interpolation_flags(C_BaseEntity * e, int flag) {
	const auto var_map = reinterpret_cast<uintptr_t>(e) + 36;

	for (auto index = 0; index < *reinterpret_cast<int*>(var_map + 20); index++)
		*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(var_map) + index * 12) = flag;
}

void animation_fix2(C_BaseEntity * pEnt) 
{
	//who needs structs or classes not me lol
	static float oldSimtime[65];
	static float storedSimtime[65];
	static float ShotTime[65];
	static float SideTime[65][3];
	static int LastDesyncSide[65];
	static bool Delaying[65];
	static CAnimationLayer StoredLayers[64][15];
	static CBaseAnimState * StoredAnimState[65];
	static float StoredPosParams[65][24];
	static Vector oldEyeAngles[65];
	static float oldGoalfeetYaw[65];
	static float OldFraction;
	static float OldDuckAmount[65];
	bool update = false;
	bool shot = false;

	static bool jittering[65];

	auto* AnimState = pEnt->GetAnimState();
	if (!AnimState)
		return;

	if (storedSimtime[pEnt->EntIndex()] != pEnt->m_flSimulationTime())
	{
	//	jittering[pEnt->EntIndex()] = false;
		pEnt->UpdateClientSideAnimation();

	//	memcpy(StoredPosParams[pEnt->EntIndex()], PosParams, sizeof(float) * 24);
	//	memcpy(StoredLayers[pEnt->EntIndex()], pEnt->AnimOverlays(), (sizeof(CAnimationLayer) * 15));

		oldGoalfeetYaw[pEnt->EntIndex()] = AnimState->m_flGoalFeetYaw;
	//	OldFraction = AnimState->m_flUnknownFraction = 0;
	//	if(Globals::isfakeducking)
	//	OldDuckAmount[pEnt->EntIndex()] = AnimState->m_fDuckAmount = 0;

		if (pEnt->GetActiveWeapon())
		{
			if (ShotTime[pEnt->EntIndex()] != pEnt->GetActiveWeapon()->GetLastShotTime())
			{
				shot = true;
				ShotTime[pEnt->EntIndex()] = pEnt->GetActiveWeapon()->GetLastShotTime();
			}
			else
				shot = false;
		}
		else
		{
			shot = false;
			ShotTime[pEnt->EntIndex()] = 0.f;
		}

		/*	float angToLocal = Utils::NormalizeYaw(Utils::CalcAngle(g::pLocalEntity->GetOrigin(), pEnt->GetOrigin()).y);

		float Back = Utils::NormalizeYaw(angToLocal);
		float DesyncFix = 0;
		float Resim = Utils::NormalizeYaw((0.24f / (pEnt->m_flSimulationTime() - oldSimtime[pEnt->EntIndex()]))*(oldEyeAngles[pEnt->EntIndex()].y - pEnt->GetEyeAngles().y));

		if (Resim > 58.f)
			Resim = 58.f;
		if (Resim < -58.f)
			Resim = -58.f;

		if (pEnt->GetVelocity().Length2D() > 0.5f && !shot)
		{
			float Delta = Utils::NormalizeYaw(Utils::NormalizeYaw(Utils::CalcAngle(Vector(0, 0, 0), pEnt->GetVelocity()).y) - Utils::NormalizeYaw(Utils::NormalizeYaw(AnimState->m_flGoalFeetYaw + RemapVal(PosParams[11], 0, 1, -60, 60)) + Resim));

			int CurrentSide = 0;

			if (Delta < 0)
			{
				CurrentSide = 1;
				SideTime[pEnt->EntIndex()][1] = g_pGlobalVars->curtime;
			}
			else if (Delta > 0)
			{
				CurrentSide = 2;
				SideTime[pEnt->EntIndex()][2] = g_pGlobalVars->curtime;
			}

			if (LastDesyncSide[pEnt->EntIndex()] == 1)
			{
				Resim += (58.f - Resim);
				DesyncFix += (58.f - Resim);
			}
			if (LastDesyncSide[pEnt->EntIndex()] == 2)
			{
				Resim += (-58.f - Resim);
				DesyncFix += (-58.f - Resim);
			}

			if (LastDesyncSide[pEnt->EntIndex()] != CurrentSide)
			{
				Delaying[pEnt->EntIndex()] = true;

				if (.5f < (g_pGlobalVars->curtime - SideTime[pEnt->EntIndex()][LastDesyncSide[pEnt->EntIndex()]]))
				{
					LastDesyncSide[pEnt->EntIndex()] = CurrentSide;
					Delaying[pEnt->EntIndex()] = false;
				}
			}

			if (!Delaying[pEnt->EntIndex()])
				LastDesyncSide[pEnt->EntIndex()] = CurrentSide;
		}
		else if (!shot)
		{
			float Delta = Utils::NormalizeYaw(Utils::NormalizeYaw(Utils::NormalizeYaw(Utils::NormalizeYaw(AnimState->m_flGoalFeetYaw + RemapVal(PosParams[11], 0, 1, -60, 60))) + Resim));

			if (Delta > 58.f)
				Delta = 58.f;
			if (Delta < -58.f)
				Delta = -58.f;

			Resim += Delta;
			DesyncFix += Delta;

			if (Resim > 58.f)
				Resim = 58.f;
			if (Resim < -58.f)
				Resim = -58.f;
		}
		
		float Equalized = Utils::NormalizeYaw(Utils::NormalizeYaw(AnimState->m_flGoalFeetYaw + RemapVal(PosParams[11], 0, 1, -60, 60)) + Resim);

		float JitterDelta = fabs(Utils::NormalizeYaw(oldEyeAngles[pEnt->EntIndex()].y - pEnt->GetEyeAngles().y));

		if (JitterDelta >= 70.f && !shot)
			jittering[pEnt->EntIndex()] = true;
		
		if (pEnt != g::pLocalEntity && pEnt->GetTeam() != g::pLocalEntity->GetTeam() && (pEnt->GetFlags() & FL_ONGROUND))
		{
			if (jittering[pEnt->EntIndex()])
				AnimState->m_flGoalFeetYaw = Utils::NormalizeYaw(pEnt->GetEyeAngles().y + DesyncFix);
			else
				AnimState->m_flGoalFeetYaw = Equalized;

			pEnt->SetLowerBodyYaw(AnimState->m_flGoalFeetYaw);
		}
		*/
		//StoredAnimState[pEnt->EntIndex()] = AnimState;

	//	oldEyeAngles[pEnt->EntIndex()] = pEnt->GetEyeAngles();

		oldSimtime[pEnt->EntIndex()] = storedSimtime[pEnt->EntIndex()];

		storedSimtime[pEnt->EntIndex()] = pEnt->m_flSimulationTime();

		//AnimState->m_flUnknownFraction = OldFraction;
		//AnimState->m_fDuckAmount = OldDuckAmount[pEnt->EntIndex()];

	}
	//if (pEnt != g::pLocalEntity && pEnt->GetTeam() != g::pLocalEntity->GetTeam() && (pEnt->GetFlags() & FL_ONGROUND))
	//	pEnt->SetLowerBodyYaw(AnimState->m_flGoalFeetYaw);

	//AnimState = StoredAnimState[pEnt->EntIndex()];

	//memcpy((void*)PosParams, &StoredPosParams[pEnt->EntIndex()], (sizeof(float) * 24));
	//memcpy(pEnt->AnimOverlays(), StoredLayers[pEnt->EntIndex()], (sizeof(CAnimationLayer) * 15));

	if (pEnt != g::pLocalEntity && pEnt->GetTeam() != g::pLocalEntity->GetTeam() && (pEnt->GetFlags() & FL_ONGROUND) && jittering[pEnt->EntIndex()])
		pEnt->SetAbsAngles(QAngle(0, pEnt->GetEyeAngles().y, 0));
	else
		pEnt->SetAbsAngles(QAngle(0, oldGoalfeetYaw[pEnt->EntIndex()], 0));
}

void ApplyPrecachedModels()
{
	if (!g::pLocalEntity->IsAlive())
		return;

	C_BaseCombatWeapon* pWeapon = g::pLocalEntity->GetActiveWeapon();

	if (!pWeapon)
		return;

	const auto view_model_handle = g::pLocalEntity->GetViewModelIndex();
	if (!view_model_handle)
		return;

	const auto view_model = reinterpret_cast<C_BaseViewmodel*>(g_pEntityList->GetClientEntityFromHandle(view_model_handle));

	auto viewmodel_weapon = reinterpret_cast<C_BaseCombatWeapon*>(g_pEntityList->GetClientEntity(view_model->GetWeaponIndex()));
	if (!viewmodel_weapon)
		return;

	auto world_model_handle = viewmodel_weapon->m_hWeaponWorldModel();
	if (!world_model_handle.IsValid())
		return;

	const auto world_model = (C_BaseCombatWeapon*)(g_pEntityList->GetClientEntityFromHandle(world_model_handle));
	if (!world_model)
		return;

	const int index = g_pModelInfo->GetModelIndex("models/weapons/v_ak47beast.mdl");

	if (viewmodel_weapon == pWeapon && view_model->m_nModelIndex() != index)
	{
		if (pWeapon->sGetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_AK47)
		{
			view_model->SetModelIndex(index);
		}
	}

	const int index2 = g_pModelInfo->GetModelIndex("models/weapons/v_knife_karam.mdl");

	if (viewmodel_weapon == pWeapon && view_model->m_nModelIndex() != index2)
	{
		if (pWeapon->is_knife())
		{
			view_model->SetModelIndex(index2);
			world_model->SetModelIndex(index2 + 1);
		}
	}

	const int index3 = g_pModelInfo->GetModelIndex("models/weapons/v_cod9_ballista.mdl");

	if (viewmodel_weapon == pWeapon && view_model->m_nModelIndex() != index3)
	{
		if (pWeapon->sGetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_AWP)
		{
			view_model->SetModelIndex(index3);
		}
	}
}

float get_max_desync_delta_test(C_BaseEntity* entity) {

	auto animstate = entity->GetAnimState();
	if (!animstate)
		return 0.f;

	float duckammount = *(float *)(animstate + 0xA4);
	float speedfraction = max(0, min(*reinterpret_cast<float*>(animstate + 0xF8), 1));

	float speedfactor = max(0, min(1, *reinterpret_cast<float*> (animstate + 0xFC)));

	float unk1 = ((*reinterpret_cast<float*> (animstate + 0x11C) * -0.30000001) - 0.19999999) * speedfraction;
	float unk2 = unk1 + 1.f;
	float unk3;

	if (duckammount > 0) {

		unk2 += ((duckammount * speedfactor) * (0.5f - unk2));

	}

	unk3 = *(float *)(animstate + 0x334) * unk2;

	return unk3;

}

bool using_fake_angles[65];
int missed_shots[65];

void ResolveDesyncBruteforce(C_BaseEntity* pPlayerEntity)
{
	static float old_simtime[65];

	missed_shots[pPlayerEntity->EntIndex()] = Globals::shots_fired[pPlayerEntity->EntIndex()] - Globals::shots_hit[pPlayerEntity->EntIndex()];
	const float at_target_yaw = Utils::CalcAngle(pPlayerEntity->GetVecOrigin(), g::pLocalEntity->GetVecOrigin()).y;

	if (pPlayerEntity->m_flSimulationTime() != old_simtime[pPlayerEntity->EntIndex()])
	{
		using_fake_angles[pPlayerEntity->EntIndex()] = pPlayerEntity->m_flSimulationTime() - old_simtime[pPlayerEntity->EntIndex()] == g_pGlobalVars->intervalPerTick; //entity->GetSimTime() - old_simtime[entity->GetIndex()] >= TICKS_TO_TIME(2)
		old_simtime[pPlayerEntity->EntIndex()] = pPlayerEntity->m_flSimulationTime();
	}

	if (!using_fake_angles[pPlayerEntity->EntIndex()])
	{
		switch (missed_shots[pPlayerEntity->EntIndex()] % 3)
		{
		case 0:
			break;
		case 1:
			pPlayerEntity->GetEyeAngles2()->y = at_target_yaw + 89.f;
			break;
		case 2:
			pPlayerEntity->GetEyeAngles2()->y = at_target_yaw - 58.f;
			break;
		}
	}
}

void resolverdo()
{
	for (int it = 1; it <= g_pEngine->GetMaxClients(); ++it)
	{
		C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(it);

		if (!pPlayerEntity
			|| !pPlayerEntity->IsAlive()
			|| pPlayerEntity == g::pLocalEntity 
			|| pPlayerEntity->GetTeam() == g::pLocalEntity->GetTeam()
			|| pPlayerEntity->IsDormant())
			continue;

		ResolveDesyncBruteforce(pPlayerEntity);
	}
}

template<class T>
static T* FindHudElement(const char* name)
{
	static auto pThis = *reinterpret_cast<DWORD**> (Utils::FindSignature("client_panorama.dll", "B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08") + 1); //(Utilities::Memory::FindPatternV2("client_panorama.dll", "B9 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 46 24") + 1);

	static auto find_hud_element = reinterpret_cast<DWORD(__thiscall*)(void*, const char*)>(Utils::FindSignature("client_panorama.dll", "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28"));
	return (T*)find_hud_element(pThis, name);
}

bool ReallocatedDeathNoticeHUD = false;

void PreserveKillFeed()
{
	static void(__thiscall *ClearDeathNotices)(DWORD);
	static DWORD* deathNotice;

	if (g::pLocalEntity && g_pEngine->IsInGame() && g_pEngine->IsConnected())
	{
		if (g::pLocalEntity) {
			if (!deathNotice) deathNotice = FindHudElement<DWORD>("CCSGO_HudDeathNotice");
			if (deathNotice) {
				float* localDeathNotice = (float*)((DWORD)deathNotice + 0x50);
				if (localDeathNotice) *localDeathNotice = g_Settings.bPreserveKillFeed ? FLT_MAX : 1.5f;
				if (Globals::preservedelete && deathNotice - 20) {
					if (!ClearDeathNotices) ClearDeathNotices = (void(__thiscall*)(DWORD))Utils::FindSignature("client_panorama.dll", "55 8B EC 83 EC 0C 53 56 8B 71 58");
					if (ClearDeathNotices) { ClearDeathNotices(((DWORD)deathNotice - 20)); Globals::preservedelete = false; }

				}
			}
		}
	}
}




void __fastcall Hooks::FrameStageNotify(void* ecx, void* edx, int stage)
{
	static auto oFrameStageNotify = g_Hooks.pClientDllHook->GetOriginal<FrameStageNotify_t>(37);

	g::pLocalEntity = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
	
	if (g_pEngine->IsInGame() && g_pEngine->IsConnected())
	{
		switch (stage)
		{
			case FRAME_NET_UPDATE_POSTDATAUPDATE_START:
			
			//	KnifeApplyCallbk();
				//resolverdo();
				if (g::pLocalEntity->IsAlive())
				{
					skin_changer->KnifeChanger();
					skin_changer->ChangeSkins();

					if (g_Settings.bSkinChangerWindow)
						ApplyPrecachedModels();

					if (g_Settings.bNoFlash)
						g::pLocalEntity->SetFlashDuration(0);
				}
				
			//	if(g_Settings.bPreserveKillFeed)
					//PreserveKillFeed();
				

				if (g_Settings.bNoSmoke)
				{
					std::vector<const char*> vistasmoke_mats =
					{
						//	"particle/vistasmokev1/vistasmokev1_fire",
							"particle/vistasmokev1/vistasmokev1_smokegrenade",
							"particle/vistasmokev1/vistasmokev1_emods",
							"particle/vistasmokev1/vistasmokev1_emods_impactdust",
					};

					for (auto matName : vistasmoke_mats)
					{
						IMaterial* mat = g_pMaterialSystem->FindMaterial(matName, "Other textures");
						mat->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
					}
					*(int*)(smokecout) = 0;
				}
				
			break;
			case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
				break;
			case FRAME_RENDER_START:	
				
			//	if (Globals::isfakeducking && g::pLocalEntity && g::pLocalEntity->IsAlive())
			//		g::pLocalEntity->GetAnimState()->m_fDuckAmount = 0.f;

				g::pLocalEntity->SetAbsAngles(QAngle(0, g::pLocalEntity->GetAnimState()->m_flGoalFeetYaw, 0));

				if (g::pLocalEntity && g::pLocalEntity->IsAlive())
				{
					if (*(bool*)((DWORD)g_pInput + 0xAD))
					{
						g_pPrediction->SetLocalViewAngles(Globals::AngleAA);
					}
				}
	
				if(g_Settings.bSkinChangerWindow)
					ApplyPrecachedModels();
				/*
				for (int it = 1; it <= g_pEngine->GetMaxClients(); ++it)
				{
					C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(it);

					if (!pPlayerEntity
						|| !pPlayerEntity->IsAlive()
						|| pPlayerEntity->IsDormant())
						continue;

						animation_fix33(pPlayerEntity);
				}*/
				for (int i = 1; i <= g_pGlobalVars->maxClients; i++)
				{
					if (i == g_pEngine->GetLocalPlayer()) continue;

					C_BaseEntity* pCurEntity = g_pEntityList->GetClientEntity(i);
					if (!pCurEntity) continue;

					*(int*)((uintptr_t)pCurEntity + 0xA30) = g_pGlobalVars->framecount; //we'll skip occlusion checks now
					*(int*)((uintptr_t)pCurEntity + 0xA28) = 0;//clear occlusion flags
				}
	
				/*
				for (int it = 1; it <= g_pEngine->GetMaxClients(); ++it)
				{
					C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(it);

					if (!pPlayerEntity
						|| !pPlayerEntity->IsAlive()
						|| pPlayerEntity->IsDormant())
						continue;

					pPlayerEntity->SetAbsAngles(QAngle(0.f, pPlayerEntity->GetAnimState()->m_flGoalFeetYaw, 0.f));
				}*/
				break;
			case FRAME_NET_UPDATE_END:
				/*for (int it = 1; it <= g_pEngine->GetMaxClients(); ++it)
				{
					C_BaseEntity* pPlayerEntity = g_pEntityList->GetClientEntity(it);

					if (!pPlayerEntity
						|| !pPlayerEntity->IsAlive()
						|| pPlayerEntity->IsDormant()
						|| pPlayerEntity == g::pLocalEntity
						|| pPlayerEntity->GetTeam() == g::pLocalEntity->GetTeam())
						continue;

					apply_interpolation_flags(pPlayerEntity, 0);
				}*/
				break;
		}
	}
	oFrameStageNotify(ecx, stage);
}

void ThirdpersonOV(C_BaseEntity* local)
{
	int button;
	switch (g_Settings.iThirdPersonKey)
	{
	case 0:
		break;
	case 1:
		button = 0x54; //t
		break;
	case 2:
		button = VK_MBUTTON; //m3
		break;
	case 3:
		button = 0x05; //m4
		break;
	}

	/* return if local is nullptr */
	if (g::pLocalEntity == nullptr)
		return;

	/* check if we are connected */
	if (!g_pEngine->IsConnected() && !g_pEngine->IsInGame())
		return;

	/* vec angles */
	static Vector vecAngles;

	/* pointer to localplayer */
	C_BaseEntity* localplayer = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	/* get view angles */
	g_pEngine->GetViewAngles(vecAngles);

	if (g_Settings.bThirdPersonDead && !g::pLocalEntity->IsAlive() && g::pLocalEntity->GetObserverMode() == 4)
	{
		g::pLocalEntity->SetObserverMode(5);
	}

	/* if we have clicked the key */
	if (g_Settings.iThirdPersonKey > 0 && GetKeyState(button) && g_Settings.iThirdPersonDistance > 0 && local->IsAlive())
	{
		/* if we are not in thirdperson */
		if (!g_pInput->m_fCameraInThirdPerson)
		{
			/* getting correct distance */
			auto GetCorrectDistance = [&localplayer](float ideal_distance) -> float
			{
				/* vector for the inverse angles */
				Vector inverseAngles;
				g_pEngine->GetViewAngles(inverseAngles);

				/* inverse angles by 180 */
				inverseAngles.x *= -1.f, inverseAngles.y += 180.f;

				/* vector for direction */
				Vector direction;
				Utils::AngleVectors(inverseAngles, &direction);

				/* ray, trace & filters */
				Ray_t ray;
				trace_t trace;
				CTraceFilter filter;

				/* dont trace local player */
				filter.pSkip = g::pLocalEntity;

				/* create ray */
				ray.Init(localplayer->GetVecOrigin() + localplayer->GetViewOffset(), (localplayer->GetVecOrigin() + localplayer->GetViewOffset()) + (direction * ideal_distance));

				/* trace ray */
				g_pTrace->TraceRay(ray, MASK_SHOT, &filter, &trace);

				/* return the ideal distance */
				return (ideal_distance * trace.fraction) - 10.f;
			};

			/* change the distance from player to camera */
			vecAngles.z = GetCorrectDistance(g_Settings.iThirdPersonDistance);

			/* make player thirdperson */
			g_pInput->m_fCameraInThirdPerson = true;

			/* set camera view */
			g_pInput->m_vecCameraOffset = Vector(vecAngles.x, vecAngles.y, vecAngles.z);
		}
	}
	else
	{
		/* set player to firstperson */
		g_pInput->m_fCameraInThirdPerson = false;

		/* return to default view */
		g_pInput->m_vecCameraOffset = Vector(vecAngles.x, vecAngles.y, 0);
	}
}

void __fastcall Hooks::OverrideView(void* thisptr, void* edx, CViewSetup* setup)
{
	static auto oOverrideView = g_Hooks.pClientModeHook->GetOriginal<OverrideView_t>(18);
	
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());
	
		if (local_player && local_player->GetHealth() > 0 && local_player->IsScoped() && g_Settings.bScopeNoZoom)
			setup->fov = 90;

	ThirdpersonOV(local_player);

//	if (Globals::isfakeducking && g::pLocalEntity && g::pLocalEntity->IsAlive())
//		setup->origin.z = g::pLocalEntity->GetAbsOrigin().z + 64.f;

	oOverrideView(thisptr, setup);
}

void __fastcall Hooks::DrawModelExecute(void* ecx, void* edx, IMatRenderContext* context, const DrawModelState_t& state, const ModelRenderInfo_t& render_info, matrix3x4_t* matrix)
{
	static auto oDrawModelExecute = g_Hooks.pModelRenderHook->GetOriginal<DrawModelExecute_t>(21);
	
	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	auto entity = g_pEntityList->GetClientEntity(render_info.entity_index);

		/// localplayer
		if (entity == local_player)
		{
			if (local_player && local_player->IsScoped())
				g_pRenderView->SetBlend(g_Settings.iScopedBlend * 0.01f);
		}
		/*
		if (render_info.pModel)
		{
			std::string modelName(g_pModelInfo->GetModelName(render_info.pModel));
			if (modelName.find("models/player") != std::string::npos && entity && !entity->IsAlive())
				return;
		}*/
	
		oDrawModelExecute(ecx, context, state, render_info, matrix);
}

void __stdcall Hooks::PaintTraverse(int VGUIPanel, bool ForceRepaint, bool AllowForce)
{
	static auto oPaintTraverse = g_Hooks.pPanelHook->GetOriginal<PaintTraverse_t>(41);

	auto local_player = g_pEntityList->GetClientEntity(g_pEngine->GetLocalPlayer());

	std::string panel_name = g_pPanel->GetName(VGUIPanel);
/*	if (panel_name == "FocusOverlayPanel")
	{
		//LogEvents();
	}
	*/
	// no scope overlay
	if (panel_name == "HudZoom" && g_Settings.bScopeNoZoom)
		return;

	oPaintTraverse(g_pPanel, VGUIPanel, ForceRepaint, AllowForce);
}