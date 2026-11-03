// silent.h — Silent Aero live backdrop: low-res banded 3D scene (dusk sky, blast halo, god rays, broken swooshes,
// dead eco-towers, dead grass, ash, dirty bubbles). Rendered at ~270p and upscaled nearest-neighbour.
// Shared by the launcher and the game. Everything is prefixed sa* to stay out of the game's namespace.
#ifndef SILENT_H
#define SILENT_H
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

static const char *saVS =
GLSL_HEAD
"in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal; in vec4 vertexColor;\n"
"uniform mat4 mvp; uniform mat4 matModel; uniform mat4 matNormal; uniform vec2 snapRes;\n"
"out vec2 uv; out vec3 wp; out vec3 wn;\n"
"void main(){ uv=vertexTexCoord; wp=vec3(matModel*vec4(vertexPosition,1.0)); wn=normalize(vec3(matNormal*vec4(vertexNormal,0.0)));\n"
" vec4 p=mvp*vec4(vertexPosition,1.0); if(p.w>0.05){ vec2 s=snapRes*0.5; p.xy=floor(p.xy/p.w*s+0.5)/s*p.w; } gl_Position=p; }\n";
static const char *saFS =
GLSL_HEAD
"in vec2 uv; in vec3 wp; in vec3 wn;\n"
"uniform sampler2D texture0; uniform vec4 colDiffuse; uniform vec3 viewPos; uniform vec3 lightDir; uniform vec3 lightCol; uniform vec3 ambient;\n"
"uniform float bubble; uniform float sky; uniform vec3 skyTop; uniform vec3 skyBot; uniform float dirt; uniform float groundFog;\n"
"uniform float gloss; uniform float rim; uniform float emissive; uniform float nofog; uniform vec3 fogColor; uniform float fogDensity; uniform vec3 rimCol;\n"
"out vec4 finalColor;\n"
"const float bay[16]=float[16](0.,8.,2.,10.,12.,4.,14.,6.,3.,11.,1.,9.,15.,7.,13.,5.);\n"
"float bay4(){ ivec2 q=ivec2(mod(gl_FragCoord.xy,4.0)); return bay[q.y*4+q.x]/16.0; }\n"
"float band(float x,float n){ return floor(x*n+bay4())/n; }\n"
"vec3 rainbow(float h){ return 0.5+0.5*cos(6.2832*(h+vec3(0.0,0.33,0.67))); }\n"
"float hash(vec2 p){ return fract(sin(dot(p,vec2(12.9898,78.233)))*43758.5453); }\n"
"void main(){ vec4 t=texture(texture0,uv)*colDiffuse;\n"
" vec3 n=normalize(wn); vec3 v=normalize(viewPos-wp); vec3 l=normalize(-lightDir); vec3 h=normalize(l+v);\n"
" float a=t.a; vec3 col;\n"
" if(sky>0.5){ vec3 dir=normalize(wp-viewPos); float k=clamp(dir.y*1.6+0.35,0.0,1.0); k=band(k,7.0); col=mix(skyBot,skyTop,k); a=1.0; }\n"
" else if(bubble>0.5){\n"
"  float fr=1.0-max(dot(n,v),0.0); float fb=band(fr,5.0);\n"
"  vec3 film=rainbow(fr*1.7+n.y*0.35+0.15)*(1.0-dirt*0.75)+vec3(0.55,0.47,0.36)*dirt;\n"
"  col=mix(t.rgb,film,0.55)*(0.35+fb);\n"
"  float sp=step(0.985,dot(n,h)); float sp2=step(0.93,dot(n,normalize(v+vec3(0.3,-0.6,0.0))))*0.45;\n"
"  col+=vec3(sp)+vec3(sp2)*vec3(0.85,0.95,1.0);\n"
"  a=clamp(0.06+fb*0.85+sp,0.0,1.0)*t.a;\n"
"  if(dirt>0.0){ float sp3=step(1.0-dirt*0.10,hash(floor(uv*vec2(48.0,24.0)))); col=mix(col,vec3(0.18,0.16,0.13),sp3); a=max(a,sp3*0.9); }\n"
" }\n"
" else if(emissive>0.5){ col=t.rgb; }\n"
" else {\n"
"  float d=band(max(dot(n,l),0.0),4.0);\n"
"  float s=step(0.975,dot(n,h))*gloss + band(pow(max(dot(n,h),0.0),12.0),3.0)*gloss*0.25;\n"
"  float f=band(pow(1.0-max(dot(n,v),0.0),2.5),3.0)*rim;\n"
"  float top=step(0.55,n.y)*gloss*0.18;\n"
"  col=t.rgb*(ambient+lightCol*d)+lightCol*s+rimCol*f+vec3(top); }\n"
" float dist=length(wp-viewPos);\n"
" float fo=1.0-exp(-pow(dist*fogDensity,2.0));\n"
" fo=max(fo, groundFog*clamp((dist-3.0)/12.0,0.0,1.0)*clamp(1.0-wp.y/2.5,0.0,1.0));\n"   // low fog that swallows distant grass
" float fg=band(fo,6.0); if(nofog<0.5 && sky<0.5) col=mix(col,fogColor,clamp(fg,0.0,1.0));\n"
" col+=(bay4()-0.5)/16.0;\n"
" col=floor(clamp(col,0.0,1.0)*15.0+0.5)/15.0;\n"
" finalColor=vec4(col,a); }\n";

