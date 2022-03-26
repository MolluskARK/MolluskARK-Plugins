# ARK-Plugins
Some ARK plugins I've written and found interesting or useful. Anything with a version number less than 1.0 is incomplete and likely buggy and probably shouldn't be loaded on an active server.<br/><br/>
Compiled plugins can be found in **Publish/**

## Plugins
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

### SpyglassStats v0.2
Display dino stats as a notification when players:
- Look at a dino while a Spyglass is equipped
- View a dino's inventory
- Equip a cryopod with a dino inside

Future plans:
- Choice of "spyglass" weapon. Allow admins to specify blueprint for another weapon to display stats on
- Configuration options (allow stats on wild/unallied dinos, etc)
- Try to fix a couple cases where notifications get stuck on
- Maybe some customization for notification text

### StructurePickup v0.1
Very experimental. Allow some structures to always be picked up while others have a cooldown. A little tricky without mods due to client HUD. Currently I'm fiddling with structure net relevancy VERY frequently on APrimalStructure::Unstasis(), which seems like a bad plan. This also hijacks the structure's `CustomData` field, which seems to be unused but I need to verify it.<br/><br/>
Requires `AlwaysAllowStructurePickup=true`. Pickup on existing structures is disabled when the saved game is loaded, so this should be loaded at server start.

### TidyDams v1.0
When a player leaves a beaver dam with only wood, the dam will be automatically destroyed and the wood will drop into an item cache.<br/><br/>
This may not work on servers with modded beaver dams or resource items. Configurable cache decay timer.
