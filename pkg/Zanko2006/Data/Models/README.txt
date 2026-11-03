ZANKO 2006 - MODELS
===================

Every prop, item, held weapon, vehicle and creature part in the game is an .obj file in this
folder, with a matching .mtl. The game loads these files on start. Edit one (Blender, etc.)
and the change shows up in-game. Delete a file and the game re-creates the default box version.

Folders
  creatures/<type>/   animated parts: bound, longneck, crawler, dog, spirit
                      (leg, arm, torso, head, neck, jaw, tail, limb, body)
  items/              pickups lying in the world
  viewmodels/         what you hold in first person (hand, steel_pipe, fire_axe, katana, revolver ...)
  props/              crate, vending_machine (+ vending_panel = glowing front), campfire, campfire_flame,
                      barricade_plank, plank_wall, shimenawa_ward, crt_tv
  vehicles/<car>/     paint.obj (tinted per car) + details.obj (glass, wheels, underbody)

Rules
  - Units: metres. +Y up. +Z = the direction the object faces (creatures walk toward +Z).
    Blender export: Forward = Z, Up = Y.
  - The origin is the pivot. Creature parts rotate around it (hip, shoulder, neck ...), so keep the
    origin where the default file has it.
  - Materials: Kd = colour multiplier, map_Kd = texture. If map_Kd names a game texture
    (flesh.png, cloth.png, plain.png ...) the game uses its own copy from Textures/ or a mod.
    Any other PNG path is loaded as-is, relative to the .mtl.
  - The game's shader adds PS1 vertex wobble, dithering and fog to whatever you model.
  - Collision is separate from the model: cars, crates and vending machines keep their box
    collision when remodelled.

Not OBJ: buildings, roads and the city layout are generated procedurally for every new game.

Prefer shipping changes as a mod (see Mods/README.txt) so the originals stay intact.
