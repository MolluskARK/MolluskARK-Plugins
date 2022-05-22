#include <fstream>
#include <cmath>
#include "API/ARK/Ark.h"
#include "BlueprintHooks.h"
#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")
#pragma comment(lib, "BlueprintHooks.lib")

nlohmann::json config;

FString cryoClassPath = "Blueprint'/Game/Extinction/CoreBlueprints/Weapons/PrimalItem_WeaponEmptyCryopod.PrimalItem_WeaponEmptyCryopod'";
FString canDeployName = "CanDeploy";

UFunction* GetUFunction(UClass* inClass, FString* funcName)
{
	if (!inClass || !funcName)
		return nullptr;

	for (TPair<FName, UFunction*> pair : inClass->FuncMapField())
		if (pair.Value->NameField().ToString() == *funcName)
			return pair.Value;

	return nullptr;
}

void ScriptHook_CanDeploy(UObject* _this, BlueprintHooks::FFrame* Stack)
{
	if (!_this || !_this->IsA(UPrimalItem::GetPrivateStaticClass()))
		return;

	UPrimalItem* item = static_cast<UPrimalItem*>(_this);
	AShooterCharacter* shooterCharacter = item->GetOwnerPlayer();
	if (!shooterCharacter)
		return;

	AShooterPlayerController* shooterController = ArkApi::GetApiUtils().FindControllerFromCharacter(shooterCharacter);
	if (!shooterController)
		return;

	bool* can = BlueprintHooks::GetFunctionOutput<bool>(Stack, FString("Can"));
	FVector* newLocation = BlueprintHooks::GetFunctionOutput<FVector>(Stack, FString("NewLocation"));
	FString* failureReason = BlueprintHooks::GetFunctionOutput<FString>(Stack, FString("FailureReason"));

	if (!can || !newLocation || !failureReason)
	{
		Log::GetLog()->error("Output param not found");
		return;
	}

	FString mapName;
	ArkApi::GetApiUtils().GetShooterGameMode()->GetMapName(&mapName);
	nlohmann::json messageTypes = config[mapName.ToString()];
	for (nlohmann::json messageType : messageTypes)
	{
		for (nlohmann::json zone : messageType["Boxes"])
		{
			if (((zone["Point1"][0] <= zone["Point2"][0]) ? ((zone["Point1"][0] <= newLocation->X) && (newLocation->X <= zone["Point2"][0])) : ((zone["Point2"][0] <= newLocation->X) && (newLocation->X <= zone["Point1"][0]))) &&
				((zone["Point1"][1] <= zone["Point2"][1]) ? ((zone["Point1"][1] <= newLocation->Y) && (newLocation->Y <= zone["Point2"][1])) : ((zone["Point2"][1] <= newLocation->Y) && (newLocation->Y <= zone["Point1"][1]))) &&
				((zone["Point1"][2] <= zone["Point2"][2]) ? ((zone["Point1"][2] <= newLocation->Z) && (newLocation->Z <= zone["Point2"][2])) : ((zone["Point2"][2] <= newLocation->Z) && (newLocation->Z <= zone["Point1"][2]))))
			{
				std::string message = messageType["Message"];
				*failureReason = FString(message);
				*can = false;
				break;
			}
		}
	}
}

DECLARE_HOOK(AShooterGameMode_InitGame, void, AShooterGameMode*, FString*, FString*, FString*);
void Hook_AShooterGameMode_InitGame(AShooterGameMode* _this, FString* MapName, FString* Options, FString* ErrorMessage)
{
	AShooterGameMode_InitGame_original(_this, MapName, Options, ErrorMessage);

	if (!ArkApi::Tools::IsPluginLoaded("BlueprintHooks"))
	{
		Log::GetLog()->error("InitGame() - BlueprintHooks not loaded");
		return;
	}

	UClass* cryoClass = UVictoryCore::BPLoadClass(&cryoClassPath);
	UFunction* canDeploy = GetUFunction(cryoClass, &canDeployName);
	if (canDeploy)
		BlueprintHooks::SetBlueprintPostHook(canDeploy, ScriptHook_CanDeploy);
	else
		Log::GetLog()->error("InitGame() - CanDeploy UFunction not found");
}

