// aero.h — shared config, paths and dark "Frutiger Aero" UI for KIRIMACHI + Launcher
#ifndef AERO_H
#define AERO_H
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raymath.h"

#define GAME_VERSION "0.9.3"
#if defined(PLATFORM_WEB)
#define GLSL_HEAD "#version 300 es\nprecision highp float;\nprecision highp int;\n"
#include <emscripten/emscripten.h>
#else
#define GLSL_HEAD "#version 330\n"
#endif

// ---------------------------------------------------------------- config
typedef struct {
    int resW, resH, fullscreen, vsync, renderH;
    float sens, aimLag, fov, volume, music;
    int autoUpdate;
    char channel[16];
} Config;
static Config cfg;
static char gBase[600];

static void aeroInitPaths(void) {
#if defined(PLATFORM_WEB)
    snprintf(gBase, sizeof gBase, "/"); return;   // Data/, Textures/, Mods/ are preloaded at the virtual filesystem root
#endif
    const char *app = GetApplicationDirectory();
    char t[700];
    snprintf(t, sizeof t, "%s../Data", app);
    if (DirectoryExists(t)) snprintf(gBase, sizeof gBase, "%s../", app);
    else snprintf(gBase, sizeof gBase, "%s", app);
}
static const char *P(const char *rel) {
    static _Thread_local char buf[6][800]; static _Thread_local int k = 0;   // per thread: the launcher's update threads build paths while the UI does too
    k = (k + 1) % 6;
    snprintf(buf[k], 800, "%s%s", gBase, rel);
    return buf[k];
}
static void ensureDir(const char *rel) { const char *p = P(rel); if (!DirectoryExists(p)) MakeDirectory(p); }

static void cfgDefaults(void) {
    cfg.resW = 1280; cfg.resH = 720; cfg.fullscreen = 0; cfg.vsync = 1; cfg.renderH = 240;
    cfg.sens = 0.0022f; cfg.aimLag = 6.0f; cfg.fov = 70.0f; cfg.volume = 0.8f; cfg.music = 0.6f;
    cfg.autoUpdate = 1; strcpy(cfg.channel, "stable");
}
static void cfgLoad(void) {
    cfgDefaults();
    FILE *f = fopen(P("Data/config.ini"), "r");
    if (!f) return;
    char line[256], k[64], v[128];
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == ';') continue;
        if (sscanf(line, "%63[^=]=%127s", k, v) != 2) continue;
        if (!strcmp(k, "resW")) cfg.resW = atoi(v);
        else if (!strcmp(k, "resH")) cfg.resH = atoi(v);
        else if (!strcmp(k, "fullscreen")) cfg.fullscreen = atoi(v);
        else if (!strcmp(k, "vsync")) cfg.vsync = atoi(v);
        else if (!strcmp(k, "renderH")) cfg.renderH = atoi(v);
        else if (!strcmp(k, "sens")) cfg.sens = (float)atof(v);
        else if (!strcmp(k, "aimLag")) cfg.aimLag = (float)atof(v);
        else if (!strcmp(k, "fov")) cfg.fov = (float)atof(v);
        else if (!strcmp(k, "volume")) cfg.volume = (float)atof(v);
        else if (!strcmp(k, "music")) cfg.music = (float)atof(v);
        else if (!strcmp(k, "autoUpdate")) cfg.autoUpdate = atoi(v);
        else if (!strcmp(k, "channel")) { strncpy(cfg.channel, v, 15); cfg.channel[15] = 0; }
    }
    fclose(f);
    if (cfg.renderH < 120) cfg.renderH = 240;
    if (cfg.resW < 640) cfg.resW = 1280;
    if (cfg.resH < 360) cfg.resH = 720;
}
static void cfgSave(void) {
    ensureDir("Data");
    FILE *f = fopen(P("Data/config.ini"), "w");
    if (!f) return;
    fprintf(f, "# KIRIMACHI configuration (shared by Launcher and game)\n");
    fprintf(f, "resW=%d\nresH=%d\nfullscreen=%d\nvsync=%d\nrenderH=%d\n", cfg.resW, cfg.resH, cfg.fullscreen, cfg.vsync, cfg.renderH);
    fprintf(f, "sens=%.5f\naimLag=%.2f\nfov=%.1f\nvolume=%.2f\nmusic=%.2f\n", cfg.sens, cfg.aimLag, cfg.fov, cfg.volume, cfg.music);
    fprintf(f, "autoUpdate=%d\nchannel=%s\n", cfg.autoUpdate, cfg.channel);
    fclose(f);
}

// ---------------------------------------------------------------- user profile
typedef struct { char name[32]; int kills, deaths, wins, sessions; float best, playtime; unsigned int hints; } Profile;
static Profile prof;
static void profLoad(const char *name) {
    memset(&prof, 0, sizeof prof);
    strncpy(prof.name, name, 31);
    char rel[128]; snprintf(rel, sizeof rel, "Data/Userdata/%s/profile.ini", name);
    FILE *f = fopen(P(rel), "r");
    if (!f) return;
    char line[256], k[64], v[128];
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "%63[^=]=%127s", k, v) != 2) continue;
        if (!strcmp(k, "kills")) prof.kills = atoi(v);
        else if (!strcmp(k, "deaths")) prof.deaths = atoi(v);
        else if (!strcmp(k, "wins")) prof.wins = atoi(v);
        else if (!strcmp(k, "sessions")) prof.sessions = atoi(v);
        else if (!strcmp(k, "best")) prof.best = (float)atof(v);
        else if (!strcmp(k, "playtime")) prof.playtime = (float)atof(v);
        else if (!strcmp(k, "hints")) prof.hints = (unsigned int)strtoul(v, NULL, 10);
    }
    fclose(f);
}
static void profSave(void) {
    if (!prof.name[0]) return;
    ensureDir("Data"); ensureDir("Data/Userdata");
    char rel[128]; snprintf(rel, sizeof rel, "Data/Userdata/%s", prof.name); ensureDir(rel);
    snprintf(rel, sizeof rel, "Data/Userdata/%s/profile.ini", prof.name);
    FILE *f = fopen(P(rel), "w");
    if (!f) return;
    fprintf(f, "name=%s\nkills=%d\ndeaths=%d\nwins=%d\nsessions=%d\nbest=%.1f\nplaytime=%.1f\nhints=%u\n",
            prof.name, prof.kills, prof.deaths, prof.wins, prof.sessions, prof.best, prof.playtime, prof.hints);
    fclose(f);
}

// ---------------------------------------------------------------- UI  (Silent Aero)
// Shapes are drawn into a low-resolution layer (screen / aK) and upscaled nearest-neighbour, so every widget reads as a
// chunky, hard-banded mock-3D object. Text is queued and drawn afterwards at native resolution so it stays legible.
#include "rlgl.h"
static Font aFont, aFontB, aFontJ; static Texture2D aKanji;   // kanji strip (Data/UI/kanji.png): 北 東 南 西 残 光 青 龍
static bool aFontOwn = false, aFontJOwn = false;
static float aU = 1.0f;
static bool aClickUsed = false;
static Sound aBlip; static bool aBlipOk = false;
static Sound aErr; static bool aErrOk = false;   // played when a greyed-out button is clicked
static void aMakeErr(void) {   // two short low buzzes, the second lower: "not available"
    int sr = 22050, n = sr * 30 / 100; short *d = (short *)MemAlloc(n * sizeof(short));
    for (int i = 0; i < n; i++) { float t = (float)i / sr, f = t < 0.13f ? 220 : 165, lt = t < 0.13f ? t : t - 0.15f;
        float env = (t < 0.13f || t > 0.15f) ? expf(-lt * 18) * fminf(1, lt * 400) : 0;
        float sq = sinf(t * 2 * PI * f) + 0.35f * sinf(t * 2 * PI * f * 2) + 0.2f * (sinf(t * 2 * PI * f * 3) > 0 ? 1 : -1);
        d[i] = (short)(sq * env * 0.32f * 32767); }
    Wave w = {(unsigned)n, (unsigned)sr, 16, 1, d}; aErr = LoadSoundFromWave(w); UnloadWave(w); aErrOk = true;
}
static int aCorrupt = 0;            // 0 afterglow, 1 sadness, 2 depression, 3 otherworld

// palette
#define C_ASH      (Color){18, 21, 22, 255}
#define C_STEEL    (Color){70, 88, 91, 255}
#define C_TEAL     (Color){47, 93, 94, 255}
#define C_BONE     (Color){216, 210, 196, 255}
#define C_BONE2    (Color){180, 174, 161, 255}
#define C_AMBER    (Color){176, 122, 62, 255}
#define C_CYAN     (Color){111, 214, 208, 255}
#define C_RED      (Color){163, 38, 30, 255}
#define C_FOG      (Color){185, 179, 165, 255}
#define C_RUST     (Color){107, 46, 26, 255}
#define C_TALLOW   (Color){201, 179, 138, 255}
#define C_GEL      (Color){80, 112, 114, 255}

