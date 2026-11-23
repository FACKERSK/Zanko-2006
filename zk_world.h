// zk_world.h — multi-floor buildings (rooms, stairs, junk piles, searchable furniture), subway, the Aero wilds outside the city
#define FH 3.2f          // floor-to-floor height
#define SLAB 0.2f
#define WILD 150.0f      // walkable wilds beyond the city edge
#define WEST 440.0f      // the world reaches this far west: the high-rise district beyond the west tunnel (0.9.0)
#define SUBY -7.0f       // subway floor
static float SUB_Z = 0, subX0, subX1; static float subEx[3]; static int nSubEx;   // 0.9.8: the line's band (z) and its entrances (x), found in the planned town (subwayPlace)
#define TUN_Z0 (SUB_Z + 1.0f)
#define TUN_Z1 (SUB_Z + ROADW - 1.0f)

// ---------------------------------------------------------------- ground holes (subway stairwells)
#define MAX_HOLE 8
static Rectangle holes[MAX_HOLE]; static int nHole;
#define MAX_MAPFOOT 2400
static Rectangle mapFoot[MAX_MAPFOOT]; static float mapFootH[MAX_MAPFOOT], mapFootY[MAX_MAPFOOT]; static short mapFootF[MAX_MAPFOOT]; static int nMapFoot;   // footprints in their building's frame
static bool inHole(float x, float z) { for (int i = 0; i < nHole; i++) if (x > holes[i].x && x < holes[i].x + holes[i].width && z > holes[i].y && z < holes[i].y + holes[i].height) return true; return false; }
// basement stairwells: openings in a building's ground floor (in the building's own frame) where the ground drops to the basement
#define MAX_PIT 600
typedef struct { Rectangle r; short f; float x0, z0, x1, z1; } Pit;   // r in frame f; x0..z1 its world bounds
static Pit pits[MAX_PIT]; static int nPit;
static Vector3 xfInv(int f, Vector3 p);
static bool inPit(float x, float z) {
    for (int i = 0; i < nPit; i++) { Pit *q = &pits[i]; if (x < q->x0 || x > q->x1 || z < q->z0 || z > q->z1) continue;
        Vector3 L = xfInv(q->f, V3(x, 0, z)); if (L.x > q->r.x && L.x < q->r.x + q->r.width && L.z > q->r.y && L.z < q->r.y + q->r.height) return true; }
    return false;
}
static bool gBasementAll = false;   // the high-rise district: every building that can have one gets a basement
static bool rectOverlap(Rectangle a, Rectangle b) { return a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y; }
// building footprints in the block being generated (their floor replaces the sidewalk surface, so nothing overlaps)
#define MAX_FOOT 16
static Rectangle foots[MAX_FOOT]; static float footH[MAX_FOOT]; static int nFoot;
// ground surface split around cut-outs and into <= 8 m tiles: big triangles + PS1 vertex snapping cause depth shimmer
static void tileBox(int t, Rectangle r, float y0, float y1, Color c, int mask) {
    const float TS = 8.0f;
    int nx = (int)ceilf(r.width / TS), nz = (int)ceilf(r.height / TS);
    for (int i = 0; i < nx; i++) for (int j = 0; j < nz; j++) {
        float x0 = r.x + i * r.width / nx, x1 = r.x + (i + 1) * r.width / nx, z0 = r.y + j * r.height / nz, z1 = r.y + (j + 1) * r.height / nz;
        box(t, V3(x0, y0, z0), V3(x1, y1, z1), c, false, mask);
    }
}
static void cutBox(int t, Rectangle r, float y0, float y1, Color c, int mask, Rectangle *cut, int ncut) {
    if (r.width <= 0.01f || r.height <= 0.01f) return;
    for (int i = 0; i < ncut; i++) {
        Rectangle h = cut[i];
        if (!rectOverlap(r, h)) continue;
        float hx0 = fmaxf(h.x, r.x), hx1 = fminf(h.x + h.width, r.x + r.width), hz0 = fmaxf(h.y, r.y), hz1 = fminf(h.y + h.height, r.y + r.height);
        cutBox(t, (Rectangle){r.x, r.y, hx0 - r.x, r.height}, y0, y1, c, mask, cut, ncut);
        cutBox(t, (Rectangle){hx1, r.y, r.x + r.width - hx1, r.height}, y0, y1, c, mask, cut, ncut);
        cutBox(t, (Rectangle){hx0, r.y, hx1 - hx0, hz0 - r.y}, y0, y1, c, mask, cut, ncut);
        cutBox(t, (Rectangle){hx0, hz1, hx1 - hx0, r.y + r.height - hz1}, y0, y1, c, mask, cut, ncut);
        return;
    }
    tileBox(t, r, y0, y1, c, mask);
}
// a ground box with every hole cut out of it
static void groundBox(int t, Rectangle r, float y0, float y1, Color c, bool solid, int mask) {
    if (r.width <= 0.01f || r.height <= 0.01f) return;
    for (int i = 0; i < nHole; i++) {
        Rectangle h = holes[i];
        if (!rectOverlap(r, h)) continue;
        float hx0 = fmaxf(h.x, r.x), hx1 = fminf(h.x + h.width, r.x + r.width), hz0 = fmaxf(h.y, r.y), hz1 = fminf(h.y + h.height, r.y + r.height);
        groundBox(t, (Rectangle){r.x, r.y, hx0 - r.x, r.height}, y0, y1, c, solid, mask);
        groundBox(t, (Rectangle){hx1, r.y, r.x + r.width - hx1, r.height}, y0, y1, c, solid, mask);
        groundBox(t, (Rectangle){hx0, r.y, hx1 - hx0, hz0 - r.y}, y0, y1, c, solid, mask);
        groundBox(t, (Rectangle){hx0, hz1, hx1 - hx0, r.y + r.height - hz1}, y0, y1, c, solid, mask);
        return;
    }
    box(t, V3(r.x, y0, r.y), V3(r.x + r.width, y1, r.y + r.height), c, solid, mask);
}
// slab with one rectangular opening (stairwell)
static void slabWithHole(int t, Rectangle r, Rectangle h, float y0, float y1, Color c) {
    if (h.width <= 0) { box(t, V3(r.x, y0, r.y), V3(r.x + r.width, y1, r.y + r.height), c, true, 47); return; }
    float hx0 = fmaxf(h.x, r.x), hx1 = fminf(h.x + h.width, r.x + r.width), hz0 = fmaxf(h.y, r.y), hz1 = fminf(h.y + h.height, r.y + r.height);
    if (hx0 > r.x + 0.01f) box(t, V3(r.x, y0, r.y), V3(hx0, y1, r.y + r.height), c, true, 47);
    if (hx1 < r.x + r.width - 0.01f) box(t, V3(hx1, y0, r.y), V3(r.x + r.width, y1, r.y + r.height), c, true, 47);
    if (hz0 > r.y + 0.01f) box(t, V3(hx0, y0, r.y), V3(hx1, y1, hz0), c, true, 47);
    if (hz1 < r.y + r.height - 0.01f) box(t, V3(hx0, y0, hz1), V3(hx1, y1, r.y + r.height), c, true, 47);
}

// ---------------------------------------------------------------- wilds: Frutiger Aero hills that should not exist
// ---------------------------------------------------------------- v0.5 layout: block mask, baked heightfield, sea, tunnels
// The city is an NB x NB grid with the south-west blocks cut away for the bay. Everything outside the city is a baked
// 2 m heightfield (hills, the bay floor, the east mountain, flattened road corridors, ridges over tunnels).
enum { BK_NORMAL, BK_GONE, BK_OLD, BK_PARK, BK_STATION, BK_MARKET, BK_PLAZA };   // values match tools/export5b.py
static unsigned char blockKindA[NB][NB];
static bool blockOn(int i, int j) { return i >= 0 && j >= 0 && i < NB && j < NB && blockKindA[i][j] != BK_GONE; }
static bool segV(int i, int j) { return j >= 0 && j < NB && i >= 0 && i <= NB && (blockOn(i - 1, j) || blockOn(i, j)); }   // vertical road strip at column line i, beside block row j
static bool segH(int i, int j) { return i >= 0 && i < NB && j >= 0 && j <= NB && (blockOn(i, j - 1) || blockOn(i, j)); }
static bool nodeOn(int i, int j) { return blockOn(i - 1, j - 1) || blockOn(i, j - 1) || blockOn(i - 1, j) || blockOn(i, j); }
static bool inCore(float x, float z);
static bool cityLand(float x, float z) {   // the town's ground (the fixed Atami-style town: zk_design.h)
    return inCore(x, z);
    if (x < 0 || z < 0 || x > CITY || z > CITY) return false;
    int ci = (int)(x / PITCH), cj = (int)(z / PITCH); float u = x - ci * PITCH, v = z - cj * PITCH;
    bool rx = u < ROADW, rz = v < ROADW;
    if (rx && rz) return nodeOn(ci, cj);
    if (rx) return segV(ci, cj);
    if (rz) return segH(ci, cj);
    return blockOn(ci, cj);
}
#define HF_S 2.0f
#define HF_X0 (-WEST)
#define HF_N ((int)((CITY + WILD + WEST) / HF_S) + 1)
#define HF_OLD_X0 (-150.0f)   // terrain files written before 0.9.0 counted from here
static bool hfOut(float x, float z) { return z < -WILD - 30 || z > CITY + WILD + 2 || x > CITY + WILD + 2; }   // grid rows outside the world (the grid is square, the world is not)
static float *hf, *hfd;      // heights, distance to city land (both HF_N x HF_N)
static bool hfReady = false;
static float *terrEdit = NULL;   // the Mod Maker's terrain brush: metres added to the ground (HF_N x HF_N), outside the town
static float hfAt(const float *a, float x, float z) {
    float fx = (x - HF_X0) / HF_S, fz = (z - HF_X0) / HF_S;
    fx = clampf(fx, 0, HF_N - 1.001f); fz = clampf(fz, 0, HF_N - 1.001f);
    int ix = (int)fx, iz = (int)fz; float tx = fx - ix, tz = fz - iz;
    const float *r0 = a + iz * HF_N, *r1 = a + (iz + 1) * HF_N;
    return (r0[ix] * (1 - tx) + r0[ix + 1] * tx) * (1 - tz) + (r1[ix] * (1 - tx) + r1[ix + 1] * tx) * tz;
}
static float cityDist(float x, float z) { if (cityLand(x, z)) return 0; return hfReady ? hfAt(hfd, x, z) : 50; }
static float terrainH(float x, float z) { return hfReady ? hfAt(hf, x, z) : -0.3f; }
// tunnels: axis-aligned bores (dir 0 +x, 1 -x, 2 +z, 3 -z) from a portal into the hills, ending in a collapse
typedef struct { float px, pz, fy, w, h, len; int dir, kind, through; } Bore;   // kind 0 road tunnel, 1 old brick tunnel; through 1: open at the far end, 2: that far portal (its face only)
#define MAX_BORE 8
static Bore bores[MAX_BORE]; static int nBore;
static int inBore(float x, float z, float y) {   // index+1 when (x,z,y) is inside a tunnel's interior volume
    for (int i = 0; i < nBore; i++) { Bore *b = &bores[i];
        if (y < b->fy - 1 || y > b->fy + b->h + 0.5f) continue;
        float a = b->dir == 0 ? x - b->px : b->dir == 1 ? b->px - x : b->dir == 2 ? z - b->pz : b->pz - z;
        float l = (b->dir < 2) ? z - b->pz : x - b->px;
        if (a >= -0.5f && a <= b->len + 2 && fabsf(l) <= b->w / 2 + 0.6f) return i + 1; }
    return 0;
}
static float coast_d(float x, float z); static float coastSigned(float x, float z); static bool coastSeaward(float x, float z); static bool mainRoadAt(float x, float z, float m); static float polyEdgeDist(const float (*p)[2], int n, float x, float z);
static float roadRastAt(float x, float z);
static float bankAt(float x, float z);
static float baseGroundY(float x, float z, float y) {
    if (cityLand(x, z)) { if (inHole(x, z) || inPit(x, z)) return -1e9f; float r = roadRastAt(x, z); return (r > SIDEH + 0.05f || (r > ROADY - 0.05f && r < SIDEH - 0.06f)) && y > r - 1.0f ? r : SIDEH; }   // the streets lie a kerb below the town (a road climbing out of town starts on town land)
    int b = inBore(x, z, y); if (b) return bores[b - 1].fy;
    float h = terrainH(x, z);
    { float r = roadRastAt(x, z); if (r > -1e8f && y > r - 1.0f) return r; }   // a road or paved strip out of town: the drawn surface (0.9.14: the ground under it is laid just below it)
    { float bk = bankAt(x, z); if (bk > h) h = bk; }   // (0.9.14: the bank built up beside a road)
    if (h > -4.0f && h < SIDEH - 0.3f && !cityLand(x, z) && coast_d(x, z) < 11 && coast_d(x, z) > 5.0f && coastSeaward(x, z)) return fminf(h, -3.64f);   // below road level on the sea side: the flat apron at the foot of the seawall
    return h;
}
static float baseGround(float x, float z) { return baseGroundY(x, z, 1e9f); }
static bool inRiverRaw(float x, float z);
static float coastSigned(float x, float z);
static float seaDepth(float x, float z) { if (cityLand(x, z)) return 0; if (inRiverRaw(x, z)) return 1.0f; return fmaxf(0, -4.0f - terrainH(x, z)); }   // SEA_Y
static float touge_d(float x, float z);   // distance to the mountain pass road (zk_layout.h)
static bool inCore(float x, float z);
static float coast_d(float x, float z);
static float coastSigned(float x, float z);
static float wildK(Vector3 p) {
    if (p.y < -40 || inBore(p.x, p.z, p.y)) return 0;
    float k = clampf((cityDist(p.x, p.z) - 4) / 26, 0, 1);
    if (hfReady) { if (coast_d(p.x, p.z) < 30) k = fminf(k, 0.08f); if (touge_d(p.x, p.z) < 40) k = fminf(k, 0.15f); }
    return k;
}
static Model terrain; static bool terrainOk = false; static Texture2D blissTex;
// 0.9.14: the ground in 64 m tiles, so only what is in front of you and short of the fog is drawn (it was one model of millions of vertices)
#define TER_TILE 64.0f
#define TER_MAXT 400
static Model terTiles[TER_MAXT]; static Vector3 terTMn[TER_MAXT], terTMx[TER_MAXT]; static int nTerT;
static Texture2D makeBliss(void) {   // saturated XP-wallpaper grass
    int S = 64; Image im = GenImageColor(S, S, (Color){72, 168, 52, 255});
    unsigned int r = 31;
    Color pal[4] = {{96, 196, 64, 255}, {58, 146, 44, 255}, {124, 212, 80, 255}, {84, 178, 58, 255}};
    for (int k = 0; k < 520; k++) { r = r * 1103515245u + 12345u; int x = (r >> 8) % S; r = r * 1103515245u + 12345u; int y = (r >> 8) % S;
        int c = (r >> 4) % 4; for (int j = 0; j < 3; j++) ImageDrawPixel(&im, x, (y - j + S) % S, pal[c]); }
    Texture2D t = LoadTextureFromImage(im); UnloadImage(im);
    SetTextureFilter(t, TEXTURE_FILTER_POINT); SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
    return t;
}
static bool terrainSkip(float ax, float az);   // zk_layout.h: cells hidden behind tunnel headwalls
static float terrainSkipY(float ax, float az); static bool terrMouth(float x, float z, float lo, float hi);
static bool terPaveTint;   // 0.9.14: town ground under the paving: concrete grey, not grass
static void terrVert(Mesh *m, int *vp, float x, float z, float y, Vector3 nn) {
    int v = *vp; if (!m->vertices) { *vp = v + 1; return; }   // (counting pass)
    m->vertices[v * 3] = x; m->vertices[v * 3 + 1] = y; m->vertices[v * 3 + 2] = z;
    m->normals[v * 3] = nn.x; m->normals[v * 3 + 1] = nn.y; m->normals[v * 3 + 2] = nn.z;
    m->texcoords[v * 2] = x / 5; m->texcoords[v * 2 + 1] = z / 5;
    float sh = 0.82f + 0.18f * nn.y;
    // tint: wet sand at the waterline, dark cedar on the mountain, grey rock near its top
    float r = 1, g = 1, b = 1, flat = 0;   // flat > 0: the vertex colour replaces the grass texture by that much (see the shader)
    if (!cityLand(x, z)) {   // blended per vertex, so there are no saw-tooth edges between grass, sand and concrete
        float sand = clampf((-1.6f - y) / 1.2f, 0, 1);   // sand below the seawall and under the water
        float face = 0; if (hfReady && y > -4.4f && y < SIDEH) { float cd = coast_d(x, z); face = clampf((SIDEH - 0.35f - y) / 0.5f, 0, 1) * clampf((y + 3.9f) / 0.5f, 0, 1) * clampf((12 - cd) / 3, 0, 1); }   // the seawall
        float sr = Lerp(0.78f, 0.6f, face), sg = Lerp(0.72f, 0.6f, face), sb = Lerp(0.56f, 0.58f, face); flat = fmaxf(sand, face);
        r = Lerp(r, sr, flat); g = Lerp(g, sg, flat); b = Lerp(b, sb, flat); }
    float mt = clampf((y - 12) / 14, 0, 1); r = Lerp(r, 0.55f, mt); g = Lerp(g, 0.72f, mt); b = Lerp(b, 0.62f, mt);
    float rk = clampf((y - 34) / 10, 0, 1); r = Lerp(r, 1.45f, rk); g = Lerp(g, 0.95f, rk); b = Lerp(b, 1.05f, rk);
    if (terPaveTint) { r = 0.66f; g = 0.66f; b = 0.64f; flat = 1; }
    m->colors[v * 4] = (unsigned char)clampf(255 * sh * r, 0, 255); m->colors[v * 4 + 1] = (unsigned char)clampf(255 * sh * g, 0, 255);
    m->colors[v * 4 + 2] = (unsigned char)clampf(255 * sh * b, 0, 255); m->colors[v * 4 + 3] = flat > 0.01f ? (unsigned char)(255 * (0.6f - 0.09f * flat)) : 255;
    *vp = v + 1;
}
#define TSUB 4   // cells along the town's edge and under roads are laid in 0.5 m pieces, so grass meets paving and asphalt along a fine line, not 2 m steps
static bool rvCorridor(float x, float z);
// 0.9.13: the ground is one closed solid, like a cube: a top over every cell of the world (no holes: under the town, the roads,
// a tunnel's headwall or the seawall it is laid lower, out of sight, instead of being left out), upright faces wherever two
// pieces of the top do not meet (and round the edge of the world), and a floor under it all. Only a tunnel's mouth is open.
#define TER_BOT (-20.0f)
static unsigned char *terT;   // per 0.5 m column: 0 none (outside the world), 1 ground, 2 town (laid far down), 3 under a road (a little down), 4 behind a headwall / at the seawall (down to terLow), +8 river bed
static float *terLow;          // per 2 m cell: how low a type-4 column lies
static int terNS;              // columns along a side
static int terIdx(int I, int J) { return J * terNS + I; }
static float terTop(int t, float base, int ci, int cj, int n) {
    if (t & 8) base = fminf(base, -2.3f);
    switch (t & 7) { case 2: return -12.0f; case 5: return SIDEH - 0.2f; case 3: return base - 0.15f; case 4: return fminf(base, terLow[cj * n + ci]); default: return base; }
}
static float terBase(int ci, int cj, float uu, float ww) {   // the heightfield over cell (ci, cj) at (uu, ww) in 0..1
    float h00 = hf[cj * HF_N + ci], h10 = hf[cj * HF_N + ci + 1], h01 = hf[(cj + 1) * HF_N + ci], h11 = hf[(cj + 1) * HF_N + ci + 1];
    return (h00 * (1 - uu) + h10 * uu) * (1 - ww) + (h01 * (1 - uu) + h11 * uu) * ww;
}
// 0.9.14: the ground meets every road and pavement out of town: under one it lies just below the drawn surface (never above it, never far below),
// and beside one it is built up in a bank that falls away at about 30 degrees to the natural ground (no road or pavement standing on air)
static float *terEmb; static unsigned char *terPave; static int terNV;
static float bankAt(float x, float z) {
    if (!terEmb) return -1e9f; int I = (int)lroundf((x - HF_X0) / (HF_S / TSUB)), J = (int)lroundf((z - HF_X0) / (HF_S / TSUB));
    if (I < 0 || J < 0 || I >= terNV || J >= terNV) return -1e9f; size_t k = (size_t)J * terNV + I; return terPave[k] ? -1e9f : terEmb[k];
}
static float terH(int t, int ci, int cj, int n, float uu, float ww) {
    float base = terBase(ci, cj, uu, ww);
    if (terEmb && (t & 7) != 2) { int vi = ci * TSUB + (int)lroundf(uu * TSUB), vj = cj * TSUB + (int)lroundf(ww * TSUB); size_t k = (size_t)vj * terNV + vi; float e = terEmb[k];
        if (terPave[k]) { if ((t & 7) == 4) return fminf(e, terLow[cj * n + ci]); return e; }   // under a road or pavement: just below it
        if (e > base && (t & 7) == 1 && !(t & 8)) return e; }
    return terTop(t, base, ci, cj, n);
}
static void genTerrain(void) {
    if (terrainOk) { for (int k = 0; k < nTerT; k++) UnloadModel(terTiles[k]); nTerT = 0; terrainOk = false; }
    const float S = HF_S, q = S / TSUB; int n = HF_N - 1, NS = n * TSUB; terNS = NS;
    free(terT); free(terLow); terT = calloc((size_t)NS * NS, 1); terLow = malloc(sizeof(float) * (size_t)n * n);
    bool *uni = calloc((size_t)n * n, 1);   // the whole cell one kind: one quad
    for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) {
        float ax = HF_X0 + i * S, az = HF_X0 + j * S; terLow[j * n + i] = -1e9f;
        if (hfOut(ax, az)) continue;
        float sy = terrainSkipY(ax, az); terLow[j * n + i] = sy;
        int nl = 0, np2 = 0; for (int c = 0; c < 4; c++) { float x = ax + ((c & 1) ? S - 0.1f : 0.1f), z = az + ((c & 2) ? S - 0.1f : 0.1f); nl += cityLand(x, z); np2 += inCore(x, z); }
        bool road = hfReady && mainRoadAt(ax + S / 2, az + S / 2, 1.6f), rc = rvCorridor(ax + S / 2, az + S / 2);
        int whole = sy > -1e8f ? 4 : (nl == 4 || np2 == 4) ? 2 : (nl == 0 && np2 == 0 && !road) ? 1 : 0;
        if (whole && !rc) { uni[j * n + i] = 1; for (int b = 0; b < TSUB; b++) for (int a = 0; a < TSUB; a++) terT[terIdx(i * TSUB + a, j * TSUB + b)] = (unsigned char)whole; continue; }
        for (int b = 0; b < TSUB; b++) for (int a = 0; a < TSUB; a++) { float x = ax + (a + 0.5f) * q, z = az + (b + 0.5f) * q;
            int t = whole && whole != 2 ? whole : (cityLand(x, z) || inCore(x, z)) ? 2 : (hfReady && mainRoadAt(x, z, 0.05f)) ? 3 : 1;
            if (rvCorridor(x, z) && t != 2) t |= 8;
            terT[terIdx(i * TSUB + a, j * TSUB + b)] = (unsigned char)t; } }
    if (getenv("KIRI_TERRDBG")) { float tx, tz; sscanf(getenv("KIRI_TERRDBG"), "%f %f", &tx, &tz);
        for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) { float ax = HF_X0 + i * S, az = HF_X0 + j * S; if (fabsf(ax + 1 - tx) > 9 || fabsf(az + 1 - tz) > 9) continue;
            TraceLog(LOG_INFO, "TCELL %.0f %.0f type %d uni %d low %.2f h %.2f..%.2f", ax, az, terT[terIdx(i * TSUB, j * TSUB)], uni[j * n + i], terLow[j * n + i], fminf(hf[j * HF_N + i], hf[(j + 1) * HF_N + i + 1]), fmaxf(hf[j * HF_N + i], hf[(j + 1) * HF_N + i + 1])); } }
    {   // 0.9.14: town ground lies just under the paving (no void under a gap in it), except under buildings (basements), holes, pits and the river, where it stays far down
        unsigned char *fm = calloc((size_t)NS * NS, 1);
        for (int f = 0; f < nMapFoot; f++) { Rectangle r = mapFoot[f]; Vector3 c4[4] = {V3(r.x, 0, r.y), V3(r.x + r.width, 0, r.y), V3(r.x, 0, r.y + r.height), V3(r.x + r.width, 0, r.y + r.height)};
            float mnx = 1e9f, mxx = -1e9f, mnz = 1e9f, mxz = -1e9f; for (int k = 0; k < 4; k++) { Vector3 w4 = mapFootF[f] ? xfP(mapFootF[f], c4[k]) : c4[k]; mnx = fminf(mnx, w4.x); mxx = fmaxf(mxx, w4.x); mnz = fminf(mnz, w4.z); mxz = fmaxf(mxz, w4.z); }
            int i0 = (int)floorf((mnx - 0.6f - HF_X0) / q), i1 = (int)ceilf((mxx + 0.6f - HF_X0) / q), j0 = (int)floorf((mnz - 0.6f - HF_X0) / q), j1 = (int)ceilf((mxz + 0.6f - HF_X0) / q);
            for (int J = j0 < 0 ? 0 : j0; J <= j1 && J < NS; J++) for (int I = i0 < 0 ? 0 : i0; I <= i1 && I < NS; I++) fm[terIdx(I, J)] = 1; }
        #define FMRECT(mnx_, mnz_, mxx_, mxz_) do { int i0 = (int)floorf(((mnx_) - 0.6f - HF_X0) / q), i1 = (int)ceilf(((mxx_) + 0.6f - HF_X0) / q), j0 = (int)floorf(((mnz_) - 0.6f - HF_X0) / q), j1 = (int)ceilf(((mxz_) + 0.6f - HF_X0) / q); \
            for (int J = j0 < 0 ? 0 : j0; J <= j1 && J < NS; J++) for (int I = i0 < 0 ? 0 : i0; I <= i1 && I < NS; I++) fm[terIdx(I, J)] = 1; } while (0)
        for (int h2 = 0; h2 < nHole; h2++) FMRECT(holes[h2].x, holes[h2].y, holes[h2].x + holes[h2].width, holes[h2].y + holes[h2].height);
        for (int p2 = 0; p2 < nPit; p2++) FMRECT(pits[p2].x0, pits[p2].z0, pits[p2].x1, pits[p2].z1);
        #undef FMRECT
        int n5 = 0;
        for (int J = 0; J < NS; J++) for (int I = 0; I < NS; I++) { size_t k = terIdx(I, J); if ((terT[k] & 7) != 2 || fm[k]) continue;
            float x = HF_X0 + (I + 0.5f) * q, z = HF_X0 + (J + 0.5f) * q;
            if (rvCorridor(x, z)) continue;
            terT[k] = 5; n5++; uni[(J / TSUB) * n + I / TSUB] = 0; }
        free(fm); TraceLog(LOG_INFO, "TERRAIN town ground under the paving: %d pieces", n5); }
    {   // the banks along roads and pavements (see terH)
        int NV = NS + 1; terNV = NV; free(terEmb); free(terPave); terEmb = malloc(sizeof(float) * (size_t)NV * NV); terPave = calloc((size_t)NV * NV, 1); float *bv = malloc(sizeof(float) * (size_t)NV * NV);
        for (int J = 0; J < NV; J++) for (int I = 0; I < NV; I++) { int ci = I / TSUB, cj = J / TSUB; if (ci >= n) ci = n - 1; if (cj >= n) cj = n - 1;
            float uu = (float)(I - ci * TSUB) / TSUB, ww = (float)(J - cj * TSUB) / TSUB, x = HF_X0 + I * q, z = HF_X0 + J * q; size_t k = (size_t)J * NV + I;
            bv[k] = terBase(ci, cj, uu, ww); terEmb[k] = -1e9f;
            float r = fmaxf(fmaxf(roadRastAt(x - 0.25f, z - 0.25f), roadRastAt(x + 0.25f, z - 0.25f)), fmaxf(roadRastAt(x - 0.25f, z + 0.25f), roadRastAt(x + 0.25f, z + 0.25f)));
            if (r > -1e8f && !inCore(x, z)) { terEmb[k] = r - 0.12f; terPave[k] = 1; } }
        const float SL = 0.6f * q, SD = 0.6f * q * 1.41421f;
        for (int pass = 0; pass < 4; pass++) { bool fw = !(pass & 1);
            for (int jj = 0; jj < NV; jj++) { int J = fw ? jj : NV - 1 - jj; for (int ii = 0; ii < NV; ii++) { int I = fw ? ii : NV - 1 - ii; size_t k = (size_t)J * NV + I; if (terPave[k]) continue;
                float best = terEmb[k]; int dj = fw ? -1 : 1;
                for (int di = -1; di <= 1; di++) { int I2 = I + di, J2 = J + dj; if (I2 >= 0 && I2 < NV && J2 >= 0 && J2 < NV) { float c = terEmb[(size_t)J2 * NV + I2] - (di ? SD : SL); if (c > best) best = c; } }
                { int I2 = I + (fw ? -1 : 1); if (I2 >= 0 && I2 < NV) { float c = terEmb[(size_t)J * NV + I2] - SL; if (c > best) best = c; } }
                if (best > bv[k] + 0.01f) terEmb[k] = best; } } }
        int nb = 0; for (size_t k = 0; k < (size_t)NV * NV; k++) if (!terPave[k] && terEmb[k] > -1e8f) nb++;
        for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) if (uni[j * n + i]) { bool any = false;   // a cell with a bank or a road in it is drawn in its 0.5 m pieces
            for (int b = 0; b <= TSUB && !any; b++) for (int a = 0; a <= TSUB && !any; a++) { size_t k = (size_t)(j * TSUB + b) * NV + i * TSUB + a; if (terEmb[k] > -1e8f) any = true; }
            if (any) uni[j * n + i] = 0; }
        free(bv); TraceLog(LOG_INFO, "TERRAIN banks: %d points raised beside roads", nb); }
    for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) {   // a cell all of one kind, with no road or bank in it, is one quad again
        int t0 = terT[terIdx(i * TSUB, j * TSUB)]; bool same = true;
        for (int b = 0; b < TSUB && same; b++) for (int a = 0; a < TSUB && same; a++) if (terT[terIdx(i * TSUB + a, j * TSUB + b)] != t0) same = false;
        for (int b = 0; b <= TSUB && same; b++) for (int a = 0; a <= TSUB && same; a++) if (terEmb[(size_t)(j * TSUB + b) * terNV + i * TSUB + a] > -1e8f) same = false;
        uni[j * n + i] = same && !rvCorridor(HF_X0 + i * S + S / 2, HF_X0 + j * S + S / 2); }
    #define HFN(gi, gj) ({ int gi_ = (gi), gj_ = (gj); float hx_ = hf[gj_ * HF_N + (gi_ < HF_N - 1 ? gi_ + 1 : gi_)] - hf[gj_ * HF_N + (gi_ > 0 ? gi_ - 1 : gi_)], hz_ = hf[(gj_ < HF_N - 1 ? gj_ + 1 : gj_) * HF_N + gi_] - hf[(gj_ > 0 ? gj_ - 1 : gj_) * HF_N + gi_]; Vector3Normalize(V3(-hx_, 2 * HF_S, -hz_)); })
    Mesh m = {0}; int v = 0, walls = 0;
    for (int pass = 0; pass < 2; pass++) {
        if (pass == 1) { m.vertexCount = v; m.triangleCount = v / 3; m.vertices = MemAlloc(v * 3 * sizeof(float)); m.texcoords = MemAlloc(v * 2 * sizeof(float)); m.normals = MemAlloc(v * 3 * sizeof(float)); m.colors = MemAlloc(v * 4); v = 0; walls = 0; }
        // the top
        for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) {
            int t0 = terT[terIdx(i * TSUB, j * TSUB)]; if (!t0) continue;
            Vector3 n00 = HFN(i, j), n10 = HFN(i + 1, j), n01 = HFN(i, j + 1), n11 = HFN(i + 1, j + 1); float ax = HF_X0 + i * S, az = HF_X0 + j * S;
            int sub = uni[j * n + i] ? 1 : TSUB;
            for (int b = 0; b < sub; b++) for (int a = 0; a < sub; a++) {
                int t = uni[j * n + i] ? t0 : terT[terIdx(i * TSUB + a, j * TSUB + b)]; if (!t) continue; terPaveTint = (t & 7) == 5;
                float u[4] = {(float)a / sub, (float)a / sub, (float)(a + 1) / sub, (float)(a + 1) / sub}, w[4] = {(float)b / sub, (float)(b + 1) / sub, (float)(b + 1) / sub, (float)b / sub};
                int ord[6] = {0, 1, 2, 0, 2, 3};
                for (int k = 0; k < 6; k++) { float uu = u[ord[k]], ww = w[ord[k]], y = terH(t, i, j, n, uu, ww);
                    Vector3 nn = Vector3Normalize(Vector3Add(Vector3Scale(Vector3Add(Vector3Scale(n00, 1 - uu), Vector3Scale(n10, uu)), 1 - ww), Vector3Scale(Vector3Add(Vector3Scale(n01, 1 - uu), Vector3Scale(n11, uu)), ww)));
                    terrVert(&m, &v, ax + uu * S, az + ww * S, y, nn); } } }
        // the sides: between every two 0.5 m columns whose tops do not meet, and along the edge of the world (down to the floor)
        for (int J = 0; J < NS; J++) for (int I = 0; I < NS; I++) { int t = terT[terIdx(I, J)]; if (!t) continue;
            int ci = I / TSUB, cj = J / TSUB; float ua = (float)(I % TSUB) / TSUB, wa = (float)(J % TSUB) / TSUB;
            for (int e = 0; e < 4; e++) {   // e 0: +x, 1: +z, 2: -x, 3: -z (the -x / -z sides only where there is no column)
                int I2 = I + (e == 0) - (e == 2), J2 = J + (e == 1) - (e == 3);
                int t2 = (I2 < 0 || J2 < 0 || I2 >= NS || J2 >= NS) ? 0 : terT[terIdx(I2, J2)];
                if (e >= 2 && t2) continue;
                float u0, w0, u1, w1; if (e == 0 || e == 2) { u0 = u1 = ua + (e == 0 ? 1.0f / TSUB : 0); w0 = wa; w1 = wa + 1.0f / TSUB; } else { w0 = w1 = wa + (e == 1 ? 1.0f / TSUB : 0); u0 = ua; u1 = ua + 1.0f / TSUB; }
                float a0 = terH(t, ci, cj, n, u0, w0), a1 = terH(t, ci, cj, n, u1, w1), c0, c1;
                if (t2) { int ci2 = I2 / TSUB, cj2 = J2 / TSUB; float x0w = ci + u0, z0w = cj + w0, x1w = ci + u1, z1w = cj + w1;   // the same two points, seen from the neighbouring cell
                    c0 = terH(t2, ci2, cj2, n, x0w - ci2, z0w - cj2); c1 = terH(t2, ci2, cj2, n, x1w - ci2, z1w - cj2); } else { c0 = c1 = TER_BOT; }
                if (fabsf(a0 - c0) < 0.003f && fabsf(a1 - c1) < 0.003f) continue;
                float x0 = HF_X0 + (ci + u0) * S, z0 = HF_X0 + (cj + w0) * S, x1 = HF_X0 + (ci + u1) * S, z1 = HF_X0 + (cj + w1) * S;
                if (terrMouth((x0 + x1) / 2, (z0 + z1) / 2, fminf(fminf(a0, c0), fminf(a1, c1)), fmaxf(fmaxf(a0, c0), fmaxf(a1, c1)))) continue;   // the tunnel's mouth stays open
                Vector3 nn = e == 0 ? V3(1, 0, 0) : e == 1 ? V3(0, 0, 1) : e == 2 ? V3(-1, 0, 0) : V3(0, 0, -1); terPaveTint = (t & 7) == 5 || (t2 & 7) == 5;
                Vector3 P[4] = {V3(x0, c0, z0), V3(x1, c1, z1), V3(x1, a1, z1), V3(x0, a0, z0)};
                int o1[6] = {0, 1, 2, 0, 2, 3}, o2[6] = {0, 2, 1, 0, 3, 2};
                for (int k = 0; k < 6; k++) terrVert(&m, &v, P[o1[k]].x, P[o1[k]].z, P[o1[k]].y, nn);
                for (int k = 0; k < 6; k++) terrVert(&m, &v, P[o2[k]].x, P[o2[k]].z, P[o2[k]].y, Vector3Negate(nn));
                walls++; } }
        terPaveTint = false;
        {   // the floor under it all
            float x0 = HF_X0, x1 = HF_X0 + n * S, z0 = HF_X0, z1 = HF_X0 + n * S; Vector3 P[4] = {V3(x0, TER_BOT, z0), V3(x1, TER_BOT, z0), V3(x1, TER_BOT, z1), V3(x0, TER_BOT, z1)}; int o[6] = {0, 1, 2, 0, 2, 3};
            for (int k = 0; k < 6; k++) terrVert(&m, &v, P[o[k]].x, P[o[k]].z, P[o[k]].y, V3(0, -1, 0)); }
    }
    #undef HFN
    free(uni);   // (terEmb / terPave stay: walking follows the banks too)
    TraceLog(LOG_INFO, "TERRAIN closed: %d vertices, %d side pieces", v, walls);
    if (!blissTex.id) blissTex = makeBliss();
    {   // cut into tiles by where each triangle lies
        int TS = (int)ceilf(n * S / TER_TILE); if (TS * TS > TER_MAXT - 1) TS = 19; int nt = TS * TS + 1;   // (+1: the floor under it all)
        int *cnt = calloc((size_t)nt, sizeof(int)), *tri = malloc(sizeof(int) * (size_t)(v / 3));
        for (int k = 0; k < v / 3; k++) { float cx = (m.vertices[k * 9] + m.vertices[k * 9 + 3] + m.vertices[k * 9 + 6]) / 3, cz = (m.vertices[k * 9 + 2] + m.vertices[k * 9 + 5] + m.vertices[k * 9 + 8]) / 3, cy = m.vertices[k * 9 + 1];
            int ti = (int)((cx - HF_X0) / TER_TILE), tj = (int)((cz - HF_X0) / TER_TILE); ti = ti < 0 ? 0 : ti >= TS ? TS - 1 : ti; tj = tj < 0 ? 0 : tj >= TS ? TS - 1 : tj;
            int id = (cy <= TER_BOT + 0.01f && m.vertices[k * 9 + 4] <= TER_BOT + 0.01f && m.vertices[k * 9 + 7] <= TER_BOT + 0.01f && fabsf(m.vertices[k * 9] - m.vertices[k * 9 + 3]) > 100) ? TS * TS : tj * TS + ti;
            tri[k] = id; cnt[id]++; }
        nTerT = 0;
        for (int id = 0; id < nt; id++) { if (!cnt[id]) continue;
            Mesh t = {0}; t.vertexCount = cnt[id] * 3; t.triangleCount = cnt[id]; t.vertices = MemAlloc(t.vertexCount * 3 * sizeof(float)); t.texcoords = MemAlloc(t.vertexCount * 2 * sizeof(float)); t.normals = MemAlloc(t.vertexCount * 3 * sizeof(float)); t.colors = MemAlloc(t.vertexCount * 4);
            Vector3 mn = {1e9f, 1e9f, 1e9f}, mx = {-1e9f, -1e9f, -1e9f}; int o = 0;
            for (int k = 0; k < v / 3; k++) { if (tri[k] != id) continue;
                for (int q2 = 0; q2 < 3; q2++) { int src = k * 3 + q2, dst = o * 3 + q2;
                    memcpy(&t.vertices[dst * 3], &m.vertices[src * 3], 12); memcpy(&t.normals[dst * 3], &m.normals[src * 3], 12); memcpy(&t.texcoords[dst * 2], &m.texcoords[src * 2], 8); memcpy(&t.colors[dst * 4], &m.colors[src * 4], 4);
                    Vector3 P = {m.vertices[src * 3], m.vertices[src * 3 + 1], m.vertices[src * 3 + 2]}; mn = Vector3Min(mn, P); mx = Vector3Max(mx, P); }
                o++; }
            UploadMesh(&t, false); Model md = LoadModelFromMesh(t); md.materials[0].shader = sh; md.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = blissTex;
            terTiles[nTerT] = md; terTMn[nTerT] = mn; terTMx[nTerT] = mx; nTerT++; }
        free(cnt); free(tri); MemFree(m.vertices); MemFree(m.texcoords); MemFree(m.normals); MemFree(m.colors);
        TraceLog(LOG_INFO, "TERRAIN in %d tiles", nTerT); }
    terrainOk = true;
}
static void genWilds(void) {   // lone trees, white info pylons and a dead Aero billboard on the hills
    for (int k = 0; k < 90; k++) {
        float x = fr(-WILD + 10, CITY + WILD - 10), z = fr(-WILD + 10, CITY + WILD - 10);
        if (cityDist(x, z) < 18 || seaDepth(x, z) > 0 || terrainH(x, z) < 0.3f || coast_d(x, z) < 14 || touge_d(x, z) < 8) continue;
        float y = terrainH(x, z);
        if (k % 9 == 0) {
            box(T_PLAIN, V3(x - 0.3f, y - 0.5f, z - 0.3f), V3(x + 0.3f, y + 4.5f, z + 0.3f), rgb(240, 248, 250), true, ALL);
            box(T_GLASS, V3(x - 0.9f, y + 3.2f, z - 0.08f), V3(x + 0.9f, y + 4.4f, z + 0.08f), rgb(120, 200, 240), false, ALLB);
        } else {
            float h = fr(2.5f, 4.5f), cw = fr(1.6f, 2.8f);
            box(T_WOOD, V3(x - 0.18f, y - 0.4f, z - 0.18f), V3(x + 0.18f, y + h, z + 0.18f), rgb(140, 110, 80), true, ALL);
            box(T_HEDGE, V3(x - cw, y + h - 0.6f, z - cw), V3(x + cw, y + h + cw * 1.1f, z + cw), rgb(150, 240, 130), false, ALLB);
        }
    }
}

