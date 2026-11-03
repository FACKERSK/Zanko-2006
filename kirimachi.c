// ZANKŌ 2006 (残光, "Afterglow") — PS1-style survival horror sandbox
// C99 + raylib 5.5
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "aero.h"
#include "modzip.h"
#include "silent.h"
#include "panel3d.h"
#include <time.h>
#include <stdarg.h>

#define NB 12   // street grid of the fixed town (concept map 5b, zk_design.h)
#define BLOCK 34.0f
#define ROADW 10.0f
#define PITCH (BLOCK + ROADW)
#define CITY (NB * PITCH + ROADW)
#define SIDEH 0.15f
#define GH 3.35f
#define MAX_SOLID 240000
#define MAX_OPEN 20000
#define MAX_VEND 500
#define MAX_LIGHT 3000
#define MAX_SIGN 900
#define MAX_LAMP 900
#define MAX_ITEM 2600
#define MAX_CR 110
#define MAX_PART 420
#define MAX_BUILD 220
#define MAX_CRATE 140
#define MAX_ASH 220
#define GCELL 8.0f
#define GN 112   // broadphase grid: 8 m cells from -170 m, covering the whole map

enum { T_ASPHALT, T_SIDEWALK, T_CONCRETE, T_FACADE, T_FACADE2, T_TILEWALL, T_BLOCKWALL, T_SHUTTER, T_ROOF, T_GLASS,
       T_PLASTER, T_TATAMI, T_WOOD, T_FLESH, T_CLOTH, T_METAL, T_VEND, T_SIGN, T_SIGNH, T_PLAIN, T_RUST, T_ECO, T_HEDGE, T_PAPER, T_GRASS, T_SKIN, T_COUNT };
static const char *texNames[T_COUNT] = {"asphalt.png", "sidewalk.png", "concrete.png", "facade_a.png", "facade_b.png", "tilewall.png",
    "blockwall.png", "shutter.png", "roof.png", "glass.png", "plaster.png", "tatami.png", "wood.png", "flesh.png", "cloth.png",
    "metal.png", "vending.png", "sign_v.png", "sign_h.png", "plain.png", "rust.png", "ecoglass.png", "hedge.png", "ofuda.png", "grass.png", "skin.png"};
static float texSU[T_COUNT] = {4, 1.5f, 3, 3, 3, 1.5f, 0.8f, 2, 4, 1.5f, 2, 0.9f, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 3, 1};
static float texSV[T_COUNT] = {4, 1.5f, 3, 3.2f, 3.2f, 1.5f, 0.4f, 2, 4, 1.5f, 2, 1.8f, 2, 1, 1, 1, 1, 1, 1, 1, 1, 3.2f, 2, 1, 3};

#include "zk_items.h"
#include "zk_jp.h"

enum { ST_MENU, ST_PLAY, ST_PAUSE, ST_SETTINGS, ST_CONTROLS, ST_DEAD, ST_WIN, ST_LORE, ST_TRAITS };

// ============================================================ utils
static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static unsigned char cb(float v) { return (unsigned char)clampf(v, 0, 255); }
static Color rgb(float r, float g, float b) { return (Color){cb(r), cb(g), cb(b), 255}; }
static unsigned int rs = 1;
static unsigned int rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static float fr(float a, float b) { return a + (b - a) * ((rnd() & 0xFFFFFF) / 16777215.0f); }
static int ri(int n) { return (int)(rnd() % (unsigned)n); }
static float hash2(int x, int y, int s) {
    unsigned int h = (unsigned int)x * 374761393u + (unsigned int)y * 668265263u + (unsigned int)s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u; h ^= h >> 16;
    return (float)(h & 0xFFFFFF) / 16777215.0f;
}
static float vnoise(float x, float y, int s) {
    int xi = (int)floorf(x), yi = (int)floorf(y); float fx = x - xi, fy = y - yi;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    float a = hash2(xi, yi, s), b = hash2(xi + 1, yi, s), c = hash2(xi, yi + 1, s), d = hash2(xi + 1, yi + 1, s);
    return Lerp(Lerp(a, b, fx), Lerp(c, d, fx), fy);
}
static float wrapA(float a) { while (a > PI) a -= 2 * PI; while (a < -PI) a += 2 * PI; return a; }

// ============================================================ textures
static Texture2D tex[T_COUNT];
static Image genImage(int k) {
    int W = 32, H = 32;
    if (k == T_FACADE || k == T_FACADE2) { W = 64; H = 64; }
    if (k == T_VEND) { W = 32; H = 64; }
    if (k == T_SIGN) { W = 32; H = 96; }
    if (k == T_SIGNH) { W = 96; H = 32; }
    if (k == T_BLOCKWALL) { W = 32; H = 16; }
    if (k == T_TATAMI) { W = 32; H = 64; }
    if (k == T_ECO) { W = 64; H = 64; }
    if (k == T_PAPER) { W = 16; H = 48; }
    Image img = GenImageColor(W, H, BLACK);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        float n = hash2(x, y, k * 131 + 3), m = vnoise(x * 0.2f, y * 0.2f, k * 7 + 1), m2 = vnoise(x * 0.07f, y * 0.07f, k * 5 + 11);
        Color c = WHITE;
        switch (k) {
        case T_ASPHALT: { float v = 36 + 18 * m + 14 * n; if (n > 0.97f) v += 35; if (fabsf(vnoise(x * 0.15f, y * 0.15f, 99) - 0.5f) < 0.02f) v *= 0.5f; c = rgb(v, v, v * 1.05f); } break;
        case T_SIDEWALK: { bool g = (x % 16 == 0) || (y % 16 == 0); float v = (g ? 70 : 118) + 25 * m + 10 * n; c = rgb(v, v * 0.98f, v * 0.95f); } break;
        case T_CONCRETE: { float v = 105 + 30 * m + 12 * n - 35 * m2 * (y / 32.0f); c = rgb(v, v, v * 0.97f); } break;
        case T_FACADE: case T_FACADE2: {
            float base = (k == T_FACADE) ? 150 : 112; float v = base + 20 * m + 8 * n;
            c = (k == T_FACADE) ? rgb(v, v * 0.93f, v * 0.82f) : rgb(v * 0.95f, v, v * 1.02f);
            bool win = x >= 12 && x < 52 && y >= 10 && y < 40;
            bool frame = x >= 10 && x < 54 && y >= 8 && y < 42 && !win;
            if (frame) c = rgb(58, 60, 64);
            if (win) { float g = 18 + 10 * n; if (x == 31 || x == 32) g = 55; if (y < 14) g += 10; c = rgb(g, g + 4, g + 12); }
            if (k == T_FACADE2 && y >= 44 && y < 54) c = (y == 44 || y == 53 || x % 6 == 0) ? rgb(150, 150, 150) : rgb(70, 72, 75);
            if (!win && !frame && y > 41 && x >= 12 && x < 52 && vnoise(x * 0.6f, 0, k) > 0.6f) c = rgb(v * 0.6f, v * 0.6f, v * 0.55f);
        } break;
        case T_TILEWALL: { bool g = (x % 8 == 0) || (y % 8 == 0); float v = (g ? 90 : 172) + 15 * n - 30 * m2; c = rgb(v * 0.95f, v * 0.97f, v); } break;
        case T_BLOCKWALL: { int row = y / 8; int off = (row % 2) * 8; bool g = (y % 8 == 0) || ((x + off) % 16 == 0); float v = (g ? 70 : 122) + 25 * m + 10 * n; if (m2 > 0.6f) v *= 0.75f; c = rgb(v, v, v * 0.95f); } break;
        case T_SHUTTER: { float v = (y % 4 == 0) ? 60 : (y % 4 == 1 ? 150 : 115); v += 10 * n; c = rgb(v, v * 1.02f, v * 1.05f); if (vnoise(x * 0.3f, y * 0.05f, 4) > 0.68f) c = rgb(110, 60, 35); } break;
        case T_ROOF: { float v = 68 + 20 * m + 8 * n; if (m2 > 0.62f) v = 35; c = rgb(v, v, v); } break;
        case T_GLASS: { float v = 25 + 15 * m; float hl = ((x + y) % 24 < 3) ? 40 : 0; c = rgb(v + hl, v + 10 + hl, v + 25 + hl); } break;
        case T_PLASTER: { float v = 170 + 20 * m + 8 * n - 40 * m2; c = rgb(v, v * 0.96f, v * 0.88f); } break;
        case T_TATAMI: { bool border = (x < 3 || x >= 29); float v = (y % 2 == 0) ? 150 : 130; v += 10 * n; c = border ? rgb(25, 45, 30) : rgb(v * 0.85f, v * 0.9f, v * 0.5f); } break;
        case T_WOOD: { int pl = x / 8; bool g = (x % 8 == 0); float v = (g ? 40 : 90) + 25 * vnoise(pl * 3.0f, y * 0.1f, 2) + 10 * n; c = rgb(v * 1.1f, v * 0.8f, v * 0.55f); } break;
        case T_FLESH: { float v = 0.7f + 0.3f * m; c = rgb(165 * v, 135 * v, 125 * v); if (m2 > 0.6f) c = rgb(110 * v, 40 * v, 40 * v); if (n > 0.95f) c = rgb(40, 15, 15); } break;
        case T_CLOTH: { float v = 0.65f + 0.25f * m + 0.1f * n; c = rgb(175 * v, 165 * v, 140 * v); if (y % 12 < 2) c = rgb(90 * v, 80 * v, 65 * v); if (m2 > 0.65f) c = rgb(95, 35, 30); } break;
        case T_METAL: { float v = 190 + 15 * m + 6 * n; if (m2 > 0.7f) v -= 60; c = rgb(v, v, v); } break;
        case T_VEND: {
            c = rgb(225, 228, 232);
            if (y >= 4 && y < 40) {
                int row = (y - 4) / 12, col = x / 5, ly = (y - 4) % 12;
                if (x >= 1 && x < 31 && ly >= 2 && ly < 10 && (x % 5) != 0) {
                    Color cs[5] = {{200, 40, 40, 255}, {40, 120, 210, 255}, {240, 200, 40, 255}, {40, 170, 90, 255}, {235, 120, 30, 255}};
                    c = cs[(int)(hash2(col, row, 77) * 5) % 5];
                    if (ly < 4) c = rgb(240, 240, 240);
                } else if (ly >= 10) c = rgb(150, 150, 160);
            }
            if (y >= 44 && y < 50) c = rgb(60, 60, 70);
            if (y >= 52 && y < 60 && x >= 6 && x < 26) c = rgb(20, 20, 25);
        } break;
        case T_SIGN: case T_SIGNH: { float v = 80 + 15 * n; c = rgb(v, v, v); if (x < 2 || y < 2 || x >= W - 2 || y >= H - 2) c = rgb(210, 210, 210); } break;
        case T_RUST: { float v = 0.6f + 0.4f * m; c = (m2 > 0.5f) ? rgb(120 * v, 60 * v, 30 * v) : rgb(90 * v, 80 * v, 70 * v); } break;
        case T_ECO: {   // aero curtain-wall glass: teal panes, white mullions, sky reflection gradient, grime
            bool mull = (x % 32 < 2) || (y % 32 < 2) || (y % 32 == 16);
            float refl = 1.0f - (y % 32) / 40.0f;
            c = mull ? rgb(200 + 20 * n, 215 + 20 * n, 215 + 20 * n) : rgb(40 + 60 * refl + 15 * m, 120 + 70 * refl + 15 * m, 130 + 80 * refl + 15 * m);
            if (!mull && ((x + y) % 40 < 3)) c = rgb(180, 240, 240);
            if (!mull && m2 > 0.62f) c = rgb(30 + 20 * n, 50 + 20 * n, 45 + 20 * n);   // soot / cracks
            if (!mull && hash2(x / 32, y / 32, 51) > 0.8f) c = rgb(15, 20, 22);       // shattered pane
        } break;
        case T_HEDGE: { float v = 0.45f + 0.35f * m + 0.2f * n; c = rgb(55 * v, 110 * v, 60 * v); if (m2 > 0.65f) c = rgb(90 * v, 80 * v, 50 * v); } break;
        case T_PAPER: { float v = 225 + 20 * n; c = rgb(v, v * 0.96f, v * 0.85f); if (x < 1 || x > 14) c = rgb(200, 40, 30); } break;
        case T_SKIN: { float v = 0.9f + 0.07f * m + 0.03f * n; c = rgb(235 * v, 205 * v, 185 * v); if (y % 9 == 0 && m2 > 0.5f) c = rgb(215 * v, 180 * v, 162 * v); } break;   // clean skin for the hands (flesh.png is the creatures' torn skin)
        case T_GRASS: {   // Aero hillside grass: saturated green with lighter blades
            float v = 0.75f + 0.2f * m + 0.1f * n; c = rgb(80 * v, 175 * v, 70 * v);
            if (hash2(x, y / 3, 61) > 0.82f) c = rgb(150 * v, 225 * v, 110 * v);
            if (hash2(x / 2, y, 62) > 0.93f) c = rgb(45, 120, 50);
        } break;
        default: { float v = 222 + 20 * n; c = rgb(v, v, v); } break;
        }
        ImageDrawPixel(&img, x, y, c);
    }
    if (k == T_PAPER) {   // ofuda brush strokes
        for (int g = 0; g < 4; g++) { ImageDrawRectangle(&img, 5 + ri(3), 4 + g * 10, 2, 7 + ri(3), (Color){20, 20, 20, 255}); ImageDrawRectangle(&img, 3 + ri(4), 6 + g * 10, 6 + ri(3), 2, (Color){20, 20, 20, 255}); }
        ImageDrawRectangle(&img, 4, 38, 8, 6, (Color){200, 30, 30, 255});
    }
    if (k == T_SIGN || k == T_SIGNH) {   // pseudo-kanji glyph strokes
        int cells = 4;
        for (int g = 0; g < cells; g++) {
            int ox = (k == T_SIGN) ? 5 : 5 + g * 22, oy = (k == T_SIGN) ? 5 + g * 22 : 5, S = 20;
            for (int s = 0; s < 3 + ri(3); s++) {
                int t = ri(4);
                if (t == 0) ImageDrawRectangle(&img, ox + ri(6), oy + 2 + ri(S - 4), 8 + ri(12), 2, WHITE);
                else if (t == 1) ImageDrawRectangle(&img, ox + 2 + ri(S - 4), oy + ri(6), 2, 8 + ri(12), WHITE);
                else if (t == 2) { int bx = ox + 2 + ri(6), by = oy + 2 + ri(6), bw = 6 + ri(8), bh = 6 + ri(8);
                    ImageDrawRectangleLines(&img, (Rectangle){(float)bx, (float)by, (float)bw, (float)bh}, 2, WHITE); }
                else { for (int d = 0; d < 8; d++) ImageDrawRectangle(&img, ox + 4 + d, oy + 4 + d + ri(2), 2, 2, WHITE); }
            }
        }
    }
    return img;
}
// ============================================================ mods (Mods/*.zip and Mods/<folder>/)
#define MAX_MODS 32
static char modDir[MAX_MODS][600], modName[MAX_MODS][64]; static int nMods;
static bool noMods = false;
static unsigned int gSeedArg = 0;
static void loadMods(void) {   // enabled mods from Data/mods.ini (Mod Manager), top = highest priority
    ensureDir("Mods"); ensureDir("Mods/.cache");
    nMods = 0;
    if (noMods) { TraceLog(LOG_INFO, "MODS: disabled by --nomods"); return; }
    static ModEntry ml[MZ_MAX];
    char modsAbs[700], iniAbs[700]; snprintf(modsAbs, sizeof modsAbs, "%s", P("Mods")); snprintf(iniAbs, sizeof iniAbs, "%s", P("Data/mods.ini"));
    int n = modsScan(modsAbs, iniAbs, ml, MZ_MAX, false);
    for (int i = 0; i < n && nMods < MAX_MODS; i++) {
        ModEntry *m = &ml[i];
        if (!m->enabled || !m->valid) continue;
        if (m->isZip) {
            char out[700]; snprintf(out, sizeof out, "%s/%s", P("Mods/.cache"), GetFileNameWithoutExt(m->file));
            char stamp[760]; snprintf(stamp, sizeof stamp, "%s/.zipstamp", out);
            long mt = GetFileModTime(m->path); bool fresh = false;
            char *st = LoadFileText(stamp); if (st) { fresh = atol(st) == mt; UnloadFileText(st); }
            if (!fresh) {
                MakeDirectory(out);
                if (mzUnzipTo(m->path, out) < 0) { TraceLog(LOG_WARNING, "MODS: could not read %s", m->path); continue; }
                char b[32]; snprintf(b, sizeof b, "%ld", mt); SaveFileText(stamp, b);
            }
            // a zip may wrap everything in one top folder: use it when mod.ini sits there
            char wrap[800]; snprintf(wrap, sizeof wrap, "%s/%s/mod.ini", out, GetFileNameWithoutExt(m->file));
            char top[760]; snprintf(top, sizeof top, "%s/mod.ini", out);
            if (!FileExists(top) && FileExists(wrap)) snprintf(modDir[nMods], 600, "%s/%s", out, GetFileNameWithoutExt(m->file));
            else snprintf(modDir[nMods], 600, "%s", out);
        } else snprintf(modDir[nMods], 600, "%s", m->path);
        snprintf(modName[nMods], 64, "%s", m->name);
        TraceLog(LOG_INFO, "MODS: loaded '%s' (priority %d) from %s", modName[nMods], nMods + 1, modDir[nMods]);
        nMods++;
    }
}
static bool modFind(const char *rel, char *out, int outSize) {   // first mod providing <rel> wins
    for (int i = 0; i < nMods; i++) { snprintf(out, outSize, "%s/%s", modDir[i], rel); if (FileExists(out)) return true; }
    return false;
}
static void loadTextures(void) {
    ensureDir("Textures"); ensureDir("Mods");
    unsigned int keep = rs; rs = 777;
    for (int i = 0; i < T_COUNT; i++) {
        char rel[128], mp[800];
        snprintf(rel, sizeof rel, "Textures/%s", texNames[i]);
        if (modFind(rel, mp, sizeof mp)) tex[i] = LoadTexture(mp);
        else if (snprintf(rel, sizeof rel, "Mods/%s", texNames[i]), FileExists(P(rel))) { tex[i] = LoadTexture(P(rel)); }
        else {
            snprintf(rel, sizeof rel, "Textures/%s", texNames[i]);
            if (FileExists(P(rel))) tex[i] = LoadTexture(P(rel));
            else { Image im = genImage(i); ExportImage(im, P(rel)); tex[i] = LoadTextureFromImage(im); UnloadImage(im); }
        }
        SetTextureFilter(tex[i], TEXTURE_FILTER_POINT);
        SetTextureWrap(tex[i], TEXTURE_WRAP_REPEAT);
    }
    rs = keep;
}

// ============================================================ shader
static const char *VS =
GLSL_HEAD
"in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal; in vec4 vertexColor;\n"
"uniform mat4 mvp; uniform mat4 matModel; uniform mat4 matNormal; uniform vec2 snapRes;\n"
"out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragPos; out vec3 fragNormal;\n"
"void main(){ fragTexCoord=vertexTexCoord; fragColor=vertexColor;\n"
" fragPos=vec3(matModel*vec4(vertexPosition,1.0));\n"
" fragNormal=normalize(vec3(matNormal*vec4(vertexNormal,0.0)));\n"
" vec4 p=mvp*vec4(vertexPosition,1.0);\n"
" if(p.w>0.05){ vec2 s=snapRes*0.5; p.xy=floor(p.xy/p.w*s+0.5)/s*p.w; }\n"
" gl_Position=p; }\n";
static const char *FS =
GLSL_HEAD
"in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragPos; in vec3 fragNormal;\n"
"uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
"uniform vec3 viewPos; uniform vec3 flashDir; uniform float flashOn; uniform float muzzle;\n"
"uniform vec3 fogColor; uniform float fogDensity; uniform float emissive; uniform float ambient; uniform float desat; uniform float rage;\n"
"uniform vec3 lightPos[8]; uniform vec3 lightCol[8]; uniform float lightRange[8]; uniform int numLights;\n"
"out vec4 finalColor;\n"
"const float bay[16]=float[16](0.,8.,2.,10.,12.,4.,14.,6.,3.,11.,1.,9.,15.,7.,13.,5.);\n"
"void main(){\n"
" vec4 tx=texture(texture0,fragTexCoord)*fragColor*colDiffuse;\n"
" if(tx.a<0.5) discard;\n"
" vec3 n=normalize(fragNormal);\n"
" vec3 toF=fragPos-viewPos; float dist=length(toF); vec3 dir=toF/max(dist,0.0001);\n"
" vec3 col;\n"
" if(emissive>0.5){ col=tx.rgb; } else {\n"
"  vec3 light=vec3(0.52,0.56,0.66)*ambient*(0.7+0.3*max(n.y,0.0));\n"
"  float cone=smoothstep(0.88,0.975,dot(dir,flashDir));\n"
"  light+=vec3(1.0,0.95,0.85)*flashOn*cone/(1.0+0.012*dist*dist)*max(dot(n,-dir),0.2)*2.4;\n"
"  light+=vec3(1.0,0.75,0.4)*muzzle*2.5/(1.0+0.08*dist*dist);\n"
"  for(int i=0;i<8;i++){ if(i>=numLights) break; vec3 L=lightPos[i]-fragPos; float d=length(L);\n"
"   float a=clamp(1.0-d/lightRange[i],0.0,1.0); a*=a; light+=lightCol[i]*a*(0.3+0.7*max(dot(n,L/max(d,0.001)),0.0)); }\n"
"  col=tx.rgb*light; }\n"
" if(emissive<1.5){ float f=1.0-exp(-pow(dist*fogDensity,2.0)); col=mix(col,fogColor,clamp(f,0.0,1.0)); }\n"
" ivec2 q=ivec2(mod(gl_FragCoord.xy,4.0)); col+=(bay[q.y*4+q.x]/16.0-0.5)/24.0;\n"
" float gy=dot(col,vec3(0.299,0.587,0.114)); col=mix(col,vec3(gy),desat); col=mix(col,vec3(gy*1.3,gy*0.25,gy*0.2),rage*0.6);\n"
" col=floor(clamp(col,0.0,1.0)*31.0+0.5)/31.0;\n"
" finalColor=vec4(col,1.0); }\n";
static Shader sh;
static int locView, locFlashDir, locFlashOn, locMuzzle, locFog, locFogD, locEmis, locAmb, locLP, locLC, locLR, locNL, locSnap, locDesat, locRage;
static float curEmis = -1;
static void setEmis(float e) { if (e != curEmis) { SetShaderValue(sh, locEmis, &e, SHADER_UNIFORM_FLOAT); curEmis = e; } }

// ============================================================ static mesh builder
typedef struct { float *v, *t, *n; unsigned char *c; int cnt, cap; } MB;
static MB mb[T_COUNT];
static Model stat[T_COUNT]; static bool statOk[T_COUNT];
static void mbPush(MB *b, Vector3 p, Vector2 uv, Vector3 n, Color c) {
    if (b->cnt >= b->cap) {
        b->cap = b->cap ? b->cap * 2 : 1024;
        b->v = realloc(b->v, b->cap * 12); b->t = realloc(b->t, b->cap * 8);
        b->n = realloc(b->n, b->cap * 12); b->c = realloc(b->c, b->cap * 4);
    }
    int i = b->cnt++;
    b->v[i * 3] = p.x; b->v[i * 3 + 1] = p.y; b->v[i * 3 + 2] = p.z;
    b->t[i * 2] = uv.x; b->t[i * 2 + 1] = uv.y;
    b->n[i * 3] = n.x; b->n[i * 3 + 1] = n.y; b->n[i * 3 + 2] = n.z;
    b->c[i * 4] = c.r; b->c[i * 4 + 1] = c.g; b->c[i * 4 + 2] = c.b; b->c[i * 4 + 3] = 255;
}
static Vector2 uvOf(int t, Vector3 p, Vector3 n) {
    float su = texSU[t], sv = texSV[t];
    if (fabsf(n.y) > 0.5f) return (Vector2){p.x / su, p.z / sv};
    if (fabsf(n.x) > 0.5f) return (Vector2){p.z / su, -p.y / sv};
    return (Vector2){p.x / su, -p.y / sv};
}
// ---- building frames: a building is generated in its own straight (local) coordinates, then turned to face its street.
// Frame 0 is the world. world = C + RotY(phi)(local - C) with RotY(phi): x' = x cos + z sin, z' = -x sin + z cos (raylib MatrixRotateY).
#define MAX_XF 1400
typedef struct { float cx, cz, c, s; } Xf;
static Xf xfs[MAX_XF] = {{0, 0, 1, 0}}; static int nXf = 1, curXf = 0;
static int newXf(float cx, float cz, float phi) { if (fabsf(phi) < 1e-4f || nXf >= MAX_XF) return 0; xfs[nXf] = (Xf){cx, cz, cosf(phi), sinf(phi)}; return nXf++; }
static Vector3 xfP(int f, Vector3 p) { if (!f) return p; Xf *x = &xfs[f]; float dx = p.x - x->cx, dz = p.z - x->cz; return (Vector3){x->cx + dx * x->c + dz * x->s, p.y, x->cz - dx * x->s + dz * x->c}; }
static Vector3 xfI(int f, Vector3 p) { if (!f) return p; Xf *x = &xfs[f]; float dx = p.x - x->cx, dz = p.z - x->cz; return (Vector3){x->cx + dx * x->c - dz * x->s, p.y, x->cz + dx * x->s + dz * x->c}; }
static Vector3 xfD(int f, Vector3 d) { if (!f) return d; Xf *x = &xfs[f]; return (Vector3){d.x * x->c + d.z * x->s, d.y, -d.x * x->s + d.z * x->c}; }
static Vector3 xfDI(int f, Vector3 d) { if (!f) return d; Xf *x = &xfs[f]; return (Vector3){d.x * x->c - d.z * x->s, d.y, d.x * x->s + d.z * x->c}; }
static float xfYaw(int f) { return f ? atan2f(xfs[f].s, xfs[f].c) : 0; }
static Matrix xfM(int f) { if (!f) return MatrixIdentity(); Xf *x = &xfs[f]; return MatrixMultiply(MatrixMultiply(MatrixTranslate(-x->cx, 0, -x->cz), MatrixRotateY(atan2f(x->s, x->c))), MatrixTranslate(x->cx, 0, x->cz)); }
static void xfPush(int f) { rlDrawRenderBatchActive(); rlPushMatrix(); if (f) rlMultMatrixf(MatrixToFloat(xfM(f))); }
static void xfPop(void) { rlDrawRenderBatchActive(); rlPopMatrix(); }
static void xfBox(int f, Vector3 mn, Vector3 mx, Vector3 *wmn, Vector3 *wmx) {   // world AABB of a framed box
    if (!f) { *wmn = mn; *wmx = mx; return; }
    *wmn = (Vector3){1e9f, mn.y, 1e9f}; *wmx = (Vector3){-1e9f, mx.y, -1e9f};
    for (int k = 0; k < 4; k++) { Vector3 q = xfP(f, (Vector3){k & 1 ? mx.x : mn.x, 0, k & 2 ? mx.z : mn.z});
        wmn->x = fminf(wmn->x, q.x); wmn->z = fminf(wmn->z, q.z); wmx->x = fmaxf(wmx->x, q.x); wmx->z = fmaxf(wmx->z, q.z); }
}
static void quad(int t, Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 n, Color col) {
    Vector3 q[6] = {a, b, c, a, c, d};
    if (curXf) { Vector3 wn = xfD(curXf, n); for (int i = 0; i < 6; i++) mbPush(&mb[t], xfP(curXf, q[i]), uvOf(t, q[i], n), wn, col); return; }   // texture follows the walls
    for (int i = 0; i < 6; i++) mbPush(&mb[t], q[i], uvOf(t, q[i], n), n, col);
}
static void meshBox(int t, Vector3 mn, Vector3 mx, Color col, int mask) {
    float x0 = mn.x, y0 = mn.y, z0 = mn.z, x1 = mx.x, y1 = mx.y, z1 = mx.z;
    if (mask & 1) quad(t, (Vector3){x1, y0, z1}, (Vector3){x1, y0, z0}, (Vector3){x1, y1, z0}, (Vector3){x1, y1, z1}, (Vector3){1, 0, 0}, col);
    if (mask & 2) quad(t, (Vector3){x0, y0, z0}, (Vector3){x0, y0, z1}, (Vector3){x0, y1, z1}, (Vector3){x0, y1, z0}, (Vector3){-1, 0, 0}, col);
    if (mask & 4) quad(t, (Vector3){x0, y0, z1}, (Vector3){x1, y0, z1}, (Vector3){x1, y1, z1}, (Vector3){x0, y1, z1}, (Vector3){0, 0, 1}, col);
    if (mask & 8) quad(t, (Vector3){x1, y0, z0}, (Vector3){x0, y0, z0}, (Vector3){x0, y1, z0}, (Vector3){x1, y1, z0}, (Vector3){0, 0, -1}, col);
    if (mask & 16) quad(t, (Vector3){x0, y1, z1}, (Vector3){x1, y1, z1}, (Vector3){x1, y1, z0}, (Vector3){x0, y1, z0}, (Vector3){0, 1, 0}, col);
    if (mask & 32) quad(t, (Vector3){x0, y0, z0}, (Vector3){x1, y0, z0}, (Vector3){x1, y0, z1}, (Vector3){x0, y0, z1}, (Vector3){0, -1, 0}, col);
}
#define ALL 31
#define ALLB 63

// ============================================================ world data
typedef struct { Vector3 mn, mx; bool off; short f; } Solid;   // mn/mx in the frame f (0 = world)
static Solid sol[MAX_SOLID]; static int nSol;
static int *cellL[GN * GN]; static int cellN[GN * GN], cellCap[GN * GN];
static int qb[16384], qn; static unsigned int qs[MAX_SOLID], qstamp;
static int cellOf(float v) { int c = (int)floorf((v + 170) / GCELL); return c < 0 ? 0 : (c >= GN ? GN - 1 : c); }
static int addSolidF(Vector3 mn, Vector3 mx, int f) {
    if (nSol >= MAX_SOLID) return -1;
    int id = nSol++;
    sol[id] = (Solid){mn, mx, false, (short)f};
    Vector3 wmn, wmx; xfBox(f, mn, mx, &wmn, &wmx);
    for (int cx = cellOf(wmn.x); cx <= cellOf(wmx.x); cx++) for (int cz = cellOf(wmn.z); cz <= cellOf(wmx.z); cz++) {
        int c = cz * GN + cx;
        if (cellN[c] >= cellCap[c]) { cellCap[c] = cellCap[c] ? cellCap[c] * 2 : 32; cellL[c] = realloc(cellL[c], cellCap[c] * sizeof(int)); }
        cellL[c][cellN[c]++] = id;
    }
    return id;
}
static int addSolid(Vector3 mn, Vector3 mx) { return addSolidF(mn, mx, curXf); }
static Vector3 solL(const Solid *s, Vector3 p) { return s->f ? xfI(s->f, p) : p; }   // a world point in the solid's own frame
static void query(float x0, float z0, float x1, float z1) {
    qstamp++; qn = 0;
    for (int cx = cellOf(x0); cx <= cellOf(x1); cx++) for (int cz = cellOf(z0); cz <= cellOf(z1); cz++) {
        int c = cz * GN + cx;
        for (int k = 0; k < cellN[c]; k++) {
            int id = cellL[c][k];
            if (qs[id] != qstamp) { qs[id] = qstamp; if (!sol[id].off && qn < 16384) qb[qn++] = id; }
        }
    }
}
static int box(int t, Vector3 mn, Vector3 mx, Color c, bool solid, int mask) {
    meshBox(t, mn, mx, c, mask);
    return solid ? addSolid(mn, mx) : -1;
}
#define V3(a, b, c) ((Vector3){(a), (b), (c)})