static Shader saSh; static bool saReady = false;
static int saLBub, saLSky, saLSkyT, saLSkyB, saLDirt, saLNoFog, saLGloss, saLRim, saLEmis, saLView, saLLight, saLLCol, saLAmb, saLFog, saLFogD, saLSnap, saLRimCol, saLGFog;
static Model saCube, saSphere, saSphereLo, saCyl, saGround, saBlade;
static Texture2D saWhite, saEco, saFacade, saGrass;
static RenderTexture2D saRT; static int saRW = 0, saRH = 0;
static Vector3 saCamPos;

static void saF(int loc, float v) { SetShaderValue(saSh, loc, &v, SHADER_UNIFORM_FLOAT); }
static void saV3(int loc, Vector3 v) { SetShaderValue(saSh, loc, &v, SHADER_UNIFORM_VEC3); }
static void saMat(float gloss, float rim, float emis) { saF(saLGloss, gloss); saF(saLRim, rim); saF(saLEmis, emis); }
static void saDraw(Model *m, Matrix M, Color c, Texture2D t) { m->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = t; m->transform = M; DrawModel(*m, (Vector3){0, 0, 0}, 1, c); }
static Matrix saTRS(Vector3 p, Vector3 rot, Vector3 s) {
    Matrix R = MatrixMultiply(MatrixMultiply(MatrixRotateZ(rot.z), MatrixRotateX(rot.x)), MatrixRotateY(rot.y));
    return MatrixMultiply(MatrixMultiply(MatrixScale(s.x, s.y, s.z), R), MatrixTranslate(p.x, p.y, p.z));
}
static Model saModel(Mesh m) { Model md = LoadModelFromMesh(m); md.materials[0].shader = saSh; return md; }
static Texture2D saTex(const char *path, Color fallback) {
    Texture2D t;
    if (path && FileExists(path)) t = LoadTexture(path);
    else { Image im = GenImageChecked(32, 32, 4, 4, fallback, (Color){(unsigned char)(fallback.r * 0.7f), (unsigned char)(fallback.g * 0.7f), (unsigned char)(fallback.b * 0.7f), 255}); t = LoadTextureFromImage(im); UnloadImage(im); }
    SetTextureFilter(t, TEXTURE_FILTER_POINT); SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
    return t;
}
static Texture2D saMakeGrass(void) {
    int S = 64; Image im = GenImageColor(S, S, (Color){92, 90, 66, 255});
    unsigned int r = 99;
    Color pal[5] = {{120, 114, 84, 255}, {70, 68, 50, 255}, {140, 130, 92, 255}, {56, 74, 46, 255}, {30, 28, 24, 255}};
    for (int k = 0; k < 420; k++) {
        r = r * 1103515245u + 12345u; int x = (r >> 8) % S; r = r * 1103515245u + 12345u; int y = (r >> 8) % S;
        r = r * 1103515245u + 12345u; int c = (r >> 8) % 5; int len = 3 + (r >> 4) % 5;
        for (int j = 0; j < len; j++) ImageDrawPixel(&im, (x + j / 2) % S, (y - j + S) % S, pal[c]);
    }
    for (int k = 0; k < 90; k++) { r = r * 1103515245u + 12345u; ImageDrawPixel(&im, (r >> 8) % S, (r >> 16) % S, (Color){170, 165, 152, 255}); }
    Texture2D t = LoadTextureFromImage(im); UnloadImage(im);
    SetTextureFilter(t, TEXTURE_FILTER_POINT); SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
    return t;
}
static Mesh saTiledPlane(float w, float d, float tile) {
    Mesh m = GenMeshPlane(w, d, 8, 8);
    for (int i = 0; i < m.vertexCount; i++) { m.texcoords[i * 2] *= tile * w / d; m.texcoords[i * 2 + 1] *= tile; }
    UpdateMeshBuffer(m, 1, m.texcoords, m.vertexCount * 2 * sizeof(float), 0);
    return m;
}
// texDir: folder holding ecoglass.png / facade_a.png (may be NULL -> generated stand-ins)
static void saInit(const char *ecoPath, const char *facadePath) {
    if (saReady) return;
    saSh = LoadShaderFromMemory(saVS, saFS);
    saLNoFog = GetShaderLocation(saSh, "nofog"); saLBub = GetShaderLocation(saSh, "bubble"); saLSky = GetShaderLocation(saSh, "sky"); saLSkyT = GetShaderLocation(saSh, "skyTop");
    saLSkyB = GetShaderLocation(saSh, "skyBot"); saLDirt = GetShaderLocation(saSh, "dirt"); saLGloss = GetShaderLocation(saSh, "gloss"); saLRim = GetShaderLocation(saSh, "rim");
    saLEmis = GetShaderLocation(saSh, "emissive"); saLView = GetShaderLocation(saSh, "viewPos"); saLLight = GetShaderLocation(saSh, "lightDir"); saLLCol = GetShaderLocation(saSh, "lightCol");
    saLAmb = GetShaderLocation(saSh, "ambient"); saLFog = GetShaderLocation(saSh, "fogColor"); saLFogD = GetShaderLocation(saSh, "fogDensity"); saLSnap = GetShaderLocation(saSh, "snapRes");
    saLRimCol = GetShaderLocation(saSh, "rimCol"); saLGFog = GetShaderLocation(saSh, "groundFog");
    Image wi = GenImageColor(4, 4, WHITE); saWhite = LoadTextureFromImage(wi); UnloadImage(wi);
    saCube = saModel(GenMeshCube(1, 1, 1)); saSphere = saModel(GenMeshSphere(1, 14, 18)); saSphereLo = saModel(GenMeshSphere(1, 6, 8));
    saCyl = saModel(GenMeshCylinder(1, 1, 14)); saBlade = saModel(GenMeshCone(1, 1, 3));
    saGround = saModel(saTiledPlane(80, 60, 18));
    saEco = saTex(ecoPath, (Color){70, 110, 120, 255}); saFacade = saTex(facadePath, (Color){120, 118, 110, 255});
    saGrass = saMakeGrass();
    saReady = true;
}