// ---------------------------------------------------------------- spawning at a position (all creature types)
static void crStats(Cr *c) {
    static const float hp[18] = {3, 5, 2, 2, 1, 6, 4, 3, 3, 12, 4, 3, 8, 7, 4, 10, 2, 5};
    static const float sp[18] = {3.2f, 2.4f, 4.8f, 6.0f, 3.6f, 2.4f, 3.0f, 6.5f, 4.6f, 2.2f, 3.0f, 5.0f, 1.6f, 4.2f, 3.4f, 2.6f, 4.4f, 3.0f};
    int t = c->type < 0 || c->type > 17 ? 0 : c->type;
    c->maxhp = c->hp = hp[t];
    c->spd = sp[t] * fr(0.92f, 1.1f);
    c->var = ri(4);
}
static int spawnAt(Vector3 p, int type, bool alerted) {
    int slot = -1;
    for (int i = 0; i < nCr; i++) if (!crs[i].on) { slot = i; break; }
    if (slot < 0) { if (nCr >= MAX_CR) return -1; slot = nCr++; }
    Cr *c = &crs[slot]; memset(c, 0, sizeof *c);
    memset(&gore[slot], 0, sizeof gore[slot]);   // (a fresh body)
    c->type = type; crStats(c);
    c->pos = p; c->yaw = fr(-PI, PI); c->on = true;
    c->state = alerted ? CS_CHASE : CS_WANDER; c->tgt = c->pos; c->groanT = fr(2, 10); c->t = fr(0, 100); c->lastKnown = p;
    return slot;
}
// indoor spawn: on a random floor next to a piece of furniture
static void spawnIndoor(Vector3 nearP, float minD) {
    for (int tries = 0; tries < 30; tries++) {
        int i = ri(nCont > 0 ? nCont : 1); if (nCont == 0) return;
        Cont *c = &conts[i]; if (!c->on || c->kind == CK_PILE || (c->region == 9 && c->pos.y < -5)) continue;   // not down in the subway (basements are fine)
        if (Vector3Distance(c->pos, nearP) < minD) continue;
        Vector3 p = Vector3Add(c->pos, V3(sinf(c->yaw) * 1.1f, 0, cosf(c->yaw) * 1.1f));
        if (!pointFree(p.x, p.z, 0.35f, p.y + 0.05f)) continue;
        float r = fr(0, 1);
        int t = r < 0.32f ? 0 : r < 0.44f ? 2 : r < 0.56f ? 7 : r < 0.68f ? 8 : r < 0.78f ? 11 : r < 0.88f ? 14 : 6;   // crawlers and nurses come from inside
        int s = spawnAt(p, t, false);
        if (s >= 0 && t == 8) { crs[s].hanging = true; crs[s].pos.y = p.y + 2.6f; }
        return;
    }
}

// ---------------------------------------------------------------- multi-floor building
// kinds: 0 house 1 apartment 2 shop 3 konbini 4 koban 5 station 6 eco-tower
static Rectangle gNoWinR = {0, 0, 0, 0};   // the stairwell shaft of the building being built (its outer wall gets no windows)
static int gExtTex = T_PLASTER; static Color gExtCol = {230, 228, 220, 255}; static int jpWinFilled;   // (the outside finish of the building being built: a window an inner wall runs into is walled up in it)
static void extFloor(Rectangle r, int kind, int street, float y0, float yTop, bool ground, int extTex, Color ec) {
    gExtTex = extTex; gExtCol = ec;
    curFY = y0;
    for (int s = 0; s < 4; s++) {
        float A, B; bool alongX = (s == 0 || s == 2);
        if (alongX) { A = r.x; B = r.x + r.width; } else { A = r.y + curT; B = r.y + r.height - curT; }
        float len = B - A; int n = (int)(len / 3.0f); if (n < 1) n = 1;
        float mw = len / n;
        int doorM = (ground && s == street) ? n / 2 : -1;
        if (kind == 4 && s == street && ground) doorM = 0;
        for (int m = 0; m < n; m++) {
            float u0 = A + m * mw, u1 = u0 + mw;
            bool storefront = ground && (((s == street) && (kind == 2 || kind == 3)) || kind == 6) && mw > 1.6f;
            if (m == doorM && mw > 1.6f) {
                float d0 = (u0 + u1) / 2 - 0.65f, d1 = (u0 + u1) / 2 + 0.65f;
                if (storefront && kind == 3) { d0 = u0 + 0.3f; d1 = u1 - 0.3f; }
                wallPiece(s, u0, d0, y0, yTop, extTex, ec);
                wallPiece(s, d1, u1, y0, yTop, extTex, ec);
                Vector3 lmx, lmn = sideMn(s, d0, d1, y0 + 2.3f, yTop, &lmx, 0);
                box(extTex, lmn, lmx, ec, false, ALLB);
                int st = (kind == 3 || kind == 4 || kind == 5) ? 0 : (fr(0, 1) < (kind == 6 ? 0.5f : 0.3f) ? 0 : 1);
                addOpening(s, d0, d1, y0, y0 + 2.3f, OP_DOOR, st);
                continue;
            }
            if (storefront && kind == 2 && fr(0, 1) < 0.5f) { wallPiece(s, u0, u1, y0, yTop, T_SHUTTER, WHITE); continue; }
            float winP = kind == 1 ? 0.6f : kind == 0 ? 0.5f : 0.45f;
            bool shaftWin = false; if (gNoWinR.width > 0) { Vector3 wmx, wmn = sideMn(s, u0, u1, y0, y0 + 1, &wmx, 0); shaftWin = wmn.x < gNoWinR.x + gNoWinR.width && wmx.x > gNoWinR.x && wmn.z < gNoWinR.y + gNoWinR.height && wmx.z > gNoWinR.y; }   // 0.9.8: no window where the stair flights run past it
            if (!shaftWin && (storefront || fr(0, 1) < winP)) {
                float p = storefront ? 0.15f : 0.4f;
                float sill = storefront ? 0.35f : 0.9f, top = storefront ? 2.6f : 2.1f;
                if (!storefront) {   // 0.9.8: windows of different sizes: standard, wide, narrow, a small high one, a long low strip
                    float r = fr(0, 1), ww;
                    if (r < 0.3f) { ww = mw - 0.8f; }
                    else if (r < 0.45f) { ww = mw - 0.5f; sill = 0.6f; top = 2.2f; }
                    else if (r < 0.7f) { ww = 0.9f; }
                    else if (r < 0.85f) { ww = 0.6f; sill = 1.4f; top = 2.0f; }
                    else { ww = mw - 0.6f; sill = 1.3f; top = 2.0f; }
                    ww = clampf(ww, 0.5f, mw - 0.5f); p = (mw - ww) / 2; }
                wallPiece(s, u0, u0 + p, y0, yTop, extTex, ec);
                wallPiece(s, u1 - p, u1, y0, yTop, extTex, ec);
                wallPiece(s, u0 + p, u1 - p, y0, y0 + sill, extTex, ec);
                wallPiece(s, u0 + p, u1 - p, y0 + top, yTop, extTex, ec);
                addOpening(s, u0 + p, u1 - p, y0 + sill, y0 + top, OP_WINDOW, 0);
            } else wallPiece(s, u0, u1, y0, yTop, extTex, ec);
        }
    }
    curFY = SIDEH;
}
static void addDebris(Vector3 mn, Vector3 mx) {
    if (nDeb >= MAX_DEB) return;
    Debris *d = &debs[nDeb++];
    d->mn = mn; d->mx = mx; d->hp = 7; d->alive = true; d->seed = ri(1000); d->f = (short)curXf;
    d->sid = addSolid(mn, mx);
}
static bool boxFree(float x0, float z0, float x1, float z1, float y) {   // no solid overlaps this footprint between y and y+1.8 (footprint in the current building frame)
    Vector3 wmn, wmx; xfBox(curXf, V3(x0, 0, z0), V3(x1, 0, z1), &wmn, &wmx);
    query(wmn.x, wmn.z, wmx.x, wmx.z);
    for (int i = 0; i < nSubRes; i++) if (y > SIDEH - 0.5f && wmx.x > subRes[i].x && wmn.x < subRes[i].x + subRes[i].width && wmx.z > subRes[i].y && wmn.z < subRes[i].y + subRes[i].height) return false;
    for (int k = 0; k < qn; k++) { Solid *s = &sol[qb[k]]; if (s->off || s->mx.y <= y + 0.05f || s->mn.y >= y + 1.8f) continue;
        float a0 = x0, b0 = z0, a1 = x1, b1 = z1;
        if (s->f != curXf) {   // the footprint's corners in the solid's frame (a slightly larger box: conservative)
            a0 = b0 = 1e9f; a1 = b1 = -1e9f;
            for (int q = 0; q < 4; q++) { Vector3 L = solL(s, xfP(curXf, V3(q & 1 ? x1 : x0, 0, q & 2 ? z1 : z0))); a0 = fminf(a0, L.x); a1 = fmaxf(a1, L.x); b0 = fminf(b0, L.z); b1 = fmaxf(b1, L.z); } }
        if (a0 < s->mx.x && a1 > s->mn.x && b0 < s->mx.z && b1 > s->mn.z) return false; }
    return true;
}
// place a searchable piece of furniture against an inside wall of `in` at floor y
#define MAX_KEEP 24
static Rectangle keepClear[MAX_KEEP], keepTight[MAX_KEEP]; static Vector2 keepDoor[MAX_KEEP]; static int nKeep;   // doorways on the floor being furnished: the room to walk through them (and a tighter zone for small rooms)
static bool fTight;   // set while a toilet, washroom or bath is fitted out: fixtures may stand closer to the door there
static void keepAdd(Rectangle wide, Rectangle tight) { if (nKeep >= MAX_KEEP) return; keepClear[nKeep] = wide; keepTight[nKeep] = tight; keepDoor[nKeep] = (Vector2){wide.x + wide.width / 2, wide.y + wide.height / 2}; nKeep++; }
static bool blocksDoor(float x0, float z0, float x1, float z1, float y) {
    for (int k = 0; k < nKeep; k++) { Rectangle c = fTight ? keepTight[k] : keepClear[k]; if (x0 < c.x + c.width && x1 > c.x && z0 < c.y + c.height && z1 > c.y) return true; }
    for (int i = 0; i < nOps; i++) { Open *o = &ops[i]; if (o->type != OP_DOOR || opInner[i] || o->f != curXf || fabsf(o->mn.y - y) > 0.6f) continue;   // keep 1.2 m clear around every door (an inner one has its doorway's own keep-clear)
        float dm = fTight ? 0.7f : 1.2f; if (x0 < o->mx.x + dm && x1 > o->mn.x - dm && z0 < o->mx.z + dm && z1 > o->mn.z - dm) return true; }
    return false;
}
// a body on the floor of `in`: its own random stream, so the rest of the town comes out as before
static void placeBody(Rectangle in, float y, int var, int region, unsigned int seed) {
    unsigned int h = seed * 2654435761u + 0x9e3779b9u; if (!h) h = 1;
    #define HR() (h ^= h << 13, h ^= h >> 17, h ^= h << 5, (float)(h & 0xffffff) / 16777216.0f)
    for (int tries = 0; tries < 10; tries++) {
        bool rot = HR() < 0.5f; float hx = rot ? 0.9f : 0.32f, hz = rot ? 0.32f : 0.9f;
        if (in.width < 2 * hx + 0.4f || in.height < 2 * hz + 0.4f) continue;
        float x = in.x + hx + 0.2f + HR() * (in.width - 2 * hx - 0.4f), z = in.y + hz + 0.2f + HR() * (in.height - 2 * hz - 0.4f);
        if (!boxFree(x - hx, z - hz, x + hx, z + hz, y) || blocksDoor(x - hx, z - hz, x + hx, z + hz, y)) continue;
        int c = addCont(CK_BODY, V3(x, y + 0.02f, z), (rot ? PI / 2 : 0) + (HR() < 0.5f ? PI : 0), region, false); if (c >= 0) conts[c].var = (short)var;
        break; }
    #undef HR
}
static void furnish(Rectangle in, float y, int kind, int region) {
    Vector3 sz = ckSize[kind];
    for (int tries = 0; tries < 14; tries++) {
        int side = ri(4); float x, z, yaw;
        if (side == 0) { x = fr(in.x + sz.x / 2 + 0.2f, in.x + in.width - sz.x / 2 - 0.2f); z = in.y + sz.z / 2 + 0.02f; yaw = 0; }
        else if (side == 2) { x = fr(in.x + sz.x / 2 + 0.2f, in.x + in.width - sz.x / 2 - 0.2f); z = in.y + in.height - sz.z / 2 - 0.02f; yaw = PI; }
        else if (side == 1) { z = fr(in.y + sz.x / 2 + 0.2f, in.y + in.height - sz.x / 2 - 0.2f); x = in.x + in.width - sz.z / 2 - 0.02f; yaw = -PI / 2; }
        else { z = fr(in.y + sz.x / 2 + 0.2f, in.y + in.height - sz.x / 2 - 0.2f); x = in.x + sz.z / 2 + 0.02f; yaw = PI / 2; }
        if (in.width < sz.x + 0.6f || in.height < sz.x + 0.6f) return;
        bool rot = fabsf(sinf(yaw)) > 0.5f; float hx = (rot ? sz.z : sz.x) / 2 + 0.05f, hz = (rot ? sz.x : sz.z) / 2 + 0.05f;
        // keep a 0.9 m walkway in front of it
        float fx = x + sinf(yaw) * (sz.z / 2 + 0.5f), fz = z + cosf(yaw) * (sz.z / 2 + 0.5f);
        if (!boxFree(x - hx + 0.06f, z - hz + 0.06f, x + hx - 0.06f, z + hz - 0.06f, y) || !boxFree(fx - 0.35f, fz - 0.35f, fx + 0.35f, fz + 0.35f, y)) continue;
        if (fx < in.x || fx > in.x + in.width || fz < in.y || fz > in.y + in.height) continue;
        if (blocksDoor(x - hx, z - hz, x + hx, z + hz, y)) continue;
        bool nearStair = false;
        for (int s = nStair - 1; s >= 0 && s >= nStair - 6; s--) { Vector3 a = stairs[s].bot, b = stairs[s].top;
            if ((fabsf(a.y - y) < 0.5f && Vector3Distance(V3(a.x, 0, a.z), V3(x, 0, z)) < 1.9f) || (fabsf(b.y - y) < 0.5f && Vector3Distance(V3(b.x, 0, b.z), V3(x, 0, z)) < 1.9f)) nearStair = true; }
        if (nearStair) continue;
        addCont(kind, V3(x, y, z), yaw, region, true);
        return;
    }
}
// ---------------------------------------------------------------- designed interiors (fixed town: every room is laid out on purpose)
// walls: 0 min-z, 1 max-x, 2 max-z, 3 min-x. A piece "against wall w" faces into the room.
static Rectangle fAvoid;   // stairwell band of the floor being furnished
static bool fFree(float x0, float z0, float x1, float z1, float y) {
    if (x0 < fAvoid.x + fAvoid.width && x1 > fAvoid.x && z0 < fAvoid.y + fAvoid.height && z1 > fAvoid.y) return false;
    return boxFree(x0 + 0.03f, z0 + 0.03f, x1 - 0.03f, z1 - 0.03f, y) && !blocksDoor(x0, z0, x1, z1, y);
}
static float wallYaw(int wall) { return wall == 0 ? 0 : wall == 2 ? PI : wall == 1 ? -PI / 2 : PI / 2; }
static void wallFoot(Rectangle rm, int wall, float u, float w, float d, float *x0, float *z0, float *x1, float *z1) {   // footprint of a w x d piece centred at u along the wall
    if (wall == 0) { *x0 = u - w / 2; *x1 = u + w / 2; *z0 = rm.y + 0.02f; *z1 = *z0 + d; }
    else if (wall == 2) { *x0 = u - w / 2; *x1 = u + w / 2; *z1 = rm.y + rm.height - 0.02f; *z0 = *z1 - d; }
    else if (wall == 1) { *z0 = u - w / 2; *z1 = u + w / 2; *x1 = rm.x + rm.width - 0.02f; *x0 = *x1 - d; }
    else { *z0 = u - w / 2; *z1 = u + w / 2; *x0 = rm.x + 0.02f; *x1 = *x0 + d; }
}
static float wallLo(Rectangle rm, int wall) { return wall == 0 || wall == 2 ? rm.x : rm.y; }
static float wallHi(Rectangle rm, int wall) { return wall == 0 || wall == 2 ? rm.x + rm.width : rm.y + rm.height; }
static bool frontFree(int wall, float x0, float z0, float x1, float z1, float y, float clr) {   // walking room in front of it
    if (wall == 0) return fFree(x0 + 0.2f, z1, x1 - 0.2f, z1 + clr, y);
    if (wall == 2) return fFree(x0 + 0.2f, z0 - clr, x1 - 0.2f, z0, y);
    if (wall == 1) return fFree(x0 - clr, z0 + 0.2f, x0, z1 - 0.2f, y);
    return fFree(x1, z0 + 0.2f, x1 + clr, z1 - 0.2f, y);
}
// a run of pieces along a wall, packed from `from` (0 start, 1 end); kinds >= 0 are containers, < 0 are fixtures from fixTab
typedef struct { float w, d, h; int tex; Color c; } Fix;
enum { FX_SINK = -1, FX_STOVE = -2, FX_BOOKS = -3, FX_TANSU = -4, FX_REG = -5, FX_ATM = -6, FX_COPIER = -7, FX_BOARD = -8, FX_COOLER = -9, FX_RACK = -10, FX_BENCH = -11, FX_MAGS = -12, FX_PLANT = -13, FX_CRATES = -14, FX_SCREEN = -15 };
static const Fix fixTab[] = {{0, 0, 0, 0, {0}},
    {0.9f, 0.6f, 0.88f, T_METAL, {196, 200, 204, 255}}, {0.7f, 0.6f, 0.88f, T_METAL, {90, 92, 96, 255}}, {0.9f, 0.32f, 1.8f, T_WOOD, {120, 90, 66, 255}},
    {1.0f, 0.45f, 1.1f, T_WOOD, {104, 70, 46, 255}}, {0.5f, 0.4f, 1.05f, T_PLAIN, {230, 230, 225, 255}}, {0.7f, 0.6f, 1.6f, T_METAL, {60, 120, 170, 255}},
    {1.0f, 0.7f, 1.1f, T_PLAIN, {210, 212, 214, 255}}, {1.6f, 0.06f, 1.9f, T_PAPER, {236, 236, 228, 255}}, {0.35f, 0.35f, 1.15f, T_PLAIN, {220, 230, 240, 255}},
    {1.2f, 0.4f, 1.3f, T_METAL, {150, 150, 150, 255}}, {1.6f, 0.45f, 0.45f, T_WOOD, {130, 100, 76, 255}}, {1.4f, 0.35f, 1.0f, T_WOOD, {160, 140, 110, 255}},
    {0.45f, 0.45f, 1.3f, T_HEDGE, {70, 120, 70, 255}}, {1.0f, 0.8f, 1.2f, T_PLAIN, {170, 140, 100, 255}}, {0.9f, 0.06f, 0.6f, T_PLAIN, {30, 30, 34, 255}}};
