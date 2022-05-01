#include <fstream>
#include <cmath>
#include "API/ARK/Ark.h"
#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

#define UPROP_FLAG_Edit                0x001
#define UPROP_FLAG_Parm                0x080
#define UPROP_FLAG_OutParm             0x100

#define USCRIPT_EX_FINAL_FUNCTION_SIZE 9

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


// scriptFunction = The BP script function we are setting a hook in
// nativeFunction = The native function to call at the end of the script
void SetScriptHook(UFunction* scriptUFunction, FNativeFuncPtr nativeFunction)
{
	if (!scriptUFunction)
	{
		Log::GetLog()->error("SetScriptHook() - null scriptUFunction");
		return;
	}

	if (!nativeFunction)
	{
		Log::GetLog()->error("SetScriptHook() - null nativeFunction");
		return;
	}

	// For now, initialize the new UFunction by copying the one we're hooking.
	// This may not be a good idea, need to investigate!
	UFunction* hookUFunction = static_cast<UFunction*>(malloc(scriptUFunction->ClassField()->PropertiesSizeField()));
	if (!hookUFunction)
	{
		Log::GetLog()->error("SetScriptHook() - null hookUFunction");
		return;
	}
	memcpy(hookUFunction, scriptUFunction, scriptUFunction->ClassField()->PropertiesSizeField());

	// Set the native function flag and point the UFunction to our native function
	FString funcName = FString("Hook_") + hookUFunction->NameField().ToString();
	hookUFunction->NameField() = FName(funcName.ToString().c_str(), EFindName::FNAME_Add);
	hookUFunction->FunctionFlagsField() = static_cast<unsigned int>(EFunctionFlags::FUNC_Native);
	hookUFunction->FuncField() = nativeFunction;

	int scriptSize = scriptUFunction->ScriptField().Num();
	if (scriptSize == 0)
	{
		Log::GetLog()->error("SetScriptHook() - Not a script function");
		return;
	}

	if (scriptSize < 3)
	{
		Log::GetLog()->error("SetScriptHook() - Ex_Return not found (< 3 bytes)");
		return;
	}

	// Find the Ex_Return offset
	int hookOffset;
	if ((scriptUFunction->ScriptField()[scriptSize - 3] == static_cast<unsigned char>(EExprToken::EX_Return)) &&
		(scriptUFunction->ScriptField()[scriptSize - 2] == static_cast<unsigned char>(EExprToken::EX_Nothing)) &&
		(scriptUFunction->ScriptField()[scriptSize - 1] == static_cast<unsigned char>(EExprToken::EX_EndOfScript)))
	{
		hookOffset = scriptSize - 3;
	}
	else
	{
		if (scriptSize < 11)
		{
			Log::GetLog()->error("SetScriptHook() - Ex_Return not found (< 11 bytes)");
			return;
		}

		if ((scriptUFunction->ScriptField()[scriptSize - 11] == static_cast<unsigned char>(EExprToken::EX_Return)) &&
			(scriptUFunction->ScriptField()[scriptSize - 10] == static_cast<unsigned char>(EExprToken::EX_LocalOutVariable)) &&
			(scriptUFunction->ScriptField()[scriptSize - 1] == static_cast<unsigned char>(EExprToken::EX_EndOfScript)))
		{
			hookOffset = scriptSize - 11;
		}
		else
		{
			Log::GetLog()->error("SetScriptHook() - Ex_Return not found");
			return;
		}
	}

	// Insert a call to our new UFunction at hookOffset
	unsigned char instructions[USCRIPT_EX_FINAL_FUNCTION_SIZE] = {
		static_cast<unsigned char>(EExprToken::EX_FinalFunction),
		  reinterpret_cast<DWORD64>(hookUFunction) >> 0 & 0xFF,
		  reinterpret_cast<DWORD64>(hookUFunction) >> 8 & 0xFF,
		  reinterpret_cast<DWORD64>(hookUFunction) >> 16 & 0xFF,
		  reinterpret_cast<DWORD64>(hookUFunction) >> 24 & 0xFF,
		  reinterpret_cast<DWORD64>(hookUFunction) >> 32 & 0xFF,
		  reinterpret_cast<DWORD64>(hookUFunction) >> 40 & 0xFF,
		  reinterpret_cast<DWORD64>(hookUFunction) >> 48 & 0xFF,
		  reinterpret_cast<DWORD64>(hookUFunction) >> 56 & 0xFF
	};
	for (int i = USCRIPT_EX_FINAL_FUNCTION_SIZE - 1; i >= 0; --i)
	{
		scriptUFunction->ScriptField().Insert(instructions[i], hookOffset);
	}

	Log::GetLog()->info("Script hook set - " + scriptUFunction->NameField().ToString().ToString() + " -> " + hookUFunction->NameField().ToString().ToString());
}