enum { OP_WINDOW, OP_DOOR };
typedef struct { Vector3 mn, mx; int type, axis, sid, bsid, state; float hp, side, fy; short f; } Open;   // mn/mx in frame f
typedef struct { Vector3 mn, mx; int sid; float hp; bool alive; int seed; short f; } Debris;   // junk piles (chairs, tables) blocking stairs and doorways
#define MAX_DEB 1400
static Debris debs[MAX_DEB]; static int nDeb;
typedef struct { Vector3 bot, top; } Stair;   // walkable stair runs (creature navigation between floors)
#define MAX_STAIR 2400
static Stair stairs[MAX_STAIR]; static int nStair;   // window: 0 intact 1 broken 2 barricaded; door: 0 open 1 locked 2 barricaded
static Open ops[MAX_OPEN]; static int nOps;
typedef struct { Vector3 mn, mx; int face, sid, light; float hp; bool broken; Color body; short f; } Vend;
static Vend vends[MAX_VEND]; static int nVend;
typedef struct { Vector3 pos, col; float range, cur, seed; int mode; } Light;   // mode 0 steady 1 flicker 2 off
static Light lights[MAX_LIGHT]; static int nLight;
typedef struct { Vector3 mn, mx; Color tint; int light, tex, text, face; bool plate; short f; } Sign;   // text: index into the sign atlas; face 0 -z 1 +x 2 +z 3 -x
static Sign signs[MAX_SIGN]; static int nSign;
typedef struct { Vector3 pos; int light; } Lamp;
static Lamp lamps[MAX_LAMP]; static int nLamp;
typedef struct { Vector3 pos; float s, hp; int sid; bool alive; } Crate;
static Crate crates[MAX_CRATE]; static int nCrate;
typedef struct { Vector3 mn, mx; int sid, kind; float hp, maxhp; bool alive, open; int axis, cont; } Build;
static Build builds[MAX_BUILD]; static int nBuild;
static int baseShrine = -1;   // the player's kamidana: wards only work within BASE_R of it
typedef struct { Vector3 pos; int type, amount; bool on; } Item;
static Item items[MAX_ITEM]; static int nItem;
typedef struct { Vector3 pos; float yaw, hp, maxhp, spd, t, atkCd, wind, hurt, deadT, stuckT, lastD, groanT, twT, sideT; float tw[6]; int type, state; Vector3 tgt; bool on; float revealT; bool phantom;
    Vector3 kb, lastKnown; float stun, losT, seeT, scream, losCheck, lunge, flinch; int var; bool hanging, seen; float calm, chaseT, dashT; } Cr;
enum { CS_WANDER, CS_CHASE, CS_DEAD };
static Cr crs[MAX_CR]; static int nCr;
typedef struct { Vector3 pos, vel; float life, size, floor; int t; Color tint; } Part;
#define MAX_SPLAT 120
typedef struct { Vector3 p; float r, life; } Splat;
static Splat splats[MAX_SPLAT]; static int splatI;
static Part parts[MAX_PART];
static Vector3 ash[MAX_ASH];
static Vector3 phonePos, stationCenter;
#define MAX_WARD 64
#define MAX_SCREEN 180
typedef struct { Vector3 pos; float r, life; int kind; bool on; } Ward;   // kind 0 salt circle, 1 shimenawa, 2 shrine
static Ward wards[MAX_WARD]; static int nWard;
#define MAX_FIRE 24
#define MAX_TV 700
typedef struct { Vector3 pos; float life; int light; bool on; } Fire;
static Fire fires[MAX_FIRE]; static int nFire;
typedef struct { Vector3 pos; float ang; } TV;
static TV tvs[MAX_TV]; static int nTV;
typedef struct { Vector3 c; float w, h, ang; int light; bool kiosk; } Screen;
static Screen screens[MAX_SCREEN]; static int nScreen;
static void addWard(Vector3 p, float r, float life, int kind) {
    int slot = -1;
    for (int i = 0; i < nWard; i++) if (!wards[i].on) { slot = i; break; }
    if (slot < 0) { if (nWard >= MAX_WARD) return; slot = nWard++; }
    wards[slot] = (Ward){p, r, life, kind, true};
}

typedef struct {
    Vector3 pos; float vy; bool ground;
    float yaw, pitch, tyaw, tpitch;
    float hp, stam, food, water, batt; bool flash, exhausted;
    Slot hands, back, sl[6 + 18]; int quick[6]; int weapon; int mag;   // weapon: 0 melee/fists, 1 revolver (derived from hands)
    int wounds; float bleedAcc, painT, regenT; float sk[6];               // bleeding wounds, skill XP
    bool invOpen; int openC; float searchT; int searchC; int dragFrom; Slot drag;
    float swingDur, swingHitAt, hitStopT, punch, roll, hitFlash; int swingKind;
    float swingT, swingCd; bool hitDone; float reloadT, shootCd, kick, muzzle;
    float climbT, climbDur; Vector3 climbFrom, climbTo; int climbKind;
    float bob, stepAcc, hurtT, shake;
    bool build; int buildKind;
    int kills; float time;
    float sanity, warmth, rest, boredom, peaceT, lowT, corrupt, involT, watchT; int stage, watchKind; bool sleeping, turned, roof, nearFire;
    float tod; int day;                                   // time of day in hours (0-24), day counter
    float sick, infect, woundT, vomitT, fracT; int fracture;   // food poisoning 0-100, wound infection 0-100, fracture 0 none 1 broken 2 splinted
    int trS, trW; bool mapOpen; float saveT;              // chosen strength / weakness trait
    bool blocking; float blockT, blockK, landK, eyeK, swayX, swayY, sprintK, readFlip;   // guard, animation springs
} Player;
static Player pl;
static Camera3D cam;
static int state = ST_MENU, settingsBack = ST_MENU;
static bool gameActive = false;
static float evacT = -1; static bool evacOn = false;
static char msg[128]; static float msgT;
static char prompt[96];
static unsigned int worldSeed;

// ============================================================ audio
#define SR 22050
static Sound sJingle;
static Sound sImpact, sSplat, sBone, sStinger, sBreath, sScream;
static Sound sStep, sSwing, sHit, sClang, sGlass, sShot, sClick, sHurt, sScreech, sGroan[2], sStatic, sDrone, sHeart, sPick, sWood, sPlace, sPhone, sReload;
static Sound groanPool[8];
static bool audioOk = false;
static Sound mkS(float *b, int n) {
    short *d = MemAlloc(n * 2);
    for (int i = 0; i < n; i++) d[i] = (short)(clampf(b[i], -1, 1) * 30000);
    Wave w = {(unsigned int)n, SR, 16, 1, d};
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}
static float nz(void) { return fr(-1, 1); }
static void genSounds(void) {
    if (!IsAudioDeviceReady()) return;
    int N = SR * 6; float *b = MemAlloc(N * sizeof(float)); float lp, lp2; int n;
#define T ((float)i / SR)
    n = SR / 8; lp = 0; for (int i = 0; i < n; i++) { lp += (nz() - lp) * 0.08f; b[i] = lp * 3.0f * expf(-T * 40); } sStep = mkS(b, n);
    n = SR / 4; lp = 0; for (int i = 0; i < n; i++) { float f = 0.05f + 0.25f * sinf(T / 0.25f * PI); lp += (nz() - lp) * f; b[i] = lp * 0.9f * sinf(T / 0.25f * PI); } sSwing = mkS(b, n);
    n = SR / 4; lp = 0; for (int i = 0; i < n; i++) { lp += (nz() - lp) * 0.1f; b[i] = (lp * 2.0f + sinf(T * 2 * PI * 70) * 0.8f) * expf(-T * 18); } sHit = mkS(b, n);
    n = SR / 2; for (int i = 0; i < n; i++) b[i] = (sinf(T * 2 * PI * 820) * 0.4f + sinf(T * 2 * PI * 1370) * 0.3f + sinf(T * 2 * PI * 2210) * 0.2f + nz() * 0.3f * expf(-T * 60)) * expf(-T * 9); sClang = mkS(b, n);
    n = SR * 7 / 10; lp = 0; for (int i = 0; i < n; i++) { float hp = nz(); lp += (hp - lp) * 0.3f; float tink = 0; for (int k = 0; k < 6; k++) tink += sinf(T * 2 * PI * (2500 + k * 731)) * expf(-(T - k * 0.07f) * 25) * (T > k * 0.07f); b[i] = (hp - lp) * 1.4f * expf(-T * 10) + tink * 0.12f; } sGlass = mkS(b, n);
    n = SR * 7 / 10; lp = 0; lp2 = 0; for (int i = 0; i < n; i++) { lp += (nz() - lp) * 0.35f; lp2 += (lp - lp2) * 0.05f; b[i] = (lp * 1.8f * expf(-T * 22) + lp2 * 4 * expf(-T * 6) + sinf(T * 2 * PI * 60) * expf(-T * 12)); } sShot = mkS(b, n);
    n = SR / 20; for (int i = 0; i < n; i++) b[i] = nz() * expf(-T * 120) * 0.8f; sClick = mkS(b, n);
    n = SR * 4 / 10; lp = 0; for (int i = 0; i < n; i++) { lp += (nz() - lp) * 0.15f; b[i] = (lp * 1.5f + sinf(T * 2 * PI * (220 - T * 200)) * 0.6f) * expf(-T * 8); } sHurt = mkS(b, n);
    n = SR * 9 / 10; lp = 0; for (int i = 0; i < n; i++) { float f = 900 + 500 * sinf(T * 23) + 300 * nz(); float s = sinf(T * 2 * PI * f * (1 + 0.3f * sinf(T * 51))); lp += (nz() - lp) * 0.4f; b[i] = (s * 0.5f + lp * 0.6f) * sinf(fminf(T / 0.9f, 1) * PI) * 0.9f; } sScreech = mkS(b, n);
    for (int g = 0; g < 2; g++) { n = SR * 3 / 2; lp = 0; float ph = 0; for (int i = 0; i < n; i++) { float f = (g ? 55 : 75) + 18 * sinf(T * (g ? 4.3f : 6.1f)) + 8 * nz(); ph += f / SR; float saw = 2 * (ph - floorf(ph)) - 1; lp += (saw + nz() * 0.6f - lp) * 0.06f; b[i] = lp * 2.2f * sinf(T / 1.5f * PI) * (0.7f + 0.3f * sinf(T * 31)); } sGroan[g] = mkS(b, n); }
    n = SR * 2; lp = 0; for (int i = 0; i < n; i++) { float x = nz(); lp += (x - lp) * 0.5f; float crackle = (hash2(i / 40, 0, 3) > 0.92f) ? nz() : 0; b[i] = ((x - lp) * 0.6f + crackle * 0.5f) * (0.8f + 0.2f * sinf(T * 2 * PI * 50)); } sStatic = mkS(b, n);
    n = SR * 6; lp = 0; for (int i = 0; i < n; i++) { lp += (nz() - lp) * 0.01f; float hum = sinf(T * 2 * PI * 41) * 0.25f + sinf(T * 2 * PI * 41.7f) * 0.2f + sinf(T * 2 * PI * 82.3f) * 0.08f; float env = fminf(fminf(T, 6 - T) / 0.4f, 1); b[i] = (hum * (0.6f + 0.4f * sinf(T * 1.05f)) + lp * 3) * env * 0.7f; } sDrone = mkS(b, n);
    n = SR * 7 / 10; for (int i = 0; i < n; i++) { float a = expf(-T * 30) * sinf(T * 2 * PI * 48); float t2 = T - 0.2f; float c = t2 > 0 ? expf(-t2 * 30) * sinf(t2 * 2 * PI * 44) : 0; b[i] = (a + c * 0.8f) * 1.2f; } sHeart = mkS(b, n);
    n = SR / 7; for (int i = 0; i < n; i++) { float f = T < 0.06f ? 880 : 1320; b[i] = (sinf(T * 2 * PI * f) > 0 ? 0.25f : -0.25f) * expf(-T * 12); } sPick = mkS(b, n);
    n = SR * 4 / 10; lp = 0; for (int i = 0; i < n; i++) { lp += (nz() - lp) * 0.25f; float crack = (hash2(i / 200, 1, 9) > 0.7f) ? nz() : 0; b[i] = (lp * 1.5f + crack * 0.7f + sinf(T * 2 * PI * 140) * 0.5f) * expf(-T * 12); } sWood = mkS(b, n);
    n = SR * 6 / 10; for (int i = 0; i < n; i++) { float tt = fmodf(T, 0.2f); b[i] = (sinf(tt * 2 * PI * 320) * 0.6f + nz() * 0.4f) * expf(-tt * 40); } sPlace = mkS(b, n);
    n = SR * 2; for (int i = 0; i < n; i++) { float on = fmodf(T, 1.0f) < 0.6f && fmodf(T * 20, 1) < 0.5f; b[i] = on * (sinf(T * 2 * PI * 1300) * 0.3f + sinf(T * 2 * PI * 1700) * 0.3f); } sPhone = mkS(b, n);
    n = SR * 8 / 10; for (int i = 0; i < n; i++) { float tt = T < 0.4f ? T : T - 0.4f; b[i] = nz() * expf(-tt * 70) * 0.7f + sinf(tt * 2 * PI * 1900) * expf(-tt * 50) * 0.3f; } sReload = mkS(b, n);
    n = SR / 14; for (int i = 0; i < n; i++) b[i] = sinf(T * 2 * PI * (1200 + T * 6000)) * 0.25f * expf(-T * 30); aBlip = mkS(b, n); aBlipOk = true;
    // impact: low body thud with a dropping pitch, a crunch, and a short noise burst
    n = SR * 45 / 100; lp = 0; { float ph = 0; for (int i = 0; i < n; i++) { float f = 95 * expf(-T * 9) + 38; ph += f / SR; lp += (nz() - lp) * 0.18f;
        float crunch = (hash2(i / 90, 7, 3) > 0.55f ? nz() : 0) * expf(-T * 22);
        float sub = sinf(T * 2 * PI * (48 + 30 * expf(-T * 14))) * expf(-T * 5) * 1.3f;
        b[i] = (sinf(ph * 2 * PI) * 1.2f * expf(-T * 7) + lp * 2.6f * expf(-T * 24) + crunch * 0.9f + sub); } } sImpact = mkS(b, n);
    n = SR * 35 / 100; lp = 0; for (int i = 0; i < n; i++) { lp += (nz() - lp) * 0.35f; float am = 0.5f + 0.5f * sinf(T * 2 * PI * (60 + 140 * T)); b[i] = lp * 1.6f * am * expf(-T * 11); } sSplat = mkS(b, n);
    n = SR * 4 / 10; lp = 0; for (int i = 0; i < n; i++) { float snap = T < 0.012f ? nz() * 1.2f : 0; lp += (nz() - lp) * 0.12f; b[i] = snap + sinf(T * 2 * PI * 70) * expf(-T * 14) * 0.9f + lp * 1.4f * expf(-T * 18) + (hash2(i / 60, 9, 1) > 0.7f ? nz() * 0.5f * expf(-T * 10) : 0); } sBone = mkS(b, n);
    n = SR * 18 / 10; { float p1 = 0, p2 = 0, p3 = 0; lp = 0; for (int i = 0; i < n; i++) { float rise = 1 + T * 0.35f; p1 += 233 * rise / SR; p2 += 247 * rise / SR; p3 += 349 * rise / SR;
        float saw = (2 * (p1 - floorf(p1)) - 1) + (2 * (p2 - floorf(p2)) - 1) * 0.8f + (2 * (p3 - floorf(p3)) - 1) * 0.6f; lp += (saw * 0.4f + nz() * 0.5f - lp) * 0.2f;
        b[i] = lp * fminf(T * 8, 1) * expf(-fmaxf(T - 0.9f, 0) * 3) * 1.3f; } } sStinger = mkS(b, n);
    n = SR * 2; lp = 0; for (int i = 0; i < n; i++) { float cyc = fmodf(T, 1.0f); float env = cyc < 0.45f ? sinf(cyc / 0.45f * PI) : sinf((cyc - 0.45f) / 0.55f * PI) * 0.7f; lp += (nz() - lp) * 0.06f; b[i] = lp * 3.0f * env; } sBreath = mkS(b, n);
    n = SR * 16 / 10; { float ph = 0; lp = 0; for (int i = 0; i < n; i++) { float f = 520 + 300 * sinf(T * 37) + 180 * nz(); ph += f / SR; float sq = sinf(ph * 2 * PI) > 0 ? 1 : -1; lp += (sq * 0.6f + nz() * 0.6f - lp) * 0.45f; b[i] = lp * sinf(fminf(T / 1.6f, 1) * PI) * 0.95f; } } sScream = mkS(b, n);
    {   // public-broadcast jingle: cheerful arpeggio on a worn tape (wow/flutter + bitcrush)
        float notes[8] = {523.25f, 659.25f, 783.99f, 1046.5f, 987.77f, 783.99f, 659.25f, 783.99f};
        n = SR * 3; float ph = 0, ph2 = 0;
        for (int i = 0; i < n; i++) {
            int k = (int)(T / 0.3f); if (k > 7) k = 7;
            float tn = T - k * 0.3f, wob = 1 + 0.012f * sinf(T * 2 * PI * 0.8f) + 0.004f * sinf(T * 2 * PI * 7);
            ph += notes[k] * wob / SR; ph2 += notes[k] * 0.5f * wob / SR;
            float tone = sinf(ph * 2 * PI) * 0.5f + (sinf(ph2 * 2 * PI) > 0 ? 0.12f : -0.12f) + sinf(ph * 4 * PI) * 0.1f;
            float env = expf(-tn * 3.5f) * (k == 7 ? expf(-tn * 1.5f) : 1);
            float v = tone * env * 0.6f + nz() * 0.02f;
            b[i] = floorf(v * 24) / 24;
        }
        sJingle = mkS(b, n);
    }
#undef T
    MemFree(b);
    for (int i = 0; i < 8; i++) groanPool[i] = LoadSoundAlias(sGroan[i & 1]);
    SetSoundVolume(sStatic, 0); SetSoundVolume(sDrone, 0.35f);
    audioOk = true;
}
static void play(Sound s, float vol) { if (!audioOk) return; SetSoundVolume(s, vol); SetSoundPitch(s, fr(0.92f, 1.08f)); PlaySound(s); }
static void playAt(Sound s, Vector3 p, float range, float vol) {
    float d = Vector3Distance(p, pl.pos);
    if (d > range) return;
    play(s, vol * powf(1 - d / range, 1.4f));
}

// ============================================================ generation helpers
static int addLight(Vector3 p, Vector3 col, float range, int mode) {
    if (nLight >= MAX_LIGHT) return -1;
    lights[nLight] = (Light){xfP(curXf, p), col, range, 1, fr(0, 100), mode};
    return nLight++;
}
#include "zk_cont.h"
static int addLight(Vector3 p, Vector3 col, float range, int mode);
static void addScreen(Vector3 c, float w, float h, float ang, bool kiosk) {
    if (nScreen >= MAX_SCREEN) return;
    Vector3 out = V3(sinf(ang), 0, cosf(ang));
    int li = fr(0, 1) < 0.15f ? -1 : addLight(Vector3Add(c, Vector3Scale(out, 1.5f)), V3(0.35f, 0.85f, 0.9f), kiosk ? 5 : 11, ri(3) == 0 ? 1 : 0);
    screens[nScreen++] = (Screen){xfP(curXf, c), w, h, ang + xfYaw(curXf), li, kiosk};
}
static int randomLoot(int kind) {   // kind 0 house 1 apartment 2 shop 3 konbini
    float r = fr(0, 1);
    if (kind == 3) return r < 0.32f ? IT_FOOD : r < 0.64f ? IT_DRINK : r < 0.72f ? IT_BATTERY : r < 0.78f ? IT_SALT : r < 0.84f ? IT_BANDAGE : r < 0.92f ? IT_RAMEN : IT_MANGA;
    if (kind == 2) return r < 0.3f ? IT_FOOD : r < 0.55f ? IT_DRINK : r < 0.72f ? IT_BATTERY : r < 0.92f ? IT_PLANKS : IT_SALT;
    if (kind == 7) return r < 0.45f ? IT_OFUDA : r < 0.75f ? IT_SALT : r < 0.9f ? IT_ROPE : IT_FOOD;
    return r < 0.20f ? IT_FOOD : r < 0.36f ? IT_DRINK : r < 0.48f ? IT_BANDAGE : r < 0.57f ? IT_BATTERY : r < 0.70f ? IT_PLANKS : r < 0.76f ? IT_OFUDA : r < 0.80f ? IT_SALT : r < 0.88f ? IT_MANGA : r < 0.93f ? IT_VHS : IT_RAMEN;
}
static int lootAmount(int t) { return t == IT_PLANKS ? 2 + ri(3) : t == IT_AMMO ? 2 + ri(2) : t == IT_OFUDA ? 1 + ri(2) : 1; }
static bool pointFree(float x, float z, float r, float yTop) {   // (x, z) in the current building frame while generating
    Vector3 W = xfP(curXf, (Vector3){x, 0, z});
    query(W.x - r * 1.5f, W.z - r * 1.5f, W.x + r * 1.5f, W.z + r * 1.5f);
    for (int k = 0; k < qn; k++) {
        Solid *s = &sol[qb[k]];
        if (s->mx.y <= yTop) continue;
        if (s->mn.y >= yTop + 1.8f) continue;   // ceilings / overhangs don't block floor space
        Vector3 L = solL(s, W);
        if (L.x + r > s->mn.x && L.x - r < s->mx.x && L.z + r > s->mn.z && L.z - r < s->mx.z) return false;
    }
    return true;
}

// wall piece on a side of rectangle r
static Rectangle curB; static float curT = 0.3f; static float curFY = SIDEH;
static Vector3 sideMn(int s, float u0, float u1, float y0, float y1, Vector3 *mx, float thinC) {
    Rectangle r = curB; float t = curT; Vector3 mn;
    float a0, a1;
    if (thinC > 0) {   // thin centered piece (glass / door panel)
        if (s == 0) { a0 = r.y + t / 2 - thinC; a1 = r.y + t / 2 + thinC; }
        else if (s == 2) { a0 = r.y + r.height - t / 2 - thinC; a1 = r.y + r.height - t / 2 + thinC; }
        else if (s == 1) { a0 = r.x + r.width - t / 2 - thinC; a1 = r.x + r.width - t / 2 + thinC; }
        else { a0 = r.x + t / 2 - thinC; a1 = r.x + t / 2 + thinC; }
    } else {
        if (s == 0) { a0 = r.y; a1 = r.y + t; }
        else if (s == 2) { a0 = r.y + r.height - t; a1 = r.y + r.height; }
        else if (s == 1) { a0 = r.x + r.width - t; a1 = r.x + r.width; }
        else { a0 = r.x; a1 = r.x + t; }
    }
    if (s == 0 || s == 2) { mn = V3(u0, y0, a0); *mx = V3(u1, y1, a1); }
    else { mn = V3(a0, y0, u0); *mx = V3(a1, y1, u1); }
    return mn;
}
static void wallPiece(int s, float u0, float u1, float y0, float y1, int t, Color c) {
    if (u1 - u0 < 0.01f || y1 - y0 < 0.01f) return;
    Vector3 mx, mn = sideMn(s, u0, u1, y0, y1, &mx, 0);
    box(t, mn, mx, c, true, ALL);
}
static void addOpening(int s, float u0, float u1, float y0, float y1, int type, int stt) {
    if (nOps >= MAX_OPEN) return;
    Open *o = &ops[nOps];
    Vector3 mx, mn;
    if (type == OP_WINDOW) mn = sideMn(s, u0, u1, y0, y1, &mx, 0.04f);
    else mn = sideMn(s, u0, u1, y0, y1, &mx, 0);
    o->mn = mn; o->mx = mx; o->type = type; o->axis = (s == 0 || s == 2) ? 1 : 0;
    o->side = (s == 0 || s == 3) ? -1.0f : 1.0f;
    o->state = stt; o->bsid = -1; o->hp = 0; o->sid = -1; o->fy = curFY; o->f = (short)curXf;
    if (type == OP_WINDOW) { o->sid = addSolid(mn, mx); }
    else if (stt == 1) {
        Vector3 dmx, dmn = sideMn(s, u0, u1, y0, y1, &dmx, 0.06f);
        o->mn = dmn; o->mx = dmx; o->sid = addSolid(dmn, dmx); o->hp = 4;
    }
    nOps++;
}

// kinds: 0 house 1 apartment 2 shop 3 konbini 4 koban 5 station 6 eco-tower
static void interiorLoot(Rectangle r, int kind, int count) {
    for (int i = 0; i < count; i++) {
        for (int tries = 0; tries < 12; tries++) {
            float x = fr(r.x + 0.9f, r.x + r.width - 0.9f), z = fr(r.y + 0.9f, r.y + r.height - 0.9f);
            if (!pointFree(x, z, 0.2f, SIDEH + 0.05f)) continue;
            int t = randomLoot(kind);
            addItem(V3(x, SIDEH, z), t, lootAmount(t));
            break;
        }
    }
}
#include "zk_design.h"
#include "zk_world.h"
static void shrine(Rectangle lot) {   // roadside hokora with torii and shimenawa; permanent spirit ward
    float cx = lot.x + lot.width / 2, cz = lot.y + lot.height / 2;
    Color red = rgb(190, 50, 35);
    box(T_PLAIN, V3(cx - 1.6f, SIDEH, cz + 1.5f), V3(cx - 1.35f, SIDEH + 3.0f, cz + 1.75f), red, true, ALL);
    box(T_PLAIN, V3(cx + 1.35f, SIDEH, cz + 1.5f), V3(cx + 1.6f, SIDEH + 3.0f, cz + 1.75f), red, true, ALL);
    box(T_PLAIN, V3(cx - 2.1f, SIDEH + 3.0f, cz + 1.45f), V3(cx + 2.1f, SIDEH + 3.25f, cz + 1.8f), rgb(40, 30, 30), false, ALLB);
    box(T_PLAIN, V3(cx - 1.8f, SIDEH + 2.5f, cz + 1.55f), V3(cx + 1.8f, SIDEH + 2.68f, cz + 1.7f), red, false, ALLB);
    box(T_CLOTH, V3(cx - 1.35f, SIDEH + 2.25f, cz + 1.56f), V3(cx + 1.35f, SIDEH + 2.42f, cz + 1.69f), rgb(220, 200, 140), false, ALLB);
    for (int k = -1; k <= 1; k++) box(T_PAPER, V3(cx + k * 0.6f - 0.08f, SIDEH + 1.9f, cz + 1.6f), V3(cx + k * 0.6f + 0.08f, SIDEH + 2.25f, cz + 1.64f), WHITE, false, ALLB);
    box(T_WOOD, V3(cx - 0.7f, SIDEH, cz - 1.3f), V3(cx + 0.7f, SIDEH + 1.4f, cz - 0.2f), rgb(160, 120, 90), true, ALL);
    box(T_ROOF, V3(cx - 0.95f, SIDEH + 1.4f, cz - 1.5f), V3(cx + 0.95f, SIDEH + 1.6f, cz), rgb(120, 110, 100), true, ALL);
    box(T_CONCRETE, V3(cx - 2.2f, SIDEH, cz - 0.9f), V3(cx - 1.8f, SIDEH + 1.2f, cz - 0.5f), WHITE, true, ALL);
    box(T_CONCRETE, V3(cx + 1.8f, SIDEH, cz - 0.9f), V3(cx + 2.2f, SIDEH + 1.2f, cz - 0.5f), WHITE, true, ALL);
    addLight(V3(cx, SIDEH + 1.0f, cz), V3(1.0f, 0.55f, 0.25f), 7, 1);
    addItem(V3(cx - 0.3f, SIDEH, cz), IT_OFUDA, 2);
    if (fr(0, 1) < 0.7f) addItem(V3(cx + 0.4f, SIDEH, cz + 0.3f), IT_SALT, 1);
    if (fr(0, 1) < 0.6f) addItem(V3(cx, SIDEH, cz + 0.8f), IT_ROPE, 1);
    addWard(V3(cx, SIDEH, cz), 5.0f, 0, 2);
}
static void kiosk(float x, float z, float ang) {
    if (!pointFree(x, z, 0.6f, SIDEH + 0.05f)) return;
    box(T_PLAIN, V3(x - 0.35f, SIDEH, z - 0.35f), V3(x + 0.35f, SIDEH + 1.9f, z + 0.35f), rgb(225, 238, 240), true, ALL);
    box(T_GLASS, V3(x - 0.42f, SIDEH + 1.9f, z - 0.42f), V3(x + 0.42f, SIDEH + 2.05f, z + 0.42f), rgb(120, 220, 210), false, ALLB);
    Vector3 out = V3(sinf(ang), 0, cosf(ang));
    addScreen(Vector3Add(V3(x, SIDEH + 1.35f, z), Vector3Scale(out, 0.37f)), 0.55f, 0.42f, ang, true);
}
static void blockWall(Vector3 a, Vector3 b, float h) {   // axis-aligned segment on ground with random gaps
    bool ax = fabsf(b.x - a.x) > fabsf(b.z - a.z);
    float len = ax ? b.x - a.x : b.z - a.z;
    float u = 0;
    while (u < len) {
        float seg = fminf(fr(3, 8), len - u);
        if (fr(0, 1) < 0.8f) {
            if (ax) box(T_BLOCKWALL, V3(a.x + u, SIDEH, a.z - 0.1f), V3(a.x + u + seg, SIDEH + h, a.z + 0.1f), WHITE, true, ALL);
            else box(T_BLOCKWALL, V3(a.x - 0.1f, SIDEH, a.z + u), V3(a.x + 0.1f, SIDEH + h, a.z + u + seg), WHITE, true, ALL);
        }
        u += seg + fr(1.2f, 2.5f);
    }
}
static void addCrate(Vector3 p) {
    if (nCrate >= MAX_CRATE || inHole(p.x, p.z)) return;
    float s = fr(0.7f, 1.0f);
    Crate *c = &crates[nCrate++];
    c->pos = xfP(curXf, p); c->s = s; c->hp = 3; c->alive = true;
    c->sid = addSolid(V3(p.x - s / 2, p.y, p.z - s / 2), V3(p.x + s / 2, p.y + s, p.z + s / 2));
}
static void vending(float x, float z, int face) {
    if (nVend >= MAX_VEND || inHole(x, z) || inHole(x + 1, z) || inHole(x - 1, z)) return;
    Vend *v = &vends[nVend];
    float w = 1.0f, d = 0.8f; Vector3 mn, mx;
    if (face == 0 || face == 2) { mn = V3(x - w / 2, SIDEH, z - d / 2); mx = V3(x + w / 2, SIDEH + 1.85f, z + d / 2); }
    else { mn = V3(x - d / 2, SIDEH, z - w / 2); mx = V3(x + d / 2, SIDEH + 1.85f, z + w / 2); }
    if (!pointFree(x, z, 0.6f, SIDEH + 0.05f)) return;
    v->mn = mn; v->mx = mx; v->face = face; v->hp = 3; v->broken = false; v->f = (short)curXf;
    Color bodies[4] = {{230, 230, 235, 255}, {200, 40, 40, 255}, {40, 90, 190, 255}, {235, 235, 120, 255}};
    v->body = bodies[ri(4)];
    v->sid = addSolid(mn, mx);
    Vector3 out = face == 0 ? V3(0, 0, -1) : face == 2 ? V3(0, 0, 1) : face == 1 ? V3(1, 0, 0) : V3(-1, 0, 0);
    v->light = addLight(Vector3Add(V3(x, SIDEH + 1.2f, z), Vector3Scale(out, 0.9f)), V3(0.75f, 0.85f, 1.0f), 6.5f, fr(0, 1) < 0.15f ? 1 : 0);
    nVend++;
}
#define MAX_CAR 64
typedef struct { Vector3 pos; float yaw; int kind; Color paint; } Car;
static Car cars[MAX_CAR]; static int nCar;
static void car(float x, float z, bool alongZ) {
    int kind = ri(2); float L = kind ? 4.4f : 3.4f, W = 1.7f;
    Color cs[6] = {{235, 235, 235, 255}, {190, 190, 195, 255}, {60, 60, 66, 255}, {150, 40, 40, 255}, {70, 90, 140, 255}, {210, 200, 170, 255}};
    Color c = cs[ri(6)];
    float hx = alongZ ? W / 2 : L / 2, hz = alongZ ? L / 2 : W / 2;
    if (!pointFree(x, z, fmaxf(hx, hz), 0.05f)) return;
    addSolid(V3(x - hx, 0.0f, z - hz), V3(x + hx, 0.95f, z + hz));
    float cx0 = alongZ ? x - hx + 0.08f : x - hx * 0.55f, cx1 = alongZ ? x + hx - 0.08f : x + hx * 0.35f;
    float cz0 = alongZ ? z - hz * 0.55f : z - hz + 0.08f, cz1 = alongZ ? z + hz * 0.35f : z + hz - 0.08f;
    addSolid(V3(cx0, 0.95f, cz0), V3(cx1, 1.5f, cz1));
    if (nCar < MAX_CAR) cars[nCar++] = (Car){V3(x, 0, z), alongZ ? (ri(2) ? 0 : PI) : (ri(2) ? PI / 2 : -PI / 2), kind, c};
    if (fr(0, 1) < 0.35f) addItem(V3(x + (alongZ ? hx + 0.5f : 0), 0.0f, z + (alongZ ? 0 : hz + 0.5f)), randomLoot(1), 1);
}