static void fixDetail(int f, float x0, float z0, float x1, float z1, float y, int wall) {   // small readable details on top of a fixture
    float cx = (x0 + x1) / 2, cz = (z0 + z1) / 2;
    if (f == FX_STOVE) for (int k = 0; k < 2; k++) box(T_PLAIN, V3(cx - 0.25f + k * 0.3f, y + 0.88f, cz - 0.1f), V3(cx - 0.05f + k * 0.3f, y + 0.9f, cz + 0.1f), rgb(30, 30, 30), false, ALLB);
    if (f == FX_SINK) box(T_METAL, V3(cx - 0.3f, y + 0.82f, cz - 0.18f), V3(cx + 0.3f, y + 0.885f, cz + 0.18f), rgb(150, 156, 160), false, ALLB);
    if (f == FX_BOOKS || f == FX_MAGS) { int rows = f == FX_BOOKS ? 4 : 2; for (int r = 0; r < rows; r++) for (int b = 0; b < 6; b++) { float t = (b + 0.5f) / 6, bx = x0 + (x1 - x0) * t, bz = z0 + (z1 - z0) * t;
        bool alongX = wall == 0 || wall == 2; float hw = alongX ? (x1 - x0) / 14 : 0.1f, hd = alongX ? 0.1f : (z1 - z0) / 14; Color bc = (Color){(unsigned char)(80 + (b * 53 + r * 31) % 150), (unsigned char)(60 + (b * 97 + r * 17) % 140), (unsigned char)(60 + (b * 41 + r * 71) % 150), 255};
        float yy = y + 0.15f + r * (f == FX_BOOKS ? 0.42f : 0.4f); box(T_PLAIN, V3((alongX ? bx : cx) - hw, yy, (alongX ? cz : bz) - hd), V3((alongX ? bx : cx) + hw, yy + 0.3f, (alongX ? cz : bz) + hd), bc, false, ALLB); } }
    if (f == FX_REG) box(T_PLAIN, V3(cx - 0.2f, y + 1.05f, cz - 0.15f), V3(cx + 0.2f, y + 1.3f, cz + 0.15f), rgb(40, 40, 44), false, ALLB);
    if (f == FX_ATM) box(T_PLAIN, V3(cx - 0.22f, y + 1.0f, cz - 0.22f), V3(cx + 0.22f, y + 1.3f, cz + 0.22f), rgb(120, 200, 240), false, ALLB);
    if (f == FX_PLANT) box(T_PLAIN, V3(x0 + 0.05f, y, z0 + 0.05f), V3(x1 - 0.05f, y + 0.4f, z1 - 0.05f), rgb(150, 90, 60), false, ALLB);
}
static bool standFree(float x, float z, float y) {   // room for a person to stand here: the player's own size (0.3 m radius), things up to a step high do not count
    const float R = 0.3f; Vector3 wmn, wmx; xfBox(curXf, V3(x - R, 0, z - R), V3(x + R, 0, z + R), &wmn, &wmx); query(wmn.x, wmn.z, wmx.x, wmx.z);
    for (int k = 0; k < qn; k++) { Solid *s = &sol[qb[k]]; if (s->off || s->mx.y <= y + 0.42f || s->mn.y >= y + 1.7f) continue;
        float lx = x, lz = z; if (s->f != curXf) { Vector3 L = solL(s, xfP(curXf, V3(x, 0, z))); lx = L.x; lz = L.z; }   // the person as a 0.6 m square turned with the solid (the old corner box grew to 0.85 m on a turned building)
        if (lx + R > s->mn.x && lx - R < s->mx.x && lz + R > s->mn.z && lz - R < s->mx.z) return false; }
    return true;
}
// In a small room a piece is only set down if one can still walk from every doorway to every other and stand inside (a washing machine never walls the bath off).
#define PC_N 140
static Rectangle pcRoom; static bool pcOn = false; static int pcSol = -1, pcW, pcH; static unsigned char pcG[PC_N * PC_N]; static float pcY;
static bool pathOk(float x0, float z0, float x1, float z1) {
    if (!pcOn) return true; const float q = 0.1f, M = 0.6f; float gx0 = pcRoom.x - M, gz0 = pcRoom.y - M;
    if (pcSol < 0) { pcSol = nSol; pcW = (int)((pcRoom.width + 2 * M) / q) + 1; pcH = (int)((pcRoom.height + 2 * M) / q) + 1; if (pcW > PC_N || pcH > PC_N) { pcOn = false; return true; }
        for (int j = 0; j < pcH; j++) for (int i = 0; i < pcW; i++) pcG[j * pcW + i] = standFree(gx0 + (i + 0.5f) * q, gz0 + (j + 0.5f) * q, pcY) ? 1 : 0; }
    else if (pcSol != nSol) {   // pieces set down since: shut the cells they take (with a person's half-width round them), no full rebuild
        for (int k = pcSol; k < nSol; k++) { Solid *s = &sol[k]; if (s->off || s->f != curXf || s->mx.y <= pcY + 0.42f || s->mn.y >= pcY + 1.7f) continue;
            int i0 = (int)floorf((s->mn.x - 0.3f - gx0) / q), i1 = (int)ceilf((s->mx.x + 0.3f - gx0) / q), j0 = (int)floorf((s->mn.z - 0.3f - gz0) / q), j1 = (int)ceilf((s->mx.z + 0.3f - gz0) / q);
            for (int j = j0 < 0 ? 0 : j0; j < j1 && j < pcH; j++) for (int i = i0 < 0 ? 0 : i0; i < i1 && i < pcW; i++) { float x = gx0 + (i + 0.5f) * q, z = gz0 + (j + 0.5f) * q; if (x + 0.3f > s->mn.x && x - 0.3f < s->mx.x && z + 0.3f > s->mn.z && z - 0.3f < s->mx.z) pcG[j * pcW + i] = 0; } }
        pcSol = nSol; }
    Vector2 dp[8]; int nd = 0;
    for (int k = 0; k < nKeep + nOps && nd < 8; k++) { Vector2 c;
        if (k < nKeep) c = keepDoor[k]; else { Open *o = &ops[k - nKeep]; if (o->type != OP_DOOR || opInner[k - nKeep] || o->f != curXf || fabsf(o->mn.y - pcY) > 0.6f) continue; c = (Vector2){(o->mn.x + o->mx.x) / 2, (o->mn.z + o->mx.z) / 2}; }
        if (c.x < pcRoom.x - 0.45f || c.x > pcRoom.x + pcRoom.width + 0.45f || c.y < pcRoom.y - 0.45f || c.y > pcRoom.y + pcRoom.height + 0.45f) continue;
        float d[4] = {fabsf(c.y - pcRoom.y), fabsf(c.x - (pcRoom.x + pcRoom.width)), fabsf(c.y - (pcRoom.y + pcRoom.height)), fabsf(c.x - pcRoom.x)}; int wl = 0; for (int e = 1; e < 4; e++) if (d[e] < d[wl]) wl = e; if (d[wl] > 0.6f) continue;
        float nx = wl == 1 ? -1 : wl == 3 ? 1 : 0, nz = wl == 0 ? 1 : wl == 2 ? -1 : 0; float ex = wl == 1 ? pcRoom.x + pcRoom.width : wl == 3 ? pcRoom.x : c.x, ez = wl == 0 ? pcRoom.y : wl == 2 ? pcRoom.y + pcRoom.height : c.y;
        dp[nd++] = (Vector2){ex + nx * 0.4f, ez + nz * 0.4f}; }
    if (nd == 0) return true;
    static unsigned char g[PC_N * PC_N]; static int st[PC_N * PC_N]; int n = 0, got = 0;
    for (int j = 0; j < pcH; j++) for (int i = 0; i < pcW; i++) { float x = gx0 + (i + 0.5f) * q, z = gz0 + (j + 0.5f) * q; g[j * pcW + i] = pcG[j * pcW + i] && !(x > x0 - 0.3f && x < x1 + 0.3f && z > z0 - 0.3f && z < z1 + 0.3f); }
    #define PC_CELL(p_) ((int)((p_.y - gz0) / q) * pcW + (int)((p_.x - gx0) / q))
    int c0 = PC_CELL(dp[0]); if (c0 < 0 || c0 >= pcW * pcH) return true; if (!pcG[c0]) return true;   // (the doorway was shut before this piece: not this piece's doing)
    if (!g[c0]) return false; g[c0] = 2; st[n++] = c0;
    while (n > 0) { int c = st[--n], i = c % pcW, j = c / pcW; float x = gx0 + (i + 0.5f) * q, z = gz0 + (j + 0.5f) * q; if (x > pcRoom.x && x < pcRoom.x + pcRoom.width && z > pcRoom.y && z < pcRoom.y + pcRoom.height) got++;
        if (i + 1 < pcW && g[c + 1] == 1) { g[c + 1] = 2; st[n++] = c + 1; } if (i > 0 && g[c - 1] == 1) { g[c - 1] = 2; st[n++] = c - 1; }
        if (j + 1 < pcH && g[c + pcW] == 1) { g[c + pcW] = 2; st[n++] = c + pcW; } if (j > 0 && g[c - pcW] == 1) { g[c - pcW] = 2; st[n++] = c - pcW; } }
    if (got < 12) return false;
    for (int k = 1; k < nd; k++) { int c = PC_CELL(dp[k]); if (c >= 0 && c < pcW * pcH && pcG[c] && g[c] != 2) return false; }
    #undef PC_CELL
    return true;
}
static bool ppDry;   // (trying a wall out: nothing is set down)
static bool atWindow(float x0, float z0, float x1, float z1, float y) {
    for (int i = 0; i < nOps; i++) { Open *o = &ops[i]; if (o->type != OP_WINDOW || o->f != curXf || fabsf(o->fy - y) > 0.6f) continue;
        if (x0 - 0.3f < o->mx.x && x1 + 0.3f > o->mn.x && z0 - 0.3f < o->mx.z && z1 + 0.3f > o->mn.z) return true; }
    return false;
}
static bool putPiece(int k, Rectangle rm, int wall, float u, float y, int region) {
    float w, d, h = 0; if (k >= 0) { w = ckSize[k].x; d = ckSize[k].z; } else { w = fixTab[-k].w; d = fixTab[-k].d; h = fixTab[-k].h; }
    float x0, z0, x1, z1; wallFoot(rm, wall, u, w, d, &x0, &z0, &x1, &z1);
    if (x0 < rm.x || x1 > rm.x + rm.width || z0 < rm.y || z1 > rm.y + rm.height) return false;
    if (!fFree(x0, z0, x1, z1, y) || !frontFree(wall, x0, z0, x1, z1, y, k == FX_BOARD || k == FX_SCREEN ? 0.3f : fTight ? 0.45f : 0.75f)) return false;
    if ((k >= 0 ? ckSize[k].y : h) > 1.2f && k != FX_BOARD && k != FX_SCREEN && atWindow(x0, z0, x1, z1, y)) return false;   // nothing tall stands in front of a window
    if (!pathOk(x0, z0, x1, z1)) return false;
    if (ppDry) return true;
    if (k >= 0) addCont(k, V3((x0 + x1) / 2, y, (z0 + z1) / 2), wallYaw(wall), region, true);
    else { float yb = (k == FX_BOARD || k == FX_SCREEN) ? y + 0.9f : y; box(fixTab[-k].tex, V3(x0, yb, z0), V3(x1, yb + h, z1), fixTab[-k].c, true, ALL); fixDetail(k, x0, z0, x1, z1, y, wall); }
    return true;
}
static int runAlong(Rectangle rm, int wall, float y, const int *ks, int n, int fromEnd, int region) {   // pack pieces side by side; returns how many fitted
    float lo = wallLo(rm, wall) + 0.25f, hi = wallHi(rm, wall) - 0.25f, cur = fromEnd ? hi : lo; int got = 0;
    for (int i = 0; i < n; i++) { float w = ks[i] >= 0 ? ckSize[ks[i]].x : fixTab[-ks[i]].w; bool ok = false;
        for (float t = 0; t < hi - lo; t += 0.15f) { float u = fromEnd ? cur - w / 2 - t : cur + w / 2 + t; if (u - w / 2 < lo - 0.01f || u + w / 2 > hi + 0.01f) break;
            if (putPiece(ks[i], rm, wall, u, y, region)) { cur = fromEnd ? u - w / 2 - 0.02f : u + w / 2 + 0.02f; ok = true; got++; break; } }
        if (!ok) continue; }
    return got;
}
static bool freeBox(float cx, float cz, float w, float d, float h, float y, int tex, Color c, bool solid) {
    if (!fFree(cx - w / 2, cz - d / 2, cx + w / 2, cz + d / 2, y) || (solid && h > 0.42f && !pathOk(cx - w / 2, cz - d / 2, cx + w / 2, cz + d / 2))) return false;
    box(tex, V3(cx - w / 2, y, cz - d / 2), V3(cx + w / 2, y + h, cz + d / 2), c, solid, ALL); return true;
}
static void chair(float cx, float cz, float y, int face, Color c) {   // seat + backrest; face: direction the sitter looks (0 +z,1 -x,2 -z,3 +x)
    if (!fFree(cx - 0.24f, cz - 0.24f, cx + 0.24f, cz + 0.24f, y)) return;
    box(T_WOOD, V3(cx - 0.22f, y, cz - 0.22f), V3(cx + 0.22f, y + 0.45f, cz + 0.22f), c, true, ALL);
    float bx0 = cx - 0.22f, bx1 = cx + 0.22f, bz0 = cz - 0.22f, bz1 = cz + 0.22f;
    if (face == 0) bz1 = bz0 + 0.06f; else if (face == 2) bz0 = bz1 - 0.06f; else if (face == 1) bx0 = bx1 - 0.06f; else bx1 = bx0 + 0.06f;
    box(T_WOOD, V3(bx0, y + 0.45f, bz0), V3(bx1, y + 0.95f, bz1), c, false, ALLB);
}
static bool tableSet(float cx, float cz, float y, bool alongX, int seats, float h, Color tc) {   // table with chairs (h 0.72) or a low chabudai with zabuton (h 0.35)
    float w = alongX ? 1.4f : 0.8f, d = alongX ? 0.8f : 1.4f; if (h < 0.5f) { w = alongX ? 1.1f : 0.8f; d = alongX ? 0.8f : 1.1f; }
    if (!fFree(cx - w / 2 - 0.5f, cz - d / 2 - 0.5f, cx + w / 2 + 0.5f, cz + d / 2 + 0.5f, y) || (h >= 0.42f && !pathOk(cx - w / 2 - 0.55f, cz - d / 2 - 0.55f, cx + w / 2 + 0.55f, cz + d / 2 + 0.55f))) return false;   // (with its chairs: still a way round it)
    box(T_WOOD, V3(cx - w / 2, y + h - 0.05f, cz - d / 2), V3(cx + w / 2, y + h, cz + d / 2), tc, true, ALL);
    box(T_WOOD, V3(cx - 0.06f, y, cz - 0.06f), V3(cx + 0.06f, y + h - 0.05f, cz + 0.06f), aMul(tc, 0.7f), false, ALLB);
    for (int s2 = 0; s2 < seats; s2++) { float ox = alongX ? (s2 % 2 ? 0.35f : -0.35f) : (s2 < 2 ? -w / 2 - 0.35f : w / 2 + 0.35f), oz = alongX ? (s2 < 2 ? -d / 2 - 0.35f : d / 2 + 0.35f) : (s2 % 2 ? 0.35f : -0.35f);
        int face = alongX ? (s2 < 2 ? 0 : 2) : (s2 < 2 ? 3 : 1);
        if (h < 0.5f) box(T_CLOTH, V3(cx + ox - 0.27f, y, cz + oz - 0.27f), V3(cx + ox + 0.27f, y + 0.09f, cz + oz + 0.27f), rgb(150, 60, 50), false, ALLB);   // zabuton
        else chair(cx + ox, cz + oz, y, face, rgb(140, 105, 80)); }
    return true;
}
static void wBedAdd(float x0, float z0, float x1, float z1, float y0, float y1) {   // a bed the town was built with, kept in world space
    if (nWBed >= MAX_WBED) return; Vector3 a = xfP(curXf, V3(x0, y0, z0)), b = xfP(curXf, V3(x1, y1, z1));
    wBedMn[nWBed] = Vector3Min(a, b); wBedMx[nWBed] = Vector3Max(a, b); nWBed++;
}
static void futon(Rectangle rm, int wall, float u, float y, Color cover) {
    float x0, z0, x1, z1; bool alongX = wall == 1 || wall == 3; wallFoot(rm, wall, u, 1.0f, 2.0f, &x0, &z0, &x1, &z1);
    if (alongX) { float t; t = x0; (void)t; }
    if (!fFree(x0, z0, x1, z1, y)) return;
    wBedAdd(x0, z0, x1, z1, y, y + 0.2f);
    box(T_CLOTH, V3(x0, y, z0), V3(x1, y + 0.12f, z1), rgb(236, 232, 222), false, ALLB);
    float k = 0.35f; box(T_CLOTH, V3(x0 + 0.02f, y + 0.12f, z0 + (wall == 0 ? 0.55f : 0.02f)), V3(x1 - 0.02f, y + 0.2f, z1 - (wall == 2 ? 0.55f : 0.02f)), cover, false, ALLB); (void)k;
}
// ---- Japanese room pieces (0.9.0)
static bool wallBox(Rectangle rm, int wall, float u, float w, float d, float y0, float h, int tex, Color c, bool solid) {   // a w x d piece against a wall, if it fits
    float x0, z0, x1, z1; wallFoot(rm, wall, u, w, d, &x0, &z0, &x1, &z1);
    if (x0 < rm.x || x1 > rm.x + rm.width || z0 < rm.y || z1 > rm.y + rm.height || !fFree(x0, z0, x1, z1, y0)) return false;
    if (solid && y0 < pcY + 0.3f && !pathOk(x0, z0, x1, z1)) return false;
    box(tex, V3(x0, y0, z0), V3(x1, y0 + h, z1), c, solid, ALL); return true;
}
static float wallAt(Rectangle rm, int wall, float frac) { return wallLo(rm, wall) + (wallHi(rm, wall) - wallLo(rm, wall)) * frac; }
static Rectangle floorHoleNow;   // the stairwell opening in the floor being furnished (from the flight below), in the building's frame
static int ffN = 0;
static void floorFinish(Rectangle rm, float y, int tex, Color c) {   // a room's floor covering, cut round the stairwell
    Rectangle h = floorHoleNow; y += 0.0016f * (ffN++ % 6);   // (rooms that meet or overlap under a partition never share a height)
    float hx0 = fmaxf(h.x, rm.x), hx1 = fminf(h.x + h.width, rm.x + rm.width), hz0 = fmaxf(h.y, rm.y), hz1 = fminf(h.y + h.height, rm.y + rm.height);
    if (h.width <= 0 || hx0 >= hx1 || hz0 >= hz1) { box(tex, V3(rm.x, y + 0.016f, rm.y), V3(rm.x + rm.width, y + 0.024f, rm.y + rm.height), c, false, 16); return; }
    if (hx0 > rm.x + 0.01f) box(tex, V3(rm.x, y + 0.016f, rm.y), V3(hx0, y + 0.024f, rm.y + rm.height), c, false, 16);
    if (hx1 < rm.x + rm.width - 0.01f) box(tex, V3(hx1, y + 0.016f, rm.y), V3(rm.x + rm.width, y + 0.024f, rm.y + rm.height), c, false, 16);
    if (hz0 > rm.y + 0.01f) box(tex, V3(hx0, y + 0.016f, rm.y), V3(hx1, y + 0.024f, hz0), c, false, 16);
    if (hz1 < rm.y + rm.height - 0.01f) box(tex, V3(hx0, y + 0.016f, hz1), V3(hx1, y + 0.024f, rm.y + rm.height), c, false, 16);
}
static void sofa(Rectangle rm, int wall, float u, float y, Color c) {   // a two-seat sofa against a wall, facing into the room
    float x0, z0, x1, z1; wallFoot(rm, wall, u, 1.7f, 0.8f, &x0, &z0, &x1, &z1);
    if (x0 < rm.x || x1 > rm.x + rm.width || z0 < rm.y || z1 > rm.y + rm.height || !fFree(x0, z0, x1, z1, y)) return;
    box(T_CLOTH, V3(x0, y, z0), V3(x1, y + 0.42f, z1), c, true, ALL);
    float bx0 = x0, bx1 = x1, bz0 = z0, bz1 = z1; if (wall == 0) bz1 = bz0 + 0.2f; else if (wall == 2) bz0 = bz1 - 0.2f; else if (wall == 1) bx0 = bx1 - 0.2f; else bx1 = bx0 + 0.2f;
    box(T_CLOTH, V3(bx0, y + 0.42f, bz0), V3(bx1, y + 0.85f, bz1), aMul(c, 0.85f), false, ALLB);
}
static void tokonomaU(Rectangle rm, int wall, float u, float y) {   // the alcove: a raised board, a hanging scroll, a vase
    float x0, z0, x1, z1; wallFoot(rm, wall, u, 1.6f, 0.75f, &x0, &z0, &x1, &z1);
    if (x0 < rm.x || x1 > rm.x + rm.width || z0 < rm.y || z1 > rm.y + rm.height || !fFree(x0, z0, x1, z1, y)) return;
    box(T_WOOD, V3(x0, y, z0), V3(x1, y + 0.14f, z1), rgb(120, 85, 55), true, ALL);
    float sx0, sz0, sx1, sz1; wallFoot(rm, wall, u, 0.5f, 0.02f, &sx0, &sz0, &sx1, &sz1); box(T_PLAIN, V3(sx0, y + 0.6f, sz0), V3(sx1, y + 1.9f, sz1), rgb(226, 218, 196), false, ALL);
    { float ix0, iz0, ix1, iz1; wallFoot(rm, wall, u, 0.06f, 0.03f, &ix0, &iz0, &ix1, &iz1); box(T_PLAIN, V3(ix0, y + 0.9f, iz0), V3(ix1, y + 1.6f, iz1), rgb(30, 28, 26), false, ALL); }   // one brush stroke
    float vx = (x0 + x1) / 2 + (wall == 0 || wall == 2 ? 0.45f : 0), vz = (z0 + z1) / 2 + (wall == 1 || wall == 3 ? 0.45f : 0); box(T_PLAIN, V3(vx - 0.08f, y + 0.14f, vz - 0.08f), V3(vx + 0.08f, y + 0.45f, vz + 0.08f), rgb(70, 90, 110), false, ALL);
}
static void tokonoma(Rectangle rm, int wall, float y) { tokonomaU(rm, wall, wallAt(rm, wall, 0.75f), y); }
static void butsudanU(Rectangle rm, int wall, float u, float y) {   // the family altar: black lacquer, gold inside, a photograph
    float x0, z0, x1, z1; wallFoot(rm, wall, u, 0.7f, 0.5f, &x0, &z0, &x1, &z1);
    if (x0 < rm.x || x1 > rm.x + rm.width || z0 < rm.y || z1 > rm.y + rm.height || !fFree(x0, z0, x1, z1, y)) return;
    box(T_WOOD, V3(x0, y, z0), V3(x1, y + 1.3f, z1), rgb(30, 22, 20), true, ALL);
    float fx0 = x0, fx1 = x1, fz0 = z0, fz1 = z1; float e = 0.01f; if (wall == 0) fz0 = fz1 - e; else if (wall == 2) fz1 = fz0 + e; else if (wall == 1) fx1 = fx0 + e; else fx0 = fx1 - e;
    box(T_PLAIN, V3(fx0 + (wall == 0 || wall == 2 ? 0.1f : -0.005f), y + 0.55f, fz0 + (wall == 1 || wall == 3 ? 0.1f : -0.005f)), V3(fx1 - (wall == 0 || wall == 2 ? 0.1f : -0.005f), y + 1.15f, fz1 - (wall == 1 || wall == 3 ? 0.1f : -0.005f)), rgb(200, 160, 60), false, ALL);
}
static void butsudan(Rectangle rm, int wall, float y) { butsudanU(rm, wall, wallAt(rm, wall, 0.25f), y); }
static bool wetStrip(Rectangle rm, int wall, float y, int region) {   // bathroom, toilet and washroom behind a partition along one wall
    bool alongX = wall == 0 || wall == 2; float D = 1.7f, len = alongX ? rm.width : rm.height; if (len < 3.4f || (alongX ? rm.height : rm.width) < D + 2.0f) return false;
    Rectangle st = alongX ? (Rectangle){rm.x, wall == 0 ? rm.y : rm.y + rm.height - D, rm.width, D} : (Rectangle){wall == 3 ? rm.x : rm.x + rm.width - D, rm.y, D, rm.height};
    if (!fFree(st.x + 0.05f, st.y + 0.05f, st.x + st.width - 0.05f, st.y + st.height - 0.05f, y)) return false;   // clear of the stairs and the doorway
    float yT = y + FH - SLAB, wc = alongX ? (wall == 0 ? st.y + D : st.y) : (wall == 3 ? st.x + D : st.x), lo = alongX ? st.x : st.y, door0 = lo + len - 1.3f, door1 = door0 + 0.8f;
    Color pc = rgb(226, 224, 218);
    if (alongX) { box(T_PLASTER, V3(lo, y, wc - 0.05f), V3(door0, yT, wc + 0.05f), pc, true, ALL); box(T_PLASTER, V3(door1, y, wc - 0.05f), V3(lo + len, yT, wc + 0.05f), pc, true, ALL); box(T_PLASTER, V3(door0, y + 2.05f, wc - 0.05f), V3(door1, yT, wc + 0.05f), pc, false, ALLB); }
    else { box(T_PLASTER, V3(wc - 0.05f, y, lo), V3(wc + 0.05f, yT, door0), pc, true, ALL); box(T_PLASTER, V3(wc - 0.05f, y, door1), V3(wc + 0.05f, yT, lo + len), pc, true, ALL); box(T_PLASTER, V3(wc - 0.05f, y + 2.05f, door0), V3(wc + 0.05f, yT, door1), pc, false, ALLB); }
    floorFinish(st, y, T_TILEWALL, rgb(210, 214, 216));
    float b = lo + 0.3f;   // inside the strip, along the outer wall: the unit bath, a partition, the toilet, the basin and washing machine
    float x0, z0, x1, z1; wallFoot(st, wall, b + 0.85f, 1.6f, D - 0.1f, &x0, &z0, &x1, &z1); box(T_PLAIN, V3(x0, y, z0), V3(x1, y + 0.55f, z1), rgb(236, 238, 238), true, ALL);
    box(T_PLAIN, V3(x0 + 0.12f, y + 0.551f, z0 + 0.12f), V3(x1 - 0.12f, y + 0.56f, z1 - 0.12f), rgb(120, 150, 160), false, 16);   // cold water still standing in the tub
    float pw = b + 1.75f; if (alongX) box(T_PLASTER, V3(pw, y, st.y), V3(pw + 0.08f, yT, st.y + st.height), pc, true, ALL); else box(T_PLASTER, V3(st.x, y, pw), V3(st.x + st.width, yT, pw + 0.08f), pc, true, ALL);
    wallBox(st, wall, b + 2.2f, 0.42f, 0.62f, y, 0.42f, T_PLAIN, rgb(240, 240, 238), true); wallBox(st, wall, b + 2.2f, 0.46f, 0.2f, y + 0.42f, 0.42f, T_PLAIN, rgb(232, 232, 228), false);   // toilet bowl and tank
    if (b + 3.1f < door0 - 0.1f) { wallBox(st, wall, b + 2.95f, 0.62f, 0.62f, y, 0.9f, T_PLAIN, rgb(238, 238, 236), true); }   // washing machine
    addCont(CK_CABINET, alongX ? V3(door0 - 0.6f, y, wall == 0 ? st.y + 0.3f : st.y + D - 0.3f) : V3(wall == 3 ? st.x + 0.3f : st.x + D - 0.3f, y, door0 - 0.6f), wallYaw(wall), region, false);   // the vanity: medicine, a towel
    return true;
}
static int opp(int w) { return (w + 2) % 4; }


// ---- placing by purpose (0.9.4): a piece goes on the wall that suits it, where there is really room for it
static void wallIn(int wall, float *nx, float *nz) { *nx = wall == 1 ? -1 : wall == 3 ? 1 : 0; *nz = wall == 0 ? 1 : wall == 2 ? -1 : 0; }   // the way into the room from a wall
static int roomDoors(Rectangle rm, float y, int *first, float *uFirst) {   // which walls have a doorway (bit per wall); the first one found and where along it
    int mask = 0; *first = -1; *uFirst = 0;
    for (int k = 0; k < nKeep + nOps; k++) { Vector2 c;
        if (k < nKeep) c = keepDoor[k]; else { Open *o = &ops[k - nKeep]; if (o->type != OP_DOOR || opInner[k - nKeep] || o->f != curXf || fabsf(o->mn.y - y) > 0.6f) continue; c = (Vector2){(o->mn.x + o->mx.x) / 2, (o->mn.z + o->mx.z) / 2}; }
        if (c.x < rm.x - 0.45f || c.x > rm.x + rm.width + 0.45f || c.y < rm.y - 0.45f || c.y > rm.y + rm.height + 0.45f) continue;
        float d[4] = {fabsf(c.y - rm.y), fabsf(c.x - (rm.x + rm.width)), fabsf(c.y - (rm.y + rm.height)), fabsf(c.x - rm.x)}; int wl = 0; for (int q = 1; q < 4; q++) if (d[q] < d[wl]) wl = q;
        if (d[wl] > 0.6f) continue; mask |= 1 << wl; if (*first < 0) { *first = wl; *uFirst = (wl == 0 || wl == 2) ? c.x : c.y; } }
    return mask;
}
static bool spotOn(Rectangle rm, int wall, float w, float d, float h, float y, float frac, float clr, float *uOut) {   // a free place for a w x d piece against a wall, as near frac along it as there is
    float lo = wallLo(rm, wall) + 0.04f + w / 2, hi = wallHi(rm, wall) - 0.04f - w / 2; if (hi < lo) return false; float u0 = lo + (hi - lo) * frac;
    for (float t = 0; t <= hi - lo + 0.01f; t += 0.1f) for (int sg = 1; sg >= -1; sg -= 2) { float u = u0 + sg * t; if (u < lo - 0.001f || u > hi + 0.001f) continue;
        float x0, z0, x1, z1; wallFoot(rm, wall, u, w, d, &x0, &z0, &x1, &z1);
        if (x0 < rm.x - 0.001f || x1 > rm.x + rm.width + 0.001f || z0 < rm.y - 0.001f || z1 > rm.y + rm.height + 0.001f) continue;
        if (!fFree(x0, z0, x1, z1, y) || (clr > 0 && !frontFree(wall, x0, z0, x1, z1, y, clr)) || (h > 1.2f && atWindow(x0, z0, x1, z1, y))) continue;
        if (y < pcY + 0.3f && !pathOk(x0, z0, x1, z1)) continue;
        *uOut = u; return true; }
    return false;
}
static int runBest(Rectangle rm, float y, const int *ks, int n, const int *walls, int nw, int fromEnd, int region) {   // the run goes on the first wall that takes all of it, else on the one that takes most
    int best = -1, bg = 0; ppDry = true;
    for (int i = 0; i < nw; i++) { if (walls[i] < 0) continue; int g = runAlong(rm, walls[i], y, ks, n, fromEnd, region); if (g > bg) { bg = g; best = walls[i]; } if (g == n) break; }
    ppDry = false; if (best >= 0) runAlong(rm, best, y, ks, n, fromEnd, region);
    return best;
}
static void wallOrder(int *out, int a, int b, int c, int d, int doorMask, int skip) {   // walls in order of preference, those with a doorway last; skip: one not to use (-1: none)
    int in[4] = {a, b, c, d}, n = 0; for (int pass = 0; pass < 2; pass++) for (int i = 0; i < 4; i++) { if (in[i] < 0 || in[i] == skip) continue; bool dr = (doorMask >> in[i]) & 1; if (dr == (pass == 1)) out[n++] = in[i]; }
    while (n < 4) out[n++] = -1;
}
static bool sofaAt(float cx, float cz, int face, float y, Color c) {   // a two-seat sofa standing free, the sitters looking the way `face` says (0 +z, 1 -x, 2 -z, 3 +x)
    bool lx = face == 0 || face == 2; float hw = lx ? 0.85f : 0.4f, hd = lx ? 0.4f : 0.85f, x0 = cx - hw, x1 = cx + hw, z0 = cz - hd, z1 = cz + hd;
    if (!fFree(x0, z0, x1, z1, y) || !pathOk(x0, z0, x1, z1)) return false;
    box(T_CLOTH, V3(x0, y, z0), V3(x1, y + 0.42f, z1), c, true, ALL);
    float bx0 = x0, bx1 = x1, bz0 = z0, bz1 = z1; if (face == 0) bz1 = bz0 + 0.2f; else if (face == 2) bz0 = bz1 - 0.2f; else if (face == 1) bx0 = bx1 - 0.2f; else bx1 = bx0 + 0.2f;
    box(T_CLOTH, V3(bx0, y + 0.42f, bz0), V3(bx1, y + 0.85f, bz1), aMul(c, 0.85f), false, ALLB);
    return true;
}
static void tvOn(Rectangle rm, int wall, float u, float y) {   // a low stand against a wall with the television on it, screen to the room
    float x0, z0, x1, z1; wallFoot(rm, wall, u, 1.2f, 0.42f, &x0, &z0, &x1, &z1); box(T_WOOD, V3(x0, y, z0), V3(x1, y + 0.45f, z1), rgb(70, 52, 40), true, ALL);
    float cx = (x0 + x1) / 2, cz = (z0 + z1) / 2; bool ax = wall == 0 || wall == 2; float nx, nz; wallIn(wall, &nx, &nz);
    box(T_PLAIN, V3(cx - (ax ? 0.42f : 0.13f), y + 0.45f, cz - (ax ? 0.13f : 0.42f)), V3(cx + (ax ? 0.42f : 0.13f), y + 0.98f, cz + (ax ? 0.13f : 0.42f)), rgb(44, 44, 48), false, ALLB);   // the set: a deep CRT
    box(T_GLASS, V3(cx + nx * 0.135f - (ax ? 0.36f : 0.005f), y + 0.52f, cz + nz * 0.135f - (ax ? 0.005f : 0.36f)), V3(cx + nx * 0.135f + (ax ? 0.36f : 0.005f), y + 0.93f, cz + nz * 0.135f + (ax ? 0.005f : 0.36f)), rgb(70, 86, 92), false, ALL);   // its dead screen
}
static bool bedOn(Rectangle rm, int wall, float u, float y, bool futonStyle, Color cover) {   // a bed (or a futon) with its head to a wall
    float x0, z0, x1, z1; wallFoot(rm, wall, u, 1.0f, 2.0f, &x0, &z0, &x1, &z1); float nx, nz; wallIn(wall, &nx, &nz);
    wBedAdd(x0, z0, x1, z1, y, y + (futonStyle ? 0.2f : 0.55f));
    if (futonStyle) { box(T_CLOTH, V3(x0, y, z0), V3(x1, y + 0.12f, z1), rgb(236, 232, 222), false, ALLB); }
    else { box(T_WOOD, V3(x0, y, z0), V3(x1, y + 0.3f, z1), rgb(130, 95, 70), true, ALL); box(T_CLOTH, V3(x0 + 0.03f, y + 0.3f, z0 + 0.03f), V3(x1 - 0.03f, y + 0.46f, z1 - 0.03f), rgb(232, 230, 224), false, ALLB);
        float hx0 = x0, hx1 = x1, hz0 = z0, hz1 = z1; if (wall == 0) hz1 = hz0 + 0.06f; else if (wall == 2) hz0 = hz1 - 0.06f; else if (wall == 1) hx0 = hx1 - 0.06f; else hx1 = hx0 + 0.06f; box(T_WOOD, V3(hx0, y + 0.3f, hz0), V3(hx1, y + 0.85f, hz1), rgb(118, 86, 62), false, ALLB); }   // the headboard
    float top = y + (futonStyle ? 0.12f : 0.46f), cx = (x0 + x1) / 2, cz = (z0 + z1) / 2;
    { float px = cx - nx * 0.72f, pz = cz - nz * 0.72f; bool ax = wall == 0 || wall == 2; box(T_CLOTH, V3(px - (ax ? 0.28f : 0.16f), top, pz - (ax ? 0.16f : 0.28f)), V3(px + (ax ? 0.28f : 0.16f), top + 0.09f, pz + (ax ? 0.16f : 0.28f)), rgb(240, 238, 232), false, ALLB); }   // the pillow, at the wall end
    { float qx = cx + nx * 0.28f, qz = cz + nz * 0.28f; bool ax = wall == 0 || wall == 2; box(T_CLOTH, V3(qx - (ax ? 0.49f : 0.7f), top, qz - (ax ? 0.7f : 0.49f)), V3(qx + (ax ? 0.49f : 0.7f), top + 0.07f, qz + (ax ? 0.7f : 0.49f)), cover, false, ALLB); }   // the quilt
    return true;
}
// room plans
enum { RM_LIVING, RM_KITCHEN, RM_BED, RM_SHOP, RM_STOCK, RM_KONBINI, RM_KOBAN, RM_OFFICE, RM_STUDY, RM_LDK, RM_WASHITSU, RM_WASHWET, RM_WBED, RM_TOILET, RM_SENMEN, RM_BATH, RM_HALL, RM_NANDO };
static void planRoom(Rectangle rm, float y, int plan, int outer, int region, int street, bool tatami) {
    int sideA = (outer + 1) % 4, sideB = (outer + 3) % 4;
    pcRoom = rm; pcY = y; pcSol = -1; pcOn = rm.width < 12.6f && rm.height < 12.6f;   // every room: a piece never cuts one doorway off from another
    float cx = rm.x + rm.width / 2, cz = rm.y + rm.height / 2; bool longX = rm.width >= rm.height;
    switch (plan) {
    case RM_KITCHEN: { int run[] = {CK_FRIDGE, CK_CABINET, FX_SINK, FX_STOVE, CK_CABINET}; runAlong(rm, outer, y, run, 5, 0, region);
        int s2[] = {FX_TANSU}; runAlong(rm, sideA, y, s2, 1, 1, region);
        float ox = outer == 1 ? -0.8f : outer == 3 ? 0.8f : 0, oz = outer == 0 ? 0.8f : outer == 2 ? -0.8f : 0; tableSet(cx + ox, cz + oz, y, longX, 4, 0.72f, rgb(170, 130, 96)); } break;
    case RM_LIVING: { tableSet(cx, cz, y, longX, 4, tatami ? 0.35f : 0.72f, tatami ? rgb(120, 80, 54) : rgb(170, 130, 96));
        int run[] = {FX_BOOKS, CK_CABINET, FX_PLANT}; runAlong(rm, outer, y, run, 3, 0, region);
        int r2[] = {FX_TANSU, CK_BOX}; runAlong(rm, sideA, y, r2, 2, 0, region); } break;
    case RM_STUDY: { int r1[] = {CK_DESK, CK_DESK}; runAlong(rm, outer, y, r1, 2, 0, region); int r2[] = {FX_BOOKS, FX_BOOKS, CK_WARDROBE}; runAlong(rm, sideA, y, r2, 3, 0, region); } break;
    case RM_SHOP: { int r1[] = {CK_SHELF, CK_SHELF, CK_SHELF}; runAlong(rm, sideA, y, r1, 3, 0, region); runAlong(rm, sideB, y, r1, 2, 0, region);
        int r2[] = {CK_COUNTER, FX_REG}; runAlong(rm, outer, y, r2, 1, 1, region);
        if (!freeBox(cx, cz, longX ? 2.2f : 0.9f, longX ? 0.9f : 2.2f, 1.3f, y, T_METAL, rgb(200, 200, 196), true)) freeBox(cx, cz, 0.9f, 0.9f, 1.1f, y, T_WOOD, rgb(150, 120, 90), true); } break;
    case RM_STOCK: { int r1[] = {CK_BOX, FX_CRATES, CK_BOX, CK_TOOLBOX}; runAlong(rm, outer, y, r1, 4, 0, region); int r2[] = {CK_SHELF, CK_DESK}; runAlong(rm, sideA, y, r2, 2, 0, region); } break;
    case RM_KONBINI: {
        int drinks[] = {CK_FRIDGE, CK_FRIDGE, CK_FRIDGE, CK_FRIDGE}; runAlong(rm, opp(street), y, drinks, 4, 0, region);
        int side[] = {CK_SHELF, CK_SHELF, FX_COPIER}; runAlong(rm, (street + 1) % 4, y, side, 3, 0, region);
        int cnt[] = {CK_COUNTER, FX_REG, FX_ATM}; runAlong(rm, (street + 3) % 4, y, cnt, 3, 1, region);
        int mag[] = {FX_MAGS, FX_MAGS}; runAlong(rm, street, y, mag, 2, 0, region);
        bool rowsX = street == 0 || street == 2;   // gondola rows run parallel to the shop front
        for (int g = -1; g <= 1; g += 2) { float gx = rowsX ? cx : cx + g * 1.35f, gz = rowsX ? cz + g * 1.35f : cz;
            if (freeBox(gx, gz, rowsX ? fminf(4.0f, rm.width - 3.2f) : 0.9f, rowsX ? 0.9f : fminf(4.0f, rm.height - 3.2f), 1.35f, y, T_METAL, rgb(205, 208, 210), true))
                for (int sh = 0; sh < 3; sh++) { float yy = y + 0.3f + sh * 0.38f, L = rowsX ? fminf(4.0f, rm.width - 3.2f) : fminf(4.0f, rm.height - 3.2f);
                    Color pc = sh == 0 ? rgb(230, 120, 60) : sh == 1 ? rgb(90, 160, 200) : rgb(220, 200, 90);
                    if (rowsX) { box(T_PLAIN, V3(gx - L / 2 + 0.05f, yy, gz - 0.47f), V3(gx + L / 2 - 0.05f, yy + 0.2f, gz - 0.45f), pc, false, ALLB); box(T_PLAIN, V3(gx - L / 2 + 0.05f, yy, gz + 0.45f), V3(gx + L / 2 - 0.05f, yy + 0.2f, gz + 0.47f), pc, false, ALLB); }
                    else { box(T_PLAIN, V3(gx - 0.47f, yy, gz - L / 2 + 0.05f), V3(gx - 0.45f, yy + 0.2f, gz + L / 2 - 0.05f), pc, false, ALLB); box(T_PLAIN, V3(gx + 0.45f, yy, gz - L / 2 + 0.05f), V3(gx + 0.47f, yy + 0.2f, gz + L / 2 - 0.05f), pc, false, ALLB); } } }
    } break;
    case RM_KOBAN: { int r1[] = {CK_GUNLOCKER, CK_LOCKER, CK_LOCKER}; runAlong(rm, opp(street), y, r1, 3, 0, region);
        int r2[] = {FX_BOARD}; runAlong(rm, (street + 1) % 4, y, r2, 1, 0, region); int r3[] = {FX_BENCH}; runAlong(rm, (street + 3) % 4, y, r3, 1, 0, region);
        float ox = street == 1 ? 1.0f : street == 3 ? -1.0f : 0, oz = street == 2 ? 1.0f : street == 0 ? -1.0f : 0;
        if (addCont(CK_DESK, V3(cx + ox, y, cz + oz), wallYaw(opp(street)), region, true) >= 0) { chair(cx + ox * 1.6f, cz + oz * 1.6f, y, opp(street) == 0 ? 0 : opp(street) == 2 ? 2 : opp(street) == 1 ? 1 : 3, rgb(60, 60, 70)); } } break;
    case RM_LDK: {   // living-dining-kitchen: the kitchen run on one wall, the table beside it, and across the room the television with the sofa facing it
        floorFinish(rm, y, T_WOOD, rgb(196, 160, 120));
        int dw; float du; int dm = roomDoors(rm, y, &dw, &du); int ord[4]; wallOrder(ord, outer, sideA, sideB, opp(outer), dm, -1);
        int run[] = {CK_FRIDGE, CK_CABINET, FX_SINK, FX_STOVE, CK_CABINET}; int kw = runBest(rm, y, run, 5, ord, 4, 0, region); if (kw < 0) kw = outer;
        float nx, nz; wallIn(kw, &nx, &nz); bool kx = kw == 0 || kw == 2; bool tabled = false;
        for (int tr = 0; tr < 6 && !tabled; tr++) { float fr2 = tr % 3 == 0 ? 0.3f : tr % 3 == 1 ? 0.7f : 0.5f, dist = tr < 3 ? 1.85f : 2.3f, u = wallAt(rm, kw, fr2);
            float tx = kx ? u : (kw == 3 ? rm.x : rm.x + rm.width) + nx * dist, tz = kx ? (kw == 0 ? rm.y : rm.y + rm.height) + nz * dist : u;
            tabled = tableSet(tx, tz, y, kx, 4, 0.72f, rgb(170, 130, 96)); }
        // the television on a wall away from the kitchen, the sofa looking at it across a low table
        { int to[4]; wallOrder(to, opp(kw), (kw + 1) % 4, (kw + 3) % 4, -1, dm, kw); Color sc = (Color[]){{120, 110, 100, 255}, {90, 110, 130, 255}, {140, 90, 80, 255}}[ri(3)]; bool done = false;
          for (int q = 0; q < 3 && !done; q++) { int tw = to[q]; if (tw < 0) continue; float u; float depth = (tw == 0 || tw == 2) ? rm.height : rm.width; if (depth < 2.6f) continue;
              for (int fq = 0; fq < 3 && !done; fq++) { if (!spotOn(rm, tw, 1.2f, 0.42f, 0.6f, y, fq == 0 ? 0.75f : fq == 1 ? 0.25f : 0.5f, 0.6f, &u)) continue;
                  float tnx, tnz; wallIn(tw, &tnx, &tnz); bool ax = tw == 0 || tw == 2; float wx = ax ? u : (tw == 3 ? rm.x : rm.x + rm.width), wz = ax ? (tw == 0 ? rm.y : rm.y + rm.height) : u;
                  float dist = fminf(3.0f, depth - 0.45f); bool sat = false;
                  for (float dd = dist; dd >= 2.1f && !sat; dd -= 0.3f) sat = sofaAt(wx + tnx * dd, wz + tnz * dd, opp(tw), y, sc);
                  if (!sat) continue;
                  tvOn(rm, tw, u, y); freeBox(wx + tnx * 1.35f, wz + tnz * 1.35f, ax ? 0.9f : 0.5f, ax ? 0.5f : 0.9f, 0.35f, y, T_WOOD, rgb(110, 80, 58), true); done = true; } } }
        { int po[4]; wallOrder(po, opp(kw), (kw + 1) % 4, (kw + 3) % 4, -1, dm, kw); int r2[] = {FX_PLANT}; runBest(rm, y, r2, 1, po, 3, 1, region); } } break;
    case RM_WASHITSU: {   // tatami room: the tokonoma and the altar on the wall away from the door, the closet (oshiire) on another, a low table with cushions in the middle
        floorFinish(rm, y, T_TATAMI, WHITE);
        int dw; float du; int dm = roomDoors(rm, y, &dw, &du); int ord[4]; wallOrder(ord, dw >= 0 ? opp(dw) : sideA, sideA, sideB, outer, dm, -1); int tkw = -1;
        for (int q = 0; q < 4 && tkw < 0; q++) { float u; if (ord[q] >= 0 && spotOn(rm, ord[q], 1.6f, 0.75f, 1.9f, y, 0.8f, 0.5f, &u)) { tkw = ord[q]; tokonomaU(rm, tkw, u, y); float u2; if (spotOn(rm, tkw, 0.7f, 0.5f, 1.3f, y, 0.1f, 0.5f, &u2)) butsudanU(rm, tkw, u2, y); } }
        { int co[4]; wallOrder(co, sideB, sideA, opp(outer), outer, dm, tkw); int r1[] = {CK_WARDROBE, CK_WARDROBE}; runBest(rm, y, r1, 2, co, 4, 0, region); }
        { bool ok = false; for (int tr = 0; tr < 5 && !ok; tr++) ok = tableSet(cx + (tr == 1 ? 0.5f : tr == 2 ? -0.5f : 0), cz + (tr == 3 ? 0.5f : tr == 4 ? -0.5f : 0), y, longX, 4, 0.35f, rgb(120, 80, 54)); } } break;
    case RM_WASHWET: {   // the back room: bathroom, toilet and washroom behind a partition at one end, a tatami room in the rest
        int ws = sideA; if (!wetStrip(rm, sideA, y, region)) { ws = sideB; if (!wetStrip(rm, sideB, y, region)) { ws = outer; if (!wetStrip(rm, outer, y, region)) ws = -1; } }   // whichever end is clear of the stairs and the doorway
        bool alongX = ws == 0 || ws == 2; Rectangle rest = rm; if (ws >= 0) { if (alongX) { if (ws == 0) { rest.y += 1.8f; } rest.height -= 1.8f; } else { if (ws == 3) { rest.x += 1.8f; } rest.width -= 1.8f; } }
        floorFinish(rest, y, T_TATAMI, WHITE); tokonoma(rest, ws >= 0 ? opp(ws) : sideA, y);
        int r1[] = {CK_WARDROBE}; runAlong(rest, outer, y, r1, 1, 1, region);
        tableSet(rest.x + rest.width / 2, rest.y + rest.height / 2, y, rest.width >= rest.height, 2, 0.35f, rgb(120, 80, 54)); } break;
    case RM_TOILET: {   // the toilet: the bowl on the wall facing the door, its tank behind it, a hand basin by the door, slippers
        floorFinish(rm, y, T_TILEWALL, rgb(214, 216, 212)); fTight = true;
        int dw; float du; int dm = roomDoors(rm, y, &dw, &du); int ord[4]; wallOrder(ord, dw >= 0 ? opp(dw) : outer, outer, sideA, sideB, dm, -1); int bw = -1; float bu = 0;
        for (int q = 0; q < 4 && bw < 0; q++) if (ord[q] >= 0 && spotOn(rm, ord[q], 0.46f, 0.66f, 0.5f, y, 0.5f, 0, &bu)) bw = ord[q];
        if (bw >= 0) { float x0, z0, x1, z1, nx, nz; wallIn(bw, &nx, &nz); wallFoot(rm, bw, bu, 0.46f, 0.2f, &x0, &z0, &x1, &z1); box(T_PLAIN, V3(x0, y, z0), V3(x1, y + 0.82f, z1), rgb(232, 232, 228), true, ALL);   // the tank
            wallFoot(rm, bw, bu, 0.4f, 0.66f, &x0, &z0, &x1, &z1); if (bw == 0) z0 += 0.2f; else if (bw == 2) z1 -= 0.2f; else if (bw == 1) x1 -= 0.2f; else x0 += 0.2f;
            box(T_PLAIN, V3(x0, y, z0), V3(x1, y + 0.4f, z1), rgb(240, 240, 238), true, ALL); box(T_PLAIN, V3(x0 + 0.04f, y + 0.4f, z0 + 0.04f), V3(x1 - 0.04f, y + 0.43f, z1 - 0.04f), rgb(222, 222, 218), false, ALLB);   // bowl and lid
            int sw = (bw + 1) % 4; float su; if (!spotOn(rm, sw, 0.3f, 0.12f, 0.2f, y + 0.6f, 0.5f, 0, &su)) { sw = (bw + 3) % 4; if (!spotOn(rm, sw, 0.3f, 0.12f, 0.2f, y + 0.6f, 0.5f, 0, &su)) sw = -1; }
            if (sw >= 0) { wallFoot(rm, sw, su, 0.14f, 0.1f, &x0, &z0, &x1, &z1); box(T_PLAIN, V3(x0, y + 0.62f, z0), V3(x1, y + 0.74f, z1), rgb(244, 244, 240), false, ALLB); } }   // the paper
        if (dw >= 0) { float nx, nz; wallIn(dw, &nx, &nz); bool ax = dw == 0 || dw == 2; float sx = ax ? du : (dw == 3 ? rm.x : rm.x + rm.width) + nx * 0.3f, sz = ax ? (dw == 0 ? rm.y : rm.y + rm.height) + nz * 0.3f : du;
            box(T_PLAIN, V3(sx - 0.14f, y + 0.024f, sz - 0.14f), V3(sx + 0.14f, y + 0.06f, sz + 0.14f), rgb(90, 140, 120), false, ALL); }   // the slippers
        { int bo[4]; wallOrder(bo, sideA, sideB, outer, opp(outer), dm, bw); for (int q = 0; q < 4; q++) { float u; if (bo[q] >= 0 && spotOn(rm, bo[q], 0.36f, 0.24f, 0.2f, y + 0.7f, 0.8f, 0, &u)) { wallBox(rm, bo[q], u, 0.36f, 0.22f, y + 0.72f, 0.12f, T_PLAIN, rgb(236, 236, 234), false); break; } } }   // the hand basin
        fTight = false; } break;
    case RM_SENMEN: {   // the washroom: the basin cabinet with its mirror, the washing machine beside it, a laundry basket
        floorFinish(rm, y, T_TILEWALL, rgb(210, 214, 216)); fTight = true;
        int dw; float du; int dm = roomDoors(rm, y, &dw, &du); int ord[4]; wallOrder(ord, outer, sideA, sideB, opp(outer), dm, -1); int vw = -1; float vu = 0;
        for (int q = 0; q < 4 && vw < 0; q++) if (ord[q] >= 0 && spotOn(rm, ord[q], ckSize[CK_CABINET].x, ckSize[CK_CABINET].z, 0.9f, y, 0.2f, 0.45f, &vu) && putPiece(CK_CABINET, rm, ord[q], vu, y, region)) vw = ord[q];
        if (vw >= 0) { float x0, z0, x1, z1; wallFoot(rm, vw, vu, 0.7f, 0.05f, &x0, &z0, &x1, &z1); box(T_GLASS, V3(x0, y + 1.15f, z0), V3(x1, y + 1.8f, z1), rgb(170, 190, 200), false, ALL);   // the mirror
            wallFoot(rm, vw, vu, 0.5f, 0.4f, &x0, &z0, &x1, &z1); box(T_PLAIN, V3(x0, y + ckSize[CK_CABINET].y, z0), V3(x1, y + ckSize[CK_CABINET].y + 0.04f, z1), rgb(240, 240, 238), false, ALLB); }   // the basin
        { bool wm = false; for (int q = 0; q < 4 && !wm; q++) { int ww = q == 0 && vw >= 0 ? vw : ord[q]; float u; if (ww >= 0 && spotOn(rm, ww, 0.62f, 0.62f, 0.9f, y, 0.9f, 0.4f, &u)) { wm = wallBox(rm, ww, u, 0.62f, 0.62f, y, 0.9f, T_PLAIN, rgb(238, 238, 236), true);
              if (wm) { float x0, z0, x1, z1; wallFoot(rm, ww, u, 0.4f, 0.5f, &x0, &z0, &x1, &z1); box(T_PLAIN, V3(x0, y + 0.9f, z0), V3(x1, y + 0.92f, z1), rgb(150, 160, 170), false, ALLB); } } } }   // the washing machine and its lid
        fTight = false; } break;
    case RM_BATH: {   // the bathroom: a deep tub across the wall away from the door, the washing place in front of it with a stool and a bowl, a shower on the side wall
        floorFinish(rm, y, T_TILEWALL, rgb(196, 206, 210)); fTight = true;
        int dw; float du; int dm = roomDoors(rm, y, &dw, &du); int ord[4]; wallOrder(ord, dw >= 0 ? opp(dw) : outer, outer, sideA, sideB, dm, -1); int tw = -1; float tu = 0, tl = 0;
        for (int q = 0; q < 4 && tw < 0; q++) { if (ord[q] < 0) continue; float len = fminf(wallHi(rm, ord[q]) - wallLo(rm, ord[q]) - 0.1f, 1.6f); for (float l2 = len; l2 >= 1.0f && tw < 0; l2 -= 0.2f) if (spotOn(rm, ord[q], l2, 0.75f, 0.6f, y, 0.0f, 0, &tu)) { tw = ord[q]; tl = l2; } }
        if (tw >= 0) { float x0, z0, x1, z1; wallFoot(rm, tw, tu, tl, 0.75f, &x0, &z0, &x1, &z1); box(T_PLAIN, V3(x0, y, z0), V3(x1, y + 0.58f, z1), rgb(236, 238, 238), true, ALL); box(T_PLAIN, V3(x0 + 0.08f, y + 0.581f, z0 + 0.08f), V3(x1 - 0.08f, y + 0.59f, z1 - 0.08f), rgb(110, 140, 150), false, 16);
            float nx, nz; wallIn(tw, &nx, &nz); float sx = (x0 + x1) / 2 + nx * 0.85f, sz = (z0 + z1) / 2 + nz * 0.85f;
            if (sx > rm.x + 0.2f && sx < rm.x + rm.width - 0.2f && sz > rm.y + 0.2f && sz < rm.y + rm.height - 0.2f) { box(T_PLAIN, V3(sx - 0.15f, y, sz - 0.15f), V3(sx + 0.15f, y + 0.25f, sz + 0.15f), rgb(150, 190, 200), false, ALL);   // the stool
                box(T_PLAIN, V3(sx + (nz != 0 ? 0.3f : 0) - 0.12f, y, sz + (nx != 0 ? 0.3f : 0) - 0.12f), V3(sx + (nz != 0 ? 0.3f : 0) + 0.12f, y + 0.12f, sz + (nx != 0 ? 0.3f : 0) + 0.12f), rgb(226, 214, 150), false, ALL); }   // the wash bowl
            for (int q = 1; q <= 3; q += 2) { int sw = (tw + q) % 4; float u; if (spotOn(rm, sw, 0.1f, 0.1f, 0.2f, y + 0.9f, 0.5f, 0, &u)) { wallBox(rm, sw, u, 0.08f, 0.08f, y + 0.9f, 0.7f, T_METAL, rgb(190, 192, 196), false); break; } } }   // the shower
        fTight = false; } break;
    case RM_HALL: floorFinish(rm, y, T_WOOD, rgb(170, 130, 92)); break;   // the corridor (rouka): polished boards
    case RM_WBED: case RM_BED: {   // a bedroom: the bed (a futon in a tatami room) with its head to a wall, in the corner away from the door; the desk at the window with its chair; wardrobe and shelves on a blank wall
        bool fut = plan == RM_BED; if (!fut) floorFinish(rm, y, T_WOOD, rgb(186, 150, 112));
        int dw; float du; int dm = roomDoors(rm, y, &dw, &du); int ord[4]; wallOrder(ord, dw >= 0 ? opp(dw) : sideA, sideA, sideB, outer, dm, -1); int hw = -1; float hu = 0;
        Color cov = (Color[]){{96, 116, 160, 255}, {150, 84, 92, 255}, {110, 140, 110, 255}, {180, 160, 120, 255}}[ri(4)];
        for (int q = 0; q < 4 && hw < 0; q++) { if (ord[q] < 0) continue; float far = 0; if (dw >= 0 && dw != ord[q] && dw != opp(ord[q])) far = dw == 0 || dw == 3 ? 1.0f : 0.0f; else if (dw >= 0) far = du > (wallLo(rm, ord[q]) + wallHi(rm, ord[q])) / 2 ? 0.0f : 1.0f;
            if (spotOn(rm, ord[q], 1.0f, 2.0f, 0.5f, y, far, 0, &hu)) hw = ord[q]; }
        if (hw >= 0) bedOn(rm, hw, hu, y, fut, cov);
        if (fut && rm.width * rm.height > 14 && hw >= 0) { float u2; if (spotOn(rm, hw, 1.0f, 2.0f, 0.5f, y, hu > (wallLo(rm, hw) + wallHi(rm, hw)) / 2 ? 1.0f : 0.0f, 0, &u2) && fabsf(u2 - hu) < 1.6f) bedOn(rm, hw, u2, y, true, rgb(150, 80, 90)); }
        { int dk[4]; wallOrder(dk, outer, sideA, sideB, opp(outer), dm, -1); for (int q = 0; q < 4; q++) { float u; if (dk[q] < 0 || !spotOn(rm, dk[q], ckSize[CK_DESK].x, ckSize[CK_DESK].z, 0.75f, y, 0.5f, 0.75f, &u)) continue;   // the desk, at the window if it can be
              if (!putPiece(CK_DESK, rm, dk[q], u, y, region)) continue; float nx, nz; wallIn(dk[q], &nx, &nz); bool ax = dk[q] == 0 || dk[q] == 2; float bx = ax ? u : (dk[q] == 3 ? rm.x : rm.x + rm.width), bz = ax ? (dk[q] == 0 ? rm.y : rm.y + rm.height) : u;
              chair(bx + nx * (ckSize[CK_DESK].z + 0.36f), bz + nz * (ckSize[CK_DESK].z + 0.36f), y, opp(dk[q]), rgb(70, 70, 80)); break; } }
        { int wo[4]; wallOrder(wo, opp(outer), sideB, sideA, outer, dm, -1); int r1[] = {CK_WARDROBE, FX_BOOKS}; int ww = runBest(rm, y, r1, 2, wo, 4, 0, region); int r2[] = {CK_BOX}; int bo[4]; wallOrder(bo, sideB, sideA, opp(outer), outer, dm, ww); runBest(rm, y, r2, 1, bo, 4, 1, region); } } break;
    case RM_NANDO: {   // the store room: boxes and crates along the walls, a shelf, what the family put away
        floorFinish(rm, y, T_WOOD, rgb(160, 124, 90));
        int dw; float du; int dm = roomDoors(rm, y, &dw, &du); int ord[4]; wallOrder(ord, outer, sideA, sideB, opp(outer), dm, -1);
        if (fminf(rm.width, rm.height) < 1.8f || rm.width * rm.height < 3.0f) { int r0[] = {CK_BOX}; runBest(rm, y, r0, 1, ord, 4, 0, region); break; }   // a closet: one box, room to step in
        int r1[] = {CK_BOX, FX_CRATES, CK_BOX}; int a = runBest(rm, y, r1, 3, ord, 4, 0, region); int o2[4]; wallOrder(o2, sideA, sideB, opp(outer), outer, dm, a); int r2[] = {CK_WARDROBE}; if (runBest(rm, y, r2, 1, o2, 4, 0, region) < 0) { int r3[] = {CK_BOX}; runBest(rm, y, r3, 1, o2, 4, 1, region); } } break;
    case RM_OFFICE: { int r1[] = {CK_LOCKER, CK_LOCKER, CK_CABINET, FX_COOLER}; runAlong(rm, outer, y, r1, 4, 0, region);
        int r2[] = {FX_BOARD}; runAlong(rm, sideA, y, r2, 1, 0, region);
        for (int isl = -1; isl <= 1; isl += 2) { float ix = longX ? cx + isl * 1.7f : cx, iz = longX ? cz : cz + isl * 1.7f;   // a desk island: two desks face to face, screens, chairs
            if (!fFree(ix - 1.4f, iz - 1.4f, ix + 1.4f, iz + 1.4f, y)) continue;
            int a = addCont(CK_DESK, V3(ix, y, iz - (longX ? 0.3f : 0)), longX ? PI : PI / 2, region, true); (void)a;
            if (longX) { addCont(CK_DESK, V3(ix, y, iz + 0.3f), 0, region, true); chair(ix, iz - 0.95f, y, 0, rgb(50, 60, 70)); chair(ix, iz + 0.95f, y, 2, rgb(50, 60, 70));
                box(T_PLAIN, V3(ix - 0.25f, y + 0.75f, iz - 0.1f), V3(ix + 0.25f, y + 1.1f, iz - 0.06f), rgb(30, 30, 34), false, ALLB); box(T_PLAIN, V3(ix - 0.25f, y + 0.75f, iz + 0.06f), V3(ix + 0.25f, y + 1.1f, iz + 0.1f), rgb(30, 30, 34), false, ALLB); } }
        } break;
    }
}
#include "zk_bpresets.h"
static const BPreset *bP = NULL;   // set while a preset is being built (Mod Maker parts); NULL: a town lot, details chosen by kind
// 0.9.7: what sticks out of a wall (balconies, AC units, awnings, pent roofs, stairs) is left off where it would reach into the lot of
// another building, and is solid (cloth and glass aside) so it does not stand in the air as a ghost.
static bool bdMode; static int bdOwn = -1; static int bdSkipped;
static float tLots[2400][8]; static int nTLots;   // (zk_layout.h: townPlan)
static bool bdInLot(int l, float x, float z, float m) { float cx = tLots[l][0] + tLots[l][2] / 2, cz = tLots[l][1] + tLots[l][3] / 2, R = (tLots[l][2] + tLots[l][3]) / 2 + m; if (fabsf(x - cx) > R || fabsf(z - cz) > R) return false;
    float c = cosf(tLots[l][7]), sn = sinf(tLots[l][7]), dx = x - cx, dz = z - cz, lx = dx * c - dz * sn, lz = dx * sn + dz * c; return fabsf(lx) < tLots[l][2] / 2 + m && fabsf(lz) < tLots[l][3] / 2 + m; }
