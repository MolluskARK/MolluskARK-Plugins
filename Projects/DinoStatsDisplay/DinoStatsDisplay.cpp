#include <map>
#include <sstream>
#include <iomanip>
#include "API/ARK/Ark.h"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(AShooterWeapon_Tick, void, AShooterWeapon*, float);
DECLARE_HOOK(AShooterPlayerController_ServerActorViewRemoteInventory_Implementation, void, AShooterPlayerController*, UPrimalInventoryComponent*);
DECLARE_HOOK(UPrimalCharacterStatusComponent_ServerApplyLevelUp, void, UPrimalCharacterStatusComponent*, EPrimalCharacterStatusValue::Type, AShooterPlayerController*);
DECLARE_HOOK(AShooterCharacter_ServerSetViewingInventory_Implementation, void, AShooterCharacter*, bool);
DECLARE_HOOK(AShooterCharacter_OnWeaponEquipped, void, AShooterCharacter*, AShooterWeapon*);
DECLARE_HOOK(AShooterCharacter_OnWeaponUnequipped, void, AShooterCharacter*, AShooterWeapon*);
DECLARE_HOOK(APrimalDinoCharacter_OnCryo, void, APrimalDinoCharacter*, AShooterPlayerController*);
DECLARE_HOOK(APrimalDinoCharacter_OnUncryo, void, APrimalDinoCharacter*, AShooterPlayerController*);

#define STATS_WILD_HEALTH   0
#define STATS_WILD_STAMINA  1
#define STATS_WILD_OXYGEN   3
#define STATS_WILD_FOOD     4
#define STATS_WILD_WEIGHT   7
#define STATS_WILD_DAMAGE   8
#define STATS_WILD_SPEED    9
#define STATS_DOM_HEALTH    12
#define STATS_DOM_STAMINA   13
#define STATS_DOM_OXYGEN    15
#define STATS_DOM_FOOD      16
#define STATS_DOM_WEIGHT    19
#define STATS_DOM_DAMAGE    20
#define STATS_DOM_SPEED     21

struct _SpyglassTarget {
	int           dinoDomLevel;
	int           dinoTeam;
	unsigned int  dinoId1;
	unsigned int  dinoId2;
	FString       dinoName;
	FString       dinoStats;
	FColor        color;
	bool          isInInventory;
};
std::map<uint64, struct _SpyglassTarget> SpyglassTargets;

FString spyglassClassPath = "Blueprint'/Game/PrimalEarth/CoreBlueprints/Weapons/WeapSpyglass.WeapSpyglass'";
UClass* spyglassClass;

FString cryoClassPath = "Blueprint'/Game/Extinction/CoreBlueprints/Weapons/WeapEmptyCryopod.WeapEmptyCryopod'";
UClass* cryoClass;

