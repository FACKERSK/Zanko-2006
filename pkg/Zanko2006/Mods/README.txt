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

Load order: set in the launcher's Mods tab (saved to Data/mods.ini). The top mod wins
when two mods provide the same file. New mods start enabled at the bottom of the list.
Mods can be installed by dragging .zip files onto the launcher window.
Loaded mods are listed under the Profile panel in the main menu.

ExampleMod.zip shows both kinds of override (a recoloured vending machine texture and a
remodelled onigiri). Unzip it to see the structure. Untick it in the launcher's Mods tab to disable it.

Legacy: a bare Mods/<name>.png also overrides a texture.