// stage: 0 afterglow, 1 sadness, 2 depression, 3 otherworld
static void saSky(Vector3 cam, float t, int stage) {
    rlDisableDepthMask();
    saMat(0, 0, 1); saF(saLNoFog, 1); saF(saLSky, 1);
    if (stage == 3) { saV3(saLSkyT, (Vector3){0.08f, 0.02f, 0.02f}); saV3(saLSkyB, (Vector3){0.45f, 0.16f, 0.1f}); }
    else { float g = stage * 0.04f; saV3(saLSkyT, (Vector3){0.16f - g * 0.5f, 0.18f - g * 0.5f, 0.19f - g * 0.5f}); saV3(saLSkyB, (Vector3){0.62f - g * 2.2f, 0.55f - g * 1.6f, 0.43f - g}); }
    saDraw(&saSphere, saTRS(cam, (Vector3){0, 0, 0}, (Vector3){-150, -150, -150}), WHITE, saWhite);
    saF(saLSky, 0);
    if (stage < 3) {
        Vector3 sp = {28, 30, -90};
        float pulse = 0.5f + 0.5f * sinf(t * 0.6f);
        for (int k = 0; k < 6; k++) saDraw(&saSphere, saTRS(Vector3Add(sp, (Vector3){0, 0, k * 0.2f}), (Vector3){0, 0, 0}, (Vector3){22 - k * 3.2f, 22 - k * 3.2f, 0.3f}), (Color){206, 176, 128, (unsigned char)(34 + k * 22 - stage * 8)}, saWhite);
        saDraw(&saSphere, saTRS(Vector3Add(sp, (Vector3){0, 0, 2}), (Vector3){0, 0, 0}, (Vector3){5.0f, 5.0f, 0.3f}), (Color){244, 232, 204, 240}, saWhite);
        for (int r = 0; r < 2; r++) { float R = 8 + r * 4.0f + fmodf(t * 0.8f, 4.0f) * (r == 1); saDraw(&saCyl, saTRS(Vector3Add(sp, (Vector3){0, 0, 2.5f}), (Vector3){PI / 2, 0, 0}, (Vector3){R, 0.04f, R}), (Color){240, 222, 186, (unsigned char)(120 - r * 50)}, saWhite); }
        for (int k = 0; k < 9; k++) {
            float ang = -0.9f + k * 0.23f + (k % 2) * 0.05f + sinf(t * 0.05f) * 0.03f;
            Vector3 c = Vector3Add(sp, (Vector3){cosf(ang - PI / 2) * 40, sinf(ang - PI / 2) * 40, 3});
            unsigned char al = (unsigned char)(8 + 10 * ((k + (int)(t * 0.5f)) % 3 == 0 ? pulse : 0.4f));
            saDraw(&saCube, saTRS(c, (Vector3){0, 0, ang}, (Vector3){2.2f + (k % 3), 80, 0.1f}), (Color){230, 200, 150, al}, saWhite);
        }
        for (int r = 0; r < 3; r++) {   // broken glossy ribbons, slowly travelling
            float R0 = 60 + r * 4.0f, rot = t * (0.006f + r * 0.002f);
            for (int k = 0; k < 64; k++) {
                float a0 = PI * 0.18f + k * (PI * 0.62f / 64) + rot;
                if (((k + (int)(t * 0.7f) * (r + 1)) / 6 + r) % 3 == 0) continue;
                Vector3 c = {-10 + cosf(a0) * R0, -38 + sinf(a0) * R0, -70.0f + r};
                saDraw(&saCube, saTRS(c, (Vector3){0, 0, a0}, (Vector3){0.5f + (2 - r) * 0.35f, 2.4f, 0.1f}), (Color){235, 215, 175, (unsigned char)(40 - r * 10 - stage * 6)}, saWhite);
            }
        }
    }
    saF(saLNoFog, 0);
    rlEnableDepthMask();
}
static void saLandscape(int stage) {
    saMat(0, 0, 0);
    saDraw(&saGround, saTRS((Vector3){0, 0, -10}, (Vector3){0, 0, 0}, (Vector3){1, 1, 1}), stage == 3 ? (Color){130, 60, 46, 255} : WHITE, saGrass);
    for (int i = 0; i < 6; i++) {
        float x = -14 + i * 6.0f, z = -8 - (i % 3) * 5.0f, s = 4 + (i % 2) * 2.5f;
        saMat(0.25f, 0.15f, 0);
        saDraw(&saSphere, saTRS((Vector3){x, -s * 0.55f, z}, (Vector3){0, (float)i, 0}, (Vector3){s * 1.6f, s, s}), (Color){(unsigned char)(215 - i * 8), (unsigned char)(210 - i * 8), 190, 255}, saGrass);
    }
    for (int i = 0; i < 9; i++) {
        float x = -24 + i * 6.2f, z = -34 - (i % 3) * 4.0f, h = 8.0f + (i * 37 % 9);
        saMat(0.6f, 0.4f, 0);
        saDraw(&saCube, saTRS((Vector3){x, h / 2, z}, (Vector3){0, 0.2f * (i % 2), i % 4 == 1 ? 0.06f : 0}, (Vector3){3.2f, h, 3.2f}), (Color){150, 160, 158, 255}, (i % 3 == 0) ? saFacade : saEco);
        if (i % 3 != 1) { saMat(0, 0, 0); saDraw(&saSphereLo, saTRS((Vector3){x, h + 0.3f, z}, (Vector3){0, 0, 0}, (Vector3){1.8f, 0.8f, 1.8f}), (Color){74, 80, 58, 255}, saWhite); }
    }
}
static void saGrassField(Vector3 c, float rx, float rz, int n, float t, int stage) {
    unsigned int r = 1234;
    Color dead[4] = {{140, 128, 90, 255}, {110, 104, 76, 255}, {70, 92, 56, 255}, {40, 36, 30, 255}};
    if (stage == 3) { dead[0] = (Color){120, 60, 40, 255}; dead[1] = (Color){90, 40, 30, 255}; dead[2] = (Color){60, 30, 24, 255}; }
    saMat(0.6f, 0.3f, 0);
    for (int i = 0; i < n; i++) {
        r = r * 1103515245u + 12345u; float x = c.x + ((r >> 8) % 1000 / 500.0f - 1) * rx;
        r = r * 1103515245u + 12345u; float z = c.z + ((r >> 8) % 1000 / 500.0f - 1) * rz;
        r = r * 1103515245u + 12345u; int k = (r >> 8) % 4; float h = 0.12f + ((r >> 12) % 100) / 100.0f * 0.16f;
        if (k == 3) h *= 0.5f;
        float lean = ((r >> 16) % 100 / 100.0f - 0.5f) * 0.9f + sinf(t * 0.9f + x * 0.8f) * 0.07f;
        saDraw(&saBlade, saTRS((Vector3){x, -0.01f, z}, (Vector3){0.0f, (float)(r % 7), lean}, (Vector3){0.035f, h, 0.035f}), dead[k], saWhite);
    }
}
static void saBubbles(int n, float spread, Vector3 center, float t) {
    typedef struct { Vector3 p; float s, d; int i; } B;
    B b[32]; int m = 0;
    for (int i = 0; i < n && m < 32; i++) {
        float a = i * 2.39996f, r = spread * sqrtf((i + 0.5f) / n);
        float rise = fmodf(t * (0.05f + (i % 3) * 0.02f) + i * 0.37f, 2.4f) - 1.2f;
        Vector3 p = {center.x + cosf(a) * r + sinf(t * 0.4f + i) * 0.12f, center.y + sinf(a * 1.3f) * spread * 0.45f + rise, center.z + sinf(a) * r * 0.6f};
        b[m++] = (B){p, 0.07f + (i % 5) * 0.045f, Vector3Distance(p, saCamPos), i};
    }
    for (int x = 0; x < m; x++) for (int y = x + 1; y < m; y++) if (b[y].d > b[x].d) { B tmp = b[x]; b[x] = b[y]; b[y] = tmp; }
    rlDisableDepthMask();
    for (int k = 0; k < m; k++) {
        float s = b[k].s; int i = b[k].i;
        if (i % 4 == 1) { rlEnableDepthMask(); saMat(0.2f, 0.2f, 0); saDraw(&saSphereLo, saTRS(b[k].p, (Vector3){(float)i, (float)i, 0}, (Vector3){s * 0.3f, s * 0.24f, s * 0.3f}), (Color){120, 114, 104, 255}, saWhite); rlDisableDepthMask(); continue; }
        saF(saLBub, 1); saF(saLDirt, 0.55f + (i % 3) * 0.15f);
        Vector3 sc = i % 4 == 3 ? (Vector3){s, s * 0.8f, s} : (Vector3){s, s, s};
        saDraw(&saSphere, saTRS(b[k].p, (Vector3){0.3f, (float)i + t * 0.2f, 0}, sc), (Color){150, 140, 120, 255}, saWhite);
        saF(saLBub, 0); saF(saLDirt, 0);
    }
    rlEnableDepthMask();
}
static void saAsh(int n, Vector3 c, float r, float t, int stage) {
    saMat(0, 0, 1);
    Color col = stage == 3 ? (Color){255, 110, 50, 255} : (Color){200, 194, 182, 255};
    for (int i = 0; i < n; i++) {
        float x = c.x + fmodf(i * 7.31f, r * 2) - r + sinf(t * 0.5f + i) * 0.1f;
        float fall = stage == 3 ? -t * 0.18f : t * 0.22f;   // embers rise, ash falls
        float y = c.y + fmodf(fmodf(i * 3.17f - fall, r) + r, r) - r / 2;
        float z = c.z + fmodf(i * 5.77f, r) - r / 2;
        saDraw(&saCube, saTRS((Vector3){x, y, z}, (Vector3){(float)i, i * 2.0f, 0}, (Vector3){0.016f, 0.016f, 0.016f}), col, saWhite);
    }
}
// Seiryu boot orb: dirty glass sphere around a teal core, ring of bubbles, progress tube
static void saOrb(float t, float progress) {
    Vector3 o = {0.4f, 2.1f, 0.6f};
    saMat(2.0f, 1.0f, 0);
    saDraw(&saSphere, saTRS(o, (Vector3){0, t * 0.5f, 0}, (Vector3){0.55f, 0.42f + 0.03f * sinf(t * 2), 0.55f}), (Color){50, 100, 100, 255}, saWhite);
    rlDisableDepthMask(); saF(saLBub, 1); saF(saLDirt, 0.5f);
    saDraw(&saSphere, saTRS(Vector3Add(o, (Vector3){0, 0.15f, 0}), (Vector3){0, t * 0.2f, 0}, (Vector3){0.9f, 0.9f, 0.9f}), (Color){150, 140, 120, 255}, saWhite);
    saF(saLBub, 0); saF(saLDirt, 0); rlEnableDepthMask();
    int lit = (int)(progress * 12);
    for (int k = 0; k < 12; k++) {
        float a = k * 2 * PI / 12 + t * 0.6f;
        Vector3 p = {o.x + cosf(a) * 1.3f, o.y + 0.15f + sinf(a) * 0.25f, o.z + sinf(a) * 0.5f};
        if (k < lit) { saMat(1.8f, 1.0f, 0); saDraw(&saSphere, saTRS(p, (Vector3){0, 0, 0}, (Vector3){0.07f, 0.07f, 0.07f}), (Color){111, 214, 208, 255}, saWhite); }
        else { rlDisableDepthMask(); saF(saLBub, 1); saF(saLDirt, 0.6f); saDraw(&saSphere, saTRS(p, (Vector3){0, (float)k, 0}, (Vector3){0.08f + (k % 3) * 0.03f, 0.08f + (k % 3) * 0.03f, 0.08f + (k % 3) * 0.03f}), (Color){150, 140, 120, 255}, saWhite); saF(saLBub, 0); saF(saLDirt, 0); rlEnableDepthMask(); }
    }
}
// Renders the backdrop into the low-res target and draws it full-screen.
// mode 0: wallpaper   mode 1: Seiryu boot orb (progress 0..1)
static void saBackdrop(float tReal, int stage, int mode, float progress, float brightness) {
    if (!saReady) return;
    int SW = GetScreenWidth(), SH = GetScreenHeight();
    int RH = 270, RW = (int)(RH * (float)SW / (float)SH + 0.5f);
    if (RW != saRW || RH != saRH) { if (saRW) UnloadRenderTexture(saRT); saRT = LoadRenderTexture(RW, RH); SetTextureFilter(saRT.texture, TEXTURE_FILTER_POINT); saRW = RW; saRH = RH; }
    float t = floorf(tReal * 12.0f) / 12.0f;   // stepped animation, 12 fps
    Camera3D cam = {0}; cam.up = (Vector3){0, 1, 0}; cam.fovy = 45; cam.projection = CAMERA_PERSPECTIVE;
    cam.position = (Vector3){sinf(t * 0.05f) * 0.35f, 1.4f + sinf(t * 0.08f) * 0.05f, 5};
    cam.target = (Vector3){0.6f + sinf(t * 0.05f) * 0.2f, 2.4f, 0};
    saCamPos = cam.position;
    BeginTextureMode(saRT);
    ClearBackground(BLACK);
    BeginMode3D(cam);
    BeginShaderMode(saSh);
    float snap[2] = {(float)RW, (float)RH}; SetShaderValue(saSh, saLSnap, snap, SHADER_UNIFORM_VEC2);
    bool hell = stage == 3; float dim = 1.0f - stage * 0.08f;
    saV3(saLView, cam.position); saV3(saLLight, Vector3Normalize((Vector3){-0.5f, -0.6f, -0.6f}));
    saV3(saLLCol, hell ? (Vector3){0.95f, 0.55f, 0.4f} : (Vector3){0.95f * dim, 0.82f * dim, 0.62f * dim});
    saV3(saLAmb, hell ? (Vector3){0.32f, 0.2f, 0.18f} : (Vector3){0.32f, 0.34f, 0.36f});
    saV3(saLFog, hell ? (Vector3){0.24f, 0.08f, 0.06f} : (Vector3){0.34f - stage * 0.03f, 0.33f - stage * 0.03f, 0.30f - stage * 0.02f});
    saV3(saLRimCol, hell ? (Vector3){0.9f, 0.5f, 0.35f} : stage ? (Vector3){0.6f, 0.65f, 0.65f} : (Vector3){0.55f, 0.85f, 0.82f});
    saF(saLFogD, 0.075f + stage * 0.012f); saF(saLGFog, 0.85f);
    EndShaderMode();
    saSky(cam.position, t, stage);
    saLandscape(stage);
    saGrassField((Vector3){0.2f, 0, 2.2f}, 3.6f, 1.6f, 520, t, stage);
    if (mode == 1) saOrb(t, progress);
    else saBubbles(7, 1.0f, (Vector3){-1.6f, 2.6f, 1.2f}, t);
    saAsh(hell ? 120 : 160, (Vector3){0, 2.0f, 2.4f}, 3.5f, t, stage);
    EndMode3D();
    EndTextureMode();
    unsigned char b = (unsigned char)(255 * fminf(1, fmaxf(0, brightness)));
    rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD); BeginBlendMode(BLEND_CUSTOM);   // opaque: the target's alpha channel is not meaningful
    DrawTexturePro(saRT.texture, (Rectangle){0, 0, (float)RW, -(float)RH}, (Rectangle){0, 0, (float)SW, (float)SH}, (Vector2){0, 0}, 0, (Color){b, b, b, 255});
    EndBlendMode();
}
#endif
