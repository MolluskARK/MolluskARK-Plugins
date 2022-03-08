#include <fstream>
#include "API/ARK/Ark.h"
#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(UWorld_InitWorld, void, UWorld*, UWorld::InitializationValues);

std::string configMessage;

void Hook_UWorld_InitWorld(UWorld* _this, UWorld::InitializationValues IVS)
{
	Log::GetLog()->info("UWorld::InitWorld() called");
	UWorld_InitWorld_original(_this, IVS);
}

void TestCmd(AShooterPlayerController* shooterController, FString* input, EChatSendMode::Type)
{
	ArkApi::GetApiUtils().SendChatMessage(shooterController, "/test", configMessage.c_str());
}

void ReadConfig()
{
	nlohmann::json config;
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/" + PLUGIN_NAME + "/config.json";
	std::ifstream file{ config_path };
	if (!file.is_open())
		throw std::runtime_error("Can't open config.json");

	file >> config;

	file.close();

	configMessage = config["Message"];
}

void ReloadConfig(APlayerController* playerController, FString*, bool)
{
	AShooterPlayerController* shooterController = static_cast<AShooterPlayerController*>(playerController);

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		ArkApi::GetApiUtils().SendServerMessage(shooterController, FColorList::Red, "Failed to reload config");

		Log::GetLog()->error(error.what());
		return;
	}

	ArkApi::GetApiUtils().SendServerMessage(shooterController, FColorList::Green, "Reloaded config");
}

void ReloadConfigRcon(RCONClientConnection* connection, RCONPacket* packet, UWorld*)
{
	FString reply;

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());

		reply = error.what();
		connection->SendMessageW(packet->Id, 0, &reply);
		return;
	}

	reply = "Reloaded config";
	connection->SendMessageW(packet->Id, 0, &reply);
}

void Load()
{
	Log::Get().Init(PLUGIN_NAME);

	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}

	ArkApi::GetCommands().AddConsoleCommand(PLUGIN_NAME".Reload", &ReloadConfig);
	ArkApi::GetCommands().AddRconCommand(PLUGIN_NAME".Reload", &ReloadConfigRcon);

	ArkApi::GetCommands().AddChatCommand("/test", &TestCmd);

	ArkApi::GetHooks().SetHook("UWorld.InitWorld", &Hook_UWorld_InitWorld,
		&UWorld_InitWorld_original);
}

void Unload()
{
	ArkApi::GetCommands().RemoveConsoleCommand(PLUGIN_NAME".Reload");
	ArkApi::GetCommands().RemoveRconCommand(PLUGIN_NAME".Reload");

	ArkApi::GetCommands().RemoveChatCommand("/test");

	ArkApi::GetHooks().DisableHook("UWorld.InitWorld", &Hook_UWorld_InitWorld);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		Unload();
		break;
	}
	return TRUE;
}