static void genPoles(float x0, float z0) {
    for (int s = 0; s < 4; s++) {
        bool ax = (s == 0 || s == 2);
        float fixedC = s == 0 ? z0 + 0.45f : s == 2 ? z0 + BLOCK - 0.45f : s == 1 ? x0 + BLOCK - 0.45f : x0 + 0.45f;
        float start = (ax ? x0 : z0) + 1.0f + fr(0, 2);
        float prev = -1;
        for (float u = start; u < (ax ? x0 : z0) + BLOCK - 1.0f; u += fr(11, 13)) {
            float px = ax ? u : fixedC, pz = ax ? fixedC : u;
            if (inHole(px, pz) || inHole(px + 0.8f, pz) || inHole(px - 0.8f, pz)) { prev = -1; continue; }
            box(T_CONCRETE, V3(px - 0.14f, SIDEH, pz - 0.14f), V3(px + 0.14f, 8.6f, pz + 0.14f), rgb(150, 150, 140), true, ALL);
            box(T_CONCRETE, ax ? V3(px - 0.7f, 7.7f, pz - 0.06f) : V3(px - 0.06f, 7.7f, pz - 0.7f), ax ? V3(px + 0.7f, 7.82f, pz + 0.06f) : V3(px + 0.06f, 7.82f, pz + 0.7f), rgb(120, 120, 115), false, ALLB);
            if (fr(0, 1) < 0.3f) box(T_METAL, V3(px - 0.35f, 6.0f, pz - 0.35f), V3(px + 0.35f, 6.9f, pz + 0.35f), rgb(120, 125, 120), false, ALLB);
            if (prev >= 0) {
                for (int w = 0; w < 3; w++) {
                    float y = 7.4f + w * 0.35f - 0.25f, off = (w - 1) * 0.5f;
                    if (ax) box(T_PLAIN, V3(prev, y, pz + off - 0.02f), V3(u, y + 0.035f, pz + off + 0.02f), rgb(25, 25, 25), false, ALLB);
                    else box(T_PLAIN, V3(px + off - 0.02f, y, prev), V3(px + off + 0.02f, y + 0.035f, u), rgb(25, 25, 25), false, ALLB);
                }
            }
            if (fr(0, 1) < 0.5f && nLamp < MAX_LAMP) {   // street lamp arm toward road
                Vector3 outd = s == 0 ? V3(0, 0, -1) : s == 2 ? V3(0, 0, 1) : s == 1 ? V3(1, 0, 0) : V3(-1, 0, 0);
                Vector3 head = Vector3Add(V3(px, 6.2f, pz), Vector3Scale(outd, 1.3f));
                Vector3 a = V3(px, 6.25f, pz);
                Vector3 mn = Vector3Min(a, head), mx = Vector3Max(a, head);
                box(T_CONCRETE, V3(mn.x - 0.05f, 6.25f, mn.z - 0.05f), V3(mx.x + 0.05f, 6.35f, mx.z + 0.05f), rgb(120, 120, 115), false, ALLB);
                float r = fr(0, 1);
                int li = addLight(V3(head.x, 5.9f, head.z), V3(1.0f, 0.62f, 0.28f), 12, r < 0.4f ? 0 : r < 0.7f ? 1 : 2);
                lamps[nLamp++] = (Lamp){head, li};
            }
            prev = u;
        }
    }
}
static void genBlock(float x0, float z0, int bi, int bj) {
    float L = x0 + 2.4f, R = x0 + BLOCK - 2.4f, Tz = z0 + 2.4f, B = z0 + BLOCK - 2.4f;
    int layout = ri(3);
    bool koban = (bi == kobanB[0] && bj == kobanB[1]), konbini = false;
    for (int k = 0; k < 5; k++) if (bi == konbiniB[k][0] && bj == konbiniB[k][1]) konbini = true;
    Rectangle lots[4]; int nl = 0;
    float mxs = (L + R) / 2, mzs = (Tz + B) / 2;
    if (!koban && !konbini && ri(10) < 6) {   // a Japanese town block: a ring of narrow houses and shops of mixed widths, each facing its street,
        // back yards, laundry poles and a shared alley in the middle; corners go to the wider side
        for (int s = 0; s < 4; s++) {
            float a0 = (s == 0 || s == 2) ? L : Tz, a1 = (s == 0 || s == 2) ? R : B, u = a0 + (s & 1 ? 9.0f : 0), uEnd = a1 - (s & 1 ? 9.0f : 0);
            while (u < uEnd - 4.5f) {
                float w = fminf(fr(5.0f, 9.0f), uEnd - u), d = fr(7.0f, 9.5f), set = fr(0, 0.9f);
                if (uEnd - (u + w) < 4.5f) w = uEnd - u;   // no sliver at the end of a row
                Rectangle r;
                if (s == 0) r = (Rectangle){u, Tz + set, w - 0.6f, d};
                else if (s == 2) r = (Rectangle){u, B - set - d, w - 0.6f, d};
                else if (s == 1) r = (Rectangle){R - set - d, u, d, w - 0.6f};
                else r = (Rectangle){L + set, u, d, w - 0.6f};
                float rr = fr(0, 1); int kind = rr < 0.42f ? 0 : rr < 0.75f ? 2 : 1, floors = kind == 1 ? 2 + ri(3) : 1 + ri(2);
                if (kind == 0 && fr(0, 1) < 0.07f) shrine(r); else if (fr(0, 1) < 0.08f) { /* an empty lot: a parked car or a garden */ }
                else building(r, kind, floors, s);
                u += w;
            }
        }
        for (int k2 = 0; k2 < 3; k2++) { float x = fr(L + 9.5f, R - 9.5f), z = fr(Tz + 9.5f, B - 9.5f);   // yards: laundry poles, potted plants
            if (!pointFree(x, z, 0.8f, SIDEH + 0.05f)) continue;
            if (k2 == 0) { box(T_METAL, V3(x - 1.2f, SIDEH, z - 0.03f), V3(x - 1.14f, SIDEH + 1.8f, z + 0.03f), rgb(150, 150, 150), true, ALL); box(T_METAL, V3(x + 1.14f, SIDEH, z - 0.03f), V3(x + 1.2f, SIDEH + 1.8f, z + 0.03f), rgb(150, 150, 150), true, ALL);
                box(T_METAL, V3(x - 1.2f, SIDEH + 1.75f, z - 0.02f), V3(x + 1.2f, SIDEH + 1.8f, z + 0.02f), rgb(170, 170, 170), false, ALLB); box(T_CLOTH, V3(x - 0.8f, SIDEH + 1.0f, z - 0.01f), V3(x + 0.3f, SIDEH + 1.75f, z + 0.01f), rgb(220, 215, 200), false, ALLB); }
            else box(T_HEDGE, V3(x - 0.4f, SIDEH, z - 0.4f), V3(x + 0.4f, SIDEH + 0.9f, z + 0.4f), rgb(70, 120, 70), true, ALL); }
        layout = -1;
    }
    if (layout == -1) {}
    else if (layout == 0) lots[nl++] = (Rectangle){L, Tz, R - L, B - Tz};
    else if (layout == 1) {
        if (ri(2)) { lots[nl++] = (Rectangle){L, Tz, mxs - 1 - L, B - Tz}; lots[nl++] = (Rectangle){mxs + 1, Tz, R - mxs - 1, B - Tz}; blockWall(V3(mxs, 0, Tz + 1), V3(mxs, 0, B - 1), 1.8f); }
        else { lots[nl++] = (Rectangle){L, Tz, R - L, mzs - 1 - Tz}; lots[nl++] = (Rectangle){L, mzs + 1, R - L, B - mzs - 1}; blockWall(V3(L + 1, 0, mzs), V3(R - 1, 0, mzs), 1.8f); }
    } else {
        lots[nl++] = (Rectangle){L, Tz, mxs - 1 - L, mzs - 1 - Tz}; lots[nl++] = (Rectangle){mxs + 1, Tz, R - mxs - 1, mzs - 1 - Tz};
        lots[nl++] = (Rectangle){L, mzs + 1, mxs - 1 - L, B - mzs - 1}; lots[nl++] = (Rectangle){mxs + 1, mzs + 1, R - mxs - 1, B - mzs - 1};
        blockWall(V3(mxs, 0, Tz + 1), V3(mxs, 0, B - 1), 1.8f); blockWall(V3(L + 1, 0, mzs), V3(R - 1, 0, mzs), 1.8f);
    }
    for (int i = 0; i < nl; i++) {
        Rectangle lot = lots[i];
        float dl = lot.x - x0, dr = x0 + BLOCK - (lot.x + lot.width), dt = lot.y - z0, db = z0 + BLOCK - (lot.y + lot.height);
        int street = 0; float best = dt;
        if (dr < best) { best = dr; street = 1; } if (db < best) { best = db; street = 2; } if (dl < best) { best = dl; street = 3; }
        int kind; int floors;
        if (koban && i == 0) {
            Rectangle k = {lot.x + 0.5f, lot.y + 0.5f, 4.2f, 4.2f};
            if (street == 1) k.x = lot.x + lot.width - 4.7f; if (street == 2) k.y = lot.y + lot.height - 4.7f;
            building(k, 4, 1, street);
            continue;
        }
        if (konbini && i == 0) { kind = 3; floors = 1; }
        else { float r = fr(0, 1); kind = (lot.width * lot.height > 300) ? (r < 0.5f ? 6 : r < 0.85f ? 1 : 2) : (r < 0.4f ? 0 : r < 0.7f ? 2 : 1); floors = kind == 6 ? 7 + ri(5) : kind == 1 ? 3 + ri(4) : kind == 2 ? 1 + ri(2) : 1 + ri(2); }
        if (kind == 0 && fr(0, 1) < 0.18f) { shrine(lot); continue; }
        float ins = kind == 0 ? 1.4f : 0.6f;
        Rectangle br = {lot.x + ins + fr(0, 0.6f), lot.y + ins + fr(0, 0.6f), 0, 0};
        br.width = lot.x + lot.width - ins - br.x - fr(0, 0.6f); br.height = lot.y + lot.height - ins - br.y - fr(0, 0.6f);
        if (br.width < 4 || br.height < 4) continue;
        building(br, kind, floors, street);
        if (kind == 0) {   // house garden wall with gate gap toward street
            float gx = br.x + br.width / 2, gz = br.y + br.height / 2;
            for (int s = 0; s < 4; s++) {
                Vector3 a, b;
                if (s == 0) { a = V3(lot.x, 0, lot.y); b = V3(lot.x + lot.width, 0, lot.y); }
                else if (s == 2) { a = V3(lot.x, 0, lot.y + lot.height); b = V3(lot.x + lot.width, 0, lot.y + lot.height); }
                else if (s == 1) { a = V3(lot.x + lot.width, 0, lot.y); b = V3(lot.x + lot.width, 0, lot.y + lot.height); }
                else { a = V3(lot.x, 0, lot.y); b = V3(lot.x, 0, lot.y + lot.height); }
                if (s == street) {
                    if (s == 0 || s == 2) { blockWall(a, V3(gx - 1.2f, 0, a.z), 1.5f); blockWall(V3(gx + 1.2f, 0, a.z), b, 1.5f); }
                    else { blockWall(a, V3(a.x, 0, gz - 1.2f), 1.5f); blockWall(V3(a.x, 0, gz + 1.2f), b, 1.5f); }
                } else blockWall(a, b, 1.5f);
            }
        }
    }
    // vending machines + crates along the block edge
    int nv = 1 + ri(3);
    for (int k = 0; k < nv; k++) {
        int s = ri(4); float u = fr(4, BLOCK - 4);
        if (s == 0) vending(x0 + u, z0 + 1.6f, 0); else if (s == 2) vending(x0 + u, z0 + BLOCK - 1.6f, 2);
        else if (s == 1) vending(x0 + BLOCK - 1.6f, z0 + u, 1); else vending(x0 + 1.6f, z0 + u, 3);
    }
    for (int k = 0; k < 3; k++) {
        float x = fr(x0 + 1.2f, x0 + BLOCK - 1.2f), z = fr(z0 + 1.2f, z0 + BLOCK - 1.2f);
        if (pointFree(x, z, 0.7f, SIDEH + 0.05f)) addCrate(V3(x, SIDEH, z));
    }
    if (fr(0, 1) < 0.55f) { int c = ri(4); float kx = c & 1 ? x0 + BLOCK - 1.3f : x0 + 1.3f, kz = c & 2 ? z0 + BLOCK - 1.3f : z0 + 1.3f;
        float ang = atan2f((c & 1 ? 1.0f : -1.0f), (c & 2 ? 1.0f : -1.0f)); kiosk(kx, kz, ang); }
}
static void genPark(float x0, float z0) {
    // clean-water park: pond with rim, dead fountain, aero info pylon
    float px0 = x0 + 8, px1 = x0 + BLOCK - 8, pz0 = z0 + 6, pz1 = z0 + 15;
    box(T_CONCRETE, V3(px0 - 0.4f, SIDEH, pz0 - 0.4f), V3(px1 + 0.4f, SIDEH + 0.45f, pz0), rgb(225, 235, 235), true, ALL);
    box(T_CONCRETE, V3(px0 - 0.4f, SIDEH, pz1), V3(px1 + 0.4f, SIDEH + 0.45f, pz1 + 0.4f), rgb(225, 235, 235), true, ALL);
    box(T_CONCRETE, V3(px0 - 0.4f, SIDEH, pz0), V3(px0, SIDEH + 0.45f, pz1), rgb(225, 235, 235), true, ALL);
    box(T_CONCRETE, V3(px1, SIDEH, pz0), V3(px1 + 0.4f, SIDEH + 0.45f, pz1), rgb(225, 235, 235), true, ALL);
    box(T_GLASS, V3(px0, SIDEH + 0.05f, pz0), V3(px1, SIDEH + 0.18f, pz1), rgb(70, 150, 140), false, 16);
    float fx = (px0 + px1) / 2, fz = (pz0 + pz1) / 2;
    box(T_CONCRETE, V3(fx - 1.2f, SIDEH, fz - 1.2f), V3(fx + 1.2f, SIDEH + 0.7f, fz + 1.2f), rgb(220, 230, 230), true, ALL);
    box(T_CONCRETE, V3(fx - 0.25f, SIDEH + 0.7f, fz - 0.25f), V3(fx + 0.25f, SIDEH + 2.2f, fz + 0.25f), rgb(220, 230, 230), true, ALL);
    box(T_GLASS, V3(fx - 0.7f, SIDEH + 2.2f, fz - 0.7f), V3(fx + 0.7f, SIDEH + 2.35f, fz + 0.7f), rgb(120, 220, 210), false, ALLB);
    box(T_PLAIN, V3(x0 + 4, SIDEH, z0 + 20), V3(x0 + 4.6f, SIDEH + 3.2f, z0 + 20.6f), rgb(230, 240, 242), true, ALL);
    addScreen(V3(x0 + 4.3f, SIDEH + 3.9f, z0 + 20.3f), 2.4f, 1.4f, PI, false);
    box(T_PLAIN, V3(x0 + 4.25f, SIDEH + 3.2f, z0 + 20.2f), V3(x0 + 4.35f, SIDEH + 4.6f, z0 + 20.4f), rgb(200, 210, 210), false, ALL);
    for (int k = 0; k < 18; k++) {
        float x = fr(x0 + 3, x0 + BLOCK - 3), z = fr(z0 + 3, z0 + BLOCK - 3);
        if (!pointFree(x, z, 1.5f, SIDEH + 0.05f)) continue;
        float h = fr(2.5f, 4.0f);
        box(T_WOOD, V3(x - 0.18f, SIDEH, z - 0.18f), V3(x + 0.18f, SIDEH + h, z + 0.18f), rgb(110, 90, 80), true, ALL);
        for (int f = 0; f < 3; f++) { float s = fr(1.4f, 2.4f), ox = fr(-0.8f, 0.8f), oz = fr(-0.8f, 0.8f), oy = h - 0.4f + f * 0.6f;
            box(T_CLOTH, V3(x + ox - s / 2, SIDEH + oy, z + oz - s / 2), V3(x + ox + s / 2, SIDEH + oy + s * 0.6f, z + oz + s / 2), rgb(70, 60, 75), false, ALLB); }
    }
    for (int k = 0; k < 5; k++) { float x = fr(x0 + 4, x0 + BLOCK - 6), z = fr(z0 + 4, z0 + BLOCK - 4);
        if (!pointFree(x, z, 1.2f, SIDEH + 0.05f)) continue;
        box(T_WOOD, V3(x, SIDEH + 0.4f, z), V3(x + 1.8f, SIDEH + 0.5f, z + 0.5f), WHITE, true, ALL);
        box(T_METAL, V3(x + 0.1f, SIDEH, z + 0.1f), V3(x + 0.2f, SIDEH + 0.4f, z + 0.4f), rgb(80, 80, 80), true, ALL);
        box(T_METAL, V3(x + 1.6f, SIDEH, z + 0.1f), V3(x + 1.7f, SIDEH + 0.4f, z + 0.4f), rgb(80, 80, 80), true, ALL);
        if (fr(0, 1) < 0.5f) addItem(V3(x + 0.9f, SIDEH + 0.5f, z + 0.25f), randomLoot(0), 1); }
    float sx = x0 + BLOCK / 2, sz = z0 + BLOCK / 2;   // swing frame
    box(T_RUST, V3(sx - 2, SIDEH, sz - 0.1f), V3(sx - 1.85f, SIDEH + 2.4f, sz + 0.1f), WHITE, true, ALL);
    box(T_RUST, V3(sx + 1.85f, SIDEH, sz - 0.1f), V3(sx + 2, SIDEH + 2.4f, sz + 0.1f), WHITE, true, ALL);
    box(T_RUST, V3(sx - 2, SIDEH + 2.3f, sz - 0.08f), V3(sx + 2, SIDEH + 2.45f, sz + 0.08f), WHITE, false, ALLB);
    for (int k = 0; k < 4; k++) addCrate(V3(fr(x0 + 3, x0 + BLOCK - 3), SIDEH, fr(z0 + 3, z0 + BLOCK - 3)));
    addLight(V3(sx, 5, sz), V3(0.9f, 0.85f, 0.7f), 10, 1);
}
static void genStation(float x0, float z0) {
    // station building
    building((Rectangle){x0 + 3, z0 + 3, 12, 9}, 5, 2, 0);
    // platform + tracks
    float pz0 = z0 + 17, pz1 = z0 + 22;
    box(T_CONCRETE, V3(x0 + 2, SIDEH, pz0), V3(x0 + 32, 1.05f, pz1), rgb(170, 170, 170), true, ALL);
    box(T_PLAIN, V3(x0 + 2, 1.05f, pz1 - 0.7f), V3(x0 + 32, 1.07f, pz1 - 0.4f), rgb(230, 200, 40), false, 16);
    for (int k = 0; k < 2; k++) box(T_CONCRETE, V3(x0 + 2 - 1.0f * (k + 1), SIDEH, pz0), V3(x0 + 2 - 1.0f * k, SIDEH + 0.32f * (2 - k), pz1), rgb(160, 160, 160), true, ALL);
    for (float x = x0 + 4; x < x0 + 32; x += 6) {
        box(T_METAL, V3(x, 1.05f, pz0 + 2.3f), V3(x + 0.2f, 4.2f, pz0 + 2.5f), rgb(90, 95, 100), true, ALL);
    }
    box(T_METAL, V3(x0 + 2, 4.2f, pz0 - 0.3f), V3(x0 + 32, 4.4f, pz1 + 0.3f), rgb(110, 115, 120), true, ALLB);
    for (int k = 0; k < 4; k++) addLight(V3(x0 + 6 + k * 7, 3.9f, (pz0 + pz1) / 2), V3(0.85f, 0.9f, 1.0f), 9, k == 1 ? 1 : (k == 3 ? 2 : 0));
    for (float x = x0; x < x0 + BLOCK; x += 0.8f) box(T_WOOD, V3(x, SIDEH, pz1 + 2.0f), V3(x + 0.3f, SIDEH + 0.12f, pz1 + 5.0f), rgb(90, 80, 70), false, ALL);
    box(T_RUST, V3(x0, SIDEH + 0.12f, pz1 + 2.6f), V3(x0 + BLOCK, SIDEH + 0.25f, pz1 + 2.7f), WHITE, false, ALL);
    box(T_RUST, V3(x0, SIDEH + 0.12f, pz1 + 4.3f), V3(x0 + BLOCK, SIDEH + 0.25f, pz1 + 4.4f), WHITE, false, ALL);
    phonePos = V3(x0 + 18, 1.05f, pz0 + 0.8f);
    box(T_METAL, V3(phonePos.x - 0.25f, 1.05f, phonePos.z - 0.15f), V3(phonePos.x + 0.25f, 2.35f, phonePos.z + 0.15f), rgb(40, 140, 80), true, ALL);
    addScreen(V3(x0 + 9, 3.2f, pz0 + 0.3f), 3.2f, 1.6f, PI, false);
    stationCenter = V3(x0 + BLOCK / 2, 1, z0 + BLOCK / 2);
    for (int k = 0; k < 6; k++) addCrate(V3(fr(x0 + 16, x0 + 31), SIDEH, fr(z0 + 3, z0 + 15)));
}

static void freeWorld(void) {
    for (int t = 0; t < T_COUNT; t++) {
        if (statOk[t]) { UnloadModel(stat[t]); statOk[t] = false; }
        free(mb[t].v); free(mb[t].t); free(mb[t].n); free(mb[t].c);
        memset(&mb[t], 0, sizeof(MB));
    }
    for (int c = 0; c < GN * GN; c++) cellN[c] = 0;
    nSol = nOps = nVend = nLight = nSign = nLamp = nCrate = nBuild = nItem = nCr = nWard = nScreen = nFire = nTV = nCar = 0;
    nCont = nDeb = nStair = nHole = 0; baseShrine = -1; nMapFoot = 0; nWs = 0; nXf = 1; curXf = 0;
    memset(parts, 0, sizeof parts);
}
static void finalizeMeshes(void) {
    for (int t = 0; t < T_COUNT; t++) {
        if (mb[t].cnt == 0) continue;
        Mesh m = {0};
        m.vertexCount = mb[t].cnt; m.triangleCount = mb[t].cnt / 3;
        m.vertices = mb[t].v; m.texcoords = mb[t].t; m.normals = mb[t].n; m.colors = mb[t].c;
        UploadMesh(&m, false);
        stat[t] = LoadModelFromMesh(m);
        stat[t].materials[0].shader = sh;
        stat[t].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex[t];
        statOk[t] = true;
        memset(&mb[t], 0, sizeof(MB));   // ownership moved to the model
    }
}
#include "zk_layout.h"
static bool onRoad(float x, float z) { return fmodf(x, PITCH) < ROADW || fmodf(z, PITCH) < ROADW; }
static void spawnCreatureT(Vector3 nearP, float minD, float maxD, bool alerted, int forceType);
static void spawnCreature(Vector3 nearP, float minD, float maxD, bool alerted) { spawnCreatureT(nearP, minD, maxD, alerted, -1); }
static void spawnCreatureT(Vector3 nearP, float minD, float maxD, bool alerted, int forceType) {
    int slot = -1;
    for (int i = 0; i < nCr; i++) if (!crs[i].on) { slot = i; break; }
    if (slot < 0) { if (nCr >= MAX_CR) return; slot = nCr++; }
    for (int tries = 0; tries < 40; tries++) {
        float x, z;
        if (maxD > 0) { float a = fr(0, 2 * PI), d = fr(minD, maxD); x = nearP.x + cosf(a) * d; z = nearP.z + sinf(a) * d; }
        else { x = fr(2, CITY - 2); z = fr(2, CITY - 2); if (Vector3Distance(V3(x, 0, z), pl.pos) < minD) continue; }
        if (x < 1 || z < 1 || x > CITY - 1 || z > CITY - 1) continue;
        if (forceType != 4 && (!pointFree(x, z, 0.5f, 0.2f) || !cityLand(x, z))) continue;
        if (forceType == 4) { bool warded = false; for (int w = 0; w < nWard; w++) if (wards[w].on && Vector3Distance(V3(x, 0, z), V3(wards[w].pos.x, 0, wards[w].pos.z)) < wards[w].r + 6) warded = true; if (warded) continue; }
        Cr *c = &crs[slot]; memset(c, 0, sizeof *c);
        float r = fr(0, 1);
        c->type = forceType >= 0 ? forceType : (r < 0.3f ? 0 : r < 0.45f ? 1 : r < 0.6f ? 2 : r < 0.72f ? 3 : r < 0.84f ? 6 : r < 0.94f ? 7 : 0);
        crStats(c);
        c->pos = V3(x, SIDEH, z); c->yaw = fr(-PI, PI); c->on = true;
        c->state = alerted ? CS_CHASE : CS_WANDER; c->tgt = c->pos; c->groanT = fr(2, 10); c->t = fr(0, 100); c->lastKnown = c->pos;
        if (forceType < 0 && c->type == 3) { Vector3 p = c->pos; for (int k2 = 0; k2 < 1 + ri(2); k2++) spawnAt(Vector3Add(p, V3(fr(-2, 2), 0, fr(-2, 2))), 3, alerted); }   // dogs hunt in packs
        return;
    }
}
static int streetOfKind(int k, float minW) {   // shot tests: the longest street of a kind (0 street, 1 avenue, 2 stairs, 3 bridge)
    int best = 0, bn = -1; for (int i = 0; i < nStreet; i++) if (dStreets[i][3] == k && dStreets[i][2] / 10.0f >= minW && dStreets[i][1] > bn) { bn = dStreets[i][1]; best = i; } return best; }
static Vector3 seaSidePt(int i, float d) {   // shot tests: a point d metres off the coast road, on the side towards the sea
    Vector2 nn = pnorm(coastP, nCoast, i); Vector3 a = V3(coastP[i].x + nn.x * d, 0, coastP[i].z + nn.y * d), b = V3(coastP[i].x - nn.x * d, 0, coastP[i].z - nn.y * d);
    return terrainH(a.x, a.z) < terrainH(b.x, b.z) ? a : b; }
