#include "sdk.hpp"
#include "../helpers/utils.h"
#include "../helpers/console.h"
#include "../settings/globals.h"
#include "../valve_sdk/classids.h"

#include <optional>
#include <map>

#define PRINT_INTERFACE(name) console::print("%-20s: %p", #name, name)

namespace g
{
	CLocalPlayer local_player;
	
	ActiveChannels* active_channel = nullptr;
	Channel* channels = nullptr;
	CInput* input = nullptr;
	IMDLCache* mdl_cache = nullptr;
	ICvar* cvar = nullptr;
	IPanel* vgui_panel = nullptr;
	IVModelInfo* mdl_info = nullptr;
	ISurface* surface = nullptr;
	IClientMode* client_mode = nullptr;
	IMoveHelper* move_helper = nullptr;
	IPrediction* prediction = nullptr;
	IVModelRender* mdl_render = nullptr;
	IVRenderView* render_view = nullptr;
	IEngineTrace* engine_trace = nullptr;
	IEngineSound* engine_sound = nullptr;
	CClientState* client_state = nullptr;
	IInputSystem* input_system = nullptr;
	IVDebugOverlay* debug_overlay = nullptr;
	IGameEventManager2* game_events = nullptr;
	IViewRender* view_render = nullptr;
	IMaterialSystem* mat_system = nullptr;
	IBaseClientDLL* base_client = nullptr;
	CGameMovement* game_movement = nullptr;
	IVEngineClient* engine_client = nullptr;
	CGlobalVarsBase* global_vars = nullptr;
	IDirect3DDevice9* d3_device = nullptr;
	IClientEntityList* entity_list = nullptr;
	IPhysicsSurfaceProps* physics_surface = nullptr;
	CSPlayerResource** player_resource = nullptr;
	CHud* hud_system = nullptr;
	ILocalize* localize = nullptr;
	IMemAlloc* mem_alloc = nullptr;
	IFileSystem* file_system = nullptr;
	IViewRenderBeams* view_render_beams = nullptr;
	ServerClassDLL* server_class = nullptr;
	CHudChat* hud_chat = nullptr;
	CFireBullets* fire_bullets = nullptr;
	CSGameRulesProxy* game_rules_proxy = nullptr;
	
	ISteamUser* steam_user = nullptr;
	ISteamHTTP* steam_http = nullptr;
	ISteamClient* steam_client = nullptr;
	ISteamFriends* steam_friends = nullptr;
	ISteamGameCoordinator* game_coordinator = nullptr;
	ISteamUtils* steam_utils = nullptr;
	ISteamUserStats* steam_user_stats = nullptr;

	template<typename T>
	T* get_interface(const std::string_view moduleName, const std::string_view szInterfaceVersion)
	{
		auto get_module_factory = [](HMODULE module)->CreateInterfaceFn
		{
			return reinterpret_cast<CreateInterfaceFn>(GetProcAddress(module, "CreateInterface"));
		};

		auto result = reinterpret_cast<T*>(get_module_factory(utils::get_module(moduleName.data()))(szInterfaceVersion.data(), nullptr));

		return result;
	}

	template <typename T>
	T get_steam_interface(const std::string_view version)
	{
		static const auto steam_api = utils::get_module("steam_api.dll");
		return reinterpret_cast<T(__cdecl*)(void)>(GetProcAddress(steam_api, version.data()))();
	}

	void get_class_ids()
	{
		for (auto clazz = g::server_class->GetAllClasses(); clazz; clazz = clazz->m_pNext)
		{
			globals::class_ids.insert(std::make_pair(clazz->m_pNetworkName, clazz->m_ClassID));
		}

		classids::initialize();

		g::cvar->ConsolePrintf("\n\nClass ids initialized.\n\n");
	}