// Cache target info
void SetSpyglassTargetInfo(AShooterPlayerController* shooterController, APrimalDinoCharacter* target, bool isInCryo)
{
	uint64 id = ArkApi::GetApiUtils().GetSteamIdFromController(shooterController);
	UPrimalCharacterStatusComponent* targetComponent = target->GetCharacterStatusComponent();

	if (!targetComponent || (targetComponent->BPGetCurrentStatusValue(EPrimalCharacterStatusValue::Health) <= 0))
		return;

	char* levelUps = targetComponent->NumberOfLevelUpPointsAppliedField()();
	FString dinoName;
	FString name;

	target->GetDescriptiveName(&name);

	if (isInCryo)
		SpyglassTargets[id].dinoName = "CRYOPOD: " + name;
	else
		SpyglassTargets[id].dinoName = name;

	if (target->TribeNameField().Len() > 0)
		SpyglassTargets[id].dinoName += " (" + target->TribeNameField() + ")";
	else if (target->OwningPlayerNameField().Len())
		SpyglassTargets[id].dinoName += " (" + target->OwningPlayerNameField() + ")";

	SpyglassTargets[id].dinoStats = FString("Health:  " + std::to_string(levelUps[STATS_WILD_HEALTH]) + (levelUps[STATS_DOM_HEALTH] ? " (" + std::to_string(levelUps[STATS_DOM_HEALTH]) + ")" : "  ") +
		"   Stamina:  " + std::to_string(levelUps[STATS_WILD_STAMINA]) + (levelUps[STATS_DOM_STAMINA] ? " (" + std::to_string(levelUps[STATS_DOM_STAMINA]) + ")" : "  ") +
		"   Oxygen:  " + std::to_string(levelUps[STATS_WILD_OXYGEN]) + (levelUps[STATS_DOM_OXYGEN] ? " (" + std::to_string(levelUps[STATS_DOM_OXYGEN]) + ")" : "  ") +
		"   Food:  " + std::to_string(levelUps[STATS_WILD_FOOD]) + (levelUps[STATS_DOM_FOOD] ? " (" + std::to_string(levelUps[STATS_DOM_FOOD]) + ")" : "  ") +
		"\nWeight:  " + std::to_string(levelUps[STATS_WILD_WEIGHT]) + (levelUps[STATS_DOM_WEIGHT] ? " (" + std::to_string(levelUps[STATS_DOM_WEIGHT]) + ")" : "  ") +
		"   Damage:  " + std::to_string(levelUps[STATS_WILD_DAMAGE]) + (levelUps[STATS_DOM_DAMAGE] ? " (" + std::to_string(levelUps[STATS_DOM_DAMAGE]) + ")" : "  ") +
		"   Speed:  " + std::to_string(levelUps[STATS_WILD_SPEED]) + (levelUps[STATS_DOM_SPEED] ? " (" + std::to_string(levelUps[STATS_DOM_SPEED]) + ")" : "  "));

	if (target->TargetingTeamField() == shooterController->TargetingTeamField())
		SpyglassTargets[id].color = FColorList::Green;
	else
		SpyglassTargets[id].color = FColorList::Yellow;

	SpyglassTargets[id].dinoDomLevel = targetComponent->ExtraCharacterLevelField();
	SpyglassTargets[id].dinoTeam = target->TargetingTeamField();
	SpyglassTargets[id].dinoId1 = target->DinoID1Field();
	SpyglassTargets[id].dinoId2 = target->DinoID2Field();
}

// Display cached target info for an AShooterPlayerController
void ShowSpyglassTargetInfo(AShooterPlayerController* shooterController, float DisplayTime)
{
	uint64 id = ArkApi::GetApiUtils().GetSteamIdFromController(shooterController);
	FString emptyLine = " ";
	FString emptyLines = " \n ";

	shooterController->ClientServerNotificationSingle((SpyglassTargets[id].isInInventory || (DisplayTime == 0)) ? &emptyLine : &SpyglassTargets[id].dinoName, SpyglassTargets[id].color, 0.7f, DisplayTime, nullptr, nullptr, 0xABABAB);
	shooterController->ClientServerNotificationSingle((DisplayTime == 0) ? &emptyLines : &SpyglassTargets[id].dinoStats, SpyglassTargets[id].color, 0.6f, DisplayTime, nullptr, nullptr, 0xABABABC);
}

