#!/bin/sh
# Build Zanko 2006 (game + launcher) for Linux (test) and Windows (release)
set -e
cd "$(dirname "$0")"
RL=${RL:-/home/claude/raylib-linux/src}; RW=${RW:-/home/claude/raylib/src}   # raylib 5.5 src folders (Linux build, MinGW build)
gcc -O2 -std=gnu99 -Wall -Wno-unused-function -Wno-missing-braces -o kirimachi_linux kirimachi.c net.c -I$RL -L$RL -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
gcc -O2 -std=gnu99 -w -DZK_MODMAKER -o modmaker_linux kirimachi.c net.c -I$RL -L$RL -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
gcc -O2 -std=gnu99 -Wall -Wno-unused-function -Wno-missing-braces -o launcher_linux launcher.c -I$RL -L$RL -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
if [ "$1" = "win" ]; then
x86_64-w64-mingw32-windres art/icon.rc -O coff -o art/icon.res
x86_64-w64-mingw32-gcc -O2 -std=gnu99 -Wno-missing-braces -o Zanko.exe kirimachi.c net.c art/icon.res -I$RW -L$RW -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32 -static -mwindows
x86_64-w64-mingw32-gcc -O2 -std=gnu99 -w -DZK_MODMAKER -o ModMaker.exe kirimachi.c net.c art/icon.res -I$RW -L$RW -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32 -static -mwindows
x86_64-w64-mingw32-gcc -O2 -std=gnu99 -Wno-missing-braces -o Launcher.exe launcher.c art/icon.res -I$RW -L$RW -lraylib -lopengl32 -lgdi32 -lwinmm -lurlmon -lshell32 -static -mwindows
fi
if [ "$1" = "installer" ]; then   # Windows binaries into pkg/Zanko2006/Root, then the NSIS installer into pkg/
"$0" win
cp Zanko.exe Launcher.exe ModMaker.exe pkg/Zanko2006/Root/
V=$(cat pkg/Zanko2006/Data/version.txt)
(cd installer && makensis -V2 -DVER="$V" zanko2006.nsi)
(cd pkg && rm -f "Zanko2006_v$V.zip" && zip -qr "Zanko2006_v$V.zip" Zanko2006 -x "Zanko2006/Data/Userdata/*")   # the version selector downloads this
fi