static bool bdClear(Vector3 mn, Vector3 mx) {   // the box (in the current frame) keeps out of every other building
    if (bdOwn < 0) return true;
    for (int i = 0; i <= 4; i++) for (int j = 0; j <= 4; j++) { Vector3 w = xfP(curXf, V3(Lerp(mn.x, mx.x, i / 4.0f), 0, Lerp(mn.z, mx.z, j / 4.0f)));
        for (int l = 0; l < nTLots; l++) if (l != bdOwn && bdInLot(l, w.x, w.z, 0.05f)) return false; }
    return true;
}
static void fBox(Rectangle r, int face, float u0, float u1, float y0, float y1, float o0, float o1, int t, Color c, bool solid) {   // a box on one face: u along the face, o outward from the wall
    Vector3 mn, mx;
    if (face == 0) { mn = V3(r.x + u0, y0, r.y - o1); mx = V3(r.x + u1, y1, r.y - o0); }
    else if (face == 2) { mn = V3(r.x + u0, y0, r.y + r.height + o0); mx = V3(r.x + u1, y1, r.y + r.height + o1); }
    else if (face == 1) { mn = V3(r.x + r.width + o0, y0, r.y + u0); mx = V3(r.x + r.width + o1, y1, r.y + u1); }
    else { mn = V3(r.x - o1, y0, r.y + u0); mx = V3(r.x - o0, y1, r.y + u1); }
    if (bdMode && o1 > 0.08f) { if (!bdClear(mn, mx)) { bdSkipped++; return; } if (t != T_CLOTH && t != T_GLASS) solid = true; }
    box(t, mn, mx, c, solid, ALL);
}
static float fLen(Rectangle r, int face) { return (face == 0 || face == 2) ? r.width : r.height; }
static void bDetails(Rectangle r, int kind, int floors, int street, float top, Color ec, unsigned fl) {   // the clutter that makes a Japanese street: balconies, laundry, AC units, pipes, tanks
    int side = (street + 1) % 4, back = (street + 2) % 4; float L = fLen(r, street), Ls = fLen(r, side);
    Color dark = (Color){(unsigned char)(ec.r * 0.7f), (unsigned char)(ec.g * 0.7f), (unsigned char)(ec.b * 0.7f), 255};
    if ((fl & BD_BRICKBASE)) fBox(r, street, 0, L, SIDEH, SIDEH + 1.0f, 0, 0.06f, T_BRICK, WHITE, false);
    for (int fc = 0; fc < 4; fc++) if (fc != street) fBox(r, fc, -0.03f, fLen(r, fc) + 0.03f, SIDEH, SIDEH + 0.45f, 0, 0.05f, T_CONCRETE, rgb(120, 118, 112), false);   // the base course along the sides and back
    if (kind == 2 && floors > 1 && !(fl & BD_BANDS)) fl |= BD_BANDS;   // floor lines on every shop-house
    if ((fl & BD_BANDS) && floors > 1) for (int f = 1; f < floors; f++) for (int fc = 0; fc < 4; fc++) fBox(r, fc, -0.05f, fLen(r, fc) + 0.05f, SIDEH + f * FH - 0.08f, SIDEH + f * FH + 0.08f, 0, 0.07f, T_CONCRETE, dark, false);
    if ((fl & BD_BALC) && floors > 1) for (int f = 1; f < floors; f++) {   // balconies on the street face, with railings (and laundry poles)
        float y = SIDEH + f * FH; int nb = L > 9 ? 2 : 1; float bw = (L - 1.0f) / nb;
        for (int b = 0; b < nb; b++) { float u0 = 0.5f + b * bw + 0.1f, u1 = u0 + bw - 0.2f;
            fBox(r, street, u0, u1, y - 0.12f, y + 0.05f, 0, 0.95f, T_CONCRETE, ec, false);
            fBox(r, street, u0, u1, y + 0.05f, y + 1.0f, 0.88f, 0.95f, T_METAL, rgb(150, 155, 160), false);
            fBox(r, street, u0, u0 + 0.06f, y + 0.05f, y + 1.0f, 0, 0.95f, T_METAL, rgb(150, 155, 160), false);
            fBox(r, street, u1 - 0.06f, u1, y + 0.05f, y + 1.0f, 0, 0.95f, T_METAL, rgb(150, 155, 160), false);
            if (fl & BD_LAUND) { fBox(r, street, u0 + 0.2f, u1 - 0.2f, y + 1.75f, y + 1.8f, 0.6f, 0.65f, T_METAL, rgb(170, 170, 170), false);
                int nc = 1 + ri(4); for (int k = 0; k < nc; k++) { float cu = fr(u0 + 0.4f, u1 - 0.9f); Color cc = (Color[]){{220, 220, 225, 255}, {90, 120, 180, 255}, {200, 80, 70, 255}, {230, 210, 120, 255}, {120, 160, 110, 255}}[ri(5)];
                    fBox(r, street, cu, cu + fr(0.4f, 0.8f), y + fr(0.9f, 1.3f), y + 1.75f, 0.6f, 0.64f, T_CLOTH, cc, false); } } } }
    if (fl & BD_AC) for (int f = 0; f < floors && f < 8; f++) { float u = fr(0.6f, Ls - 1.5f), y = SIDEH + f * FH + (f ? 0.2f : 0.3f);   // outdoor AC units down the side wall
        fBox(r, side, u, u + 0.8f, y, y + 0.6f, 0, 0.3f, T_METAL, rgb(205, 205, 198), false); fBox(r, side, u + 0.15f, u + 0.55f, y + 0.12f, y + 0.48f, 0.3f, 0.31f, T_METAL, rgb(80, 82, 85), false); }
    if (fl & BD_PIPES) { fBox(r, street, 0.12f, 0.24f, SIDEH, top + 0.5f, 0, 0.12f, T_METAL, rgb(130, 130, 125), false); fBox(r, street, L - 0.24f, L - 0.12f, SIDEH, top + 0.5f, 0, 0.12f, T_METAL, rgb(130, 130, 125), false); }
    if ((fl & BD_STAIR) && floors > 1) { int sd = back; float Lb = fLen(r, sd);   // an open steel stair up the back wall
        for (int f = 0; f < floors - 1 && f < 6; f++) { float y = SIDEH + f * FH; bool lr = f % 2;
            for (int k = 0; k < 8; k++) { float u = lr ? Lb - 1.0f - k * 0.35f : 1.0f + k * 0.35f; fBox(r, sd, fminf(u, u + 0.35f), fmaxf(u, u + 0.35f), y + k * FH / 8, y + k * FH / 8 + 0.08f, 0.05f, 1.0f, T_METAL, rgb(110, 115, 118), false); }
            fBox(r, sd, 0.4f, Lb - 0.4f, y + FH - 0.08f, y + FH, 0.05f, 1.05f, T_METAL, rgb(110, 115, 118), false);
            fBox(r, sd, 0.4f, Lb - 0.4f, y + FH, y + FH + 1.0f, 1.0f, 1.05f, T_METAL, rgb(140, 145, 148), false); } }
    if (fl & BD_AWN) { Color ac = (Color[]){{190, 60, 50, 255}, {50, 110, 70, 255}, {60, 80, 150, 255}, {200, 150, 60, 255}}[ri(4)];   // striped awning over the shop front
        for (float u = 0.3f; u < L - 0.3f; u += 0.6f) fBox(r, street, u, fminf(u + 0.6f, L - 0.3f), GH - 0.35f, GH - 0.15f, 0, 1.3f, T_CLOTH, ((int)(u / 0.6f)) % 2 ? ac : rgb(235, 232, 225), false);
        fBox(r, street, 0.3f, L - 0.3f, GH - 0.65f, GH - 0.35f, 1.25f, 1.3f, T_CLOTH, ac, false); }
    if (fl & BD_SHUT) fBox(r, street, 0.4f, L - 0.4f, GH - 0.3f, GH + 0.15f, 0, 0.3f, T_SHUTTER, rgb(170, 170, 165), false);   // shutter box over the shop front
    if (fl & BD_MAIL) { float u = L > 4 ? L - 1.2f : 0.3f; fBox(r, street, u, u + 0.4f, SIDEH + 0.9f, SIDEH + 1.35f, 0, 0.25f, T_METAL, rgb(170, 40, 40), false); }
    {   // Odawara house details (0.9.0, after street photos): block wall + fence, garage shutter, pent roof, door hood, carport
        int sd = street; float len = (sd == 0 || sd == 2) ? r.width : r.height - 2 * curT, base = (sd == 0 || sd == 2) ? 0 : curT; int nm = (int)(len / 3.0f); if (nm < 1) nm = 1; float mw = len / nm, uc = base + (nm / 2 + 0.5f) * mw;
        Color rc = bP && bP->roof.a ? bP->roof : rgb(72, 74, 80);
        if (fl & BD_FENCE) { float o0 = 0.55f, o1 = 0.72f; bool ok = true;
            for (float u = 0; u <= L && ok; u += 1.0f) { Vector3 q = sd == 0 ? V3(r.x + u, 0, r.y - 0.8f) : sd == 2 ? V3(r.x + u, 0, r.y + r.height + 0.8f) : sd == 1 ? V3(r.x + r.width + 0.8f, 0, r.y + u) : V3(r.x - 0.8f, 0, r.y + u); Vector3 wq = xfP(curXf, q); if (mainRoadAt(wq.x, wq.z, 0.15f)) ok = false; }
            if (ok) for (int pc = 0; pc < 2; pc++) { float a0 = pc ? uc + 1.0f : 0.1f, a1 = pc ? L - 0.1f : uc - 1.0f; if (a1 - a0 < 0.5f) continue;
                fBox(r, sd, a0, a1, SIDEH, SIDEH + 0.75f, o0, o1, T_BLOCKWALL, WHITE, true);
                fBox(r, sd, a0, a1, SIDEH + 1.25f, SIDEH + 1.3f, o0 + 0.06f, o1 - 0.06f, T_METAL, rgb(150, 110, 95), false);
                for (float u = a0 + 0.05f; u < a1; u += 0.22f) fBox(r, sd, u, u + 0.025f, SIDEH + 0.75f, SIDEH + 1.3f, o0 + 0.07f, o0 + 0.1f, T_METAL, rgb(150, 110, 95), false); } }
        if ((fl & BD_GARAGE) && len >= 6.0f) { float g0 = uc > len / 2 ? base + 0.3f : base + len - 2.9f; fBox(r, sd, g0, g0 + 2.5f, SIDEH, SIDEH + 2.25f, 0, 0.05f, T_SHUTTER, rgb(200, 200, 196), false);
            fBox(r, sd, g0 - 0.1f, g0 + 2.6f, SIDEH + 2.25f, SIDEH + 2.55f, 0, 0.25f, T_PLAIN, rgb(185, 180, 170), false); }   // its shutter box
        if ((fl & BD_SKIRT) && floors >= 2) { float y = SIDEH + FH; for (int fc = 0; fc < 4; fc++) { float Lf = fLen(r, fc);   // the pent roof between the floors of an old house
            fBox(r, fc, -0.55f, Lf + 0.55f, y - 0.1f, y + 0.05f, 0, 0.75f, T_ROOF, (Color){(unsigned char)(rc.r * 0.85f), (unsigned char)(rc.g * 0.85f), (unsigned char)(rc.b * 0.85f), 255}, false);
            fBox(r, fc, -0.4f, Lf + 0.4f, y + 0.05f, y + 0.22f, 0, 0.4f, T_ROOF, rc, false); } }
        if (fl & BD_HISASHI) { fBox(r, sd, uc - 0.95f, uc + 0.95f, SIDEH + 2.45f, SIDEH + 2.58f, 0, 0.9f, T_PLAIN, rgb(200, 198, 192), false);   // the hood over the front door
            if (floors >= 2) for (int m = 0; m < nm; m++) { float u0 = base + m * mw + 0.4f; fBox(r, sd, u0, u0 + mw - 0.8f, SIDEH + FH + 2.2f, SIDEH + FH + 2.28f, 0, 0.35f, T_PLAIN, rgb(190, 188, 182), false); } }
        if (fl & BD_CARPORT) { int cs = (sd + 1) % 4; float Lc = fLen(r, cs); bool ok = true;   // a polycarbonate carport against the side wall, nearest the street
            float c0 = sd == 0 || sd == 3 ? 0.2f : Lc - 5.2f, c1 = c0 + 5.0f; if (c0 < 0) { c0 = 0; c1 = fminf(5.0f, Lc); }
            for (int q = 0; q < 2 && ok; q++) { float u = q ? c1 - 0.1f : c0 + 0.1f; Vector3 lp = cs == 0 ? V3(r.x + u, 0, r.y - 2.7f) : cs == 2 ? V3(r.x + u, 0, r.y + r.height + 2.7f) : cs == 1 ? V3(r.x + r.width + 2.7f, 0, r.y + u) : V3(r.x - 2.7f, 0, r.y + u);
                Vector3 wq = xfP(curXf, lp); if (mainRoadAt(wq.x, wq.z, 0.3f) || !pointFree(lp.x, lp.z, 0.4f, SIDEH + 0.1f)) ok = false; }
            if (ok) { for (int q = 0; q < 2; q++) { float u = q ? c1 - 0.15f : c0 + 0.05f; fBox(r, cs, u, u + 0.1f, SIDEH, SIDEH + 2.3f, 2.6f, 2.7f, T_METAL, rgb(120, 110, 100), true); }
                fBox(r, cs, c0, c1, SIDEH + 2.3f, SIDEH + 2.38f, 0, 2.8f, T_METAL, rgb(120, 110, 100), false); fBox(r, cs, c0 + 0.1f, c1 - 0.1f, SIDEH + 2.38f, SIDEH + 2.42f, 0.1f, 2.75f, T_GLASS, rgb(150, 160, 150), false); } }
    }
    if (fl & BD_BIKES) for (int k = 0; k < 1 + ri(3); k++) { float u = 0.6f + k * 0.7f; if (u > L - 1) break;   // bicycles parked against the front
        { Vector3 q = street == 0 ? V3(r.x + u, 0, r.y - 2.0f) : street == 2 ? V3(r.x + u, 0, r.y + r.height + 2.0f) : street == 1 ? V3(r.x + r.width + 2.0f, 0, r.y + u) : V3(r.x - 2.0f, 0, r.y + u);
          Vector3 wq = xfP(curXf, q); if (mainRoadAt(wq.x, wq.z, 0.1f)) continue; }   // never out in the road
        fBox(r, street, u, u + 0.06f, SIDEH + 0.3f, SIDEH + 0.95f, 0.3f, 1.9f, T_METAL, (Color[]){{60, 60, 65, 255}, {180, 180, 185, 255}, {150, 40, 40, 255}}[ri(3)], false);
        fBox(r, street, u - 0.02f, u + 0.08f, SIDEH, SIDEH + 0.62f, 0.32f, 0.38f, T_PLAIN, rgb(30, 30, 30), false); fBox(r, street, u - 0.02f, u + 0.08f, SIDEH, SIDEH + 0.62f, 1.82f, 1.88f, T_PLAIN, rgb(30, 30, 30), false); }
    if ((fl & (BD_PITCH | BD_HIP)) == 0) {   // the roof
        {   // a parapet round the flat roof with a coping on top, and a cornice line under it: no building ends in a bare box edge
            Color pc = (Color){(unsigned char)(ec.r * 0.82f), (unsigned char)(ec.g * 0.82f), (unsigned char)(ec.b * 0.82f), 255};
            for (int fc = 0; fc < 4; fc++) { float Lf = fLen(r, fc);
                fBox(r, fc, 0, Lf, top, top + 0.55f, -0.18f, 0, T_CONCRETE, pc, true);
                fBox(r, fc, -0.05f, Lf + 0.05f, top + 0.55f, top + 0.62f, -0.22f, 0.05f, T_CONCRETE, rgb(170, 170, 166), false);
                fBox(r, fc, -0.04f, Lf + 0.04f, top - 0.32f, top - 0.2f, 0, 0.08f, T_CONCRETE, pc, false); } }
        if (floors >= 3 && !(fl & BD_TANK)) {   // the stair hut and a few condensers on the roof
            float hx = r.x + r.width * 0.15f, hz = r.y + r.height * 0.55f, hw = fminf(2.6f, r.width * 0.3f), hd = fminf(3.2f, r.height * 0.35f);
            box(T_CONCRETE, V3(hx, top, hz), V3(hx + hw, top + 2.4f, hz + hd), (Color){(unsigned char)(ec.r * 0.9f), (unsigned char)(ec.g * 0.9f), (unsigned char)(ec.b * 0.9f), 255}, true, ALL);
            box(T_CONCRETE, V3(hx - 0.1f, top + 2.4f, hz - 0.1f), V3(hx + hw + 0.1f, top + 2.5f, hz + hd + 0.1f), rgb(150, 150, 146), false, ALL);
            box(T_METAL, V3(hx + hw * 0.3f, top, hz - 0.02f), V3(hx + hw * 0.7f, top + 2.0f, hz), rgb(110, 115, 120), false, ALL);
            for (int k = 0; k < 2 + ri(3); k++) { float cx = r.x + r.width * fr(0.5f, 0.85f), cz = r.y + r.height * fr(0.15f, 0.8f);
                box(T_METAL, V3(cx, top, cz), V3(cx + 0.8f, top + 0.6f, cz + 0.3f), rgb(205, 205, 200), true, ALL); } }
        if (fl & BD_TANK) { float tx = r.x + r.width * fr(0.2f, 0.6f), tz = r.y + r.height * fr(0.2f, 0.6f);
            for (int k = 0; k < 4; k++) box(T_METAL, V3(tx + (k & 1) * 1.5f, top, tz + (k >> 1) * 1.5f), V3(tx + (k & 1) * 1.5f + 0.12f, top + 1.2f, tz + (k >> 1) * 1.5f + 0.12f), rgb(120, 120, 115), false, ALL);
            box(T_METAL, V3(tx - 0.2f, top + 1.2f, tz - 0.2f), V3(tx + 1.82f, top + 2.6f, tz + 1.82f), rgb(185, 190, 195), true, ALL); }
        if (fl & BD_DISH) { float dx = r.x + r.width - 2, dz = r.y + 1.5f; box(T_METAL, V3(dx, top, dz), V3(dx + 0.1f, top + 0.9f, dz + 0.1f), rgb(150, 150, 150), false, ALL); box(T_PLAIN, V3(dx - 0.4f, top + 0.8f, dz - 0.05f), V3(dx + 0.5f, top + 1.6f, dz + 0.05f), rgb(225, 225, 225), false, ALL); }
        if ((fl & BD_RFENCE) && floors > 1) for (int fc = 0; fc < 4; fc++) { float Lf = fLen(r, fc); for (float u = 0; u <= Lf; u += 2.0f) fBox(r, fc, u, u + 0.06f, top + 0.9f, top + 2.2f, -0.12f, -0.06f, T_METAL, rgb(120, 125, 120), false);
            fBox(r, fc, 0, Lf, top + 2.15f, top + 2.22f, -0.12f, -0.06f, T_METAL, rgb(120, 125, 120), false); }
        if ((fl & BD_BILL) && nSign < MAX_SIGN) { Color bc = (Color[]){{255, 80, 70, 255}, {80, 220, 255, 255}, {255, 210, 80, 255}, {255, 120, 220, 255}}[ri(4)];   // rooftop billboard
            float L2 = fminf(L - 1, 8); fBox(r, street, (L - L2) / 2, (L - L2) / 2 + 0.1f, top, top + 2.0f, -1.2f, -1.1f, T_METAL, rgb(100, 100, 100), false); fBox(r, street, (L + L2) / 2 - 0.1f, (L + L2) / 2, top, top + 2.0f, -1.2f, -1.1f, T_METAL, rgb(100, 100, 100), false);
            fBox(r, street, (L - L2) / 2, (L + L2) / 2, top + 2.0f, top + 4.2f, -1.25f, -1.05f, T_SIGNH, bc, false); }
    }
}
static void hipRoof(Rectangle r, float top, Color rc) {   // a low hipped tile roof with deep eaves, the commonest Odawara house roof
    float E = 0.65f, span = fminf(r.width, r.height), rise = fminf(span * 0.28f, 2.0f); int steps = 7;
    for (int k = 0; k < steps; k++) { float t0 = (float)k / steps, ins = t0 * (span / 2 + E) - E, h0 = top + t0 * rise, h1 = top + (k + 1.0f) / steps * rise;
        Color c = (Color){(unsigned char)(rc.r * (0.84f + 0.03f * k)), (unsigned char)(rc.g * (0.84f + 0.03f * k)), (unsigned char)(rc.b * (0.84f + 0.03f * k)), 255};
        if (r.width - 2 * ins < 0.3f || r.height - 2 * ins < 0.3f) break;
        box(T_ROOF, V3(r.x + ins, h0, r.y + ins), V3(r.x + r.width - ins, h1, r.y + r.height - ins), c, true, ALL); }
    box(T_ROOF, V3(r.x - E, top - 0.12f, r.y - E), V3(r.x + r.width + E, top, r.y + r.height + E), (Color){(unsigned char)(rc.r * 0.6f), (unsigned char)(rc.g * 0.6f), (unsigned char)(rc.b * 0.6f), 255}, false, ALL);   // the eave's dark underside
}
static void pitchedRoof(Rectangle r, float top, Color rc) {   // a tiled gable roof in steps (eaves overhang the walls)
    bool alongX = r.width >= r.height; float span = alongX ? r.height : r.width, rise = fminf(span * 0.35f, 2.4f); int steps = 6;
    for (int k = 0; k < steps; k++) { float t0 = (float)k / steps, ins = t0 * (span / 2 + 0.6f) - 0.6f, h0 = top + t0 * rise, h1 = top + (k + 1.0f) / steps * rise;
        Color c = (Color){(unsigned char)(rc.r * (0.85f + 0.03f * k)), (unsigned char)(rc.g * (0.85f + 0.03f * k)), (unsigned char)(rc.b * (0.85f + 0.03f * k)), 255};
        if (alongX) box(T_ROOF, V3(r.x - 0.5f, h0, r.y + ins), V3(r.x + r.width + 0.5f, h1, r.y + r.height - ins), c, true, ALL);
        else box(T_ROOF, V3(r.x + ins, h0, r.y - 0.5f), V3(r.x + r.width - ins, h1, r.y + r.height + 0.5f), c, true, ALL); }
}
// ---------------------------------------------------------------- Japanese floor plans for houses and flats
// Worked in (u, d): d runs from the street wall (0) to the back (D), u across (0..W). The stairwell lies either along the back
// (a corridor runs behind the rooms) or along one side (the stair hall runs the full depth there). Rooms get plaster partitions
// with 0.8 m doorways (lintel at 2.05 m) and are furnished by planRoom.
typedef struct { Rectangle in; int st; float W, D; } JpF;
static Rectangle jpRect(const JpF *J, float u0, float d0, float u1, float d1) {
    float ua = fminf(u0, u1), ub = fmaxf(u0, u1), da = fminf(d0, d1), db = fmaxf(d0, d1); Rectangle in = J->in;
    switch (J->st) { case 0: return (Rectangle){in.x + ua, in.y + da, ub - ua, db - da};
                     case 2: return (Rectangle){in.x + ua, in.y + in.height - db, ub - ua, db - da};
                     case 3: return (Rectangle){in.x + da, in.y + ua, db - da, ub - ua};
                     default: return (Rectangle){in.x + in.width - db, in.y + ua, db - da, ub - ua}; }
}
static int jpSide(const JpF *J, int which) {   // 0 front (street), 1 back, 2 the u=0 side, 3 the u=W side -> wall index
    int s = J->st; if (which == 0) return s; if (which == 1) return opp(s);
    bool uIsX = s == 0 || s == 2; return which == 2 ? (uIsX ? 3 : 0) : (uIsX ? 1 : 2);
}
static float jpY0, jpYT; static int jpPT = T_PLASTER; static Color jpPC;
static FILE *jpDump; static int jpDumpN = -1;   // KIRI_PLANDUMP=file: the plans of the first houses, for drawing
static void jpDumpRect(const char *tag, Rectangle q) { if (!jpDump) return; Vector3 c[4] = {xfP(curXf, V3(q.x, 0, q.y)), xfP(curXf, V3(q.x + q.width, 0, q.y)), xfP(curXf, V3(q.x + q.width, 0, q.y + q.height)), xfP(curXf, V3(q.x, 0, q.y + q.height))};
    fprintf(jpDump, "%s %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n", tag, c[0].x, c[0].z, c[1].x, c[1].z, c[2].x, c[2].z, c[3].x, c[3].z); }