void UnsetScriptHook(UFunction* scriptUFunction)
{
	if (!scriptUFunction)
	{
		Log::GetLog()->error("SetScriptHook() - null scriptUFunction");
		return;
	}

	int scriptSize = scriptUFunction->ScriptField().Num();
	if (scriptSize == 0)
	{
		Log::GetLog()->error("SetScriptHook() - Not a script function");
		return;
	}

	if (scriptSize < 12)
	{
		Log::GetLog()->error("SetScriptHook() - Hook not found (< 12 bytes)");
		return;
	}

	// Find the Ex_Return offset
	int hookOffset;
	UFunction* hookUFunction = nullptr;
	if ((scriptUFunction->ScriptField()[scriptSize - 12] == static_cast<unsigned char>(EExprToken::EX_FinalFunction)) &&
		(scriptUFunction->ScriptField()[scriptSize - 3] == static_cast<unsigned char>(EExprToken::EX_Return)) &&
		(scriptUFunction->ScriptField()[scriptSize - 2] == static_cast<unsigned char>(EExprToken::EX_Nothing)) &&
		(scriptUFunction->ScriptField()[scriptSize - 1] == static_cast<unsigned char>(EExprToken::EX_EndOfScript)))
	{
		hookOffset = scriptSize - 12;
		hookUFunction = reinterpret_cast<UFunction*>(
			static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 11]) << 0 |
			static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 10]) << 8 |
			static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 9]) << 16 |
			static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 8]) << 24 |
			static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 7]) << 32 |
			static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 6]) << 40 |
			static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 5]) << 48 |
			static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 4]) << 56);
	}
	else
	{
		if (scriptSize < 20)
		{
			Log::GetLog()->error("SetScriptHook() - Hook not found (< 20 bytes)");
			return;
		}

		if ((scriptUFunction->ScriptField()[scriptSize - 20] == static_cast<unsigned char>(EExprToken::EX_FinalFunction)) &&
			(scriptUFunction->ScriptField()[scriptSize - 11] == static_cast<unsigned char>(EExprToken::EX_Return)) &&
			(scriptUFunction->ScriptField()[scriptSize - 10] == static_cast<unsigned char>(EExprToken::EX_LocalOutVariable)) &&
			(scriptUFunction->ScriptField()[scriptSize - 1] == static_cast<unsigned char>(EExprToken::EX_EndOfScript)))
		{
			hookOffset = scriptSize - 20;
			hookUFunction = reinterpret_cast<UFunction*>(
				static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 19]) << 0 |
				static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 18]) << 8 |
				static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 17]) << 16 |
				static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 16]) << 24 |
				static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 15]) << 32 |
				static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 14]) << 40 |
				static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 13]) << 48 |
				static_cast<DWORD64>(scriptUFunction->ScriptField()[scriptSize - 12]) << 56);
		}
		else
		{
			Log::GetLog()->error("SetScriptHook() - Hook not found");
			return;
		}
	}

	scriptUFunction->ScriptField().RemoveAt(hookOffset, USCRIPT_EX_FINAL_FUNCTION_SIZE, true);
	free(hookUFunction);

	Log::GetLog()->info("Blueprint script hook unset - " + scriptUFunction->NameField().ToString().ToString());
}

void ScriptHook_CanDeploy(UObject* _this, FFrame* Stack, void* const Result)
{
	if (!_this || !Stack->NodeField() || !_this->IsA(UPrimalItem::GetPrivateStaticClass()))
		return;

	int numOut = 0;
	for (UProperty* property = Stack->NodeField()->PropertyLinkField(); property; property = property->PropertyLinkNextField())
		if ((property->PropertyFlagsField() & UPROP_FLAG_Parm) && (property->PropertyFlagsField() & UPROP_FLAG_OutParm))
			++numOut;

	UPrimalItem* item = static_cast<UPrimalItem*>(_this);
	AShooterCharacter* shooterCharacter = item->GetOwnerPlayer();
	if (!shooterCharacter)
		return;

	AShooterPlayerController* shooterController = ArkApi::GetApiUtils().FindControllerFromCharacter(shooterCharacter);
	if (!shooterController)
		return;

	bool* can = nullptr;
	FVector* newLocation = nullptr;
	FString* failureReason = nullptr;

	FOutParmRec* outRec = Stack->OutParmsField();
	for (int i = 0; i < numOut; ++i)
	{
		if (outRec->PropertyField()->NameField().ToString() == FString("Can"))
			can = reinterpret_cast<bool*>(outRec->PropAddrField());
		else if (outRec->PropertyField()->NameField().ToString() == FString("NewLocation"))
			newLocation = reinterpret_cast<FVector*>(outRec->PropAddrField());
		else if (outRec->PropertyField()->NameField().ToString() == FString("FailureReason"))
			failureReason = reinterpret_cast<FString*>(outRec->PropAddrField());

		outRec = outRec->NextOutParmField();
	}
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

	UClass* cryoClass = cryoClass = UVictoryCore::BPLoadClass(&cryoClassPath);
	UFunction* canDeploy = GetUFunction(cryoClass, &canDeployName);
	if (canDeploy)
		SetScriptHook(canDeploy, ScriptHook_CanDeploy);
	else
		Log::GetLog()->error("Load() - CanDeploy UFunction not found");
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

	UClass* cryoClass = cryoClass = UVictoryCore::BPLoadClass(&cryoClassPath);
	UFunction* canDeploy = GetUFunction(cryoClass, &canDeployName);
	if (canDeploy)
		SetScriptHook(canDeploy, ScriptHook_CanDeploy);
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

	UClass* cryoClass = cryoClass = UVictoryCore::BPLoadClass(&cryoClassPath);
	UFunction* canDeploy = GetUFunction(cryoClass, &canDeployName);
	if (canDeploy)
		UnsetScriptHook(canDeploy);
	else
		Log::GetLog()->error("Load() - CanDeploy UFunction not found");
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