// Display stats if a dino is in the item
void ShowCryopodStats(UPrimalItem* item, AShooterPlayerController* shooterController, int msgId)
{
	FCustomItemData itemData;
	item->GetCustomItemData(FName("Dino", EFindName::FNAME_Find), &itemData);
	if ((itemData.CustomDataStrings.Num() < 2) || (itemData.CustomDataBytes.ByteArrays.Num() < 1) || (itemData.CustomDataBytes.ByteArrays[0].Bytes.Num() < sizeof(FCustomItemByteArray)))
		return;

	FVector vec{ 0,0,0 };
	FRotator rotation{ 0,0,0 };
	FARKDinoData dinoData;

	dinoData.DinoNameInMap = itemData.CustomDataStrings[0];
	dinoData.DinoName = itemData.CustomDataStrings[1];
	dinoData.DinoClass = itemData.CustomDataClasses[0];
	dinoData.DinoData = itemData.CustomDataBytes.ByteArrays[0].Bytes;

	// WARNING: This will not build against the main Michidu/ARK-Server-API repo. SpawnFromDinoData() is not declared there. SpawnFromDinoDataEX() is declared incorrectly there and will lead to undesired behavior and crashes.
	APrimalDinoCharacter* dino = APrimalDinoCharacter::SpawnFromDinoData(&dinoData, ArkApi::GetApiUtils().GetWorld(), &vec, &rotation, shooterController->TargetingTeamField(), false, shooterController);
	if (!dino)
		return;

	uint64 id = ArkApi::GetApiUtils().GetSteamIdFromController(shooterController);
	UPrimalCharacterStatusComponent* dinoComponent = dino->GetCharacterStatusComponent();

	if (!dinoComponent || (dinoComponent->BPGetCurrentStatusValue(EPrimalCharacterStatusValue::Health) <= 0))
		return;

	char* levelUps = dinoComponent->NumberOfLevelUpPointsAppliedField()();
	FString name;
	dino->GetDescriptiveName(&name);
	std::stringstream ss;
	ss << std::setw(40) << name.ToString() <<
		std::setw(12) << std::to_string(levelUps[STATS_WILD_HEALTH]) + (levelUps[STATS_DOM_HEALTH] ? " (" + std::to_string(levelUps[STATS_DOM_HEALTH]) + ")" : " (-)") <<
		std::setw(12) << std::to_string(levelUps[STATS_WILD_STAMINA]) + (levelUps[STATS_DOM_STAMINA] ? " (" + std::to_string(levelUps[STATS_DOM_STAMINA]) + ")" : " (-)") <<
		std::setw(12) << std::to_string(levelUps[STATS_WILD_OXYGEN]) + (levelUps[STATS_DOM_OXYGEN] ? " (" + std::to_string(levelUps[STATS_DOM_OXYGEN]) + ")" : " (-)") <<
		std::setw(12) << std::to_string(levelUps[STATS_WILD_FOOD]) + (levelUps[STATS_DOM_FOOD] ? " (" + std::to_string(levelUps[STATS_DOM_FOOD]) + ")" : " (-)") <<
		std::setw(12) << std::to_string(levelUps[STATS_WILD_WEIGHT]) + (levelUps[STATS_DOM_WEIGHT] ? " (" + std::to_string(levelUps[STATS_DOM_WEIGHT]) + ")" : " (-)") <<
		std::setw(12) << std::to_string(levelUps[STATS_WILD_DAMAGE]) + (levelUps[STATS_DOM_DAMAGE] ? " (" + std::to_string(levelUps[STATS_DOM_DAMAGE]) + ")" : " (-)") <<
		std::setw(12) << std::to_string(levelUps[STATS_WILD_SPEED]) + (levelUps[STATS_DOM_SPEED] ? " (" + std::to_string(levelUps[STATS_DOM_SPEED]) + ")" : " (-)") << std::endl;

	//ArkApi::GetApiUtils().SendNotification(shooterController, FColorList::Green, 1.2, 15, nullptr, ss.str().c_str());
	FString out = FString(ss.str());
	shooterController->ClientServerNotificationSingle(&out, FColorList::Green, 0.6f, 15, nullptr, nullptr, msgId);

	dino->Destroy(true, false);

	return;
}