	void initialize()
	{
		global_vars = **reinterpret_cast<CGlobalVarsBase***>(utils::pattern_scan(GLOBAL_VARS) + 1);
		client_mode = *reinterpret_cast<IClientMode**>(utils::pattern_scan(CLIENT_MODE) + 1);
		input = *reinterpret_cast<CInput**>(utils::pattern_scan(CINPUT) + 1);
		move_helper = **reinterpret_cast<IMoveHelper***>(utils::pattern_scan(MOVE_HELPER) + 2);
		view_render = *(IViewRender**)(utils::pattern_scan(VIEW_RENDER) + 1);
		d3_device = **reinterpret_cast<IDirect3DDevice9***>(utils::pattern_scan(D3D_DEVICE) + 1);
		client_state = **reinterpret_cast<CClientState***>(utils::pattern_scan(CLIENT_STATE) + 1);
		local_player = *reinterpret_cast<CLocalPlayer*>(utils::pattern_scan(LOCAL_PLAYER) + 2);
		hud_system = *reinterpret_cast<CHud**>(utils::pattern_scan(CHUD) + 1);
		player_resource = *reinterpret_cast<CSPlayerResource***>(utils::pattern_scan(PLAYER_RESOURCE) + 2);
		view_render_beams = *reinterpret_cast<IViewRenderBeams**>(utils::pattern_scan(VIEW_RENDER_BEAMS) + 1);
		fire_bullets = *reinterpret_cast<CFireBullets**>(utils::pattern_scan(FIRE_BULLETS) + 0x131);
		game_rules_proxy = **reinterpret_cast<CSGameRulesProxy***>(utils::pattern_scan(GAME_RULES_PROXY) + 1);
		
		base_client = get_interface<IBaseClientDLL>("client.dll", "VClient018");
		entity_list = get_interface<IClientEntityList>("client.dll", "VClientEntityList003");
		prediction = get_interface<IPrediction>("client.dll", "VClientPrediction001");
		game_movement = get_interface<CGameMovement>("client.dll", "GameMovement001");
		mdl_cache = get_interface<IMDLCache>("datacache.dll", "MDLCache004");
		engine_client = get_interface<IVEngineClient>("engine.dll", "VEngineClient014");
		mdl_info = get_interface<IVModelInfo>("engine.dll", "VModelInfoClient004");
		mdl_render = get_interface<IVModelRender>("engine.dll", "VEngineModel016");
		render_view = get_interface<IVRenderView>("engine.dll", "VEngineRenderView014");
		engine_trace = get_interface<IEngineTrace>("engine.dll", "EngineTraceClient004");
		debug_overlay = get_interface<IVDebugOverlay>("engine.dll", "VDebugOverlay004");
		game_events = get_interface<IGameEventManager2>("engine.dll", "GAMEEVENTSMANAGER002");
		mat_system = get_interface<IMaterialSystem>("materialsystem.dll", "VMaterialSystem080");
		cvar = get_interface<ICvar>("vstdlib.dll", "VEngineCvar007");
		vgui_panel = get_interface<IPanel>("vgui2.dll", "VGUI_Panel009");
		surface = get_interface<ISurface>("vguimatsurface.dll", "VGUI_Surface031");
		physics_surface = get_interface<IPhysicsSurfaceProps>("vphysics.dll", "VPhysicsSurfaceProps001");
		engine_sound = get_interface<IEngineSound>("engine.dll", "IEngineSoundClient003");
		input_system = get_interface<IInputSystem>("inputsystem.dll", "InputSystemVersion001");
		localize = get_interface<ILocalize>("localize.dll", "Localize_001");
		file_system = get_interface<IFileSystem>("filesystem_stdio.dll", "VFileSystem017");
		server_class = get_interface<ServerClassDLL>("server.dll", "ServerGameDLL005");
	
		active_channel = *reinterpret_cast<ActiveChannels**>(utils::pattern_scan("engine.dll", " 8B 1D ? ? ? ? 89 5C 24 48") + 2);
		channels = *reinterpret_cast<Channel**>(utils::pattern_scan("engine.dll", "81 C2 ? ? ? ? 8B 72 54") + 2);

		mem_alloc = *(IMemAlloc**)GetProcAddress(utils::get_module("tier0.dll"), "g_pMemAlloc");

		const auto _steam_user = get_steam_interface<HSteamUser>("SteamAPI_GetHSteamUser");
		const auto _steam_pipe = get_steam_interface<HSteamPipe>("SteamAPI_GetHSteamPipe");

		steam_client = get_steam_interface<ISteamClient*>("SteamClient");
		steam_user = steam_client->GetISteamUser(_steam_user, _steam_user, "SteamUser019");
		game_coordinator = static_cast<ISteamGameCoordinator*>(steam_client->GetISteamGenericInterface(_steam_user, _steam_pipe, "SteamGameCoordinator001"));
		steam_friends = steam_client->GetISteamFriends(_steam_user, _steam_pipe, "SteamFriends015");
		steam_http = steam_client->GetISteamHTTP(_steam_user, _steam_pipe, "STEAMHTTP_INTERFACE_VERSION002");
		steam_utils = steam_client->GetISteamUtils(_steam_pipe, "SteamUtils009");
		steam_user_stats = steam_client->GetISteamUserStats(_steam_user, _steam_pipe, "STEAMUSERSTATS_INTERFACE_VERSION011");

		hud_chat = hud_system->FindHudElement<CHudChat>("CHudChat");

#ifdef _DEBUG
		PRINT_INTERFACE(active_channel);
		PRINT_INTERFACE(channels);
		PRINT_INTERFACE(global_vars);
		PRINT_INTERFACE(client_mode);
		PRINT_INTERFACE(input);
		PRINT_INTERFACE(view_render);
		PRINT_INTERFACE(move_helper);
		PRINT_INTERFACE(d3_device);
		PRINT_INTERFACE(client_state);
		PRINT_INTERFACE(base_client);
		PRINT_INTERFACE(entity_list);
		PRINT_INTERFACE(prediction);
		PRINT_INTERFACE(game_movement);
		PRINT_INTERFACE(mdl_cache);
		PRINT_INTERFACE(engine_client);
		PRINT_INTERFACE(mdl_info);
		PRINT_INTERFACE(mdl_render);
		PRINT_INTERFACE(render_view);
		PRINT_INTERFACE(engine_trace);
		PRINT_INTERFACE(debug_overlay);
		PRINT_INTERFACE(game_events);
		PRINT_INTERFACE(mat_system);
		PRINT_INTERFACE(cvar);
		PRINT_INTERFACE(vgui_panel);
		PRINT_INTERFACE(surface);
		PRINT_INTERFACE(physics_surface);
		PRINT_INTERFACE(local_player);
		PRINT_INTERFACE(game_coordinator);
		PRINT_INTERFACE(steam_client);
		PRINT_INTERFACE(steam_user);
		PRINT_INTERFACE(engine_sound);
		PRINT_INTERFACE(input_system);
		PRINT_INTERFACE(steam_friends);
		PRINT_INTERFACE(steam_http);
		PRINT_INTERFACE(steam_utils);
		PRINT_INTERFACE(hud_system);
		PRINT_INTERFACE(localize);
		PRINT_INTERFACE(mem_alloc);
		PRINT_INTERFACE(file_system);
		PRINT_INTERFACE(view_render_beams);
		PRINT_INTERFACE(hud_chat);
		PRINT_INTERFACE(server_class);	
		PRINT_INTERFACE(steam_user_stats);
#endif
	}
}
