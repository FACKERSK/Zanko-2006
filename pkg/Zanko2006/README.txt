ZANKO 2006 (Afterglow) - v0.3.0  "Survival"
==========================================
Start:  Root/Launcher.exe   (or Root/Zanko.exe directly, plays as Guest)

Folders
  Root/      Launcher.exe, Zanko.exe
  Data/      config.ini, launcher.ini, mods.ini (load order), news.txt, version.txt,
             UI/kanji.png, Models/ (all .obj models), Userdata/ (accounts + per-player stats)
  Textures/  all textures (.png)
  Mods/      mods (.zip or folders) - see Mods/README.txt

Launcher tabs
  Play        patch notes, survivor record, PLAY
  Mods        enable/disable, load order (top wins conflicts), conflicts, drag-and-drop .zip install
  Servers     placeholder until multiplayer
  Parameters  memory allocation (--ram), V-Sync, FPS limit, windowed, skip menu, no mods,
              log file, render height, world seed, additional free-form parameters
  Options     display, audio, gameplay, update settings (shared with the in-game Settings)

Zanko.exe startup parameters
  --user NAME  --ram MB  --windowed  --fullscreen  --nomods  --newgame  --res WxH
  --render N  --fov N  --seed N  --novsync  --fps N  --log
  --ram reserves a resident working set for the game process (Windows). It is not a heap cap.

Not in this build: multiplayer, online accounts, auto-update source (set update_url in
Data/launcher.ini once the GitHub repository exists).