static Vector3 opCenter(Open *o); static Vector3 opNormal(Open *o);
static void genWorld(unsigned int seed) {
    freeWorld();
    hfReady = false; layoutInit();
    rs = 20031103u; worldSeed = seed;   // the town is fixed: same streets, buildings and rooms every run
    // subway stairwells cut holes into the ground; the city opens onto the wilds (no perimeter wall)
    nHole = 0;
    {   // road surface: tiles only where no sidewalk block covers it
        for (int i = 0; i <= NB; i++) for (int j = 0; j <= NB; j++) {
            if (nodeOn(i, j)) tileBox(T_ASPHALT, (Rectangle){i * PITCH, j * PITCH, ROADW, ROADW}, -0.5f, 0, WHITE, 16);
            if (segV(i, j)) tileBox(T_ASPHALT, (Rectangle){i * PITCH, ROADW + j * PITCH, ROADW, BLOCK}, -0.5f, 0, WHITE, 16);
            if (segH(i, j)) tileBox(T_ASPHALT, (Rectangle){ROADW + i * PITCH, j * PITCH, BLOCK, ROADW}, -0.5f, 0, WHITE, 16);
        }
    }
    // road markings
    for (int i = 0; i <= NB; i++) {
        float c = i * PITCH + ROADW / 2;
        for (int j = 0; j < NB; j++) {
            float s0 = ROADW + j * PITCH;
            for (float u = s0 + 1; u < s0 + BLOCK - 1; u += 3) {
                if (segV(i, j)) box(T_PLAIN, V3(c - 0.07f, 0, u), V3(c + 0.07f, 0.05f, u + 1.5f), rgb(190, 190, 180), false, 16);
                if (segH(j, i)) box(T_PLAIN, V3(u, 0, c - 0.07f), V3(u + 1.5f, 0.05f, c + 0.07f), rgb(190, 190, 180), false, 16);
            }
        }
        for (int j = 0; j <= NB; j++) {   // crosswalks next to intersection (i,j)
            float ix = i * PITCH, iz = j * PITCH;
            if (iz + ROADW + 3.5f < CITY && segV(i, j)) for (float u = ix + 0.6f; u < ix + ROADW - 0.4f; u += 1.0f) box(T_PLAIN, V3(u, 0, iz + ROADW + 0.5f), V3(u + 0.5f, 0.05f, iz + ROADW + 3.0f), rgb(200, 200, 190), false, 16);
            if (ix + ROADW + 3.5f < CITY && segH(i, j)) for (float u = iz + 0.6f; u < iz + ROADW - 0.4f; u += 1.0f) box(T_PLAIN, V3(ix + ROADW + 0.5f, 0, u), V3(ix + ROADW + 3.0f, 0.05f, u + 0.5f), rgb(200, 200, 190), false, 16);
        }
    }
    // blocks
    for (int bi = 0; bi < NB; bi++) for (int bj = 0; bj < NB; bj++) {
        float x0 = ROADW + bi * PITCH, z0 = ROADW + bj * PITCH;
        int bk = blockKindA[bi][bj];
        if (bk == BK_GONE) continue;
        if (bk == BK_PLAZA) { genPlaza(x0, z0, bi, bj); continue; }
        groundBox(T_SIDEWALK, (Rectangle){x0, z0, BLOCK, BLOCK}, 0, SIDEH, WHITE, true, 0);   // collision only
        nFoot = 0;
        genPoles(x0, z0);
        if (bk == BK_STATION) genStation(x0, z0);
        else if (bk == BK_PARK) genPark(x0, z0);
        else if (bk == BK_OLD) genOldTown(x0, z0, bi, bj);
        else if (bk == BK_MARKET) genMarket(x0, z0);
        else genBlock(x0, z0, bi, bj);
        for (int f = 0; f < nFoot && nMapFoot < MAX_MAPFOOT; f++) { mapFootH[nMapFoot] = footH[f]; mapFootY[nMapFoot] = 0; mapFootF[nMapFoot] = 0; mapFoot[nMapFoot++] = foots[f]; }
        {   // visible sidewalk: tiles, cut around subway holes and building floors
            Rectangle cut[MAX_FOOT + MAX_HOLE]; int nc = 0;
            for (int i = 0; i < nFoot; i++) cut[nc++] = foots[i];
            for (int i = 0; i < nHole; i++) cut[nc++] = holes[i];
            cutBox(T_SIDEWALK, (Rectangle){x0, z0, BLOCK, BLOCK}, 0, SIDEH, WHITE, ALL, cut, nc);
        }
    }
    // cars on roads
    for (int k = 0; k < 60; k++) {
        bool alongZ = ri(2);
        int line = ri(NB + 1), seg = ri(NB);
        if (alongZ ? !segV(line, seg) : !segH(seg, line)) continue;
        float lane = fr(0, 1) < 0.5f ? 2.6f : ROADW - 2.6f;
        float u = ROADW + seg * PITCH + fr(3, BLOCK - 3);
        if (alongZ) car(line * PITCH + lane, u, true); else car(u, line * PITCH + lane, false);
    }
    bakeHeightfield();
    genLayoutWorld();
    // scarce extras: 2 axes, 1 katana, 3 ammo stashes
    int want[6] = {IT_AXE, IT_AXE, IT_KATANA, IT_AMMO, IT_AMMO, IT_AMMO};
    for (int w = 0; w < 6; w++) {
        for (int tries = 0; tries < 200; tries++) {
            int o = ri(nOps > 0 ? nOps : 1);
            if (nOps == 0) break;
            Open *op = &ops[o];
            Vector3 c = opCenter(op);
            Vector3 inward = Vector3Negate(opNormal(op));
            Vector3 p = Vector3Add(c, Vector3Scale(inward, 1.6f));
            if (Vector3Distance(p, V3(SPAWN_X, 0, SPAWN_Z)) < 60 && want[w] != IT_AMMO) continue;
            if (!pointFree(p.x, p.z, 0.25f, SIDEH + 0.05f)) continue;
            addItem(V3(p.x, SIDEH, p.z), want[w], want[w] == IT_AMMO ? 2 : 1);
            break;
        }
    }
    genWilds();
    genWater();
    worldSolids = nSol; worldLights = nLight;
    TraceLog(LOG_INFO, "WORLD: %d solids, %d containers, %d stairs, %d junk piles, %d openings, %d water sources, %d buildings", nSol, nCont, nStair, nDeb, nOps, nWs, nMapFoot);
    finalizeMeshes();
    genTerrain();
    rs = seed ? seed : 1;   // runs differ from here on: creatures, weather, events
}

// ============================================================ physics
static void resolve(Vector3 *p, float r, float h, float step) {
    query(p->x - r - 0.1f, p->z - r - 0.1f, p->x + r + 0.1f, p->z + r + 0.1f);
    for (int k = 0; k < qn; k++) {
        Solid *s = &sol[qb[k]];
        if (s->mx.y <= p->y + step) continue;
        if (s->mn.y >= p->y + h) continue;
        Vector3 L = solL(s, *p);   // push out in the solid's own frame, then back to the world
        float cx = clampf(L.x, s->mn.x, s->mx.x), cz = clampf(L.z, s->mn.z, s->mx.z);
        float dx = L.x - cx, dz = L.z - cz, d2 = dx * dx + dz * dz;
        if (d2 < r * r) {
            if (d2 > 1e-8f) { float d = sqrtf(d2); L.x += dx / d * (r - d); L.z += dz / d * (r - d); }
            else {
                float l = L.x - s->mn.x, rr = s->mx.x - L.x, t = L.z - s->mn.z, b = s->mx.z - L.z;
                float m = fminf(fminf(l, rr), fminf(t, b));
                if (m == l) L.x = s->mn.x - r; else if (m == rr) L.x = s->mx.x + r; else if (m == t) L.z = s->mn.z - r; else L.z = s->mx.z + r;
            }
            Vector3 Wp = s->f ? xfP(s->f, L) : L; p->x = Wp.x; p->z = Wp.z;
        }
    }
    if (p->y > -6 && hfReady && !cityLand(p->x, p->z) && !inBore(p->x, p->z, p->y) && seaDepth(p->x, p->z) > 0.05f) {   // the sea and the river push you back to shore
        for (int it = 0; it < 16 && seaDepth(p->x, p->z) > 0.05f; it++) {
            float gx = terrainH(p->x + 1, p->z) - terrainH(p->x - 1, p->z), gz = terrainH(p->x, p->z + 1) - terrainH(p->x, p->z - 1), L = sqrtf(gx * gx + gz * gz);
            if (L < 1e-4f) break; p->x += gx / L * 0.25f; p->z += gz / L * 0.25f; } }
    p->x = clampf(p->x, -WILD + 4, CITY + WILD - 4); p->z = clampf(p->z, -WILD + 4, CITY + WILD - 4);
}
static float groundAt(Vector3 p, float r, float step) {
    float b = baseGroundY(p.x, p.z, p.y);
    float g = p.y > b - 1.2f ? b : -1e9f; float rr = r * 0.7f;
    query(p.x - rr, p.z - rr, p.x + rr, p.z + rr);
    for (int k = 0; k < qn; k++) {
        Solid *s = &sol[qb[k]]; Vector3 L = solL(s, p);
        if (L.x > s->mn.x - rr && L.x < s->mx.x + rr && L.z > s->mn.z - rr && L.z < s->mx.z + rr)
            if (s->mx.y <= p.y + step && s->mx.y > g) g = s->mx.y;
    }
    return g;
}
static float ceilAt(Vector3 p, float r) {
    float c = 1e9f; float rr = r * 0.7f;
    query(p.x - rr, p.z - rr, p.x + rr, p.z + rr);
    for (int k = 0; k < qn; k++) {
        Solid *s = &sol[qb[k]]; Vector3 L = solL(s, p);
        if (L.x > s->mn.x - rr && L.x < s->mx.x + rr && L.z > s->mn.z - rr && L.z < s->mx.z + rr)
            if (s->mn.y >= p.y + 0.4f && s->mn.y < c) c = s->mn.y;
    }
    return c;
}
static bool findLedge(Vector3 p, Vector3 f, Vector3 *dest) {
    for (float d = 0.45f; d <= 1.05f; d += 0.2f) {
        float x = p.x + f.x * d, z = p.z + f.z * d; float best = -1; bool blocked = false;
        query(x - 0.05f, z - 0.05f, x + 0.05f, z + 0.05f);
        for (int k = 0; k < qn; k++) {
            Solid *s = &sol[qb[k]]; Vector3 L = solL(s, (Vector3){x, 0, z});
            if (L.x < s->mn.x || L.x > s->mx.x || L.z < s->mn.z || L.z > s->mx.z) continue;
            float dh = s->mx.y - p.y;
            if (dh > 0.45f && dh <= 2.45f) { if (s->mx.y > best) best = s->mx.y; }
            else if (dh > 2.45f && s->mn.y < p.y + 2.45f) blocked = true;
        }
        if (blocked) return false;
        if (best > 0) {
            bool ok = true;
            query(x - 0.28f, z - 0.28f, x + 0.28f, z + 0.28f);
            for (int k = 0; k < qn; k++) {
                Solid *s = &sol[qb[k]]; Vector3 L = solL(s, (Vector3){x, 0, z});
                if (L.x + 0.25f < s->mn.x || L.x - 0.25f > s->mx.x || L.z + 0.25f < s->mn.z || L.z - 0.25f > s->mx.z) continue;
                if (s->mn.y < best + 1.7f && s->mx.y > best + 0.02f) { ok = false; break; }
            }
            if (ok) { *dest = V3(x, best, z); return true; }
        }
    }
    return false;
}
static Vector3 opCenter(Open *o) { return xfP(o->f, Vector3Scale(Vector3Add(o->mn, o->mx), 0.5f)); }   // world
static Vector3 opNormal(Open *o) { return xfD(o->f, o->axis == 1 ? V3(0, 0, o->side) : V3(o->side, 0, 0)); }
static int facingOpening(Vector3 eyeP, Vector3 f, float maxD, int wantType, int wantState) {
    int best = -1; float bd = maxD;
    for (int i = 0; i < nOps; i++) {
        Open *o = &ops[i];
        if (wantType >= 0 && o->type != wantType) continue;
        if (wantState >= 0 && o->state != wantState) continue;
        Vector3 c = opCenter(o);
        Vector3 d = V3(c.x - eyeP.x, 0, c.z - eyeP.z);
        float dist = Vector3Length(d);
        if (dist > bd || dist < 0.01f) continue;
        Vector3 fl = Vector3Normalize(V3(f.x, 0, f.z));
        if (Vector3DotProduct(Vector3Scale(d, 1.0f / dist), fl) < 0.55f) continue;
        bd = dist; best = i;
    }
    return best;
}

// ============================================================ particles / messages
static void spawnParts(Vector3 p, int n, int t, Color tint, float spd, float size) {
    for (int k = 0; k < n; k++) for (int i = 0; i < MAX_PART; i++) if (parts[i].life <= 0) {
        parts[i] = (Part){p, V3(fr(-spd, spd), fr(0.5f, spd * 1.5f), fr(-spd, spd)), fr(0.6f, 1.4f), size * fr(0.6f, 1.3f), p.y - 2.0f, t, tint};
        break;
    }
}
static void say(const char *s) { snprintf(msg, sizeof msg, "%s", s); msgT = 3.0f; }
static void alertAll(Vector3 p, float radius) {
    // noise: they come to where the sound was, not to where you are
    for (int i = 0; i < nCr; i++) if (crs[i].on && crs[i].type != 4 && crs[i].state == CS_WANDER && Vector3Distance(crs[i].pos, p) < radius) {   // spirits ignore noise
        crs[i].state = CS_CHASE; crs[i].lastKnown = Vector3Add(p, V3(fr(-1.5f, 1.5f), 0, fr(-1.5f, 1.5f))); crs[i].losT = 0; }
}
static void breakWindow(Open *o) {
    if (o->state != 0) return;
    o->state = 1; if (o->sid >= 0) sol[o->sid].off = true;
    spawnParts(opCenter(o), 14, T_GLASS, rgb(180, 200, 220), 2.5f, 0.07f);
    playAt(sGlass, opCenter(o), 40, 0.9f);
    alertAll(opCenter(o), 26);
}

// ============================================================ input indirection (scripted demo recorder)
typedef struct { bool on; unsigned char down[512], pressed[512]; bool lmb, lmbP; Vector2 md; float wheel; } VIn;
static VIn vin;
static bool KD(int k) { return vin.on ? vin.down[k] : IsKeyDown(k); }
static bool KP(int k) { return vin.on ? vin.pressed[k] : IsKeyPressed(k); }
static bool MD(void) { return vin.on ? vin.lmb : IsMouseButtonDown(MOUSE_BUTTON_LEFT); }
static bool MP(void) { return vin.on ? vin.lmbP : IsMouseButtonPressed(MOUSE_BUTTON_LEFT); }
static Vector2 MDel(void) { return vin.on ? vin.md : GetMouseDelta(); }
static float MWheel(void) { return vin.on ? vin.wheel : GetMouseWheelMove(); }

// ============================================================ game reset
#include "zk_animdef.h"
#include "zk_inv.h"
enum { CH_GOD, CH_STAM, CH_NEEDS, CH_AMMO, CH_WEAR, CH_ONEHIT, CH_NOCLIP, CH_PEACE, CH_TIME, CH_LIGHT, CH_N };
static struct { bool avail, menu, on[CH_N]; int key; float speed, frozenTod; char mod[64]; } cht = {.speed = 1, .key = KEY_F8};   // cheat menu state (zk_cheat.h)
static void cheatsReset(void);
#include "zk_player.h"

// ============================================================ sanity
static void updateSanity(float dt, Vector3 aim) {
    Vector3 eye = Vector3Add(pl.pos, V3(0, 1.6f, 0));
    pl.roof = ceilAt(pl.pos, 0.3f) < pl.pos.y + 9;
    pl.nearFire = false;
    for (int i = 0; i < nFire; i++) if (fires[i].on) {
        fires[i].life -= dt;
        if (fires[i].life <= 0) { fires[i].on = false; if (fires[i].light >= 0) lights[fires[i].light].mode = 2; continue; }
        if (Vector3Distance(fires[i].pos, pl.pos) < 4.5f) pl.nearFire = true;
    }
    float target = pl.nearFire ? 95 : pl.roof ? 55 : 22;
    pl.warmth += (target - pl.warmth) * dt * (pl.nearFire ? 0.08f : 0.02f);
    if (!pl.sleeping) pl.rest = fmaxf(0, pl.rest - dt * 100.0f / 900.0f * trMul(7));
    if (!pl.sleeping && pl.watchT <= 0) pl.boredom = fminf(100, pl.boredom + dt * 100.0f / 300.0f);
    int visible = 0; float nearest = 1e9f;
    for (int i = 0; i < nCr; i++) {
        Cr *c = &crs[i]; if (!c->on || c->state == CS_DEAD) continue;
        Vector3 d = Vector3Subtract(crCenter(c), eye); float L = Vector3Length(d);
        if (L < nearest) nearest = L;
        if (L < 30 && Vector3DotProduct(Vector3Scale(d, 1 / L), aim) > 0.75f && (L < 12 || pl.flash)) visible++;
    }
    pl.peaceT = nearest > 25 ? pl.peaceT + dt : 0;
    float g = 0, l = 0;
    if (pl.food > 60 && pl.water > 60) g += 0.25f;
    if (pl.peaceT > 20) g += 0.35f;
    if (pl.roof) g += 0.25f;
    if (pl.warmth > 60) g += 0.2f;
    if (pl.rest > 60) g += 0.15f;
    if (pl.sleeping) g += 0.6f;
    // (+ human connection: no other humans exist until multiplayer — reserved)
    if (pl.food < 25 || pl.water < 25) l += 0.5f;
    l += fminf((float)visible, 4) * 0.35f;
    if (!pl.roof) l += 0.12f;
    if (pl.warmth < 35) l += 0.4f;
    if (pl.rest < 25) l += 0.4f;
    if (pl.boredom > 70) l += 0.35f;
    { float wk = wildK(pl.pos); l += wk * 3.2f; if (wk > 0.3f) g *= 0.2f; }   // the hills eat your mind
    if (pl.pos.y < -40) l += 0.3f;
    if (pl.stage >= 2) g *= 0.5f;
    if (pl.sick > 30) l += 0.25f;
    if (pl.fracture == 1) l += 0.2f;
    if (dayK() < 0.2f && !pl.roof) l += 0.15f;   // the dark outside
    pl.sanity = clampf(pl.sanity + (g - l * trMul(8)) * dt, 0, 100);
    // permanent stages
    float thr = pl.stage == 0 ? 50 : pl.stage == 1 ? 30 : 12, need = pl.stage == 1 ? 90 : 60;
    if (pl.stage < 3) {
        if (pl.sanity < thr) pl.lowT += dt; else pl.lowT = fmaxf(0, pl.lowT - dt * 0.5f);
        if (pl.lowT > need) {
            pl.stage++; pl.lowT = 0;
            say(pl.stage == 1 ? "Sadness settles in. It will not leave." : pl.stage == 2 ? "Depression. Everything is heavier now." : "Insanity. Something else is steering you.");
            play(sHeart, 1.0f);
        }
    } else {
        pl.corrupt = fminf(100, pl.corrupt + dt * 100.0f / 600.0f * (pl.sanity < 12 ? 1.0f : 0.5f));
        if (pl.corrupt >= 100) pl.turned = true;
    }
}

// ============================================================ creatures
#include "zk_creat.h"
#include "zk_text.h"
#include "zk_surv.h"

// ============================================================ drawing
static Model cube[T_COUNT], cyl, scrQuad;
static RenderTexture2D scrRT;
static const char *bcast[] __attribute__((unused)) = {"A BRIGHT, GREEN FUTURE", "CLEAN WATER FOR|EVERY CITIZEN", "THANK YOU FOR|REMAINING CALM", "PLEASE STAY|INDOORS",
    "ECO-TOWER LIVING|2006", "THE MINISTRY|IS WITH YOU", "SMILE, NEIGHBOR!", "RELIEF IS|ON THE WAY", "BREATHE DEEPLY.|ALL IS WELL."};
