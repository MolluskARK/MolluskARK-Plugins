#include <map>
#include "API/ARK/Ark.h"
#include "Timer.h"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(AShooterCharacter_OnWeaponEquipped, void, AShooterCharacter*, AShooterWeapon*);

#define STATS_TEXT_SECS		5

#define STATS_WILD_HEALTH	0
#define STATS_WILD_STAMINA	1
#define STATS_WILD_OXYGEN	3
#define STATS_WILD_FOOD		4
#define STATS_WILD_WEIGHT	7
#define STATS_WILD_DAMAGE	8
#define STATS_WILD_SPEED	9
#define STATS_DOM_HEALTH	12
#define STATS_DOM_STAMINA	13
#define STATS_DOM_OXYGEN	15
#define STATS_DOM_FOOD		16
#define STATS_DOM_WEIGHT	19
#define STATS_DOM_DAMAGE	20
#define STATS_DOM_SPEED		21

struct _playerTargets_t {
	APrimalDinoCharacter* target;
	FVector location;
	int timerSecs;
};
std::map<uint64, struct _playerTargets_t> playerTargets;

FString spyglassClassPath = "Blueprint'/Game/PrimalEarth/CoreBlueprints/Weapons/WeapSpyglass.WeapSpyglass'";
UClass* spyglassClass;

FVector zeroVector;

void SpyglassStatsInternal(AShooterPlayerController* shooterController)
{
	if (!shooterController)
		return;

	uint64 id = ArkApi::GetApiUtils().GetSteamIdFromController(shooterController);
	std::string statsString;
	FString dinoName;
	FString notification;
	AShooterCharacter* shooterCharacter = shooterController->LastControlledPlayerCharacterField();
	if (!shooterCharacter)
		return;

	//TODO Additional checks to ensure player is in a state where this is all appropriate

	if (!spyglassClass)
		spyglassClass = UVictoryCore::BPLoadClass(&spyglassClassPath);

	AShooterWeapon* weapon = shooterCharacter->CurrentWeaponField();
	if (weapon && weapon->IsA(spyglassClass))
	{
		// Spyglass is equipped. Check player aim for a target
		AActor* currentAimed = shooterCharacter->GetAimedActor(ECollisionChannel::ECC_GameTraceChannel2, nullptr, 0.0, 0.0, nullptr, nullptr, false, false, false);
		if (currentAimed && currentAimed->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
		{
			APrimalDinoCharacter* currentDino = static_cast<APrimalDinoCharacter*>(currentAimed);
			if ((currentDino != playerTargets[id].target) || playerTargets[id].timerSecs <= 0)
			{
				// New target or time to update text location
				playerTargets[id].target = currentDino;
				currentDino->GetActorCenterTraceLocation(&playerTargets[id].location);
				playerTargets[id].timerSecs = STATS_TEXT_SECS - 1;
			}
			else
			{
				// Same target as before
				--playerTargets[id].timerSecs;
			}
		}
		else
		{
			// Not targeting a dino
			--playerTargets[id].timerSecs;
		}
	}
	else
	{
		// No Spyglass equipped. We're done!
		return;
	}

	if (playerTargets[id].target && (playerTargets[id].timerSecs >= 0) && playerTargets[id].target->GetCharacterStatusComponent() &&
		(playerTargets[id].target->GetCharacterStatusComponent()->BPGetCurrentStatusValue(EPrimalCharacterStatusValue::Health) > 0))
	{
		char* levelUps = playerTargets[id].target->GetCharacterStatusComponent()->NumberOfLevelUpPointsAppliedField()();
		if (playerTargets[id].target->TargetingTeamField() < 50000)
		{
			statsString = "\nHealth: " + std::to_string(levelUps[STATS_WILD_HEALTH]) + "   Stamina: " + std::to_string(levelUps[STATS_WILD_STAMINA]) +
				"\nOxygen: " + std::to_string(levelUps[STATS_WILD_OXYGEN]) + "   Food: " + std::to_string(levelUps[STATS_WILD_FOOD]) +
				"\nWeight: " + std::to_string(levelUps[STATS_WILD_WEIGHT]) + "   Damage: " + std::to_string(levelUps[STATS_WILD_DAMAGE]) +
				"\nSpeed: " + std::to_string(levelUps[STATS_WILD_SPEED]);
		}
		else
		{
			statsString = "\nHP: " + std::to_string(levelUps[STATS_WILD_HEALTH]) + " - " + std::to_string(levelUps[STATS_DOM_HEALTH]) +
				"   Stm: " + std::to_string(levelUps[STATS_WILD_STAMINA]) + " - " + std::to_string(levelUps[STATS_DOM_STAMINA]) +
				"\nOxy: " + std::to_string(levelUps[STATS_WILD_OXYGEN]) + " - " + std::to_string(levelUps[STATS_DOM_OXYGEN]) +
				"   Fd: " + std::to_string(levelUps[STATS_WILD_FOOD]) + " - " + std::to_string(levelUps[STATS_DOM_FOOD]) +
				"\nWt: " + std::to_string(levelUps[STATS_WILD_WEIGHT]) + " - " + std::to_string(levelUps[STATS_DOM_WEIGHT]) +
				"   Dmg: " + std::to_string(levelUps[STATS_WILD_DAMAGE]) + " - " + std::to_string(levelUps[STATS_DOM_DAMAGE]) +
				"\nSpeed: " + std::to_string(levelUps[STATS_WILD_SPEED]) + " - " + std::to_string(levelUps[STATS_DOM_SPEED]);
		}

		playerTargets[id].target->GetDescriptiveName(&dinoName);
		notification = FString("** ") + dinoName + FString(" **") + FString(statsString);
		shooterController->ClientAddFloatingText(playerTargets[id].location, &notification, FColorList::OrangeRed, (float)0.6, (float)0.6, (float)1.1, zeroVector, (float)0.6, 0, (float)0.1);
	}

	// Run again in 1 sec (NOTE: This will cause the server to crash if the DLL is unloaded while a player has a spyglass equipped)
	API::Timer::Get().DelayExecute(&SpyglassStatsInternal, 1, shooterController);
}

void StartSpyglassStats(AShooterPlayerController* shooterController)
{
	uint64 id = ArkApi::GetApiUtils().GetSteamIdFromController(shooterController);
	playerTargets[id].target = nullptr;
	SpyglassStatsInternal(shooterController);
}

void Hook_AShooterCharacter_OnWeaponEquipped(AShooterCharacter* _this, AShooterWeapon* NewWeapon)
{
	AShooterCharacter_OnWeaponEquipped_original(_this, NewWeapon);

	if (!spyglassClass)
		spyglassClass = UVictoryCore::BPLoadClass(&spyglassClassPath);

	if (NewWeapon->IsA(spyglassClass))
		StartSpyglassStats(static_cast<AShooterPlayerController*>(_this->ControllerField()));
}

void Load()
{
	Log::Get().Init(PLUGIN_NAME);

	ArkApi::GetHooks().SetHook("AShooterCharacter.OnWeaponEquipped", &Hook_AShooterCharacter_OnWeaponEquipped,
		&AShooterCharacter_OnWeaponEquipped_original);
}

void Unload()
{
	ArkApi::GetHooks().DisableHook("AShooterCharacter.OnWeaponEquipped", &Hook_AShooterCharacter_OnWeaponEquipped);
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