static void aeroLoadFont(void) {
    char ur[700], ub[700]; snprintf(ur, sizeof ur, "%s", P("Data/UI/ui.ttf")); snprintf(ub, sizeof ub, "%s", P("Data/UI/ui_bold.ttf"));
    const char *reg[] = {"C:/Windows/Fonts/segoeui.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", ur, NULL};
    const char *bold[] = {"C:/Windows/Fonts/segoeuib.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", ub, NULL};
    aFont = GetFontDefault(); aFontB = aFont; aFontJ = aFont;
    int cps[110], nc = 0; for (int c = 32; c < 127; c++) cps[nc++] = c; cps[nc++] = 0x14C; cps[nc++] = 0x14D; cps[nc++] = 0x16B; cps[nc++] = 0x16A; cps[nc++] = 0xB7; cps[nc++] = 0x2014;
    for (int i = 0; reg[i]; i++) if (FileExists(reg[i])) { aFont = LoadFontEx(reg[i], 48, cps, nc); aFontOwn = true; break; }
    aFontB = aFont;
    for (int i = 0; bold[i]; i++) if (FileExists(bold[i])) { aFontB = LoadFontEx(bold[i], 96, cps, nc); break; }
    if (FileExists(P("Data/UI/kanji.png"))) { aKanji = LoadTexture(P("Data/UI/kanji.png")); SetTextureFilter(aKanji, TEXTURE_FILTER_BILINEAR); aFontJOwn = aKanji.id > 0; }
    SetTextureFilter(aFont.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(aFontB.texture, TEXTURE_FILTER_BILINEAR);
}

// ---- low-res layer + deferred text
static RenderTexture2D aRT; static int aRTw = 0, aRTh = 0, aK = 2; static bool aLayerOn = false;
typedef struct { char s[200]; float x, y, size; Color c; int font; } AQ;
#define AQMAX 600
static AQ aQ[AQMAX]; static int aQn = 0;
static void aeroBegin(float refH) { aU = GetScreenHeight() / refH; aClickUsed = false; }
static void aLayerBegin(void) {
    int SW = GetScreenWidth(), SH = GetScreenHeight();
    aK = (int)roundf(SH / 360.0f); if (aK < 1) aK = 1;
    int w = (SW + aK - 1) / aK, h = (SH + aK - 1) / aK;
    if (w != aRTw || h != aRTh) { if (aRTw) UnloadRenderTexture(aRT); aRT = LoadRenderTexture(w, h); SetTextureFilter(aRT.texture, TEXTURE_FILTER_POINT); aRTw = w; aRTh = h; }
    BeginTextureMode(aRT);
    ClearBackground((Color){0, 0, 0, 0});
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    rlPushMatrix(); rlScalef(1.0f / aK, 1.0f / aK, 1);
    aLayerOn = true; aQn = 0;
}
static void aFlushText(float alpha);
static void aLayerEnd(float alpha) {
    if (!aLayerOn) return;
    rlDrawRenderBatchActive();
    rlPopMatrix(); EndBlendMode(); EndTextureMode();
    aLayerOn = false;
    unsigned char a = (unsigned char)(255 * fminf(1, fmaxf(0, alpha)));
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    DrawTexturePro(aRT.texture, (Rectangle){0, 0, (float)aRTw, -(float)aRTh}, (Rectangle){0, 0, (float)aRTw * aK, (float)aRTh * aK}, (Vector2){0, 0}, 0, (Color){a, a, a, a});
    EndBlendMode();
    aFlushText(alpha);
}
static Font aFontOf(int f) { return f == 2 ? aFontJ : f == 1 ? aFontB : aFont; }
static const int aKanjiCp[8] = {0x5317, 0x6771, 0x5357, 0x897F, 0x6B8B, 0x5149, 0x9752, 0x9F8D};
static void aRawText(const char *s, float x, float y, float size, Color c, int font) {
    if (font == 2) {   // kanji from the baked glyph strip
        float gx = x; const char *p = s;
        while (*p) { int n = 0; int cp = GetCodepointNext(p, &n); p += n;
            for (int k = 0; k < 8; k++) if (aKanjiCp[k] == cp) {
                Rectangle src = {k * 64.0f, 0, 64, 64};
                DrawTexturePro(aKanji, src, (Rectangle){roundf(gx + 1.5f * aU), roundf(y + 1.5f * aU), size, size}, (Vector2){0, 0}, 0, (Color){0, 0, 0, (unsigned char)(c.a * 0.7f)});
                DrawTexturePro(aKanji, src, (Rectangle){roundf(gx), roundf(y), size, size}, (Vector2){0, 0}, 0, c); }
            gx += size; }
        return;
    }
    Font f = aFontOf(font);
    float lum = (0.299f * c.r + 0.587f * c.g + 0.114f * c.b) / 255.0f;
    if (lum > 0.45f) {   // light text: a firm dark outline so it reads on any glass
        Color o = (Color){0, 0, 0, (unsigned char)(c.a * 0.85f)};
        DrawTextEx(f, s, (Vector2){roundf(x + 1.5f * aU), roundf(y + 1.5f * aU)}, size, size * 0.04f, o);
        DrawTextEx(f, s, (Vector2){roundf(x + 1.0f * aU), roundf(y)}, size, size * 0.04f, (Color){0, 0, 0, (unsigned char)(c.a * 0.5f)});
        DrawTextEx(f, s, (Vector2){roundf(x - 0.6f * aU), roundf(y + 0.6f * aU)}, size, size * 0.04f, (Color){0, 0, 0, (unsigned char)(c.a * 0.4f)});
    }
    DrawTextEx(f, s, (Vector2){roundf(x), roundf(y)}, size, size * 0.04f, c);
}
static void aFlushText(float alpha) {
    for (int i = 0; i < aQn; i++) { Color c = aQ[i].c; c.a = (unsigned char)(c.a * fminf(1, fmaxf(0, alpha))); aRawText(aQ[i].s, aQ[i].x, aQ[i].y, aQ[i].size, c, aQ[i].font); }
    aQn = 0;
}
static void aTextF(const char *s, float x, float y, float size, Color c, int font) {
    if (!s || !s[0]) return;
    if (aLayerOn) { if (aQn < AQMAX) { AQ *q = &aQ[aQn++]; snprintf(q->s, sizeof q->s, "%s", s); q->x = x; q->y = y; q->size = size * aU; q->c = c; q->font = font; } }
    else aRawText(s, x, y, size * aU, c, font);
}
static Vector2 aMeasure(const char *s, float size, bool bold) { Font f = bold ? aFontB : aFont; return MeasureTextEx(f, s, size * aU, size * aU * 0.04f); }
static void aText(const char *s, float x, float y, float size, Color c, bool bold) { aTextF(s, x, y, size, c, bold ? 1 : 0); }
static void aTextC(const char *s, float cx, float y, float size, Color c, bool bold) { Vector2 m = aMeasure(s, size, bold); aText(s, cx - m.x / 2, y, size, c, bold); }

// ---- input indirection (lets the scripted demo recorder drive the UI)
static bool vidOn = false; static Vector2 vidMouse = {0, 0}; static bool vidPress = false, vidDown = false;
static bool aIn3D = false, aMO = false; static Vector2 aMOp = {0, 0};   // drawing into a 3D panel's content texture
typedef struct { int kind; Rectangle r; int state; Color base; float v; } AW3;   // kind 0 gel button, 1 checkbox cube, 2 slider thumb, 3 caption close
#define AW3MAX 96
static AW3 aW3[AW3MAX]; static int aW3n = 0;
static void aW3Add(int kind, Rectangle r, int state, Color base, float v) { if (aW3n < AW3MAX) aW3[aW3n++] = (AW3){kind, r, state, base, v}; }
static Vector2 aRealMouse(void) { return vidOn ? vidMouse : GetMousePosition(); }
static Vector2 aMousePos(void) { return aMO ? aMOp : vidOn ? vidMouse : GetMousePosition(); }
static bool aPadClick = false, aPadDown = false;   // the game's gamepad A button acts as the left mouse button in menus
// ---- gamepad focus: in menus the D-pad / left stick step through the buttons, toggles and sliders in the order they are drawn,
// A presses the one in focus, left / right move a slider. The game calls aNavBegin once a frame before drawing its UI.
static bool aNavOn = false, aNavA = false; static int aNavIdx = 0, aNavN = 0, aNavCur = 0, aNavSide = 0;
#define A_NAVMAX 256
static Rectangle aNavR[A_NAVMAX]; static bool aNavSl[A_NAVMAX], aNavNextSl = false;   // last frame's focusable widgets: where they are, which are sliders
static int aNavPick(int dx, int dy) {   // the nearest widget in that direction from the focused one (by position on screen, not drawing order)
    if (aNavIdx < 0 || aNavIdx >= aNavN || aNavIdx >= A_NAVMAX) return -1;
    Rectangle c = aNavR[aNavIdx]; float cx = c.x + c.width / 2, cy = c.y + c.height / 2, best = 1e9f; int bi = -1;
    for (int i = 0; i < aNavN && i < A_NAVMAX; i++) { if (i == aNavIdx) continue; Rectangle r = aNavR[i]; float x = r.x + r.width / 2, y = r.y + r.height / 2, ddx = x - cx, ddy = y - cy;
        if (dy) { if (ddy * dy < 4) continue; float sc = fabsf(ddy) + 2.5f * fabsf(ddx); if (sc < best) { best = sc; bi = i; } }
        else { if (ddx * dx < 4 || fabsf(ddy) > c.height * 1.5f + 20) continue; float sc = fabsf(ddx) + 3 * fabsf(ddy); if (sc < best) { best = sc; bi = i; } } }
    return bi;
}
static void aNavBegin(bool on, int move, int side, bool a) {
    aNavN = aNavCur; aNavCur = 0; aNavOn = on; aNavA = on && a; aNavSide = on ? side : 0;
    if (aNavN <= 0) { aNavIdx = 0; return; }
    if (aNavIdx >= aNavN) aNavIdx = aNavN - 1;
    if (on && move) { int p = aNavPick(0, move); if (p >= 0) aNavIdx = p; else { aNavIdx += move; if (aNavIdx < 0) aNavIdx = aNavN - 1; if (aNavIdx >= aNavN) aNavIdx = 0; } }   // up / down: the widget above or below
    if (on && side && !(aNavIdx < A_NAVMAX && aNavSl[aNavIdx])) { int p = aNavPick(side, 0); if (p >= 0) aNavIdx = p; aNavSide = 0; }   // left / right: a slider moves, anything else steps sideways
}
static void aRRL(Rectangle r, float rpx, float px, Color c);
static Rectangle aGrow(Rectangle r, float d);
static bool aNavFocus(Rectangle r) {   // registers a widget; true when it has the pad's focus (and draws the focus ring)
    int id = aNavCur++; if (id < A_NAVMAX) { aNavR[id] = r; aNavSl[id] = aNavNextSl; } aNavNextSl = false; if (!aNavOn || id != aNavIdx) return false;
    float k = 0.6f + 0.4f * sinf((float)GetTime() * 6); aRRL(aGrow(r, 3 * aU), 6 * aU, 2, (Color){111, 214, 208, (unsigned char)(150 + 100 * k)}); return true;
}
static bool aMPressed(void) { return vidOn ? vidPress : (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || aPadClick); }
static bool aMDown(void) { return vidOn ? vidDown : (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || aPadDown); }
static void aDrawCursor(void) {
    if (!vidOn) return;
    Vector2 p = vidMouse; float s = 1.15f * aU;
    Vector2 a = p, b = {p.x, p.y + 20 * s}, c = {p.x + 5 * s, p.y + 15.5f * s}, d = {p.x + 8.5f * s, p.y + 23 * s}, e = {p.x + 11.5f * s, p.y + 21.5f * s}, f = {p.x + 8 * s, p.y + 14.5f * s}, g = {p.x + 14 * s, p.y + 14 * s};
    Vector2 pts[7] = {a, b, c, d, e, f, g};
    DrawTriangle(a, b, g, C_BONE); DrawTriangle(c, d, e, C_BONE); DrawTriangle(c, e, f, C_BONE);
    for (int i = 0; i < 7; i++) DrawLineEx(pts[i], pts[(i + 1) % 7], 1.3f * s, BLACK);
}

// ---- drawing helpers (screen-space coordinates; the layer matrix maps them to low-res pixels)
static Color aLerpC(Color a, Color b, float t) {
    return (Color){(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t), (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t)};
}
static Color aMul(Color c, float k) { return (Color){(unsigned char)fminf(255, c.r * k), (unsigned char)fminf(255, c.g * k), (unsigned char)fminf(255, c.b * k), c.a}; }
static Color aAdd(Color c, int k) { return (Color){(unsigned char)fminf(255, fmaxf(0, c.r + k)), (unsigned char)fminf(255, fmaxf(0, c.g + k)), (unsigned char)fminf(255, fmaxf(0, c.b + k)), c.a}; }
static Color aA(Color c, int a) { c.a = (unsigned char)a; return c; }
static unsigned int aHash(const char *s, float x) { unsigned int h = 2166136261u; if (s) for (; *s; s++) h = (h ^ (unsigned char)*s) * 16777619u; h ^= (unsigned int)(x * 13); return h * 2654435761u; }
static float aPx(void) { return aLayerOn ? (float)aK : 1.0f; }    // one low-res pixel in screen units
static Rectangle aClip = {0, 0, 0, 0}; static bool aClipOn = false;
static void aScissor(float x, float y, float w, float h) {
    x = floorf(x); y = floorf(y); w = ceilf(w); h = ceilf(h);
    if (aClipOn) {
        float x1 = fmaxf(x, aClip.x), y1 = fmaxf(y, aClip.y), x2 = fminf(x + w, aClip.x + aClip.width), y2 = fminf(y + h, aClip.y + aClip.height);
        x = x1; y = y1; w = fmaxf(0, x2 - x1); h = fmaxf(0, y2 - y1);
    }
    if (aLayerOn) { rlDrawRenderBatchActive(); float k = (float)aK; BeginScissorMode((int)floorf(x / k), (int)floorf(y / k), (int)ceilf(w / k) + 1, (int)ceilf(h / k) + 1); }
    else BeginScissorMode((int)x, (int)y, (int)w, (int)h);
}
static void aScissorEnd(void) {
    EndScissorMode();
    if (aClipOn) aScissor(aClip.x, aClip.y, aClip.width, aClip.height);
}
static void aClipBegin(Rectangle r) { aClip = r; aClipOn = false; aScissor(r.x, r.y, r.width, r.height); aClipOn = true; }
static void aClipEnd(void) { aClipOn = false; EndScissorMode(); }
static float aRnd(Rectangle r, float px) { float m = fminf(r.width, r.height); return m > 0 ? fminf(1.0f, 2 * px / m) : 0; }
// rounded rectangle with hard vertical colour bands
static void aGrad(Rectangle r, float rnd, Color top, Color bot, int bands) {
    if (r.width <= 0 || r.height <= 0) return;
    float sh = r.height / bands;
    for (int i = 0; i < bands; i++) {
        float y0 = r.y + i * sh, y1 = (i == bands - 1) ? r.y + r.height : r.y + (i + 1) * sh;
        aScissor(r.x - 2, y0, r.width + 4, y1 - y0);
        DrawRectangleRounded(r, rnd, 8, aLerpC(top, bot, bands > 1 ? (float)i / (bands - 1) : 0));
        aScissorEnd();
    }
}
static void aRR(Rectangle r, float rpx, Color c) { DrawRectangleRounded(r, aRnd(r, rpx), 8, c); }
static void aRRL(Rectangle r, float rpx, float px, Color c) { DrawRectangleRoundedLinesEx(r, aRnd(r, rpx), 8, px * aPx(), c); }
static void aPunch(Rectangle r, float rpx) {   // clear a hole in the panel texture so the 3D widget behind shows through
    rlDrawRenderBatchActive();
    rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD); BeginBlendMode(BLEND_CUSTOM);
    aRR(r, rpx, (Color){0, 0, 0, 0});
    rlDrawRenderBatchActive();
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD); BeginBlendMode(BLEND_CUSTOM_SEPARATE);
}
static void aLine(float x1, float y1, float x2, float y2, float px, Color c) { DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, px * aPx(), c); }
static Rectangle aGrow(Rectangle r, float d) { return (Rectangle){r.x - d, r.y - d, r.width + 2 * d, r.height + 2 * d}; }
// lit chrome/gel ellipse: banded concentric shading + hard specular dot
static void aBlob(float cx, float cy, float rx, float ry, Color base, bool spec) {
    DrawEllipse((int)cx, (int)(cy + ry * 0.12f), rx * 1.08f, ry * 1.08f, (Color){8, 10, 10, base.a});
    DrawEllipse((int)cx, (int)cy, rx, ry, aMul(base, 0.7f));
    DrawEllipse((int)(cx - rx * 0.08f), (int)(cy - ry * 0.1f), rx * 0.82f, ry * 0.78f, base);
    DrawEllipse((int)(cx - rx * 0.16f), (int)(cy - ry * 0.22f), rx * 0.55f, ry * 0.5f, aMul(base, 1.25f));
    if (spec) DrawEllipse((int)(cx - rx * 0.32f), (int)(cy - ry * 0.4f), fmaxf(rx * 0.18f, aPx()), fmaxf(ry * 0.16f, aPx()), (Color){255, 255, 255, (unsigned char)(base.a * 0.9f)});
}