static void updateBroadcast(float t) {
    const int W = 160, H = 96;
    BeginTextureMode(scrRT);
    float lost = hash2((int)(t / 2.5f), 1, 5);
    if (lost > 0.9f) {
        ClearBackground((Color){10, 14, 18, 255});
        for (int y = 0; y < H; y += 2) for (int x = 0; x < W; x += 4) { float v = hash2(x + (int)(t * 60), y, 2) * 120; DrawRectangle(x, y, 4, 2, (Color){(unsigned char)v, (unsigned char)v, (unsigned char)v, 255}); }
        if (jpOk) jpDrawFit(jpNoSignal, (Rectangle){0, H / 2 - 10, W, 20}, (Color){240, 240, 240, 255}, false); else DrawText("NO SIGNAL", W / 2 - MeasureText("NO SIGNAL", 10) / 2, H / 2 - 5, 10, (Color){240, 240, 240, 255});
        EndTextureMode(); return;
    }
    DrawRectangleGradientV(0, 0, W, H, (Color){70, 175, 225, 255}, (Color){150, 225, 210, 255});
    DrawCircleGradient(128, 18, 34, (Color){255, 255, 225, 230}, (Color){255, 255, 225, 0});
    DrawCircle(30, 130, 70, (Color){70, 180, 80, 255}); DrawCircle(135, 140, 80, (Color){55, 160, 70, 255});
    for (int k = 0; k < 6; k++) { float bx = fmodf(k * 31 + t * 6, W), by = H - fmodf(t * 10 + k * 17, H + 10);
        DrawCircleLines((int)bx, (int)by, 3 + k % 3, (Color){255, 255, 255, 150}); DrawPixel((int)bx - 1, (int)by - 1, WHITE); }
    int mi = (int)(t / 6) % JP_NBCAST;
    char line[2][96] = {"", ""}; const char *m = jpBcast[mi]; const char *bar = strchr(m, '|');
    if (bar) { snprintf(line[0], 96, "%.*s", (int)(bar - m), m); snprintf(line[1], 96, "%s", bar + 1); } else snprintf(line[0], 96, "%s", m);
    for (int l = 0; l < 2; l++) if (line[l][0]) { Rectangle lr = {6, (float)(bar ? 20 + l * 22 : 30), W - 12, 20};
        if (jpOk) { jpDrawFit(line[l], (Rectangle){lr.x + 1, lr.y + 1, lr.width, lr.height}, (Color){20, 70, 90, 255}, false); jpDrawFit(line[l], lr, WHITE, false); } }
    DrawRectangle(0, 80, W, 16, (Color){8, 50, 70, 230});
    if (jpOk) { Vector2 tm = MeasureTextEx(jpFont, jpTicker, 13, 1); float tx = W - fmodf(t * 28, tm.x + W); DrawTextEx(jpFont, jpTicker, (Vector2){tx, 81}, 13, 1, (Color){200, 250, 240, 255}); }
    DrawText("2006", 4, 3, 10, (Color){255, 255, 255, 200});
    float g = hash2((int)(t * 9), 0, 3);
    if (g > 0.8f) for (int k = 0; k < 5; k++) { int y = (int)(hash2(k, (int)(t * 9), 4) * H); int off = (int)((hash2(k, (int)(t * 9), 6) - 0.5f) * 30);
        DrawRectangle(off, y, W, 2 + k % 3, (Color){(unsigned char)(255 * hash2(k, 1, (int)(t * 9))), 40, 60, 160}); }
    EndTextureMode();
}
static Mesh genQuad(void) {
    Mesh m = {0};
    m.vertexCount = 4; m.triangleCount = 2;
    m.vertices = MemAlloc(12 * sizeof(float)); m.texcoords = MemAlloc(8 * sizeof(float)); m.normals = MemAlloc(12 * sizeof(float));
    m.indices = MemAlloc(6 * sizeof(unsigned short));
    float v[12] = {-0.5f, -0.5f, 0, 0.5f, -0.5f, 0, 0.5f, 0.5f, 0, -0.5f, 0.5f, 0}, tc[8] = {0, 0, 1, 0, 1, 1, 0, 1};
    unsigned short id[6] = {0, 1, 2, 0, 2, 3};
    memcpy(m.vertices, v, sizeof v); memcpy(m.texcoords, tc, sizeof tc); memcpy(m.indices, id, sizeof id);
    for (int i = 0; i < 4; i++) { m.normals[i * 3] = 0; m.normals[i * 3 + 1] = 0; m.normals[i * 3 + 2] = 1; }
    UploadMesh(&m, false);
    return m;
}
static void drawM(Model *m, Matrix M, Color tint) { m->transform = M; DrawModel(*m, V3(0, 0, 0), 1.0f, tint); }
static void drawBox(int t, Vector3 mn, Vector3 mx, Color tint) {
    Vector3 c = Vector3Scale(Vector3Add(mn, mx), 0.5f), s = Vector3Subtract(mx, mn);
    drawM(&cube[t], MatrixMultiply(MatrixScale(s.x, s.y, s.z), MatrixTranslate(c.x, c.y, c.z)), tint);
}
static Matrix frameM(Matrix parent, Vector3 pivot, float rx, float ry, float rz) {
    return MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixRotateZ(rz), MatrixRotateX(rx)), MatrixRotateY(ry)), MatrixTranslate(pivot.x, pivot.y, pivot.z)), parent);
}
// ---- OBJ assets: every prop / creature part / item / held weapon is an .obj + .mtl in Data/Models (mods can replace them).
// The default file is written from the box definition at the call site the first time it is needed, then loaded from disk.
typedef struct { int tex; Vector3 off, size; Color col; } ABox;
typedef struct { char id[64]; Model m; } Asset;
#define MAX_ASSET 200
static Asset assets[MAX_ASSET]; static int nAsset;
static int objVn = 0;
static void objQuad(FILE *f, int *vi, Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 n) {
    fprintf(f, "v %.4f %.4f %.4f\nv %.4f %.4f %.4f\nv %.4f %.4f %.4f\nv %.4f %.4f %.4f\n", a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z, d.x, d.y, d.z);
    fprintf(f, "vn %.1f %.1f %.1f\n", n.x, n.y, n.z);
    int vn = ++objVn;
    int k = *vi;
    fprintf(f, "f %d/1/%d %d/2/%d %d/3/%d %d/4/%d\n", k, vn, k + 1, vn, k + 2, vn, k + 3, vn);
    *vi += 4;
}
static void writeAssetObj(const char *objPath, const char *id, const ABox *bx, int n) {
    char dir[900]; snprintf(dir, sizeof dir, "%s", GetDirectoryPath(objPath)); MakeDirectory(dir);
    char mtlPath[900]; snprintf(mtlPath, sizeof mtlPath, "%s/%s.mtl", dir, GetFileNameWithoutExt(objPath));
    int depth = 2; for (const char *c = id; *c; c++) if (*c == '/') depth++;   // Data/Models/<id> -> back to the game folder
    char up[64] = ""; for (int i = 0; i < depth; i++) strcat(up, "../");
    FILE *m = fopen(mtlPath, "w");
    if (m) {
        fprintf(m, "# Zanko 2006 material file for %s\n# map_Kd may point to any PNG; textures from Textures/ (or a mod's Textures/) are swapped in by name.\n", id);
        for (int i = 0; i < n; i++)
            fprintf(m, "\nnewmtl part%d\nKa 0 0 0\nKd %.3f %.3f %.3f\nKs 0 0 0\nd 1\nillum 1\nmap_Kd %sTextures/%s\n", i, bx[i].col.r / 255.0f, bx[i].col.g / 255.0f, bx[i].col.b / 255.0f, up, texNames[bx[i].tex]);
        fclose(m);
    }
    FILE *f = fopen(objPath, "w");
    if (!f) return;
    fprintf(f, "# Zanko 2006 asset: %s\n# Units: metres. +Y up, +Z forward (the way the creature/object faces). Origin = pivot used by the game.\n", id);
    fprintf(f, "# Remodel freely in Blender etc. (export: OBJ, Y up, -Z forward OFF, triangulate optional). Keep the pivot at the origin.\n");
    fprintf(f, "mtllib %s.mtl\no %s\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n", GetFileNameWithoutExt(objPath), GetFileNameWithoutExt(objPath));
    int vi = 1; objVn = 0;
    for (int i = 0; i < n; i++) {
        Vector3 mn = Vector3Subtract(bx[i].off, Vector3Scale(bx[i].size, 0.5f)), mx = Vector3Add(bx[i].off, Vector3Scale(bx[i].size, 0.5f));
        float x0 = mn.x, y0 = mn.y, z0 = mn.z, x1 = mx.x, y1 = mx.y, z1 = mx.z;
        fprintf(f, "g part%d\nusemtl part%d\n", i, i);
        objQuad(f, &vi, V3(x1, y0, z1), V3(x1, y0, z0), V3(x1, y1, z0), V3(x1, y1, z1), V3(1, 0, 0));
        objQuad(f, &vi, V3(x0, y0, z0), V3(x0, y0, z1), V3(x0, y1, z1), V3(x0, y1, z0), V3(-1, 0, 0));
        objQuad(f, &vi, V3(x0, y0, z1), V3(x1, y0, z1), V3(x1, y1, z1), V3(x0, y1, z1), V3(0, 0, 1));
        objQuad(f, &vi, V3(x1, y0, z0), V3(x0, y0, z0), V3(x0, y1, z0), V3(x1, y1, z0), V3(0, 0, -1));
        objQuad(f, &vi, V3(x0, y1, z1), V3(x1, y1, z1), V3(x1, y1, z0), V3(x0, y1, z0), V3(0, 1, 0));
        objQuad(f, &vi, V3(x0, y0, z0), V3(x1, y0, z0), V3(x1, y0, z1), V3(x0, y0, z1), V3(0, -1, 0));
    }
    fclose(f);
}
static void assetFixMaterials(Model *m, const char *objPath) {
    // raylib loads Kd + map_Kd; swap textures that name a game texture for the resolved (mod-aware) one
    char mtlPath[900]; snprintf(mtlPath, sizeof mtlPath, "%s/%s.mtl", GetDirectoryPath(objPath), GetFileNameWithoutExt(objPath));
    char *t = LoadFileText(mtlPath);
    int idx = -1;
    if (t) {
        char *line = t;
        while (line && *line) {
            char *nl = strchr(line, '\n'); if (nl) *nl = 0;
            while (*line == ' ' || *line == '\t') line++;
            if (!strncmp(line, "newmtl", 6)) idx++;
            else if (!strncmp(line, "Kd ", 3) && idx >= 0 && idx < m->materialCount) {   // raylib ignores Kd when map_Kd is set
                float r = 1, g = 1, bb = 1; sscanf(line + 3, "%f %f %f", &r, &g, &bb);
                m->materials[idx].maps[MATERIAL_MAP_DIFFUSE].color = (Color){cb(r * 255), cb(g * 255), cb(bb * 255), 255};
            }
            else if (!strncmp(line, "map_Kd", 6) && idx >= 0 && idx < m->materialCount) {
                char fn[300]; snprintf(fn, sizeof fn, "%s", GetFileName(line + 7));
                for (char *c = fn; *c; c++) if (*c == '\r' || *c == ' ') { *c = 0; break; }
                for (int k = 0; k < T_COUNT; k++) if (!strcmp(fn, texNames[k])) { m->materials[idx].maps[MATERIAL_MAP_DIFFUSE].texture = tex[k]; break; }
            }
            line = nl ? nl + 1 : NULL;
        }
        UnloadFileText(t);
    }
    for (int i = 0; i < m->materialCount; i++) {
        m->materials[i].shader = sh;
        if (m->materials[i].maps[MATERIAL_MAP_DIFFUSE].texture.id == rlGetTextureIdDefault()) m->materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = tex[T_PLAIN];
        SetTextureFilter(m->materials[i].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_FILTER_POINT);
        SetTextureWrap(m->materials[i].maps[MATERIAL_MAP_DIFFUSE].texture, TEXTURE_WRAP_REPEAT);
    }
}
static int assetGet(const char *id, const ABox *bx, int n) {
    for (int i = 0; i < nAsset; i++) if (!strcmp(assets[i].id, id)) return i;
    if (nAsset >= MAX_ASSET) return -1;
    char rel[200], path[900];
    snprintf(rel, sizeof rel, "Models/%s.obj", id);
    if (modFind(rel, path, sizeof path)) TraceLog(LOG_INFO, "ASSET: %s from mod (%s)", id, path);
    else {
        snprintf(rel, sizeof rel, "Data/Models/%s.obj", id);
        snprintf(path, sizeof path, "%s", P(rel));
        if (!FileExists(path)) writeAssetObj(path, id, bx, n);
    }
    Asset *a = &assets[nAsset];
    snprintf(a->id, sizeof a->id, "%s", id);
    a->m = LoadModel(path);
    if (a->m.meshCount == 0 || a->m.meshes[0].vertexCount == 0) { TraceLog(LOG_WARNING, "ASSET: failed to load %s", path); return -1; }
    assetFixMaterials(&a->m, path);
    return nAsset++;
}
static void assetDraw(int a, Matrix F, Color tint) { if (a >= 0) drawM(&assets[a].m, F, tint); }
#define ADRAW(id, F, tint, ...) do { static int _a = -2; if (_a == -2) { ABox _b[] = {__VA_ARGS__}; _a = assetGet(id, _b, (int)(sizeof _b / sizeof _b[0])); } assetDraw(_a, (F), (tint)); } while (0)
#define BX(t, ox, oy, oz, sx, sy, sz, c) {(t), {(ox), (oy), (oz)}, {(sx), (sy), (sz)}, c}
static Matrix TRS(Vector3 p, float yaw, Vector3 sc) { return MatrixMultiply(MatrixMultiply(MatrixScale(sc.x, sc.y, sc.z), MatrixRotateY(yaw)), MatrixTranslate(p.x, p.y, p.z)); }
#define CFL {205, 190, 185, 255}
#define CDK {120, 100, 95, 255}
#define CWH {255, 255, 255, 255}
static float crLook(Cr *c) {   // heads turn to follow you: slowly while they wander, snapping when they hunt
    if (c->state == CS_DEAD) return 0;
    float d = Vector3Distance(c->pos, pl.pos); if (d > 26) return 0;
    float a = wrapA(atan2f(pl.pos.x - c->pos.x, pl.pos.z - c->pos.z) - c->yaw);
    float lim = c->state == CS_CHASE ? 0.9f : 1.5f;
    return clampf(a, -lim, lim) * (c->state == CS_CHASE ? 1.0f : 0.7f);
}
#include "zk_draw.h"
#include "zk_anim.h"
static void drawCreature(Cr *c) {
    float fall = c->state == CS_DEAD ? fminf(c->deadT / 0.5f, 1) * PI * 0.5f : 0;
    float sink = c->state == CS_DEAD && c->deadT > 10 ? (c->deadT - 10) * 0.25f : 0;
    Matrix base = MatrixMultiply(MatrixMultiply(MatrixRotateZ(fall), MatrixRotateY(c->yaw)), MatrixTranslate(c->pos.x, c->pos.y - sink, c->pos.z));
    float a = c->t * (c->type == 2 ? 12 : 7);
    float *w = c->tw; bool alive = c->state != CS_DEAD;
    if (!alive) for (int k = 0; k < 6; k++) w[k] *= 0.9f;
    Color tn = c->hurt > 0.12f ? (Color){255, 255, 255, 255} : c->hurt > 0 ? (Color){255, 70, 60, 255} : WHITE;
    if (c->flinch > 0.01f) base = MatrixMultiply(MatrixMultiply(MatrixRotateX(-c->flinch * 0.55f), MatrixRotateY(c->yaw)), MatrixTranslate(c->pos.x, c->pos.y - sink, c->pos.z));
    float wind = c->wind > 0 ? 1 - c->wind / 0.38f : 0;
    if (c->type >= 5) { drawCreature2(c, base, tn, w, a, wind); return; }
    if (c->type == 0) {   // bound, faceless
        for (int s = -1; s <= 1; s += 2) { Matrix L = frameM(base, V3(0.11f * s, 0.85f, 0), sinf(a) * 0.45f * s + w[s > 0 ? 0 : 1] * 0.3f, 0, 0);
            ADRAW("creatures/bound/leg", L, tn, BX(T_FLESH, 0, -0.42f, 0, 0.15f, 0.85f, 0.15f, CFL)); }
        Matrix Tm = frameM(base, V3(0, 0.85f, 0), 0.18f + w[2] * 0.4f + wind * 0.5f, w[4] * 0.3f, w[3] * 0.3f);
        ADRAW("creatures/bound/torso", Tm, tn, BX(T_CLOTH, 0, 0.5f, 0, 0.5f, 1.0f, 0.34f, CWH), BX(T_PLAIN, 0, 0.3f, 0, 0.53f, 0.07f, 0.37f, ((Color){70, 50, 40, 255})), BX(T_PLAIN, 0, 0.7f, 0, 0.53f, 0.07f, 0.37f, ((Color){70, 50, 40, 255})));
        for (int s = -1; s <= 1; s += 2) { Matrix Am = frameM(Tm, V3(0.29f * s, 0.92f, 0), -0.15f - wind * 1.9f + sinf(a + s) * 0.2f + w[4] * 0.6f, 0, 0.1f * s + w[3] * 0.3f);   // arms that hang past the knees
            ADRAW("creatures/bound/arm", Am, tn, BX(T_FLESH, 0, -0.5f, 0, 0.08f, 1.0f, 0.08f, CFL), BX(T_FLESH, 0, -1.06f, 0.02f, 0.11f, 0.16f, 0.06f, ((Color){90, 50, 45, 255})),
                  BX(T_FLESH, -0.03f, -1.2f, 0.02f, 0.02f, 0.14f, 0.02f, ((Color){80, 45, 40, 255})), BX(T_FLESH, 0.03f, -1.21f, 0.02f, 0.02f, 0.15f, 0.02f, ((Color){80, 45, 40, 255}))); }
        Matrix Hm = MatrixMultiply(MatrixRotateY(crLook(c)), frameM(Tm, V3(0, 1.0f, 0), w[5] * 1.2f, w[0] * 1.5f, w[1] * 1.2f));
        ADRAW("creatures/bound/head", Hm, tn, BX(T_FLESH, 0, 0.17f, 0.02f, 0.27f, 0.34f, 0.27f, CFL), BX(T_CLOTH, 0, 0.22f, 0.02f, 0.3f, 0.12f, 0.3f, ((Color){150, 130, 110, 255})));
        {   float open = c->state == CS_CHASE ? 0.6f + fabsf(w[2]) * 0.6f + wind * 0.8f : 0.15f;   // the face splits open vertically
            Matrix Mo = frameM(Hm, V3(0, 0.1f, 0.14f), 0, 0, 0);
            ADRAW("creatures/bound/maw", MatrixMultiply(MatrixScale(1, 1, 1), Mo), WHITE, BX(T_PLAIN, 0, 0, 0, 0.05f + open * 0.07f, 0.2f, 0.02f, ((Color){30, 4, 4, 255})));
            if (open > 0.3f) for (int k2 = -2; k2 <= 2; k2++) { ADRAW("creatures/bound/tooth", frameM(Mo, V3(-0.03f - open * 0.03f, k2 * 0.035f, 0.012f), 0, 0, 0), WHITE, BX(T_PLAIN, 0.012f, 0, 0, 0.024f, 0.012f, 0.012f, ((Color){230, 225, 200, 255})));
                ADRAW("creatures/bound/tooth", frameM(Mo, V3(0.03f + open * 0.03f, k2 * 0.035f + 0.015f, 0.012f), 0, PI, 0), WHITE, BX(T_PLAIN, 0.012f, 0, 0, 0.024f, 0.012f, 0.012f, ((Color){230, 225, 200, 255}))); }
            if (alive) glowEyes(Hm, V3(-0.07f, 0.24f, 0.155f), V3(0.07f, 0.23f, 0.155f), c->state == CS_CHASE ? 1 : 0.6f, (Color){255, 230, 200, 255}); }
    } else if (c->type == 1) {   // tall long-neck
        for (int s = -1; s <= 1; s += 2) { Matrix L = frameM(base, V3(0.12f * s, 1.2f, 0), sinf(a * 0.7f) * 0.35f * s, 0, w[s > 0 ? 0 : 1] * 0.2f);
            ADRAW("creatures/longneck/leg", L, tn, BX(T_FLESH, 0, -0.6f, 0, 0.11f, 1.2f, 0.11f, CDK)); }
        Matrix Tm = frameM(base, V3(0, 1.2f, 0), 0.25f + wind * 0.4f, w[3] * 0.2f, w[2] * 0.2f);
        ADRAW("creatures/longneck/torso", Tm, tn, BX(T_FLESH, 0, 0.35f, 0, 0.36f, 0.7f, 0.22f, CDK));
        for (int s = -1; s <= 1; s += 2) { Matrix Am = frameM(Tm, V3(0.24f * s, 0.65f, 0), -0.2f + sinf(a * 0.5f + s) * 0.25f - wind * 1.6f, 0, 0.12f * s + w[4] * 0.3f);
            ADRAW("creatures/longneck/arm", Am, tn, BX(T_FLESH, 0, -0.6f, 0, 0.07f, 1.2f, 0.07f, CDK), BX(T_FLESH, 0, -1.25f, 0.02f, 0.13f, 0.2f, 0.09f, ((Color){80, 40, 40, 255}))); }
        Matrix Nm = frameM(Tm, V3(0, 0.7f, 0), 0.55f + w[5] * 0.6f, w[0] * 0.5f, w[1] * 0.5f);
        ADRAW("creatures/longneck/neck", Nm, tn, BX(T_FLESH, 0, 0.38f, 0, 0.08f, 0.76f, 0.08f, CFL));
        Matrix Hm = MatrixMultiply(MatrixRotateY(crLook(c) * 1.4f), frameM(Nm, V3(0, 0.76f, 0), w[2] * 2.0f, w[3] * 2.5f, w[4] * 2.0f));
        for (int k2 = -2; k2 <= 2; k2++) ADRAW("creatures/longneck/tooth", frameM(Hm, V3(k2 * 0.022f, 0.02f, 0.16f), 0, 0, 0), WHITE, BX(T_PLAIN, 0, 0, 0, 0.012f, 0.045f, 0.012f, ((Color){235, 230, 210, 255})));
        ADRAW("creatures/longneck/head", Hm, tn, BX(T_FLESH, 0, 0.12f, 0.03f, 0.2f, 0.3f, 0.24f, CFL), BX(T_PLAIN, 0, 0.04f, 0.15f, 0.12f, 0.1f, 0.02f, ((Color){20, 5, 5, 255})));
        if (alive) glowEyes(Hm, V3(-0.05f, 0.18f, 0.155f), V3(0.05f, 0.2f, 0.155f), 1, (Color){255, 60, 40, 255});
    } else if (c->type == 3) {   // mutated dog
        Matrix Rm = frameM(base, V3(0, 0.62f, 0), w[2] * 0.12f + sinf(a * 2) * 0.05f, 0, w[3] * 0.1f);
        ADRAW("creatures/dog/body", Rm, tn, BX(T_FLESH, 0, 0, 0, 0.34f, 0.32f, 1.0f, CDK), BX(T_FLESH, 0.12f, 0.18f, -0.15f, 0.22f, 0.2f, 0.3f, CFL), BX(T_FLESH, -0.1f, 0.14f, 0.2f, 0.16f, 0.16f, 0.2f, CFL));
        Matrix Hm = MatrixMultiply(MatrixRotateY(crLook(c) * 0.6f), frameM(Rm, V3(0, 0.1f, 0.5f), -0.2f + w[0] * 0.8f, w[1] * 0.8f, w[5] * 0.6f));
        ADRAW("creatures/dog/head", Hm, tn, BX(T_FLESH, 0, 0, 0.18f, 0.24f, 0.2f, 0.36f, CDK));
        if (alive) glowEyes(Hm, V3(-0.08f, 0.06f, 0.3f), V3(0.08f, 0.06f, 0.3f), 1, (Color){255, 200, 60, 255});
        Matrix J = frameM(Hm, V3(0, -0.08f, 0.05f), 0.35f + fabsf(w[4]) * 0.6f + wind * 0.6f, 0, 0);
        ADRAW("creatures/dog/jaw", J, tn, BX(T_FLESH, 0, -0.04f, 0.16f, 0.2f, 0.07f, 0.32f, ((Color){140, 40, 40, 255})));
        for (int k = 0; k < 4; k++) {
            float s = (k & 1) ? 1.0f : -1.0f, z = (k & 2) ? 0.38f : -0.38f;
            Matrix Lm = frameM(Rm, V3(0.14f * s, -0.1f, z), sinf(a * 1.4f + k * 1.9f) * 0.7f, 0, 0.05f * s);
            ADRAW("creatures/dog/leg", Lm, tn, BX(T_FLESH, 0, -0.28f, 0, 0.08f, 0.56f, 0.08f, CDK));
        }
        Matrix Tl = frameM(Rm, V3(0, 0.08f, -0.5f), -0.6f + w[2] * 0.6f, 0, 0);
        ADRAW("creatures/dog/tail", Tl, tn, BX(T_FLESH, 0, 0, -0.18f, 0.05f, 0.05f, 0.36f, CDK));
    } else if (c->type == 4) {   // yokai spirit
        if (c->state != CS_DEAD && hash2((int)(c->t * 14), (int)(c->pos.x), 8) > 0.86f) return;
        float fade = c->state == CS_DEAD ? 1 - c->deadT / 1.2f : 1;
        float bob = 0;   // update() already floats it clear of the floor
        Matrix Bm = MatrixMultiply(MatrixMultiply(MatrixScale(fade, 1, fade), MatrixRotateY(c->yaw)), MatrixTranslate(c->pos.x, c->pos.y + bob, c->pos.z));
        setEmis(1);
        for (int f = 0; f < 2; f++) {   // two hitodama: cold blue soul-flames circling it
            float ang = c->t * 1.3f + f * PI;
            Vector3 hp = V3(c->pos.x + cosf(ang) * 0.75f, c->pos.y + bob + 1.1f + sinf(c->t * 2.3f + f) * 0.2f, c->pos.z + sinf(ang) * 0.75f);
            DrawCube(hp, 0.13f, 0.13f, 0.13f, (Color){120, 200, 255, 255}); DrawCube(V3(hp.x, hp.y + 0.11f, hp.z), 0.07f, 0.12f, 0.07f, (Color){190, 235, 255, 255}); }
        Matrix Tm = frameM(Bm, V3(0, 0, 0), 0.08f + w[2] * 0.15f, 0, w[3] * 0.15f);
        ADRAW("creatures/spirit/body", Tm, WHITE, BX(T_CLOTH, 0, 0.75f, 0, 0.48f, 1.5f, 0.32f, ((Color){150, 195, 230, 255})), BX(T_CLOTH, 0, 0.08f, 0, 0.62f, 0.16f, 0.44f, ((Color){90, 150, 200, 255})));
        for (int s2 = -1; s2 <= 1; s2 += 2) { Matrix Am = frameM(Tm, V3(0.27f * s2, 1.4f, 0.02f), -0.15f + w[4] * 0.2f, 0, 0.06f * s2);
            ADRAW("creatures/spirit/arm", Am, WHITE, BX(T_CLOTH, 0, -0.4f, 0, 0.13f, 0.8f, 0.13f, ((Color){140, 185, 225, 255})), BX(T_FLESH, 0, -0.88f, 0, 0.08f, 0.16f, 0.06f, ((Color){175, 215, 240, 255}))); }
        Matrix Hm = frameM(Tm, V3(0, 1.5f, 0.02f), 0.35f + w[0] * 0.3f, w[1] * 0.4f, w[5] * 0.4f);
        ADRAW("creatures/spirit/head", Hm, WHITE, BX(T_FLESH, 0, 0.15f, 0, 0.24f, 0.3f, 0.24f, ((Color){175, 215, 240, 255})), BX(T_PLAIN, 0, 0.08f, 0.04f, 0.3f, 0.62f, 0.3f, ((Color){8, 8, 10, 255})), BX(T_PLAIN, 0, -0.15f, 0.13f, 0.26f, 0.5f, 0.06f, ((Color){8, 8, 10, 255})));
        setEmis(0);
    } else {   // crawler
        Matrix Rm = frameM(base, V3(0, 0.45f, 0), w[2] * 0.15f, 0, w[3] * 0.2f);
        ADRAW("creatures/crawler/body", Rm, tn, BX(T_FLESH, 0, 0, 0, 0.42f, 0.24f, 0.9f, CFL), BX(T_CLOTH, 0, 0.05f, -0.1f, 0.44f, 0.2f, 0.4f, CWH));
        Matrix Hm = frameM(Rm, V3(0, 0.05f, 0.48f), w[0] * 1.3f, w[1] * 1.5f, w[5] * 1.2f);
        ADRAW("creatures/crawler/head", Hm, tn, BX(T_FLESH, 0, 0.02f, 0.1f, 0.24f, 0.22f, 0.26f, CFL), BX(T_PLAIN, 0, -0.05f, 0.235f, 0.16f, 0.06f, 0.01f, ((Color){30, 5, 5, 255})));
        if (alive) glowEyes(Hm, V3(-0.06f, 0.06f, 0.235f), V3(0.06f, 0.06f, 0.235f), 1, (Color){255, 240, 220, 255});
        for (int k = 0; k < 4; k++) {
            float s = (k & 1) ? 1.0f : -1.0f, z = (k & 2) ? 0.35f : -0.35f;
            Matrix Lm = frameM(Rm, V3(0.2f * s, -0.02f, z), sinf(a + k * 1.7f) * 0.6f, 0, 0.6f * s);
            ADRAW("creatures/crawler/limb", Lm, tn, BX(T_FLESH, 0, -0.3f, 0, 0.08f, 0.6f, 0.08f, CDK));
        }
    }
}
static void drawPlanks(Vector3 mn, Vector3 mx, int axis) {
    float h = mx.y - mn.y;
    int n = (int)(h / 0.35f); if (n < 2) n = 2;
    for (int k = 0; k < n; k++) {
        float y = mn.y + (k + 0.5f) * h / n;
        Vector3 a = mn, b = mx; a.y = y - 0.1f; b.y = y + 0.1f;
        if (axis == 1) { a.x -= 0.15f; b.x += 0.15f; } else { a.z -= 0.15f; b.z += 0.15f; }
        Vector3 c = Vector3Scale(Vector3Add(a, b), 0.5f), sz = Vector3Subtract(b, a);
        float yaw = (sz.x >= sz.z) ? 0 : PI / 2, len = fmaxf(sz.x, sz.z), th = fminf(sz.x, sz.z);
        ADRAW("props/barricade_plank", TRS(c, yaw + (k % 2 ? 0.04f : -0.04f), V3(len, sz.y / 0.2f, fmaxf(th, 0.05f) / 0.06f)), ((Color){230, 210, 190, 255}), BX(T_WOOD, 0, 0, 0, 1.0f, 0.2f, 0.06f, CWH));
    }
}
static void setupLights(Vector3 eye, float t) {
    int idx[8]; float dd[8]; int n = 0;
    for (int i = 0; i < nLight; i++) {
        Light *L = &lights[i];
        if (L->mode == 2) { L->cur = 0; continue; }
        if (L->mode == 1) { float f = sinf(t * 23 + L->seed) + sinf(t * 7.3f + L->seed * 2) + (hash2((int)(t * 12), (int)L->seed, 4) > 0.8f ? -2 : 0); L->cur = f > -0.5f ? 1 : 0.15f; }
        else L->cur = 1;
        float d = Vector3DistanceSqr(L->pos, eye);
        if (d > 50 * 50 || L->cur < 0.2f) continue;
        if (n < 8) { idx[n] = i; dd[n] = d; n++; }
        else { int w = 0; for (int k = 1; k < 8; k++) if (dd[k] > dd[w]) w = k; if (d < dd[w]) { idx[w] = i; dd[w] = d; } }
    }
    Vector3 lp[8], lc[8]; float lr[8];
    for (int k = 0; k < n; k++) { Light *L = &lights[idx[k]]; lp[k] = L->pos; lc[k] = Vector3Scale(L->col, L->cur * 1.3f); lr[k] = L->range; }
    SetShaderValueV(sh, locLP, lp, SHADER_UNIFORM_VEC3, n > 0 ? n : 1);
    SetShaderValueV(sh, locLC, lc, SHADER_UNIFORM_VEC3, n > 0 ? n : 1);
    SetShaderValueV(sh, locLR, lr, SHADER_UNIFORM_FLOAT, n > 0 ? n : 1);
    SetShaderValue(sh, locNL, &n, SHADER_UNIFORM_INT);
}
static void drawWorld(Vector3 eye, float t) {
    curEmis = -1; setEmis(0);
    if (terrainOk) { terrain.transform = MatrixIdentity(); DrawModel(terrain, V3(0, 0, 0), 1, WHITE); }
    for (int i = 0; i < T_COUNT; i++) if (statOk[i]) { stat[i].transform = MatrixIdentity(); DrawModel(stat[i], V3(0, 0, 0), 1, WHITE); }
    float R2 = 62 * 62;
    for (int i = 0; i < nOps; i++) {
        Open *o = &ops[i];
        if (Vector3DistanceSqr(opCenter(o), eye) > R2) continue;
        if (o->f) xfPush(o->f);
        if (o->type == OP_WINDOW && o->state == 0) drawBox(T_GLASS, o->mn, o->mx, rgb(150, 165, 185));
        else if (o->state == 2) drawPlanks(o->mn, o->mx, o->axis);
        else if (o->type == OP_DOOR && o->state == 1) drawBox(T_WOOD, o->mn, o->mx, rgb(170, 140, 120));
        if (o->f) xfPop();
    }
    for (int i = 0; i < nCrate; i++) if (crates[i].alive && Vector3DistanceSqr(crates[i].pos, eye) < R2) {
        Crate *k = &crates[i]; ADRAW("props/crate", TRS(k->pos, 0, V3(k->s, k->s, k->s)), WHITE, BX(T_WOOD, 0, 0.5f, 0, 1, 1, 1, ((Color){200, 170, 130, 255}))); }
    for (int i = 0; i < nBuild; i++) if (builds[i].alive && Vector3DistanceSqr(builds[i].mn, eye) < R2) drawBuildPiece(&builds[i]);
    drawWater(eye, t);
    for (int i = 0; i < nCont; i++) if (conts[i].on && Vector3DistanceSqr(conts[i].pos, eye) < 46 * 46) drawCont(&conts[i], t);
    for (int i = 0; i < nDeb; i++) if (debs[i].alive && Vector3DistanceSqr(xfP(debs[i].f, debs[i].mn), eye) < R2) { if (debs[i].f) xfPush(debs[i].f); drawDebris(&debs[i]); if (debs[i].f) xfPop(); }
    for (int i = 0; i < nCar; i++) {
        Car *k = &cars[i]; if (Vector3DistanceSqr(k->pos, eye) > R2 * 1.3f) continue;
        Matrix M = TRS(k->pos, k->yaw, V3(1, 1, 1)); setEmis(0);
        if (k->kind == 0) {
            ADRAW("vehicles/kei_car/paint", M, k->paint, BX(T_METAL, 0, 0.62f, 0, 1.7f, 0.65f, 3.4f, CWH), BX(T_METAL, 0, 1.475f, -0.17f, 1.54f, 0.05f, 1.53f, CWH));
            ADRAW("vehicles/kei_car/details", M, WHITE, BX(T_GLASS, 0, 1.2f, -0.17f, 1.54f, 0.5f, 1.53f, ((Color){150, 160, 170, 255})), BX(T_RUST, 0, 0.15f, 0, 1.7f, 0.3f, 3.4f, ((Color){60, 55, 50, 255})),
                  BX(T_RUST, -0.82f, 0.28f, 1.1f, 0.16f, 0.55f, 0.6f, ((Color){60, 60, 60, 255})), BX(T_RUST, 0.82f, 0.28f, 1.1f, 0.16f, 0.55f, 0.6f, ((Color){60, 60, 60, 255})),
                  BX(T_RUST, -0.82f, 0.28f, -1.1f, 0.16f, 0.55f, 0.6f, ((Color){60, 60, 60, 255})), BX(T_RUST, 0.82f, 0.28f, -1.1f, 0.16f, 0.55f, 0.6f, ((Color){60, 60, 60, 255})));
        } else {
            ADRAW("vehicles/sedan/paint", M, k->paint, BX(T_METAL, 0, 0.62f, 0, 1.7f, 0.65f, 4.4f, CWH), BX(T_METAL, 0, 1.475f, -0.22f, 1.54f, 0.05f, 1.98f, CWH));
            ADRAW("vehicles/sedan/details", M, WHITE, BX(T_GLASS, 0, 1.2f, -0.22f, 1.54f, 0.5f, 1.98f, ((Color){150, 160, 170, 255})), BX(T_RUST, 0, 0.15f, 0, 1.7f, 0.3f, 4.4f, ((Color){60, 55, 50, 255})),
                  BX(T_RUST, -0.82f, 0.28f, 1.45f, 0.16f, 0.55f, 0.65f, ((Color){60, 60, 60, 255})), BX(T_RUST, 0.82f, 0.28f, 1.45f, 0.16f, 0.55f, 0.65f, ((Color){60, 60, 60, 255})),
                  BX(T_RUST, -0.82f, 0.28f, -1.45f, 0.16f, 0.55f, 0.65f, ((Color){60, 60, 60, 255})), BX(T_RUST, 0.82f, 0.28f, -1.45f, 0.16f, 0.55f, 0.65f, ((Color){60, 60, 60, 255})));
        }
    }
    for (int i = 0; i < nVend; i++) {
        Vend *v = &vends[i]; if (Vector3DistanceSqr(xfP(v->f, v->mn), eye) > R2) continue;
        if (v->f) xfPush(v->f);
        Vector3 vc = V3((v->mn.x + v->mx.x) / 2, v->mn.y, (v->mn.z + v->mx.z) / 2);
        float vy = v->face == 0 ? PI : v->face == 2 ? 0 : v->face == 1 ? PI / 2 : -PI / 2;
        Matrix VM = TRS(vc, vy, V3(1, 1, 1));
        setEmis(0); ADRAW("props/vending_machine", VM, v->body, BX(T_PLAIN, 0, 0.925f, 0, 1.0f, 1.85f, 0.8f, CWH), BX(T_PLAIN, 0, 1.9f, 0.05f, 1.02f, 0.1f, 0.85f, ((Color){200, 200, 205, 255})));
        float on = v->broken ? 0 : (v->light >= 0 ? lights[v->light].cur : 1);
        setEmis(on > 0.5f ? 1 : 0);
        ADRAW("props/vending_panel", VM, on > 0.5f ? WHITE : rgb(90, 90, 100), BX(T_VEND, 0, 1.0f, 0.41f, 0.9f, 1.7f, 0.03f, CWH));
        if (v->f) xfPop();
    }
    for (int i = 0; i < nSign; i++) {
        Sign *s = &signs[i]; if (Vector3DistanceSqr(xfP(s->f, s->mn), eye) > 80 * 80) continue;
        float on = s->light >= 0 ? lights[s->light].cur : 0;
        if (s->f) xfPush(s->f);
        if (s->plate) { setEmis(0); drawBox(T_PLAIN, s->mn, s->mx, s->tint); drawSignText(s, s->text == SGN_SEAL ? rgb(200, 30, 25) : rgb(235, 235, 230)); if (s->f) xfPop(); continue; }
        if (on > 0.5f) { setEmis(1); drawBox(T_PLAIN, s->mn, s->mx, (Color){(unsigned char)(s->tint.r * 0.35f), (unsigned char)(s->tint.g * 0.35f), (unsigned char)(s->tint.b * 0.35f), 255}); drawSignText(s, s->tint); }
        else { setEmis(0); drawBox(T_PLAIN, s->mn, s->mx, (Color){(unsigned char)(s->tint.r / 6), (unsigned char)(s->tint.g / 6), (unsigned char)(s->tint.b / 6), 255}); drawSignText(s, (Color){(unsigned char)(s->tint.r / 2), (unsigned char)(s->tint.g / 2), (unsigned char)(s->tint.b / 2), 255}); }
        setEmis(0); if (s->f) xfPop();
    }
    for (int i = 0; i < nLamp; i++) {
        Lamp *l = &lamps[i]; if (Vector3DistanceSqr(l->pos, eye) > 90 * 90) continue;
        float on = lights[l->light].cur; bool lit = lights[l->light].mode != 2 && on > 0.5f;
        setEmis(lit ? 1 : 0);
        drawBox(T_PLAIN, V3(l->pos.x - 0.25f, 5.95f, l->pos.z - 0.18f), V3(l->pos.x + 0.25f, 6.12f, l->pos.z + 0.18f), lit ? rgb(255, 190, 110) : rgb(60, 60, 60));
    }
    // salt circles
    setEmis(1);
    for (int w = 0; w < nWard; w++) if (wards[w].on && wards[w].kind == 0) {
        Ward *wd = &wards[w]; float a = clampf(wd->life / 20, 0.3f, 1);
        for (int k = 0; k < 28; k++) { float an = k * 2 * PI / 28; Vector3 q = V3(wd->pos.x + cosf(an) * wd->r, wd->pos.y + 0.02f, wd->pos.z + sinf(an) * wd->r);
            drawBox(T_PLAIN, V3(q.x - 0.12f, q.y, q.z - 0.12f), V3(q.x + 0.12f, q.y + 0.03f, q.z + 0.12f), rgb(200 * a + 30, 200 * a + 30, 200 * a + 30)); }
    }
    setEmis(0);
    // public-information screens + kiosks (shared broadcast feed)
    for (int i = 0; i < nScreen; i++) {
        Screen *sc = &screens[i]; if (Vector3DistanceSqr(sc->c, eye) > 85 * 85) continue;
        float on = sc->light >= 0 ? lights[sc->light].cur : 0.0f;
        bool dead = sc->light < 0;
        setEmis(dead ? 0 : 1);
        Matrix M = MatrixMultiply(MatrixMultiply(MatrixScale(sc->w, sc->h, 1), MatrixRotateY(sc->ang)), MatrixTranslate(sc->c.x, sc->c.y, sc->c.z));
        drawM(&scrQuad, M, dead ? rgb(25, 30, 32) : (on > 0.5f ? WHITE : rgb(120, 140, 150)));
        setEmis(0);
        Vector3 back = Vector3Subtract(sc->c, Vector3Scale(V3(sinf(sc->ang), 0, cosf(sc->ang)), 0.08f));
        Matrix F = MatrixMultiply(MatrixMultiply(MatrixScale(sc->w + 0.25f, sc->h + 0.25f, 0.12f), MatrixRotateY(sc->ang)), MatrixTranslate(back.x, back.y, back.z));
        drawM(&cube[T_PLAIN], F, rgb(215, 228, 232));
    }
    for (int i = 0; i < nTV; i++) { TV *tv = &tvs[i]; if (Vector3DistanceSqr(tv->pos, eye) > 40 * 40) continue;
        setEmis(0); ADRAW("props/crt_tv", TRS(V3(tv->pos.x - sinf(tv->ang) * 0.26f, tv->pos.y - 0.3f, tv->pos.z - cosf(tv->ang) * 0.26f), tv->ang, V3(1, 1, 1)), WHITE, BX(T_PLAIN, 0, 0.3f, 0, 0.8f, 0.6f, 0.5f, ((Color){60, 58, 55, 255})), BX(T_PLAIN, 0.3f, 0.12f, 0.252f, 0.08f, 0.06f, 0.01f, ((Color){150, 150, 150, 255})));
        setEmis(1); drawM(&scrQuad, MatrixMultiply(MatrixMultiply(MatrixScale(0.62f, 0.44f, 1), MatrixRotateY(tv->ang)), MatrixTranslate(tv->pos.x, tv->pos.y, tv->pos.z)), rgb(200, 210, 210)); }
    for (int i = 0; i < nFire; i++) if (fires[i].on) {
        Fire *f = &fires[i]; Vector3 c = f->pos;
        setEmis(0);
        ADRAW("props/campfire", TRS(c, 0, V3(1, 1, 1)), WHITE, BX(T_WOOD, 0, 0.07f, 0, 0.9f, 0.14f, 0.16f, ((Color){120, 80, 60, 255})), BX(T_WOOD, 0, 0.07f, 0, 0.16f, 0.14f, 0.9f, ((Color){120, 80, 60, 255})),
              BX(T_CONCRETE, 0.5f, 0.06f, 0.3f, 0.18f, 0.12f, 0.18f, CWH), BX(T_CONCRETE, -0.45f, 0.06f, -0.35f, 0.18f, 0.12f, 0.18f, CWH), BX(T_CONCRETE, -0.3f, 0.06f, 0.48f, 0.18f, 0.12f, 0.18f, CWH));
        setEmis(1);
        for (int k = 0; k < 4; k++) { float h = 0.25f + 0.25f * fabsf(sinf(t * (7 + k) + k)), w2 = 0.36f - k * 0.06f, ox = sinf(t * 5 + k * 2) * 0.06f;
            ADRAW("props/campfire_flame", TRS(V3(c.x + ox, c.y + 0.12f + k * 0.08f, c.z), t * 2 + k, V3(w2, h, w2)), k < 2 ? rgb(255, 150, 40) : rgb(255, 230, 120), BX(T_PLAIN, 0, 0.5f, 0, 1, 1, 1, CWH)); }
    }
    setEmis(0);
    setEmis(0);
    for (int i = 0; i < nCr; i++) if (crs[i].on && Vector3DistanceSqr(crs[i].pos, eye) < 70 * 70) {
        Cr *c = &crs[i];
        if (state == ST_PLAY && pl.stage == 3 && !c->phantom && c->state != CS_DEAD && c->revealT <= 0) {   // insanity: real monsters unseen until they strike
            if (pl.corrupt > 30 || hash2((int)(t * 6), i, 21) > 0.5f) continue;
        }
        drawCreature(c);
    }
    for (int i = 0; i < MAX_PART; i++) if (parts[i].life > 0) { Part *p = &parts[i]; float s = p->size;
        drawBox(p->t, V3(p->pos.x - s, p->pos.y - s, p->pos.z - s), V3(p->pos.x + s, p->pos.y + s, p->pos.z + s), p->tint); }
    setEmis(1);
    for (int i = 0; i < MAX_ASH; i++) { Vector3 a = ash[i]; drawBox(T_PLAIN, V3(a.x - 0.025f, a.y - 0.025f, a.z - 0.025f), V3(a.x + 0.025f, a.y + 0.025f, a.z + 0.025f), rgb(150, 150, 150)); }
    setEmis(0);
}
// make sure every .obj asset exists on disk (and is loaded) before play
static void prewarmAssets(void) {
    RenderTexture2D d = LoadRenderTexture(8, 8);
    Camera3D c3 = {{0, 2, -5}, {0, 1, 0}, {0, 1, 0}, 60, CAMERA_PERSPECTIVE};
    BeginTextureMode(d); BeginMode3D(c3);
    Cr c = {0}; c.on = true; c.state = CS_DEAD;
    for (int t = 0; t < 10; t++) { c.type = t; drawCreature(&c); }
    c.state = CS_WANDER; for (int t = 0; t < 10; t++) { c.type = t; drawCreature(&c); }
    for (int t = 0; t < IT_N; t++) drawItemType(t, MatrixIdentity());
    Player keep = pl;
    pl.build = true; drawViewmodel(V3(0, 1, 0), V3(0, 0, 1)); pl.build = false;
    pl.hands.t = IT_REVOLVER; pl.muzzle = 1; drawViewmodel(V3(0, 1, 0), V3(0, 0, 1));
    int melee[] = {-1, IT_PIPE, IT_AXE, IT_KATANA, IT_BAT, IT_CROWBAR, IT_KNIFE, IT_SLEDGE, IT_HAMMER};
    for (int m = 0; m < 9; m++) { pl.hands.t = (short)melee[m]; drawViewmodel(V3(0, 1, 0), V3(0, 0, 1)); }
    pl = keep;
    drawPlanks(V3(0, 0, 0), V3(1, 1, 0.1f), 1);
    crates[0] = (Crate){V3(0, 0, 0), 1, 1, -1, true}; nCrate = 1;
    for (int k2 = 0; k2 < BK_N; k2++) { Build b = {0}; b.mn = V3(0, 0, 0); b.mx = V3(2, 2.2f, 0.2f); b.kind = k2; b.hp = b.maxhp = 1; b.alive = true; drawBuildPiece(&b); }
    for (int k2 = 0; k2 < CK_N; k2++) { Cont ct = {0}; ct.kind = k2; ct.on = true; slotsClear(ct.s, CSLOTS); drawCont(&ct, 0); }
    { Debris db = {V3(0, 0, 0), V3(1.2f, 1.1f, 1.2f), -1, 1, true, 0}; drawDebris(&db); }
    vends[0] = (Vend){V3(0, 0, 0), V3(1, 1.85f, 0.8f), 2, -1, -1, 3, false, WHITE}; nVend = 1;
    fires[0] = (Fire){V3(0, 0, 0), 10, -1, true}; nFire = 1;
    tvs[0] = (TV){V3(0, 0.8f, 0), 0}; nTV = 1;
    cars[0] = (Car){V3(0, 0, 0), 0, 0, WHITE}; cars[1] = (Car){V3(0, 0, 0), 0, 1, WHITE}; nCar = 2;
    drawWorld(V3(0, 1, -2), 0);
    nCrate = nBuild = nVend = nFire = nTV = nCar = 0;
    EndMode3D(); EndTextureMode();
    UnloadRenderTexture(d);
}
static void updateFX(float dt, Vector3 eye) {
    for (int i = 0; i < MAX_PART; i++) if (parts[i].life > 0) {
        Part *p = &parts[i]; p->life -= dt; p->vel.y -= 9.8f * dt; p->pos = Vector3Add(p->pos, Vector3Scale(p->vel, dt));
        if (p->pos.y < 0.02f) { p->pos.y = 0.02f; p->vel = Vector3Scale(p->vel, 0.3f); }
    }
    static bool init = false;
    for (int i = 0; i < MAX_ASH; i++) {
        Vector3 *a = &ash[i];
        if (!init || a->y < eye.y - 3 || fabsf(a->x - eye.x) > 14 || fabsf(a->z - eye.z) > 14) *a = V3(eye.x + fr(-14, 14), eye.y + fr(init ? 6 : -3, 9), eye.z + fr(-14, 14));
        a->y -= dt * 0.55f; a->x += sinf((float)GetTime() * 0.7f + i) * dt * 0.3f;
    }
    init = true;
}