static float furnDumpY;
static void furnDumpBox(const char *tag, int k, Vector3 mn, Vector3 mx, float yaw) { if (!furnDump) return; Vector3 c[4] = {xfP(curXf, V3(mn.x, 0, mn.z)), xfP(curXf, V3(mx.x, 0, mn.z)), xfP(curXf, V3(mx.x, 0, mx.z)), xfP(curXf, V3(mn.x, 0, mx.z))};
    Vector3 f0 = xfP(curXf, V3((mn.x + mx.x) / 2, 0, (mn.z + mx.z) / 2)), f1 = xfP(curXf, V3((mn.x + mx.x) / 2 + sinf(yaw) * 0.4f, 0, (mn.z + mx.z) / 2 + cosf(yaw) * 0.4f));
    fprintf(furnDump, "%s %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %d %.2f %.2f %.2f %.2f\n", tag, c[0].x, c[0].z, c[1].x, c[1].z, c[2].x, c[2].z, c[3].x, c[3].z, k, mn.y - furnDumpY, mx.y - furnDumpY, f1.x - f0.x, f1.z - f0.z); }
// Partitions are noted first and built together once the whole floor is laid out (jpBuildWalls): a doorway then cuts every wall drawn
// along its line (two rooms' walls on one line used to shut each other's doors), and a room that still has no way in gets one.
typedef struct { int ax; float pos, lo, hi; } JpWall;   // ax 0: at constant u, running lo..hi in d; ax 1: at constant d, running in u
static Vector2 gStNear; static bool gStNearLo = true; static float gStNu = -1, gStNd = -1;   // (0.9.13: the stairwell door's centre in the plan's own (u, d))   // 0.9.8: the stairwell's door landing (building frame), and whether it is at the low end in the plan's own (u, d)
typedef struct { int ax; float pos, at; int fix; } JpDoor; static float jpFixLo[2], jpFixHi[2];   // fix: the stairwell's doors (never moved or pruned)
static JpWall jpW[96]; static int jpNW; static JpDoor jpDr[48]; static int jpND; static int jpDoorsHand; static Vector3 handDoor[512]; static int nHandDoor;   // (the Mod Maker's doorway markers: handDoorsCollect)
static void jpWallU(const JpF *J, float u, float d0, float d1, float door) {   // a partition at constant u from d0 to d1, a doorway centred at d = door (<0: none)
    (void)J; if (d1 - d0 < 0.05f) return; if (jpNW < 96) jpW[jpNW++] = (JpWall){0, u, d0, d1}; if (door >= 0 && jpND < 48) jpDr[jpND++] = (JpDoor){0, u, door};
}
static void jpWallD(const JpF *J, float d, float u0, float u1, float door) {   // a partition at constant d from u0 to u1
    (void)J; if (u1 - u0 < 0.05f) return; if (jpNW < 96) jpW[jpNW++] = (JpWall){1, d, u0, u1}; if (door >= 0 && jpND < 48) jpDr[jpND++] = (JpDoor){1, d, door};
}
static int jpRooms; static Rectangle jpRoomR[16]; static int jpRoomP[16], jpRoomO[16]; static float jpRoomUD[16][4];
static void jpRoom(const JpF *J, float u0, float d0, float u1, float d1, int plan, int outerWhich) {
    if (jpRooms >= 12 || u1 - u0 < 0.7f || d1 - d0 < 0.7f) return;
    Rectangle r = jpRect(J, u0 + 0.05f, d0 + 0.05f, u1 - 0.05f, d1 - 0.05f); jpRoomR[jpRooms] = r; jpRoomP[jpRooms] = plan; jpRoomO[jpRooms] = jpSide(J, outerWhich);
    jpRoomUD[jpRooms][0] = fminf(u0, u1); jpRoomUD[jpRooms][1] = fminf(d0, d1); jpRoomUD[jpRooms][2] = fmaxf(u0, u1); jpRoomUD[jpRooms][3] = fmaxf(d0, d1); jpRooms++;
}
// ---- the floor as a grid (0.15 m): walls shut cells, doorways open them again; filled from where one comes in
#define JG_Q 0.1f
#define JG_MAX 320
static unsigned char jg[JG_MAX * JG_MAX]; static int jgW, jgD;
static int jgCell(float u, float d) { int i = (int)floorf(u / JG_Q), j = (int)floorf(d / JG_Q); if (i < 0 || j < 0 || i >= jgW || j >= jgD) return -1; return j * jgW + i; }
static bool jgFree(float u, float d) { int c = jgCell(u, d); return c >= 0 && jg[c] != 0; }
static bool jgReached(float u, float d) { int c = jgCell(u, d); return c >= 0 && jg[c] == 2; }
static int jpRoomAt(float u, float d) { for (int r = 0; r < jpRooms; r++) if (u > jpRoomUD[r][0] && u < jpRoomUD[r][2] && d > jpRoomUD[r][1] && d < jpRoomUD[r][3]) return r; return -1; }
static bool jpDoorCut(int ax, float pos, float at) { for (int k = 0; k < jpND; k++) if (jpDr[k].ax == ax && fabsf(jpDr[k].pos - pos) < 0.04f && fabsf(jpDr[k].at - at) < 0.36f) return true; return false; }
static float jgAu, jgAd; static bool jgAok;   // where the stairs come up, in (u, d)
static float jgSu0, jgSu1, jgSd0, jgSd1;
static void jgFlood(const JpF *J, int f, float doorU, bool hasStairs, float su0, float su1, float sd0, float sd1) {
    jgSu0 = su0; jgSu1 = su1; jgSd0 = sd0; jgSd1 = sd1;
    jgW = (int)ceilf(J->W / JG_Q); jgD = (int)ceilf(J->D / JG_Q); if (jgW > JG_MAX) jgW = JG_MAX; if (jgD > JG_MAX) jgD = JG_MAX; memset(jg, 1, (size_t)jgW * jgD);
    for (int k = 0; k < jpNW; k++) { JpWall *q = &jpW[k]; for (float t = q->lo; t <= q->hi; t += JG_Q * 0.5f) { if (jpDoorCut(q->ax, q->pos, t)) continue;
        { int i0 = (int)floorf(q->pos / JG_Q - 1.0f); for (int ii = i0; ii <= i0 + 2; ii++) { float cc = (ii + 0.5f) * JG_Q; if (fabsf(cc - q->pos) > 0.101f) continue; int c = q->ax == 0 ? jgCell(cc, t) : jgCell(t, cc); if (c >= 0) jg[c] = 0; } } } }   // every cell whose middle is within 0.1 m of the wall line (the old test lost whole walls to rounding)
    if (hasStairs) { float a0 = su0, a1 = su1, b0 = sd0, b1 = sd1, ld = fmaxf(0.8f, (fmaxf(su1 - su0, sd1 - sd0) - 3.6f) / 2) - 0.05f; if (su1 - su0 >= sd1 - sd0) { if (gStNearLo) a0 += ld; else a1 -= ld; b0 += 0.15f; b1 -= 0.15f; } else { if (gStNearLo) b0 += ld; else b1 -= ld; a0 += 0.15f; a1 -= 0.15f; }   // 0.9.8: only the door landing is floor; the far end is the half landing   // (the landings: as long as the building's own)   // the flights and the opening: not floor (the landings at both ends are)
        for (float u = a0; u < a1; u += JG_Q * 0.5f) for (float d = b0; d < b1; d += JG_Q * 0.5f) { int c = jgCell(u, d); if (c >= 0) jg[c] = 0; } }
    {   // a person is 0.6 m wide: every cell within 0.25 m of something solid (or of the outer wall) is no place to stand
        static unsigned char t[JG_MAX * JG_MAX]; memcpy(t, jg, (size_t)jgW * jgD);
        for (int j = 0; j < jgD; j++) for (int i = 0; i < jgW; i++) { if (!t[j * jgW + i]) continue; bool bl = i < 2 || j < 2 || i >= jgW - 2 || j >= jgD - 2;
            for (int b = -2; b <= 2 && !bl; b++) for (int a = -2; a <= 2 && !bl; a++) if (!t[(j + b) * jgW + i + a]) bl = true;
            if (bl) jg[j * jgW + i] = 0; } }
    static int st[JG_MAX * JG_MAX]; int n = 0;
    #define JG_SEED(u_, d_) do { int c_ = jgCell(u_, d_); if (c_ >= 0 && jg[c_] == 1) { jg[c_] = 2; st[n++] = c_; } } while (0)
    if (f == 0) { for (float dd = 0.25f; dd < 1.3f && n == 0; dd += 0.15f) for (float du = 0; du < 0.6f && n == 0; du += 0.15f) { JG_SEED(doorU + du, dd); JG_SEED(doorU - du, dd); } }
    if (f > 0 && jgAok) for (float du = -0.6f; du <= 0.61f; du += JG_Q) for (float dd = -0.6f; dd <= 0.61f; dd += JG_Q) JG_SEED(jgAu + du, jgAd + dd);
    if (n == 0) for (int r = 0; r < jpRooms; r++) if (jpRoomP[r] == RM_HALL) for (float u = jpRoomUD[r][0] + 0.2f; u < jpRoomUD[r][2] - 0.15f; u += JG_Q) for (float d = jpRoomUD[r][1] + 0.2f; d < jpRoomUD[r][3] - 0.15f; d += JG_Q) JG_SEED(u, d);
    #undef JG_SEED
    while (n > 0) { int c = st[--n], i = c % jgW, j = c / jgW;
        if (i + 1 < jgW && jg[c + 1] == 1) { jg[c + 1] = 2; st[n++] = c + 1; } if (i > 0 && jg[c - 1] == 1) { jg[c - 1] = 2; st[n++] = c - 1; }
        if (j + 1 < jgD && jg[c + jgW] == 1) { jg[c + jgW] = 2; st[n++] = c + jgW; } if (j > 0 && jg[c - jgW] == 1) { jg[c - jgW] = 2; st[n++] = c - jgW; } }
}
static bool jpRoomReached(int r) { bool any = false; for (float u = jpRoomUD[r][0] + 0.25f; u < jpRoomUD[r][2] - 0.2f; u += JG_Q) for (float d = jpRoomUD[r][1] + 0.25f; d < jpRoomUD[r][3] - 0.2f; d += JG_Q) { if (jgReached(u, d)) return true; if (jgFree(u, d)) any = true; } return !any; }   // (a room that is all stairwell has nothing to reach)
static int jpDoorsAdded = 0, jpRoomsShut = 0, jpDoorsPruned = 0; static char jbTrace[2000];
// an inner wall that meets the outside wall where a window is: the window is walled up (outside finish outside, plaster inside)
static int doorLate[20000], nDoorLate;
static void innerDoor(bool alongX, float u0, float u1, float c, float y0, bool open) {   // 0.9.8: a hinged leaf in an inner doorway (alongX: the wall runs along x at z = c)
    if (nOps >= MAX_OPEN) return; Open *o = &ops[nOps]; memset(o, 0, sizeof *o); opInner[nOps] = 1;
    if (alongX) { o->mn = V3(u0, y0, c - 0.03f); o->mx = V3(u1, y0 + 2.1f, c + 0.03f); o->axis = 1; } else { o->mn = V3(c - 0.03f, y0, u0); o->mx = V3(c + 0.03f, y0 + 2.1f, u1); o->axis = 0; }
    o->type = OP_DOOR; o->side = 1; o->state = open ? 0 : 1; o->bsid = -1; o->hp = 4; o->fy = y0; o->f = (short)curXf; o->locked = false; o->swing = open ? 1 : 0; o->sid = addSolid(o->mn, o->mx); if (o->sid >= 0) sol[o->sid].off = true; if (!open && nDoorLate < 20000) doorLate[nDoorLate++] = o->sid; nOps++;   // (shut ones turn solid once the town is built: furniture is placed as if open)
}
static void jpWinFill(const JpF *J, float u, float d) {
    Rectangle q = jpRect(J, u - 0.01f, d - 0.01f, u + 0.01f, d + 0.01f); float px = q.x + q.width / 2, pz = q.y + q.height / 2;
    float cx = J->in.x + J->in.width / 2, cz = J->in.y + J->in.height / 2;
    for (int i = 0; i < nOps; i++) { Open *o = &ops[i]; if (o->type != OP_WINDOW || o->f != curXf || o->mn.y < jpY0 - 0.2f || o->mn.y > jpY0 + 2.4f) continue;
        bool thinX = o->mx.x - o->mn.x < o->mx.z - o->mn.z;
        float a0 = thinX ? o->mn.z : o->mn.x, a1 = thinX ? o->mx.z : o->mx.x, along = thinX ? pz : px, across = thinX ? px : pz, wpos = thinX ? (o->mn.x + o->mx.x) / 2 : (o->mn.z + o->mx.z) / 2;
        if (along < a0 - 0.12f || along > a1 + 0.12f || fabsf(across - wpos) > 0.45f) continue;
        float out = thinX ? (wpos > cx ? 1 : -1) : (wpos > cz ? 1 : -1);
        Vector3 mn = o->mn, mx = o->mx; if (thinX) { mn.x = wpos - 0.1f; mx.x = wpos + 0.1f; } else { mn.z = wpos - 0.1f; mx.z = wpos + 0.1f; }
        mn.y -= 0.01f; mx.y += 0.01f;   // (a hair over the sill and lintel: no seam)
        {   // 0.9.8: only the part of the window on the inner wall's side is walled up; the larger part stays a (smaller) window
            float keepLo = a0, keepHi = along - 0.25f, fLo = along - 0.25f, fHi = a1 + 0.01f;
            if (a1 - along > along - a0) { keepLo = along + 0.25f; keepHi = a1; fLo = a0 - 0.01f; fHi = along + 0.25f; }
            if (keepHi - keepLo >= 0.5f) {
                if (thinX) { mn.z = fLo; mx.z = fHi; } else { mn.x = fLo; mx.x = fHi; }
                Vector3 omn = mn, omx = mx, imn = mn, imx = mx;
                if (thinX) { if (out > 0) { omn.x = wpos; imx.x = wpos; } else { omx.x = wpos; imn.x = wpos; } } else { if (out > 0) { omn.z = wpos; imx.z = wpos; } else { omx.z = wpos; imn.z = wpos; } }
                box(gExtTex, omn, omx, gExtCol, true, ALL); box(T_PLASTER, imn, imx, (Color){236, 232, 222, 255}, true, ALL);
                if (thinX) { o->mn.z = keepLo; o->mx.z = keepHi; } else { o->mn.x = keepLo; o->mx.x = keepHi; }
                jpWinFilled++; continue; } }
        if (thinX) { mn.z -= 0.01f; mx.z += 0.01f; } else { mn.x -= 0.01f; mx.x += 0.01f; }
        Vector3 omn = mn, omx = mx, imn = mn, imx = mx;
        if (thinX) { if (out > 0) { omn.x = wpos; imx.x = wpos; } else { omx.x = wpos; imn.x = wpos; } } else { if (out > 0) { omn.z = wpos; imx.z = wpos; } else { omx.z = wpos; imn.z = wpos; } }
        box(gExtTex, omn, omx, gExtCol, true, ALL); box(T_PLASTER, imn, imx, (Color){236, 232, 222, 255}, true, ALL);
        o->type = 2; jpWinFilled++; }   // (no longer a window: no glass, nothing comes through it)
}
static void jpBuildWalls(const JpF *J, int f, float doorU, bool hasStairs, float su0, float su1, float sd0, float sd1) {
    if (hasStairs && jpNW < 94 && jpND < 46) {   // 0.9.7: the stairwell is walled off from the rooms: a wall along its room side (a doorway at each landing) and one across its inner end
        bool alongU = su1 - su0 >= sd1 - sd0; float L0 = alongU ? su0 : sd0, L1 = alongU ? su1 : sd1, C0 = alongU ? sd0 : su0, C1 = alongU ? sd1 : su1, Lmax = alongU ? J->W : J->D;
        float lead = fmaxf(0.8f, ((L1 - L0) - 3.6f) / 2), roomC = C0 < 0.3f ? C1 : C0, g0 = L0 + lead / 2, g1 = L1 - lead / 2;
        int axL = alongU ? 1 : 0;   // (ax 0: a wall at constant u, running along d)
        {   // 0.9.11: the wall along the flights and the half landing (the door landing stays open to the plan; the stairwell wall and its door are built with the stairs).
            // A room wall alongside the door landing gets a doorway opposite the stairwell door, and a toilet or bath there becomes a little lobby
            float gc = gStNearLo ? L0 + lead / 2 : L1 - lead / 2;
            { float g2 = alongU ? gStNu : gStNd; if (getenv("KIRI_GCDBG")) { static int ng = 0; if (ng++ < 40) TraceLog(LOG_INFO, "GCDBG f %d lo %d L %.2f..%.2f lead %.2f gc %.2f g2 %.2f nu %.2f nd %.2f alongU %d", f, gStNearLo, L0, L1, lead, gc, g2, gStNu, gStNd, alongU); }
              if (g2 > L0 - 0.5f && g2 < L1 + 0.5f) gc = g2; }   // 0.9.13: exactly where the stairwell's own door is
            bool hiBand = roomC == C0; { for (int k = 0; k < jpNW; k++) if (jpW[k].ax == axL && fabsf(jpW[k].pos - roomC) < 0.25f && (hiBand ? jpW[k].pos <= roomC + 0.01f : jpW[k].pos >= roomC - 0.01f)) { roomC = jpW[k].pos; break; } }   // (snapped only away from the shaft: never into the flights)
            jpW[jpNW++] = gStNearLo ? (JpWall){axL, roomC, L0 + lead, L1} : (JpWall){axL, roomC, L0, L1 - lead};
            for (int k = 0; k < jpNW - 1; k++) { JpWall *q = &jpW[k]; if (q->ax != axL || fabsf(q->pos - roomC) > 0.6f || q->lo > gc - 0.3f || q->hi < gc + 0.3f) continue;
                // 0.9.13: an open doorway (no door in it) straight across from the stairwell door: one door, not two back to back
                for (int m = jpND - 1; m >= 0; m--) if (jpDr[m].ax == axL && fabsf(jpDr[m].pos - q->pos) < 0.05f && fabsf(jpDr[m].at - gc) < 0.9f) jpDr[m] = jpDr[--jpND];
                if (jpND < 48) jpDr[jpND++] = (JpDoor){axL, q->pos, gc, 3}; }
            float pr = hiBand ? roomC - 0.3f : roomC + 0.3f; int r = alongU ? jpRoomAt(gc, pr) : jpRoomAt(pr, gc);
            if (r >= 0 && (jpRoomP[r] == RM_TOILET || jpRoomP[r] == RM_BATH)) jpRoomP[r] = RM_HALL; }
        (void)Lmax;
        jpFixLo[0] = L0 + 0.42f; jpFixHi[0] = fmaxf(L0 + 0.42f, L0 + lead - 0.1f); jpFixLo[1] = fminf(L1 - 0.42f, L1 - lead + 0.1f); jpFixHi[1] = L1 - 0.42f;
        (void)g0; (void)g1; }
    // 0.9.11: a toilet or a bath is closed on every side: any stretch of its edge with no wall gets one (the doors are found below like any room's)
    for (int r = 0; r < jpRooms; r++) if (jpRoomP[r] == RM_TOILET || jpRoomP[r] == RM_BATH) {
        float *q = jpRoomUD[r];
        for (int side = 0; side < 4; side++) { int ax = side < 2 ? 0 : 1; float pos = side == 0 ? q[0] : side == 1 ? q[2] : side == 2 ? q[1] : q[3], lo = ax == 0 ? q[1] : q[0], hi = ax == 0 ? q[3] : q[2];
            if (pos < 0.08f || (ax == 0 ? pos > J->W - 0.08f : pos > J->D - 0.08f)) continue;   // (the outer wall)
            float cov[24][2]; int nc = 0;
            for (int k = 0; k < jpNW && nc < 24; k++) if (jpW[k].ax == ax && fabsf(jpW[k].pos - pos) < 0.12f && jpW[k].hi > lo && jpW[k].lo < hi) { cov[nc][0] = jpW[k].lo; cov[nc][1] = jpW[k].hi; nc++; }
            float t = lo;
            while (t < hi - 0.1f && jpNW < 96) { float nxt = hi; bool inside = false;
                for (int c = 0; c < nc; c++) { if (cov[c][0] <= t + 0.01f && cov[c][1] > t + 0.01f) { inside = true; t = cov[c][1]; break; } if (cov[c][0] > t && cov[c][0] < nxt) nxt = cov[c][0]; }
                if (inside) continue;
                if (nxt - t > 0.1f) jpW[jpNW++] = (JpWall){ax, pos, t, nxt};
                t = nxt; } } }
    int roomsReal = jpRooms;   // the two landings count as rooms while the doors are worked out: each must be reachable (the next flight starts at the other one)
    if (false && hasStairs && jpRooms <= 14) { bool alongU = su1 - su0 >= sd1 - sd0; float L0 = alongU ? su0 : sd0, L1 = alongU ? su1 : sd1, lead = fmaxf(0.8f, ((L1 - L0) - 3.6f) / 2);
        for (int e = 0; e < 2; e++) { float l0 = e ? L1 - lead : L0, l1 = e ? L1 : L0 + lead; int r = jpRooms++; jpRoomP[r] = RM_HALL;
            if (alongU) { jpRoomUD[r][0] = l0; jpRoomUD[r][2] = l1; jpRoomUD[r][1] = sd0; jpRoomUD[r][3] = sd1; } else { jpRoomUD[r][1] = l0; jpRoomUD[r][3] = l1; jpRoomUD[r][0] = su0; jpRoomUD[r][2] = su1; } } }
    // 1. every room gets a way in: through the wall it shares with the hall if it can, else with a living room; the bath from the washroom
    {   // a doorway keeps clear of the walls that meet its wall (a door 0.2 m from a corner opens onto the end of a partition): moved along its stretch, or dropped (one is added again below if a room needs it)
        for (int k = jpND - 1; k >= 0; k--) { JpDoor *d = &jpDr[k]; float lo = -1e9f, hi = 1e9f, segLo = 1e9f, segHi = -1e9f;
            for (int m = 0; m < jpNW; m++) { JpWall *q = &jpW[m];
                if (q->ax == d->ax && fabsf(q->pos - d->pos) < 0.04f) { if (q->lo < d->at + 0.4f && q->hi > d->at - 0.4f) { segLo = fminf(segLo, q->lo); segHi = fmaxf(segHi, q->hi); } continue; }
                if (q->ax != d->ax && d->pos > q->lo - 0.06f && d->pos < q->hi + 0.06f) { if (q->pos <= d->at && q->pos > lo) lo = q->pos; if (q->pos > d->at && q->pos < hi) hi = q->pos; } }
            if (segLo > segHi) continue;
            lo = fmaxf(lo, segLo); hi = fminf(hi, segHi); float a0 = lo + 0.5f, a1 = hi - 0.5f;   // centres 0.5 m from a wall meeting it or the end of its own wall
            if (d->fix == 3) continue;   // (the archway opposite the stairwell door stays where that door is)
            if (d->fix) { float f0 = fmaxf(a0, jpFixLo[d->fix - 1]), f1 = fminf(a1, jpFixHi[d->fix - 1]); if (f1 < f0) { jpDr[k] = jpDr[--jpND]; continue; } d->at = clampf(d->at, f0, f1); continue; }   // (a stairwell door with no clean place on its landing goes: one is found below)
            if (a1 < a0) { if (hi - lo > 0.85f) d->at = (lo + hi) / 2; continue; }
            d->at = clampf(d->at, a0, a1); } }   // (a stairwell door stays on its landing)
    bool gave[16] = {0}; int addedSince = 0; jbTrace[0] = 0;
    for (int pass = 0; pass < 40; pass++) {
        jgFlood(J, f, doorU, hasStairs, su0, su1, sd0, sd1); int R = -1;
        for (int r = 0; r < jpRooms && R < 0; r++) if (jpRoomP[r] != RM_BATH && !jpRoomReached(r)) R = r;
        for (int r = 0; r < jpRooms && R < 0; r++) if (!jpRoomReached(r)) R = r;
        if (R < 0) break;
        if (R >= 0 && gave[R]) { R = -1; for (int r = 0; r < jpRooms && R < 0; r++) if (!gave[r] && !jpRoomReached(r)) R = r;
            if (R < 0) { if (!addedSince) break; memset(gave, 0, sizeof gave); addedSince = 0; for (int r = 0; r < jpRooms && R < 0; r++) if (!jpRoomReached(r)) R = r; if (R < 0) break; } }   // every room left was tried: once a door has been added since, they get another go
        int bestSc = 0, bestAx = 0; float bestPos = 0, bestAt = 0, bestLen = 0;
        for (int k = 0; k < jpNW; k++) { JpWall *q = &jpW[k]; float run0 = 0, runSc = -1;
            for (float t = q->lo + 0.5f; ; t += 0.15f) { int sc = 0;
                if (t <= q->hi - 0.5f) for (int sd2 = -1; sd2 <= 1 && !sc; sd2 += 2) {   // the room on one side, ground already reached on the other, 0.7 m of plain wall between
                    float pin = q->pos + sd2 * 0.42f, pout = q->pos - sd2 * 0.42f; bool ok = true;
                    for (float e = -0.1f; e <= 0.11f && ok; e += 0.1f) { float ui = q->ax == 0 ? pin : t + e, di = q->ax == 0 ? t + e : pin, uo = q->ax == 0 ? pout : t + e, dq = q->ax == 0 ? t + e : pout;
                        if (jpRoomAt(ui, di) != R || !jgFree(ui, di) || !jgReached(uo, dq)) ok = false; }
                    if (!ok) continue;
                    int o = jpRoomAt(q->ax == 0 ? pout : t, q->ax == 0 ? t : pout), op = o >= 0 ? jpRoomP[o] : RM_HALL;
                    sc = op == RM_HALL ? 10 : (op == RM_SENMEN ? (jpRoomP[R] == RM_BATH ? 12 : 4) : (op == RM_TOILET || op == RM_BATH) ? 1 : (op == RM_NANDO ? 3 : (op == RM_WASHITSU || op == RM_WBED || op == RM_BED) ? 5 : 8));
                    if (jpRoomP[R] == RM_BATH && op != RM_SENMEN && sc > 6) sc = 6; }   // (a toilet or a bath as the way through only when there is no other wall at all)
                if (sc != runSc || t > q->hi - 0.5f) { if (runSc > 0) { float len = t - 0.15f - run0; if (runSc > bestSc || (runSc == bestSc && len > bestLen)) { bestSc = (int)runSc; bestLen = len; bestAx = q->ax; bestPos = q->pos; bestAt = (run0 + t - 0.15f) / 2; } } run0 = t; runSc = (float)sc; }
                if (t > q->hi - 0.5f) break; } }
        if (strlen(jbTrace) < 1900) { char t[100]; snprintf(t, 100, "pass %d R %d plan %d best %d ax %d pos %.2f at %.2f\n", pass, R, jpRoomP[R], bestSc, bestAx, bestPos, bestAt); strcat(jbTrace, t); }
        if (bestSc <= 0 || jpND >= 48) { gave[R] = true; continue; }   // (it may open up once a neighbour has its door)
        jpDr[jpND++] = (JpDoor){bestAx, bestPos, bestAt}; jpDoorsAdded++; addedSince++;
    }
    {   // doorways that lead nowhere go: onto the flights of stairs, into a gap no one can stand in, or between two parts of one room - unless a room would be shut off without it
        bool reach0[16]; jgFlood(J, f, doorU, hasStairs, su0, su1, sd0, sd1); for (int r = 0; r < jpRooms; r++) reach0[r] = jpRoomReached(r);
        for (int k = jpND - 1; k >= 0; k--) { JpDoor d = jpDr[k]; if (d.fix) continue; bool bad = false; int ra = -1, rb = -1;
            for (int sd2 = -1; sd2 <= 1; sd2 += 2) { bool sideOk = false; float pp = d.pos + sd2 * 0.45f;
                for (float e = -0.15f; e <= 0.16f && !sideOk; e += 0.15f) { float u = d.ax == 0 ? pp : d.at + e, dd = d.ax == 0 ? d.at + e : pp;
                    bool flight = false; if (hasStairs) { float a0 = su0, a1 = su1, b0 = sd0, b1 = sd1, ld = fmaxf(0.8f, (fmaxf(su1 - su0, sd1 - sd0) - 3.6f) / 2) - 0.05f; if (su1 - su0 >= sd1 - sd0) { if (gStNearLo) a0 += ld; else a1 -= ld; } else { if (gStNearLo) b0 += ld; else b1 -= ld; } flight = u > a0 && u < a1 && dd > b0 && dd < b1; }   // onto the flights themselves
                    if (u > 0.05f && dd > 0.05f && u < J->W - 0.05f && dd < J->D - 0.05f && !flight) sideOk = true; }
                if (!sideOk) bad = true; int rr = jpRoomAt(d.ax == 0 ? pp : d.at, d.ax == 0 ? d.at : pp); if (sd2 < 0) ra = rr; else rb = rr; }
            if (!bad && ra == rb && ra >= 0) bad = true;
            if (!bad) continue;
            jpDr[k] = jpDr[--jpND]; jgFlood(J, f, doorU, hasStairs, su0, su1, sd0, sd1); bool lost = false; for (int r = 0; r < jpRooms; r++) if (reach0[r] && !jpRoomReached(r)) lost = true;
            if (lost) { jpDr[jpND++] = jpDr[k]; jpDr[k] = d; } else jpDoorsPruned++; } }
    for (int i = 0; i < nHandDoor; i++) { Vector3 hp = handDoor[i]; if (hp.y < jpY0 - 0.7f || hp.y > jpY0 + 1.8f) continue;   // doorways placed by hand (Mod Maker)
        Vector3 L = xfI(curXf, hp); Rectangle in = J->in; float u, d;
        switch (J->st) { case 0: u = L.x - in.x; d = L.z - in.y; break; case 2: u = L.x - in.x; d = in.y + in.height - L.z; break; case 3: u = L.z - in.y; d = L.x - in.x; break; default: u = L.z - in.y; d = in.x + in.width - L.x; }
        if (u < -0.5f || d < -0.5f || u > J->W + 0.5f || d > J->D + 0.5f) continue;
        int bw = -1; float bd = 1.0f; for (int k = 0; k < jpNW; k++) { JpWall *w = &jpW[k]; float along = w->ax == 0 ? d : u, across = w->ax == 0 ? u : d; if (along < w->lo - 0.1f || along > w->hi + 0.1f) continue; float dd = fabsf(across - w->pos); if (dd < bd) { bd = dd; bw = k; } }
        if (bw < 0) continue; JpWall *w = &jpW[bw]; float at = clampf(w->ax == 0 ? d : u, w->lo + 0.45f, w->hi - 0.45f); if (w->hi - w->lo < 0.9f) at = (w->lo + w->hi) / 2;
        bool have = false; for (int k = 0; k < jpND; k++) if (jpDr[k].ax == w->ax && fabsf(jpDr[k].pos - w->pos) < 0.05f && fabsf(jpDr[k].at - at) < 0.9f) have = true;
        if (!have && jpND < 48) { jpDr[jpND++] = (JpDoor){w->ax, w->pos, at}; jpDoorsHand++; } }
    jgFlood(J, f, doorU, hasStairs, su0, su1, sd0, sd1); for (int r = 0; r < jpRooms; r++) if (!jpRoomReached(r)) jpRoomsShut++;
    if (getenv("KIRI_RADBG")) { char b[300] = ""; for (int r = 0; r < jpRooms; r++) { char t[24]; snprintf(t, 24, " p%d:%d", jpRoomP[r], jpRoomReached(r)); strcat(b, t); } int fr2 = 0, rc = 0; for (int k = 0; k < jgW * jgD; k++) { fr2 += jg[k] != 0; rc += jg[k] == 2; }
        { char wb[1200] = ""; for (int k = 0; k < jpNW && strlen(wb) < 1100; k++) { char t[48]; snprintf(t, 48, " [%d %.2f %.2f..%.2f]", jpW[k].ax, jpW[k].pos, jpW[k].lo, jpW[k].hi); strcat(wb, t); } for (int k = 0; k < jpND && strlen(wb) < 1150; k++) { char t[40]; snprintf(t, 40, " D[%d %.2f @%.2f%s]", jpDr[k].ax, jpDr[k].pos, jpDr[k].at, jpDr[k].fix ? "F" : ""); strcat(wb, t); } TraceLog(LOG_INFO, "JGWALLS f%d%s", f, wb); }
        TraceLog(LOG_INFO, "JGDBG f%d in %.1f,%.1f W %.1f D %.1f st u %.2f..%.2f d %.2f..%.2f arr %.2f,%.2f ok %d doors %d free %d reached %d rooms%s", f, J->in.x, J->in.y, J->W, J->D, su0, su1, sd0, sd1, jgAu, jgAd, jgAok, jpND, fr2, rc, b); }
    jpRooms = roomsReal;
    // 2. build: along each wall line the stretches covered by any wall, minus every doorway on that line
    bool used[96] = {0};
    for (int k = 0; k < jpNW; k++) { if (used[k]) continue; int ax = jpW[k].ax; float pos = jpW[k].pos; float iv[96][2]; int ni = 0;
        for (int m = k; m < jpNW; m++) if (!used[m] && jpW[m].ax == ax && fabsf(jpW[m].pos - pos) < 0.04f) { used[m] = true; iv[ni][0] = jpW[m].lo; iv[ni][1] = jpW[m].hi; ni++; }
        for (int a = 0; a < ni; a++) for (int b = a + 1; b < ni; b++) if (iv[b][0] < iv[a][0]) { float t0 = iv[a][0], t1 = iv[a][1]; iv[a][0] = iv[b][0]; iv[a][1] = iv[b][1]; iv[b][0] = t0; iv[b][1] = t1; }
        for (int a = 0; a < ni;) { float lo = iv[a][0], hi = iv[a][1]; int b = a + 1; while (b < ni && iv[b][0] <= hi + 0.02f) { hi = fmaxf(hi, iv[b][1]); b++; } a = b;
            { float Lmax = ax == 0 ? J->D : J->W; if (lo < 0.25f) { if (ax == 0) jpWinFill(J, pos, lo); else jpWinFill(J, lo, pos); } if (hi > Lmax - 0.25f) { if (ax == 0) jpWinFill(J, pos, hi); else jpWinFill(J, hi, pos); } }   // (ends at the outside wall)
            float dl[24]; bool arch[24]; int nd = 0; for (int m = 0; m < jpND && nd < 24; m++) if (jpDr[m].ax == ax && fabsf(jpDr[m].pos - pos) < 0.04f && jpDr[m].at > lo - 0.3f && jpDr[m].at < hi + 0.3f) { int dup = -1; for (int e = 0; e < nd; e++) if (fabsf(dl[e] - jpDr[m].at) < 0.5f) dup = e; if (dup < 0) { arch[nd] = jpDr[m].fix == 3; dl[nd++] = jpDr[m].at; } else if (jpDr[m].fix == 3) arch[dup] = true; }
            for (int x = 0; x < nd; x++) for (int y = x + 1; y < nd; y++) if (dl[y] < dl[x]) { float t = dl[x]; dl[x] = dl[y]; dl[y] = t; bool ta = arch[x]; arch[x] = arch[y]; arch[y] = ta; }
            float cur = lo;
            for (int e = 0; e <= nd; e++) { float end = e < nd ? fmaxf(dl[e] - 0.4f, lo) : hi;
                if (end - cur > 0.02f) { Rectangle q = ax == 0 ? jpRect(J, pos - 0.05f, cur, pos + 0.05f, end) : jpRect(J, cur, pos - 0.05f, end, pos + 0.05f); box(jpPT, V3(q.x, jpY0, q.y), V3(q.x + q.width, jpYT, q.y + q.height), jpPC, true, ALL); jpDumpRect("W", q); }
                if (e < nd) { float g0 = fmaxf(dl[e] - 0.4f, lo), g1 = fminf(dl[e] + 0.4f, hi); if (g1 - g0 > 0.3f) {
                        Rectangle l = ax == 0 ? jpRect(J, pos - 0.05f, g0, pos + 0.05f, g1) : jpRect(J, g0, pos - 0.05f, g1, pos + 0.05f); box(jpPT, V3(l.x, jpY0 + 2.05f, l.y), V3(l.x + l.width, jpYT, l.y + l.height), jpPC, false, ALLB); jpDumpRect("D", l);
                        if (g1 - g0 > 0.6f && !arch[e] && !getenv("KIRI_NOHD")) { bool ax2 = l.width > l.height; innerDoor(ax2, ax2 ? l.x : l.y, ax2 ? l.x + l.width : l.y + l.height, ax2 ? l.y + l.height / 2 : l.x + l.width / 2, jpY0, fr(0, 1) < 0.4f); }   // 0.9.8: a door in it, shut more often than not
                        if (ax == 0) keepAdd(jpRect(J, pos - 1.0f, dl[e] - 0.75f, pos + 1.0f, dl[e] + 0.75f), jpRect(J, pos - 0.42f, dl[e] - 0.4f, pos + 0.42f, dl[e] + 0.4f));
                        else keepAdd(jpRect(J, dl[e] - 0.75f, pos - 1.0f, dl[e] + 0.75f, pos + 1.0f), jpRect(J, dl[e] - 0.4f, pos - 0.42f, dl[e] + 0.4f, pos + 0.42f)); }
                    cur = fminf(dl[e] + 0.4f, hi); } } } }
}
// one zone beside the hall (u from a to b, d from 0 to Dr), its doors on the wall at u = hallU. living: the bigger side.
static void jpZone(const JpF *J, float a, float b, float Dr, float hallU, bool living, int f, int kind, bool top) {
    float w = b - a; int sideW = a < 0.01f ? 2 : 3;   // its outer side wall
    if (w < 1.2f) { jpRoom(J, a, 0, b, Dr, RM_NANDO, sideW); return; }
    if (f == 0 || kind == 1) {
        if (living) {
            if (Dr >= 6.4f && w >= 2.6f) { float sp = Dr * 0.58f; jpWallD(J, sp, a, b, (a + b) / 2);   // the LDK in front, the tatami room behind, a fusuma between
                jpWallU(J, hallU, 0, sp, sp - 0.9f); jpWallU(J, hallU, sp, Dr, Dr - 0.9f);
                jpRoom(J, a, 0, b, sp, RM_LDK, 0); jpRoom(J, a, sp, b, Dr, kind == 1 && f % 2 ? RM_WBED : RM_WASHITSU, 1); }
            else { jpWallU(J, hallU, 0, Dr, Dr * 0.5f); jpRoom(J, a, 0, b, Dr, RM_LDK, 0); }
            if (!top) jpWallD(J, Dr, a, b, -1);   // the corridor behind
        } else {   // the wet side: the toilet by the entrance, then the washroom and the bath off it
            float wet = w >= 3.4f ? 1.8f : w;   // a wide side keeps a room along its outer wall
            float wa = sideW == 2 ? b - wet : a, wb = sideW == 2 ? b : a + wet;   // the wet strip lies against the hall
            if (sideW == 3) { wa = a; wb = a + wet; } else { wa = b - wet; wb = b; }
            float dT = fminf(1.7f, Dr * 0.3f), dS = dT + 1.7f, dB = fminf(dS + 1.7f, Dr);
            if (Dr < 4.6f) { dT = 1.5f; dS = Dr; dB = Dr; }   // short: toilet and washroom only
            jpWallD(J, dT, wa, wb, -1); jpWallU(J, hallU, 0, dT, dT * 0.5f); jpRoom(J, wa, 0, wb, dT, RM_TOILET, 0);
            jpWallU(J, hallU, dT, dS, (dT + dS) / 2); jpRoom(J, wa, dT, wb, dS, RM_SENMEN, sideW);
            if (dB > dS + 1.0f) { jpWallD(J, dS, wa, wb, (wa + wb) / 2); jpWallU(J, hallU, dS, dB, -1); jpRoom(J, wa, dS, wb, dB, RM_BATH, sideW);
                if (Dr - dB > 1.2f) { jpWallD(J, dB, wa, wb, -1); jpWallU(J, hallU, dB, Dr, (dB + Dr) / 2); jpRoom(J, wa, dB, wb, Dr, RM_NANDO, sideW); } else if (Dr - dB > 0.1f) jpWallU(J, hallU, dB, Dr, -1); }
            else if (Dr - dS > 0.1f) jpWallU(J, hallU, dS, Dr, -1);
            if (wet < w) { float oa = sideW == 3 ? wb : a, ob = sideW == 3 ? b : wa, ou = sideW == 3 ? wb : wa;   // the room along the outer wall, entered from the back
                jpWallU(J, ou, 0, Dr, -1); jpRoom(J, oa, 0, ob, Dr, kind == 1 ? RM_WBED : RM_WASHITSU, sideW); if (!top) jpWallD(J, Dr, oa, ob, (oa + ob) / 2); }
            if (!top) jpWallD(J, Dr, wa, wb, -1);
        }
    } else {   // upstairs in a house: bedrooms, one of them tatami; a toilet on the hall side
        int n = w >= 6.0f ? 2 : 1; float step = w / n;
        for (int k = 0; k < n; k++) { float ua = a + k * step, ub = ua + step; if (k > 0) jpWallU(J, ua, 0, Dr, -1);
            int pl = (k == 0 && living) ? RM_WASHITSU : RM_WBED;
            if (!living && k == 0 && ub - ua >= 2.0f && Dr >= 4.0f) { float tw = 1.0f, ta = sideW == 3 ? ua : ub - tw;   // the upstairs toilet beside the hall
                if (sideW == 3) { jpWallU(J, ua + tw, 0, 1.7f, -1); jpWallD(J, 1.7f, ua, ua + tw, -1); jpRoom(J, ua, 0, ua + tw, 1.7f, RM_TOILET, 0); jpWallU(J, hallU, 0, 1.7f, 0.85f); jpWallU(J, hallU, 1.7f, Dr, (1.7f + Dr) / 2); jpRoom(J, ua, 1.7f, ub, Dr, RM_WBED, sideW); }
                else { jpWallU(J, ub - tw, 0, 1.7f, -1); jpWallD(J, 1.7f, ub - tw, ub, -1); jpRoom(J, ub - tw, 0, ub, 1.7f, RM_TOILET, 0); jpWallU(J, hallU, 0, 1.7f, 0.85f); jpWallU(J, hallU, 1.7f, Dr, (1.7f + Dr) / 2); jpRoom(J, ua, 1.7f, ub, Dr, RM_WBED, sideW); }
                (void)ta; jpRoom(J, sideW == 3 ? ua + tw : ua, 0, sideW == 3 ? ub : ub - tw, 1.7f, RM_NANDO, 0); if (!top) jpWallD(J, Dr, ua, ub, -1); continue; }
            jpRoom(J, ua, 0, ub, Dr, pl, k == 0 ? sideW : 0);
            float hu = (ua + ub) / 2; (void)hu;
            if ((sideW == 3 && k == 0) || (sideW == 2 && k == n - 1)) { jpWallU(J, hallU, 0, Dr, Dr * 0.6f); if (!top) jpWallD(J, Dr, ua, ub, -1); }   // the room next to the hall opens onto it
            else if (!top) jpWallD(J, Dr, ua, ub, (ua + ub) / 2); }   // the others open onto the back corridor
    }
}
// the whole width on one side of the hall (the door is near a corner): the LDK across the front, a corridor (rouka) behind it,
// and behind that the toilet, the washroom with the bath beyond, and a tatami room or bedroom. Upstairs: bedrooms off the corridor.
static void jpBigZone(const JpF *J, float a, float b, float Dr, float hallU, int f, int kind, bool top) {
    bool hallLow = hallU <= a + 0.01f; float w = b - a;
    float sp = (f == 0 || kind == 1) ? fmaxf(2.8f, Dr - 2.6f - 0.9f) : fmaxf(2.4f, Dr * 0.5f - 0.45f), cd0 = sp, cd1 = sp + 0.9f;
    if (Dr - cd1 < 1.6f) { jpZone(J, a, b, Dr, hallU, true, f, kind, top); return; }
    jpWallU(J, hallU, 0, cd0, cd0 * 0.6f); jpWallD(J, cd0, a, b, -1); jpWallD(J, cd1, a, b, -1);
    jpRoom(J, a, cd0, b, cd1, RM_HALL, 1);
    jpRoom(J, a, 0, b, cd0, (f == 0 || kind == 1) ? RM_LDK : RM_WBED, 0);
    if (!top) jpWallD(J, Dr, a, b, -1);
    // the back row, from the hall side outward, each door on the corridor
    float u = hallLow ? a : b; int dir = hallLow ? 1 : -1;
    #define JPB(width, plan) { float u0 = u, u1 = u + dir * (width); if ((dir > 0 && u1 > b - 0.3f) || (dir < 0 && u1 < a + 0.3f)) u1 = dir > 0 ? b : a; \
        float lo = fminf(u0, u1), hi = fmaxf(u0, u1); if (hi - lo > 0.8f) { jpRoom(J, lo, cd1, hi, Dr, plan, 1); float dm = (lo + hi) / 2; jpWallD(J, cd1, lo, hi, dm); \
        if ((dir > 0 && hi < b - 0.05f) || (dir < 0 && lo > a + 0.05f)) jpWallU(J, dir > 0 ? hi : lo, cd1, Dr, -1); } u = u1; }
    if (f == 0 || kind == 1) { JPB(1.0f, RM_TOILET) JPB(1.7f, RM_SENMEN) JPB(1.7f, RM_BATH) if ((dir > 0 && u < b - 0.8f) || (dir < 0 && u > a + 0.8f)) JPB(w, kind == 1 ? RM_WBED : RM_WASHITSU) }
    else { JPB(1.0f, RM_TOILET) float rest = fabsf((dir > 0 ? b : a) - u); if (rest > 5.4f) { JPB(rest / 2, RM_WASHITSU) JPB(rest, RM_WBED) } else if (rest > 0.8f) JPB(rest, RM_WASHITSU) }
    #undef JPB
}
// a narrow house with the door in the middle: the genkan with a toilet and a shoe closet either side, then the LDK across the back
// with the washroom and the bath along one wall (the usual 3-4 m frontage plan)
static void jpNarrow(const JpF *J, float hu0, float hu1, float Dr, int kind, bool top) {
    float W = J->W, gD = fminf(1.8f, Dr * 0.3f);
    if (hu0 < 0.9f) hu0 = 0; if (W - hu1 < 0.9f) hu1 = W;
    jpRoom(J, hu0, 0, hu1, gD, RM_HALL, 0);
    bool toiletL = hu0 >= 0.9f;
    if (hu0 > 0) { jpWallU(J, hu0, 0, gD, gD * 0.5f); jpRoom(J, 0, 0, hu0, gD, toiletL ? RM_TOILET : RM_NANDO, 0); }
    if (hu1 < W) { jpWallU(J, hu1, 0, gD, gD * 0.5f); jpRoom(J, hu1, 0, W, gD, toiletL ? RM_NANDO : RM_TOILET, 0); }
    jpWallD(J, gD, 0, W, (hu0 + hu1) / 2);
    bool wetR = W - hu1 >= hu0; float ww = 1.8f, wa = wetR ? W - ww : 0, wb = wetR ? W : ww, edge = wetR ? wa : wb;
    float d1 = fminf(gD + 1.8f, Dr), d2 = fminf(gD + 3.6f, Dr);
    jpWallU(J, edge, gD, d1, (gD + d1) / 2); jpRoom(J, wa, gD, wb, d1, RM_SENMEN, wetR ? 2 : 3);
    if (d2 > d1 + 1.0f) { jpWallD(J, d1, wa, wb, (wa + wb) / 2); jpWallU(J, edge, d1, d2, -1); jpRoom(J, wa, d1, wb, d2, RM_BATH, 1); }
    if (Dr - d2 > 1.2f) { jpWallD(J, d2, wa, wb, -1); jpWallU(J, edge, d2, Dr, (d2 + Dr) / 2); jpRoom(J, wa, d2, wb, Dr, RM_NANDO, wetR ? 2 : 3); }
    else if (Dr - d2 > 0.05f) jpWallU(J, edge, d2, Dr, -1);
    float la = wetR ? 0 : wb, lb = wetR ? wa : W; (void)kind;
    jpRoom(J, la, gD, lb, Dr, RM_LDK, 0);
    if (!top) { jpWallD(J, Dr, la, lb, (la + lb) / 2); jpWallD(J, Dr, wa, wb, -1); }
}
// shops, the konbini, the koban and office floors, laid out the way such places are: the room the public walks into at the front,
// the working rooms behind it off a partition (stock, office, toilet, a tatami rest room in the koban); offices: open-plan floor
// at the front, a corridor across, toilets, a copy room and a meeting room behind. hallU >= 0: the side hall the rooms open onto.
static void jpWork(const JpF *J, float a, float b, float Dr, int kind, int f, bool top, float hallU) {
    float w = b - a; int sideW = a < 0.01f ? 2 : 3;
    if (kind == 6) {   // an office floor
        float sp = fmaxf(3.0f, Dr * 0.52f), cd = fminf(sp + 1.5f, Dr - 2.0f);
        if (cd < sp + 1.0f) { jpRoom(J, a, 0, b, Dr, RM_OFFICE, 0); if (hallU >= 0) jpWallU(J, hallU, 0, Dr, Dr * 0.3f); if (!top) jpWallD(J, Dr, a, b, (a + b) / 2); return; }
        jpWallD(J, sp, a, b, a + w * 0.3f); jpWallD(J, cd, a, b, -1); if (hallU >= 0) { jpWallU(J, hallU, 0, sp, sp * 0.5f); jpWallU(J, hallU, sp, cd, -1); }
        jpRoom(J, a, 0, b, sp, RM_OFFICE, 0); jpRoom(J, a, sp, b, cd, RM_HALL, 1);
        float u = a; const float tw = 2.0f; int plans[4] = {RM_TOILET, RM_TOILET, RM_STOCK, RM_OFFICE};
        for (int k = 0; k < 4 && u < b - 0.9f; k++) { float u1 = k == 3 || u + tw > b - 1.5f ? b : u + tw;
            jpRoom(J, u, cd, u1, Dr, plans[k], 1); jpWallD(J, cd, u, u1, (u + u1) / 2); if (u1 < b - 0.05f) jpWallU(J, u1, cd, Dr, -1); u = u1; }
        if (!top) jpWallD(J, Dr, a, b, -1);
        return; }
    float sp = fminf(fmaxf(Dr * 0.64f, Dr - 3.2f), Dr - 1.6f), tl = w >= 3.6f ? 1.1f : 0;   // the public room's depth; a toilet in the back corner
    int pubPlan = kind == 3 ? RM_KONBINI : kind == 4 ? RM_KOBAN : RM_SHOP;
    jpRoom(J, a, 0, b, sp, pubPlan, 0);
    if (hallU >= 0) jpWallU(J, hallU, 0, sp, sp * 0.5f);
    if (kind == 4 && w >= 5.0f) {   // the koban: the office and a tatami rest room behind the counter room
        float m1 = a + (w - tl) * 0.55f; jpWallD(J, sp, a, b, (a + m1) / 2);
        jpRoom(J, a, sp, m1, Dr, RM_OFFICE, 1); jpWallU(J, m1, sp, Dr, (sp + Dr) / 2); jpRoom(J, m1, sp, b - tl, Dr, RM_WASHITSU, 1);
        if (tl > 0) { jpWallU(J, b - tl, sp, Dr, (sp + Dr) / 2); jpRoom(J, b - tl, sp, b, Dr, RM_TOILET, sideW); }
    } else {   // stock room across the back, the toilet off it
        jpWallD(J, sp, a, b, a + (w - tl) * 0.5f);
        jpRoom(J, a, sp, b - tl, Dr, RM_STOCK, 1);
        if (tl > 0) { jpWallU(J, b - tl, sp, Dr, (sp + Dr) / 2); jpRoom(J, b - tl, sp, b, Dr, RM_TOILET, sideW); } }
    if (!top) jpWallD(J, Dr, a, b - tl, a + (w - tl) * 0.5f);   // the back door onto the stair hall
    if (!top && tl > 0) jpWallD(J, Dr, b - tl, b, -1);
    if (hallU >= 0) jpWallU(J, hallU, sp, Dr, (sp + Dr) / 2);
    (void)f;
}
// returns true when the floor was laid out (the caller then skips its two-room plan)
// audit (KIRI_SHOT): every room of a planned floor can be walked into from the front door (upstairs: from the stair hall)
static int raOn = -1, raRooms = 0, raBad = 0; static char raList[900] = ""; static int raHist[2][2][20];   // [upstairs][bad][plan]
static bool raFree(float x, float z, float y) { return standFree(x, z, y); }
static void roomAudit(Rectangle in, float y0, int f, int kind, float sx, float sz) {   // (sx, sz): where one comes in
    const float q = 0.1f; int W = (int)(in.width / q) + 1, H = (int)(in.height / q) + 1; if (W * H > 60000 || W < 2 || H < 2) return;
    static unsigned char g[60000]; static int st[60000]; int n = 0;
    static int dOff[4000]; int nOff = 0;   // 0.9.8: doors open (a shut door is a way through)
    for (int i = 0; i < nOps && nOff < 4000; i++) { Open *o = &ops[i]; if (o->type == OP_DOOR && o->f == curXf && o->sid >= 0 && !sol[o->sid].off && fabsf(o->fy - y0) < 0.6f) { sol[o->sid].off = true; dOff[nOff++] = o->sid; } }
    for (int j = 0; j < H; j++) for (int i = 0; i < W; i++) g[j * W + i] = raFree(in.x + (i + 0.5f) * q, in.y + (j + 0.5f) * q, y0) ? 1 : 0;
    for (int i = 0; i < nOff; i++) sol[dOff[i]].off = false;
    if (sx > -1e8f) { int bi = -1; float bd = 1e9f; for (int j = 0; j < H; j++) for (int i = 0; i < W; i++) if (g[j * W + i]) { float dx = in.x + (i + 0.5f) * q - sx, dz = in.y + (j + 0.5f) * q - sz, d = dx * dx + dz * dz; if (d < bd) { bd = d; bi = j * W + i; } }
        if (bi >= 0 && bd < 1.5f * 1.5f) { g[bi] = 2; st[n++] = bi; } }
    else for (int r = 0; r < jpRooms; r++) if (jpRoomP[r] == RM_HALL) { Rectangle rm = jpRoomR[r]; for (int j = 0; j < H; j++) for (int i = 0; i < W; i++) { float x = in.x + (i + 0.5f) * q, z = in.y + (j + 0.5f) * q; if (g[j * W + i] == 1 && x > rm.x && x < rm.x + rm.width && z > rm.y && z < rm.y + rm.height) { g[j * W + i] = 2; st[n++] = j * W + i; } } }
    static int dbg = 0; int seeds = n, freeN = 0, reach = 0; for (int k = 0; k < W * H; k++) freeN += g[k] != 0;
    while (n > 0) { int c = st[--n], i = c % W, j = c / W; static const int dx[4] = {1, -1, 0, 0}, dz[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; k++) { int a = i + dx[k], b = j + dz[k]; if (a < 0 || b < 0 || a >= W || b >= H || g[b * W + a] != 1) continue; g[b * W + a] = 2; st[n++] = b * W + a; } }
    for (int k = 0; k < W * H; k++) reach += g[k] == 2; if (f > 0 && reach < freeN * 0.8f && getenv("KIRI_RADBG")) { dbg++; TraceLog(LOG_INFO, "RADBG f%d in %.1f,%.1f %.1fx%.1f arr %.2f,%.2f seeds %d free %d reached %d grid jgA %.2f,%.2f ok %d", f, in.x, in.y, in.width, in.height, sx, sz, seeds, freeN, reach, jgAu, jgAd, jgAok); }
    for (int r = 0; r < jpRooms; r++) { Rectangle rm = jpRoomR[r]; bool got = false, any = false;
        for (int j = 0; j < H && !got; j++) for (int i = 0; i < W && !got; i++) { float x = in.x + (i + 0.5f) * q, z = in.y + (j + 0.5f) * q; if (x < rm.x + 0.15f || x > rm.x + rm.width - 0.15f || z < rm.y + 0.15f || z > rm.y + rm.height - 0.15f) continue; if (g[j * W + i]) any = true; if (g[j * W + i] == 2) got = true; }
        if (!got && any && jpDump) jpDumpRect("U", rm);
        if (!got && any && getenv("KIRI_JGDUMP") && !(getenv("KIRI_JGSKIP6") && kind == 6)) { static int na = 0; if (na++ < 3) { FILE *fp = fopen(TextFormat("/tmp/claude-0/ag%d.txt", na), "w"); if (fp) { fprintf(fp, "in %.2f %.2f %.2f %.2f room %.2f %.2f %.2f %.2f start %.2f %.2f\n", in.x, in.y, in.width, in.height, rm.x, rm.y, rm.width, rm.height, sx, sz);
            int stp = getenv("KIRI_JGFULL") ? 1 : 2; for (int j = 0; j < H; j += stp) { for (int i = 0; i < W; i += stp) { float x = in.x + (i + 0.5f) * q, z = in.y + (j + 0.5f) * q; bool inR = x > rm.x && x < rm.x + rm.width && z > rm.y && z < rm.y + rm.height; fputc(g[j * W + i] == 0 ? '#' : g[j * W + i] == 2 ? (inR ? 'R' : '.') : (inR ? 'x' : 'o'), fp); } fputc('\n', fp); }
            float cx = rm.x + rm.width / 2, cz = rm.y + rm.height / 2; Vector3 wmn, wmx; xfBox(curXf, V3(cx - 2.5f, 0, cz - 2.5f), V3(cx + 2.5f, 0, cz + 2.5f), &wmn, &wmx); query(wmn.x, wmn.z, wmx.x, wmx.z);
            for (int k = 0; k < qn; k++) { Solid *s = &sol[qb[k]]; if (s->off || s->mx.y <= y0 + 0.42f || s->mn.y >= y0 + 1.7f) continue; fprintf(fp, "S f%d same %d %.2f..%.2f x %.2f..%.2f z %.2f..%.2f\n", s->f, s->f == curXf, s->mn.y - y0, s->mx.y - y0, s->mn.x, s->mx.x, s->mn.z, s->mx.z); }
            fclose(fp); } } }
        if (!got && any && getenv("KIRI_JGDUMP")) { static int nd = 0; if (nd++ < 3) { FILE *fp = fopen(TextFormat("/tmp/claude-0/jg%d.txt", nd), "w"); if (fp) { fprintf(fp, "room %d plan %d ud %.2f %.2f %.2f %.2f W %d D %d doors %d walls %d\n", r, jpRoomP[r], jpRoomUD[r][0], jpRoomUD[r][1], jpRoomUD[r][2], jpRoomUD[r][3], jgW, jgD, jpND, jpNW); fprintf(fp, "stair u %.2f..%.2f d %.2f..%.2f arr %.2f %.2f ok %d f %d\n", jgSu0, jgSu1, jgSd0, jgSd1, jgAu, jgAd, jgAok, f);
            fputs(jbTrace, fp); for (int k = 0; k < jpNW; k++) fprintf(fp, "W ax %d pos %.2f %.2f..%.2f\n", jpW[k].ax, jpW[k].pos, jpW[k].lo, jpW[k].hi); for (int k = 0; k < jpND; k++) fprintf(fp, "D ax %d pos %.2f at %.2f\n", jpDr[k].ax, jpDr[k].pos, jpDr[k].at);
            for (int j = 0; j < jgD; j += 2) { for (int i = 0; i < jgW; i += 2) { int rr = jpRoomAt((i + 0.5f) * JG_Q, (j + 0.5f) * JG_Q); fputc(jg[j * jgW + i] == 0 ? '#' : jg[j * jgW + i] == 2 ? (rr == r ? 'R' : '.') : (rr == r ? 'x' : 'o'), fp); } fputc('\n', fp); } fclose(fp); } } }
        raRooms++; if (jpRoomP[r] < 20) raHist[f > 0][!got && any][jpRoomP[r]]++; if (!got && any) { raBad++; Vector3 c = xfP(curXf, V3(rm.x + rm.width / 2, 0, rm.y + rm.height / 2)); if (strlen(raList) < 840) { char t[64]; snprintf(t, 64, " (kind %d floor %d plan %d at %.0f,%.0f)", kind, f, jpRoomP[r], c.x, c.z); strcat(raList, t); } } }
}
static Vector2 jpArr, jpArrNext; static bool jpArrOk, jpArrNextOk;
static bool jpFloor(Rectangle in, int street, Rectangle stairR, bool hasStairs, float y0, float yTop, int f, int kind, float doorLocal, int region) {
    JpF J = {in, street, (street == 0 || street == 2) ? in.width : in.height, (street == 0 || street == 2) ? in.height : in.width};
    if (J.W < 5.0f || J.D < 4.6f) return false;
    if (jpDumpN < 0) { jpDumpN = 0; if (getenv("KIRI_PLANDUMP")) { jpDump = fopen(getenv("KIRI_PLANDUMP"), "w"); if (jpDump) setvbuf(jpDump, NULL, _IOLBF, 0); } }
    if (jpDump && (jpDumpN >= 400)) { if (jpDumpN >= 400) { fclose(jpDump); jpDump = NULL; } }
    FILE *keepDump = jpDump; if (jpDump && f > 2) jpDump = NULL;
    { static float px = 1e9f, pz = 0; static int pi = 0; if (!pi) { pi = 1; if (getenv("KIRI_PLANAT")) sscanf(getenv("KIRI_PLANAT"), "%f %f", &px, &pz); } if (jpDump && px < 1e8f && (fabsf(in.x - px) > 0.2f || fabsf(in.y - pz) > 0.2f)) jpDump = NULL; }
    if (jpDump && getenv("KIRI_PLANWORK") && !((kind == 2 && f == 0) || kind == 3 || kind == 4 || kind == 6)) jpDump = NULL;   // (dump only the shops and offices)
    if (jpDump) { fprintf(jpDump, "B %d %d %d %d\n", jpDumpN, f, kind, street); jpDumpRect("O", in); if (hasStairs) jpDumpRect("S", stairR); { Vector3 dp = xfP(curXf, (street == 0 || street == 2) ? V3(doorLocal, 0, street == 0 ? in.y : in.y + in.height) : V3(street == 3 ? in.x : in.x + in.width, 0, doorLocal)); fprintf(jpDump, "E %.2f %.2f\n", dp.x, dp.z); } if (f == 0 || !hasStairs) jpDumpN++; }
    jpY0 = y0; jpYT = yTop; jpPT = T_PLASTER; jpPC = kind == 0 ? rgb(226, 218, 200) : rgb(222, 222, 216); jpRooms = 0; jpNW = 0; jpND = 0;
    float doorU = (street == 0 || street == 2) ? doorLocal - in.x : doorLocal - in.y;   // the front door's position across
    jgAok = false; if (f > 0 && jpArrOk) { float x = jpArr.x, z = jpArr.y; switch (street) { case 0: jgAu = x - in.x; jgAd = z - in.y; break; case 2: jgAu = x - in.x; jgAd = in.y + in.height - z; break; case 3: jgAu = z - in.y; jgAd = x - in.x; break; default: jgAu = z - in.y; jgAd = in.x + in.width - x; } jgAok = true; }
    // the stairwell in (u, d)
    float su0 = 0, su1 = 0, sd0 = J.D, sd1 = J.D;
    if (hasStairs) { Rectangle s = stairR; float xs[2] = {s.x, s.x + s.width}, zs[2] = {s.y, s.y + s.height}; su0 = sd0 = 1e9f; su1 = sd1 = -1e9f;
        for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) { float x = xs[i], z = zs[j], u, d;
            switch (street) { case 0: u = x - in.x; d = z - in.y; break; case 2: u = x - in.x; d = in.y + in.height - z; break; case 3: u = z - in.y; d = x - in.x; break; default: u = z - in.y; d = in.x + in.width - x; }
            su0 = fminf(su0, u); su1 = fmaxf(su1, u); sd0 = fminf(sd0, d); sd1 = fmaxf(sd1, d); }
        su0 = fmaxf(su0, 0); su1 = fminf(su1, J.W); sd0 = fmaxf(sd0, 0); sd1 = fminf(sd1, J.D);
        { float x = gStNear.x, z = gStNear.y, nu, nd; switch (street) { case 0: nu = x - in.x; nd = z - in.y; break; case 2: nu = x - in.x; nd = in.y + in.height - z; break; case 3: nu = z - in.y; nd = x - in.x; break; default: nu = z - in.y; nd = in.x + in.width - x; }
          gStNearLo = su1 - su0 >= sd1 - sd0 ? fabsf(nu - su0) < fabsf(nu - su1) : fabsf(nd - sd0) < fabsf(nd - sd1); gStNu = nu; gStNd = nd; } }
    bool atBack = !hasStairs || sd0 > J.D * 0.45f;
    bool work = (kind == 2 && f == 0) || kind == 3 || kind == 4 || kind == 6;   // a shop, the konbini, the koban, an office floor
    if (kind == 2 && f > 0) kind = 1;   // the family lives over the shop: a flat
    bool top = !hasStairs;   // (nothing above or below that needs the back corridor)
    if (atBack) {
        float Dr = hasStairs ? sd0 - 0.05f : J.D; if (Dr < 3.0f) { jpDump = keepDump; return false; }
        if (work) { if (hasStairs) jpRoom(&J, 0, Dr, J.W, J.D, RM_HALL, 1); jpWork(&J, 0, J.W, Dr, kind, f, top, -1); goto jpDoneRooms; }
        float hw = f == 0 ? 1.7f : 1.0f, hu0 = clampf(doorU - hw / 2, 0, J.W - hw);   // (wide enough for the genkan downstairs)   // the entrance hall from the genkan straight back
        if (f > 0) hu0 = clampf((jgAok ? jgAu : (su0 + su1) / 2) - hw / 2, 0, J.W - hw);   // upstairs the hall starts at the landing where the stairs come up (not at the middle of the stairwell, where there is only the railing)
        float hu1 = hu0 + hw; bool leftBig = hu0 > J.W - hu1;
        if ((f == 0 || kind == 1) && hu0 < 2.6f && J.W - hu1 < 2.6f && hu0 >= 0.9f && J.W - hu1 >= 0.9f) {   // narrow, the door in the middle
            if (hasStairs) jpRoom(&J, 0, Dr, J.W, J.D, RM_HALL, 1);
            jpNarrow(&J, hu0, hu1, Dr, kind, top);
        } else if (fminf(hu0, J.W - hu1) < 1.2f) {   // the door is near a corner: the hall runs along that wall, the rest is one wide zone
            if (hu0 < J.W - hu1) { hu0 = 0; } else { hu1 = J.W; }
            jpRoom(&J, hu0, 0, hu1, Dr, RM_HALL, 0); if (hasStairs) jpRoom(&J, 0, Dr, J.W, J.D, RM_HALL, 1);
            if (hu0 < 0.01f) jpBigZone(&J, hu1, J.W, Dr, hu1, f, kind, top); else jpBigZone(&J, 0, hu0, Dr, hu0, f, kind, top);
        } else {
            jpRoom(&J, hu0, 0, hu1, Dr, RM_HALL, 0); if (hasStairs) jpRoom(&J, 0, Dr, J.W, J.D, RM_HALL, 1);
            if (hu0 > 0.5f) jpZone(&J, 0, hu0, Dr, hu0, leftBig, f, kind, top);
            if (J.W - hu1 > 0.5f) jpZone(&J, hu1, J.W, Dr, hu1, !leftBig, f, kind, top); }
    } else {   // the stairs along a side wall: that side is the hall the whole way back
        bool low = su0 < J.W - su1; float hu0 = low ? 0 : su0 - 0.1f, hu1 = low ? su1 + 0.1f : J.W;
        float a = low ? hu1 : 0, b = low ? J.W : hu0; if (b - a < 2.6f) { jpDump = keepDump; return false; }
        float hallU = low ? hu1 : hu0;
        jpRoom(&J, hu0, 0, hu1, J.D, RM_HALL, low ? 2 : 3);
        float Dr = J.D;
        if (work) { jpWork(&J, a, b, Dr, kind, f, true, hallU); goto jpDoneRooms; }
        if (f == 0 || kind == 1) {   // front: LDK; back: the wet rooms and a tatami room
            float sp = fmaxf(Dr * 0.55f, Dr - 5.0f); jpWallD(&J, sp, a, b, -1); jpWallU(&J, hallU, 0, sp, sp * 0.5f); jpRoom(&J, a, 0, b, sp, RM_LDK, 0);
            float wet = fminf(1.8f, (b - a) * 0.45f), wa = low ? b - wet : a, wb = low ? b : a + wet;
            float mid = (sp + Dr) / 2; jpWallU(&J, low ? wa : wb, sp, Dr, -1); jpWallD(&J, mid, wa, wb, (wa + wb) / 2);
            jpRoom(&J, wa, sp, wb, mid, RM_SENMEN, low ? 3 : 2); jpRoom(&J, wa, mid, wb, Dr, RM_BATH, 1);
            float ra = low ? a : wb, rb = low ? wa : b; jpWallU(&J, hallU, sp, Dr, (sp + Dr) / 2);
            if (rb - ra >= 2.4f) { float tz = sp + 1.6f; jpWallD(&J, tz, ra, rb, -1); jpRoom(&J, ra, sp, rb, tz, RM_TOILET, 1); jpRoom(&J, ra, tz, rb, Dr, RM_WASHITSU, 1); }
            else jpRoom(&J, ra, sp, rb, Dr, RM_TOILET, 1);
            /* the washroom door: from the room next to it */ jpWallD(&J, sp, wa, wb, (wa + wb) / 2);
        } else {   // upstairs: two or three bedrooms off the stair hall
            int n = Dr >= 8.0f ? 3 : 2; float step = Dr / n;
            for (int k = 0; k < n; k++) { float d0 = k * step, d1 = d0 + step; if (k > 0) jpWallD(&J, d0, a, b, -1); jpWallU(&J, hallU, d0, d1, (d0 + d1) / 2);
                jpRoom(&J, a, d0, b, d1, k == 0 ? RM_WASHITSU : k == n - 1 && n == 3 ? RM_TOILET : RM_WBED, k == 0 ? 0 : low ? 3 : 2); }
        }
    }
    jpDoneRooms:
    jpBuildWalls(&J, f, doorU, hasStairs, su0, su1, sd0, sd1);
    if (jpDump && f > 0 && jpArrOk) { Vector3 ap = xfP(curXf, V3(jpArr.x, 0, jpArr.y)); fprintf(jpDump, "A %.2f %.2f\n", ap.x, ap.z); }
    if (raOn < 0) raOn = getenv("KIRI_SHOT") || getenv("KIRI_ROOMAUDIT") || getenv("KIRI_PLANDUMP") ? 1 : 0;
    furnDump = jpDump; furnDumpY = y0;
    static int noFurn = -1; if (noFurn < 0) noFurn = getenv("KIRI_NOFURN") ? 1 : 0;
    for (int i = 0; i < jpRooms && !noFurn; i++) { planRoom(jpRoomR[i], y0, jpRoomP[i], jpRoomO[i], region, street, kind == 0); if (jpDump) { char t[8]; snprintf(t, 8, "R%d", jpRoomP[i]); jpDumpRect(t, jpRoomR[i]); } }
    furnDump = NULL;
    if (raOn < 0) raOn = getenv("KIRI_SHOT") || getenv("KIRI_ROOMAUDIT") || getenv("KIRI_PLANDUMP") ? 1 : 0;
    if (raOn) { if (f == 0) { float du = doorU, dd = 0.7f, sx, sz; switch (street) { case 0: sx = in.x + du; sz = in.y + dd; break; case 2: sx = in.x + du; sz = in.y + in.height - dd; break; case 3: sx = in.x + dd; sz = in.y + du; break; default: sx = in.x + in.width - dd; sz = in.y + du; } roomAudit(in, y0, f, kind, sx, sz); } else if (jpArrOk) roomAudit(in, y0, f, kind, jpArr.x, jpArr.y); else roomAudit(in, y0, f, kind, -1e9f, 0); }
    if (!jpDump) jpDump = keepDump;
    return true;
}
static void stain(Vector3 p, float r) {   // dried blood on a floor, for good (in the current frame): a few overlapping dark patches
    for (int k = 0; k < 3 + ri(3); k++) { float a = fr(-r * 0.6f, r * 0.6f), b = fr(-r * 0.6f, r * 0.6f), w = fr(r * 0.3f, r * 0.7f), d = fr(r * 0.3f, r * 0.7f), y = p.y + 0.004f + k * 0.0015f; box(T_GORE, V3(p.x + a - w, y, p.z + b - d), V3(p.x + a + w, y + 0.002f, p.z + b + d), (Color){(unsigned char)(70 + ri(40)), 8, 8, 255}, false, 16); }
}
// 0.9.7: the stairwell: a wall along the flights' room side with a doorway at each landing, and one across its far end
// 0.9.8: the stairwell is a closed concrete shaft: a wall along its room side with a door at each landing, and one across its inner end with a door
// (flights: the wall along the flights too; a planned floor builds that part with its rooms)
static void stairwellWalls(bool runX, float uStart, float uE, float LEAD, float bandLo, float bandHi, bool stripHigh, float uMax, float y0, float yTop, bool base, bool flights) {
    float t = 0.1f, c0 = stripHigh ? bandLo - t : bandHi, c1 = c0 + t, cD = c0 + t / 2, yD = y0 + 2.1f; Color wc = base ? rgb(120, 118, 112) : rgb(168, 166, 160); int tex = T_CONCRETE;
    float g[2][2] = {{uStart + LEAD / 2 - 0.45f, uStart + LEAD / 2 + 0.45f}, {uE - LEAD / 2 - 0.45f, uE - LEAD / 2 + 0.45f}};
    if (g[0][0] < uStart + 0.05f) { g[0][1] += uStart + 0.05f - g[0][0]; g[0][0] = uStart + 0.05f; }
    bool endWall = uMax - uE > 0.3f; float uW = endWall ? uE + t : uE;
    #define SWB(a_, b_, ya_, yb_) do { if ((b_) - (a_) > 0.01f) { if (runX) box(tex, V3(a_, ya_, c0), V3(b_, yb_, c1), wc, true, ALL); else box(tex, V3(c0, ya_, a_), V3(c1, yb_, b_), wc, true, ALL); } } while (0)
    if (!base && y0 < 1 && getenv("KIRI_SWLOG")) { static int nl = 0; if (nl++ < 12) { float m = (g[0][0] + g[0][1]) / 2, sg = stripHigh ? 1.0f : -1.0f;
        Vector3 P = xfP(curXf, runX ? V3(m, 0, cD) : V3(cD, 0, m)), N = Vector3Subtract(xfP(curXf, runX ? V3(m, 0, cD + sg) : V3(cD + sg, 0, m)), P), U = Vector3Subtract(xfP(curXf, runX ? V3(m + 1, 0, cD) : V3(cD, 0, m + 1)), P);
        TraceLog(LOG_INFO, "SWDOOR %.2f %.2f in %.2f %.2f along %.2f %.2f", P.x, P.z, N.x, N.z, U.x, U.z); } }
    SWB(uStart, g[0][0], y0, yTop);
    SWB(g[0][0], g[0][1], yD, yTop); innerDoor(runX, g[0][0], g[0][1], cD, y0, false);
    if (flights) SWB(g[0][1], uW, y0, yTop); else { SWB(g[0][1], uStart + LEAD, y0, yTop); SWB(uE - LEAD, uW, y0, yTop); }   // 0.9.8: the far end is the half landing: walled
    #undef SWB
    if (endWall) {   // across the inner end of the shaft, a door in the middle
        float e0 = bandLo, e1 = bandHi, m = (e0 + e1) / 2, d0 = m - 0.45f, d1 = m + 0.45f;
        #define SWE(a_, b_, ya_, yb_) do { if ((b_) - (a_) > 0.01f) { if (runX) box(tex, V3(uE, ya_, a_), V3(uE + t, yb_, b_), wc, true, ALL); else box(tex, V3(a_, ya_, uE), V3(b_, yb_, uE + t), wc, true, ALL); } } while (0)
        SWE(e0, e1, y0, yTop); (void)d0; (void)d1;
        #undef SWE
    }
    for (int k = 0; k < 1; k++) { Rectangle kc = runX ? (Rectangle){g[k][0] - 0.2f, stripHigh ? c0 - 1.1f : c1, 1.3f, 1.1f} : (Rectangle){stripHigh ? c0 - 1.1f : c1, g[k][0] - 0.2f, 1.1f, 1.3f}; keepAdd(kc, kc); }
}
// 0.9.7: a basement with a plan: a corridor along the stairwell, store rooms, a boiler room; one light that flickers or is dead; things
// that happened down here
static void basementPlan(Rectangle r, Rectangle in, bool runX, float uStart, float runLen, float cMin, float cMax, float corC, bool stripHigh, float BY, int kind) {
    float uEnd = uStart + runLen, yT = SIDEH - 0.32f, corW = 1.4f, wallC = stripHigh ? corC - corW : corC + corW;   // the corridor between corC and wallC
    float farC = stripHigh ? cMin : cMax; Color wc = rgb(118, 116, 110);
    if (fabsf(farC - wallC) < 2.2f) { wallC = corC; }   // too narrow for rooms: one room off the stairs
    int nr = runLen > 9 ? 3 : 2; float rw = runLen / nr; int boiler = ri(nr);
    #define BBOX(u0_, c0_, u1_, c1_, y0_, y1_, tx_, col_, sol_, m_) do { float a0_ = fminf(c0_, c1_), a1_ = fmaxf(c0_, c1_); if (runX) box(tx_, V3(u0_, y0_, a0_), V3(u1_, y1_, a1_), col_, sol_, m_); else box(tx_, V3(a0_, y0_, u0_), V3(a1_, y1_, u1_), col_, sol_, m_); } while (0)
    if (wallC != corC) for (int k = 0; k < nr; k++) {   // the corridor wall, one door per room
        float u0 = uStart + k * rw, u1 = u0 + rw, dc = (u0 + u1) / 2 + fr(-rw * 0.2f, rw * 0.2f), t = 0.1f;
        BBOX(u0, wallC - t / 2, dc - 0.45f, wallC + t / 2, BY, yT, T_CONCRETE, wc, true, ALL); BBOX(dc + 0.45f, wallC - t / 2, u1, wallC + t / 2, BY, yT, T_CONCRETE, wc, true, ALL);
        BBOX(dc - 0.45f, wallC - t / 2, dc + 0.45f, wallC + t / 2, BY + 2.05f, yT, T_CONCRETE, wc, false, ALLB);
        { Rectangle kc = runX ? (Rectangle){dc - 0.6f, fminf(wallC, farC) < wallC ? wallC - 1.1f : wallC, 1.2f, 1.1f} : (Rectangle){fminf(wallC, farC) < wallC ? wallC - 1.1f : wallC, dc - 0.6f, 1.1f, 1.2f}; keepAdd(kc, kc); }
        { Rectangle kc = runX ? (Rectangle){dc - 0.6f, fminf(corC, wallC), 1.2f, fabsf(wallC - corC)} : (Rectangle){fminf(corC, wallC), dc - 0.6f, fabsf(wallC - corC), 1.2f}; keepAdd(kc, kc); }
        if (k > 0) BBOX(u0 - t / 2, wallC, u0 + t / 2, farC, BY, yT, T_CONCRETE, wc, true, ALL);   // between the rooms
        float rc0 = stripHigh ? farC + 0.35f : farC - 0.35f, sgn = stripHigh ? 1 : -1; float yaw = runX ? (stripHigh ? 0 : PI) : (stripHigh ? -PI / 2 : PI / 2);
        if (k == boiler) {   // the boiler room: the boiler, pipes along the ceiling, a dark stain spreading from under it
            float bx = (u0 + u1) / 2, bc = (wallC + farC) / 2; BBOX(bx - 0.6f, bc - 0.6f, bx + 0.6f, bc + 0.6f, BY, BY + 1.8f, T_METAL, rgb(80, 84, 80), true, ALL);
            BBOX(u0 + 0.1f, farC + sgn * 0.15f, u1 - 0.1f, farC + sgn * 0.27f, yT - 0.35f, yT - 0.23f, T_METAL, rgb(110, 90, 70), false, ALL);
            stain(runX ? V3(bx + 0.9f, BY, bc) : V3(bc, BY, bx + 0.9f), 1.1f);
        } else {   // store rooms: shelves, boxes, lockers along the far wall
            int kinds[6] = {CK_SHELF, CK_BOX, CK_LOCKER, CK_TOOLBOX, CK_BOX, CK_SHELF}; float u = u0 + 0.35f;
            for (int q = 0; q < 6; q++) { int ck = kinds[(q + ri(3)) % 6]; float wdt = ckSize[ck].x, dep = ckSize[ck].z; if (u + wdt > u1 - 0.3f) break;
                float cc = rc0 + sgn * (dep / 2 - 0.3f); Vector3 p = runX ? V3(u + wdt / 2, BY, cc) : V3(cc, BY, u + wdt / 2); addCont(ck, p, yaw, 9, true); u += wdt + fr(0.2f, 0.8f); } }
        if (ri(3) == 0) { Vector3 c = runX ? V3(fr(u0 + 0.6f, u1 - 0.6f), BY, (wallC + farC) / 2) : V3((wallC + farC) / 2, BY, fr(u0 + 0.6f, u1 - 0.6f)); stain(c, fr(0.5f, 1.2f)); }   // something was dragged
    }
    {   // the dead, more often than upstairs
        Rectangle br = runX ? (Rectangle){uStart, fminf(wallC, farC), runLen, fabsf(farC - wallC)} : (Rectangle){fminf(wallC, farC), uStart, fabsf(farC - wallC), runLen};
        if (br.width > 1.2f && br.height > 1.2f && ri(10) < 4) placeBody(br, BY, 1, 0, (unsigned)ri(100000)); }
    {   // drag marks and a handprint trail along the corridor
        for (int k = 0; k < 3; k++) { float u = uStart + fr(0.8f, runLen - 0.8f); Vector3 c = runX ? V3(u, BY, (corC + wallC) / 2) : V3((corC + wallC) / 2, BY, u); if (ri(2)) stain(c, fr(0.3f, 0.7f)); } }
    for (int k = 0; k < nr; k++) { float u = uStart + (k + 0.5f) * rw, c = (wallC + farC) / 2; addLight(runX ? V3(u, yT - 0.5f, c) : V3(c, yT - 0.5f, u), V3(0.75f, 0.8f, 0.62f), 5, ri(3) ? 2 : 1); }   // most bulbs are dead; the rest flicker
    { float u = uStart + runLen / 2, c = (corC + wallC) / 2; addLight(runX ? V3(u, yT - 0.4f, c) : V3(c, yT - 0.4f, u), V3(0.7f, 0.2f, 0.15f), 4, 1); }   // a red emergency lamp in the corridor, flickering
    #undef BBOX
    (void)r; (void)in; (void)kind; (void)uEnd;
}
static void building(Rectangle r, int kind, int floors, int street) {
    gNoWinR = (Rectangle){0, 0, 0, 0};
    unsigned bfl = 0;   // details: a preset's, or a town lot's picked by its kind
    if (bP) bfl = bP->flags;
    else if (kind == 0) bfl = (ri(3) ? BD_AC : 0) | (ri(2) ? BD_MAIL : 0) | (floors <= 2 && ri(5) < 4 ? (ri(3) ? BD_HIP : BD_PITCH) : 0) | (ri(3) == 0 ? BD_BIKES : 0) | (ri(4) == 0 ? BD_PIPES : 0)
                              | (ri(5) < 3 ? BD_FENCE : 0) | (ri(10) < 3 ? BD_GARAGE : 0) | (ri(3) == 0 ? BD_HISASHI : 0) | (floors == 2 && ri(5) == 0 ? BD_SKIRT : 0) | (ri(8) == 0 ? BD_CARPORT : 0) | (floors >= 2 && ri(3) == 0 ? BD_BALC : 0);   // Odawara houses
    else if (kind == 1) bfl = (floors > 2 && ri(3) ? BD_BALC | BD_LAUND : 0) | BD_AC | (ri(2) ? BD_TANK : 0) | (ri(2) ? BD_BANDS : 0) | (ri(3) == 0 ? BD_PIPES : 0) | (floors <= 3 && ri(3) == 0 ? BD_STAIR : 0);
    else if (kind == 2) bfl = (ri(5) < 3 ? BD_AWN : 0) | (ri(3) == 0 ? BD_SHUT : 0) | (ri(2) ? BD_AC : 0) | (ri(4) == 0 ? BD_BIKES : 0) | (floors > 3 && ri(2) ? BD_BALC | BD_LAUND : 0);
    else if (kind == 6) bfl = BD_BANDS | (ri(2) ? BD_RFENCE : 0) | BD_AC;
    else bfl = BD_AC;
    curB = r; curT = 0.3f;
    if (nFoot < MAX_FOOT) { footH[nFoot] = floors * FH + 0.6f; foots[nFoot++] = (Rectangle){r.x + curT, r.y + curT, r.width - 2 * curT, r.height - 2 * curT}; }
    int extTex = kind == 0 ? T_PLASTER : kind == 1 ? T_CONCRETE : kind == 2 ? T_TILEWALL : kind == 3 ? T_PLAIN : kind == 4 ? T_PLASTER : T_CONCRETE;
    Color ec = kind == 3 ? rgb(215, 220, 225) : kind == 4 ? rgb(200, 200, 205) : kind == 6 ? rgb(235, 242, 242) : rgb(fr(200, 255), fr(195, 245), fr(185, 235));
    {   // the outside finish: each kind of building draws from the finishes such buildings really have (picked by where it stands, so a given building never changes)
        float pk = hash2((int)floorf(r.x * 2), (int)floorf(r.y * 2), 9041 + kind), tn = hash2((int)floorf(r.x * 2), (int)floorf(r.y * 2), 9077);
        if (kind == 0) { extTex = pk < 0.22f ? T_PLASTER : pk < 0.5f ? T_XSTUCCO : pk < 0.8f ? T_XSIDING : T_XBOARDS; if (extTex == T_XBOARDS) ec = rgb(205 + 40 * tn, 200 + 40 * tn, 195 + 40 * tn); }
        else if (kind == 1) { extTex = pk < 0.2f ? T_CONCRETE : pk < 0.55f ? T_XTILE : pk < 0.8f ? T_XPANEL : T_XSTUCCO; if (extTex == T_XTILE) { static const Color tc[5] = {{226, 216, 198, 255}, {196, 150, 120, 255}, {214, 222, 226, 255}, {176, 190, 178, 255}, {236, 232, 224, 255}}; ec = tc[(int)(tn * 4.99f)]; } }
        else if (kind == 2) { extTex = pk < 0.2f ? T_TILEWALL : pk < 0.5f ? T_XTILE : pk < 0.78f ? T_XSTUCCO : pk < 0.9f ? T_XSIDING : T_XCORR; if (extTex == T_XCORR) ec = tn < 0.5f ? rgb(170, 196, 200) : rgb(214, 208, 196); }
        else if (kind == 3) extTex = T_XPANEL;
        else if (kind == 4) extTex = T_XTILE;
        else if (kind == 5) extTex = T_XPANEL; }
    if (bP && bP->tex >= 0) extTex = bP->tex;
    if (bP && bP->wall.a) ec = bP->wall;
    Rectangle in = {r.x + curT, r.y + curT, r.width - 2 * curT, r.height - 2 * curT};
    bool runX = in.width >= in.height;
    float runLen = runX ? in.width : in.height, cross = runX ? in.height : in.width;
    int acc = floors > 6 ? 6 : floors;                       // floors you can walk into
    const float SW = 1.05f, RUN = 3.6f; const int NS = 12;
    const int NSH = 10;   // 0.9.8: steps in each half flight
    // a step: concrete from yb up to its tread, a handrail on the shaft wall and a railing (solid) on the well side
    #define STEPBOX(u0_, u1_, cS_, yb_, top_, wallHi_) do { float _u0 = (u0_), _u1 = (u1_), _c = (cS_), _t = (top_); bool _wh = (wallHi_); \
        if (runX) box(T_CONCRETE, V3(_u0, (yb_), _c), V3(_u1, _t, _c + SW), rgb(170, 165, 160), true, ALL); else box(T_CONCRETE, V3(_c, (yb_), _u0), V3(_c + SW, _t, _u1), rgb(170, 165, 160), true, ALL); \
        float _h0 = _wh ? _c + SW - 0.09f : _c + 0.04f, _hy = _t + 0.85f; \
        if (runX) box(T_METAL, V3(_u0, _hy, _h0), V3(_u1 + 0.01f, _hy + 0.05f, _h0 + 0.05f), rgb(110, 112, 110), false, ALL); else box(T_METAL, V3(_h0, _hy, _u0), V3(_h0 + 0.05f, _hy + 0.05f, _u1 + 0.01f), rgb(110, 112, 110), false, ALL); \
        float _r0 = _wh ? _c + 0.01f : _c + SW - 0.05f; \
        if (runX) { box(T_METAL, V3(_u0, _t + 0.9f, _r0), V3(_u1 + 0.01f, _t + 0.96f, _r0 + 0.04f), rgb(120, 124, 120), true, ALL); box(T_METAL, V3(_u0 + 0.02f, _t, _r0 + 0.01f), V3(_u0 + 0.04f, _t + 0.9f, _r0 + 0.03f), rgb(120, 124, 120), false, ALL); } \
        else { box(T_METAL, V3(_r0, _t + 0.9f, _u0), V3(_r0 + 0.04f, _t + 0.96f, _u1 + 0.01f), rgb(120, 124, 120), true, ALL); box(T_METAL, V3(_r0 + 0.01f, _t, _u0 + 0.02f), V3(_r0 + 0.03f, _t + 0.9f, _u0 + 0.04f), rgb(120, 124, 120), false, ALL); } \
        { Vector3 _m = runX ? V3((_u0 + _u1) / 2, 0, _r0 + 0.02f) : V3(_r0 + 0.02f, 0, (_u0 + _u1) / 2); (void)_m; } \
        if (runX) box(T_PLAIN, V3(_u0, _t, _r0), V3(_u1, _t + 0.96f, _r0 + 0.04f), rgb(0, 0, 0), true, 0); else box(T_PLAIN, V3(_r0, _t, _u0), V3(_r0 + 0.04f, _t + 0.96f, _u1), rgb(0, 0, 0), true, 0); \
    } while (0)
    if (runLen < 5.7f || cross < 2 * SW + 2.0f) acc = 1;
    if (kind == 3 || kind == 4) acc = 1;
    // switchback stairwell: two strips along the wall farthest from the street.
    // even flights climb strip A toward +u, odd flights climb strip B back toward -u; each flight starts where the last one ends.
    int back = (street + 2) % 4;
    bool stripHigh;   // true: strips at the max side of the cross axis
    if (runX) stripHigh = back == 2 ? true : back == 0 ? false : ri(2); else stripHigh = back == 1 ? true : back == 3 ? false : ri(2);
    float cMin = runX ? in.y : in.x, cMax = runX ? in.y + in.height : in.x + in.width, uStart = runX ? in.x : in.y;
    float c0A = stripHigh ? cMax - SW : cMin, c0B = stripHigh ? c0A - SW : c0A + SW;
    float bandLo = fminf(c0A, c0B), bandHi = fmaxf(c0A, c0B) + SW;
    const float LEAD = runLen >= 6.7f ? 1.4f : 1.0f;   // the landing at each end of the flights (1.4 m where the building is long enough: room to step off and turn)
    float uL = uStart + LEAD, uH = uL + RUN;
    // partition wall position: on a pillar between exterior wall modules, clear of the stairwell and of the front door
    float partC = -1;
    {
        float A = runX ? r.y + curT : r.x, len = runX ? r.height - 2 * curT : r.width;
        int nm = (int)(len / 3.0f); float mw = nm > 0 ? len / nm : len, best = 1e9f, freeLo = acc > 1 && !stripHigh ? bandHi : cMin, freeHi = acc > 1 && stripHigh ? bandLo : cMax;
        for (int m = 1; m < nm; m++) {
            float c = A + m * mw;
            if (c < cMin + 1.6f || c > cMax - 1.6f) continue;
            if (acc > 1 && c > bandLo - 0.5f && c < bandHi + 0.5f) continue;
            float d = fabsf(c - (freeLo + freeHi) / 2);
            if (d < best) { best = d; partC = c; }
        }
    }
    int ft = kind == 0 ? T_TATAMI : (kind == 3 || kind == 4 || kind == 6) ? T_TILEWALL : T_WOOD;
    Rectangle prevHole = {0, 0, 0, 0};
    // a basement under some buildings: a flight down on the stair strip beyond the flight up, one big room under the whole floor
    float uB0 = uL, uB1 = uH; Rectangle bHole = {0, 0, 0, 0};   // 0.9.8: the flight down runs right under the ground floor's flight up (strip A), entered from the far landing
    const float uE = uH + LEAD;   // the stairwell's far end
    bool basement = acc > 1 && kind != 0 && nPit < MAX_PIT &&
                    (gBasementAll || kind == 6 || (kind == 1 && floors >= 2 && ri(10) < 7) || (kind == 2 && floors >= 2 && ri(10) < 6) || (bP && (bfl & BD_BRICKBASE)) ||
                     (gBasementK > 1 && hash2((int)(uStart * 7 + c0A * 3), floors * 17 + kind, 83) < (gBasementK - 1) * 0.3f));
    if (gBasementK <= 0) basement = false;   // a map can switch them off
    if (basement) {
        float BY = SIDEH - FH, rise = FH / NS, tread = RUN / NS;
        bHole = runX ? (Rectangle){uL - 0.02f, bandLo, uE - uL + 0.02f, bandHi - bandLo} : (Rectangle){bandLo, uL - 0.02f, bandHi - bandLo, uE - uL + 0.02f};   // 0.9.8: the whole shaft beyond the door landing
        { Pit *q = &pits[nPit++]; q->r = bHole; q->f = (short)curXf; q->x0 = q->z0 = 1e9f; q->x1 = q->z1 = -1e9f;
          for (int k = 0; k < 4; k++) { Vector3 w = xfP(curXf, V3(bHole.x + (k & 1) * bHole.width, 0, bHole.y + (k >> 1) * bHole.height)); q->x0 = fminf(q->x0, w.x); q->x1 = fmaxf(q->x1, w.x); q->z0 = fminf(q->z0, w.z); q->z1 = fmaxf(q->z1, w.z); } }
        prevHole = bHole;   // the ground floor's finish is cut there
        {   // 0.9.8: down from the ground floor's door landing the same way round: strip B down to the half landing, strip A back to the basement's door landing
            float hh = FH / 2, rs = hh / NSH, tr = RUN / NSH;
            for (int fl = 0; fl < 2; fl++) { float cS = fl ? c0A : c0B, yS = SIDEH - fl * hh; bool wallHi = fl ? stripHigh : !stripHigh;
                for (int k = 0; k < NSH; k++) { float u0 = fl ? uH - (k + 1) * tr : uL + k * tr, u1 = u0 + tr, top = yS - (k + 1) * rs;
                    STEPBOX(u0, u1, cS, BY - 0.3f, top, wallHi); } }
            if (runX) box(T_CONCRETE, V3(uH, BY - 0.3f, bandLo), V3(uE, SIDEH - hh, bandHi), rgb(150, 146, 140), true, ALL); else box(T_CONCRETE, V3(bandLo, BY - 0.3f, uH), V3(bandHi, SIDEH - hh, uE), rgb(150, 146, 140), true, ALL);   // the half landing
            float mA = c0A + SW / 2, mB = c0B + SW / 2;
            Vector3 t1 = runX ? V3(uL - 0.45f, SIDEH, mB) : V3(mB, SIDEH, uL - 0.45f), b1 = runX ? V3(uH + 0.45f, SIDEH - hh, mB) : V3(mB, SIDEH - hh, uH + 0.45f);
            Vector3 t2 = runX ? V3(uH + 0.45f, SIDEH - hh, mA) : V3(mA, SIDEH - hh, uH + 0.45f), b2 = runX ? V3(uL - 0.45f, BY, mA) : V3(mA, BY, uL - 0.45f);
            if (nStair < MAX_STAIR) stairs[nStair++] = (Stair){xfP(curXf, b1), xfP(curXf, t1)};
            if (nStair < MAX_STAIR) stairs[nStair++] = (Stair){xfP(curXf, b2), xfP(curXf, t2)};
            (void)rise; (void)tread; (void)uB0; (void)uB1; }
        box(T_CONCRETE, V3(r.x, BY - 0.3f, r.y), V3(r.x + r.width, BY, r.y + r.height), rgb(120, 118, 112), true, ALL);      // basement floor
        box(T_CONCRETE, V3(r.x, BY - 0.3f, r.y), V3(r.x + r.width, SIDEH, r.y + curT), rgb(150, 148, 140), true, ALL);    // its walls
        box(T_CONCRETE, V3(r.x, BY - 0.3f, r.y + r.height - curT), V3(r.x + r.width, SIDEH, r.y + r.height), rgb(150, 148, 140), true, ALL);
        box(T_CONCRETE, V3(r.x, BY - 0.3f, r.y), V3(r.x + curT, SIDEH, r.y + r.height), rgb(150, 148, 140), true, ALL);
        box(T_CONCRETE, V3(r.x + r.width - curT, BY - 0.3f, r.y), V3(r.x + r.width, SIDEH, r.y + r.height), rgb(150, 148, 140), true, ALL);
        slabWithHole(T_CONCRETE, in, bHole, SIDEH - 0.32f, SIDEH - 0.02f, rgb(135, 135, 132));                              // its ceiling, open over the stairs
        stairwellWalls(runX, uStart, uE, LEAD, bandLo, bandHi, stripHigh, uStart + runLen, BY, SIDEH, true, true);
        basementPlan(r, in, runX, uStart, runLen, cMin, cMax, stripHigh ? bandLo - 0.1f : bandHi + 0.1f, stripHigh, BY, kind);
    }
    gNoWinR = acc > 1 ? (runX ? (Rectangle){uL - 0.1f, bandLo - 0.3f, uE - uL + 0.2f, bandHi - bandLo + 0.6f} : (Rectangle){bandLo - 0.3f, uL - 0.1f, bandHi - bandLo + 0.6f, uE - uL + 0.2f}) : (Rectangle){0, 0, 0, 0};
    for (int f = 0; f < acc; f++) {
        float y0 = SIDEH + f * FH, yTop = y0 + FH - SLAB;
        extFloor(r, kind, street, y0, yTop, f == 0, extTex, ec);
        {   // floor finish, open over the stairwell coming up from below
            Rectangle fr2 = in, h = prevHole; floorHoleNow = prevHole;
            if (h.width <= 0) box(ft, V3(in.x, y0, in.y), V3(in.x + in.width, y0 + 0.015f, in.y + in.height), f == 0 ? WHITE : rgb(215, 210, 205), false, 16);
            else {
                float hx0 = fmaxf(h.x, fr2.x), hx1 = fminf(h.x + h.width, fr2.x + fr2.width), hz0 = fmaxf(h.y, fr2.y), hz1 = fminf(h.y + h.height, fr2.y + fr2.height);
                Color fc = rgb(215, 210, 205);
                if (hx0 > fr2.x + 0.01f) box(ft, V3(fr2.x, y0, fr2.y), V3(hx0, y0 + 0.015f, fr2.y + fr2.height), fc, false, 16);
                if (hx1 < fr2.x + fr2.width - 0.01f) box(ft, V3(hx1, y0, fr2.y), V3(fr2.x + fr2.width, y0 + 0.015f, fr2.y + fr2.height), fc, false, 16);
                if (hz0 > fr2.y + 0.01f) box(ft, V3(hx0, y0, fr2.y), V3(hx1, y0 + 0.015f, hz0), fc, false, 16);
                if (hz1 < fr2.y + fr2.height - 0.01f) box(ft, V3(hx0, y0, hz1), V3(hx1, y0 + 0.015f, fr2.y + fr2.height), fc, false, 16);
            }
        }
        // flight from this floor up
        Rectangle hole = {0, 0, 0, 0};
        jpArr = jpArrNext; jpArrOk = jpArrNextOk && f > 0; jpArrNextOk = false;   // where the flight from below comes up onto this floor
        if (f < acc - 1) {   // 0.9.8: two half flights per storey round the shaft: up strip A to the half landing at the far end, turn, up strip B back over the door landing
            bool solidUnder = f == 0 && !basement;   // (nothing below the ground floor's flights: built solid down to the floor)
            float hh = FH / 2, rise = hh / NSH, tread = RUN / NSH;
            for (int fl = 0; fl < 2; fl++) { float cS = fl ? c0B : c0A, yS = y0 + fl * hh; bool wallHi = fl ? !stripHigh : stripHigh;
                for (int k = 0; k < NSH; k++) {
                    float u0 = fl ? uH - (k + 1) * tread : uL + k * tread, u1 = u0 + tread, top = yS + (k + 1) * rise, yb = solidUnder ? y0 : top - 0.35f;
                    STEPBOX(u0, u1, cS, yb, top, wallHi); } }
            {   // the half landing across both strips
                float yl = y0 + hh, yb = solidUnder ? y0 : yl - 0.25f;
                if (runX) box(T_CONCRETE, V3(uH, yb, bandLo), V3(uE, yl, bandHi), rgb(165, 160, 155), true, ALL); else box(T_CONCRETE, V3(bandLo, yb, uH), V3(bandHi, yl, uE), rgb(165, 160, 155), true, ALL); }
            hole = runX ? (Rectangle){uL - 0.02f, bandLo, uE - uL + 0.02f, bandHi - bandLo} : (Rectangle){bandLo, uL - 0.02f, bandHi - bandLo, uE - uL + 0.02f};
            {   float mA = c0A + SW / 2, mB = c0B + SW / 2;
                Vector3 b1 = runX ? V3(uL - 0.45f, y0, mA) : V3(mA, y0, uL - 0.45f), t1 = runX ? V3(uH + 0.45f, y0 + hh, mA) : V3(mA, y0 + hh, uH + 0.45f);
                Vector3 b2 = runX ? V3(uH + 0.45f, y0 + hh, mB) : V3(mB, y0 + hh, uH + 0.45f), t2 = runX ? V3(uL - 0.45f, y0 + FH, mB) : V3(mB, y0 + FH, uL - 0.45f);
                if (nStair < MAX_STAIR) stairs[nStair++] = (Stair){xfP(curXf, b1), xfP(curXf, t1)};
                if (nStair < MAX_STAIR) stairs[nStair++] = (Stair){xfP(curXf, b2), xfP(curXf, t2)};
                (void)fr(0, 1);   // (the old junk draw, kept so the rest of the town comes out the same)
                jpArrNext = (Vector2){t2.x, t2.z}; jpArrNextOk = true; }
        } else if (f > 0) {   // the top floor: a rail across strip A where the shaft opens below the landing
            float yU = y0, ry = yU + 1.0f; Color rc = rgb(110, 112, 110);
            if (runX) box(T_METAL, V3(uL - 0.04f, yU, c0A), V3(uL + 0.02f, ry, c0A + SW), rc, true, ALL); else box(T_METAL, V3(c0A, yU, uL - 0.04f), V3(c0A + SW, ry, uL + 0.02f), rc, true, ALL);
        }
        // ceiling / next floor slab with the stairwell cut out
        prevHole = hole;
        slabWithHole(T_CONCRETE, r, hole, yTop, yTop + SLAB, rgb(150, 150, 150));
        { float ext = 0;   // (0.9.7: the flight down is under the flights up)
          fAvoid = acc > 1 ? (runX ? (Rectangle){uL - LEAD, bandLo - 0.15f, RUN + 2 * LEAD + ext, bandHi - bandLo + 0.3f} : (Rectangle){bandLo - 0.15f, uL - LEAD, bandHi - bandLo + 0.3f, RUN + 2 * LEAD + ext}) : (Rectangle){0, 0, 0, 0}; }
        // houses and flats: a Japanese plan (hall, LDK, washitsu, toilet, washroom, bath, bedrooms); other buildings: one partition, two rooms
        float jpDoor; { bool aX = street == 0 || street == 2; float A = aX ? r.x : r.y + curT, len = aX ? r.width : r.height - 2 * curT; int nm = (int)(len / 3.0f); if (nm < 1) nm = 1; jpDoor = A + (nm / 2 + 0.5f) * (len / nm); }
        bool jpTry = (kind == 0 || kind == 1 || kind == 2 || kind == 3 || kind == 4 || kind == 6) && ((street == 0 || street == 2) ? in.width : in.height) >= 5.0f && ((street == 0 || street == 2) ? in.height : in.width) >= 4.6f;
        // one partition wall splitting the floor into two rooms, with a doorway (sometimes piled shut)
        nKeep = 0;
        if (partC > 0 && !jpTry) {
            float a = runX ? in.x : in.y, b = runX ? in.x + in.width : in.y + in.height;
            float gap = fr(a + 0.8f, b - 1.9f);
            Color pc = kind == 0 ? rgb(225, 215, 195) : rgb(200, 200, 196);
            int pt = kind == 0 ? T_PLASTER : T_CONCRETE;
            float mc = partC;
            { Rectangle kc = runX ? (Rectangle){gap - 0.6f, mc - 1.3f, 2.3f, 2.6f} : (Rectangle){mc - 1.3f, gap - 0.6f, 2.6f, 2.3f}; keepAdd(kc, kc); }
            if (runX) { box(pt, V3(r.x, y0, mc - 0.06f), V3(gap, yTop, mc + 0.06f), pc, true, ALL); box(pt, V3(gap + 1.1f, y0, mc - 0.06f), V3(r.x + r.width, yTop, mc + 0.06f), pc, true, ALL);
                box(pt, V3(gap, y0 + 2.2f, mc - 0.06f), V3(gap + 1.1f, yTop, mc + 0.06f), pc, false, ALLB); }
            else { box(pt, V3(mc - 0.06f, y0, r.y), V3(mc + 0.06f, yTop, gap), pc, true, ALL); box(pt, V3(mc - 0.06f, y0, gap + 1.1f), V3(mc + 0.06f, yTop, r.y + r.height), pc, true, ALL);
                box(pt, V3(mc - 0.06f, y0 + 2.2f, gap), V3(mc + 0.06f, yTop, gap + 1.1f), pc, false, ALLB); }
            if (f > 0 && fr(0, 1) < 0.18f) { if (runX) addDebris(V3(gap, y0, mc - 0.5f), V3(gap + 1.1f, y0 + 1.1f, mc + 0.5f)); else addDebris(V3(mc - 0.5f, y0, gap), V3(mc + 0.5f, y0 + 1.1f, gap + 1.1f)); }
        }
        // designed rooms: the stairwell stays clear, each room gets a plan for this kind of building and storey
        if (f == 0 && (kind == 0 || kind == 1) && nKeep < 7) {   // the genkan inside the front door: a tiled step-down, the raised edge, a shoe cabinet
            int sd = street; bool aX = sd == 0 || sd == 2; float A = aX ? r.x : r.y + curT, len = aX ? r.width : r.height - 2 * curT; int nm = (int)(len / 3.0f); if (nm < 1) nm = 1; float mw = len / nm, uc = A + (nm / 2 + 0.5f) * mw;
            float D = 1.3f; Rectangle g = sd == 0 ? (Rectangle){uc - 0.8f, in.y, 1.6f, D} : sd == 2 ? (Rectangle){uc - 0.8f, in.y + in.height - D, 1.6f, D} : sd == 1 ? (Rectangle){in.x + in.width - D, uc - 0.8f, D, 1.6f} : (Rectangle){in.x, uc - 0.8f, D, 1.6f};
            bool gav = g.x < fAvoid.x + fAvoid.width && g.x + g.width > fAvoid.x && g.y < fAvoid.y + fAvoid.height && g.y + g.height > fAvoid.y;
            if (!gav && boxFree(g.x + 0.05f, g.y + 0.05f, g.x + g.width - 0.05f, g.y + g.height - 0.05f, y0)) {   // (it stands in front of the door on purpose)
                box(T_TILEWALL, V3(g.x, y0 + 0.017f, g.y), V3(g.x + g.width, y0 + 0.038f, g.y + g.height), rgb(120, 118, 112), false, 16);
                Rectangle e = sd == 0 ? (Rectangle){g.x, g.y + D, g.width, 0.12f} : sd == 2 ? (Rectangle){g.x, g.y - 0.12f, g.width, 0.12f} : sd == 1 ? (Rectangle){g.x - 0.12f, g.y, 0.12f, g.height} : (Rectangle){g.x + D, g.y, 0.12f, g.height};
                box(T_WOOD, V3(e.x, y0, e.y), V3(e.x + e.width, y0 + 0.14f, e.y + e.height), rgb(140, 100, 66), true, ALL);   // agarikamachi: you step up into the house
                Rectangle sb = aX ? (Rectangle){g.x + g.width, sd == 0 ? g.y : g.y + D - 0.38f, 0.9f, 0.38f} : (Rectangle){sd == 3 ? g.x : g.x + D - 0.38f, g.y + g.height, 0.38f, 0.9f};
                if (!jpTry && fFree(sb.x, sb.y, sb.x + sb.width, sb.y + sb.height, y0)) { box(T_WOOD, V3(sb.x, y0, sb.y), V3(sb.x + sb.width, y0 + 0.95f, sb.y + sb.height), rgb(160, 125, 90), true, ALL);
                    box(T_PLAIN, V3(sb.x + 0.1f, y0 + 0.95f, sb.y + 0.08f), V3(sb.x + 0.3f, y0 + 1.15f, sb.y + 0.25f), rgb(90, 130, 90), false, ALL); }   // a pot plant on top
                for (int k = 0; k < 2 + ri(3); k++) { float sx = g.x + fr(0.2f, g.width - 0.4f), sz = g.y + fr(0.2f, g.height - 0.4f); box(T_PLAIN, V3(sx, y0 + 0.026f, sz), V3(sx + 0.12f, y0 + 0.1f, sz + 0.28f), (Color[]){{40, 40, 44, 255}, {200, 200, 205, 255}, {150, 60, 50, 255}}[ri(3)], false, ALLB); }   // shoes left behind
                { Rectangle kc = {g.x - 0.4f, g.y - 0.4f, g.width + 0.8f, g.height + 0.8f}; keepAdd(kc, kc); } } }
        {   Rectangle rA = in, rB = in; int oA, oB; bool two = partC > 0;
            if (two) { if (runX) { rA.height = partC - in.y - 0.06f; rB.y = partC + 0.06f; rB.height = in.y + in.height - rB.y; oA = 0; oB = 2; }
                       else { rA.width = partC - in.x - 0.06f; rB.x = partC + 0.06f; rB.width = in.x + in.width - rB.x; oA = 3; oB = 1; } }
            else { oA = runX ? (street == 0 ? 2 : 0) : (street == 3 ? 1 : 3); oB = oA; }
            bool aFront = two && (oA == street);   // which room has the street wall
            Rectangle front = two ? (aFront ? rA : rB) : in, back = two ? (aFront ? rB : rA) : in; int oF = two ? (aFront ? oA : oB) : oA, oBk = two ? (aFront ? oB : oA) : oA;
            int pF, pB;
            if (kind == 3) { pF = RM_KONBINI; pB = RM_STOCK; }
            else if (kind == 4) { pF = RM_KOBAN; pB = RM_STOCK; }
            else if (kind == 6) { pF = RM_OFFICE; pB = RM_OFFICE; }
            else if (kind == 2) { pF = f == 0 ? RM_SHOP : RM_KITCHEN; pB = f == 0 ? RM_STOCK : RM_BED; }
            else if (kind == 1) { pF = f % 2 ? RM_WBED : RM_LDK; pB = RM_WASHWET; }   // every flat: an LDK (or a western room) at the front, tatami and the wet rooms behind
            else { pF = f == 0 ? RM_LDK : RM_WBED; pB = f == 0 ? RM_WASHWET : RM_WASHITSU; }   // a house: LDK and genkan below, bedrooms above
            if (kind == 2 && f > 0) { pF = RM_LDK; pB = RM_WASHWET; }   // the family lives over the shop
            if (!two && kind != 3 && kind != 4) { pF = f == 0 ? (kind == 2 ? RM_SHOP : RM_LDK) : (kind == 0 ? RM_WASHITSU : RM_WBED); }
            gStNear = runX ? (Vector2){uStart + LEAD / 2, (bandLo + bandHi) / 2} : (Vector2){(bandLo + bandHi) / 2, uStart + LEAD / 2};
            bool jpDone = jpTry && jpFloor(in, street, fAvoid, acc > 1, y0, yTop, f, kind, jpDoor, kind);
            if (acc > 1) stairwellWalls(runX, uStart, uE, LEAD, bandLo, bandHi, stripHigh, uStart + runLen, y0, f < acc - 1 ? y0 + FH : yTop, false, !jpDone);   // (up through the slab: the shaft is open floor to floor)   // 0.9.7: the stairs in a stairwell of their own, a door at each landing (a planned floor builds it with its rooms)
            if (!jpDone) { planRoom(front, y0, pF, oF, kind, street, kind == 0);
                if (two) planRoom(back, y0, pB, oBk, kind, street, kind == 0); }
            {   // the dead: someone who never got out
                Vector3 wq = xfP(curXf, V3(in.x, y0, in.y)); unsigned int sd = (unsigned int)((int)(wq.x * 7.3f) * 92821 + (int)(wq.z * 5.1f) * 68917 + f * 131 + worldSeed);
                float pr = (kind == 4 ? 0.45f : kind == 6 ? 0.22f : kind == 1 ? 0.2f : kind == 0 ? 0.2f : 0.08f) * gBodyK; if (wq.x < -185 && wq.z > 110 && wq.z < 300) pr *= 1.8f;
                if ((float)((sd * 2246822519u) >> 8 & 0xffff) / 65536.0f < pr) placeBody(front, y0, kind == 4 ? 2 : (kind == 6 && (sd & 3) == 0) ? 3 : 1, 0, sd); }
            if ((kind <= 2) && ri(10) < 5) { Rectangle q = two && ri(2) ? back : front;   // blood on the floor: a patch, sometimes a trail to the door
                if (q.width > 1.6f && q.height > 1.6f) { Vector3 c = V3(fr(q.x + 0.6f, q.x + q.width - 0.6f), y0, fr(q.y + 0.6f, q.y + q.height - 0.6f)); stain(c, fr(0.4f, 0.9f));
                    if (ri(2)) for (int k = 1; k <= 3; k++) stain(V3(c.x + (q.x + q.width / 2 - c.x) * k * 0.25f, y0, c.z + (q.y + q.height / 2 - c.z) * k * 0.25f), 0.22f); } }
        }
        // light (most upper floors stay dark)
        if (f == 0 || fr(0, 1) < 0.3f) addLight(V3(in.x + in.width / 2, yTop - 0.3f, in.y + in.height / 2), kind == 6 ? V3(0.5f, 0.9f, 0.85f) : V3(0.9f, 0.85f, 0.7f), 9, fr(0, 1) < 0.55f ? 1 : (fr(0, 1) < (kind <= 1 ? 0.5f : 0.3f) ? 2 : 0));   // houses: half the bulbs that aren't flickering are dead
        // a television on the ground floor of homes
        if (f == 0 && (kind == 0 || kind == 1) && !jpTry && nTV < MAX_TV && in.width > 3) {
            float tx = in.x + in.width - 1.4f, tz = stripHigh && runX ? in.y + 0.45f : in.y + 0.45f;
            if (pointFree(tx, tz, 0.6f, y0 + 0.05f)) {
                box(T_WOOD, V3(tx - 0.55f, y0, tz - 0.3f), V3(tx + 0.55f, y0 + 0.5f, tz + 0.3f), rgb(150, 120, 100), true, ALL);
                addSolid(V3(tx - 0.4f, y0 + 0.5f, tz - 0.25f), V3(tx + 0.4f, y0 + 1.1f, tz + 0.25f));
                tvs[nTV++] = (TV){xfP(curXf, V3(tx, y0 + 0.8f, tz + 0.26f)), xfYaw(curXf)};
            }
        }
    }
    float gh = GH;
    float top = SIDEH + acc * FH;
    if (floors > acc) {   // unreachable upper storeys: solid facade
        float h = (floors - acc) * FH;
        int ft2 = kind == 6 ? T_ECO : (kind == 1) ? (ri(2) ? T_FACADE : T_FACADE2) : (ri(2) ? T_FACADE2 : T_FACADE);
        box(ft2, V3(r.x, top, r.y), V3(r.x + r.width, top + h, r.y + r.height), ec, true, 15);
        top += h;
    }
    box(T_ROOF, V3(r.x, top - 0.01f, r.y), V3(r.x + r.width, top + 0.07f, r.y + r.height), WHITE, false, 16);
    if (bfl & (BD_PITCH | BD_HIP)) { Color rc = bP && bP->roof.a ? bP->roof : (Color[]){{80, 70, 70, 255}, {60, 80, 120, 255}, {110, 60, 50, 255}, {70, 70, 80, 255}, {60, 95, 75, 255}, {72, 74, 80, 255}}[ri(6)];
        if (bfl & BD_HIP) hipRoof(r, top, rc); else pitchedRoof(r, top, rc); }
    else if (floors > 1) {
        box(T_CONCRETE, V3(r.x, top, r.y), V3(r.x + r.width, top + 0.9f, r.y + 0.2f), ec, true, ALL);
        box(T_CONCRETE, V3(r.x, top, r.y + r.height - 0.2f), V3(r.x + r.width, top + 0.9f, r.y + r.height), ec, true, ALL);
        box(T_CONCRETE, V3(r.x, top, r.y), V3(r.x + 0.2f, top + 0.9f, r.y + r.height), ec, true, ALL);
        box(T_CONCRETE, V3(r.x + r.width - 0.2f, top, r.y), V3(r.x + r.width, top + 0.9f, r.y + r.height), ec, true, ALL);
        if (kind == 1) { float wx = r.x + r.width * 0.3f, wz = r.y + r.height * 0.4f; box(T_RUST, V3(wx, top, wz), V3(wx + 2.4f, top + 2.6f, wz + 2.4f), WHITE, true, ALL); }
        if (kind == 6) {
            for (int k = 0; k < 7; k++) { float hx = fr(r.x + 1, r.x + r.width - 3), hz = fr(r.y + 1, r.y + r.height - 3), hs = fr(1.0f, 2.4f);
                box(T_HEDGE, V3(hx, top, hz), V3(hx + hs, top + fr(0.6f, 1.3f), hz + hs * fr(0.5f, 1.2f)), WHITE, true, ALL); }
            box(T_GLASS, V3(r.x + 1, top + 0.4f, r.y + r.height - 4), V3(r.x + r.width * 0.5f, top + 0.5f, r.y + r.height - 1.2f), rgb(80, 120, 170), true, ALL);
        }
    } else {
        for (int k = 0; k < 2; k++) { float ax = fr(r.x + 1, r.x + r.width - 2), az = fr(r.y + 1, r.y + r.height - 2);
            box(T_METAL, V3(ax, top, az), V3(ax + 0.9f, top + 0.65f, az + 0.35f), rgb(170, 170, 160), true, ALL); }
    }
    if (kind == 4) addLight(V3(r.x + r.width / 2, gh + 0.6f, r.y - 0.3f), V3(1.0f, 0.1f, 0.05f), 7, 1);
    // signage
    if (kind == 2 || kind == 3 || kind == 4 || kind == 5) {
        if (nSign < MAX_SIGN) {
            Vector3 mx, mn = sideMn(street, (street == 0 || street == 2 ? r.x : r.y) + 0.6f, (street == 0 || street == 2 ? r.x + r.width : r.y + r.height) - 0.6f, gh + 0.32f, gh + 1.2f, &mx, 0);
            float o = (street == 0 || street == 3) ? -0.18f : 0.18f;
            if (street == 0 || street == 2) { if (o < 0) { mn.z -= 0.18f; mx.z = mn.z + 0.12f; } else { mx.z += 0.18f; mn.z = mx.z - 0.12f; } }
            else { if (o < 0) { mn.x -= 0.18f; mx.x = mn.x + 0.12f; } else { mx.x += 0.18f; mn.x = mx.x - 0.12f; } }
            Color tints[6] = {{255, 70, 60, 255}, {70, 220, 255, 255}, {255, 210, 70, 255}, {120, 255, 140, 255}, {255, 120, 220, 255}, {240, 240, 255, 255}};
            Color tc = kind == 3 ? (Color){90, 200, 255, 255} : kind == 5 ? (Color){240, 240, 255, 255} : tints[ri(6)];
            Vector3 c = Vector3Scale(Vector3Add(mn, mx), 0.5f);
            Vector3 out = (street == 0) ? V3(0, 0, -1) : (street == 2) ? V3(0, 0, 1) : (street == 1) ? V3(1, 0, 0) : V3(-1, 0, 0);
            float rr = fr(0, 1);
            int li = rr < 0.2f ? -1 : addLight(Vector3Add(c, Vector3Scale(out, 1.0f)), V3(tc.r / 255.0f * 0.9f, tc.g / 255.0f * 0.9f, tc.b / 255.0f * 0.9f), 8, rr < 0.5f ? 1 : 0);
            signs[nSign++] = (Sign){mn, mx, tc, li, T_SIGNH, kind == 3 ? SGN_KONBINI : kind == 4 ? SGN_KOBAN : kind == 5 ? SGN_STATION : ri(JP_NSHOP), street, false, (short)curXf};
        }
    }
    if (kind == 6) {   // public-information screen on the street face
        float ang = street == 0 ? PI : street == 2 ? 0 : street == 1 ? PI / 2 : -PI / 2;
        Vector3 out = V3(sinf(ang), 0, cosf(ang));
        float fx = street == 1 ? r.x + r.width : street == 3 ? r.x : r.x + r.width / 2;
        float fz = street == 0 ? r.y : street == 2 ? r.y + r.height : r.y + r.height / 2;
        addScreen(Vector3Add(V3(fx, gh + 4.2f, fz), Vector3Scale(out, 0.06f)), fminf(7.0f, (street == 0 || street == 2 ? r.width : r.height) - 2), 3.6f, ang, false);
    }
    if ((kind == 2 || kind == 1) && floors > 1 && nSign < MAX_SIGN && fr(0, 1) < 0.7f) {   // vertical sign
        bool ax = (street == 0 || street == 2);
        float u = ax ? r.x + 0.4f : r.y + 0.4f;
        float face = (street == 0) ? r.y : (street == 2) ? r.y + r.height : (street == 1) ? r.x + r.width : r.x;
        float dir = (street == 0 || street == 3) ? -1.0f : 1.0f;
        Vector3 mn, mx;
        float y0s = gh + 1.4f, y1s = y0s + 3.0f;
        if (ax) { mn = V3(u, y0s, fminf(face, face + dir * 0.95f)); mx = V3(u + 0.14f, y1s, fmaxf(face, face + dir * 0.95f)); }
        else { mn = V3(fminf(face, face + dir * 0.95f), y0s, u); mx = V3(fmaxf(face, face + dir * 0.95f), y1s, u + 0.14f); }
        Color tints[4] = {{255, 80, 70, 255}, {80, 230, 255, 255}, {255, 220, 90, 255}, {255, 130, 230, 255}};
        Color tc = tints[ri(4)];
        Vector3 c = Vector3Scale(Vector3Add(mn, mx), 0.5f);
        int li = fr(0, 1) < 0.35f ? -1 : addLight(c, V3(tc.r / 255.0f * 0.8f, tc.g / 255.0f * 0.8f, tc.b / 255.0f * 0.8f), 9, ri(2));
        signs[nSign++] = (Sign){mn, mx, tc, li, T_SIGN, ri(JP_NSHOP), street, false, (short)curXf};
    }
    bdMode = true; bDetails(r, kind, floors, street, top, ec, bfl); bdMode = false;
}

