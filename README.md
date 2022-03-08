# ARK-Plugins
Some ARK plugins I've written and found interesting or useful.<br/><br/>
Compiled plugins ready for use can be found in **Publish/**
## Plugins
### InventoryAccess
Work in progress. Allows you to specify `Tribe`, `Allies`, or `All` permission levels for:
- Access to living (offline/unconscious) player's inventories (Always disabled on PvE, this setting only affects PvP servers)
- Access to dead player's inventories and death caches
- Access to living (unconscious) tamed dino's inventories (Always disabled on PvE, this setting only affects PvP servers)
- Access to dead tamed dino's inventories and death caches
- Access to item caches intentionally dropped from a player's or dino's inventory
- Activating/deactivating structures (e.g., toggling another tribe's generator on/off)
### PluginTemplate
Simple example plugin. Demonstrates installing a hook, installing a chat command, and reading from a config file.
### SpyglassStats
Basic proof of concept, needs fleshing out. My attempt at the classic spyglass mod. Dino stats notification floats in the world near your target when you have a spyglass equipped. Warning: this will crash the server if unloaded while a player has a spyglass equipped.
### StructurePickup
Very experimental. Allow some structures to always be picked up while others have a cooldown. A little tricky without mods due to client HUD. Currently I'm fiddling with structure net relevancy VERY frequently on APrimalStructure::Unstasis(), which seems like a bad plan. Requires `AlwaysAllowStructurePickup=true`.
### TidyDams
When a player leaves a beaver dam with only wood, the dam will be automatically destroyed and the wood will drop into an item cache. Configurable cache decay timer.
