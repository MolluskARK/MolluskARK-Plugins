# MolluskARK-Plugins
Some ARK plugins I've written and found interesting or useful. Anything with a version number less than 1.0 is incomplete and likely buggy.

Compiled plugins can be found in the **Publish** directory. I'm working on migrating these to the [Releases](https://github.com/MolluskARK/MolluskARK-Plugins/releases) page.

## Plugins

### BlueprintHooks v0.1
Provides Blueprint script function hooking for other plugins. Multiple hooks from multiple plugins can be installed on the same Blueprint functions.

**For developers:** This plugin comes with `BlueprintHooks.h` and `BlueprintHooks.lib` files. The DLL exports functions for setting/disabled pre-hooks and post-hooks. The header also defines templated functions that can be used inside your hook functions to fetch pointers to the original Blueprint function's inputs, outputs, and local variables.
- **bool SetBlueprintPreHook(UFunction\* blueprintFunction, BlueprintHookFuncPtr hookFunction)**
- **bool SetBlueprintPostHook(UFunction\* blueprintFunction, BlueprintHookFuncPtr hookFunction)**
- **bool DisableBlueprintPreHook(UFunction\* blueprintFunction, BlueprintHookFuncPtr hookFunction)**
- **bool DisableBlueprintPostHook(UFunction\* blueprintFunction, BlueprintHookFuncPtr hookFunction)**
- **T\* GetFunctionLocalVar(FFrame\* stack, const FString& name)**
- **T\* GetFunctionInput(FFrame\* stack, const FString& name)**
- **T\* GetFunctionOutput(FFrame\* stack, const FString& name)**

**Note:** The main fork of ArkServerApi ([Michidu/ARK-Server-API](https://github.com/Michidu/ARK-Server-API)) is sufficient for running this plugin and for developers using the library. The MolluskARK fork of ArkServerApi ([MolluskARK/ARK-Server-API:mollusk](https://github.com/MolluskARK/Ark-Server-Plugins/tree/mollusk)) is required to actually build this plugin from source.

### DinoStatsDisplay v0.3
Display dino stats as a notification when players:
- Look at a dino while a Spyglass is equipped
- View a dino's inventory
- Equip a cryopod with a dino inside

`/cryos` chat command displays a list of stats for dinos that are in cryopods in your inventory and in the inventory you are viewing (if one is open)

Future plans:
- Choice of "spyglass" weapon. Allow admins to specify blueprint for another weapon to display stats on
- Configuration options (allow stats on wild/unallied dinos, chat command name, etc)
- Some customization for notification text
- Improve `/cryos` chat command output

### InventoryAccess v0.1
Work in progress. Allows you to specify `Tribe`, `Allies`, or `All` permission levels for:
- Access to living (offline/unconscious) player's inventories
- Access to dead player's inventories and death caches
- Access to living (unconscious) tamed dino's inventories
- Access to dead tamed dino's inventories and death caches
- Access to item caches intentionally dropped from a player's or dino's inventory
- Activating/deactivating structures (e.g., toggling another tribe's generator on/off)

This restricts actions that are allowed in the game, it does not enable new permissions. E.g., on PvE servers you can't access the inventory of an unconscious player outside your tribe, even with `"LivePlayer": "All"`.

### PluginTemplate v1.0
Simple example plugin. Demonstrates installing a hook, installing a chat command, and reading from a config file.

### StructurePickup v0.1
Very experimental. Allow some structures to always be picked up while others have a cooldown. A little tricky without mods due to client HUD. Currently I'm fiddling with structure net relevancy VERY frequently on APrimalStructure::Unstasis(), which seems like a bad plan. This also hijacks the structure's `CustomData` field, which seems to be unused but I need to verify it.

Requires `AlwaysAllowStructurePickup=true`. Pickup on existing structures is disabled when the saved game is loaded, so this should be loaded at server start.

### TidyDams v1.0
When a player leaves a beaver dam with only wood, the dam will be automatically destroyed and the wood will drop into an item cache.<br/><br/>
This may not work on servers with modded beaver dams or resource items. Configurable cache decay timer.

## Plugins (BlueprintHooks Required)
These plugins require the **BlueprintHooks** plugin to also be running on the ARK server for Blueprint script function hooking.

### CryopodBlock v0.2
Block Cryopods from releasing in areas defined by config file. The `CryopodBlock.zones` console command shows blocked areas in-game.

### DinoStealBlock v0.1
Block Pegomastax from stealing cryopods.

Future plans: Provide a list of items to block in config file. Possibly also support blocking Ichthyornis item stealing.

## Plugins (Forked)
Open source plugins forked from other developers.

### Permissions v1.0
Up to date with official **Permissions v1.9** and compatible with plugins that use the official **Permissions** plugin.

MolluskARK changes:
- Timed permissions no longer incorrectly become permanent
- The same group can safely be simultaneously assigned to multiple group categories (player permanent groups, player timed groups, tribe permanent groups, tribe timed groups). This change makes the plugin more flexible for users and prevents various runtime errors.
