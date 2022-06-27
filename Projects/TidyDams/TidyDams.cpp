#include <fstream>
#include "API/ARK/Ark.h"
#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(AShooterGameMode_InitGame, void, AShooterGameMode*, FString*, FString*, FString*);
DECLARE_HOOK(UPrimalInventoryComponent_ServerCloseRemoteInventory, void, UPrimalInventoryComponent*, AShooterPlayerController*);

int configCacheDecayMins;

FString damClassPath = "Blueprint'/Game/PrimalEarth/CoreBlueprints/Inventories/PrimalInventoryBP_BeaverDam.PrimalInventoryBP_BeaverDam'";
UClass* damClass;
FString woodClassPath = "Blueprint'/Game/PrimalEarth/CoreBlueprints/Resources/PrimalItemResource_Wood.PrimalItemResource_Wood'";
UClass* woodClass;

void Hook_AShooterGameMode_InitGame(AShooterGameMode* _this, FString* MapName, FString* Options, FString* ErrorMessage)
{
	AShooterGameMode_InitGame_original(_this, MapName, Options, ErrorMessage);

	damClass = UVictoryCore::BPLoadClass(&damClassPath);
	if (!damClass)
		Log::GetLog()->error("InitGame() - Beaver dam class not found");

	woodClass = UVictoryCore::BPLoadClass(&woodClassPath);
	if (!woodClass)
		Log::GetLog()->error("InitGame() - Wood class not found");
}

void Hook_UPrimalInventoryComponent_ServerCloseRemoteInventory(UPrimalInventoryComponent* _this, AShooterPlayerController* ByPC)
{
	bool isOnlyWood = true;

	UPrimalInventoryComponent_ServerCloseRemoteInventory_original(_this, ByPC);

	if (!damClass || !woodClass)
		return;

	// If the closed inventory is a dam that only contains wood, drop it into an item cache
	if (_this->IsA(damClass) && (_this->InventoryItemsField().Num() > 0))
	{
		for (UPrimalItem* item : _this->InventoryItemsField())
		{
			if (item && !item->IsA(woodClass))
			{
				isOnlyWood = false;
				break;
			}
		}

		if (isOnlyWood) {
			FVector vec = { 0, 0, 0 };
			_this->BPDropInventoryDeposit(ArkApi::GetApiUtils().GetWorld()->GetTimeSeconds() + (configCacheDecayMins * 60), INT_MAX, false, vec);
		}
	}
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

	configCacheDecayMins = config["CacheDecayMins"];
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

	ArkApi::GetHooks().SetHook("AShooterGameMode.InitGame", &Hook_AShooterGameMode_InitGame,
		&AShooterGameMode_InitGame_original);
	ArkApi::GetHooks().SetHook("UPrimalInventoryComponent.ServerCloseRemoteInventory", &Hook_UPrimalInventoryComponent_ServerCloseRemoteInventory,
		&UPrimalInventoryComponent_ServerCloseRemoteInventory_original);

	if (ArkApi::GetApiUtils().GetStatus() != ArkApi::ServerStatus::Ready)
		return;

	damClass = UVictoryCore::BPLoadClass(&damClassPath);
	if (!damClass)
		Log::GetLog()->error("Load() - Beaver dam class not found");

	woodClass = UVictoryCore::BPLoadClass(&woodClassPath);
	if (!woodClass)
		Log::GetLog()->error("Load() - Wood class not found");
}

void Unload()
{
	ArkApi::GetCommands().RemoveConsoleCommand(PLUGIN_NAME".Reload");
	ArkApi::GetCommands().RemoveRconCommand(PLUGIN_NAME".Reload");

	ArkApi::GetHooks().DisableHook("AShooterGameMode.InitGame", &Hook_AShooterGameMode_InitGame);
	ArkApi::GetHooks().DisableHook("UPrimalInventoryComponent.ServerCloseRemoteInventory", &Hook_UPrimalInventoryComponent_ServerCloseRemoteInventory);
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