void ZonesCmd(APlayerController* playerController, FString*, bool)
{
	if (!playerController->IsA(AShooterPlayerController::GetPrivateStaticClass()))
		return;

	AShooterPlayerController* shooterController = static_cast<AShooterPlayerController*>(playerController);
	FString mapName;
	ArkApi::GetApiUtils().GetShooterGameMode()->GetMapName(&mapName);
	nlohmann::json messageTypes = config[mapName.ToString()];
	for (nlohmann::json messageType : messageTypes)
	{
		for (nlohmann::json zone : messageType["Boxes"])
		{
			FVector point1 = { (float)zone["Point1"][0], (float)zone["Point1"][1], (float)zone["Point1"][2] };
			FVector point2 = { (float)zone["Point2"][0], (float)zone["Point2"][1], (float)zone["Point2"][2] };
			FVector zero = { 0, 0, 0 };
			FVector location = { ((float)zone["Point1"][0] + zone["Point2"][0]) / 2, ((float)zone["Point1"][1] + zone["Point2"][1]) / 2, ((float)zone["Point1"][2] + zone["Point2"][2]) / 2 };
			FVector extant = { std::abs((float)zone["Point1"][0] - zone["Point2"][0]) / 2, std::abs((float)zone["Point1"][1] - zone["Point2"][1]) / 2, std::abs((float)zone["Point1"][2] - zone["Point2"][2]) / 2 };
			FRotator rot = { 0, 0, 0 };

			shooterController->MulticastDrawDebugBox(location, extant, FColorList::Green, rot, 30, true);
			FString text = "Point1";
			shooterController->ClientAddFloatingText(point1, &text, FColorList::Red, 0.75f, 0.75f, 30, zero, 0.2f, 0.2f, 0);
			text = "Point2";
			shooterController->ClientAddFloatingText(point2, &text, FColorList::Red, 0.75f, 0.75f, 30, zero, 0.2f, 0.2f, 0);
		}
	}
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

	ArkApi::GetCommands().AddConsoleCommand(PLUGIN_NAME".Zones", &ZonesCmd);

	SET_HOOK(AShooterGameMode, InitGame);

	if (ArkApi::GetApiUtils().GetStatus() != ArkApi::ServerStatus::Ready)
		return;

	if (!ArkApi::Tools::IsPluginLoaded("BlueprintHooks"))
	{
		Log::GetLog()->error("Load() - BlueprintHooks not loaded");
		return;
	}

	UClass* cryoClass = cryoClass = UVictoryCore::BPLoadClass(&cryoClassPath);
	UFunction* canDeploy = GetUFunction(cryoClass, &canDeployName);
	if (canDeploy)
	{
		BlueprintHooks::SetBlueprintPostHook(canDeploy, ScriptHook_CanDeploy);
	}
	else
		Log::GetLog()->error("Load() - CanDeploy UFunction not found");
}

#define DISABLE_HOOK(_class, _func) ArkApi::GetHooks().DisableHook(#_class "." #_func, \
	&Hook_ ## _class ## _ ## _func)
void Unload()
{
	ArkApi::GetCommands().RemoveConsoleCommand(PLUGIN_NAME".Reload");
	ArkApi::GetCommands().RemoveRconCommand(PLUGIN_NAME".Reload");

	ArkApi::GetCommands().RemoveConsoleCommand(PLUGIN_NAME".Zones");

	DISABLE_HOOK(AShooterGameMode, InitGame);

	if (!ArkApi::Tools::IsPluginLoaded("BlueprintHooks"))
	{
		Log::GetLog()->error("Unload() - BlueprintHooks not loaded");
		return;
	}

	UClass* cryoClass = cryoClass = UVictoryCore::BPLoadClass(&cryoClassPath);
	UFunction* canDeploy = GetUFunction(cryoClass, &canDeployName);
	if (canDeploy) {
		BlueprintHooks::DisableBlueprintPostHook(canDeploy, ScriptHook_CanDeploy);
	}
	else
		Log::GetLog()->error("Unload() - CanDeploy UFunction not found");
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