// ---------------------------------------------------------------- subway
static Vector2 subEnt[4]; static int nSubEnt;   // street-level mouths of the subway stairs, for the map
static void subwayEntrance(float x0, float z0) {   // stairwell from the sidewalk down to the platform level of the line under road 3
    float hx0 = x0 + 14, hx1 = x0 + 24, hz0 = z0 + 0.3f, hz1 = z0 + 2.3f;
    if (nSubEnt < 4) { subEnt[nSubEnt++] = (Vector2){hx0 + 4, (hz0 + hz1) / 2}; TraceLog(LOG_INFO, "SUBENT %.1f %.1f", hx0, (hz0 + hz1) / 2); }
    if (nPit < MAX_PIT) { Pit *q = &pits[nPit++]; q->r = (Rectangle){hx0, hz0, hx1 - hx0, hz1 - hz0}; q->f = 0; q->x0 = hx0; q->x1 = hx1; q->z0 = hz0; q->z1 = hz1; }   // the ground is open over the stairs
    if (nSubRes < 4) subRes[nSubRes++] = (Rectangle){hx0 - 1.6f, hz0 - 0.8f, hx1 - hx0 + 2.4f, hz1 - hz0 + 2.0f};   // nothing else is put down here
    const float PY = SUBY + 0.9f;
    const int N = 24; float rise = (SIDEH - PY) / N, tread = 8.1f / N;
    for (int k = 0; k < N; k++) {
        float u0 = hx0 + k * tread, top = SIDEH - (k + 1) * rise;
        box(T_CONCRETE, V3(u0, SUBY - 0.3f, hz0), V3(u0 + tread, top, hz1), rgb(150, 150, 145), true, ALL);
    }
    box(T_TILEWALL, V3(hx0 + 8.1f, SUBY - 0.3f, hz0), V3(hx1, PY, hz1), rgb(190, 200, 200), true, ALL);   // landing
    // earth walls around the shaft
    float cx0 = hx0 + 8.2f, cx1 = hx1 - 0.1f;
    box(T_TILEWALL, V3(hx0 - 0.3f, SUBY - 0.3f, hz0 - 0.3f), V3(cx0, SIDEH - 0.01f, hz0), rgb(180, 190, 190), true, ALL);
    box(T_TILEWALL, V3(cx1, SUBY - 0.3f, hz0 - 0.3f), V3(hx1 + 0.3f, SIDEH - 0.01f, hz0), rgb(180, 190, 190), true, ALL);
    box(T_TILEWALL, V3(cx0, PY + 3.0f, hz0 - 0.3f), V3(cx1, SIDEH - 0.01f, hz0), rgb(180, 190, 190), true, ALL);
    box(T_TILEWALL, V3(hx0 - 0.3f, SUBY - 0.3f, hz1), V3(hx1 + 0.3f, SIDEH - 0.01f, hz1 + 0.3f), rgb(180, 190, 190), true, ALL);
    box(T_TILEWALL, V3(hx1, SUBY - 0.3f, hz0), V3(hx1 + 0.3f, SIDEH - 0.01f, hz1), rgb(180, 190, 190), true, ALL);
    box(T_TILEWALL, V3(hx0 - 0.3f, SUBY - 0.3f, hz0), V3(hx0, SIDEH - 0.01f, hz1), rgb(180, 190, 190), true, ALL);
    // corridor from the landing under the road edge into the tunnel
    box(T_TILEWALL, V3(cx0, SUBY - 0.3f, TUN_Z1), V3(cx1, PY, hz0), rgb(190, 200, 200), true, ALL);
    box(T_CONCRETE, V3(cx0 - 0.3f, PY + 3.0f, TUN_Z1), V3(cx1 + 0.3f, PY + 3.4f, hz0), rgb(120, 125, 125), true, ALLB);
    box(T_TILEWALL, V3(cx0 - 0.3f, PY, TUN_Z1 + 0.4f), V3(cx0, PY + 3.0f, hz0), rgb(180, 190, 190), true, ALL);
    box(T_TILEWALL, V3(cx1, PY, TUN_Z1 + 0.4f), V3(cx1 + 0.3f, PY + 3.0f, hz0), rgb(180, 190, 190), true, ALL);
    // railings and canopy at street level, open at the top of the stairs
    box(T_METAL, V3(hx0, SIDEH, hz0 - 0.08f), V3(hx1, SIDEH + 1.0f, hz0), rgb(170, 180, 180), true, ALL);
    box(T_METAL, V3(hx0, SIDEH, hz1), V3(hx1, SIDEH + 1.0f, hz1 + 0.08f), rgb(170, 180, 180), true, ALL);
    box(T_METAL, V3(hx1, SIDEH, hz0), V3(hx1 + 0.08f, SIDEH + 1.0f, hz1), rgb(170, 180, 180), true, ALL);
    box(T_GLASS, V3(hx0 - 0.2f, SIDEH + 2.6f, hz0 - 0.3f), V3(hx0 + 3.0f, SIDEH + 2.7f, hz1 + 0.3f), rgb(120, 190, 220), false, ALLB);
    if (nSign < MAX_SIGN) { int li = addLight(V3(hx0 + 0.5f, SIDEH + 2.2f, (hz0 + hz1) / 2), V3(0.4f, 0.8f, 1.0f), 6, 1);
        signs[nSign++] = (Sign){V3(hx0 - 0.25f, SIDEH + 1.4f, hz0 + 0.3f), V3(hx0 - 0.15f, SIDEH + 2.4f, hz1 - 0.3f), (Color){80, 200, 255, 255}, li, T_SIGN, SGN_SUBWAY, 3, false}; }
    {   // tall lit totem at the stair mouth: a green-striped post with a glowing box on top, seen from down the street
        float tx = hx0 - 0.9f, tz = hz1 + 0.6f;
        box(T_METAL, V3(tx - 0.12f, SIDEH, tz - 0.12f), V3(tx + 0.12f, SIDEH + 4.2f, tz + 0.12f), rgb(70, 76, 78), true, ALL);
        box(T_PLAIN, V3(tx - 0.13f, SIDEH + 2.8f, tz - 0.13f), V3(tx + 0.13f, SIDEH + 3.0f, tz + 0.13f), rgb(60, 170, 90), false, ALL);
        box(T_SIGN, V3(tx - 0.45f, SIDEH + 4.2f, tz - 0.45f), V3(tx + 0.45f, SIDEH + 5.1f, tz + 0.45f), rgb(120, 230, 160), false, ALL);
        addLight(V3(tx, SIDEH + 4.6f, tz), V3(0.45f, 1.0f, 0.65f), 9, 0);
        addLight(V3(hx0 + 4, SIDEH - 1.6f, (hz0 + hz1) / 2), V3(0.7f, 0.85f, 0.95f), 6, 1); }   // flickering tube halfway down the shaft
    if (nStair < MAX_STAIR) stairs[nStair++] = (Stair){V3(hx0 + 9.0f, PY, (hz0 + hz1) / 2), V3(hx0 - 0.6f, SIDEH, (hz0 + hz1) / 2)};
    if (nStair < MAX_STAIR) stairs[nStair++] = (Stair){V3((cx0 + cx1) / 2, PY, TUN_Z1 - 1.0f), V3((cx0 + cx1) / 2, PY, hz0 + 0.5f)};
}
static void genSubway(void) {
    float X0 = subX0, X1 = subX1;
    box(T_CONCRETE, V3(X0, SUBY - 0.4f, TUN_Z0 - 0.4f), V3(X1, SUBY, TUN_Z1 + 0.4f), rgb(110, 110, 105), true, ALL);                // track bed
    box(T_CONCRETE, V3(X0, SUBY + 4.2f, TUN_Z0 - 0.4f), V3(X1, SUBY + 4.6f, TUN_Z1 + 0.4f), rgb(90, 92, 92), true, ALLB);          // ceiling
    box(T_TILEWALL, V3(X0, SUBY, TUN_Z0 - 0.4f), V3(X1, SUBY + 4.2f, TUN_Z0), rgb(170, 182, 182), true, ALL);                       // far wall
    float gaps[3]; for (int g = 0; g < nSubEx; g++) gaps[g] = subEx[g] + 22.2f;
    float wx = X0;
    for (int g = 0; g < nSubEx; g++) { box(T_TILEWALL, V3(wx, SUBY, TUN_Z1), V3(gaps[g], SUBY + 4.2f, TUN_Z1 + 0.4f), rgb(170, 182, 182), true, ALL); wx = gaps[g] + 1.7f;
        box(T_TILEWALL, V3(gaps[g], SUBY + 3.9f, TUN_Z1), V3(wx, SUBY + 4.2f, TUN_Z1 + 0.4f), rgb(170, 182, 182), false, ALLB); }
    box(T_TILEWALL, V3(wx, SUBY, TUN_Z1), V3(X1, SUBY + 4.2f, TUN_Z1 + 0.4f), rgb(170, 182, 182), true, ALL);
    box(T_CONCRETE, V3(X0 - 0.4f, SUBY, TUN_Z0 - 0.4f), V3(X0, SUBY + 4.2f, TUN_Z1 + 0.4f), rgb(90, 92, 92), true, ALL);
    box(T_CONCRETE, V3(X1, SUBY, TUN_Z0 - 0.4f), V3(X1 + 0.4f, SUBY + 4.2f, TUN_Z1 + 0.4f), rgb(90, 92, 92), true, ALL);
    // platform along the near side, rails along the far side
    float PZ = TUN_Z1 - 3.4f;
    box(T_TILEWALL, V3(X0, SUBY, PZ), V3(X1, SUBY + 0.9f, TUN_Z1), rgb(200, 205, 200), true, ALL);
    box(T_PLAIN, V3(X0, SUBY + 0.9f, PZ), V3(X1, SUBY + 0.93f, PZ + 0.25f), rgb(220, 190, 60), false, 16);
    for (int r = 0; r < 2; r++) { float z = TUN_Z0 + 1.0f + r * 1.1f;
        box(T_METAL, V3(X0, SUBY, z), V3(X1, SUBY + 0.12f, z + 0.08f), rgb(130, 120, 110), false, ALL); }
    for (float x = X0 + 2; x < X1 - 1; x += 1.4f) box(T_WOOD, V3(x, SUBY, TUN_Z0 + 0.8f), V3(x + 0.25f, SUBY + 0.05f, TUN_Z0 + 2.5f), rgb(90, 75, 60), false, 16);
    // steps from track bed up to platform every 40 m (so creatures and you can climb out)
    for (float x = X0 + 12; x < X1 - 10; x += 40) { box(T_CONCRETE, V3(x, SUBY, PZ - 0.5f), V3(x + 1.2f, SUBY + 0.45f, PZ), rgb(150, 150, 145), true, ALL); }
    // pillars, lights, furniture
    for (float x = X0 + 8; x < X1 - 4; x += 12) {
        box(T_TILEWALL, V3(x, SUBY + 0.9f, PZ + 1.4f), V3(x + 0.5f, SUBY + 4.2f, PZ + 1.9f), rgb(185, 195, 195), true, ALL);
        float r = fr(0, 1); addLight(V3(x + 0.25f, SUBY + 3.6f, PZ + 1.0f), V3(0.75f, 0.9f, 0.85f), 8, r < 0.25f ? 0 : r < 0.6f ? 1 : 2);
    }
    float stY = SUBY + 0.9f;
    for (int k = 0; k < 26; k++) {
        float x = fr(X0 + 4, X1 - 4);
        int ck = fr(0, 1) < 0.45f ? CK_LOCKER : fr(0, 1) < 0.5f ? CK_BOX : fr(0, 1) < 0.5f ? CK_FIRECAB : CK_COUNTER;
        float z = TUN_Z1 - ckSize[ck].z / 2 - 0.02f;
        if (pointFree(x, z, 0.6f, stY + 0.05f)) addCont(ck, V3(x, stY, z), PI, 9, true);
    }
    // a dead train: shell with open doors and seats
    float tx0 = floorf(X0 + (X1 - X0) * 0.55f), tx1 = tx0 + 18, tz0 = TUN_Z0 + 0.5f, tz1 = PZ - 0.2f;
    box(T_METAL, V3(tx0, SUBY + 0.4f, tz0), V3(tx1, SUBY + 0.6f, tz1), rgb(170, 180, 185), true, ALL);                      // floor
    box(T_METAL, V3(tx0, SUBY + 3.3f, tz0), V3(tx1, SUBY + 3.5f, tz1), rgb(190, 200, 205), true, ALLB);                     // roof
    box(T_METAL, V3(tx0, SUBY + 0.6f, tz0), V3(tx1, SUBY + 3.3f, tz0 + 0.12f), rgb(200, 210, 215), true, ALL);               // far side
    for (float x = tx0; x < tx1; x += 4.5f) { float d0 = x + 1.5f, d1 = x + 2.8f;
        box(T_METAL, V3(x, SUBY + 0.6f, tz1 - 0.12f), V3(fminf(d0, tx1), SUBY + 3.3f, tz1), rgb(200, 210, 215), true, ALL);
        if (d1 < tx1) box(T_METAL, V3(d1, SUBY + 0.6f, tz1 - 0.12f), V3(fminf(x + 4.5f, tx1), SUBY + 3.3f, tz1), rgb(200, 210, 215), true, ALL);
        box(T_CLOTH, V3(x + 0.2f, SUBY + 0.6f, tz0 + 0.15f), V3(x + 1.3f, SUBY + 1.05f, tz0 + 0.65f), rgb(80, 120, 160), true, ALL); }
    box(T_METAL, V3(tx0 - 0.12f, SUBY + 0.6f, tz0), V3(tx0, SUBY + 3.3f, tz1), rgb(200, 210, 215), true, ALL);
    box(T_METAL, V3(tx1, SUBY + 0.6f, tz0), V3(tx1 + 0.12f, SUBY + 3.3f, tz1), rgb(200, 210, 215), true, ALL);
    box(T_PLAIN, V3(tx0, SUBY + 2.0f, tz1 + 0.001f), V3(tx1, SUBY + 2.15f, tz1 + 0.01f), rgb(60, 170, 90), false, ALL);   // green stripe
    addCont(CK_LOCKER, V3(tx1 - 1.0f, SUBY + 0.6f, tz0 + 0.4f), 0, 9, true);
    addLight(V3((tx0 + tx1) / 2, SUBY + 3.0f, (tz0 + tz1) / 2), V3(0.8f, 0.95f, 0.9f), 9, 1);
    nSubEnt = 0;
    for (int b = 0; b < nSubEx; b++) subwayEntrance(subEx[b], SUB_Z + ROADW);
}
static void spawnUnderground(void) {
    if (nSubEx == 0) return;
    for (int k = 0; k < 9; k++) {
        float x = fr(subX0 + 6, subX1 - 6);
        if (Vector3Distance(V3(x, SUBY, 0), V3(pl.pos.x, SUBY, 0)) < 25 && pl.pos.y < -2) continue;
        float r = fr(0, 1);
        int t = r < 0.3f ? 2 : r < 0.55f ? 0 : r < 0.8f ? 8 : r < 0.92f ? 6 : 5;
        bool onPlat = fr(0, 1) < 0.5f;
        Vector3 p = V3(x, onPlat ? SUBY + 0.9f : SUBY, onPlat ? TUN_Z1 - 1.7f : TUN_Z0 + 1.6f);
        int s = spawnAt(p, t, false);
        if (s >= 0 && t == 8) { crs[s].hanging = true; crs[s].pos.y = SUBY + 3.8f; }
    }
    static bool attendant = false;
    if (!attendant || fr(0, 1) < 0.15f) { int s = spawnAt(V3(floorf(subX0 + (subX1 - subX0) * 0.55f) - 4, SUBY + 0.9f, TUN_Z1 - 1.6f), 9, false); (void)s; attendant = true; }
}

