#include <fstream>
#include "API/ARK/Ark.h"
#include "json.hpp"

#pragma comment(lib, "ArkApi.lib")

DECLARE_HOOK(UWorld_InitWorld, void, UWorld*, UWorld::InitializationValues);
DECLARE_HOOK(AShooterGameMode_InitGame, void, AShooterGameMode*, FString*, FString*, FString*);
DECLARE_HOOK(APrimalStructure_LoadedFromSaveGame, void, APrimalStructure*);
DECLARE_HOOK(APrimalStructure_PlacedStructure, void, APrimalStructure*, AShooterPlayerController*);
DECLARE_HOOK(APrimalStructure_Unstasis, void, APrimalStructure*);

#define STRUCTURE_IS_PICKUP_CD(_structure)		(((_structure)->CustomDataField() & 0x00000001) != 0)
#define STRUCTURE_SET_PICKUP_CD(_structure)		((_structure)->CustomDataField() = (_structure)->CustomDataField() | 0x00000001)
#define STRUCTURE_CLEAR_PICKUP_CD(_structure)	((_structure)->CustomDataField() = (_structure)->CustomDataField() & 0xFFFFFFFE)

#define STRUCTURE_IS_NO_PICKUP(_structure)		(((_structure)->CustomDataField() & 0x00000002) != 0)
#define STRUCTURE_SET_NO_PICKUP(_structure)		((_structure)->CustomDataField() = (_structure)->CustomDataField() | 0x00000002)
#define STRUCTURE_CLEAR_NO_PICKUP(_structure)	((_structure)->CustomDataField() = (_structure)->CustomDataField() & 0xFFFFFFFD)

//TODO Get a BP list from config rather than hardcoding
enum _BpIndex
{
	BP_CAMPFIRE = 0,
	BP_COOKING_POT,
	BP_STANDING_TORCH,
	BP_WALL_TORCH,
	BP_LAMP,
	BP_OMNI_LAMP,
	BP_INDUSTRIAL_FORGE,
	BP_STORAGE_BOX,
	BP_WOOD_SIGN,
	BP_MORTAR_PESTLE,
	BP_SINGLE_FLAG,
	BP_MULTI_FLAG,
	BP_WOOD_WALL_SIGN,
	BP_WOOD_BILLBOARD,
	BP_WOOD_CHAIR,
	BP_WOOD_TABLE,
	BP_WOOD_BENCH,
	BP_WALL_TROPHY,
	BP_WOOD_SPIKE_WALL,
	BP_METAL_SPIKE_WALL,
	BP_METAL_SIGN,
	BP_METAL_WALL_SIGN,
	BP_METAL_BILLBOARD,
	BP_WOOD_CAGE,
	BP_VESSEL,
	BP_FEEDING_TROUGH,

	BP_TABLE_SIZE
};

struct _BpTable {
	FString path;
	UClass* ptr;
} bpTable[BP_TABLE_SIZE];


