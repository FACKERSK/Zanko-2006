# Zankō 2006 (残光, "Afterglow")

PS1-style survival horror sandbox set in a Japanese coastal town in 2006. Written in C99 with [raylib 5.5](https://www.raylib.com/).

**Download:** [Zanko2006_Setup_v0.6.1.exe](https://github.com/FACKERSK/Zanko-2006/raw/main/releases/Zanko2006_Setup_v0.6.1.exe) (Windows installer, also in the `releases/` folder). It installs for the current Windows account (no administrator rights needed). The launcher updates the game automatically from this repository's releases.

## Layout

| Path | What |
|---|---|
| `kirimachi.c`, `zk_*.h` | the game (`Zanko.exe`) |
| `launcher.c` | the launcher (`Launcher.exe`): local accounts, settings, mods, auto-update |
| `aero.h`, `panel3d.h`, `silent.h`, `modzip.h` | UI toolkit (Silent Aero), 3D menu slabs, backdrop shader, mod zips |
| `zk_design.h` | the fixed town layout, generated from concept map 5b by `tools/export5b.py` |
| `pkg/Zanko2006/` | game data shipped to players (`Data`, `Textures`, `Mods`); the built `.exe` files go into `Root/` |
| `installer/zanko2006.nsi` | NSIS 3 installer script |
| `update/stable.txt`, `update/beta.txt` | update manifests the launcher reads |
| `tools/` | map concepts and export, music synth, Japanese font subset, app icon |
| `web/` | browser build (Emscripten) |

## Build

Needs raylib 5.5 built for each target (`RL` = Linux `src` folder, `RW` = MinGW `src` folder).

```sh
RL=~/raylib-linux/src ./build.sh            # Linux test build: kirimachi_linux, launcher_linux
RW=~/raylib/src ./build.sh win              # Windows: Zanko.exe, Launcher.exe (x86_64-w64-mingw32)
RW=~/raylib/src ./build.sh installer        # Windows build + pkg/Zanko2006_Setup_v<version>.exe (needs makensis)
```

## Releasing an update

1. Raise `GAME_VERSION` in `aero.h` and `pkg/Zanko2006/Data/version.txt`; add the changes to `pkg/Zanko2006/Data/news.txt`.
2. `./build.sh installer`.
3. Copy `pkg/Zanko2006_Setup_v<version>.exe` into `releases/` (and, if you like, attach it to a GitHub release).
4. Point `update/stable.txt` (and `update/beta.txt`) at the new version and installer URL, then push. Players' launchers pick it up on their next start.

On start the launcher reads `update/<channel>.txt`. If its `version` is newer than the installed one, it downloads the installer and runs it silently (`/S /UPDATE`), closes, and the installer reopens the launcher when it is done. Saves, accounts and settings in `Data/` are kept.