// ---------------------------------------------------------------- v0.4: map footprints, water sources, day light
typedef struct { Vector3 pos; int kind; } WSrc;   // kind 0 rain barrel, 1 park fountain, 2 hill spring
#define MAX_WS 200
static WSrc wsrc[MAX_WS]; static int nWs;
static int worldSolids, worldLights;              // counts right after generation (save files restore on top of these)
static void genWater(void) {
    nWs = 0;
    for (int f = 0; f < nMapFoot && nWs < MAX_WS - 12; f += 3) {   // rain barrels beside the houses
        Rectangle r = mapFoot[f]; if (mapFootY[f] > 1) continue; curXf = mapFootF[f];   // test and place in the building's own frame
        for (int tries = 0; tries < 24; tries++) { int s = (f + tries) % 4; float u = 0.15f + 0.7f * ((tries / 4) % 6) / 5.0f, off = tries < 12 ? 0.75f : 1.4f;
            float x = s == 1 ? r.x + r.width + off : s == 3 ? r.x - off : r.x + r.width * u, z = s == 2 ? r.y + r.height + off : s == 0 ? r.y - off : r.y + r.height * u;
            if (!cityLand(x, z) || !pointFree(x, z, 0.4f, SIDEH + 0.05f)) continue;
            bool near = false; for (int k = 0; k < nWs; k++) if (Vector3Distance(wsrc[k].pos, xfP(curXf, V3(x, SIDEH, z))) < 9) near = true;
            if (near) continue;
            wsrc[nWs++] = (WSrc){xfP(curXf, V3(x, SIDEH, z)), 0}; addSolid(V3(x - 0.32f, SIDEH, z - 0.32f), V3(x + 0.32f, SIDEH + 0.95f, z + 0.32f)); break; }
        curXf = 0;
    }
    if (DPARKS_N > 0 && nWs < MAX_WS) {   // the fountain in the waterfront park
        float x = dParks[0][0], z = dParks[0][1];
        bool ok = false; for (int tries = 0; tries < 120 && !ok; tries++) { if (pointFree(x, z, 0.8f, SIDEH + 0.05f) && !mainRoadAt(x, z, 1.2f) && cityLand(x, z)) ok = true; else { float rr = tries < 60 ? 12 : 22; x = dParks[0][0] + fr(-rr, rr); z = dParks[0][1] + fr(-rr, rr); } }
        if (ok) { wsrc[nWs++] = (WSrc){V3(x, SIDEH, z), 1}; addSolid(V3(x - 0.75f, SIDEH, z - 0.75f), V3(x + 0.75f, SIDEH + 0.6f, z + 0.75f)); }   // never on a street
    }
    for (int k = 0; k < 7 && nWs < MAX_WS; k++) {   // clear springs in the hills (the water there is fine; the hills are not)
        float a = fr(0, 2 * PI), d = fr(30, 110);
        float x = CITY / 2 + cosf(a) * (CITY / 2 + d), z = CITY / 2 + sinf(a) * (CITY / 2 + d);
        x = clampf(x, -WEST + 10, CITY + WILD - 10); z = clampf(z, -WILD + 10, CITY + WILD - 10);
        if (cityDist(x, z) < 25 || seaDepth(x, z) > 0 || terrainH(x, z) < 1 || terrainH(x, z) > 20) continue;
        wsrc[nWs++] = (WSrc){V3(x, terrainH(x, z), z), 2};
    }
}
static float dayK(void) {   // 0 night, 1 full (overcast) day
    float h = pl.tod;
    if (h < 5 || h > 19.5f) return 0;
    if (h < 7) { float k = (h - 5) / 2; return k * k * (3 - 2 * k); }
    if (h > 17.5f) { float k = (h - 17.5f) / 2; return 1 - k * k * (3 - 2 * k); }
    return 1;
}
