#include <fstream>
#include "API/ARK/Ark.h"
#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(UPrimalInventoryComponent_IsAllowedInventoryAccess, bool, UPrimalInventoryComponent*, APlayerController*);
DECLARE_HOOK(APrimalStructureItemContainer_CanOpen, bool, APrimalStructureItemContainer*, APlayerController*);
DECLARE_HOOK(APrimalStructureItemContainer_CanBeActivated, bool, APrimalStructureItemContainer*);
DECLARE_HOOK(APrimalStructureItemContainer_BPCanBeActivatedByPlayer, bool, APrimalStructureItemContainer*, AShooterPlayerController*);

#define ACCESS_TRIBE	1
#define ACCESS_ALLIES	2
#define ACCESS_ALL		3

uint8 configLivePlayer;
uint8 configDeadPlayer;
uint8 configLiveDino;
uint8 configDeadDino;
uint8 configItemCache;
uint8 configActivate;

FString itemCacheClassPath = "Blueprint'/Game/PrimalEarth/Structures/DeathItemCache.DeathItemCache'";
UClass* itemCacheClass;

// This gets called when attempting to access an inventory, but also on other interactions like using the multi-use menu.
// Don't send a notification when blocking access since it may not be relevant to the player.
bool Hook_UPrimalInventoryComponent_IsAllowedInventoryAccess(UPrimalInventoryComponent* _this, APlayerController* ForPC)
{
	if (_this->GetOwner() && ForPC && _this->GetOwner()->IsA(APrimalCharacter::GetPrivateStaticClass()) && ForPC->IsA(AShooterPlayerController::GetPrivateStaticClass()))
	{
		APrimalCharacter* invCharacter = static_cast<APrimalCharacter*>(_this->GetOwner());
		int invTeam = invCharacter->TargetingTeamField();
		AShooterPlayerController* forController = static_cast<AShooterPlayerController*>(ForPC);

		if (invTeam >= 50000)
		{
			if (_this->GetOwner()->IsA(AShooterCharacter::GetPrivateStaticClass()))
			{
				if ((configLivePlayer != ACCESS_ALL) && invCharacter->GetCharacterStatusComponent() && (invCharacter->GetCharacterStatusComponent()->BPGetCurrentStatusValue(EPrimalCharacterStatusValue::Health) > 0))
				{
					// Live player character
					if (!invCharacter->IsAlliedWithOtherTeam(forController->TargetingTeamField()))
						return false;
					else if ((invTeam != forController->TargetingTeamField()) && (configLivePlayer != ACCESS_ALLIES))
						return false;
				}
				else if (configDeadPlayer != ACCESS_ALL)
				{
					// Dead player character
					if (!invCharacter->IsAlliedWithOtherTeam(forController->TargetingTeamField()) &&
						(static_cast<AShooterCharacter*>(invCharacter)->LinkedPlayerDataIDField() != ArkApi::GetApiUtils().GetPlayerID(ForPC)))
						return false;
					else if ((invTeam != forController->TargetingTeamField()) && (configDeadPlayer != ACCESS_ALLIES) &&
						(static_cast<AShooterCharacter*>(invCharacter)->LinkedPlayerDataIDField() != ArkApi::GetApiUtils().GetPlayerID(ForPC)))
						return false;
				}
			}
			else if (_this->GetOwner()->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
			{
				if ((configLiveDino != ACCESS_ALL) && invCharacter->GetCharacterStatusComponent() && (invCharacter->GetCharacterStatusComponent()->BPGetCurrentStatusValue(EPrimalCharacterStatusValue::Health) > 0))
				{
					// Live dino character
					if (!invCharacter->IsAlliedWithOtherTeam(forController->TargetingTeamField()))
						return false;
					else if ((invTeam != forController->TargetingTeamField()) && (configLiveDino != ACCESS_ALLIES))
						return false;
				}
				else if (configDeadDino != ACCESS_ALL)
				{
					// Dead dino character
					if (!invCharacter->IsAlliedWithOtherTeam(forController->TargetingTeamField()))
						return false;
					else if ((invTeam != forController->TargetingTeamField()) && (configDeadDino != ACCESS_ALLIES))
						return false;
				}
			}
		}
	}

	return UPrimalInventoryComponent_IsAllowedInventoryAccess_original(_this, ForPC);
}

bool Hook_APrimalStructureItemContainer_CanOpen(APrimalStructureItemContainer* _this, APlayerController* ForPC)
{
	AShooterPlayerController* forController = static_cast<AShooterPlayerController*>(ForPC);

	if (ForPC && ForPC->IsA(AShooterPlayerController::GetPrivateStaticClass()) && static_cast<AShooterPlayerController*>(ForPC)->CharacterField() &&
		static_cast<AShooterPlayerController*>(ForPC)->CharacterField()->IsA(AShooterCharacter::GetPrivateStaticClass()))
	{
		AShooterCharacter* forCharacter = static_cast<AShooterCharacter*>(forController->CharacterField());
		int invTeam = _this->TargetingTeamField();

		if (!itemCacheClass)
			itemCacheClass = UVictoryCore::BPLoadClass(&itemCacheClassPath);

		if ((invTeam >= 50000) && _this->IsA(itemCacheClass) && (invTeam != forCharacter->LinkedPlayerDataIDField()))
		{
			if ((_this->DeathCacheCreationTimeField() > 0) && (configDeadPlayer != ACCESS_ALL))
			{
				// Dead player cache
				if (!forCharacter->IsAlliedWithOtherTeam(invTeam))
					goto BLOCKED;
				else if ((invTeam != forController->TargetingTeamField()) && (configDeadPlayer != ACCESS_ALLIES))
					goto BLOCKED;
			}
			else if ((_this->DeathCacheCreationTimeField() == 0) && (configDeadDino != ACCESS_ALL))
			{
				// Dead dino cache
				if (!forCharacter->IsAlliedWithOtherTeam(invTeam))
					goto BLOCKED;
				else if ((invTeam != forController->TargetingTeamField()) && (configDeadDino != ACCESS_ALLIES))
					goto BLOCKED;
			}
			else if ((_this->DeathCacheCreationTimeField() < 0) && (configItemCache != ACCESS_ALL))
			{
				// Intentionally dropped cache
				if (!forCharacter->IsAlliedWithOtherTeam(invTeam))
					goto BLOCKED;
				else if ((invTeam != forController->TargetingTeamField()) && (configItemCache != ACCESS_ALLIES))
					goto BLOCKED;
			}
		}
	}

	return APrimalStructureItemContainer_CanOpen_original(_this, ForPC);

BLOCKED:
	ArkApi::GetApiUtils().SendNotification(forController, FColorList::Red, (float)1.1, 4, NULL, "You do not have access to this item cache!");
	return false;
}

bool Hook_APrimalStructureItemContainer_CanBeActivated(APrimalStructureItemContainer* _this)
{
	if (_this->TargetingTeamField() >= 50000)
		_this->bUseBPCanBeActivatedByPlayer().Set(true);

	return APrimalStructureItemContainer_CanBeActivated_original(_this);
}

bool Hook_APrimalStructureItemContainer_BPCanBeActivatedByPlayer(APrimalStructureItemContainer* _this, AShooterPlayerController* PC)
{
	AShooterPlayerController* forController = static_cast<AShooterPlayerController*>(PC);

	if (static_cast<AShooterPlayerController*>(PC)->CharacterField() && static_cast<AShooterPlayerController*>(PC)->CharacterField()->IsA(AShooterCharacter::GetPrivateStaticClass()))
	{
		AShooterCharacter* forCharacter = static_cast<AShooterCharacter*>(forController->CharacterField());
		if (_this->TargetingTeamField() >= 50000)
		{
			if ((configActivate == ACCESS_ALL) || (_this->OwningPlayerIDField() == forCharacter->LinkedPlayerDataIDField()) || (_this->bIsLocked().Get() && _this->IsPlayerControllerInPinCodeValidationList(PC)))
				return true;
			else if (!forCharacter->IsAlliedWithOtherTeam(_this->TargetingTeamField()))
				goto BLOCKED;
			else if ((_this->TargetingTeamField() != PC->TargetingTeamField()) && ((configActivate != ACCESS_ALLIES)))
				goto BLOCKED;
			else
				return true;
		}
	}

	return APrimalStructureItemContainer_BPCanBeActivatedByPlayer_original(_this, PC);

BLOCKED:
	ArkApi::GetApiUtils().SendNotification(PC, FColorList::Red, (float)1.1, 4, NULL, "You do not have access to this structure!");
	return false;
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

	FString var = FString(static_cast<std::string>(config["LivePlayer"])).ToLower();
	if (var.Equals("tribe"))
		configLivePlayer = ACCESS_TRIBE;
	else if (var.Equals("allies"))
		configLivePlayer = ACCESS_ALLIES;
	else if (var.Equals("all"))
		configLivePlayer = ACCESS_ALL;
	else
		throw std::runtime_error("LivePlayer - Invalid value");

	var = FString(static_cast<std::string>(config["DeadPlayer"])).ToLower();
	if (var.Equals("tribe"))
		configDeadPlayer = ACCESS_TRIBE;
	else if (var.Equals("allies"))
		configDeadPlayer = ACCESS_ALLIES;
	else if (var.Equals("all"))
		configDeadPlayer = ACCESS_ALL;
	else
		throw std::runtime_error("DeadPlayer - Invalid value");

	var = FString(static_cast<std::string>(config["LiveDino"])).ToLower();
	if (var.Equals("tribe"))
		configLiveDino = ACCESS_TRIBE;
	else if (var.Equals("allies"))
		configLiveDino = ACCESS_ALLIES;
	else if (var.Equals("all"))
		configLiveDino = ACCESS_ALL;
	else
		throw std::runtime_error("DeadDino - Invalid value");

	var = FString(static_cast<std::string>(config["DeadDino"])).ToLower();
	if (var.Equals("tribe"))
		configDeadDino = ACCESS_TRIBE;
	else if (var.Equals("allies"))
		configDeadDino = ACCESS_ALLIES;
	else if (var.Equals("all"))
		configDeadDino = ACCESS_ALL;
	else
		throw std::runtime_error("DeadDino - Invalid value");

	var = FString(static_cast<std::string>(config["ItemCache"])).ToLower();
	if (var.Equals("tribe"))
		configItemCache = ACCESS_TRIBE;
	else if (var.Equals("allies"))
		configItemCache = ACCESS_ALLIES;
	else if (var.Equals("all"))
		configItemCache = ACCESS_ALL;
	else
		throw std::runtime_error("ItemCache - Invalid value");

	var = FString(static_cast<std::string>(config["Activate"])).ToLower();
	if (var.Equals("tribe"))
		configActivate = ACCESS_TRIBE;
	else if (var.Equals("allies"))
		configActivate = ACCESS_ALLIES;
	else if (var.Equals("all"))
		configActivate = ACCESS_ALL;
	else
		throw std::runtime_error("Activate - Invalid value");
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

	ArkApi::GetHooks().SetHook("UPrimalInventoryComponent.IsAllowedInventoryAccess", &Hook_UPrimalInventoryComponent_IsAllowedInventoryAccess,
		&UPrimalInventoryComponent_IsAllowedInventoryAccess_original);
	ArkApi::GetHooks().SetHook("APrimalStructureItemContainer.CanOpen", &Hook_APrimalStructureItemContainer_CanOpen,
		&APrimalStructureItemContainer_CanOpen_original);
	ArkApi::GetHooks().SetHook("APrimalStructureItemContainer.CanBeActivated", &Hook_APrimalStructureItemContainer_CanBeActivated,
		&APrimalStructureItemContainer_CanBeActivated_original);
	ArkApi::GetHooks().SetHook("APrimalStructureItemContainer.BPCanBeActivatedByPlayer", &Hook_APrimalStructureItemContainer_BPCanBeActivatedByPlayer,
		&APrimalStructureItemContainer_BPCanBeActivatedByPlayer_original);
}

void Unload()
{
	ArkApi::GetCommands().RemoveConsoleCommand(PLUGIN_NAME".Reload");
	ArkApi::GetCommands().RemoveRconCommand(PLUGIN_NAME".Reload");

	ArkApi::GetHooks().DisableHook("UPrimalInventoryComponent.IsAllowedInventoryAccess", &Hook_UPrimalInventoryComponent_IsAllowedInventoryAccess);
	ArkApi::GetHooks().DisableHook("APrimalStructureItemContainer.CanOpen", &Hook_APrimalStructureItemContainer_CanOpen);
	ArkApi::GetHooks().DisableHook("APrimalStructureItemContainer.CanBeActivated", &Hook_APrimalStructureItemContainer_CanBeActivated);
	ArkApi::GetHooks().DisableHook("APrimalStructureItemContainer.BPCanBeActivatedByPlayer", &Hook_APrimalStructureItemContainer_BPCanBeActivatedByPlayer);
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