void Hook_UWorld_InitWorld(UWorld* _this, UWorld::InitializationValues IVS)
{
	UWorld_InitWorld_original(_this, IVS);

	bpTable[BP_CAMPFIRE].path = FString("Blueprint'/Game/PrimalEarth/Structures/Campfire.Campfire'");
	bpTable[BP_COOKING_POT].path = FString("Blueprint'/Game/PrimalEarth/Structures/CookingPot.CookingPot'");
	bpTable[BP_STANDING_TORCH].path = FString("Blueprint'/Game/PrimalEarth/Structures/StandingTorch.StandingTorch'");
	bpTable[BP_WALL_TORCH].path = FString("Blueprint'/Game/PrimalEarth/Structures/WallTorch.WallTorch'");
	bpTable[BP_LAMP].path = FString("Blueprint'/Game/PrimalEarth/Structures/Electric/LampPost.LampPost'");
	bpTable[BP_OMNI_LAMP].path = FString("Blueprint'/Game/PrimalEarth/Structures/Electric/LampPostOmni.LampPostOmni'");
	bpTable[BP_INDUSTRIAL_FORGE].path = FString("Blueprint'/Game/PrimalEarth/Structures/IndustrialForge.IndustrialForge'");
	bpTable[BP_STORAGE_BOX].path = FString("Blueprint'/Game/PrimalEarth/Structures/StorageBox_Small.StorageBox_Small'");
	bpTable[BP_WOOD_SIGN].path = FString("Blueprint'/Game/PrimalEarth/Structures/Sign_Small_Wood.Sign_Small_Wood'");
	bpTable[BP_MORTAR_PESTLE].path = FString("Blueprint'/Game/PrimalEarth/Structures/MortarAndPestle.MortarAndPestle'");
	bpTable[BP_SINGLE_FLAG].path = FString("Blueprint'/Game/PrimalEarth/Structures/Flag_SM_Single.Flag_SM_Single'");
	bpTable[BP_MULTI_FLAG].path = FString("Blueprint'/Game/PrimalEarth/Structures/Flag_SM.Flag_SM'");
	bpTable[BP_WOOD_WALL_SIGN].path = FString("Blueprint'/Game/PrimalEarth/Structures/Sign_Wall_Wood.Sign_Wall_Wood'");
	bpTable[BP_WOOD_BILLBOARD].path = FString("Blueprint'/Game/PrimalEarth/Structures/Sign_Large_Wood.Sign_Large_Wood'");
	bpTable[BP_WOOD_CHAIR].path = FString("Blueprint'/Game/PrimalEarth/Structures/Furniture/StructureBP_WoodChair.StructureBP_WoodChair'");
	bpTable[BP_WOOD_TABLE].path = FString("Blueprint'/Game/PrimalEarth/Structures/WoodTable.WoodTable'");
	bpTable[BP_WOOD_BENCH].path = FString("Blueprint'/Game/PrimalEarth/Structures/Furniture/StructureBP_WoodBench.StructureBP_WoodBench'");
	bpTable[BP_WALL_TROPHY].path = FString("Blueprint'/Game/PrimalEarth/Structures/TrophyWallBP.TrophyWallBP'");
	bpTable[BP_WOOD_SPIKE_WALL].path = FString("Blueprint'/Game/PrimalEarth/Structures/Wooden/SpikeWall_Wood.SpikeWall_Wood'");
	bpTable[BP_METAL_SPIKE_WALL].path = FString("Blueprint'/Game/PrimalEarth/Structures/Wooden/SpikeWall.SpikeWall'");
	bpTable[BP_METAL_SIGN].path = FString("Blueprint'/Game/PrimalEarth/Structures/Sign_Small_Metal.Sign_Small_Metal'");
	bpTable[BP_METAL_WALL_SIGN].path = FString("Blueprint'/Game/PrimalEarth/Structures/Sign_Wall_Metal.Sign_Wall_Metal'");
	bpTable[BP_METAL_BILLBOARD].path = FString("Blueprint'/Game/PrimalEarth/Structures/Sign_Large_Metal.Sign_Large_Metal'");
	bpTable[BP_WOOD_CAGE].path = FString("Blueprint'/Game/PrimalEarth/Structures/BuildingBases/Cage_Wood.Cage_Wood'");
	bpTable[BP_VESSEL].path = FString("Blueprint'/Game/ScorchedEarth/Structures/DesertFurnitureSet/Vessel/SM_Vessel_BP.SM_Vessel_BP'");
	bpTable[BP_FEEDING_TROUGH].path = FString("Blueprint'/Game/PrimalEarth/Structures/FeedingTrough.FeedingTrough'");

	for (int i = 0; i < BP_TABLE_SIZE; ++i)
	{
		bpTable[i].ptr = UVictoryCore::BPLoadClass(&bpTable[i].path);
		if (!bpTable[i].ptr)
		{
			FString msg = FString("BPLoadClass(") + bpTable[i].path + FString(") returned null");
			Log::GetLog()->error(msg.ToString());
		}
	}
	Log::GetLog()->info("BPTable loaded");
}

