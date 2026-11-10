// zk_world.h — multi-floor buildings (rooms, stairs, junk piles, searchable furniture), subway, the Aero wilds outside the city
#define FH 3.2f          // floor-to-floor height
#define SLAB 0.2f
#define WILD 150.0f      // walkable wilds beyond the city edge
#define WEST 440.0f      // the world reaches this far west: the high-rise district beyond the west tunnel (0.9.0)
#define SUBY -7.0f       // subway floor
#define TUN_Z0 (SUB_ROW * PITCH + 1.0f)
#define TUN_Z1 (SUB_ROW * PITCH + ROADW - 1.0f)

// ---------------------------------------------------------------- ground holes (subway stairwells)
#define MAX_HOLE 8
static Rectangle holes[MAX_HOLE]; static int nHole;
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
static float baseGroundY(float x, float z, float y) {
    if (cityLand(x, z)) { if (inHole(x, z) || inPit(x, z)) return -1e9f; float r = roadRastAt(x, z); return r > SIDEH + 0.05f && y > r - 1.0f ? r : SIDEH; }   // (a road climbing out of town starts on town land)
    int b = inBore(x, z, y); if (b) return bores[b - 1].fy;
    float h = terrainH(x, z);
    { float r = roadRastAt(x, z); if (r > h && y > r - 1.0f) return r; }   // a road or paved strip out of town: the drawn surface, not the coarser terrain under it
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
static void genTerrain(void) {
    if (terrainOk) { UnloadModel(terrain); terrainOk = false; }
    const float S = HF_S; int n = HF_N - 1;
    unsigned char *keep = calloc((size_t)n * n, 1); int quads = 0;
    for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) {
        float ax = HF_X0 + i * S, az = HF_X0 + j * S;
        bool land = cityLand(ax + 0.1f, az + 0.1f) && cityLand(ax + S - 0.1f, az + 0.1f) && cityLand(ax + 0.1f, az + S - 0.1f) && cityLand(ax + S - 0.1f, az + S - 0.1f);
        bool paved = inCore(ax + 0.1f, az + 0.1f) && inCore(ax + S - 0.1f, az + 0.1f) && inCore(ax + 0.1f, az + S - 0.1f) && inCore(ax + S - 0.1f, az + S - 0.1f);   // paving quads cover it
        if (land || paved || hfOut(ax, az) || terrainSkip(ax, az)) continue;
        keep[j * n + i] = 1; quads++;
    }
    Mesh m = {0};
    m.vertexCount = quads * 6; m.triangleCount = quads * 2;
    m.vertices = MemAlloc(m.vertexCount * 3 * sizeof(float)); m.texcoords = MemAlloc(m.vertexCount * 2 * sizeof(float)); m.normals = MemAlloc(m.vertexCount * 3 * sizeof(float));
    m.colors = MemAlloc(m.vertexCount * 4);
    int v = 0;
    for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) {
        if (!keep[j * n + i]) continue;
        int vi[4] = {i, i, i + 1, i + 1}, vj[4] = {j, j + 1, j + 1, j};
        int ord[6] = {0, 1, 2, 0, 2, 3};
        for (int k = 0; k < 6; k++) {
            int gi = vi[ord[k]], gj = vj[ord[k]];
            float x = HF_X0 + gi * S, z = HF_X0 + gj * S, y = hf[gj * HF_N + gi];
            float hx = hf[gj * HF_N + (gi < HF_N - 1 ? gi + 1 : gi)] - hf[gj * HF_N + (gi > 0 ? gi - 1 : gi)];
            float hz = hf[(gj < HF_N - 1 ? gj + 1 : gj) * HF_N + gi] - hf[(gj > 0 ? gj - 1 : gj) * HF_N + gi];
            Vector3 nn = Vector3Normalize(V3(-hx, 2 * S, -hz));
            m.vertices[v * 3] = x; m.vertices[v * 3 + 1] = y; m.vertices[v * 3 + 2] = z;
            m.normals[v * 3] = nn.x; m.normals[v * 3 + 1] = nn.y; m.normals[v * 3 + 2] = nn.z;
            m.texcoords[v * 2] = x / 5; m.texcoords[v * 2 + 1] = z / 5;
            float sh = 0.82f + 0.18f * nn.y;
            // tint: wet sand at the waterline, dark cedar on the mountain, grey rock near its top
            float r = 1, g = 1, b = 1, flat = 0;   // flat > 0: the vertex colour replaces the grass texture by that much (see the shader)
            if (!cityLand(x, z)) {   // blended per vertex, so there are no saw-tooth edges between grass, sand and concrete
                float sand = clampf((-1.6f - y) / 1.2f, 0, 1);   // sand below the seawall and under the water
                float face = 0; if (hfReady && y > -4.4f && y < SIDEH) { float cd = coast_d(x, z); face = clampf((SIDEH - 0.35f - y) / 0.5f, 0, 1) * clampf((y + 3.9f) / 0.5f, 0, 1) * clampf((12 - cd) / 3, 0, 1); }   // the seawall's concrete
                float sr = Lerp(0.78f, 0.6f, face), sg = Lerp(0.72f, 0.6f, face), sb = Lerp(0.56f, 0.58f, face); flat = fmaxf(sand, face);
                r = Lerp(r, sr, flat); g = Lerp(g, sg, flat); b = Lerp(b, sb, flat); }
            float mt = clampf((y - 12) / 14, 0, 1); r = Lerp(r, 0.55f, mt); g = Lerp(g, 0.72f, mt); b = Lerp(b, 0.62f, mt);
            float rk = clampf((y - 34) / 10, 0, 1); r = Lerp(r, 1.45f, rk); g = Lerp(g, 0.95f, rk); b = Lerp(b, 1.05f, rk);
            m.colors[v * 4] = (unsigned char)clampf(255 * sh * r, 0, 255); m.colors[v * 4 + 1] = (unsigned char)clampf(255 * sh * g, 0, 255);
            m.colors[v * 4 + 2] = (unsigned char)clampf(255 * sh * b, 0, 255); m.colors[v * 4 + 3] = flat > 0.01f ? (unsigned char)(255 * (0.6f - 0.09f * flat)) : 255;
            v++;
        }
    }
    free(keep);
    UploadMesh(&m, false);
    terrain = LoadModelFromMesh(m);
    terrain.materials[0].shader = sh;
    if (!blissTex.id) blissTex = makeBliss();
    terrain.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = blissTex;
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
static void extFloor(Rectangle r, int kind, int street, float y0, float yTop, bool ground, int extTex, Color ec) {
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
            if (storefront || fr(0, 1) < winP) {
                float p = storefront ? 0.15f : 0.4f;
                float sill = storefront ? 0.35f : 0.9f, top = storefront ? 2.6f : 2.1f;
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
    for (int k = 0; k < qn; k++) { Solid *s = &sol[qb[k]]; if (s->off || s->mx.y <= y + 0.05f || s->mn.y >= y + 1.8f) continue;
        float a0 = x0, b0 = z0, a1 = x1, b1 = z1;
        if (s->f != curXf) {   // the footprint's corners in the solid's frame (a slightly larger box: conservative)
            a0 = b0 = 1e9f; a1 = b1 = -1e9f;
            for (int q = 0; q < 4; q++) { Vector3 L = solL(s, xfP(curXf, V3(q & 1 ? x1 : x0, 0, q & 2 ? z1 : z0))); a0 = fminf(a0, L.x); a1 = fmaxf(a1, L.x); b0 = fminf(b0, L.z); b1 = fmaxf(b1, L.z); } }
        if (a0 < s->mx.x && a1 > s->mn.x && b0 < s->mx.z && b1 > s->mn.z) return false; }
    return true;
}
// place a searchable piece of furniture against an inside wall of `in` at floor y
static Rectangle keepClear[8]; static int nKeep;   // doorways on the floor being furnished
static bool blocksDoor(float x0, float z0, float x1, float z1, float y) {
    for (int k = 0; k < nKeep; k++) { Rectangle c = keepClear[k]; if (x0 < c.x + c.width && x1 > c.x && z0 < c.y + c.height && z1 > c.y) return true; }
    for (int i = 0; i < nOps; i++) { Open *o = &ops[i]; if (o->type != OP_DOOR || o->f != curXf || fabsf(o->mn.y - y) > 0.6f) continue;   // keep 1.2 m clear around every door
        if (x0 < o->mx.x + 1.2f && x1 > o->mn.x - 1.2f && z0 < o->mx.z + 1.2f && z1 > o->mn.z - 1.2f) return true; }
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
static bool putPiece(int k, Rectangle rm, int wall, float u, float y, int region) {
    float w, d, h = 0; if (k >= 0) { w = ckSize[k].x; d = ckSize[k].z; } else { w = fixTab[-k].w; d = fixTab[-k].d; h = fixTab[-k].h; }
    float x0, z0, x1, z1; wallFoot(rm, wall, u, w, d, &x0, &z0, &x1, &z1);
    if (x0 < rm.x || x1 > rm.x + rm.width || z0 < rm.y || z1 > rm.y + rm.height) return false;
    if (!fFree(x0, z0, x1, z1, y) || !frontFree(wall, x0, z0, x1, z1, y, k == FX_BOARD || k == FX_SCREEN ? 0.3f : 0.75f)) return false;
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
    if (!fFree(cx - w / 2, cz - d / 2, cx + w / 2, cz + d / 2, y)) return false;
    box(tex, V3(cx - w / 2, y, cz - d / 2), V3(cx + w / 2, y + h, cz + d / 2), c, solid, ALL); return true;
}
static void chair(float cx, float cz, float y, int face, Color c) {   // seat + backrest; face: direction the sitter looks (0 +z,1 -x,2 -z,3 +x)
    if (!fFree(cx - 0.24f, cz - 0.24f, cx + 0.24f, cz + 0.24f, y)) return;
    box(T_WOOD, V3(cx - 0.22f, y, cz - 0.22f), V3(cx + 0.22f, y + 0.45f, cz + 0.22f), c, true, ALL);
    float bx0 = cx - 0.22f, bx1 = cx + 0.22f, bz0 = cz - 0.22f, bz1 = cz + 0.22f;
    if (face == 0) bz1 = bz0 + 0.06f; else if (face == 2) bz0 = bz1 - 0.06f; else if (face == 1) bx0 = bx1 - 0.06f; else bx1 = bx0 + 0.06f;
    box(T_WOOD, V3(bx0, y + 0.45f, bz0), V3(bx1, y + 0.95f, bz1), c, false, ALLB);
}
static void tableSet(float cx, float cz, float y, bool alongX, int seats, float h, Color tc) {   // table with chairs (h 0.72) or a low chabudai with zabuton (h 0.35)
    float w = alongX ? 1.4f : 0.8f, d = alongX ? 0.8f : 1.4f; if (h < 0.5f) { w = alongX ? 1.1f : 0.8f; d = alongX ? 0.8f : 1.1f; }
    if (!fFree(cx - w / 2 - 0.5f, cz - d / 2 - 0.5f, cx + w / 2 + 0.5f, cz + d / 2 + 0.5f, y)) return;
    box(T_WOOD, V3(cx - w / 2, y + h - 0.05f, cz - d / 2), V3(cx + w / 2, y + h, cz + d / 2), tc, true, ALL);
    box(T_WOOD, V3(cx - 0.06f, y, cz - 0.06f), V3(cx + 0.06f, y + h - 0.05f, cz + 0.06f), aMul(tc, 0.7f), false, ALLB);
    for (int s2 = 0; s2 < seats; s2++) { float ox = alongX ? (s2 % 2 ? 0.35f : -0.35f) : (s2 < 2 ? -w / 2 - 0.35f : w / 2 + 0.35f), oz = alongX ? (s2 < 2 ? -d / 2 - 0.35f : d / 2 + 0.35f) : (s2 % 2 ? 0.35f : -0.35f);
        int face = alongX ? (s2 < 2 ? 0 : 2) : (s2 < 2 ? 3 : 1);
        if (h < 0.5f) box(T_CLOTH, V3(cx + ox - 0.27f, y, cz + oz - 0.27f), V3(cx + ox + 0.27f, y + 0.09f, cz + oz + 0.27f), rgb(150, 60, 50), false, ALLB);   // zabuton
        else chair(cx + ox, cz + oz, y, face, rgb(140, 105, 80)); }
}
static void futon(Rectangle rm, int wall, float u, float y, Color cover) {
    float x0, z0, x1, z1; bool alongX = wall == 1 || wall == 3; wallFoot(rm, wall, u, 1.0f, 2.0f, &x0, &z0, &x1, &z1);
    if (alongX) { float t; t = x0; (void)t; }
    if (!fFree(x0, z0, x1, z1, y)) return;
    box(T_CLOTH, V3(x0, y, z0), V3(x1, y + 0.12f, z1), rgb(236, 232, 222), false, ALLB);
    float k = 0.35f; box(T_CLOTH, V3(x0 + 0.02f, y + 0.12f, z0 + (wall == 0 ? 0.55f : 0.02f)), V3(x1 - 0.02f, y + 0.2f, z1 - (wall == 2 ? 0.55f : 0.02f)), cover, false, ALLB); (void)k;
}
// ---- Japanese room pieces (0.9.0)
static bool wallBox(Rectangle rm, int wall, float u, float w, float d, float y0, float h, int tex, Color c, bool solid) {   // a w x d piece against a wall, if it fits
    float x0, z0, x1, z1; wallFoot(rm, wall, u, w, d, &x0, &z0, &x1, &z1);
    if (x0 < rm.x || x1 > rm.x + rm.width || z0 < rm.y || z1 > rm.y + rm.height || !fFree(x0, z0, x1, z1, y0)) return false;
    box(tex, V3(x0, y0, z0), V3(x1, y0 + h, z1), c, solid, ALL); return true;
}
static float wallAt(Rectangle rm, int wall, float frac) { return wallLo(rm, wall) + (wallHi(rm, wall) - wallLo(rm, wall)) * frac; }
static Rectangle floorHoleNow;   // the stairwell opening in the floor being furnished (from the flight below), in the building's frame
static void floorFinish(Rectangle rm, float y, int tex, Color c) {   // a room's floor covering, cut round the stairwell
    Rectangle h = floorHoleNow;
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
static void tokonoma(Rectangle rm, int wall, float y) {   // the alcove: a raised board, a hanging scroll, a vase
    float u = wallAt(rm, wall, 0.75f), x0, z0, x1, z1; wallFoot(rm, wall, u, 1.6f, 0.75f, &x0, &z0, &x1, &z1);
    if (x0 < rm.x || x1 > rm.x + rm.width || z0 < rm.y || z1 > rm.y + rm.height || !fFree(x0, z0, x1, z1, y)) return;
    box(T_WOOD, V3(x0, y, z0), V3(x1, y + 0.14f, z1), rgb(120, 85, 55), true, ALL);
    float sx0, sz0, sx1, sz1; wallFoot(rm, wall, u, 0.5f, 0.02f, &sx0, &sz0, &sx1, &sz1); box(T_PLAIN, V3(sx0, y + 0.6f, sz0), V3(sx1, y + 1.9f, sz1), rgb(226, 218, 196), false, ALL);
    { float ix0, iz0, ix1, iz1; wallFoot(rm, wall, u, 0.06f, 0.03f, &ix0, &iz0, &ix1, &iz1); box(T_PLAIN, V3(ix0, y + 0.9f, iz0), V3(ix1, y + 1.6f, iz1), rgb(30, 28, 26), false, ALL); }   // one brush stroke
    float vx = (x0 + x1) / 2 + (wall == 0 || wall == 2 ? 0.45f : 0), vz = (z0 + z1) / 2 + (wall == 1 || wall == 3 ? 0.45f : 0); box(T_PLAIN, V3(vx - 0.08f, y + 0.14f, vz - 0.08f), V3(vx + 0.08f, y + 0.45f, vz + 0.08f), rgb(70, 90, 110), false, ALL);
}
static void butsudan(Rectangle rm, int wall, float y) {   // the family altar: black lacquer, gold inside, a photograph
    float u = wallAt(rm, wall, 0.25f), x0, z0, x1, z1; wallFoot(rm, wall, u, 0.7f, 0.5f, &x0, &z0, &x1, &z1);
    if (x0 < rm.x || x1 > rm.x + rm.width || z0 < rm.y || z1 > rm.y + rm.height || !fFree(x0, z0, x1, z1, y)) return;
    box(T_WOOD, V3(x0, y, z0), V3(x1, y + 1.3f, z1), rgb(30, 22, 20), true, ALL);
    float fx0 = x0, fx1 = x1, fz0 = z0, fz1 = z1; float e = 0.01f; if (wall == 0) fz0 = fz1 - e; else if (wall == 2) fz1 = fz0 + e; else if (wall == 1) fx1 = fx0 + e; else fx0 = fx1 - e;
    box(T_PLAIN, V3(fx0 + (wall == 0 || wall == 2 ? 0.1f : -0.005f), y + 0.55f, fz0 + (wall == 1 || wall == 3 ? 0.1f : -0.005f)), V3(fx1 - (wall == 0 || wall == 2 ? 0.1f : -0.005f), y + 1.15f, fz1 - (wall == 1 || wall == 3 ? 0.1f : -0.005f)), rgb(200, 160, 60), false, ALL);
}
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

// room plans
enum { RM_LIVING, RM_KITCHEN, RM_BED, RM_SHOP, RM_STOCK, RM_KONBINI, RM_KOBAN, RM_OFFICE, RM_STUDY, RM_LDK, RM_WASHITSU, RM_WASHWET, RM_WBED, RM_TOILET, RM_SENMEN, RM_BATH, RM_HALL, RM_NANDO };
static void planRoom(Rectangle rm, float y, int plan, int outer, int region, int street, bool tatami) {
    int sideA = (outer + 1) % 4, sideB = (outer + 3) % 4;
    float cx = rm.x + rm.width / 2, cz = rm.y + rm.height / 2; bool longX = rm.width >= rm.height;
    switch (plan) {
    case RM_KITCHEN: { int run[] = {CK_FRIDGE, CK_CABINET, FX_SINK, FX_STOVE, CK_CABINET}; runAlong(rm, outer, y, run, 5, 0, region);
        int s2[] = {FX_TANSU}; runAlong(rm, sideA, y, s2, 1, 1, region);
        float ox = outer == 1 ? -0.8f : outer == 3 ? 0.8f : 0, oz = outer == 0 ? 0.8f : outer == 2 ? -0.8f : 0; tableSet(cx + ox, cz + oz, y, longX, 4, 0.72f, rgb(170, 130, 96)); } break;
    case RM_LIVING: { tableSet(cx, cz, y, longX, 4, tatami ? 0.35f : 0.72f, tatami ? rgb(120, 80, 54) : rgb(170, 130, 96));
        int run[] = {FX_BOOKS, CK_CABINET, FX_PLANT}; runAlong(rm, outer, y, run, 3, 0, region);
        int r2[] = {FX_TANSU, CK_BOX}; runAlong(rm, sideA, y, r2, 2, 0, region); } break;
    case RM_BED: { futon(rm, outer, wallLo(rm, outer) + 0.8f, y, rgb(70, 100, 150)); if (rm.width * rm.height > 14) futon(rm, outer, wallLo(rm, outer) + 2.0f, y, rgb(150, 80, 90));
        int r1[] = {CK_WARDROBE, CK_DESK}; runAlong(rm, sideA, y, r1, 2, 0, region);
        int r2[] = {FX_BOOKS, CK_BOX}; runAlong(rm, sideB, y, r2, 2, 1, region); } break;
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
    case RM_LDK: {   // living-dining-kitchen: a wood floor, the kitchen run along the outer wall, table and chairs, a sofa facing the TV
        floorFinish(rm, y, T_WOOD, rgb(196, 160, 120));
        int run[] = {CK_FRIDGE, CK_CABINET, FX_SINK, FX_STOVE, CK_CABINET}; runAlong(rm, outer, y, run, 5, 0, region);
        float ox = outer == 1 ? -1.0f : outer == 3 ? 1.0f : 0, oz = outer == 0 ? 1.0f : outer == 2 ? -1.0f : 0; tableSet(cx + ox * 0.3f + (longX ? -rm.width * 0.18f : 0), cz + oz * 0.3f + (longX ? 0 : -rm.height * 0.18f), y, longX, 4, 0.72f, rgb(170, 130, 96));
        sofa(rm, opp(outer), wallAt(rm, opp(outer), 0.7f), y, (Color[]){{120, 110, 100, 255}, {90, 110, 130, 255}, {140, 90, 80, 255}}[ri(3)]);
        int r2[] = {CK_CABINET, FX_PLANT}; runAlong(rm, sideA, y, r2, 2, 1, region); } break;
    case RM_WASHITSU: {   // tatami room: tokonoma, butsudan, oshiire (the closet), a low table; at night the futon
        floorFinish(rm, y, T_TATAMI, WHITE); tokonoma(rm, sideA, y); butsudan(rm, sideA, y);
        int r1[] = {CK_WARDROBE, CK_WARDROBE}; runAlong(rm, sideB, y, r1, 2, 0, region);
        if (ri(2)) futon(rm, outer, wallLo(rm, outer) + 1.2f, y, rgb(70, 100, 150)); else tableSet(cx, cz, y, longX, 4, 0.35f, rgb(120, 80, 54)); } break;
    case RM_WASHWET: {   // the back room: bathroom, toilet and washroom behind a partition at one end, a tatami room in the rest
        int ws = sideA; if (!wetStrip(rm, sideA, y, region)) { ws = sideB; if (!wetStrip(rm, sideB, y, region)) { ws = outer; if (!wetStrip(rm, outer, y, region)) ws = -1; } }   // whichever end is clear of the stairs and the doorway
        bool alongX = ws == 0 || ws == 2; Rectangle rest = rm; if (ws >= 0) { if (alongX) { if (ws == 0) { rest.y += 1.8f; } rest.height -= 1.8f; } else { if (ws == 3) { rest.x += 1.8f; } rest.width -= 1.8f; } }
        floorFinish(rest, y, T_TATAMI, WHITE); tokonoma(rest, ws >= 0 ? opp(ws) : sideA, y);
        int r1[] = {CK_WARDROBE}; runAlong(rest, outer, y, r1, 1, 1, region);
        tableSet(rest.x + rest.width / 2, rest.y + rest.height / 2, y, rest.width >= rest.height, 2, 0.35f, rgb(120, 80, 54)); } break;
    case RM_TOILET: {   // the toilet room: tiled, a western bowl with its tank on the far wall, a tiny hand basin, slippers
        floorFinish(rm, y, T_TILEWALL, rgb(214, 216, 212));
        int fw = rm.width < rm.height ? (rm.y + rm.height / 2 > cz ? 2 : 0) : 1; fw = outer;   // against the outer wall
        wallBox(rm, fw, wallAt(rm, fw, 0.5f), 0.42f, 0.62f, y, 0.42f, T_PLAIN, rgb(240, 240, 238), true); wallBox(rm, fw, wallAt(rm, fw, 0.5f), 0.46f, 0.2f, y + 0.42f, 0.45f, T_PLAIN, rgb(232, 232, 228), false);
        wallBox(rm, sideA, wallAt(rm, sideA, 0.7f), 0.36f, 0.22f, y + 0.75f, 0.12f, T_PLAIN, rgb(236, 236, 234), false);
        { float x0, z0, x1, z1; wallFoot(rm, opp(fw), wallAt(rm, opp(fw), 0.5f), 0.3f, 0.3f, &x0, &z0, &x1, &z1); box(T_PLAIN, V3(x0, y + 0.024f, z0), V3(x1, y + 0.06f, z1), rgb(90, 140, 120), false, ALL); } } break;
    case RM_SENMEN: {   // the washroom (datsuijo): basin vanity, the washing machine, a towel rack; the bath opens off it
        floorFinish(rm, y, T_TILEWALL, rgb(210, 214, 216));
        int r1[] = {CK_CABINET}; runAlong(rm, outer, y, r1, 1, 0, region);
        if (!wallBox(rm, sideA, wallAt(rm, sideA, 0.35f), 0.62f, 0.62f, y, 0.9f, T_PLAIN, rgb(238, 238, 236), true)) wallBox(rm, sideB, wallAt(rm, sideB, 0.35f), 0.62f, 0.62f, y, 0.9f, T_PLAIN, rgb(238, 238, 236), true);
        wallBox(rm, outer, wallAt(rm, outer, 0.5f), 0.7f, 0.12f, y + 1.2f, 0.7f, T_GLASS, rgb(170, 190, 200), false); } break;   // the mirror
    case RM_BATH: {   // the bathroom (ofuro): a deep tub against the outer wall, the washing area with a stool, a hand shower
        floorFinish(rm, y, T_TILEWALL, rgb(196, 206, 210));
        float x0, z0, x1, z1; bool lx = rm.width >= rm.height; int tw = lx ? (outer == 1 || outer == 3 ? outer : sideA) : (outer == 0 || outer == 2 ? outer : sideA);
        wallFoot(rm, tw, wallAt(rm, tw, 0.5f), (tw == 0 || tw == 2) ? rm.width - 0.1f : rm.height - 0.1f, 0.8f, &x0, &z0, &x1, &z1);
        if (fFree(x0, z0, x1, z1, y)) { box(T_PLAIN, V3(x0, y, z0), V3(x1, y + 0.58f, z1), rgb(236, 238, 238), true, ALL); box(T_PLAIN, V3(x0 + 0.08f, y + 0.581f, z0 + 0.08f), V3(x1 - 0.08f, y + 0.59f, z1 - 0.08f), rgb(110, 140, 150), false, 16); }
        { float sx = cx - (tw == 1 ? 0.5f : tw == 3 ? -0.5f : 0), sz = cz - (tw == 2 ? 0.5f : tw == 0 ? -0.5f : 0); box(T_PLAIN, V3(sx - 0.15f, y, sz - 0.15f), V3(sx + 0.15f, y + 0.25f, sz + 0.15f), rgb(150, 190, 200), false, ALL); }
        wallBox(rm, opp(tw), wallAt(rm, opp(tw), 0.5f), 0.08f, 0.08f, y + 0.9f, 0.6f, T_METAL, rgb(190, 192, 196), false); } break;
    case RM_HALL: floorFinish(rm, y, T_WOOD, rgb(170, 130, 92)); break;   // the corridor (rouka): polished boards
    case RM_NANDO: { floorFinish(rm, y, T_WOOD, rgb(160, 124, 90)); int r1[] = {CK_BOX, FX_CRATES, CK_BOX}; runAlong(rm, outer, y, r1, 3, 0, region); int r2[] = {CK_WARDROBE}; runAlong(rm, sideA, y, r2, 1, 0, region); } break;   // the store room
    case RM_WBED: {   // a western bedroom upstairs: wood floor, a bed, desk, wardrobe
        floorFinish(rm, y, T_WOOD, rgb(186, 150, 112));
        { float x0, z0, x1, z1; wallFoot(rm, sideA, wallAt(rm, sideA, 0.3f), 1.0f, 2.0f, &x0, &z0, &x1, &z1);
          if (x0 >= rm.x && x1 <= rm.x + rm.width && z0 >= rm.y && z1 <= rm.y + rm.height && fFree(x0, z0, x1, z1, y)) { box(T_WOOD, V3(x0, y, z0), V3(x1, y + 0.3f, z1), rgb(130, 95, 70), true, ALL);
              box(T_CLOTH, V3(x0 + 0.03f, y + 0.3f, z0 + 0.03f), V3(x1 - 0.03f, y + 0.48f, z1 - 0.03f), (Color[]){{200, 200, 210, 255}, {140, 160, 190, 255}, {200, 170, 170, 255}}[ri(3)], false, ALLB); } }
        int r1[] = {CK_DESK, FX_BOOKS}; runAlong(rm, outer, y, r1, 2, 0, region); int r2[] = {CK_WARDROBE, CK_BOX}; runAlong(rm, sideB, y, r2, 2, 0, region); } break;
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
static void fBox(Rectangle r, int face, float u0, float u1, float y0, float y1, float o0, float o1, int t, Color c, bool solid) {   // a box on one face: u along the face, o outward from the wall
    Vector3 mn, mx;
    if (face == 0) { mn = V3(r.x + u0, y0, r.y - o1); mx = V3(r.x + u1, y1, r.y - o0); }
    else if (face == 2) { mn = V3(r.x + u0, y0, r.y + r.height + o0); mx = V3(r.x + u1, y1, r.y + r.height + o1); }
    else if (face == 1) { mn = V3(r.x + r.width + o0, y0, r.y + u0); mx = V3(r.x + r.width + o1, y1, r.y + u1); }
    else { mn = V3(r.x - o1, y0, r.y + u0); mx = V3(r.x - o0, y1, r.y + u1); }
    box(t, mn, mx, c, solid, ALL);
}
static float fLen(Rectangle r, int face) { return (face == 0 || face == 2) ? r.width : r.height; }
static void bDetails(Rectangle r, int kind, int floors, int street, float top, Color ec, unsigned fl) {   // the clutter that makes a Japanese street: balconies, laundry, AC units, pipes, tanks
    int side = (street + 1) % 4, back = (street + 2) % 4; float L = fLen(r, street), Ls = fLen(r, side);
    Color dark = (Color){(unsigned char)(ec.r * 0.7f), (unsigned char)(ec.g * 0.7f), (unsigned char)(ec.b * 0.7f), 255};
    if ((fl & BD_BRICKBASE)) fBox(r, street, 0, L, SIDEH, SIDEH + 1.0f, 0, 0.06f, T_BRICK, WHITE, false);
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
static void jpWallU(const JpF *J, float u, float d0, float d1, float door) {   // a partition at constant u from d0 to d1, a doorway centred at d = door (<0: none)
    if (d1 - d0 < 0.05f) return;
    if (jpDump) { if (door >= 0) { jpDumpRect("W", jpRect(J, u - 0.05f, d0, u + 0.05f, door - 0.4f)); jpDumpRect("W", jpRect(J, u - 0.05f, door + 0.4f, u + 0.05f, d1)); jpDumpRect("D", jpRect(J, u - 0.05f, door - 0.4f, u + 0.05f, door + 0.4f)); } else jpDumpRect("W", jpRect(J, u - 0.05f, d0, u + 0.05f, d1)); }
    float a = d0, b = door >= 0 ? door - 0.4f : d1;
    if (b - a > 0.02f) { Rectangle q = jpRect(J, u - 0.05f, a, u + 0.05f, b); box(jpPT, V3(q.x, jpY0, q.y), V3(q.x + q.width, jpYT, q.y + q.height), jpPC, true, ALL); }
    if (door >= 0) { Rectangle l = jpRect(J, u - 0.05f, door - 0.4f, u + 0.05f, door + 0.4f); box(jpPT, V3(l.x, jpY0 + 2.05f, l.y), V3(l.x + l.width, jpYT, l.y + l.height), jpPC, false, ALLB);
        a = door + 0.4f; if (d1 - a > 0.02f) { Rectangle q = jpRect(J, u - 0.05f, a, u + 0.05f, d1); box(jpPT, V3(q.x, jpY0, q.y), V3(q.x + q.width, jpYT, q.y + q.height), jpPC, true, ALL); }
        if (nKeep < 8) { Rectangle k = jpRect(J, u - 0.7f, door - 0.55f, u + 0.7f, door + 0.55f); keepClear[nKeep++] = k; } }
}
static void jpWallD(const JpF *J, float d, float u0, float u1, float door) {   // a partition at constant d from u0 to u1
    if (u1 - u0 < 0.05f) return;
    if (jpDump) { if (door >= 0) { jpDumpRect("W", jpRect(J, u0, d - 0.05f, door - 0.4f, d + 0.05f)); jpDumpRect("W", jpRect(J, door + 0.4f, d - 0.05f, u1, d + 0.05f)); jpDumpRect("D", jpRect(J, door - 0.4f, d - 0.05f, door + 0.4f, d + 0.05f)); } else jpDumpRect("W", jpRect(J, u0, d - 0.05f, u1, d + 0.05f)); }
    float a = u0, b = door >= 0 ? door - 0.4f : u1;
    if (b - a > 0.02f) { Rectangle q = jpRect(J, a, d - 0.05f, b, d + 0.05f); box(jpPT, V3(q.x, jpY0, q.y), V3(q.x + q.width, jpYT, q.y + q.height), jpPC, true, ALL); }
    if (door >= 0) { Rectangle l = jpRect(J, door - 0.4f, d - 0.05f, door + 0.4f, d + 0.05f); box(jpPT, V3(l.x, jpY0 + 2.05f, l.y), V3(l.x + l.width, jpYT, l.y + l.height), jpPC, false, ALLB);
        a = door + 0.4f; if (u1 - a > 0.02f) { Rectangle q = jpRect(J, a, d - 0.05f, u1, d + 0.05f); box(jpPT, V3(q.x, jpY0, q.y), V3(q.x + q.width, jpYT, q.y + q.height), jpPC, true, ALL); }
        if (nKeep < 8) { Rectangle k = jpRect(J, door - 0.55f, d - 0.7f, door + 0.55f, d + 0.7f); keepClear[nKeep++] = k; } }
}
static int jpRooms; static Rectangle jpRoomR[12]; static int jpRoomP[12], jpRoomO[12];
static void jpRoom(const JpF *J, float u0, float d0, float u1, float d1, int plan, int outerWhich) {
    if (jpRooms >= 12 || u1 - u0 < 0.7f || d1 - d0 < 0.7f) return;
    Rectangle r = jpRect(J, u0 + 0.05f, d0 + 0.05f, u1 - 0.05f, d1 - 0.05f); jpRoomR[jpRooms] = r; jpRoomP[jpRooms] = plan; jpRoomO[jpRooms] = jpSide(J, outerWhich); jpRooms++;
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
// returns true when the floor was laid out (the caller then skips its two-room plan)
static bool jpFloor(Rectangle in, int street, Rectangle stairR, bool hasStairs, float y0, float yTop, int f, int kind, float doorLocal, int region) {
    JpF J = {in, street, (street == 0 || street == 2) ? in.width : in.height, (street == 0 || street == 2) ? in.height : in.width};
    if (J.W < 5.0f || J.D < 4.6f) return false;
    if (jpDumpN < 0) { jpDumpN = 0; if (getenv("KIRI_PLANDUMP")) jpDump = fopen(getenv("KIRI_PLANDUMP"), "w"); }
    if (jpDump && (jpDumpN >= 40 || (f > 1))) { if (jpDumpN >= 40) { fclose(jpDump); jpDump = NULL; } }
    FILE *keepDump = jpDump; if (jpDump && f > 1) jpDump = NULL;
    if (jpDump) { fprintf(jpDump, "B %d %d %d %d\n", jpDumpN, f, kind, street); jpDumpRect("O", in); if (hasStairs) jpDumpRect("S", stairR); { Vector3 dp = xfP(curXf, (street == 0 || street == 2) ? V3(doorLocal, 0, street == 0 ? in.y : in.y + in.height) : V3(street == 3 ? in.x : in.x + in.width, 0, doorLocal)); fprintf(jpDump, "E %.2f %.2f\n", dp.x, dp.z); } if (f == 0 || !hasStairs) jpDumpN++; }
    jpY0 = y0; jpYT = yTop; jpPT = T_PLASTER; jpPC = kind == 0 ? rgb(226, 218, 200) : rgb(222, 222, 216); jpRooms = 0;
    float doorU = (street == 0 || street == 2) ? doorLocal - in.x : doorLocal - in.y;   // the front door's position across
    // the stairwell in (u, d)
    float su0 = 0, su1 = 0, sd0 = J.D, sd1 = J.D;
    if (hasStairs) { Rectangle s = stairR; float xs[2] = {s.x, s.x + s.width}, zs[2] = {s.y, s.y + s.height}; su0 = sd0 = 1e9f; su1 = sd1 = -1e9f;
        for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) { float x = xs[i], z = zs[j], u, d;
            switch (street) { case 0: u = x - in.x; d = z - in.y; break; case 2: u = x - in.x; d = in.y + in.height - z; break; case 3: u = z - in.y; d = x - in.x; break; default: u = z - in.y; d = in.x + in.width - x; }
            su0 = fminf(su0, u); su1 = fmaxf(su1, u); sd0 = fminf(sd0, d); sd1 = fmaxf(sd1, d); }
        su0 = fmaxf(su0, 0); su1 = fminf(su1, J.W); sd0 = fmaxf(sd0, 0); sd1 = fminf(sd1, J.D); }
    bool atBack = !hasStairs || sd0 > J.D * 0.45f;
    bool top = !hasStairs;   // (nothing above or below that needs the back corridor)
    if (atBack) {
        float Dr = hasStairs ? sd0 - 0.05f : J.D; if (Dr < 3.0f) return false;
        float hw = f == 0 ? 1.7f : 1.0f, hu0 = clampf(doorU - hw / 2, 0, J.W - hw);   // (wide enough for the genkan downstairs)   // the entrance hall from the genkan straight back
        if (f > 0) hu0 = clampf((su0 + su1) / 2 - hw / 2, 0, J.W - hw);   // upstairs the hall meets the top of the stairs
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
        float a = low ? hu1 : 0, b = low ? J.W : hu0; if (b - a < 2.6f) return false;
        float hallU = low ? hu1 : hu0;
        jpRoom(&J, hu0, 0, hu1, J.D, RM_HALL, low ? 2 : 3);
        float Dr = J.D;
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
    for (int i = 0; i < jpRooms; i++) { planRoom(jpRoomR[i], y0, jpRoomP[i], jpRoomO[i], region, street, kind == 0); if (jpDump) { char t[8]; snprintf(t, 8, "R%d", jpRoomP[i]); jpDumpRect(t, jpRoomR[i]); } }
    if (!jpDump) jpDump = keepDump;
    return true;
}
static void building(Rectangle r, int kind, int floors, int street) {
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
    if (bP && bP->tex >= 0) extTex = bP->tex;
    if (bP && bP->wall.a) ec = bP->wall;
    Rectangle in = {r.x + curT, r.y + curT, r.width - 2 * curT, r.height - 2 * curT};
    bool runX = in.width >= in.height;
    float runLen = runX ? in.width : in.height, cross = runX ? in.height : in.width;
    int acc = floors > 6 ? 6 : floors;                       // floors you can walk into
    const float SW = 1.05f, RUN = 3.6f; const int NS = 12;
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
    float uL = uStart + 1.0f, uH = uL + RUN;
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
    float uB0 = uH + 0.25f, uB1 = uB0 + RUN; Rectangle bHole = {0, 0, 0, 0};
    bool basement = acc > 1 && kind != 0 && uB1 + 0.9f < uStart + runLen && nPit < MAX_PIT &&
                    (gBasementAll || kind == 6 || (kind == 1 && floors >= 3 && ri(3) == 0) || (kind == 2 && floors >= 2 && ri(4) == 0) || (bP && (bfl & BD_BRICKBASE)) ||
                     (gBasementK > 1 && hash2((int)(uStart * 7 + c0A * 3), floors * 17 + kind, 83) < (gBasementK - 1) * 0.3f));
    if (gBasementK <= 0) basement = false;   // a map can switch them off
    if (basement) {
        float BY = SIDEH - FH, rise = FH / NS, tread = RUN / NS;
        bHole = runX ? (Rectangle){uB0 - 0.05f, c0A, RUN + 0.1f, SW} : (Rectangle){c0A, uB0 - 0.05f, SW, RUN + 0.1f};
        { Pit *q = &pits[nPit++]; q->r = bHole; q->f = (short)curXf; q->x0 = q->z0 = 1e9f; q->x1 = q->z1 = -1e9f;
          for (int k = 0; k < 4; k++) { Vector3 w = xfP(curXf, V3(bHole.x + (k & 1) * bHole.width, 0, bHole.y + (k >> 1) * bHole.height)); q->x0 = fminf(q->x0, w.x); q->x1 = fmaxf(q->x1, w.x); q->z0 = fminf(q->z0, w.z); q->z1 = fmaxf(q->z1, w.z); } }
        prevHole = bHole;   // the ground floor's finish is cut there
        for (int k = 0; k < NS; k++) { float u0 = uB0 + k * tread, u1 = u0 + tread, top = SIDEH - (k + 1) * rise;   // the flight down, top step first
            if (runX) box(T_CONCRETE, V3(u0, BY - 0.3f, c0A), V3(u1, top, c0A + SW), rgb(150, 146, 140), true, ALL); else box(T_CONCRETE, V3(c0A, BY - 0.3f, u0), V3(c0A + SW, top, u1), rgb(150, 146, 140), true, ALL); }
        box(T_CONCRETE, V3(r.x, BY - 0.3f, r.y), V3(r.x + r.width, BY, r.y + r.height), rgb(120, 118, 112), true, ALL);      // basement floor
        box(T_CONCRETE, V3(r.x, BY - 0.3f, r.y), V3(r.x + r.width, SIDEH, r.y + curT), rgb(150, 148, 140), true, ALL);    // its walls
        box(T_CONCRETE, V3(r.x, BY - 0.3f, r.y + r.height - curT), V3(r.x + r.width, SIDEH, r.y + r.height), rgb(150, 148, 140), true, ALL);
        box(T_CONCRETE, V3(r.x, BY - 0.3f, r.y), V3(r.x + curT, SIDEH, r.y + r.height), rgb(150, 148, 140), true, ALL);
        box(T_CONCRETE, V3(r.x + r.width - curT, BY - 0.3f, r.y), V3(r.x + r.width, SIDEH, r.y + r.height), rgb(150, 148, 140), true, ALL);
        slabWithHole(T_CONCRETE, in, bHole, SIDEH - 0.32f, SIDEH - 0.02f, rgb(135, 135, 132));                              // its ceiling, open over the stairs
        {   // railing round the opening on the ground floor: the room side (open for the first metre: step in from the side) and the far end
            float cE = (c0A < c0B) ? c0A + SW : c0A, ry = SIDEH + 1.0f; Color rc = rgb(150, 155, 150);
            if (runX) { box(T_METAL, V3(uB0 + 1.0f, SIDEH, cE - 0.03f), V3(uB1 + 0.05f, ry, cE + 0.03f), rc, true, ALL); box(T_METAL, V3(uB1 + 0.02f, SIDEH, c0A), V3(uB1 + 0.08f, ry, c0A + SW), rc, true, ALL); }
            else { box(T_METAL, V3(cE - 0.03f, SIDEH, uB0 + 1.0f), V3(cE + 0.03f, ry, uB1 + 0.05f), rc, true, ALL); box(T_METAL, V3(c0A, SIDEH, uB1 + 0.02f), V3(c0A + SW, ry, uB1 + 0.08f), rc, true, ALL); } }
        {   float mC = c0A + SW / 2; Vector3 bot = runX ? V3(uB1 + 0.45f, BY, mC) : V3(mC, BY, uB1 + 0.45f), topP = runX ? V3(uB0 - 0.2f, SIDEH, (c0A < c0B) ? c0A + SW + 0.5f : c0A - 0.5f) : V3((c0A < c0B) ? c0A + SW + 0.5f : c0A - 0.5f, SIDEH, uB0 - 0.2f);
            if (nStair < MAX_STAIR) stairs[nStair++] = (Stair){xfP(curXf, bot), xfP(curXf, topP)}; }
        {   // what is down there: storage along the wall away from the stairs, a boiler, one bad light
            float farC = stripHigh ? cMin + 0.3f : cMax - 0.3f; float yaw = runX ? (stripHigh ? 0 : PI) : (stripHigh ? -PI / 2 : PI / 2);
            int kinds[7] = {CK_SHELF, CK_BOX, CK_LOCKER, CK_TOOLBOX, CK_SHELF, CK_BOX, CK_LOCKER}; float u = uStart + 0.9f;
            for (int k = 0; k < 7; k++) { int ck = kinds[(k + ri(3)) % 7]; float wdt = ckSize[ck].x, dep = ckSize[ck].z; if (u + wdt > uStart + runLen - 2.4f) break;   // clear of the end wall and the boiler
                float cc = stripHigh ? farC + dep / 2 : farC - dep / 2; Vector3 p = runX ? V3(u + wdt / 2, BY, cc) : V3(cc, BY, u + wdt / 2);
                addCont(ck, p, yaw, 9, true); u += wdt + fr(0.3f, 1.2f); }
            float bx = uStart + runLen - 1.6f, bc = (cMin + cMax) / 2;
            if (runX) box(T_METAL, V3(bx - 0.6f, BY, bc - 0.6f), V3(bx + 0.6f, BY + 1.8f, bc + 0.6f), rgb(90, 95, 92), true, ALL); else box(T_METAL, V3(bc - 0.6f, BY, bx - 0.6f), V3(bc + 0.6f, BY + 1.8f, bx + 0.6f), rgb(90, 95, 92), true, ALL);
            addLight(V3(in.x + in.width / 2, SIDEH - 0.6f, in.y + in.height / 2), V3(0.8f, 0.85f, 0.7f), 8, ri(3) ? 1 : 2); }
    }
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
        if (f < acc - 1) {
            bool even = (f % 2) == 0;
            float c0 = even ? c0A : c0B;
            float rise = FH / NS, tread = RUN / NS;
            for (int k = 0; k < NS; k++) {
                float u0 = even ? uL + k * tread : uH - (k + 1) * tread, u1 = u0 + tread;
                float top = y0 + (k + 1) * rise;
                if (runX) box(T_CONCRETE, V3(u0, y0, c0), V3(u1, top, c0 + SW), rgb(170, 165, 160), true, ALL);
                else box(T_CONCRETE, V3(c0, y0, u0), V3(c0 + SW, top, u1), rgb(170, 165, 160), true, ALL);
            }
            hole = runX ? (Rectangle){uL - 0.05f, c0, RUN + 0.1f, SW} : (Rectangle){c0, uL - 0.05f, SW, RUN + 0.1f};
            float midC = c0 + SW / 2, uBot = even ? uL - 0.45f : uH + 0.45f, uTop = even ? uH + 0.45f : uL - 0.45f;
            Vector3 bot = runX ? V3(uBot, y0, midC) : V3(midC, y0, uBot);
            Vector3 topP = runX ? V3(uTop, y0 + FH, midC) : V3(midC, y0 + FH, uTop);
            if (nStair < MAX_STAIR) stairs[nStair++] = (Stair){xfP(curXf, bot), xfP(curXf, topP)};
            // railings on the floor above: both long edges of the opening, and across its low end
            float yU = y0 + FH, ry = yU + 1.0f, ue = even ? uL - 0.05f : uH - 0.01f;
            Color rc = rgb(150, 155, 150);
            if (runX) {
                box(T_METAL, V3(uL - 0.05f, yU, c0), V3(uH + 0.05f, ry, c0 + 0.06f), rc, true, ALL);
                box(T_METAL, V3(uL - 0.05f, yU, c0 + SW - 0.06f), V3(uH + 0.05f, ry, c0 + SW), rc, true, ALL);
                box(T_METAL, V3(ue, yU, c0), V3(ue + 0.06f, ry, c0 + SW), rc, true, ALL);
            } else {
                box(T_METAL, V3(c0, yU, uL - 0.05f), V3(c0 + 0.06f, ry, uH + 0.05f), rc, true, ALL);
                box(T_METAL, V3(c0 + SW - 0.06f, yU, uL - 0.05f), V3(c0 + SW, ry, uH + 0.05f), rc, true, ALL);
                box(T_METAL, V3(c0, yU, ue), V3(c0 + SW, ry, ue + 0.06f), rc, true, ALL);
            }
            // about a quarter of the landings are choked with junk
            if (fr(0, 1) < 0.28f) { float hw = 0.55f; addDebris(V3(topP.x - hw, yU, topP.z - hw), V3(topP.x + hw, yU + 1.15f, topP.z + hw)); }
        }
        // ceiling / next floor slab with the stairwell cut out
        prevHole = hole;
        slabWithHole(T_CONCRETE, r, hole, yTop, yTop + SLAB, rgb(150, 150, 150));
        { float ext = (basement && f == 0) ? RUN + 0.35f : 0;   // the flight down on the ground floor too
          fAvoid = acc > 1 ? (runX ? (Rectangle){uL - 0.7f, bandLo - 0.15f, RUN + 1.4f + ext, bandHi - bandLo + 0.3f} : (Rectangle){bandLo - 0.15f, uL - 0.7f, bandHi - bandLo + 0.3f, RUN + 1.4f + ext}) : (Rectangle){0, 0, 0, 0}; }
        // houses and flats: a Japanese plan (hall, LDK, washitsu, toilet, washroom, bath, bedrooms); other buildings: one partition, two rooms
        float jpDoor; { bool aX = street == 0 || street == 2; float A = aX ? r.x : r.y + curT, len = aX ? r.width : r.height - 2 * curT; int nm = (int)(len / 3.0f); if (nm < 1) nm = 1; jpDoor = A + (nm / 2 + 0.5f) * (len / nm); }
        bool jpTry = (kind == 0 || kind == 1) && ((street == 0 || street == 2) ? in.width : in.height) >= 5.0f && ((street == 0 || street == 2) ? in.height : in.width) >= 4.6f;
        // one partition wall splitting the floor into two rooms, with a doorway (sometimes piled shut)
        nKeep = 0;
        if (partC > 0 && !jpTry) {
            float a = runX ? in.x : in.y, b = runX ? in.x + in.width : in.y + in.height;
            float gap = fr(a + 0.8f, b - 1.9f);
            Color pc = kind == 0 ? rgb(225, 215, 195) : rgb(200, 200, 196);
            int pt = kind == 0 ? T_PLASTER : T_CONCRETE;
            float mc = partC;
            keepClear[nKeep++] = runX ? (Rectangle){gap - 0.6f, mc - 1.3f, 2.3f, 2.6f} : (Rectangle){mc - 1.3f, gap - 0.6f, 2.6f, 2.3f};
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
                box(T_TILEWALL, V3(g.x, y0 + 0.017f, g.y), V3(g.x + g.width, y0 + 0.026f, g.y + g.height), rgb(120, 118, 112), false, 16);
                Rectangle e = sd == 0 ? (Rectangle){g.x, g.y + D, g.width, 0.12f} : sd == 2 ? (Rectangle){g.x, g.y - 0.12f, g.width, 0.12f} : sd == 1 ? (Rectangle){g.x - 0.12f, g.y, 0.12f, g.height} : (Rectangle){g.x + D, g.y, 0.12f, g.height};
                box(T_WOOD, V3(e.x, y0, e.y), V3(e.x + e.width, y0 + 0.14f, e.y + e.height), rgb(140, 100, 66), true, ALL);   // agarikamachi: you step up into the house
                Rectangle sb = aX ? (Rectangle){g.x + g.width, sd == 0 ? g.y : g.y + D - 0.38f, 0.9f, 0.38f} : (Rectangle){sd == 3 ? g.x : g.x + D - 0.38f, g.y + g.height, 0.38f, 0.9f};
                if (!jpTry && fFree(sb.x, sb.y, sb.x + sb.width, sb.y + sb.height, y0)) { box(T_WOOD, V3(sb.x, y0, sb.y), V3(sb.x + sb.width, y0 + 0.95f, sb.y + sb.height), rgb(160, 125, 90), true, ALL);
                    box(T_PLAIN, V3(sb.x + 0.1f, y0 + 0.95f, sb.y + 0.08f), V3(sb.x + 0.3f, y0 + 1.15f, sb.y + 0.25f), rgb(90, 130, 90), false, ALL); }   // a pot plant on top
                for (int k = 0; k < 2 + ri(3); k++) { float sx = g.x + fr(0.2f, g.width - 0.4f), sz = g.y + fr(0.2f, g.height - 0.4f); box(T_PLAIN, V3(sx, y0 + 0.026f, sz), V3(sx + 0.12f, y0 + 0.1f, sz + 0.28f), (Color[]){{40, 40, 44, 255}, {200, 200, 205, 255}, {150, 60, 50, 255}}[ri(3)], false, ALLB); }   // shoes left behind
                keepClear[nKeep++] = (Rectangle){g.x - 0.4f, g.y - 0.4f, g.width + 0.8f, g.height + 0.8f}; } }
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
            bool jpDone = jpTry && jpFloor(in, street, fAvoid, acc > 1, y0, yTop, f, kind, jpDoor, kind);
            if (!jpDone) { planRoom(front, y0, pF, oF, kind, street, kind == 0);
                if (two) planRoom(back, y0, pB, oBk, kind, street, kind == 0); }
            {   // the dead: someone who never got out
                Vector3 wq = xfP(curXf, V3(in.x, y0, in.y)); unsigned int sd = (unsigned int)((int)(wq.x * 7.3f) * 92821 + (int)(wq.z * 5.1f) * 68917 + f * 131 + worldSeed);
                float pr = (kind == 4 ? 0.45f : kind == 6 ? 0.22f : kind == 1 ? 0.1f : 0.08f) * gBodyK; if (wq.x < -185 && wq.z > 110 && wq.z < 300) pr *= 1.8f;
                if ((float)((sd * 2246822519u) >> 8 & 0xffff) / 65536.0f < pr) placeBody(front, y0, kind == 4 ? 2 : (kind == 6 && (sd & 3) == 0) ? 3 : 1, 0, sd); }
        }
        // light (most upper floors stay dark)
        if (f == 0 || fr(0, 1) < 0.3f) addLight(V3(in.x + in.width / 2, yTop - 0.3f, in.y + in.height / 2), kind == 6 ? V3(0.5f, 0.9f, 0.85f) : V3(0.9f, 0.85f, 0.7f), 9, fr(0, 1) < 0.5f ? 1 : (f > 0 && fr(0, 1) < 0.4f ? 2 : 0));
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
    bDetails(r, kind, floors, street, top, ec, bfl);
}

// ---------------------------------------------------------------- subway
static void subwayEntrance(float x0, float z0) {   // stairwell from the sidewalk down to the platform level of the line under road 3
    float hx0 = x0 + 14, hx1 = x0 + 24, hz0 = z0 + 0.3f, hz1 = z0 + 2.3f;
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
    if (nStair < MAX_STAIR) stairs[nStair++] = (Stair){V3(hx0 + 9.0f, PY, (hz0 + hz1) / 2), V3(hx0 - 0.6f, SIDEH, (hz0 + hz1) / 2)};
    if (nStair < MAX_STAIR) stairs[nStair++] = (Stair){V3((cx0 + cx1) / 2, PY, TUN_Z1 - 1.0f), V3((cx0 + cx1) / 2, PY, hz0 + 0.5f)};
}
static void genSubway(void) {
    float X0 = SUB_X0, X1 = SUB_X1;
    box(T_CONCRETE, V3(X0, SUBY - 0.4f, TUN_Z0 - 0.4f), V3(X1, SUBY, TUN_Z1 + 0.4f), rgb(110, 110, 105), true, ALL);                // track bed
    box(T_CONCRETE, V3(X0, SUBY + 4.2f, TUN_Z0 - 0.4f), V3(X1, SUBY + 4.6f, TUN_Z1 + 0.4f), rgb(90, 92, 92), true, ALLB);          // ceiling
    box(T_TILEWALL, V3(X0, SUBY, TUN_Z0 - 0.4f), V3(X1, SUBY + 4.2f, TUN_Z0), rgb(170, 182, 182), true, ALL);                       // far wall
    float gaps[3] = {ROADW + subCols[0] * PITCH + 22.2f, ROADW + subCols[1] * PITCH + 22.2f, ROADW + subCols[2] * PITCH + 22.2f};
    float wx = X0;
    for (int g = 0; g < 3; g++) { box(T_TILEWALL, V3(wx, SUBY, TUN_Z1), V3(gaps[g], SUBY + 4.2f, TUN_Z1 + 0.4f), rgb(170, 182, 182), true, ALL); wx = gaps[g] + 1.7f;
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
    float tx0 = ROADW + 5 * PITCH + 6, tx1 = tx0 + 18, tz0 = TUN_Z0 + 0.5f, tz1 = PZ - 0.2f;
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
    for (int b = 0; b < 3; b++) subwayEntrance(ROADW + subCols[b] * PITCH, ROADW + SUB_ROW * PITCH);
}
static void spawnUnderground(void) {
    for (int k = 0; k < 9; k++) {
        float x = fr(SUB_X0 + 6, SUB_X1 - 6);
        if (Vector3Distance(V3(x, SUBY, 0), V3(pl.pos.x, SUBY, 0)) < 25 && pl.pos.y < -2) continue;
        float r = fr(0, 1);
        int t = r < 0.3f ? 2 : r < 0.55f ? 0 : r < 0.8f ? 8 : r < 0.92f ? 6 : 5;
        bool onPlat = fr(0, 1) < 0.5f;
        Vector3 p = V3(x, onPlat ? SUBY + 0.9f : SUBY, onPlat ? TUN_Z1 - 1.7f : TUN_Z0 + 1.6f);
        int s = spawnAt(p, t, false);
        if (s >= 0 && t == 8) { crs[s].hanging = true; crs[s].pos.y = SUBY + 3.8f; }
    }
    static bool attendant = false;
    if (!attendant || fr(0, 1) < 0.15f) { int s = spawnAt(V3(ROADW + 5 * PITCH + 2, SUBY + 0.9f, TUN_Z1 - 1.6f), 9, false); (void)s; attendant = true; }
}

// ---------------------------------------------------------------- v0.4: map footprints, water sources, day light
#define MAX_MAPFOOT 2400
static Rectangle mapFoot[MAX_MAPFOOT]; static float mapFootH[MAX_MAPFOOT], mapFootY[MAX_MAPFOOT]; static short mapFootF[MAX_MAPFOOT]; static int nMapFoot;   // footprints in their building's frame
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