// ============================================================ UI
static float sw, shh;
#include "zk_ui.h"
#include "zk_cheat.h"
static int menuPanel(const char *title, const char **labels, int n, bool *enabled) {
    Rectangle lg = p3Panel(0, -16, 16.5f, 560, 140, false);
    aLogo(lg.width / 2, 22 * aU, 62, "A F T E R G L O W");
    p3End(0);
    float step = n > 5 ? 52 : 62, bh = n > 5 ? 40 : 46;
    Rectangle p = p3Panel(1, -16, n > 5 ? -4.5f : -2, 380, 66 + n * step, true);
    aWindow(p, title);
    int res = -1;
    for (int i = 0; i < n; i++) {
        Rectangle r = {p.x + 40 * aU, p.y + (56 + i * step) * aU, 300 * aU, bh * aU};
        if (aButton(r, labels[i], enabled ? enabled[i] : true)) res = i;
    }
    p3End(1);
    return res;
}
static void profilePanel(void) {
    float s;
    Rectangle p = p3Panel(2, 19, 1, 360, 330, true); s = aU;
    aWindow(p, "Seiry\xC5\xAB - Profile");
    char b[96];
    snprintf(b, sizeof b, "%s", prof.name); aText(b, p.x + 24 * s, p.y + 54 * s, 20, (Color){236, 230, 216, 255}, true);
    const char *lab[5] = {"Kills", "Deaths", "Runs", "Best survival", "Playtime"};
    char val[5][32];
    snprintf(val[0], 32, "%d", prof.kills); snprintf(val[1], 32, "%d", prof.deaths); snprintf(val[2], 32, "%d", prof.sessions);
    snprintf(val[3], 32, "%02d:%02d", (int)prof.best / 60, (int)prof.best % 60); snprintf(val[4], 32, "%.1f h", prof.playtime / 3600.0f);
    for (int i = 0; i < 5; i++) { aText(lab[i], p.x + 24 * s, p.y + (94 + i * 26) * s, 15, C_BONE2, false); Vector2 m = aMeasure(val[i], 15, false); aText(val[i], p.x + p.width - 28 * s - m.x, p.y + (94 + i * 26) * s, 15, C_CYAN, false); }
    if (gameActive) { const char *st = pl.stage == 0 ? "Sanity: clear" : pl.stage == 1 ? "Sanity: Sadness" : pl.stage == 2 ? "Sanity: Depression" : "Sanity: Insanity"; aText(st, p.x + 24 * s, p.y + 228 * s, 13, C_TALLOW, false); }
    aText("Character: Nanashi no Gonbei", p.x + 24 * s, p.y + 250 * s, 13, (Color){150, 146, 136, 230}, false);
    char mb2[200] = "Mods: none";
    if (noMods) snprintf(mb2, sizeof mb2, "Mods: off (--nomods)");
    else if (nMods > 0) { snprintf(mb2, sizeof mb2, "Mods (%d): ", nMods); for (int i = 0; i < nMods && i < 3; i++) { strncat(mb2, modName[i], 30); if (i < nMods - 1 && i < 2) strcat(mb2, ", "); } }
    aText(mb2, p.x + 24 * s, p.y + 274 * s, 13, C_BONE2, false);
    aText("v" GAME_VERSION, p.x + 24 * s, p.y + 296 * s, 12, (Color){150, 146, 136, 200}, false);
    p3End(2);
}
static void settingsPanel(void) {
    Rectangle p = p3Panel(3, 0, 0, 520, 520, true);
    if (aWindow(p, "Settings")) state = settingsBack;
    float x = p.x + 40 * aU, w = 440 * aU, y = p.y + 90 * aU;
    cfg.sens = aSlider((Rectangle){x, y, w, 10 * aU}, "Mouse sensitivity", cfg.sens * 1000, 0.5f, 6, "%.1f") / 1000; y += 58 * aU;
    cfg.aimLag = aSlider((Rectangle){x, y, w, 10 * aU}, "Aim lag (lower = heavier view lag)", cfg.aimLag, 2, 16, "%.1f"); y += 58 * aU;
    cfg.fov = aSlider((Rectangle){x, y, w, 10 * aU}, "Field of view", cfg.fov, 55, 95, "%.0f"); y += 58 * aU;
    cfg.volume = aSlider((Rectangle){x, y, w, 10 * aU}, "Master volume", cfg.volume, 0, 1, "%.2f"); y += 58 * aU;
    cfg.music = aSlider((Rectangle){x, y, w, 10 * aU}, "Music volume", cfg.music, 0, 1, "%.2f"); y += 58 * aU;
    float rh = (float)cfg.renderH; rh = aSlider((Rectangle){x, y, w, 10 * aU}, "Render resolution (PS1 = 240)", rh, 180, 480, "%.0fp"); cfg.renderH = ((int)rh / 30) * 30; y += 46 * aU;
    bool fs = aToggle(x, y, "Fullscreen (borderless)", cfg.fullscreen);
    if (fs != (bool)cfg.fullscreen) { cfg.fullscreen = fs; ToggleBorderlessWindowed(); }
    y += 50 * aU;
    if (aButton((Rectangle){p.width / 2 - 110 * aU, y, 220 * aU, 44 * aU}, "Save & Back", true)) { cfgSave(); SetMasterVolume(cfg.volume); state = settingsBack; }
    p3End(3);
}
static void controlsPanel(void) {
    Rectangle p = p3Panel(3, 0, 0, 640, 752, true);
    if (aWindow(p, "Controls")) state = settingsBack;
    const char *l[] = {"Mouse", "Aim (crosshair leads, view follows)", "WASD", "Move (heavy loads slow you down)", "Shift / Ctrl", "Sprint / Sneak", "Space", "Jump - Climb ledges - Vault broken windows",
                       "LMB", "Swing / Fire / Place build piece", "Hold RMB", "Guard with melee or fists (tap just before a hit: parry)", "I", "Inspect the item in hand", "Hold E", "Search furniture - E: doors, TVs, water, gardens", "Tab", "Inventory + crafting: drag, right-click to use / equip",
                       "1 - 6", "Quickbar (weapons, food, bandages)", "H", "Put the weapon away (fists)", "G", "Barricade window or doorway - Repair - Reinforce",
                       "B", "Build mode (wheel / R choose piece)", "R", "Reload", "F", "Flashlight", "Z", "Sleep (bed, or roof + fire)",
                       "M", "Map: LMB marker - RMB drag rotate / click remove - wheel zoom", "Kamidana", "Build it first: wards only work inside your base",
                       "Saving", "Automatic: sleep, every 2 min, back to menu", "Esc", "Pause"};
    for (int i = 0; i < 20; i++) {
        aText(l[i * 2], p.x + 36 * aU, p.y + (60 + i * 31) * aU, 15, C_CYAN, true);
        aText(l[i * 2 + 1], p.x + 150 * aU, p.y + (60 + i * 31) * aU, 15, C_BONE, false);
    }
    if (aButton((Rectangle){p.width / 2 - 100 * aU, p.y + p.height - 62 * aU, 200 * aU, 42 * aU}, "Back", true)) state = settingsBack;
    p3End(3);
}

static void lorePanel(void) {
    Rectangle p = p3Panel(3, 0, 0, 680, 600, true);
    if (aWindow(p, "Afterglow  -  Japan, 2006")) state = ST_MENU;
    const char *t[] = {
        "Before the disaster, Japan followed a Frutiger Aero ideal: glass eco-towers,",
        "clean-water parks, cheerful public-information screens, and a government",
        "promise of a \"bright, green future.\"", "",
        "Fearing Japan was helping the United States gain control of the seas near",
        "China and North Korea, Russia detonated a nuclear device over Japan.", "",
        "All of Japan is sealed off from the outside world. US bases, including those",
        "in Okinawa, are under the strictest lockdown. No rescue arrives.", "",
        "The fallout mutated wildlife, humans and spirits. Every mutated being",
        "hungers for human blood. The yokai, who waited centuries to reclaim Japan,",
        "have begun to attack.", "",
        "Mutants fall when the body is destroyed. Spirits answer only to the old ways:",
        "ofuda, shimenawa and salt.", "",
        "Few humans remain. No factions have formed. You are Nanashi no Gonbei.",
        "There is no story but the one you make."};
    for (int i = 0; i < 19; i++) aText(t[i], p.x + 40 * aU, p.y + (62 + i * 24) * aU, 16, i >= 17 ? C_CYAN : C_BONE, false);
    if (aButton((Rectangle){p.width / 2 - 100 * aU, p.y + p.height - 60 * aU, 200 * aU, 42 * aU}, "Back", true)) state = ST_MENU;
    p3End(3);
}

// ============================================================ scripted demo recorder (KIRI_VIDEO=<dir>)
static bool videoMode = false; static const char *vidDir = NULL;
static int vPhase = 0, vT = 0, vWin = -1, vCr = -1, vSp = -1, vItem = -1; static Vector3 vOut, vIn;
static void vAim(Vector3 target, float rate) {
    Vector3 eye = Vector3Add(pl.pos, V3(0, 1.6f, 0)), d = Vector3Subtract(target, eye);
    float yaw = atan2f(d.x, d.z), pitch = atan2f(d.y, sqrtf(d.x * d.x + d.z * d.z));
    vin.md.x = -wrapA(yaw - pl.tyaw) / cfg.sens * rate; vin.md.y = -(pitch - pl.tpitch) / cfg.sens * rate;
}
static void vPress(int k) { vin.pressed[k] = 1; }
static void vSetupScene(void) {
    // choose a ground-floor window with clear space outside and inside
    for (int i = 0; i < nOps; i++) {
        Open *o = &ops[i]; if (o->type != OP_WINDOW || o->state != 0 || o->mx.y > 3.0f) continue;
        Vector3 c = opCenter(o), n = opNormal(o);
        Vector3 out = Vector3Add(V3(c.x, 0, c.z), Vector3Scale(n, 7)), in = Vector3Subtract(V3(c.x, 0, c.z), Vector3Scale(n, 1.8f));
        if (Vector3Distance(out, V3(30, 0, 30)) > 140) continue;
        if (!pointFree(out.x, out.z, 1.2f, 0.2f) || !pointFree(in.x, in.z, 0.6f, SIDEH + 0.05f)) continue;
        Vector3 side = V3(n.z, 0, -n.x);
        Vector3 cpos = Vector3Add(out, Vector3Scale(side, 9));
        if (!pointFree(cpos.x, cpos.z, 0.6f, 0.2f)) continue;
        vWin = i; vOut = out; vIn = in; break;
    }
    TraceLog(LOG_INFO, "VIDEO: scene window %d of %d", vWin, nOps);
    if (vWin < 0) return;
    Open *o = &ops[vWin]; Vector3 c = opCenter(o), n = opNormal(o);
    for (int i = 0; i < nCr; i++) crs[i].on = false;
    pl.pos = V3(vOut.x, groundAt(V3(vOut.x, 1, vOut.z), 0.3f, 1), vOut.z);
    pl.tyaw = pl.yaw = atan2f(c.x - pl.pos.x, c.z - pl.pos.z) + 0.6f; pl.tpitch = pl.pitch = 0;
    invAdd(IT_PLANKS, 6); invAdd(IT_OFUDA, 1); invAdd(IT_RAMEN, 1); invAdd(IT_PIPE, 1); pl.sanity = 62;
    Vector3 side = V3(n.z, 0, -n.x);
    spawnCreatureT(pl.pos, 0.01f, 0.02f, false, 0); vCr = 0;
    crs[0].pos = Vector3Add(V3(vOut.x, 0, vOut.z), Vector3Scale(side, 10)); crs[0].pos.y = groundAt(crs[0].pos, 0.3f, 1);
    crs[0].state = CS_WANDER; crs[0].spd = 3.0f;
    addItem(V3(vIn.x - n.x * 0.6f + side.x * 0.8f, SIDEH, vIn.z - n.z * 0.6f + side.z * 0.8f), IT_MANGA, 1); vItem = nItem - 1;
}
static void videoScript(void) {
    memset(vin.down, 0, sizeof vin.down); memset(vin.pressed, 0, sizeof vin.pressed);
    vin.lmb = vin.lmbP = false; vin.md = (Vector2){0, 0}; vin.wheel = 0; vidPress = false;
    vT++;
    if (state != ST_PLAY) {   // main menu: move cursor to "New Game" and click
        float step = 52, bh = 40;
        Rectangle p = {sw * 0.27f - 190 * aU, shh * 0.27f, 380 * aU, 0};
        Vector2 target = {p.x + 190 * aU, p.y + (54 + 1 * step + bh / 2) * aU};
        Vector2 start = {sw * 0.75f, shh * 0.85f};
        float k = clampf((vT - 20) / 50.0f, 0, 1); k = k * k * (3 - 2 * k);
        vidMouse = Vector2Lerp(start, target, k);
        if (vT == 85) vidPress = true;
        return;
    }
    if (vWin < 0) return;
    Open *o = &ops[vWin]; Vector3 wc = opCenter(o);
    Cr *c = vCr >= 0 ? &crs[vCr] : NULL;
    switch (vPhase) {
    case 0:   // take in the street, then notice the creature
        vAim(Vector3Add(wc, V3(sinf(vT * 0.03f) * 4, 0.3f, 0)), 0.04f);
        if (vT > 70) { c->state = CS_CHASE; vPhase = 1; vT = 0; }
        break;
    case 1:   // turn to the creature, wait for it, swing until it drops
        vAim(crCenter(c), 0.12f);
        if (c->state != CS_DEAD && Vector3Distance(c->pos, pl.pos) < 2.4f && vT % 10 == 0) vin.lmb = true;
        if (c->state == CS_DEAD && vT > 20) { vPhase = 2; vT = 0; }
        if (vT > 400) { c->state = CS_DEAD; c->deadT = 0; }
        break;
    case 2:   // walk to the window and break the glass
        vAim(wc, 0.1f);
        if (vT > 40) {
            float d = Vector3Distance(V3(wc.x, 0, wc.z), V3(pl.pos.x, 0, pl.pos.z));
            if (d > 1.25f) vin.down[KEY_W] = 1;
            else if (o->state == 0) { if (vT % 18 == 0) vin.lmb = true; }
            else { vPhase = 3; vT = 0; }
        }
        break;
    case 3:   // vault through
        vAim(Vector3Add(wc, V3(0, -0.2f, 0)), 0.1f);
        if (vT == 25) vPress(KEY_SPACE);
        if (vT > 55) { vPhase = 4; vT = 0; }
        break;
    case 4:   // turn back, barricade the window
        vAim(wc, 0.09f);
        if (vT > 45 && vT < 60 && Vector3Distance(V3(wc.x, 0, wc.z), V3(pl.pos.x, 0, pl.pos.z)) > 1.5f) vin.down[KEY_W] = 1;
        if (vT == 70) vPress(KEY_G);
        if (vT > 110) { vPhase = 5; vT = 0; }
        break;
    case 5: {   // pick up the manga, read it
        Item *it = &items[vItem];
        if (it->on) {
            vAim(it->pos, 0.1f);
            if (vT > 30 && Vector3Distance(V3(it->pos.x, 0, it->pos.z), V3(pl.pos.x, 0, pl.pos.z)) > 1.2f) vin.down[KEY_W] = 1;
            if (vT > 30 && Vector3Distance(V3(it->pos.x, 0, it->pos.z), V3(pl.pos.x, 0, pl.pos.z)) < 1.6f && vT % 15 == 0) vPress(KEY_E);
        } else if (pl.watchT <= 0 && vT < 400) { vPress(KEY_NINE); vT = 400; }
        if (vT > 400 && pl.watchT <= 0) { vPhase = 6; vT = 0;
            Vector3 n = opNormal(o); spawnCreatureT(pl.pos, 0.01f, 0.02f, true, 4);
            for (int i = 0; i < nCr; i++) if (crs[i].on && crs[i].type == 4) { vSp = i; crs[i].pos = Vector3Add(V3(wc.x, pl.pos.y + 0.3f, wc.z), Vector3Scale(n, 7)); } }
    } break;
    case 6: {   // a spirit drifts straight through the barricade — seal it with an ofuda
        Cr *sp = &crs[vSp];
        vAim(crCenter(sp), 0.1f);
        if (sp->state != CS_DEAD && Vector3Distance(sp->pos, pl.pos) < 4.2f && vT > 60) { vPress(KEY_SEVEN); }
        if (sp->state == CS_DEAD && vT > 0) { vPhase = 7; vT = 0; }
        if (vT > 360) { sp->state = CS_DEAD; sp->deadT = 0; }
    } break;
    case 7:   // build a campfire, eat a hot meal
        vAim(Vector3Add(pl.pos, V3(sinf(pl.tyaw) * 2, 0.4f, cosf(pl.tyaw) * 2)), 0.05f);
        if (vT == 20) vPress(KEY_B);
        if (vT == 30 || vT == 38 || vT == 46) vin.wheel = 1;
        if (vT == 60) { vin.lmbP = true; vin.lmb = true; }
        if (vT == 70) vPress(KEY_B);
        if (vT == 90) vPress(KEY_ZERO);
        if (vT > 210) { vPhase = 8; vT = 0; }
        break;
    default: break;
    }
}
static void videoCapture(int frame) {
    Image im = LoadImageFromScreen();
    char fn[600]; snprintf(fn, sizeof fn, "%s/g_%05d.png", vidDir, frame);
    ExportImage(im, fn); UnloadImage(im);
}

// ============================================================ main
#if defined(_WIN32)
__declspec(dllimport) void *__stdcall GetCurrentProcess(void);
__declspec(dllimport) int __stdcall SetProcessWorkingSetSize(void *, size_t, size_t);
#endif
static FILE *gLog = NULL;
static void logToFile(int level, const char *text, va_list args) {
    if (!gLog) return;
    const char *lv = level == LOG_ERROR ? "ERROR" : level == LOG_WARNING ? "WARN " : level == LOG_INFO ? "INFO " : "DEBUG";
    fprintf(gLog, "[%s] ", lv); vfprintf(gLog, text, args); fputc('\n', gLog); fflush(gLog);
}
// ---- ambient music: three synthesised loops (Data/Audio), crossfaded by where you are
static Music mus[3]; static bool musOk[3]; static float musVol[3];
static void musicInit(void) {
    const char *nm[3] = {"Data/Audio/music_city.ogg", "Data/Audio/music_hills.ogg", "Data/Audio/music_under.ogg"};
    for (int i = 0; i < 3; i++) { if (!audioOk || !FileExists(P(nm[i]))) continue; mus[i] = LoadMusicStream(P(nm[i])); musOk[i] = mus[i].frameCount > 0;
        if (musOk[i]) { mus[i].looping = true; SetMusicVolume(mus[i], 0); PlayMusicStream(mus[i]); } }
}
static void musicUpdate(float dt, bool inGame) {
    float tgt[3] = {0, 0, 0};
    if (!inGame) tgt[0] = 1;
    else if (pl.pos.y < -40 || inBore(pl.pos.x, pl.pos.z, pl.pos.y)) tgt[2] = 1;
    else { float wk = wildK(pl.pos); tgt[1] = clampf(wk * 1.6f, 0, 1); tgt[0] = 1 - tgt[1]; }
    float duck = 1;
    if (inGame) for (int i = 0; i < nCr; i++) if (crs[i].on && crs[i].state == CS_CHASE && !crs[i].phantom && Vector3Distance(crs[i].pos, pl.pos) < 25) { duck = 0.35f; break; }   // chase sounds take over
    for (int i = 0; i < 3; i++) { if (!musOk[i]) continue;
        musVol[i] += (tgt[i] * duck - musVol[i]) * fminf(1, dt * 0.5f);
        SetMusicVolume(mus[i], musVol[i] * cfg.music * 0.55f);
        if (musVol[i] > 0.002f || tgt[i] > 0) UpdateMusicStream(mus[i]); }
}
#if defined(PLATFORM_WEB)
static int zankoMain(int argc, char **argv);
int main(void) { static char *av[] = {"zanko", NULL}; return zankoMain(1, av); }   // this toolchain only links an argument-less main
static int zankoMain(int argc, char **argv) {
#else
int main(int argc, char **argv) {
#endif
    aeroInitPaths(); cfgLoad();
    char user[32] = "Guest";
    // ---- startup arguments (set in the launcher: Settings > Startup arguments)
    int ramMB = 0, fpsCap = 0; bool argNewGame = false, argNoVsync = false; int argFull = -1;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i], *nx = i + 1 < argc ? argv[i + 1] : NULL;
        if (!strcmp(a, "--user") && nx) { strncpy(user, nx, 31); user[31] = 0; i++; }
        else if (!strcmp(a, "--ram") && nx) { ramMB = atoi(nx); i++; }
        else if (!strcmp(a, "--windowed")) argFull = 0;
        else if (!strcmp(a, "--fullscreen")) argFull = 1;
        else if (!strcmp(a, "--nomods")) noMods = true;
        else if (!strcmp(a, "--res") && nx) { int w = 0, h = 0; if (sscanf(nx, "%dx%d", &w, &h) == 2 && w >= 640 && h >= 360) { cfg.resW = w; cfg.resH = h; } i++; }
        else if (!strcmp(a, "--render") && nx) { int r = atoi(nx); if (r >= 120 && r <= 1080) cfg.renderH = r; i++; }
        else if (!strcmp(a, "--fov") && nx) { float f = (float)atof(nx); if (f >= 40 && f <= 120) cfg.fov = f; i++; }
        else if (!strcmp(a, "--seed") && nx) { gSeedArg = (unsigned int)strtoul(nx, NULL, 10); i++; }
        else if (!strcmp(a, "--novsync")) argNoVsync = true;
        else if (!strcmp(a, "--fps") && nx) { fpsCap = atoi(nx); i++; }
        else if (!strcmp(a, "--newgame")) argNewGame = true;
        else if (!strcmp(a, "--log")) { gLog = fopen(P("Data/log.txt"), "w"); if (gLog) SetTraceLogCallback(logToFile); }
    }
    if (argFull >= 0) cfg.fullscreen = argFull;
    if (argNoVsync) cfg.vsync = 0;
    if (ramMB >= 256) {   // memory allocation from the launcher: reserve a resident working set for the game process
#if defined(_WIN32)
        size_t mx = (size_t)ramMB * 1024u * 1024u, mn = mx / 2;
        if (SetProcessWorkingSetSize(GetCurrentProcess(), mn, mx)) TraceLog(LOG_INFO, "MEM: working set %d-%d MB", ramMB / 2, ramMB);
        else TraceLog(LOG_WARNING, "MEM: could not set working set to %d MB", ramMB);
#else
        TraceLog(LOG_INFO, "MEM: --ram %d ignored on this platform", ramMB);
#endif
    }
    bool shotMode = getenv("KIRI_SHOT") != NULL;
    vidDir = getenv("KIRI_VIDEO"); videoMode = vidDir != NULL; vin.on = videoMode; vidOn = videoMode;
    unsigned int flags = FLAG_WINDOW_RESIZABLE; if (cfg.vsync) flags |= FLAG_VSYNC_HINT;
    SetConfigFlags(flags);
    InitWindow(cfg.resW, cfg.resH, "Zank\xC5\x8D 2006");
    rlSetClipPlanes(0.08, 700.0);   // tighter near plane: far better depth precision (no flickering ground)
    SetWindowMinSize(800, 450);
    if (FileExists(P("Data/UI/icon.png"))) { Image ico = LoadImage(P("Data/UI/icon.png")); if (ico.data) { SetWindowIcon(ico); UnloadImage(ico); } }
    SetExitKey(KEY_NULL);
    if (!cfg.vsync) SetTargetFPS(fpsCap > 0 ? fpsCap : 144); else if (fpsCap > 0) SetTargetFPS(fpsCap);
#if !defined(PLATFORM_WEB)
    if (cfg.fullscreen) ToggleBorderlessWindowed();
