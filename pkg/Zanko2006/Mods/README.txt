ZANKO 2006 - MODS
=================

Put mods here, either as a .zip file or as a folder:
  Mods/MyMod.zip
  Mods/MyMod/

Zip mods are unpacked to Mods/.cache/ when the game starts (again whenever the zip changes).

Mod layout
  mod.ini                       name=..., author=..., version=..., description=...
  Textures/<name>.png           replaces Textures/<name>.png
  Models/<path>.obj (+ .mtl)    replaces Data/Models/<path>.obj
  cheats.ini                    switches on the in-game cheat menu (see the Cheat Menu mod)
  Data/map.zmap                 replaces the whole map (streets, coast, sea, river, rail, lots, spots)
  Data/objects.zobj             placed blocks and props (the Mod Maker Build tab)

Mod Maker (Root/ModMaker.exe, or "Open Mod Maker" in the launcher's Mods tab)
  Makes and edits folder mods here. Map tab: fly through the town (right mouse look, WASD, Q/E),
  drag the points of any layer, Insert/Delete points, N for a new street, F5 rebuilds the town,
  Ctrl+S saves Data/map.zmap. Textures / Models tabs: pick one and drop a .png / .obj onto the window.
  Build tab: insert blocks and props, Move (1) / Turn (2) / Size (3) with handles, snap, texture,
  colour, solid; Ctrl+D duplicates, Del deletes. Saved as Data/objects.zobj in the mod.
  Mod tab: name, author, version, description; "Export .zip" writes Exports/<name>.zip to share.
  Multiplayer: everyone in a game needs the same map mod (the game refuses a different map).

Load order: set in the launcher's Mods tab (saved to Data/mods.ini). The top mod wins
when two mods provide the same file. New mods start enabled at the bottom of the list.
Mods can be installed by dragging .zip files onto the launcher window.
Loaded mods are listed under the Profile panel in the main menu.

ExampleMod.zip shows both kinds of override (a recoloured vending machine texture and a
remodelled onigiri). Unzip it to see the structure. Untick it in the launcher's Mods tab to disable it.

Legacy: a bare Mods/<name>.png also overrides a texture.
