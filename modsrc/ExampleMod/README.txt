EXAMPLE MOD - Zanko 2006
========================

Install: leave this .zip in the game's Mods folder. The game unpacks it to Mods/.cache/ on start.
Folders also work: Mods/<YourMod>/ with the same layout as this zip.

Layout
  mod.ini                      name, author, version, description (name is shown in the main menu)
  Textures/<name>.png          replaces Textures/<name>.png  (same file name)
  Models/<path>.obj (+ .mtl)   replaces Data/Models/<path>.obj (same relative path)

What this mod changes
  Textures/vending.png         recoloured vending machine front
  Models/items/onigiri.obj     triangular onigiri with a nori band (replaces the box version)

Model rules
  - Units are metres, +Y is up, +Z is the direction the object faces.
  - The origin is the pivot the game rotates/animates around - keep it where the original has it.
  - Materials: Kd = colour, map_Kd = texture. If map_Kd names a game texture (e.g. plain.png) the game
    uses its own (mod-aware) copy of that texture; any other PNG next to the .mtl is loaded as-is.
  - Creatures are split into animated parts (leg, torso, head ...). Replace a part file to reshape it;
    the animation keeps working because the pivot stays the same.

Load order: mods are read in alphabetical order of their file/folder names; the first mod that
provides a file wins. Delete Mods/.cache/ to force re-extraction.