#endif
    InitAudioDevice(); SetMasterVolume(cfg.volume);
    genSounds(); musicInit(); textInit();
    aeroLoadFont();
    profLoad(user);
    loadMods(); cheatsLoad();
    loadTextures();
    sh = LoadShaderFromMemory(VS, FS);
    locView = GetShaderLocation(sh, "viewPos"); locFlashDir = GetShaderLocation(sh, "flashDir"); locFlashOn = GetShaderLocation(sh, "flashOn");
    locMuzzle = GetShaderLocation(sh, "muzzle"); locFog = GetShaderLocation(sh, "fogColor"); locFogD = GetShaderLocation(sh, "fogDensity");
    locEmis = GetShaderLocation(sh, "emissive"); locAmb = GetShaderLocation(sh, "ambient"); locLP = GetShaderLocation(sh, "lightPos");
    locLC = GetShaderLocation(sh, "lightCol"); locLR = GetShaderLocation(sh, "lightRange"); locNL = GetShaderLocation(sh, "numLights"); locSnap = GetShaderLocation(sh, "snapRes");
    locDesat = GetShaderLocation(sh, "desat"); locRage = GetShaderLocation(sh, "rage");
    for (int i = 0; i < T_COUNT; i++) { cube[i] = LoadModelFromMesh(GenMeshCube(1, 1, 1)); cube[i].materials[0].shader = sh; cube[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex[i]; }
    scrRT = LoadRenderTexture(160, 96); SetTextureFilter(scrRT.texture, TEXTURE_FILTER_POINT);
    scrQuad = LoadModelFromMesh(genQuad()); scrQuad.materials[0].shader = sh; scrQuad.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = scrRT.texture;
    cyl = LoadModelFromMesh(GenMeshCylinder(1, 1, 8)); cyl.materials[0].shader = sh; cyl.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex[T_PLAIN];
    resetPlayer();
    prewarmAssets();
    if (getenv("KIRI_EXPORT")) { CloseAudioDevice(); CloseWindow(); return 0; }   // asset export only
    genWorld(12345);   // backdrop city for pause / death screens
    saInit(P("Textures/ecoglass.png"), P("Textures/facade_a.png"));
    int rtH = 0, rtW = 0; RenderTexture2D rt = {0};
    float fogc[3] = {0.075f, 0.08f, 0.095f}; Color fogC = {19, 20, 24, 255};
    SetShaderValue(sh, locFog, fogc, SHADER_UNIFORM_VEC3);
    iconsInit();
    int frame = 0; bool quitReq = false; int stTest = -1; int pauseFrame = -1;
    if (argNewGame) { newGame(); state = ST_PLAY; DisableCursor(); }
    while (!WindowShouldClose()) {
        frame++;
        float dt = fminf(GetFrameTime(), 0.05f), t = (float)GetTime();
        musicUpdate(dt, gameActive && (state == ST_PLAY || state == ST_PAUSE));
        static float vtime = 0;
        if (videoMode) { dt = 1.0f / 30.0f; vtime += dt; t = vtime; videoScript(); if (vPhase >= 8 && vT > 40) break; }
        sw = (float)GetScreenWidth(); shh = (float)GetScreenHeight();
        int wantH = cfg.renderH, wantW = (int)(wantH * sw / shh);
        if (wantH != rtH || wantW != rtW) { if (rtH) UnloadRenderTexture(rt); rt = LoadRenderTexture(wantW, wantH); SetTextureFilter(rt.texture, TEXTURE_FILTER_POINT); rtW = wantW; rtH = wantH; }
        aeroBegin(720);
        if (msgT > 0) msgT -= dt;

        // ---- debug screenshot automation
        if (shotMode) {
            if (frame == 2) { newGame(); state = ST_MENU; gameActive = false;
                int tested = 0, blocked = 0;   // sight through thin walls must be blocked
                for (int i = 0; i < nSol && tested < 300; i++) { Solid *s = &sol[i]; Vector3 sz = Vector3Subtract(s->mx, s->mn);
                    if (s->off || sz.y < 2.5f || sz.y > 3.5f) continue;
                    bool thinX = sz.x < 0.35f && sz.z > 1.5f, thinZ = sz.z < 0.35f && sz.x > 1.5f; if (!thinX && !thinZ) continue;
                    Vector3 c = Vector3Scale(Vector3Add(s->mn, s->mx), 0.5f);
                    Vector3 a = thinX ? V3(s->mn.x - 0.5f, c.y, c.z) : V3(c.x, c.y, s->mn.z - 0.5f), b = thinX ? V3(s->mx.x + 0.5f, c.y, c.z) : V3(c.x, c.y, s->mx.z + 0.5f);
                    a = xfP(s->f, a); b = xfP(s->f, b); tested++; if (!lineOfSight(a, b)) blocked++; }   // (turned buildings too)
                TraceLog(LOG_INFO, "LOSTEST: %d of %d thin walls block sight", blocked, tested); }
            if (frame == 39) gameActive = true;
            if (frame == 38) TraceLog(LOG_INFO, "EXTTEST %d street ends carried to a main road", nExt);
            if (frame == 38 && getenv("KIRI_TOWNDUMP")) {   // town coverage dump: 2 m cells
                FILE *f = fopen("town.ppm", "wb"); int N = HF_N; fprintf(f, "P6 %d %d 255\n", N, N);
                for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) { float x = HF_X0 + i * HF_S + 1, z = HF_X0 + j * HF_S + 1, h = terrainH(x, z); int hv = (int)clampf(60 + h * 6, 30, 200); unsigned char c[3] = {20, (unsigned char)hv, 30};
                    if (seaDepth(x, z) > 0) { c[0] = 30; c[1] = 70; c[2] = 140; }
                    if (inCore(x, z)) { c[0] = 190; c[1] = 186; c[2] = 176; }
                    bool rd = mainRoadAt(x, z, 0);
                    if (rd) { c[0] = 60; c[1] = 60; c[2] = 64; if (!inCore(x, z)) { c[0] = 255; c[1] = 0; c[2] = 255; } }
                    for (int m = 0; m < nMapFoot; m++) { Rectangle r = mapFoot[m]; if (x >= r.x && x <= r.x + r.width && z >= r.y && z <= r.y + r.height) { c[0] = 170; c[1] = 90; c[2] = 60; if (rd) { c[0] = 255; c[1] = 255; c[2] = 0; } break; } }
                    if (!inCore(x, z) && h > SIDEH + 0.4f && (rd || cityDist(x, z) < 3)) { c[0] = 255; c[1] = 40; c[2] = 40; }
                    fwrite(c, 1, 3, f); }
                fclose(f); }
            if (frame == 39) {   // overlap audit: every solid furniture piece against every other solid it intersects (5 cm inset)
                int bad = 0, shown = 0;
                for (int i = 0; i < nCont; i++) { Cont *c = &conts[i]; if (!c->on || c->sid < 0) continue; Solid *a = &sol[c->sid];
                    { Vector3 wmn, wmx; xfBox(a->f, a->mn, a->mx, &wmn, &wmx); query(wmn.x + 0.05f, wmn.z + 0.05f, wmx.x - 0.05f, wmx.z - 0.05f); }
                    for (int q = 0; q < qn; q++) { int id = qb[q]; if (id == c->sid) continue; Solid *b = &sol[id]; if (b->f != a->f) continue;
                        if (b->mx.y <= a->mn.y + 0.06f || b->mn.y >= a->mx.y - 0.05f) continue;
                        if (a->mn.x + 0.05f < b->mx.x && a->mx.x - 0.05f > b->mn.x && a->mn.z + 0.05f < b->mx.z && a->mx.z - 0.05f > b->mn.z) {
                            bad++; if (shown < 12) { shown++; TraceLog(LOG_INFO, "OVERLAP cont kind %d region %d at %.2f %.2f %.2f yaw %.2f  vs solid %d [%.2f %.2f %.2f]-[%.2f %.2f %.2f]", c->kind, c->region, c->pos.x, c->pos.y, c->pos.z, c->yaw, id, b->mn.x, b->mn.y, b->mn.z, b->mx.x, b->mx.y, b->mx.z); } break; } } }
                TraceLog(LOG_INFO, "OVERLAPTEST %d of %d containers intersect another solid", bad, nCont); }
            if (frame == 40) { state = ST_PLAY; pl.pos = V3(ROADW + PITCH + 2, 0, ROADW / 2); pl.tyaw = pl.yaw = PI / 2 + 0.3f; pl.tpitch = pl.pitch = 0.05f;
                for (int i = 0; i < nCr; i++) crs[i].on = false; nCr = 0;
                spawnCreature(V3(pl.pos.x + 5, 0, pl.pos.z + 1.8f), 0.01f, 0.02f, false); crs[0].pos = V3(pl.pos.x + 4.5f, 0, pl.pos.z + 1.2f); crs[0].type = 1; crs[0].yaw = -PI / 2;
                spawnCreature(pl.pos, 0.01f, 0.02f, false); crs[1].pos = V3(pl.pos.x + 7, 0, pl.pos.z - 1.5f); crs[1].type = 0; crs[1].yaw = -PI / 2;
                spawnCreature(pl.pos, 0.01f, 0.02f, false); crs[2].pos = V3(pl.pos.x + 3.5f, 0, pl.pos.z - 1.5f); crs[2].type = 3; crs[2].yaw = -PI / 2;
                spawnCreatureT(pl.pos, 0.01f, 0.02f, false, 4); crs[3].pos = V3(pl.pos.x + 9, 0.3f, pl.pos.z + 0.5f); crs[3].yaw = -PI / 2;
                invAdd(IT_AXE, 1); invAdd(IT_DUFFEL, 1); invAdd(IT_BANDAGE, 2); invAdd(IT_HAMMER, 1); invAdd(IT_PLANKS, 8); invAdd(IT_NAILS, 20); invAdd(IT_KATANA, 1); pl.wounds = 2; pl.hp = 60; }
            if (frame == 41 || frame == 42) { for (int i = 0; i < nCr; i++) crs[i].state = CS_WANDER; }
        }
#if !defined(PLATFORM_WEB)
        if (KP(KEY_F11)) { cfg.fullscreen = !cfg.fullscreen; ToggleBorderlessWindowed(); }
#endif

        // ---- update
        Vector3 eye, aim;
        if (state == ST_PLAY) {
            if (cht.avail && KP(cht.key) && !pl.invOpen && !pl.mapOpen) { cht.menu = !cht.menu; if (cht.menu) EnableCursor(); else DisableCursor(); }
            if (KP(KEY_ESCAPE) && cht.menu) { cht.menu = false; DisableCursor(); }
            else if (KP(KEY_ESCAPE)) { if (pl.mapOpen) { pl.mapOpen = false; DisableCursor(); } else if (pl.invOpen) { pl.invOpen = false; pl.openC = -1; DisableCursor(); } else { state = ST_PAUSE; EnableCursor(); pauseFrame = frame; } }
            if (cht.menu) { eye = Vector3Add(pl.pos, V3(0, 1.6f, 0)); aim = dirOf(pl.tyaw, pl.tpitch); } else {   // the cheat menu pauses the world
#if defined(PLATFORM_WEB)
            {   // browser pointer lock: request on click, pause when the browser releases it (Esc)
                static bool webLocked = false;
                if (pl.invOpen || pl.mapOpen) webLocked = false;
                else if (IsCursorHidden()) webLocked = true;
                else if (webLocked) { webLocked = false; state = ST_PAUSE; }
                else if (state == ST_PLAY && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) DisableCursor();
            }
#else
            if (state == ST_PLAY && !shotMode && !videoMode && !pl.invOpen && !pl.mapOpen && !IsCursorHidden()) DisableCursor();
#endif
            // hit-stop: a few frozen frames on impact sell the weight of a blow
            float wdt = dt;
            if (pl.hitStopT > 0) { pl.hitStopT -= dt; wdt = dt * 0.06f; }
            dt = wdt;
            updatePlayer(dt);
            updateSanity(dt, dirOf(pl.tyaw, pl.tpitch));
            survivalTick(dt);
            cheatTick(dt);
            if (pl.turned && pl.hp > 0) { pl.hp = 0; say("You have become one of them."); }
            eye = Vector3Add(pl.pos, V3(0, 1.6f + (pl.ground ? sinf(pl.bob * 2) * 0.035f : 0), 0));
            aim = dirOf(pl.tyaw, pl.tpitch);
            if (!shotMode || frame > 44) updateCreatures(dt, aim);
            if (evacOn) { evacT -= dt; if (evacT <= 0) { state = ST_WIN; EnableCursor(); prof.wins++; prof.kills += pl.kills; prof.playtime += pl.time; if (pl.time > prof.best) prof.best = pl.time; profSave(); gameActive = false; } }
            if (pl.hp <= 0 && shotMode) TraceLog(LOG_INFO, "SHOT: died at frame %d pos %.1f %.1f %.1f", frame, pl.pos.x, pl.pos.y, pl.pos.z);
            if (pl.hp <= 0) { deleteSave(); state = ST_DEAD; EnableCursor(); prof.deaths++; prof.kills += pl.kills; prof.playtime += pl.time; if (pl.time > prof.best) prof.best = pl.time; profSave(); gameActive = false; play(sScreech, 1); }
            }
        } else {
            float tt = t * 0.05f;
            eye = gameActive ? Vector3Add(pl.pos, V3(0, 1.6f, 0)) : V3(SPAWN_X + sinf(tt) * 1.5f, 2.2f, fmodf(t * 1.2f, 300) + 40);
            aim = gameActive ? dirOf(pl.tyaw, pl.tpitch) : Vector3Normalize(V3(sinf(t * 0.13f) * 0.4f, 0.02f, 1));
            if (audioOk) { if (!IsSoundPlaying(sDrone)) PlaySound(sDrone); SetSoundVolume(sStatic, 0); }
        }
        updateFX(dt, eye);
        Vector3 shake = V3(fr(-1, 1) * pl.shake * 0.13f, fr(-1, 1) * pl.shake * 0.13f, 0);
        cam.position = Vector3Add(eye, shake);
        Vector3 view = (state == ST_PLAY || gameActive) ? dirOf(pl.yaw, pl.pitch) : aim;
        cam.target = Vector3Add(cam.position, view);
        cam.up = Vector3RotateByAxisAngle(V3(0, 1, 0), Vector3Normalize(view), pl.roll); cam.fovy = cfg.fov - pl.punch * 9.0f; cam.projection = CAMERA_PERSPECTIVE;
        if (pl.punch > 0) cam.target = Vector3Add(cam.target, V3(0, -pl.punch * 0.02f, 0));

        // ---- render 3D low-res
        float flash = (pl.flash && (state == ST_PLAY || gameActive)) ? (pl.batt < 15 ? (hash2((int)(t * 15), 0, 1) > 0.3f ? 1 : 0.1f) : 1) : 0;
        if (!gameActive && state != ST_PLAY) flash = 0;
        float muzzle = pl.muzzle > 0 ? 1 : 0, amb = 0.30f, fogD = 0.040f, snap[2] = {(float)rtW, (float)rtH};
        {   // the wilds are bright, clear and wrong; the subway is darker than the street
            float wk = (state == ST_PLAY || gameActive) ? wildK(cam.position) : 0;
            Vector3 dark = {0.075f, 0.08f, 0.095f}, sky = {0.46f, 0.70f, 0.94f};
            if (cam.position.y < -40 || inBore(cam.position.x, cam.position.z, cam.position.y)) { amb = 0.16f; fogD = 0.055f; dark = (Vector3){0.03f, 0.035f, 0.035f}; }
            else if (gameActive) {   // day: flat grey overcast over the dead city; night: the old darkness
                float dk = dayK(); float dusk = (pl.tod > 16 && pl.tod < 20.5f) ? sinf((pl.tod - 16) / 4.5f * PI) : 0;
                dark = Vector3Lerp(dark, (Vector3){0.47f, 0.50f, 0.53f}, dk); dark = Vector3Lerp(dark, (Vector3){0.42f, 0.27f, 0.22f}, dusk * 0.45f);
                amb = Lerp(amb, 0.82f, dk); fogD = Lerp(fogD, 0.017f, dk);
            }
            Vector3 fc = Vector3Lerp(dark, sky, wk);
            amb = Lerp(amb, 0.95f, wk); fogD = Lerp(fogD, 0.009f, wk);
            fogc[0] = fc.x; fogc[1] = fc.y; fogc[2] = fc.z; fogC = (Color){(unsigned char)(fc.x * 255), (unsigned char)(fc.y * 255), (unsigned char)(fc.z * 255), 255};
            SetShaderValue(sh, locFog, fogc, SHADER_UNIFORM_VEC3);
        }
        SetShaderValue(sh, locView, &cam.position, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, locFlashDir, &aim, SHADER_UNIFORM_VEC3);
        SetShaderValue(sh, locFlashOn, &flash, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, locMuzzle, &muzzle, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, locAmb, &amb, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, locFogD, &fogD, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, locSnap, snap, SHADER_UNIFORM_VEC2);
        float desat = (state == ST_PLAY || gameActive) ? (pl.stage == 1 ? 0.35f : pl.stage >= 2 ? 0.65f : 0) + (pl.sanity < 30 ? (30 - pl.sanity) / 60.0f : 0) : 0;
        float rage = ((state == ST_PLAY || gameActive) && pl.stage == 3) ? pl.corrupt / 100.0f * (0.6f + 0.4f * sinf(t * 3)) : 0;
        desat = clampf(desat, 0, 0.9f);
        SetShaderValue(sh, locDesat, &desat, SHADER_UNIFORM_FLOAT); SetShaderValue(sh, locRage, &rage, SHADER_UNIFORM_FLOAT);
        setupLights(cam.position, t);
        updateBroadcast(t);
        {   // jingle from nearest live screen
            static float jt = 4; jt -= dt;
            if (jt <= 0 && audioOk) { jt = fr(12, 20); int best = -1; float bd = 38;
                for (int i = 0; i < nScreen; i++) if (screens[i].light >= 0) { float d = Vector3Distance(screens[i].c, cam.position); if (d < bd) { bd = d; best = i; } }
                if (best >= 0) { SetSoundVolume(sJingle, 0.55f * powf(1 - bd / 38, 1.3f)); SetSoundPitch(sJingle, fr(0.86f, 1.0f)); PlaySound(sJingle); } }
        }
        bool backdrop = !gameActive && (state == ST_MENU || state == ST_SETTINGS || state == ST_CONTROLS || state == ST_LORE || state == ST_TRAITS);
        if (!backdrop) {
        BeginTextureMode(rt);
        ClearBackground(fogC);
        BeginMode3D(cam);
        drawWorld(cam.position, t);
        if (state == ST_PLAY && pl.build) { Vector3 mn, mx; bool ok = buildGhost(&mn, &mx); rlDrawRenderBatchActive();
            DrawBoundingBox((BoundingBox){mn, mx}, ok ? (Color){111, 214, 208, 255} : (Color){200, 50, 40, 255});
            DrawBoundingBox((BoundingBox){Vector3Add(mn, V3(0.02f, 0.02f, 0.02f)), Vector3Subtract(mx, V3(0.02f, 0.02f, 0.02f))}, ok ? (Color){111, 214, 208, 255} : (Color){200, 50, 40, 255});
            rlDrawRenderBatchActive(); }
        if (state == ST_PLAY || state == ST_PAUSE) drawViewmodel2(cam.position, aim);
        EndMode3D();
        EndTextureMode();
        }

        // ---- composite + UI
        Vector2 cross = GetWorldToScreenEx(Vector3Add(cam.position, Vector3Scale(dirOf(pl.tyaw, pl.tpitch), 20)), cam, rtW, rtH);
        cross.x *= sw / rtW; cross.y *= shh / rtH;
        BeginDrawing();
        ClearBackground(BLACK);
        if (backdrop) saBackdrop(t, 0, 0, 0, 1.0f);
        else DrawTexturePro(rt.texture, (Rectangle){0, 0, (float)rtW, -(float)rtH}, (Rectangle){0, 0, sw, shh}, (Vector2){0, 0}, 0, WHITE);
        if (state == ST_PLAY) { drawHUD(cross, t); if (cht.menu) cheatMenu(t); }
        else {
            if (!backdrop) aBackground(state == ST_PAUSE ? 0.75f : 0.85f);
            aCorrupt = gameActive ? pl.stage : 0;
            p3Begin(t);
            if (state == ST_MENU) {
                const char *l[] = {"Continue", "New Game", "Multiplayer (not in this build)", "Settings", "Controls", "Lore", "Quit"}; bool en[] = {gameActive || saveExists(), true, false, true, true, true, true};
                int r = menuPanel("Seiry\xC5\xAB - Menu", l, 7, en);
                profilePanel();
                if (r == 0) { if (gameActive || loadGame()) { state = ST_PLAY; DisableCursor(); } else say("The save could not be read."); }
                if (r == 1) { if (videoMode) { newGame(); vSetupScene(); vT = 0; state = ST_PLAY; DisableCursor(); } else state = ST_TRAITS; }
                if (r == 3) { settingsBack = ST_MENU; state = ST_SETTINGS; }
                if (r == 4) { settingsBack = ST_MENU; state = ST_CONTROLS; }
                if (r == 5) state = ST_LORE;
                if (r == 6) { quitReq = true; }
            } else if (state == ST_PAUSE) {
                const char *l[] = {"Resume", "Settings", "Controls", "Main Menu"};
                int r = menuPanel("Paused", l, 4, NULL);
                profilePanel();
                if (r == 0 || (KP(KEY_ESCAPE) && frame != pauseFrame)) { state = ST_PLAY; DisableCursor(); }
                if (r == 1) { settingsBack = ST_PAUSE; state = ST_SETTINGS; }
                if (r == 2) { settingsBack = ST_PAUSE; state = ST_CONTROLS; }
                if (r == 3) { saveGame(); state = ST_MENU; }
            } else if (state == ST_LORE) lorePanel();
            else if (state == ST_TRAITS) traitsPanel();
            else if (state == ST_SETTINGS) settingsPanel();
            else if (state == ST_CONTROLS) controlsPanel();
            else if (state == ST_DEAD || state == ST_WIN) {
                bool win = state == ST_WIN;
                Rectangle tp = p3Panel(0, 0, 15, 600, 110, false);
                aGlow(win ? "SURVIVED" : pl.turned ? "TURNED" : "YOU DIED", tp.width / 2, 14 * aU, 64, win ? C_CYAN : C_RED);
                p3End(0);
                Rectangle p = p3Panel(1, 0, -5, 400, 270, true);
                float sw = p.width; (void)sw;
                aWindow(p, win ? "Run complete" : "Run over");
                char b[96];
                snprintf(b, sizeof b, "Survived  %02d:%02d", (int)pl.time / 60, (int)pl.time % 60); aTextC(b, sw / 2, p.y + 56 * aU, 20, C_BONE, false);
                snprintf(b, sizeof b, "Creatures put down  %d", pl.kills); aTextC(b, sw / 2, p.y + 88 * aU, 20, C_BONE, false);
                if (aButton((Rectangle){sw / 2 - 140 * aU, p.y + 134 * aU, 280 * aU, 46 * aU}, "New Game", true)) state = ST_TRAITS;
                if (aButton((Rectangle){sw / 2 - 140 * aU, p.y + 194 * aU, 280 * aU, 46 * aU}, "Main Menu", true)) state = ST_MENU;
                p3End(1);
            }
            p3Render(aCorrupt);
        }
        if (videoMode && state != ST_PLAY) aDrawCursor();
        EndDrawing();
        if (quitReq) break;
        if (videoMode) videoCapture(frame);
        if (shotMode) {
            if (frame == 30) TakeScreenshot("shot_menu.png");
            if (frame == 75) TakeScreenshot("shot_play.png");
            if (frame == 76) { pl.swingT = 0; pl.weapon = 0; state = ST_SETTINGS; settingsBack = ST_MENU; }
            if (frame == 80) TakeScreenshot("shot_settings.png");
            if (frame == 81) { state = ST_PLAY; pl.pos = V3(phonePos.x - 4, 1.05f, phonePos.z); pl.tyaw = pl.yaw = PI / 2; pl.tpitch = pl.pitch = 0; pl.build = true; }
            if (frame == 110) TakeScreenshot("shot_station.png");
            if (frame == 111) { pl.build = false; for (int i = 0; i < nScreen; i++) if (!screens[i].kiosk && screens[i].light >= 0 && lights[screens[i].light].mode == 0) { Screen *sc = &screens[i]; Vector3 o = V3(sinf(sc->ang), 0, cosf(sc->ang));
                pl.pos = Vector3Add(V3(sc->c.x, 0, sc->c.z), Vector3Scale(o, 11)); pl.pos.y = groundAt(V3(pl.pos.x, 3, pl.pos.z), 0.3f, 3); pl.tyaw = pl.yaw = sc->ang + PI; pl.tpitch = pl.pitch = 0.32f; invAdd(IT_REVOLVER, 1); { for (int q = 0; q < invNSlots(); q++) if (pl.sl[q].t == IT_REVOLVER) equipFromSlot(q); } pl.mag = 3; break; } }
            if (frame == 140) TakeScreenshot("shot_screen.png");
            if (frame == 150) updateBroadcast(20.0f);
            if (frame == 120 || frame == 150) { Image im = LoadImageFromTexture(scrRT.texture); ImageFlipVertical(&im); ImageResizeNN(&im, 480, 288); ExportImage(im, frame == 120 ? "feed_a.png" : "feed_b.png"); UnloadImage(im); }
            if (frame == 141) { for (int i = 0; i < nWard; i++) if (wards[i].kind == 2) { pl.pos = V3(wards[i].pos.x, SIDEH, wards[i].pos.z + 6); pl.tyaw = pl.yaw = PI; pl.tpitch = pl.pitch = 0.1f; break; } }
            if (frame == 160) TakeScreenshot("shot_shrine.png");
            if (frame == 161) { pl.pos = V3(ROADW + PITCH * 2 + 3, 0, ROADW / 2); pl.tyaw = pl.yaw = PI / 2; pl.tpitch = pl.pitch = 0;
                for (int i = 0; i < nCr; i++) crs[i].on = false;
                spawnCreatureT(pl.pos, 0.01f, 0.02f, false, 4); crs[0].pos = V3(pl.pos.x + 3.2f, 0.3f, pl.pos.z); crs[0].yaw = -PI / 2;
                spawnCreatureT(pl.pos, 0.01f, 0.02f, false, 1); crs[1].pos = V3(pl.pos.x + 6, 0, pl.pos.z + 2); crs[1].yaw = -PI / 2;
                spawnCreatureT(pl.pos, 0.01f, 0.02f, false, 3); crs[2].pos = V3(pl.pos.x + 4, 0, pl.pos.z - 1.8f); crs[2].yaw = -PI / 2; }
            if (frame == 163) { for (int i = 0; i < 3; i++) { crs[i].state = CS_WANDER; crs[i].spd = 0; } }
            if (frame == 175) TakeScreenshot("shot_creatures.png");
            if (frame == 176) state = ST_LORE;
            if (frame == 180) TakeScreenshot("shot_lore.png");
            if (frame == 181) { state = ST_PLAY; pl.stage = 3; pl.corrupt = 80; pl.sanity = 8; pl.hp = 24; }
            if (frame == 188) TakeScreenshot("shot_insane.png");
            if (frame == 189) { state = ST_PAUSE; }
            if (frame == 192) TakeScreenshot("shot_pause.png");
            if (frame == 194) { state = ST_PLAY; pl.stage = 0; pl.corrupt = 0; pl.sanity = 70; pl.hp = 80; pl.hurtT = 0;
                for (int i = 0; i < nCr; i++) crs[i].on = false;
                for (int i = 0; i < nCont; i++) if (conts[i].on && conts[i].kind == CK_WARDROBE) { Cont *c = &conts[i]; pl.pos = Vector3Add(c->pos, V3(sinf(c->yaw) * 1.2f, 0, cosf(c->yaw) * 1.2f));
                    pl.tyaw = pl.yaw = c->yaw + PI; pl.tpitch = pl.pitch = -0.2f; lootFill(c); slotsAdd(c->s, CSLOTS, IT_HIKEPACK, 1); slotsAdd(c->s, CSLOTS, IT_CROWBAR, 1); openInventory(i); break; } }
            if (frame == 198) TakeScreenshot("shot_inv.png");
            if (frame == 199) { pl.invOpen = false; pl.openC = -1;
                for (int i = 0; i < nStair; i++) if (stairs[i].top.y > 5 && stairs[i].top.y < 8) { pl.pos = stairs[i].top; Vector3 d = Vector3Subtract(stairs[i].top, stairs[i].bot); pl.tyaw = pl.yaw = atan2f(-d.x, -d.z); pl.tpitch = pl.pitch = -0.35f; pl.vy = 0; break; } }
            if (frame == 205) TakeScreenshot("shot_floor.png");
            if (frame == 206) { pl.pos = V3(ROADW + 2 * PITCH + 30, SUBY + 0.9f, TUN_Z1 - 1.7f); pl.tyaw = pl.yaw = PI / 2 + 0.25f; pl.tpitch = pl.pitch = 0; pl.flash = true; pl.batt = 100; spawnUnderground(); }
            if (frame == 212) TakeScreenshot("shot_subway.png");
            if (frame == 213) { for (int i = 0; i < nCr; i++) crs[i].on = false; pl.pos = V3(-70, 0, CITY / 2); pl.pos.y = terrainH(pl.pos.x, pl.pos.z); pl.tyaw = pl.yaw = -PI / 2 - 0.4f; pl.tpitch = pl.pitch = 0.05f; }
            if (frame == 220) TakeScreenshot("shot_wild.png");
            if (frame == 221) { pl.pos = V3(-8, 0, CITY / 2 + 3); pl.tyaw = pl.yaw = PI / 2; }
            if (frame == 226) TakeScreenshot("shot_edge.png");
            if (frame == 227) {   // walk up a flight of stairs with simulated input
                vin.on = true; memset(vin.down, 0, sizeof vin.down);
                for (int i = 0; i < nStair; i++) if (stairs[i].bot.y < 1 && stairs[i].top.y > 3 && stairs[i].top.y < 4) { stTest = i; Vector3 d_ = Vector3Subtract(stairs[i].top, stairs[i].bot); if (fabsf(d_.x) > 0.5f && fabsf(d_.z) > 0.5f) break; }   // prefer a turned building
                if (stTest >= 0) { Vector3 d_ = Vector3Subtract(stairs[stTest].top, stairs[stTest].bot); TraceLog(LOG_INFO, "STAIRTEST stair dir %.2f %.2f (turned building: %s)", d_.x, d_.z, fabsf(d_.x) > 0.5f && fabsf(d_.z) > 0.5f ? "yes" : "no"); }
                if (stTest >= 0) { Stair *s = &stairs[stTest]; pl.pos = s->bot; Vector3 d = Vector3Subtract(s->top, s->bot); pl.tyaw = pl.yaw = atan2f(d.x, d.z); pl.tpitch = pl.pitch = 0; pl.vy = 0; pl.hp = 100; vin.down[KEY_W] = 1; }
            }
            if (frame == 300) { if (stTest >= 0) TraceLog(LOG_INFO, "STAIRTEST: start %.2f top %.2f reached y %.2f", stairs[stTest].bot.y, stairs[stTest].top.y, pl.pos.y); TakeScreenshot("shot_upstairs.png"); vin.on = false; }
            if (frame == 301 && stTest >= 0) { vin.on = false; Stair *s = &stairs[stTest]; pl.pos = Vector3Add(s->top, Vector3Scale(Vector3Normalize(V3(s->top.x - s->bot.x, 0, s->top.z - s->bot.z)), 0.6f));
                Vector3 d = Vector3Subtract(s->bot, s->top); pl.tyaw = pl.yaw = atan2f(d.x, d.z); pl.tpitch = pl.pitch = -0.55f; pl.flash = true; pl.batt = 100; pl.vy = 0; }
            if (frame == 306) TakeScreenshot("shot_stairs_down.png");
            if (frame == 307) { vin.on = true; memset(vin.pressed, 0, sizeof vin.pressed); vin.pressed[KEY_ESCAPE] = 1; }
            if (frame == 308) { memset(vin.pressed, 0, sizeof vin.pressed); }
            if (frame == 310) { TraceLog(LOG_INFO, "ESCTEST: state after Esc = %s", state == ST_PAUSE ? "PAUSE (menu open)" : "PLAY (closed again)"); vin.on = false; state = ST_PLAY; }
            if (frame == 311) { pl.pos = V3(ROADW / 2, 0, ROADW + 20); pl.tyaw = pl.yaw = 0; pl.tpitch = pl.pitch = 0; for (int i = 0; i < nCr; i++) crs[i].on = false;
                int s2 = spawnAt(V3(ROADW / 2, 0, ROADW + 21.6f), 0, false); if (s2 >= 0) { crs[s2].yaw = PI; } }
            if (frame == 316) { for (int i = 0; i < nCr; i++) if (crs[i].on) { impactFX(&crs[i], 0.9f, false, V3(0, 0, 1)); break; } }
            if (frame == 317) TakeScreenshot("shot_hit.png");
            if (frame == 318) {   // second flight (switchback) from floor 1 to floor 2
                stTest = -1; for (int i = 0; i < nStair; i++) if (stairs[i].bot.y > 3 && stairs[i].bot.y < 4 && stairs[i].top.y > 6) { stTest = i; break; }
                if (stTest >= 0) { vin.on = true; memset(vin.down, 0, sizeof vin.down); memset(vin.pressed, 0, sizeof vin.pressed); Stair *s = &stairs[stTest]; pl.pos = s->bot; Vector3 d = Vector3Subtract(s->top, s->bot); pl.tyaw = pl.yaw = atan2f(d.x, d.z); pl.tpitch = pl.pitch = 0; vin.down[KEY_W] = 1; }
            }
            if (frame == 390) { if (stTest >= 0) TraceLog(LOG_INFO, "STAIRTEST2: start %.2f top %.2f reached y %.2f", stairs[stTest].bot.y, stairs[stTest].top.y, pl.pos.y); else TraceLog(LOG_INFO, "STAIRTEST2: no second flight found"); vin.on = false; }
            if (frame == 391) { if (stTest >= 0) { Stair *s = &stairs[stTest]; pl.pos = Vector3Add(s->bot, V3(0, 0, 0)); Vector3 d = Vector3Subtract(s->bot, s->top); pl.tyaw = pl.yaw = atan2f(-d.x, -d.z) + 2.3f; pl.tpitch = pl.pitch = -0.25f; } }
            if (frame == 396) TakeScreenshot("shot_switchback.png");
            if (frame == 397) { vin.on = false; for (int i = 0; i < nCr; i++) crs[i].on = false; pl.pos = V3(ROADW / 2, 0, ROADW + 20); pl.tyaw = pl.yaw = 0; pl.tpitch = pl.pitch = 0; pl.flash = true; pl.batt = 100;
                int a1 = spawnAt(V3(ROADW / 2 - 0.9f, 0, ROADW + 23), 4, false), a2 = spawnAt(V3(ROADW / 2 + 0.9f, 0, ROADW + 23), 6, false);
                if (a1 >= 0) { crs[a1].yaw = PI; crs[a1].stun = 99; } if (a2 >= 0) { crs[a2].yaw = PI; crs[a2].stun = 99; } }
            if (frame == 399) for (int i = 0; i < nCr; i++) if (crs[i].on) TraceLog(LOG_INFO, "CMPTEST: type %d at %.1f %.1f %.1f", crs[i].type, crs[i].pos.x, crs[i].pos.y, crs[i].pos.z);
            if (frame == 398) TakeScreenshot("shot_svs_a.png");
            if (frame == 400) { TakeScreenshot("shot_spirit_vs_screamer.png"); for (int i = 0; i < nCr; i++) if (crs[i].on) { Vector2 s = GetWorldToScreen(crCenter(&crs[i]), cam); TraceLog(LOG_INFO, "CMPTEST screen type %d -> %.0f %.0f (cam %.1f %.1f %.1f -> %.1f %.1f %.1f)", crs[i].type, s.x, s.y, cam.position.x, cam.position.y, cam.position.z, cam.target.x, cam.target.y, cam.target.z); } }
            if (frame == 401) { int sp = -1; for (int i = 0; i < nCr; i++) { if (crs[i].on && crs[i].type == 6) crs[i].on = false; if (crs[i].on && crs[i].type == 4) sp = i; }
                if (sp >= 0) { pl.pos = V3(crs[sp].pos.x, 0, crs[sp].pos.z - 1.3f); Vector3 eye = V3(pl.pos.x, pl.pos.y + 1.6f, pl.pos.z);
                    Vector3 aim = Vector3Normalize(Vector3Subtract(crCenter(&crs[sp]), eye));
                    msg[0] = 0; meleeHit(eye, aim); TraceLog(LOG_INFO, "MELEETEST spirit only: '%s'", msg);
                    int s3 = spawnAt(V3(crs[sp].pos.x + 0.25f, 0, crs[sp].pos.z - 0.4f), 6, false); if (s3 >= 0) { crs[s3].stun = 99; crs[s3].pos.y = 0; }
                    msg[0] = 0; meleeHit(eye, aim); TraceLog(LOG_INFO, "MELEETEST spirit+screamer: '%s' screamer hurt=%.2f", msg, s3 >= 0 ? crs[s3].hurt : -1.0f); }
                else TraceLog(LOG_INFO, "MELEETEST: no spirit"); }
            static int spT = -1;
            if (frame == 402) { for (int i = 0; i < nCr; i++) crs[i].on = false; pl.wounds = 0; pl.hp = 100; pl.pos = V3(ROADW / 2, 0, ROADW + 20);
                spT = spawnAt(V3(ROADW / 2, 0, ROADW + 28), 4, true); if (spT >= 0) { crs[spT].state = CS_CHASE; crs[spT].chaseT = 38; } }
            if (frame > 402) { pl.wounds = 0; pl.hp = 100; pl.food = pl.water = 100; pl.infect = 0; } if (frame > 402 && frame < 760 && frame % 30 == 0 && spT >= 0) { Cr *c = &crs[spT]; TraceLog(LOG_INFO, "SPIRITTEST f%d on=%d state=%d chaseT=%.1f calm=%.1f dist=%.1f t=%.1f", frame, c->on, c->state, c->chaseT, c->calm, Vector3Distance(c->pos, pl.pos), GetTime()); if (!c->on) spT = -1; }
            if (frame == 760) { spT = -1; for (int i = 0; i < nCr; i++) crs[i].on = false; pl.hp = 100; pl.wounds = 0; pl.sick = 0; pl.infect = 0; invReset(); pl.tod = 12; pl.sleeping = false;
                // crafting
                invAdd(IT_RAG, 3); craft(0); TraceLog(LOG_INFO, "T_CRAFT bandage: %d (rags left %d)", invCount(IT_BANDAGE), invCount(IT_RAG));
                // water: stand at a rain barrel, fill a bottle
                int wi = -1; for (int i = 0; i < nWs; i++) if (wsrc[i].kind == 0) { wi = i; break; }
                if (wi >= 0) { pl.pos = V3(wsrc[wi].pos.x - 1.0f, wsrc[wi].pos.y, wsrc[wi].pos.z); pl.yaw = pl.tyaw = PI / 2; pl.pitch = pl.tpitch = 0;
                    invAdd(IT_BOTTLE, 2); Vector3 e = Vector3Add(pl.pos, V3(0, 1.6f, 0)), a = dirOf(pl.tyaw, pl.tpitch);
                    prompt[0] = 0; bool hp = survPrompt(e, a, false); TraceLog(LOG_INFO, "T_WATER prompt(%d): '%s'", hp, prompt);
                    survPrompt(e, a, true); TraceLog(LOG_INFO, "T_WATER filled: unboiled %d, bottles %d", invCount(IT_DIRTYWATER), invCount(IT_BOTTLE)); }
                else TraceLog(LOG_INFO, "T_WATER: no rain barrel");
                pl.nearFire = true; craft(2); pl.nearFire = false; TraceLog(LOG_INFO, "T_BOIL: water %d, unboiled %d", invCount(IT_WATER), invCount(IT_DIRTYWATER));
                useItem(IT_WATER); TraceLog(LOG_INFO, "T_DRINK: bottles back %d", invCount(IT_BOTTLE));
                // wear + break, repair
                pl.hands = (Slot){IT_AXE, 1, 996}; weaponWear(1); TraceLog(LOG_INFO, "T_WEAR axe at 990 + hit -> hands %d", pl.hands.t);
                pl.hands = (Slot){IT_PIPE, 1, 800}; invAdd(IT_HAMMER, 1); invAdd(IT_SCRAP, 1); invAdd(IT_NAILS, 4); craft(6); TraceLog(LOG_INFO, "T_REPAIR pipe 800 -> %d", pl.hands.w);
                // fracture + splint
                breakLeg("test"); invAdd(IT_SPLINT, 1); useItem(IT_SPLINT); TraceLog(LOG_INFO, "T_FRACTURE after splint: %d", pl.fracture); pl.fracT = 11.98f;
                getSick(50, "test"); TraceLog(LOG_INFO, "T_SICK start %.1f", pl.sick);
                // garden + spikes
                Vector3 gp = Vector3Add(pl.pos, V3(-1.5f, 0, 0));
                int gi = nBuild++; builds[gi] = (Build){V3(gp.x - 0.9f, gp.y, gp.z - 0.5f), V3(gp.x + 0.9f, gp.y + 0.3f, gp.z + 0.5f), -1, BK_GARDEN, 36, 36, true, true, 0, -1};
                int si = nBuild++; Vector3 sp = Vector3Add(pl.pos, V3(-3, 0, 4)); builds[si] = (Build){V3(sp.x - 0.55f, sp.y, sp.z - 0.55f), V3(sp.x + 0.55f, sp.y + 0.25f, sp.z + 0.55f), -1, BK_SPIKES, 6, 6, true, false, 0, -1};
                int vc = spawnAt(V3(sp.x, sp.y, sp.z), 0, false); if (vc >= 0) { crs[vc].stun = 99; TraceLog(LOG_INFO, "T_SPIKES victim hp before %.2f", crs[vc].hp); }
                pl.yaw = pl.tyaw = -PI / 2; Vector3 e = Vector3Add(pl.pos, V3(0, 1.6f, 0)), a = Vector3Normalize(Vector3Subtract(V3(gp.x, gp.y + 0.2f, gp.z), e));
                int d0 = invCount(IT_DAIKON); survPrompt(e, a, true); TraceLog(LOG_INFO, "T_GARDEN harvest: daikon %d -> %d, replantable %d", d0, invCount(IT_DAIKON), builds[gi].open);
            }
            if (frame == 800) { for (int i = 0; i < nBuild; i++) if (builds[i].kind == BK_SPIKES) TraceLog(LOG_INFO, "T_SPIKES trap uses left %.0f", builds[i].hp);
                for (int i = 0; i < nCr; i++) if (crs[i].on) TraceLog(LOG_INFO, "T_SPIKES creature %d hp %.2f state %d", crs[i].type, crs[i].hp, crs[i].state);
                TraceLog(LOG_INFO, "T_SICK after 40 frames %.1f, fracture %d, tod %.2f", pl.sick, pl.fracture, pl.tod); }
            if (frame == 805) { for (int i = 0; i < nCr; i++) crs[i].on = false; pl.pos = V3(ROADW / 2, 0, ROADW + 40); pl.yaw = pl.tyaw = 0; pl.pitch = pl.tpitch = 0.05f; pl.tod = 12.5f; }
            if (frame == 812) TakeScreenshot("shot_day.png");
            if (frame == 813) pl.tod = 18.2f;
            if (frame == 818) TakeScreenshot("shot_dusk.png");
            if (frame == 819) { pl.tod = 23; }
            if (frame == 824) TakeScreenshot("shot_nightclock.png");
            if (frame == 825) { // save / load round trip
                gameActive = true; state = ST_PLAY; pl.hp = 100; pl.day = 3; pl.tod = 9.5f; Vector3 p0 = pl.pos; int nb = nBuild, inv = invCount(IT_DAIKON), w0 = pl.hands.w; nMark = 2; marks[0] = (Vector2){100, 100}; marks[1] = (Vector2){40, 200};
                conts[5].searched = true; conts[5].filled = true;
                bool sv = saveGame(); pl.pos = V3(1, 1, 1); pl.day = 1; nMark = 0;
                bool ld = loadGame();
                TraceLog(LOG_INFO, "T_SAVE save %d load %d | pos %.1f,%.1f vs %.1f,%.1f | day %d tod %.1f | builds %d vs %d | daikon %d vs %d | wear %d vs %d | marks %d | cont5 searched %d",
                         sv, ld, pl.pos.x, pl.pos.z, p0.x, p0.z, pl.day, pl.tod, nBuild, nb, invCount(IT_DAIKON), inv, pl.hands.w, w0, nMark, conts[5].searched);
                for (int i = 0; i < nCr; i++) crs[i].on = false; state = ST_PLAY; }
            if (frame == 830) { pl.mapOpen = true; }
            if (frame == 834) TakeScreenshot("shot_map.png");
            if (frame == 835) { pl.mapOpen = false; pl.invOpen = true; pl.openC = -1; invAdd(IT_RAG, 4); invAdd(IT_PLANKS, 2); invAdd(IT_DIRTYWATER, 1); pl.hands = (Slot){IT_BAT, 1, 640}; }
            if (frame == 839) TakeScreenshot("shot_craft.png");
            if (frame == 840) { pl.invOpen = false; state = ST_TRAITS; }
            if (frame == 844) TakeScreenshot("shot_traits.png");
            if (frame == 845) { state = ST_PLAY; pl.sick = 60; pl.infect = 50; pl.fracture = 1; pl.wounds = 1; hintN = 0; prof.hints = 0; showHint(5); }
            if (frame == 850) TakeScreenshot("shot_status.png");
            #define SHOTAT(F, X, Y, Z, YAW, PITCH, NAME) if (frame == (F)) { for (int i_ = 0; i_ < nCr; i_++) crs[i_].on = false; pl.pos = V3(X, Y, Z); pl.tyaw = pl.yaw = (YAW); pl.tpitch = pl.pitch = (PITCH); pl.flash = true; pl.batt = 100; pl.vy = 0; } \
                if (frame == (F) + 6) TakeScreenshot(NAME);
            if (frame == 852) { gameActive = true; pl.hp = 100; pl.mapOpen = false; pl.invOpen = false; pl.sick = 0; pl.infect = 0; pl.fracture = 0; pl.wounds = 0; state = ST_PLAY; pl.tod = 11; hintN = 0; }
            SHOTAT(853, coastP[nCoast / 2].x, 0.2f, coastP[nCoast / 2].z, atan2f(coastP[nCoast / 2 + 4].x - coastP[nCoast / 2].x, coastP[nCoast / 2 + 4].z - coastP[nCoast / 2].z), -0.03f, "shot_coast.png")
            SHOTAT(860, bores[0].px, 0.2f, bores[0].pz + 24, PI, 0.08f, "shot_portal_w.png")
            SHOTAT(867, bores[0].px, 0.0f, bores[0].pz - 20, PI, 0.0f, "shot_tunnel_in.png")
            SHOTAT(874, bores[0].px, 0.0f, bores[0].pz - 70, PI, 0.0f, "shot_collapse.png")
            SHOTAT(881, tougeP[40].x, tougeP[40].y + 0.1f, tougeP[40].z, atan2f(tougeP[46].x - tougeP[40].x, tougeP[46].z - tougeP[40].z), 0.05f, "shot_touge.png")
            SHOTAT(888, HAMLET_X, pads[0].y + 0.1f, HAMLET_Z + 14, PI, -0.05f, "shot_hamlet.png")
            SHOTAT(895, bores[3].px, bores[3].fy, bores[3].pz + 9, PI, 0.0f, "shot_oldtunnel.png")
            { int b_ = streetOfKind(3, 0); const Vector3 *q_ = stPts + dStreets[b_][0]; int n_ = dStreets[b_][1];
              SHOTAT(902, q_[0].x, SIDEH + 0.1f, q_[0].z, atan2f(q_[n_ - 1].x - q_[0].x, q_[n_ - 1].z - q_[0].z), -0.05f, "shot_bridge.png") }
            SHOTAT(909, bores[1].px - 24, 0.2f, bores[1].pz, PI / 2, 0.02f, "shot_portal_se.png")
            { int a_ = streetOfKind(1, 0); const Vector3 *q_ = stPts + dStreets[a_][0]; int m_ = dStreets[a_][1] / 3;
              SHOTAT(1032, q_[m_].x, SIDEH + 0.1f, q_[m_].z, atan2f(q_[m_ + 2].x - q_[m_].x, q_[m_ + 2].z - q_[m_].z), -0.03f, "shot_avenue.png") }
            { int t_ = streetOfKind(2, 0); const Vector3 *q_ = stPts + dStreets[t_][0]; int n_ = dStreets[t_][1];
              SHOTAT(1040, q_[0].x, SIDEH + 0.1f, q_[0].z, atan2f(q_[n_ - 1].x - q_[0].x, q_[n_ - 1].z - q_[0].z), -0.05f, "shot_stairs.png") }
            { Vector3 w_ = seaSidePt(nCoast / 2, 30); Vector3 c_ = coastP[nCoast / 2 + 3]; Vector3 e_ = V3(c_.x + (w_.x - coastP[nCoast / 2].x) * 0.1f, SIDEH + 0.1f, c_.z + (w_.z - coastP[nCoast / 2].z) * 0.1f);
              SHOTAT(1056, e_.x, e_.y, e_.z, atan2f(w_.x - c_.x, w_.z - c_.z) - 0.6f, -0.35f, "shot_seawall.png") if (frame > 1056 && frame < 1062) pl.flash = false; }
            SHOTAT(1048, (float)SPAWN_X, 0.2f, (float)SPAWN_Z, 0.6f, 0.0f, "shot_spawn.png")
            if (frame == 916) { pl.tod = 21; pl.mapOpen = true; }
            if (frame == 921) TakeScreenshot("shot_map2.png");
            if (frame == 922) { pl.mapOpen = false;
                Vector3 a = V3(bores[0].px, 0.3f, bores[0].pz - 30); float g1 = groundAt(a, 0.3f, 0.42f);
                Vector3 b = V3(bores[0].px, terrainH(bores[0].px, bores[0].pz - 30) + 0.3f, bores[0].pz - 30); float g2 = groundAt(b, 0.3f, 0.42f);
                Vector3 sp = seaSidePt(nCoast / 2, 40); float d0 = terrainH(sp.x, sp.z); resolve(&sp, 0.3f, 1.7f, 0.42f);
                TraceLog(LOG_INFO, "T_GROUND in tunnel %.2f (floor 0) | on hill above %.2f (terrain %.2f) | sea push: depth %.2f -> pos %.1f,%.1f terrain %.2f | land(spawn)=%d land(after push)=%d",
                         g1, g2, terrainH(bores[0].px, bores[0].pz - 30), d0, sp.x, sp.z, terrainH(sp.x, sp.z), cityLand(SPAWN_X, SPAWN_Z), cityLand(sp.x, sp.z));
                { int ci_ = nCoast / 2; Vector2 nn_ = pnorm(coastP, nCoast, ci_); char buf_[600] = ""; for (int d_ = -10; d_ <= 24; d_ += 2) { float x_ = coastP[ci_].x + nn_.x * d_, z_ = coastP[ci_].z + nn_.y * d_; char t_[40]; snprintf(t_, 40, " %d:%.1f/%.1f%s", d_, terrainH(x_, z_), coastSigned(x_, z_), cityLand(x_, z_) ? "L" : ""); strcat(buf_, t_); } TraceLog(LOG_INFO, "T_COASTPROF%s", buf_); }
                for (int b2 = 0; b2 < nBore; b2++) { Bore *bb = &bores[b2]; Vector3 mid = boreAt(bb, bb->len / 2, 0, 0); TraceLog(LOG_INFO, "T_BORE %d floor %.1f terrain above mid %.1f (ceiling %.1f)", b2, bb->fy, terrainH(mid.x, mid.z), bb->fy + bb->h + 0.5f); } }
            if (frame == 924) {   // a lit shop sign, head-on, at night
                pl.tod = 21; int best = -1;
                for (int i = 0; i < nSign; i++) if (!signs[i].plate && !signs[i].f && signs[i].tex == T_SIGNH && signs[i].text < JP_NSHOP + 1) { best = i; if (signs[i].text == SGN_KONBINI) break; }
                if (best >= 0) { Sign *s = &signs[best]; Vector3 c = xfP(s->f, Vector3Scale(Vector3Add(s->mn, s->mx), 0.5f)); float yw[4] = {0, -PI / 2, PI, PI / 2}; Vector3 off[4] = {V3(0, 0, -7), V3(7, 0, 0), V3(0, 0, 7), V3(-7, 0, 0)};
                    pl.pos = V3(c.x + off[s->face & 3].x, 0.05f, c.z + off[s->face & 3].z); pl.tyaw = pl.yaw = yw[s->face & 3]; pl.tpitch = pl.pitch = atan2f(c.y - 1.6f, 7); pl.vy = 0;
                    TraceLog(LOG_INFO, "T_SIGN text %d face %d at %.1f %.1f %.1f", s->text, s->face, c.x, c.y, c.z); }
                for (int i = 0; i < nCr; i++) crs[i].on = false; }
            if (frame == 930) TakeScreenshot("shot_sign.png");
            if (frame == 931) { pl.tod = 12; pl.tpitch = pl.pitch = -0.1f; pl.hands = (Slot){IT_KATANA, 1, 0}; animPlay(ANIM_INSPECT); }
            for (int q = 0; q < 4; q++) { if (frame == 932 + q * 7) { animId = ANIM_INSPECT; animT = animDur * (0.12f + 0.24f * q); } if (frame == 934 + q * 7) { char nm[40]; snprintf(nm, sizeof nm, "shot_inspect%d.png", q); TakeScreenshot(nm); } }
            if (frame == 963) { animId = ANIM_NONE; forceGuard = true; }
            if (frame == 972) TakeScreenshot("shot_guard.png");
            if (frame == 973) { forceGuard = false; pl.hands = (Slot){-1, 0, 0}; animUse(ANIM_EAT, IT_FOOD); }
            if (frame == 980) animT = animDur * 0.45f;
            if (frame == 982) TakeScreenshot("shot_eat.png");
            if (frame == 990) { animId = ANIM_NONE; pl.watchKind = 2; pl.watchT = 5; }
            if (frame == 1004) { TakeScreenshot("shot_manga.png"); Image mi = LoadImageFromTexture(mangaRT.texture); ExportImage(mi, "dbg_manga.png"); UnloadImage(mi); }
            if (frame == 1012) { pl.watchT = 0; pl.watchKind = 0; pl.hands = (Slot){IT_REVOLVER, 1, 0}; pl.mag = 3; animPlay(ANIM_RELOAD); }
            if (frame == 1018) animT = animDur * 0.4f;
            if (frame == 1020) TakeScreenshot("shot_reload.png");
            {   int kinds[5] = {0, 1, 2, 3, 6}; const char *nm[5] = {"shot_int_house.png", "shot_int_apt.png", "shot_int_shop.png", "shot_int_konbini.png", "shot_int_office.png"};
                for (int q = 0; q < 5; q++) { int F = 1062 + q * 7;
                    if (frame == F) { for (int i = 0; i < nCr; i++) crs[i].on = false; pl.tod = 12; pl.flash = true; pl.batt = 100;
                        for (int i = 0; i < nCont; i++) { Cont *c = &conts[i]; if (!c->on || c->region != kinds[q] || c->pos.y > 1 || !(c->kind == CK_FRIDGE || c->kind == CK_DESK || c->kind == CK_SHELF)) continue;
                            Vector3 p = Vector3Add(c->pos, V3(sinf(c->yaw) * 3.0f, 0, cosf(c->yaw) * 3.0f)); if (!pointFree(p.x, p.z, 0.3f, c->pos.y + 0.05f)) continue;
                            pl.pos = p; pl.tyaw = pl.yaw = c->yaw + PI + 0.35f; pl.tpitch = pl.pitch = -0.25f; pl.vy = 0; break; } }
                    if (frame == F + 5) TakeScreenshot(nm[q]); } }
            if (frame == 1100) { pl.pos = V3(SPAWN_X, 0, SPAWN_Z); pl.tyaw = pl.yaw = 0.6f; pl.tpitch = pl.pitch = 0; pl.hands = (Slot){IT_KATANA, 1, 0}; animId = ANIM_NONE; pl.flash = false; pl.blocking = false; forceGuard = false; }
            if (frame == 1106) TakeScreenshot("shot_katana_idle.png");
            if (frame == 1107) { pl.hands = (Slot){IT_BAT, 1, 0}; }
            if (frame == 1112) TakeScreenshot("shot_bat_idle.png");
            if (frame == 1113) { pl.hands = (Slot){IT_KATANA, 1, 0}; pl.flash = true; }
            if (frame == 1118) TakeScreenshot("shot_katana_phone.png");
            for (int q = 0; q < 3; q++) { int F = 1121 + q * 7, i = nCoast * (q + 1) / 4; Vector2 nn = pnorm(coastP, nCoast, i);
                if (frame == F) { for (int c2 = 0; c2 < nCr; c2++) crs[c2].on = false; pl.tod = 12; pl.flash = false; pl.hands = (Slot){-1, 0, 0};
                    pl.pos = V3(coastP[i].x + nn.x * 2.5f, 0.1f, coastP[i].z + nn.y * 2.5f); pl.tyaw = pl.yaw = atan2f(-nn.x, -nn.y) + 0.7f; pl.tpitch = pl.pitch = 0.02f; pl.vy = 0; }
                if (frame == F + 5) { char nm[40]; snprintf(nm, sizeof nm, "shot_coastfront%d.png", q); TakeScreenshot(nm); } }
            if (frame == 1143) { pl.mapOpen = true; pl.tod = 12; }
            if (frame == 1149) TakeScreenshot("shot_map3.png");
            {   // weapon animation stills: each held at a fixed point of its curve
                struct { int it, anim; float sw, at; const char *nm; } W[] = {
                    {IT_KATANA, -1, 0.18f, 0, "shot_w_katana_windup.png"}, {IT_KATANA, -1, 0.40f, 0, "shot_w_katana_cut.png"}, {IT_KATANA, ANIM_EQUIP, 0, 0.35f, "shot_w_katana_draw.png"},
                    {IT_KATANA, ANIM_FLOURISH, 0, 0.2f, "shot_w_katana_chiburi.png"}, {IT_BAT, -1, 0.22f, 0, "shot_w_bat_windup.png"}, {IT_BAT, -1, 0.42f, 0, "shot_w_bat_sweep.png"},
                    {IT_AXE, -1, 0.25f, 0, "shot_w_axe_raise.png"}, {IT_AXE, -1, 0.42f, 0, "shot_w_axe_chop.png"}, {IT_KNIFE, -1, 0.45f, 0, "shot_w_knife_stab.png"}};
                for (int q = 0; q < 9; q++) { int F = 1151 + q * 4;
                    if (frame == F) { pl.mapOpen = false; pl.pos = V3(SPAWN_X, 0, SPAWN_Z); pl.tyaw = pl.yaw = 0.6f; pl.tpitch = pl.pitch = 0; pl.flash = false; pl.hands = (Slot){(short)W[q].it, 1, 0}; animId = ANIM_NONE; }
                    if (frame == F + 2) { if (W[q].anim >= 0) { animPlay(W[q].anim); animT = animDur * W[q].at; pl.swingT = 0; } else { animId = ANIM_NONE; pl.swingDur = meleeCd(); pl.swingT = pl.swingDur * (1 - W[q].sw); } }
                    if (frame == F + 3) TakeScreenshot(W[q].nm); } }
            if (frame == 1160) { pl.hands = (Slot){IT_REVOLVER, 1, 50}; pl.mag = 2; cht.on[CH_GOD] = cht.on[CH_AMMO] = cht.on[CH_WEAR] = cht.on[CH_NEEDS] = true; pl.food = 10; hurtPlayer(60, 1); }
            if (frame == 1163) { TraceLog(LOG_INFO, "T_CHEAT avail %d mod '%s' key %d | god hp %.0f wounds %d | ammo %d | wear %d | food %.0f", cht.avail, cht.mod, cht.key, pl.hp, pl.wounds, pl.mag, pl.hands.w, pl.food);
                cht.on[CH_NOCLIP] = true; vin.on = true; memset(vin.down, 0, sizeof vin.down); vin.down[KEY_SPACE] = true; cht.menu = false; }
            if (frame == 1164) cht.on[CH_NOCLIP] = true;
            if (frame == 1170) { TraceLog(LOG_INFO, "T_CHEAT noclip y after 6 frames rising: %.2f", pl.pos.y); vin.down[KEY_SPACE] = false; vin.on = false; cht.on[CH_NOCLIP] = false; cht.menu = true; EnableCursor(); }
            if (frame == 1175) TakeScreenshot("shot_cheats.png");
            if (frame == 1176) { cht.menu = false; for (int i = 0; i < CH_N; i++) cht.on[i] = false; }
            static int wtS = -1; static float wtSide = 0;
            if (frame == 1177) {   // walk into the wall of a turned building: the player must stay on his side of it
                wtS = -1; for (int i = 0; i < nSol; i++) { Solid *s = &sol[i]; Vector3 sz = Vector3Subtract(s->mx, s->mn); if (!s->f || s->off || s->mn.y > 0.3f || sz.y < 2.5f || !(sz.x < 0.35f && sz.z > 3.0f)) continue;
                    if (fabsf(xfs[s->f].s) < 0.3f) continue; wtS = i; break; }
                if (wtS >= 0) { Solid *s = &sol[wtS]; Vector3 c = Vector3Scale(Vector3Add(s->mn, s->mx), 0.5f); Vector3 L = V3(s->mn.x - 0.7f, SIDEH, c.z); Vector3 W = xfP(s->f, L); Vector3 dir = xfD(s->f, V3(1, 0, 0));
                    pl.pos = W; pl.vy = 0; pl.tyaw = pl.yaw = atan2f(dir.x, dir.z); pl.tpitch = pl.pitch = 0; wtSide = -1; vin.on = true; memset(vin.down, 0, sizeof vin.down); vin.down[KEY_W] = 1; for (int i = 0; i < nCr; i++) crs[i].on = false; } }
            if (frame > 1177 && frame < 1188 && wtS >= 0) { Solid *s = &sol[wtS]; Vector3 L = xfI(s->f, pl.pos); TraceLog(LOG_INFO, "WALLDBG f%d local %.2f %.2f %.2f world %.2f %.2f", frame, L.x, L.y, L.z, pl.pos.x, pl.pos.z); }
            if (frame == 1188 && wtS >= 0) { Solid *s = &sol[wtS]; Vector3 L = xfI(s->f, pl.pos); vin.on = false;
                TraceLog(LOG_INFO, "WALLTEST turned wall (frame angle %.0f deg): player local x %.2f, wall from %.2f to %.2f -> %s", xfYaw(s->f) * RAD2DEG, L.x, s->mn.x, s->mx.x, L.x < s->mn.x ? "blocked" : "WENT THROUGH"); }
            if (frame == 1190) break;
        }
    }
    if (gameActive) saveGame();
    if (gameActive) { prof.kills += pl.kills; prof.playtime += pl.time; if (pl.time > prof.best) prof.best = pl.time; profSave(); }
    cfgSave();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