// Set short-term stats display every few frames while Spyglass is equipped
void Hook_AShooterWeapon_Tick(AShooterWeapon* _this, float DeltaSeconds)
{
	AShooterWeapon_Tick_original(_this, DeltaSeconds);

	if ((ArkApi::GetApiUtils().GetWorld()->FrameCounterField() % 6) != 0)
		return;

	if (!spyglassClass)
		spyglassClass = UVictoryCore::BPLoadClass(&spyglassClassPath);

	if (!_this->IsA(spyglassClass))
		return;
	if (_this->CurrentStateField() == EWeaponState::UnEquipping)
		return;

	AActor* owner = _this->OwnerField();
	if (!owner || !owner->IsA(AShooterCharacter::GetPrivateStaticClass()))
		return;

	AShooterCharacter* ownerCharacter = static_cast<AShooterCharacter*>(_this->OwnerField());
	AController* controller = ownerCharacter->ControllerField();
	if (!controller || !controller->IsA(AShooterPlayerController::GetPrivateStaticClass()))
		return;

	AShooterPlayerController* ownerController = static_cast<AShooterPlayerController*>(ownerCharacter->ControllerField());
	uint64 id = ArkApi::GetApiUtils().GetSteamIdFromController(ownerController);
	AActor* currentAimed = nullptr;

	// Trace Channels 2 and 7 together detect all dinos I've checked so far. I haven't investigated channels too heavily.
	std::array< ECollisionChannel,2> channels = { ECollisionChannel::ECC_GameTraceChannel2, ECollisionChannel::ECC_GameTraceChannel7 };
	for (ECollisionChannel channel : channels)
	{
		currentAimed = ownerCharacter->GetAimedActor(channel, nullptr, 0.0, 0.0, nullptr, nullptr, false, false, false);
		if (currentAimed && currentAimed->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
			break;
	}
	if (!currentAimed || !currentAimed->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
		return;

	APrimalDinoCharacter* target = static_cast<APrimalDinoCharacter*>(currentAimed);
	UPrimalCharacterStatusComponent* targetComponent = target->GetCharacterStatusComponent();
	if (!targetComponent || (targetComponent->BPGetCurrentStatusValue(EPrimalCharacterStatusValue::Health) <= 0))
		return;

	// Check if we need to refresh our target
	if ((SpyglassTargets[id].dinoDomLevel != targetComponent->ExtraCharacterLevelField()) || SpyglassTargets[id].dinoTeam != target->TargetingTeamField() ||
		(SpyglassTargets[id].dinoId1 != target->DinoID1Field()) || (SpyglassTargets[id].dinoId2 != target->DinoID2Field()))
		SetSpyglassTargetInfo(ownerController, target, false);

	ShowSpyglassTargetInfo(ownerController, SpyglassTargets[id].isInInventory ? FLT_MAX : 6);
}

// Set long-term stats display when a player views a dino's inventory
void Hook_AShooterPlayerController_ServerActorViewRemoteInventory_Implementation(AShooterPlayerController* _this, UPrimalInventoryComponent* inventoryComp)
{
	AShooterPlayerController_ServerActorViewRemoteInventory_Implementation_original(_this, inventoryComp);

	if (!inventoryComp)
		return;

	uint64 id = ArkApi::GetApiUtils().GetSteamIdFromController(_this);
	AActor* owner = inventoryComp->GetOwner();
	if (!owner || !owner->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
		return;

	SetSpyglassTargetInfo(_this, static_cast<APrimalDinoCharacter*>(owner), false);
	SpyglassTargets[id].isInInventory = true;
	ShowSpyglassTargetInfo(_this, FLT_MAX);
}

// Set long-term stats display when a level-up is applied to a dino
void Hook_UPrimalCharacterStatusComponent_ServerApplyLevelUp(UPrimalCharacterStatusComponent* _this, EPrimalCharacterStatusValue::Type LevelUpValueType, AShooterPlayerController* ByPC)
{
	UPrimalCharacterStatusComponent_ServerApplyLevelUp_original(_this, LevelUpValueType, ByPC);

	if (!ByPC)
		return;

	uint64 id = ArkApi::GetApiUtils().GetSteamIdFromController(ByPC);
	AActor* owner = _this->GetOwner();
	if (!owner || !owner->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
		return;

	SetSpyglassTargetInfo(ByPC, static_cast<APrimalDinoCharacter*>(owner), false);
	ShowSpyglassTargetInfo(ByPC, SpyglassTargets[id].isInInventory ? FLT_MAX : 6);
}

// Hide stats display when a player closes any inventory
void Hook_AShooterCharacter_ServerSetViewingInventory_Implementation(AShooterCharacter* _this, bool bIsViewing)
{
	AShooterCharacter_ServerSetViewingInventory_Implementation_original(_this, bIsViewing);

	if (bIsViewing)
		return;

	AController* controller = _this->ControllerField();
	if (!controller || !controller->IsA(AShooterPlayerController::GetPrivateStaticClass()))
		return;

	AShooterPlayerController* shooterController = static_cast<AShooterPlayerController*>(controller);
	uint64 id = ArkApi::GetApiUtils().GetSteamIdFromController(shooterController);

	ShowSpyglassTargetInfo(shooterController, 0);
	SpyglassTargets[id].isInInventory = false;
}

// Set long-term stats display when a player equips a cryo with a dino inside
void Hook_AShooterCharacter_OnWeaponEquipped(AShooterCharacter* _this, AShooterWeapon* NewWeapon)
{
	AShooterCharacter_OnWeaponEquipped_original(_this, NewWeapon);

	if (!cryoClass)
		cryoClass = UVictoryCore::BPLoadClass(&cryoClassPath);

	if (!NewWeapon || !NewWeapon->IsA(cryoClass))
		return;

	AController* controller = _this->ControllerField();
	if (!controller || !controller->IsA(AShooterPlayerController::GetPrivateStaticClass()))
		return;

	AShooterPlayerController* shooterController = static_cast<AShooterPlayerController*>(controller);
	uint64 id = ArkApi::GetApiUtils().GetSteamIdFromController(shooterController);
	AShooterWeapon* weapon = _this->CurrentWeaponField();
	if (!weapon)
		return;

	UPrimalItem* item = weapon->AssociatedPrimalItemField();
	if (!item)
		return;

	FCustomItemData itemData;
	item->GetCustomItemData(FName("Dino", EFindName::FNAME_Find), &itemData);
	if ((itemData.CustomDataStrings.Num() < 2) || (itemData.CustomDataBytes.ByteArrays.Num() < 1) || (itemData.CustomDataBytes.ByteArrays[0].Bytes.Num() < sizeof(FCustomItemByteArray)))
		return;

	FVector vec{ 0,0,0 };
	FRotator rotation{ 0,0,0 };
	FARKDinoData dinoData;

	dinoData.DinoNameInMap = itemData.CustomDataStrings[0];
	dinoData.DinoName = itemData.CustomDataStrings[1];
	dinoData.DinoClass = itemData.CustomDataClasses[0];
	dinoData.DinoData = itemData.CustomDataBytes.ByteArrays[0].Bytes;

	// WARNING: This will not build against the main Michidu/ARK-Server-API repo. SpawnFromDinoData() is not declared there. SpawnFromDinoDataEX() is declared incorrectly there and will lead to undesired behavior and crashes.
	APrimalDinoCharacter* dino = APrimalDinoCharacter::SpawnFromDinoData(&dinoData, ArkApi::GetApiUtils().GetWorld(), &vec, &rotation, shooterController->TargetingTeamField(), false, shooterController);
	if (!dino)
		return;

	SetSpyglassTargetInfo(shooterController, dino, true);
	ShowSpyglassTargetInfo(shooterController, FLT_MAX);

	dino->Destroy(true, false);
}

// Hide stats display when a player unequips a spyglass or cryo
void Hook_AShooterCharacter_OnWeaponUnequipped(AShooterCharacter* _this, AShooterWeapon* OldWeapon)
{
	AShooterCharacter_OnWeaponUnequipped_original(_this, OldWeapon);

	AController* controller = _this->ControllerField();
	if (!controller || !controller->IsA(AShooterPlayerController::GetPrivateStaticClass()))
		return;

	if (!spyglassClass)
		spyglassClass = UVictoryCore::BPLoadClass(&spyglassClassPath);
	if (!cryoClass)
		cryoClass = UVictoryCore::BPLoadClass(&cryoClassPath);

	if (!OldWeapon || (!OldWeapon->IsA(cryoClass) && !OldWeapon->IsA(spyglassClass)))
		return;

	ShowSpyglassTargetInfo(static_cast<AShooterPlayerController*>(controller), 0);
}

// Set long-term stats display when a player cryos a dino
void Hook_APrimalDinoCharacter_OnCryo(APrimalDinoCharacter* _this, AShooterPlayerController* ForPC)
{
	APrimalDinoCharacter_OnCryo_original(_this, ForPC);

	if (!ForPC)
		return;

	SetSpyglassTargetInfo(ForPC, _this, true);
	ShowSpyglassTargetInfo(ForPC, FLT_MAX);
}

// Hide stats display when a player uncryos a dino
void Hook_APrimalDinoCharacter_OnUncryo(APrimalDinoCharacter* _this, AShooterPlayerController* ForPC)
{
	APrimalDinoCharacter_OnUncryo_original(_this, ForPC);

	if (!ForPC)
		return;

	ShowSpyglassTargetInfo(ForPC, 0);
}

void CryosCmd(AShooterPlayerController* shooterController, FString* input, EChatSendMode::Type)
{
	AShooterCharacter* shooterChar = static_cast<AShooterCharacter*>(shooterController->CharacterField());
	UPrimalInventoryComponent* myInventory = shooterChar->MyInventoryComponentField();
	FCustomItemData itemData;
	FString dinoStats;
	FString cryopodLine = "\n---   Cryopods   ---";
	FString headerLine = "                                       Health    Stamina   Oxygen   Food     Weight   Damage   Speed";
	int i = 0;

	//ShowSpyglassTargetInfo(shooterController, 0);
	shooterController->ClientServerNotificationSingle(&cryopodLine, FColorList::Green, 0.7f, 15, nullptr, nullptr, 0xBABABA);
	shooterController->ClientServerNotificationSingle(&headerLine, FColorList::Green, 0.7f, 15, nullptr, nullptr, 0xBABABB);

	for (UPrimalItem* item : myInventory->InventoryItemsField())
	{
		ShowCryopodStats(item, shooterController, 0xBABABC + ++i);
	}

	TArray<TWeakObjectPtr<UPrimalInventoryComponent>> remoteInventories = shooterController->RemoteViewingInventoriesField();
	for (TWeakObjectPtr<UPrimalInventoryComponent> weakRemoteInventory : remoteInventories)
	{
		UPrimalInventoryComponent* remoteInventory = weakRemoteInventory.Get();
		if (!remoteInventory)
			return;

		for (UPrimalItem* item : remoteInventory->InventoryItemsField())
		{
			ShowCryopodStats(item, shooterController, 0xBABABC + ++i);
		}
	}
}

#define SET_HOOK(_class, _func) ArkApi::GetHooks().SetHook(#_class "." #_func, \
	&Hook_ ## _class ## _ ## _func, \
    & ## _class ## _ ## _func ## _original)
void Load()
{
	Log::Get().Init(PLUGIN_NAME);

	ArkApi::GetCommands().AddChatCommand("/cryos", &CryosCmd);

	SET_HOOK(AShooterWeapon, Tick);
	SET_HOOK(AShooterPlayerController, ServerActorViewRemoteInventory_Implementation);
	SET_HOOK(UPrimalCharacterStatusComponent, ServerApplyLevelUp);
	SET_HOOK(AShooterCharacter, ServerSetViewingInventory_Implementation);
	SET_HOOK(AShooterCharacter, OnWeaponEquipped);
	SET_HOOK(AShooterCharacter, OnWeaponUnequipped);
	SET_HOOK(APrimalDinoCharacter, OnCryo);
	SET_HOOK(APrimalDinoCharacter, OnUncryo);
}

#define DISABLE_HOOK(_class, _func) ArkApi::GetHooks().DisableHook(#_class "." #_func, \
	&Hook_ ## _class ## _ ## _func)
void Unload()
{
	ArkApi::GetCommands().RemoveChatCommand("/cryos");

	DISABLE_HOOK(AShooterWeapon, Tick);
	DISABLE_HOOK(AShooterPlayerController, ServerActorViewRemoteInventory_Implementation);
	DISABLE_HOOK(UPrimalCharacterStatusComponent, ServerApplyLevelUp);
	DISABLE_HOOK(AShooterCharacter, ServerSetViewingInventory_Implementation);
	DISABLE_HOOK(AShooterCharacter, OnWeaponEquipped);
	DISABLE_HOOK(AShooterCharacter, OnWeaponUnequipped);
	DISABLE_HOOK(APrimalDinoCharacter, OnCryo);
	DISABLE_HOOK(APrimalDinoCharacter, OnUncryo);
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