// ---- Windows Vista style faces: two-tone glass (bright upper half, deeper lower half), 1px outer rim, white inner rim
static void aVistaFace(Rectangle b, float rad, Color rim, Color t0, Color t1, Color b0, Color b1, unsigned char inner) {
    float s = aU, px = fmaxf(1, roundf(s));
    aRR(b, rad, rim);
    Rectangle in = {b.x + px, b.y + px, b.width - 2 * px, b.height - 2 * px};
    aRR(in, fmaxf(0, rad - px), b1);
    float hh = floorf(in.height * 0.48f), ins = fmaxf(px, roundf(rad * 0.45f));
    DrawRectangleGradientV((int)(in.x + ins), (int)in.y, (int)(in.width - 2 * ins), (int)hh, t0, t1);
    DrawRectangleGradientV((int)in.x, (int)(in.y + ins), (int)ins, (int)(hh - ins), t0, t1); DrawRectangleGradientV((int)(in.x + in.width - ins), (int)(in.y + ins), (int)ins, (int)(hh - ins), t0, t1);
    DrawRectangleGradientV((int)in.x, (int)(in.y + hh), (int)in.width, (int)(in.height - hh - ins), b0, b1);
    DrawRectangleGradientV((int)(in.x + ins), (int)(in.y + in.height - ins), (int)(in.width - 2 * ins), (int)ins, b1, b1);
    if (inner) aRRL(in, fmaxf(0, rad - px), 1, (Color){255, 255, 255, inner});
}
// push button: silver at rest, sky-blue on hover, deeper blue pressed (rust-red when the town turns)
static void aVistaButton(Rectangle b, int st, bool enabled) {
    float r = 3 * aU;
    if (!enabled) { aVistaFace(b, r, (Color){173, 178, 181, 255}, (Color){244, 244, 244, 255}, (Color){244, 244, 244, 255}, (Color){236, 236, 236, 255}, (Color){236, 236, 236, 255}, 120); return; }
    if (aCorrupt == 3) { if (st == 0) aVistaFace(b, r, (Color){92, 40, 26, 255}, (Color){214, 170, 150, 255}, (Color){200, 150, 128, 255}, (Color){168, 104, 82, 255}, (Color){150, 82, 60, 255}, 110);
        else aVistaFace(b, r, (Color){120, 30, 18, 255}, (Color){250, 190, 160, 255}, (Color){236, 160, 130, 255}, (Color){214, 100, 70, 255}, (Color){190, 70, 46, 255}, 140); return; }
    if (st == 2) aVistaFace(b, r, (Color){44, 98, 139, 255}, (Color){229, 244, 252, 255}, (Color){196, 229, 246, 255}, (Color){152, 209, 239, 255}, (Color){104, 179, 219, 255}, 90);
    else if (st == 1) { aRRL(aGrow(b, 1 * aU), r + aU, 1, (Color){120, 200, 250, 150}); aVistaFace(b, r, (Color){60, 127, 177, 255}, (Color){234, 246, 253, 255}, (Color){217, 240, 252, 255}, (Color){190, 230, 253, 255}, (Color){167, 217, 245, 255}, 200); }
    else aVistaFace(b, r, (Color){112, 112, 112, 255}, (Color){242, 242, 242, 255}, (Color){235, 235, 235, 255}, (Color){221, 221, 221, 255}, (Color){207, 207, 207, 255}, 190);
}
// caption buttons: minimise and maximise in smoked glass, a wide red close; joined in one strip hanging from the frame's top edge
static bool aVistaCaps(Rectangle r) {
    float s = aU, h = 19 * s, wC = 44 * s, wM = 26 * s, x1 = r.x + r.width - 14 * s, y = r.y + 1 * s;
    Rectangle grp = {x1 - wC - 2 * wM, y, wC + 2 * wM, h};
    aRR(aGrow(grp, 1 * s), 4 * s, (Color){10, 14, 18, 230});
    Vector2 m = aMousePos(); bool closed = false;
    for (int i = 0; i < 3; i++) {
        float bw = i == 0 ? wC : wM, bx = i == 0 ? x1 - wC : i == 1 ? x1 - wC - wM : x1 - wC - 2 * wM;
        Rectangle b = {bx, y, bw, h}; bool hov = CheckCollisionPointRec(m, b), dn = hov && aMDown();
        if (i == 0) { if (hov) DrawRectangleRounded(aGrow(b, 3 * s), 0.6f, 8, (Color){255, 110, 60, 90});
            aVistaFace(b, 3 * s, (Color){70, 16, 10, 255}, hov ? (Color){255, 170, 150, 255} : (Color){232, 150, 128, 255}, hov ? (Color){240, 120, 96, 255} : (Color){212, 104, 82, 255},
                       dn ? (Color){150, 26, 12, 255} : hov ? (Color){222, 52, 26, 255} : (Color){188, 42, 22, 255}, dn ? (Color){120, 30, 16, 255} : hov ? (Color){244, 110, 60, 255} : (Color){206, 78, 44, 255}, 110); }
        else { if (hov) DrawRectangleRounded(aGrow(b, 2 * s), 0.6f, 8, (Color){120, 200, 255, 70});
            aVistaFace(b, 3 * s, (Color){24, 32, 40, 255}, hov ? (Color){190, 226, 250, 230} : (Color){176, 190, 200, 200}, hov ? (Color){150, 200, 236, 230} : (Color){130, 146, 158, 200},
                       hov ? (Color){40, 110, 170, 240} : (Color){44, 58, 70, 220}, hov ? (Color){80, 170, 230, 240} : (Color){76, 96, 110, 220}, 90); }
        float cx = b.x + b.width / 2, cy = b.y + b.height / 2, k = 3.6f * s; Color gc = WHITE, sh = (Color){0, 0, 0, 160};
        for (int pass = 0; pass < 2; pass++) { float o = pass == 0 ? 1 * s : 0; Color c = pass == 0 ? sh : gc;
            if (i == 0) { aLine(cx - k + o, cy - k + o, cx + k + o, cy + k + o, 2, c); aLine(cx - k + o, cy + k + o, cx + k + o, cy - k + o, 2, c); }
            else if (i == 1) { aRRL((Rectangle){cx - k + o, cy - k * 0.85f + o, 2 * k, 1.7f * k}, 0, 1, c); DrawRectangle((int)(cx - k + o), (int)(cy - k * 0.85f + o), (int)(2 * k), (int)(2 * s), c); }
            else DrawRectangle((int)(cx - k + o), (int)(cy + k * 0.5f + o), (int)(2 * k), (int)(2 * s), c); }
        if (i == 0 && hov && !aClickUsed && aMPressed()) { aClickUsed = true; closed = true; if (aBlipOk) PlaySound(aBlip); }
    }
    return closed;
}
// ---- glass window slab; returns true when the caption close button is clicked
static void aWellDecor(Rectangle r, Rectangle in, unsigned int h);
static bool aWindow(Rectangle r, const char *title) {
    float s = aU, cap = 40 * s, rad = 9 * s;
    unsigned int h = aHash(title, 0);
    if (aIn3D) {   // the slab, caption bar and capsules are meshes; the texture carries the screen, title and close hit-test
        Rectangle cb = {r.x + 6 * s, r.y + 5 * s, r.width - 12 * s, cap - 9 * s};
        if (title && title[0]) aTextF(title, cb.x + 12 * s, cb.y + (cb.height - 17 * s) / 2 - 1 * s, 17, aCorrupt == 3 ? C_TALLOW : (Color){236, 230, 216, 255}, 0);
        Rectangle in = {r.x + 8 * s, r.y + cap, r.width - 16 * s, r.height - cap - 8 * s};
        Color wt = aCorrupt == 3 ? (Color){38, 20, 16, 252} : (Color){26, 36, 38, 252}, wb = aCorrupt == 3 ? (Color){16, 8, 7, 255} : (Color){10, 14, 15, 255};
        aGrad(in, aRnd(in, 4 * s), wt, wb, 6);
        aWellDecor(r, in, h);
        if (title && title[0]) return aVistaCaps(r);
        return false;
    }
    Color frame = aCorrupt == 3 ? (Color){110, 52, 34, 255} : aCorrupt == 2 ? (Color){96, 98, 96, 255} : aCorrupt == 1 ? (Color){100, 110, 110, 255} : (Color){92, 112, 116, 255};
    // shadow plate + dark rim
    aRR((Rectangle){r.x + 7 * s, r.y + 10 * s, r.width, r.height}, rad, (Color){6, 8, 8, 150});
    aRR(aGrow(r, 2 * s), rad + 2 * s, (Color){14, 18, 19, 255});
    // frame body: banded steel, split gloss on the upper half
    aGrad(r, aRnd(r, rad), aMul(frame, 1.3f), aMul(frame, 0.62f), 5);
    aScissor(r.x, r.y, r.width, r.height * 0.5f);
    aRR(aGrow(r, -1 * s), rad, (Color){255, 255, 255, 22});
    aScissorEnd();
    // bevels
    aLine(r.x + rad, r.y + 1.5f * s, r.x + r.width - rad, r.y + 1.5f * s, 1, (Color){200, 214, 210, 200});
    aLine(r.x + 1.5f * s, r.y + rad, r.x + 1.5f * s, r.y + r.height - rad, 1, (Color){150, 166, 164, 150});
    aLine(r.x + rad, r.y + r.height - 1.5f * s, r.x + r.width - rad, r.y + r.height - 1.5f * s, 1, (Color){16, 20, 22, 255});
    // raised caption bar
    Rectangle cb = {r.x + 6 * s, r.y + 5 * s, r.width - 12 * s, cap - 9 * s};
    aRR((Rectangle){cb.x, cb.y + 2 * s, cb.width, cb.height}, 6 * s, (Color){12, 16, 17, 200});
    Color capc = aCorrupt == 3 ? (Color){130, 70, 50, 255} : (Color){120, 140, 142, 255};
    aGrad(cb, aRnd(cb, 6 * s), aMul(capc, 1.18f), aMul(capc, 0.78f), 3);
    aScissor(cb.x, cb.y, cb.width, cb.height * 0.48f); aRR(cb, 6 * s, (Color){255, 255, 255, 34}); aScissorEnd();
    if (title && title[0]) aTextF(title, cb.x + 12 * s, cb.y + (cb.height - 17 * s) / 2 - 1 * s, 17, aCorrupt == 3 ? C_TALLOW : (Color){236, 230, 216, 255}, 0);
    bool closed = title && title[0] ? aVistaCaps(r) : false;
    if (aCorrupt == 3) {   // an eye opens in the caption bar and follows the cursor
        float ex = cb.x + cb.width * 0.55f, ey = cb.y + cb.height / 2;
        Vector2 m = aMousePos(); float dx = Clamp((m.x - ex) / 400, -1, 1), dy = Clamp((m.y - ey) / 300, -1, 1);
        DrawEllipse((int)ex, (int)ey, 26 * s, 9 * s, (Color){225, 205, 165, 255});
        DrawEllipse((int)(ex + dx * 12 * s), (int)(ey + dy * 3 * s), 7 * s, 7 * s, (Color){70, 14, 10, 255});
        DrawEllipse((int)(ex + dx * 12 * s), (int)(ey + dy * 3 * s), 3 * s, 3 * s, (Color){12, 4, 3, 255});
    }
    // recessed content well
    Rectangle in = {r.x + 8 * s, r.y + cap, r.width - 16 * s, r.height - cap - 8 * s};
    aRR(aGrow(in, 1.5f * s), 5 * s, (Color){8, 10, 10, 255});
    Color wt = aCorrupt == 3 ? (Color){38, 20, 16, 250} : (Color){26, 36, 38, 248}, wb = aCorrupt == 3 ? (Color){16, 8, 7, 252} : (Color){10, 14, 15, 250};
    aGrad(in, aRnd(in, 4 * s), wt, wb, 6);
    aScissor(in.x, in.y, in.width, in.height);
    DrawRectangle((int)in.x, (int)in.y, (int)in.width, (int)(4 * s), (Color){0, 0, 0, 110});
    {   // one diagonal reflection band across the glass
        float x0 = in.x + in.width * 0.18f, wv = in.width * 0.10f, sk = in.height * 0.35f;
        DrawTriangle((Vector2){x0 + sk, in.y}, (Vector2){x0, in.y + in.height}, (Vector2){x0 + wv, in.y + in.height}, (Color){255, 255, 255, 7});
        DrawTriangle((Vector2){x0 + sk, in.y}, (Vector2){x0 + wv, in.y + in.height}, (Vector2){x0 + sk + wv, in.y}, (Color){255, 255, 255, 7});
    }
    aScissorEnd();
    // rust stain and drips on the lower frame lip
    DrawRectangle((int)(r.x + r.width * (0.12f + (h % 40) / 100.0f)), (int)(r.y + r.height - 6 * s), (int)(r.width * 0.3f), (int)(4 * s), (Color){92, 58, 32, 150});
    for (int k = 0; k < 4; k++) {
        float dx = r.x + r.width * (0.1f + ((h >> (k * 4)) % 80) / 100.0f), dl = (6 + ((h >> (k * 3)) % 3) * 6) * s;
        DrawRectangle((int)dx, (int)(r.y + r.height - 2 * s), (int)fmaxf(2 * s, aPx()), (int)dl, (Color){80, 48, 26, 200});
        DrawRectangle((int)dx, (int)(r.y + r.height - 2 * s + dl), (int)fmaxf(2 * s, aPx()), (int)fmaxf(2 * s, aPx()), (Color){60, 34, 18, 220});
    }
    // crack (one per window, fixed by its title; grows with corruption)
    {
        float cr = 1.0f + aCorrupt * 0.6f, cx = in.x + in.width - (18 + (h % 30)) * s, cy = in.y + (10 + ((h >> 8) % 12)) * s;
        float segs[6][3] = {{0, 0, -2.6f}, {-24, 14, -2.2f}, {-40, 34, -1.9f}, {-10, 20, 2.3f}, {12, 34, 1.2f}, {-58, 48, 2.9f}};
        for (int k = 0; k < 6; k++) { float L = 12 * s * cr, a = segs[k][2]; float x = cx + segs[k][0] * 0.45f * s * cr, y = cy + segs[k][1] * 0.45f * s * cr;
            aLine(x, y, x + cosf(a) * L, y + sinf(a) * L, 1, (Color){235, 230, 215, 110}); }
    }
    if (aCorrupt >= 2) {   // grime creeping in from the corners
        aScissor(in.x, in.y, in.width, in.height);
        for (int k = 0; k < 10; k++) {
            float fx = (k % 2) ? in.x + in.width - (k / 2) * 10 * s : in.x + (k / 2) * 10 * s, fy = (k % 4 < 2) ? in.y + in.height - (k / 4) * 8 * s : in.y + (k / 4) * 8 * s;
            DrawEllipse((int)fx, (int)fy, (24 + (k % 3) * 10) * s, (14 + (k % 2) * 6) * s, aCorrupt == 3 ? (Color){70, 18, 12, 150} : (Color){200, 205, 200, 40});
        }
        aScissorEnd();
    }
    if (aCorrupt == 3) {   // rusted mesh and wet drips
        aScissor(r.x, r.y, r.width, r.height);
        for (float k = -r.height; k < r.width; k += 28 * s) { aLine(r.x + k, r.y, r.x + k + r.height, r.y + r.height, 1, (Color){60, 26, 16, 90}); aLine(r.x + k + r.height, r.y, r.x + k, r.y + r.height, 1, (Color){60, 26, 16, 90}); }
        aScissorEnd();
        for (int k = 0; k < 7; k++) { float dx = r.x + 20 * s + k * (r.width - 40 * s) / 6, dl = (14 + (k % 3) * 12) * s + fmodf((float)GetTime() * 6 * s + k * 9, 10 * s);
            DrawRectangle((int)dx, (int)(r.y + r.height), (int)(3 * s), (int)dl, (Color){90, 20, 14, 230}); DrawEllipse((int)(dx + 1.5f * s), (int)(r.y + r.height + dl), 3 * s, 4 * s, (Color){110, 22, 16, 255}); }
    }
    return closed;
}
static void aWellDecor(Rectangle r, Rectangle in, unsigned int h) {
    float s = aU;
    aScissor(in.x, in.y, in.width, in.height);
    DrawRectangle((int)in.x, (int)in.y, (int)in.width, (int)(4 * s), (Color){0, 0, 0, 110});
    { float x0 = in.x + in.width * 0.18f, wv = in.width * 0.10f, sk = in.height * 0.35f;
      DrawTriangle((Vector2){x0 + sk, in.y}, (Vector2){x0, in.y + in.height}, (Vector2){x0 + wv, in.y + in.height}, (Color){255, 255, 255, 7});
      DrawTriangle((Vector2){x0 + sk, in.y}, (Vector2){x0 + wv, in.y + in.height}, (Vector2){x0 + sk + wv, in.y}, (Color){255, 255, 255, 7}); }
    aScissorEnd();
    { float cr = 1.0f + aCorrupt * 0.6f, cx = in.x + in.width - (18 + (h % 30)) * s, cy = in.y + (10 + ((h >> 8) % 12)) * s;
      float segs[6][3] = {{0, 0, -2.6f}, {-24, 14, -2.2f}, {-40, 34, -1.9f}, {-10, 20, 2.3f}, {12, 34, 1.2f}, {-58, 48, 2.9f}};
      for (int k = 0; k < 6; k++) { float L = 12 * s * cr, a = segs[k][2]; float x = cx + segs[k][0] * 0.45f * s * cr, y = cy + segs[k][1] * 0.45f * s * cr;
          aLine(x, y, x + cosf(a) * L, y + sinf(a) * L, 1, (Color){235, 230, 215, 110}); } }
    if (aCorrupt >= 2) {
        aScissor(in.x, in.y, in.width, in.height);
        for (int k = 0; k < 10; k++) {
            float fx = (k % 2) ? in.x + in.width - (k / 2) * 10 * s : in.x + (k / 2) * 10 * s, fy = (k % 4 < 2) ? in.y + in.height - (k / 4) * 8 * s : in.y + (k / 4) * 8 * s;
            DrawEllipse((int)fx, (int)fy, (24 + (k % 3) * 10) * s, (14 + (k % 2) * 6) * s, aCorrupt == 3 ? (Color){70, 18, 12, 150} : (Color){200, 205, 200, 40});
        }
        aScissorEnd();
    }
    (void)r;
}
static void aPanel(Rectangle r) { (void)aWindow(r, NULL); }
// small glass slab without a caption (HUD gadgets)
static void aGadget(Rectangle r) {
    float s = aU, rad = 8 * s;
    if (aIn3D) { Rectangle in = aGrow(r, -7 * s); aGrad(in, aRnd(in, 5 * s), (Color){26, 36, 38, 236}, (Color){10, 14, 15, 244}, 4); return; }
    Color frame = aCorrupt == 3 ? (Color){110, 52, 34, 230} : (Color){92, 112, 116, 230};
    aRR((Rectangle){r.x + 4 * s, r.y + 6 * s, r.width, r.height}, rad, (Color){6, 8, 8, 110});
    aRR(aGrow(r, 1.5f * s), rad + 1.5f * s, (Color){14, 18, 19, 230});
    aGrad(r, aRnd(r, rad), aMul(frame, 1.25f), aMul(frame, 0.6f), 4);
    aScissor(r.x, r.y, r.width, r.height * 0.45f); aRR(aGrow(r, -s), rad, (Color){255, 255, 255, 20}); aScissorEnd();
    Rectangle in = aGrow(r, -5 * s);
    aGrad(in, aRnd(in, 5 * s), (Color){26, 36, 38, 225}, (Color){10, 14, 15, 230}, 4);
    aLine(r.x + rad, r.y + 1.5f * s, r.x + r.width - rad, r.y + 1.5f * s, 1, (Color){200, 214, 210, 160});
}
// gel capsule button
static bool aButton(Rectangle r, const char *label, bool enabled) {
    float s = aU;
    Vector2 m = aMousePos();
    bool foc = aNavFocus(r);
    bool hov = enabled && (CheckCollisionPointRec(m, r) || foc), down = hov && aMDown() && !foc;
    if (foc && aNavA && !aClickUsed) { aClickUsed = true; if (enabled) { if (aBlipOk) PlaySound(aBlip); aNavA = false; aVistaButton(r, 2, enabled); return true; } if (aErrOk) PlaySound(aErr); }
    Color base = !enabled ? (Color){70, 74, 74, 255} : aCorrupt == 3 ? (Color){120, 46, 30, 255} : C_GEL;
    if (hov && !down) base = aAdd(base, 26);
    if (down) base = aMul(base, 0.62f);
    (void)base;
    aRR((Rectangle){r.x + 1 * s, r.y + 3 * s, r.width, r.height}, 4 * s, (Color){0, 0, 0, 90});   // soft drop shadow
    aVistaButton(r, down ? 2 : hov ? 1 : 0, enabled);
    float fs = fminf(r.height / aU * 0.42f, 18);
    char lb[96]; snprintf(lb, sizeof lb, "%s%s", label, (aCorrupt == 3 && enabled && hov) ? "?" : "");
    Vector2 ms = aMeasure(lb, fs, true);
    aText(lb, r.x + (r.width - ms.x) / 2 + (down ? 1 * s : 0), r.y + (r.height - ms.y) / 2 + (down ? 1 * s : 0), fs, enabled ? (aCorrupt == 3 ? (Color){40, 8, 4, 255} : (Color){14, 22, 30, 255}) : (Color){131, 131, 131, 255}, true);
    if (hov && !aClickUsed && aMPressed()) { aClickUsed = true; if (aBlipOk) PlaySound(aBlip); return true; }
    if (!enabled && !aClickUsed && CheckCollisionPointRec(m, r) && aMPressed()) { aClickUsed = true; if (aErrOk) PlaySound(aErr); }
    return false;
}
// horizontal vial: glass tube, banded liquid, bright meniscus, sediment
static void aProgress(Rectangle r, float frac, Color liquid) {
    float s = aU; frac = frac < 0 ? 0 : (frac > 1 ? 1 : frac);
    float rad = r.height / 2;
    aRR(aGrow(r, 1.5f * s), rad + 1.5f * s, (Color){10, 14, 15, 255});
    aGrad(r, 1.0f, (Color){24, 30, 31, 235}, (Color){12, 16, 17, 235}, 3);
    if (frac > 0.01f) {
        Rectangle f = {r.x + 1.5f * s, r.y + 1.5f * s, fmaxf((r.width - 3 * s) * frac, r.height - 3 * s), r.height - 3 * s};
        aGrad(f, 1.0f, aMul(liquid, 1.3f), aMul(liquid, 0.6f), 3);
        float wob = sinf((float)GetTime() * 2.2f) * 1.0f * s;
        aLine(f.x + f.width - 1.5f * s + wob, f.y + 1 * s, f.x + f.width - 1.5f * s - wob, f.y + f.height - 1 * s, 1, aAdd(liquid, 80));
        DrawRectangle((int)(f.x + f.width * 0.2f), (int)(f.y + f.height - 2 * s), (int)fmaxf(2 * s, aPx()), (int)fmaxf(1.5f * s, aPx()), (Color){20, 12, 8, 200});
        DrawRectangle((int)(f.x + f.width * 0.55f), (int)(f.y + f.height - 2 * s), (int)fmaxf(2 * s, aPx()), (int)fmaxf(1.5f * s, aPx()), (Color){20, 12, 8, 200});
    }
    aRR((Rectangle){r.x + rad * 0.6f, r.y + 1.5f * s, r.width - rad * 1.2f, fmaxf(r.height * 0.28f, aPx())}, r.height * 0.14f, (Color){255, 255, 255, 46});
    aRRL(r, rad, 1, (Color){180, 186, 184, 110});
}
// vertical vial with chrome caps (HUD)
static void aVial(Rectangle r, float frac, Color liquid) {
    float s = aU; frac = frac < 0 ? 0 : (frac > 1 ? 1 : frac);
    Rectangle tube = {r.x, r.y + 5 * s, r.width, r.height - 10 * s};
    aRR(aGrow(tube, 1.5f * s), r.width / 2, (Color){10, 14, 15, 255});
    aGrad(tube, 1.0f, (Color){22, 28, 29, 220}, (Color){12, 16, 17, 230}, 3);
    if (frac > 0.01f) {
        float lh = fmaxf((tube.height - 3 * s) * frac, tube.width - 3 * s);
        Rectangle f = {tube.x + 1.5f * s, tube.y + tube.height - 1.5f * s - lh, tube.width - 3 * s, lh};
        aScissor(f.x - 2 * s, f.y, f.width + 4 * s, f.height + 2 * s);
        aRR(f, f.width / 2, aMul(liquid, 0.65f));
        aRR((Rectangle){f.x, f.y, f.width * 0.62f, f.height}, f.width / 2, liquid);
        aRR((Rectangle){f.x + f.width * 0.12f, f.y, f.width * 0.28f, f.height}, f.width / 4, aMul(liquid, 1.35f));
        aScissorEnd();
        DrawRectangle((int)f.x, (int)f.y, (int)f.width, (int)fmaxf(2 * s, aPx()), aAdd(liquid, 80));
        DrawRectangle((int)(f.x + f.width * 0.3f), (int)(f.y + f.height - 3 * s), (int)fmaxf(2 * s, aPx()), (int)fmaxf(2 * s, aPx()), (Color){20, 12, 8, 220});
    }
    DrawRectangle((int)(tube.x + tube.width * 0.2f), (int)(tube.y + 4 * s), (int)fmaxf(tube.width * 0.16f, aPx()), (int)(tube.height - 8 * s), (Color){255, 255, 255, 50});
    Color cap = aCorrupt == 3 ? (Color){120, 70, 50, 255} : (Color){120, 128, 126, 255};
    aGrad((Rectangle){r.x - 2 * s, r.y, r.width + 4 * s, 6 * s}, 0.4f, aMul(cap, 1.3f), aMul(cap, 0.7f), 3);
    aGrad((Rectangle){r.x - 2 * s, r.y + r.height - 6 * s, r.width + 4 * s, 6 * s}, 0.4f, aMul(cap, 1.3f), aMul(cap, 0.7f), 3);
}
// chrome rail slider with a bubble thumb
static float aSlider(Rectangle r, const char *label, float v, float mn, float mx, const char *fmt) {
    float s = aU;
    aText(label, r.x, r.y - 22 * s, 15, (Color){206, 200, 186, 255}, false);
    char b[32]; snprintf(b, sizeof b, fmt, v);
    Vector2 ms = aMeasure(b, 15, false);
    aText(b, r.x + r.width - ms.x, r.y - 22 * s, 15, C_CYAN, false);
    Rectangle hit = {r.x - 8, r.y - 8, r.width + 16, r.height + 16};
    aNavNextSl = true;   // (the widget registered next is this slider)
    if (aNavFocus(hit) && aNavSide) { v += (mx - mn) / 20 * aNavSide; v = v < mn ? mn : v > mx ? mx : v; aNavSide = 0; if (aBlipOk) PlaySound(aBlip); }
    if (aMDown() && CheckCollisionPointRec(aMousePos(), hit)) { float t = (aMousePos().x - r.x) / r.width; t = t < 0 ? 0 : (t > 1 ? 1 : t); v = mn + (mx - mn) * t; }
    float t = (v - mn) / (mx - mn); t = t < 0 ? 0 : t > 1 ? 1 : t;
    Rectangle tr = {r.x, r.y + r.height * 0.15f, r.width, fmaxf(r.height * 0.7f, 6 * s)};
    aRR(aGrow(tr, 1.5f * s), tr.height, (Color){8, 10, 10, 255});
    aGrad(tr, 1.0f, (Color){20, 24, 24, 255}, (Color){40, 48, 48, 255}, 2);
    if (t > 0.01f) aGrad((Rectangle){tr.x + s, tr.y + s, (tr.width - 2 * s) * t, tr.height - 2 * s}, 1.0f, (Color){110, 180, 176, 255}, (Color){50, 110, 110, 255}, 3);
    for (int k = 0; k <= 10; k++) { float tx = r.x + r.width * k / 10; DrawRectangle((int)tx, (int)(tr.y + tr.height + 4 * s), (int)fmaxf(s, aPx()), (int)(4 * s), (Color){120, 128, 126, 160}); }
    bool hov = CheckCollisionPointRec(aMousePos(), hit);
    if (aIn3D) { Rectangle th = {r.x + r.width * t - 8 * s, tr.y + tr.height / 2 - 12 * s, 16 * s, 24 * s}; aPunch(th, 8 * s); aW3Add(2, th, hov, (Color){180, 186, 184, 255}, 0); }
    else aBlob(r.x + r.width * t, tr.y + tr.height / 2, 7 * s, 11 * s, hov ? (Color){210, 216, 214, 255} : (Color){180, 186, 184, 255}, true);
    return v;
}
// glass-cube checkbox with a cyan filament tick
static bool aToggle(float x, float y, const char *label, bool v) {
    float s = aU;
    Rectangle r = {x, y + 1 * s, 19 * s, 19 * s};
    Rectangle hit = {x, y, 320 * s, 22 * s};
    bool foc = aNavFocus((Rectangle){x - 2 * s, y - 2 * s, 34 * s + aMeasure(label, 15, false).x, 24 * s});
    bool hov = CheckCollisionPointRec(aMousePos(), hit) || foc;
    if (foc && aNavA && !aClickUsed) { v = !v; aClickUsed = true; aNavA = false; if (aBlipOk) PlaySound(aBlip); }
    else if (hov && !foc && !aClickUsed && aMPressed()) { v = !v; aClickUsed = true; if (aBlipOk) PlaySound(aBlip); }
    if (aIn3D) {
        aPunch(aGrow(r, 2 * s), 4 * s);
        aW3Add(1, r, hov, (Color){200, 196, 180, 255}, v);
        if (v) { aLine(r.x + 4 * s, r.y + 10 * s, r.x + 8 * s, r.y + 15 * s, 3, (Color){20, 70, 68, 255}); aLine(r.x + 8 * s, r.y + 15 * s, r.x + 16 * s, r.y + 4 * s, 3, (Color){20, 70, 68, 255});
                 aLine(r.x + 4 * s, r.y + 10 * s, r.x + 8 * s, r.y + 15 * s, 1.5f, C_CYAN); aLine(r.x + 8 * s, r.y + 15 * s, r.x + 16 * s, r.y + 4 * s, 1.5f, C_CYAN); }
        aText(label, x + 30 * s, y + 1 * s, 15, (Color){206, 200, 186, 255}, false);
        return v;
    }
    aRR((Rectangle){r.x + 2 * s, r.y + 3 * s, r.width, r.height}, 4 * s, (Color){6, 8, 8, 150});
    if (hov) aRR(aGrow(r, 2.5f * s), 6 * s, C_CYAN);
    aRR(aGrow(r, 1.5f * s), 5 * s, (Color){10, 14, 15, 255});
    aGrad(r, aRnd(r, 4 * s), (Color){214, 208, 192, 255}, (Color){140, 136, 122, 255}, 4);
    aScissor(r.x, r.y, r.width, r.height * 0.45f); aRR(aGrow(r, -s), 3 * s, (Color){255, 255, 255, 60}); aScissorEnd();
    if (v) {
        aLine(r.x + 4 * s, r.y + 10 * s, r.x + 8 * s, r.y + 15 * s, 3, (Color){40, 110, 108, 255});
        aLine(r.x + 8 * s, r.y + 15 * s, r.x + 16 * s, r.y + 4 * s, 3, (Color){40, 110, 108, 255});
        aLine(r.x + 4 * s, r.y + 10 * s, r.x + 8 * s, r.y + 15 * s, 1, C_CYAN);
        aLine(r.x + 8 * s, r.y + 15 * s, r.x + 16 * s, r.y + 4 * s, 1, C_CYAN);
    }
    aText(label, x + 30 * s, y + 1 * s, 15, (Color){206, 200, 186, 255}, false);
    return v;
}
typedef struct { char buf[256]; int len; bool active, mask; int max; } TextBox;
static void aTextBox(Rectangle r, const char *label, TextBox *tb) {
    float s = aU;
    int mx = tb->max > 0 ? tb->max : 31; if (mx > 255) mx = 255;
    if (label && label[0]) aText(label, r.x, r.y - 24 * s, 16, (Color){232, 226, 212, 255}, true);
    if (!aClickUsed && aMPressed()) tb->active = CheckCollisionPointRec(aMousePos(), r);
    if (tb->active) aRR(aGrow(r, 2.5f * s), 6 * s, C_CYAN);
    aRR(aGrow(r, 1.5f * s), 5 * s, (Color){10, 14, 15, 255});
    aGrad(r, aRnd(r, 4 * s), (Color){20, 26, 28, 255}, (Color){38, 48, 50, 255}, 3);   // a dark well with bright text: readable at any size
    DrawRectangle((int)(r.x + 3 * s), (int)(r.y + 1.5f * s), (int)(r.width - 6 * s), (int)(3 * s), (Color){0, 0, 0, 120});
    if (tb->active) {
        int c = GetCharPressed();
        while (c > 0) { if (c >= 32 && c < 127 && tb->len < mx) { tb->buf[tb->len++] = (char)c; tb->buf[tb->len] = 0; } c = GetCharPressed(); }
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && tb->len > 0) tb->buf[--tb->len] = 0;
        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_V)) {
            const char *cl = GetClipboardText();
            if (cl) for (; *cl && tb->len < mx; cl++) if (*cl >= 32 && *cl < 127) { tb->buf[tb->len++] = *cl; tb->buf[tb->len] = 0; }
        }
    }
    char shown[270];
    if (tb->mask) { int i; for (i = 0; i < tb->len && i < 60; i++) shown[i] = '*'; shown[i] = 0; }
    else snprintf(shown, sizeof shown, "%s", tb->buf);
    // keep the end of long text visible
    const char *vis = shown;
    while (*vis && aMeasure(vis, 17, false).x > r.width - 24 * s) vis++;
    char vb[280]; snprintf(vb, sizeof vb, "%s%s", vis, (tb->active && ((int)(GetTime() * 2) % 2 == 0)) ? "|" : "");
    aTextF(vb, r.x + 10 * s, r.y + (r.height - 18 * s) / 2 - 1 * s, 18, (Color){244, 240, 230, 255}, 0);
}
// logo: bone wordmark, kanji, cyan subtitle
static void aLogo(float cx, float y, float size, const char *sub) {
    float s = aU;
    const char *w = aFontOwn ? "ZANK\xC5\x8C 2006" : "ZANKO 2006";
    Vector2 m = aMeasure(w, size, true);
    aTextF(w, cx - m.x / 2 + 3 * s, y + 4 * s, size, (Color){8, 10, 10, 200}, 1);
    aTextF(w, cx - m.x / 2, y, size, (Color){232, 226, 210, 255}, 1);
    if (aFontJOwn) aTextF("\xE6\xAE\x8B\xE5\x85\x89", cx + m.x / 2 + 10 * s, y + size * 0.18f * s, size * 0.55f, C_AMBER, 2);   // 残光
    if (sub) { Vector2 n = aMeasure(sub, size * 0.26f, false); aText(sub, cx - n.x / 2, y + size * 1.08f * s, size * 0.26f, (Color){150, 210, 205, 230}, false); }
}
static void aGlow(const char *str, float cx, float y, float size, Color glow) {   // big caption (death screen etc.)
    float s = aU; Vector2 m = aMeasure(str, size, true);
    aTextF(str, cx - m.x / 2 + 3 * s, y + 4 * s, size, (Color){8, 10, 10, 200}, 1);
    aTextF(str, cx - m.x / 2, y, size, aLerpC(C_BONE, (Color){glow.r, glow.g, glow.b, 255}, 0.35f), 1);
}
// darkening veil with hard horizontal fog bands (used over a paused game view)
static void aBackground(float alpha) {
    int W = GetScreenWidth(), H = GetScreenHeight();
    for (int i = 0; i < 6; i++) DrawRectangle(0, H * i / 6, W, H / 6 + 1, (Color){14, 17, 18, (unsigned char)(alpha * (150 + i * 14))});
}
// sanity "weather" orb: clear -> overcast -> rain -> rust storm; no number shown
static void aOrb(float cx, float cy, float R, float sanity, int stage, float t) {
    float s = aU;
    aGrad((Rectangle){cx - R * 0.7f, cy + R * 0.82f, R * 1.4f, R * 0.32f}, 0.5f, (Color){130, 140, 140, 255}, (Color){60, 66, 66, 255}, 3);   // chrome base
    DrawCircle((int)cx, (int)(cy + R * 0.08f), R * 1.06f, (Color){8, 10, 10, 255});
    Color core = stage == 3 ? (Color){90, 30, 20, 255} : (Color){70, 84, 86, 255};
    DrawCircle((int)cx, (int)cy, R, aMul(core, 0.65f));
    DrawCircle((int)(cx - R * 0.08f), (int)(cy - R * 0.08f), R * 0.84f, core);
    float clear = Clamp(sanity / 100.0f, 0, 1);
    if (stage < 3 && clear > 0.55f) { DrawCircle((int)(cx + R * 0.25f), (int)(cy - R * 0.25f), R * 0.3f * (clear - 0.4f) * 1.6f, (Color){230, 200, 150, 255}); }
    int clouds = (int)((1 - clear) * 7) + 1;
    for (int k = 0; k < clouds; k++) {
        float px = cx + sinf(k * 2.1f + t * 0.3f) * R * 0.45f, py = cy - R * 0.25f + (k % 3) * R * 0.14f;
        DrawEllipse((int)px, (int)py, R * 0.32f, R * 0.17f, stage == 3 ? (Color){70, 20, 14, 230} : (Color){150, 150, 146, 220});
    }
    if (clear < 0.5f || stage >= 1) {
        int n = (int)((1 - clear) * 10) + stage * 2;
        for (int k = 0; k < n; k++) { float px = cx - R * 0.5f + fmodf(k * 0.37f, 1.0f) * R, py = cy + fmodf(t * 1.6f + k * 0.23f, 1.0f) * R * 0.6f - R * 0.05f;
            aLine(px, py, px - 2 * s, py + 6 * s, 1, stage == 3 ? (Color){200, 60, 40, 220} : (Color){190, 205, 210, 200}); }
    }
    if (stage == 3 && fmodf(t, 2.7f) < 0.12f) aLine(cx + R * 0.1f, cy - R * 0.4f, cx - R * 0.1f, cy + R * 0.3f, 2, (Color){255, 200, 160, 255});
    DrawEllipse((int)(cx - R * 0.2f), (int)(cy - R * 0.5f), R * 0.5f, R * 0.22f, (Color){255, 255, 255, 46});
    DrawEllipse((int)(cx - R * 0.42f), (int)(cy - R * 0.46f), fmaxf(R * 0.09f, aPx()), fmaxf(R * 0.07f, aPx()), (Color){255, 255, 255, 220});
}
// chrome compass ring with kanji cardinals; yaw 0 faces south (+Z)
static void aCompass(float cx, float cy, float R, float yaw, int stage, float t) {
    float s = aU;
    DrawCircle((int)cx, (int)(cy + 3 * s), R + 4 * s, (Color){6, 8, 8, 140});
    DrawRing((Vector2){cx, cy}, R - 6 * s, R + 2 * s, 0, 360, 48, (Color){14, 18, 19, 255});
    DrawRing((Vector2){cx, cy}, R - 5 * s, R, 0, 360, 48, (Color){90, 96, 96, 255});
    DrawRing((Vector2){cx, cy}, R - 5 * s, R, 180, 360, 48, (Color){150, 156, 154, 255});
    DrawRing((Vector2){cx, cy}, R - 4 * s, R - 2 * s, 200, 260, 16, (Color){230, 232, 226, 255});
    DrawCircle((int)cx, (int)cy, R - 6 * s, (Color){24, 32, 33, 235});
    DrawCircle((int)cx, (int)cy, R - 12 * s, (Color){14, 20, 21, 235});
    for (int k = 0; k < 24; k++) { float a = k * 2 * PI / 24 - yaw; float r0 = R - 9 * s, r1 = R - (k % 6 == 0 ? 14 : 11) * s;
        aLine(cx + sinf(a) * r0, cy - cosf(a) * r0, cx + sinf(a) * r1, cy - cosf(a) * r1, 1, (Color){150, 156, 154, 200}); }
    // heading: player facing direction is "up" on the ring
    const char *jp[4] = {"\xE5\x8D\x97", "\xE8\xA5\xBF", "\xE5\x8C\x97", "\xE6\x9D\xB1"};   // 南 西 北 東 at yaw 0, PI/2, PI, 3PI/2
    const char *la[4] = {"S", "W", "N", "E"};
    for (int k = 0; k < 4; k++) {
        float a = k * PI / 2 - yaw;   // angle on ring relative to view direction
        float px = cx - sinf(a) * (R - 22 * s), py = cy - cosf(a) * (R - 22 * s);
        Color c = k == 2 ? (Color){210, 70, 56, 255} : (Color){216, 210, 196, 255};
        float fs = 15;
        if (aFontJOwn) aTextF(jp[k], px - fs * aU / 2, py - fs * aU / 2, fs, c, 2);
        else { Vector2 m = aMeasure(la[k], fs, true); aText(la[k], px - m.x / 2, py - m.y / 2, fs, c, true); }
    }
    DrawTriangle((Vector2){cx, cy - R + 3 * s}, (Vector2){cx - 5 * s, cy - R - 6 * s}, (Vector2){cx + 5 * s, cy - R - 6 * s}, C_CYAN);
    if (stage == 3) for (int k = 0; k < 30; k++) { float a = k * 1.7f + floorf(t * 12) * 0.9f, rr = fmodf(k * 0.61f + t, 1.0f) * (R - 12 * s);
        DrawRectangle((int)(cx + cosf(a) * rr), (int)(cy + sinf(a) * rr), (int)fmaxf(2 * s, aPx()), (int)fmaxf(2 * s, aPx()), (Color){200, 190, 180, 160}); }
}
#endif
