// zk_world.h — multi-floor buildings (rooms, stairs, junk piles, searchable furniture), subway, the Aero wilds outside the city
#define FH 3.2f          // floor-to-floor height
#define SLAB 0.2f
#define WILD 150.0f      // walkable wilds beyond the city edge
#define SUBY -7.0f       // subway floor
#define TUN_Z0 (SUB_ROW * PITCH + 1.0f)
#define TUN_Z1 (SUB_ROW * PITCH + ROADW - 1.0f)

// ---------------------------------------------------------------- ground holes (subway stairwells)
#define MAX_HOLE 8
static Rectangle holes[MAX_HOLE]; static int nHole;
static bool inHole(float x, float z) { for (int i = 0; i < nHole; i++) if (x > holes[i].x && x < holes[i].x + holes[i].width && z > holes[i].y && z < holes[i].y + holes[i].height) return true; return false; }
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
static bool cityLand(float x, float z) {   // on a block or on an existing road of the grid
    if (x < 0 || z < 0 || x > CITY || z > CITY) return false;
    int ci = (int)(x / PITCH), cj = (int)(z / PITCH); float u = x - ci * PITCH, v = z - cj * PITCH;
    bool rx = u < ROADW, rz = v < ROADW;
    if (rx && rz) return nodeOn(ci, cj);
    if (rx) return segV(ci, cj);
    if (rz) return segH(ci, cj);
    return blockOn(ci, cj);
}
#define HF_S 2.0f
#define HF_X0 (-WILD)
#define HF_N ((int)((CITY + 2 * WILD) / HF_S) + 1)
static float *hf, *hfd;      // heights, distance to city land (both HF_N x HF_N)
static bool hfReady = false;
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
typedef struct { float px, pz, fy, w, h, len; int dir, kind; } Bore;   // kind 0 road tunnel, 1 old brick tunnel
#define MAX_BORE 6
static Bore bores[MAX_BORE]; static int nBore;
static int inBore(float x, float z, float y) {   // index+1 when (x,z,y) is inside a tunnel's interior volume
    for (int i = 0; i < nBore; i++) { Bore *b = &bores[i];
        if (y < b->fy - 1 || y > b->fy + b->h + 0.5f) continue;
        float a = b->dir == 0 ? x - b->px : b->dir == 1 ? b->px - x : b->dir == 2 ? z - b->pz : b->pz - z;
        float l = (b->dir < 2) ? z - b->pz : x - b->px;
        if (a >= -0.5f && a <= b->len + 2 && fabsf(l) <= b->w / 2 + 0.6f) return i + 1; }
    return 0;
}
static float baseGroundY(float x, float z, float y) {
    if (cityLand(x, z)) return inHole(x, z) ? -1e9f : 0;
    int b = inBore(x, z, y); if (b) return bores[b - 1].fy;
    return terrainH(x, z);
}
static float baseGround(float x, float z) { return baseGroundY(x, z, 1e9f); }
static float seaDepth(float x, float z) { return cityLand(x, z) ? 0 : fmaxf(0, -0.8f - terrainH(x, z)); }
static float touge_d(float x, float z);   // distance to the mountain pass road (zk_layout.h)
static bool inCore(float x, float z);
static float coast_d(float x, float z);
static float wildK(Vector3 p) {
    if (p.y < -2 || inBore(p.x, p.z, p.y)) return 0;
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
        if (land || paved || terrainSkip(ax, az)) continue;
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
            float r = 1, g = 1, b = 1;
            if (y < 0.4f && y > -1.6f && !cityLand(x, z)) { r = 1.9f; g = 1.25f; b = 1.0f; }
            float mt = clampf((y - 12) / 14, 0, 1); r = Lerp(r, 0.55f, mt); g = Lerp(g, 0.72f, mt); b = Lerp(b, 0.62f, mt);
            float rk = clampf((y - 34) / 10, 0, 1); r = Lerp(r, 1.45f, rk); g = Lerp(g, 0.95f, rk); b = Lerp(b, 1.05f, rk);
            m.colors[v * 4] = (unsigned char)clampf(255 * sh * r, 0, 255); m.colors[v * 4 + 1] = (unsigned char)clampf(255 * sh * g, 0, 255);
            m.colors[v * 4 + 2] = (unsigned char)clampf(255 * sh * b, 0, 255); m.colors[v * 4 + 3] = 255;
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
    static const float hp[10] = {3, 5, 2, 2, 1, 6, 4, 3, 3, 12};
    static const float sp[10] = {3.2f, 2.4f, 4.8f, 6.0f, 3.6f, 2.4f, 3.0f, 6.5f, 4.6f, 2.2f};
    int t = c->type < 0 || c->type > 9 ? 0 : c->type;
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
        Cont *c = &conts[i]; if (!c->on || c->kind == CK_PILE || c->region == 9) continue;
        if (Vector3Distance(c->pos, nearP) < minD) continue;
        Vector3 p = Vector3Add(c->pos, V3(sinf(c->yaw) * 1.1f, 0, cosf(c->yaw) * 1.1f));
        if (!pointFree(p.x, p.z, 0.35f, p.y + 0.05f)) continue;
        float r = fr(0, 1);
        int t = r < 0.4f ? 0 : r < 0.55f ? 2 : r < 0.7f ? 7 : r < 0.85f ? 8 : 6;
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
    d->mn = mn; d->mx = mx; d->hp = 7; d->alive = true; d->seed = ri(1000);
    d->sid = addSolid(mn, mx);
}
static bool boxFree(float x0, float z0, float x1, float z1, float y) {   // no solid overlaps this footprint between y and y+1.8
    query(x0, z0, x1, z1);
    for (int k = 0; k < qn; k++) { Solid *s = &sol[qb[k]]; if (s->off || s->mx.y <= y + 0.05f || s->mn.y >= y + 1.8f) continue;
        if (x0 < s->mx.x && x1 > s->mn.x && z0 < s->mx.z && z1 > s->mn.z) return false; }
    return true;
}
// place a searchable piece of furniture against an inside wall of `in` at floor y
static Rectangle keepClear[8]; static int nKeep;   // doorways on the floor being furnished
static bool blocksDoor(float x0, float z0, float x1, float z1, float y) {
    for (int k = 0; k < nKeep; k++) { Rectangle c = keepClear[k]; if (x0 < c.x + c.width && x1 > c.x && z0 < c.y + c.height && z1 > c.y) return true; }
    for (int i = 0; i < nOps; i++) { Open *o = &ops[i]; if (o->type != OP_DOOR || fabsf(o->mn.y - y) > 0.6f) continue;   // keep 1.2 m clear around every door
        if (x0 < o->mx.x + 1.2f && x1 > o->mn.x - 1.2f && z0 < o->mx.z + 1.2f && z1 > o->mn.z - 1.2f) return true; }
    return false;
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
static int opp(int w) { return (w + 2) % 4; }
// room plans
enum { RM_LIVING, RM_KITCHEN, RM_BED, RM_SHOP, RM_STOCK, RM_KONBINI, RM_KOBAN, RM_OFFICE, RM_STUDY };
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
static void building(Rectangle r, int kind, int floors, int street) {
    curB = r; curT = 0.3f;
    if (nFoot < MAX_FOOT) { footH[nFoot] = floors * FH + 0.6f; foots[nFoot++] = (Rectangle){r.x + curT, r.y + curT, r.width - 2 * curT, r.height - 2 * curT}; }
    int extTex = kind == 0 ? T_PLASTER : kind == 1 ? T_CONCRETE : kind == 2 ? T_TILEWALL : kind == 3 ? T_PLAIN : kind == 4 ? T_PLASTER : T_CONCRETE;
    Color ec = kind == 3 ? rgb(215, 220, 225) : kind == 4 ? rgb(200, 200, 205) : kind == 6 ? rgb(235, 242, 242) : rgb(fr(200, 255), fr(195, 245), fr(185, 235));
    Rectangle in = {r.x + curT, r.y + curT, r.width - 2 * curT, r.height - 2 * curT};
    bool runX = in.width >= in.height;
    float runLen = runX ? in.width : in.height, cross = runX ? in.height : in.width;
    int acc = floors > 4 ? 4 : floors;                       // floors you can walk into
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
    for (int f = 0; f < acc; f++) {
        float y0 = SIDEH + f * FH, yTop = y0 + FH - SLAB;
        extFloor(r, kind, street, y0, yTop, f == 0, extTex, ec);
        {   // floor finish, open over the stairwell coming up from below
            Rectangle fr2 = in, h = prevHole;
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
            if (nStair < MAX_STAIR) stairs[nStair++] = (Stair){bot, topP};
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
        // one partition wall splitting the floor into two rooms, with a doorway (sometimes piled shut)
        nKeep = 0;
        if (partC > 0) {
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
        fAvoid = acc > 1 ? (runX ? (Rectangle){uL - 0.7f, bandLo - 0.15f, RUN + 1.4f, bandHi - bandLo + 0.3f} : (Rectangle){bandLo - 0.15f, uL - 0.7f, bandHi - bandLo + 0.3f, RUN + 1.4f}) : (Rectangle){0, 0, 0, 0};
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
            else if (kind == 1) { pF = f % 2 ? RM_BED : RM_KITCHEN; pB = f % 2 ? RM_STUDY : RM_BED; if (f == 0) { pF = RM_KITCHEN; pB = RM_LIVING; } }
            else { pF = f == 0 ? RM_LIVING : RM_BED; pB = f == 0 ? RM_KITCHEN : RM_STUDY; }
            if (!two && kind != 3 && kind != 4) { pF = f == 0 ? (kind == 2 ? RM_SHOP : RM_LIVING) : RM_BED; }
            planRoom(front, y0, pF, oF, kind, street, kind == 0);
            if (two) planRoom(back, y0, pB, oBk, kind, street, kind == 0);
        }
        // light (most upper floors stay dark)
        if (f == 0 || fr(0, 1) < 0.3f) addLight(V3(in.x + in.width / 2, yTop - 0.3f, in.y + in.height / 2), kind == 6 ? V3(0.5f, 0.9f, 0.85f) : V3(0.9f, 0.85f, 0.7f), 9, fr(0, 1) < 0.5f ? 1 : (f > 0 && fr(0, 1) < 0.4f ? 2 : 0));
        // a television on the ground floor of homes
        if (f == 0 && (kind == 0 || kind == 1) && nTV < MAX_TV && in.width > 3) {
            float tx = in.x + in.width - 1.4f, tz = stripHigh && runX ? in.y + 0.45f : in.y + 0.45f;
            if (pointFree(tx, tz, 0.6f, y0 + 0.05f)) {
                box(T_WOOD, V3(tx - 0.55f, y0, tz - 0.3f), V3(tx + 0.55f, y0 + 0.5f, tz + 0.3f), rgb(150, 120, 100), true, ALL);
                addSolid(V3(tx - 0.4f, y0 + 0.5f, tz - 0.25f), V3(tx + 0.4f, y0 + 1.1f, tz + 0.25f));
                tvs[nTV++] = (TV){V3(tx, y0 + 0.8f, tz + 0.26f), 0};
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
    if (floors > 1) {
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
            signs[nSign++] = (Sign){mn, mx, tc, li, T_SIGNH, kind == 3 ? SGN_KONBINI : kind == 4 ? SGN_KOBAN : kind == 5 ? SGN_STATION : ri(JP_NSHOP), street, false};
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
        signs[nSign++] = (Sign){mn, mx, tc, li, T_SIGN, ri(JP_NSHOP), street, false};
    }
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
#define MAX_MAPFOOT 1400
static Rectangle mapFoot[MAX_MAPFOOT]; static float mapFootH[MAX_MAPFOOT], mapFootY[MAX_MAPFOOT]; static int nMapFoot;
typedef struct { Vector3 pos; int kind; } WSrc;   // kind 0 rain barrel, 1 park fountain, 2 hill spring
#define MAX_WS 200
static WSrc wsrc[MAX_WS]; static int nWs;
static int worldSolids, worldLights;              // counts right after generation (save files restore on top of these)
static void genWater(void) {
    nWs = 0;
    for (int bi = 0; bi < NB; bi++) for (int bj = 0; bj < NB; bj++) {
        float x0 = ROADW + bi * PITCH, z0 = ROADW + bj * PITCH;
        if (!blockOn(bi, bj) || blockKindA[bi][bj] == BK_PARK || blockKindA[bi][bj] == BK_OLD) continue;
        int want = 2, got = 0;
        for (int tries = 0; tries < 40 && got < want && nWs < MAX_WS; tries++) {
            float x = x0 + 0.8f + fr(0, BLOCK - 1.6f), z = z0 + 0.8f + fr(0, BLOCK - 1.6f);
            if (inHole(x, z) || !pointFree(x, z, 0.55f, SIDEH + 0.05f)) continue;
            bool inside = false; for (int f = 0; f < nMapFoot; f++) { Rectangle r = mapFoot[f]; if (x > r.x - 0.7f && x < r.x + r.width + 0.7f && z > r.y - 0.7f && z < r.y + r.height + 0.7f) inside = true; }
            if (inside) continue;
            bool near = false; for (int w = 0; w < nWs; w++) if (Vector3Distance(wsrc[w].pos, V3(x, SIDEH, z)) < 9) near = true;
            if (near) continue;
            wsrc[nWs++] = (WSrc){V3(x, SIDEH, z), 0}; addSolid(V3(x - 0.32f, SIDEH, z - 0.32f), V3(x + 0.32f, SIDEH + 0.95f, z + 0.32f)); got++;
        }
    }
    {   // the park fountain
        int pbi = 4, pbj = 4; for (int a = 0; a < NB; a++) for (int b = 0; b < NB; b++) if (blockKindA[a][b] == BK_PARK) { pbi = a; pbj = b; }
        float x0 = ROADW + pbi * PITCH, z0 = ROADW + pbj * PITCH;
        float x = x0 + BLOCK / 2 + 4, z = z0 + BLOCK / 2 + 4;
        for (int tries = 0; tries < 30; tries++) { if (pointFree(x, z, 0.8f, SIDEH + 0.05f)) break; x = x0 + 3 + fr(0, BLOCK - 6); z = z0 + 3 + fr(0, BLOCK - 6); }
        if (nWs < MAX_WS) { wsrc[nWs++] = (WSrc){V3(x, SIDEH, z), 1}; addSolid(V3(x - 0.75f, SIDEH, z - 0.75f), V3(x + 0.75f, SIDEH + 0.6f, z + 0.75f)); }
    }
    for (int k = 0; k < 7 && nWs < MAX_WS; k++) {   // clear springs in the hills (the water there is fine; the hills are not)
        float a = fr(0, 2 * PI), d = fr(30, 110);
        float x = CITY / 2 + cosf(a) * (CITY / 2 + d), z = CITY / 2 + sinf(a) * (CITY / 2 + d);
        x = clampf(x, -WILD + 10, CITY + WILD - 10); z = clampf(z, -WILD + 10, CITY + WILD - 10);
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
