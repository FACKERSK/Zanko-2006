#!/bin/sh
# Web build (Emscripten 3.1.6, WebGL2). Output: zanko.js / zanko.wasm / zanko.data next to index.html
set -e
cd "$(dirname "$0")"
RW=/home/claude/raylib-web/src
emcc -O2 -std=gnu99 -DPLATFORM_WEB -Wno-missing-braces ../kirimachi.c -I$RW -L$RW -lraylib \
  -sUSE_GLFW=3 -sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sASYNCIFY -sALLOW_MEMORY_GROWTH=1 \
  -sINITIAL_MEMORY=134217728 -sTOTAL_STACK=2097152 -sFORCE_FILESYSTEM=1 -sEXPORTED_RUNTIME_METHODS='["callMain"]' \
  --embed-file assets/Data@/Data --embed-file assets/Textures@/Textures --embed-file assets/Mods@/Mods -o zanko.js
