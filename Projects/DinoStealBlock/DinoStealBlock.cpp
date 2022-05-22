#include <fstream>
#include "API/ARK/Ark.h"
#include "BlueprintHooks.h"
#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "BlueprintHooks.lib")

nlohmann::json config;

FString cryoClassPath = "Blueprint'/Game/Extinction/CoreBlueprints/Weapons/PrimalItem_WeaponEmptyCryopod.PrimalItem_WeaponEmptyCryopod'";
UClass* cryoClass = nullptr;
FString pegoClassPath = "Blueprint'/Game/PrimalEarth/Dinos/Pegomastax/Pegomastax_Character_BP.Pegomastax_Character_BP'";
FString canStealItemName = "CanStealItem";

UFunction* GetUFunction(UClass* inClass, FString* funcName)
{
	if (!inClass || !funcName)
		return nullptr;

	for (TPair<FName, UFunction*> pair : inClass->FuncMapField())
		if (pair.Value->NameField().ToString() == *funcName)
			return pair.Value;

	return nullptr;
}

void ScriptHook_CanStealItem(UObject* _this, BlueprintHooks::FFrame* Stack)
{
	if (!cryoClass || !_this || !_this->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
		return;

	UPrimalItem** item = BlueprintHooks::GetFunctionInput<UPrimalItem*>(Stack, FString("Item"));
	bool* canSteal = BlueprintHooks::GetFunctionOutput<bool>(Stack, FString("canSteal"));

	if (!item || !canSteal)
	{
		Log::GetLog()->error("Variable not found");
		return;
	}

	if ((*item)->IsA(cryoClass))
		*canSteal = false;
}

DECLARE_HOOK(AShooterGameMode_InitGame, void, AShooterGameMode*, FString*, FString*, FString*);
void Hook_AShooterGameMode_InitGame(AShooterGameMode* _this, FString* MapName, FString* Options, FString* ErrorMessage)
{
	AShooterGameMode_InitGame_original(_this, MapName, Options, ErrorMessage);

	cryoClass = UVictoryCore::BPLoadClass(&cryoClassPath);
	if (!cryoClass)
		Log::GetLog()->error("InitGame() - Cryopod class not found");

	UClass* pegoClass = UVictoryCore::BPLoadClass(&pegoClassPath);
	UFunction* canStealItem = GetUFunction(pegoClass, &canStealItemName);
	if (canStealItem)
		BlueprintHooks::SetBlueprintPostHook(canStealItem, ScriptHook_CanStealItem);
	else
		Log::GetLog()->error("InitGame() - Pegomastax::CanStealItem UFunction not found");
}

void ReadConfig()
{
	const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/" + PLUGIN_NAME + "/config.json";
	std::ifstream file{ config_path };
	if (!file.is_open())
		throw std::runtime_error("Can't open config.json");

	file >> config;

	file.close();
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

#define SET_HOOK(_class, _func) ArkApi::GetHooks().SetHook(#_class "." #_func, \
	&Hook_ ## _class ## _ ## _func, \
    & ## _class ## _ ## _func ## _original)
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

	SET_HOOK(AShooterGameMode, InitGame);

	if (ArkApi::GetApiUtils().GetStatus() != ArkApi::ServerStatus::Ready)
		return;

	cryoClass = UVictoryCore::BPLoadClass(&cryoClassPath);
	if (!cryoClass)
		Log::GetLog()->error("Load() - Cryopod class not found");

	UClass* pegoClass = pegoClass = UVictoryCore::BPLoadClass(&pegoClassPath);
	UFunction* canStealItem = GetUFunction(pegoClass, &canStealItemName);
	if (canStealItem)
		BlueprintHooks::SetBlueprintPostHook(canStealItem, ScriptHook_CanStealItem);
	else
		Log::GetLog()->error("Load() - CanStealItem UFunction not found");
}

#define DISABLE_HOOK(_class, _func) ArkApi::GetHooks().DisableHook(#_class "." #_func, \
	&Hook_ ## _class ## _ ## _func)
void Unload()
{
	ArkApi::GetCommands().RemoveConsoleCommand(PLUGIN_NAME".Reload");
	ArkApi::GetCommands().RemoveRconCommand(PLUGIN_NAME".Reload");

	DISABLE_HOOK(AShooterGameMode, InitGame);

	UClass* pegoClass = pegoClass = UVictoryCore::BPLoadClass(&pegoClassPath);
	UFunction* canStealItem = GetUFunction(pegoClass, &canStealItemName);
	if (canStealItem)
		BlueprintHooks::DisableBlueprintPostHook(canStealItem, ScriptHook_CanStealItem);
	else
		Log::GetLog()->error("Load() - CanStealItem UFunction not found");
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