void Hook_APrimalStructure_LoadedFromSaveGame(APrimalStructure* _this)
{
	bool canPickup = false;

	APrimalStructure_LoadedFromSaveGame_original(_this);

	if (_this->TargetingTeamField() < 50000)
		return;

	for (int i = 0; i < BP_TABLE_SIZE; ++i)
	{
		if (!bpTable[i].ptr)
		{
			// Unexpexted missing BP
			Log::GetLog()->error(bpTable[i].path.ToString());
		}
		else if (_this->IsA(bpTable[i].ptr))
		{
			canPickup = true;
			break;
		}
	}

	if (!canPickup)
	{
		FString ownerString = _this->OwnerNameField();
		int index = (int)ownerString.ToString().find(" (Pickup: ");
		if (index != std::string::npos)
		{
			ownerString = ownerString.Left(index);
			_this->NetUpdateTeamAndOwnerName(_this->TargetingTeamField(), &ownerString);
		}

		STRUCTURE_SET_NO_PICKUP(_this);
		_this->DisableStructurePickup();
	}
}

void Hook_APrimalStructure_PlacedStructure(APrimalStructure* _this, AShooterPlayerController* PC)
{
	bool canPickup = false;

	APrimalStructure_PlacedStructure_original(_this, PC);

	if (_this->TargetingTeamField() < 50000)
		return;

	for (int i = 0; i < BP_TABLE_SIZE; ++i)
	{
		if (_this->IsA(bpTable[i].ptr))
		{
			canPickup = true;
			break;
		}
	}

	if (!canPickup)
		STRUCTURE_SET_PICKUP_CD(_this);
}

void Hook_APrimalStructure_Unstasis(APrimalStructure* _this)
{
	APrimalStructure_Unstasis_original(_this);

	if (_this->TargetingTeamField() >= 50000)
	{
		if (STRUCTURE_IS_NO_PICKUP(_this))
		{
			ENetDormancy dormVal = _this->NetDormancyField().GetValue();
			_this->NetDormancyField() = (ENetDormancy)2;
			_this->FlushNetDormancy();
			_this->MultiSetPickupAllowedBeforeNetworkTime(-1.0);
			_this->NetDormancyField() = dormVal;
		}
		else if (STRUCTURE_IS_PICKUP_CD(_this))
		{
			long double difference = (_this->CreationTimeField() + 30) - ArkApi::GetApiUtils().GetWorld()->GetTimeSeconds() + 0.9;
			FString ownerString = _this->OwnerNameField();
			int index = (int)ownerString.ToString().find(" (Pickup: ");

			if ((difference >= 1))
			{
				if (index != std::string::npos)
				{
					ownerString = ownerString.Left(index + 10) + FString(std::to_string((int)difference)) + FString("s)");
				}
				else
				{
					ownerString = ownerString + FString(" (Pickup: ") + FString(std::to_string((int)difference)) + FString("s)");
				}
			}
			else
			{

				if (index != std::string::npos)
					ownerString = ownerString.Left(index);

				STRUCTURE_CLEAR_PICKUP_CD(_this);
				STRUCTURE_SET_NO_PICKUP(_this);
				_this->DisableStructurePickup();
			}

			_this->NetUpdateTeamAndOwnerName(_this->TargetingTeamField(), &ownerString);
		}
	}
}

void Load()
{
	Log::Get().Init(PLUGIN_NAME);

	ArkApi::GetHooks().SetHook("UWorld.InitWorld", &Hook_UWorld_InitWorld,
		&UWorld_InitWorld_original);
	ArkApi::GetHooks().SetHook("APrimalStructure.LoadedFromSaveGame", &Hook_APrimalStructure_LoadedFromSaveGame,
		&APrimalStructure_LoadedFromSaveGame_original);
	ArkApi::GetHooks().SetHook("APrimalStructure.PlacedStructure", &Hook_APrimalStructure_PlacedStructure,
		&APrimalStructure_PlacedStructure_original);
	ArkApi::GetHooks().SetHook("APrimalStructure.Unstasis", &Hook_APrimalStructure_Unstasis,
		&APrimalStructure_Unstasis_original);
}

void Unload()
{
	ArkApi::GetHooks().DisableHook("UWorld.InitWorld", &Hook_UWorld_InitWorld);
	ArkApi::GetHooks().DisableHook("APrimalStructure.LoadedFromSaveGame", &Hook_APrimalStructure_LoadedFromSaveGame);
	ArkApi::GetHooks().DisableHook("APrimalStructure.PlacedStructure", &Hook_APrimalStructure_PlacedStructure);
	ArkApi::GetHooks().DisableHook("APrimalStructure.Unstasis", &Hook_APrimalStructure_Unstasis);
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