// zk_layout.h — v0.5 city layout (concept 5b): bay in the south-west, old town on the shore, a two-lane coast road that
// runs along the bay into tunnels at both ends, a north road tunnel, and the east mountain with a hairpin pass (touge),
// a hamlet, shrines and a sealed old brick tunnel at the top. Tunnels are explorable up to a collapse.

// ---------------------------------------------------------------- paths
#define MAX_PATH 1200
static Vector3 coastP[MAX_PATH]; static int nCoast;     // y = road height
static Vector3 tougeP[MAX_PATH]; static int nTouge;
static Vector3 northP[MAX_PATH]; static int nNorth;
static Vector3 laneP[MAX_PATH]; static int nLane;      // hamlet lane
static Vector3 railP[MAX_PATH]; static int nRail;      // the railway between its portals
static Vector3 avenueP[MAX_PATH]; static int nAvenue;  // the diagonal avenue across the grid
static Vector3 ringP[4][MAX_PATH]; static int nRing[4];   // the road along the town's curved edge
typedef struct { float x, z, y, r, blend; } Pad;
#define MAX_PAD 64
static Pad pads[MAX_PAD]; static int nPad;
#define MT_X DMT_X
#define MT_Z DMT_Z
#define MT_R DMT_R
#define MT_H 60.0f
static bool inPoly(const float (*p)[2], int n, float x, float z) {   // even-odd rule
    bool in = false;
    for (int i = 0, j = n - 1; i < n; j = i++) if (((p[i][1] > z) != (p[j][1] > z)) && (x < (p[j][0] - p[i][0]) * (z - p[i][1]) / (p[j][1] - p[i][1]) + p[i][0])) in = !in;
    return in;
}
static float polyEdgeDist(const float (*p)[2], int n, float x, float z) {
    float best = 1e9f;
    for (int i = 0, j = n - 1; i < n; j = i++) { float ax = p[j][0], az = p[j][1], dx = p[i][0] - ax, dz = p[i][1] - az, L2 = dx * dx + dz * dz;
        float t = L2 > 0 ? clampf(((x - ax) * dx + (z - az) * dz) / L2, 0, 1) : 0, qx = ax + dx * t - x, qz = az + dz * t - z; best = fminf(best, qx * qx + qz * qz); }
    return sqrtf(best);
}
#define SEA_Y (-4.0f)      // the bay's surface: the coast road and the town sit about 4 m above it, on a seawall
#define RIVER_Y (-1.3f)    // the river's surface, 1.45 m below the town between its concrete banks
static bool inRiverRaw(float x, float z) {
    if (!inPoly(dRiver, DRIVER_N, x, z)) return false;
    for (int b = 0; b < DBRIDGES_N; b++) { float ax = dBridges[b][0], az = dBridges[b][1], dx = dBridges[b][2] - ax, dz = dBridges[b][3] - az, L2 = dx * dx + dz * dz;
        float t = L2 > 0 ? clampf(((x - ax) * dx + (z - az) * dz) / L2, -0.2f, 1.2f) : 0, qx = ax + dx * t - x, qz = az + dz * t - z;
        if (qx * qx + qz * qz < (dBridges[b][4] / 2 + 0.6f) * (dBridges[b][4] / 2 + 0.6f)) return false; }   // a bridge deck
    return true;
}
static unsigned char *townMask; static bool townReady = false;   // baked per heightfield cell: 1 = town ground (paved, flat)
static float coastSigned(float x, float z);
static bool seaSide(float x, float z);
static bool inTownRaw(float x, float z) {   // the drawn town, minus the river and the strip seaward of the coast road's seawall
    if (!inPoly(dCore, DCORE_N, x, z) || inRiverRaw(x, z)) return false;
    return coastSigned(x, z) < 5.2f;
}
static bool inCore(float x, float z) {
    if (!townReady) return inTownRaw(x, z);
    int i = (int)((x - HF_X0) / HF_S + 0.5f), j = (int)((z - HF_X0) / HF_S + 0.5f);
    if (i < 0 || j < 0 || i >= HF_N || j >= HF_N) return false;
    return townMask[j * HF_N + i] != 0;
}
#define MAX_EXT 240
static Rectangle extR[MAX_EXT]; static int nExt;   // street ends carried on to the ring road / avenue / coast road
static float pathDist(const Vector3 *p, int n, float x, float z, float *yOut);
#define MAX_STPTS 4096
static Vector3 stPts[MAX_STPTS]; static int nStreet;   // every street of the town (points from zk_design.h; widths in dStreets)
static bool mainRoadAt(float x, float z, float m) {   // on a street or the coast road (m: extra margin)
    for (int s2 = 0; s2 < nStreet; s2++) { int a = dStreets[s2][0], n = dStreets[s2][1]; float w = dStreets[s2][2] / 10.0f;
        float bx0 = 1e9f, bx1 = -1e9f, bz0 = 1e9f, bz1 = -1e9f; (void)bx0; (void)bx1; (void)bz0; (void)bz1;
        if (pathDist(stPts + a, n, x, z, NULL) < w / 2 + m) return true; }
    if (nCoast > 1 && pathDist(coastP, nCoast, x, z, NULL) < 3.7f + m) return true;
    return false;
}
static bool roadAt(float x, float z) {
    if (mainRoadAt(x, z, 0.2f)) return true;
    for (int e = 0; e < nExt; e++) if (x >= extR[e].x && x <= extR[e].x + extR[e].width && z >= extR[e].y && z <= extR[e].y + extR[e].height) return true;
    return false;
}
static void tryExt(float x, float z, float dx, float dz) {   // walk from a street end until it meets a main road
    for (float t = 1; t < 90; t += 1) { float px = x + dx * t, pz = z + dz * t;
        if (cityLand(px, pz)) return;
        if (!inCore(px, pz) && !mainRoadAt(px, pz, 1.5f)) return;   // the curb strip next to a main road counts as reaching it
        if (mainRoadAt(px, pz, -1.5f)) { if (nExt >= MAX_EXT) return; float L = t + 1.5f, hw = ROADW / 2;
            extR[nExt++] = dx != 0 ? (Rectangle){dx > 0 ? x : x - L, z - hw, L, ROADW} : (Rectangle){x - hw, dz > 0 ? z : z - L, ROADW, L}; return; } }
}
static void layoutExtensions(void) {
    nExt = 0;
    for (int i = 0; i <= NB; i++) for (int j = 0; j < NB; j++) {
        if (segV(i, j) && !segV(i, j - 1)) tryExt(i * PITCH + ROADW / 2, j * PITCH, 0, -1);
        if (segV(i, j) && !segV(i, j + 1)) tryExt(i * PITCH + ROADW / 2, (j + 1) * PITCH + ROADW, 0, 1);
        if (segH(j, i) && !segH(j - 1, i)) tryExt(j * PITCH, i * PITCH + ROADW / 2, -1, 0);
        if (segH(j, i) && !segH(j + 1, i)) tryExt((j + 1) * PITCH + ROADW, i * PITCH + ROADW / 2, 1, 0);
    }
}
static int loadPath(const float (*src)[2], int n, Vector3 *out) { int k = 0; for (int i = 0; i < n && k < MAX_PATH; i++) out[k++] = V3(src[i][0], 0, src[i][1]); return k; }
static int resample(const Vector2 *cp, int nc, Vector3 *out, int maxOut, float step) {   // polyline -> points every `step` m
    int n = 0; float carry = 0;
    for (int i = 0; i + 1 < nc && n < maxOut; i++) {
        Vector2 a = cp[i], b = cp[i + 1]; float L = Vector2Distance(a, b); if (L < 1e-4f) continue;
        for (float t = carry; t < L && n < maxOut; t += step) out[n++] = V3(a.x + (b.x - a.x) * t / L, 0, a.y + (b.y - a.y) * t / L);
        carry = fmodf(carry - L, step); if (carry < 0) carry += step;
    }
    if (n < maxOut) out[n++] = V3(cp[nc - 1].x, 0, cp[nc - 1].y);
    return n;
}
static int chaikin(Vector2 *p, int n, int maxN, int iters) {   // round corners, keep both ends
    static Vector2 tmp[MAX_PATH];
    for (int it = 0; it < iters; it++) {
        int m = 0; tmp[m++] = p[0];
        for (int i = 0; i + 1 < n && m + 2 < maxN; i++) {
            Vector2 a = p[i], b = p[i + 1];
            if (i > 0) tmp[m++] = Vector2Lerp(a, b, 0.25f);
            if (i + 2 < n) tmp[m++] = Vector2Lerp(a, b, 0.75f);
        }
        tmp[m++] = p[n - 1]; memcpy(p, tmp, m * sizeof(Vector2)); n = m;
    }
    return n;
}
static float pathDist(const Vector3 *p, int n, float x, float z, float *yOut) {
    float best = 1e9f, by = 0;
    for (int i = 0; i + 1 < n; i++) {
        float ax = p[i].x, az = p[i].z, dx = p[i + 1].x - ax, dz = p[i + 1].z - az, L2 = dx * dx + dz * dz;
        float t = L2 > 0 ? clampf(((x - ax) * dx + (z - az) * dz) / L2, 0, 1) : 0;
        float qx = ax + dx * t - x, qz = az + dz * t - z, d = qx * qx + qz * qz;
        if (d < best) { best = d; by = p[i].y + (p[i + 1].y - p[i].y) * t; }
    }
    if (yOut) *yOut = by;
    return sqrtf(best);
}
static float coast_d(float x, float z) { return nCoast > 1 ? pathDist(coastP, nCoast, x, z, NULL) : 1e9f; }
static float touge_d(float x, float z) { return nTouge > 1 ? pathDist(tougeP, nTouge, x, z, NULL) : 1e9f; }
static Vector2 pnorm(const Vector3 *p, int n, int i) {   // left normal (-dz, dx) averaged at point i
    int a = i > 0 ? i - 1 : i, b = i + 1 < n ? i + 1 : i;
    Vector2 d = Vector2Normalize((Vector2){p[b].x - p[a].x, p[b].z - p[a].z});
    return (Vector2){-d.y, d.x};
}
static void addBore(float px, float pz, float fy, int dir, int kind, float len) {
    if (nBore >= MAX_BORE) return;
    bores[nBore++] = (Bore){px, pz, fy, kind ? 5.0f : 9.0f, kind ? 4.4f : 5.6f, len, dir, kind};
}
static float boreDepth(Bore *b) { return b->kind ? 4.0f : 6.0f; }   // headwall thickness
static void boreLocal(Bore *b, float x, float z, float *a, float *l) {
    *a = b->dir == 0 ? x - b->px : b->dir == 1 ? b->px - x : b->dir == 2 ? z - b->pz : b->pz - z;
    *l = (b->dir < 2) ? z - b->pz : x - b->px;
}
static bool seaSide(float x, float z) { return inPoly(dSea, DSEA_N, x, z); }
static float coastSigned(float x, float z) {   // distance to the coast road centre line; negative on the land side
    float best = 1e9f; int bi = 0; float bt = 0;
    for (int i = 0; i + 1 < nCoast; i++) { float ax = coastP[i].x, az = coastP[i].z, dx = coastP[i + 1].x - ax, dz = coastP[i + 1].z - az, L2 = dx * dx + dz * dz;
        float t = L2 > 0 ? clampf(((x - ax) * dx + (z - az) * dz) / L2, 0, 1) : 0, qx = ax + dx * t - x, qz = az + dz * t - z, d = qx * qx + qz * qz;
        if (d < best) { best = d; bi = i; bt = t; } }
    if (nCoast < 2) return 1e9f;
    float dx = coastP[bi + 1].x - coastP[bi].x, dz = coastP[bi + 1].z - coastP[bi].z, px = x - (coastP[bi].x + dx * bt), pz = z - (coastP[bi].z + dz * bt);
    float side = dx * pz - dz * px;   // >0: left of travel (the +normal side, the sea side)
    return side >= 0 ? sqrtf(best) : -sqrtf(best);
}

// ---------------------------------------------------------------- layout definition
static void layoutInit(void) {
    for (int i = 0; i < NB; i++) for (int j = 0; j < NB; j++) blockKindA[i][j] = designKind[i][j];
    nBore = 0; nPad = 0;
    // coast road: from the west tunnel, south along the hills, around the bay past the old town and the fish market, into the east tunnel
    nCoast = loadPath(dCoast, DCOAST_N, coastP);
    for (int i = 0; i < nCoast; i++) coastP[i].y = 0;
    nStreet = 0; for (int i = 0; i < DSTREETPTS_N && i < MAX_STPTS; i++) stPts[i] = V3(dStreetPts[i][0], 0, dStreetPts[i][1]);
    nStreet = DSTREETS_N; nNorth = 0; nAvenue = 0; nRing[0] = nRing[1] = nRing[2] = nRing[3] = 0; nExt = 0;
    nRail = loadPath(dRail, DRAIL_N, railP);
    // tunnels: the coast road leaves through the hills at both ends; the railway runs into the mountain at both ends
    addBore(WEST_PX, WEST_PZ, 0, 1, 0, 40);                        // 0: coast road, west
    addBore(SOUTH_PX, SOUTH_PZ, 0, 2, 0, 52);                      // 1: coast road, south under the castle hill
    addBore(RAIL_E_X, RAIL_E_Z, 0, 0, 0, 70);                      // 2: railway, east into the mountain
    // the touge: from the avenue's east end up the mountain in hairpins to the old tunnel
    nTouge = loadPath(dTouge, DTOUGE_N, tougeP);
    {   float total = 0; for (int i = 1; i < nTouge; i++) total += Vector3Distance(V3(tougeP[i].x, 0, tougeP[i].z), V3(tougeP[i - 1].x, 0, tougeP[i - 1].z));
        float acc = 0; tougeP[0].y = 0;
        for (int i = 1; i < nTouge; i++) { acc += Vector3Distance(V3(tougeP[i].x, 0, tougeP[i].z), V3(tougeP[i - 1].x, 0, tougeP[i - 1].z)); float t = acc / total; tougeP[i].y = 34.0f * (t * t * 0.35f + t * 0.65f); } }
    addBore(tougeP[nTouge - 1].x, tougeP[nTouge - 1].z, tougeP[nTouge - 1].y, 3, 1, 62);   // 3: the old brick tunnel at the top
    addBore(RAIL_W_X, RAIL_W_Z, 0, 1, 0, 50);                      // 4: railway, west
    // hamlet lane down from the first straight, ending in a small square
    nLane = loadPath(dLane, DLANE_N, laneP);
    {   float y0; pathDist(tougeP, nTouge, laneP[0].x, laneP[0].z, &y0); float y1 = y0 - 1.5f;
        for (int i = 0; i < nLane; i++) laneP[i].y = Lerp(y0, y1, (float)i / (nLane > 1 ? nLane - 1 : 1));
        pads[nPad++] = (Pad){HAMLET_X, HAMLET_Z, y1, 12, 10}; }
    // a terrace under every old house, and under each mountain shrine
    for (int h = 0; h < DHOUSES_N && nPad < MAX_PAD; h++) { float x = dHouses[h][0], z = dHouses[h][1], yt, yl;
        float dt = pathDist(tougeP, nTouge, x, z, &yt), dl = pathDist(laneP, nLane, x, z, &yl);
        pads[nPad++] = (Pad){x, z, dl < dt ? yl : yt, fmaxf(dHouses[h][2], dHouses[h][3]) / 2 + 1.5f, 6}; }
    for (int k = 0; k < DSHRINES_N && nPad < MAX_PAD; k++) { float x = dShrines[k][0], z = dShrines[k][1], yt; pathDist(tougeP, nTouge, x, z, &yt);
        pads[nPad++] = (Pad){x, z, yt + 2, 3.5f, 7}; }
}

// ---------------------------------------------------------------- heightfield bake
static void bakeHeightfield(void) {
    int N = HF_N;
    if (!hf) { hf = malloc(sizeof(float) * N * N); hfd = malloc(sizeof(float) * N * N); }
    townReady = false; if (!townMask) townMask = malloc((size_t)N * N);
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) townMask[j * N + i] = inTownRaw(HF_X0 + i * HF_S, HF_X0 + j * HF_S);
    townReady = true;
    // distance to city land: two-pass chamfer transform
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) { float x = HF_X0 + i * HF_S, z = HF_X0 + j * HF_S; hfd[j * N + i] = (cityLand(x, z) || inCore(x, z)) ? 0 : 1e9f; }
    const float A = HF_S, B = HF_S * 1.4142f;
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) { float *d = &hfd[j * N + i];
        if (i > 0) *d = fminf(*d, d[-1] + A); if (j > 0) { *d = fminf(*d, d[-N] + A); if (i > 0) *d = fminf(*d, d[-N - 1] + B); if (i < N - 1) *d = fminf(*d, d[-N + 1] + B); } }
    for (int j = N - 1; j >= 0; j--) for (int i = N - 1; i >= 0; i--) { float *d = &hfd[j * N + i];
        if (i < N - 1) *d = fminf(*d, d[1] + A); if (j < N - 1) { *d = fminf(*d, d[N] + A); if (i < N - 1) *d = fminf(*d, d[N + 1] + B); if (i > 0) *d = fminf(*d, d[N - 1] + B); } }
    for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) {
        float x = HF_X0 + i * HF_S, z = HF_X0 + j * HF_S, d = hfd[j * N + i];
        float k = clampf(d / 45, 0, 1); k = k * k * (3 - 2 * k);
        float hills = vnoise(x * 0.011f, z * 0.011f, 77) * 24 + vnoise(x * 0.03f, z * 0.03f, 91) * 6;
        float h = -0.3f + k * hills;
        float md = ((x - MT_X) * (x - MT_X) + (z - MT_Z) * (z - MT_Z)) / (MT_R * MT_R);
        float mk = clampf(d / 30, 0, 1); h += MT_H * expf(-md) * mk * mk * (3 - 2 * mk);
        // flattened corridors: coast road and the north approach at street level
        float cd = coast_d(x, z), w = 1 - clampf((cd - 8) / 12, 0, 1); w = w * w * (3 - 2 * w); h = Lerp(h, SIDEH - 0.01f, w);
        float rd = nRail > 1 ? pathDist(railP, nRail, x, z, NULL) : 1e9f; w = 1 - clampf((rd - 6) / 12, 0, 1); w = w * w * (3 - 2 * w); h = Lerp(h, SIDEH - 0.01f, w);   // the railway's shelf
        float nd = pathDist(northP, nNorth, x, z, NULL); w = 1 - clampf((nd - 6) / 12, 0, 1); w = w * w * (3 - 2 * w); h = Lerp(h, -0.05f, w);
        // the bay: a beach at the foot of the seawall, then the sea floor falling away from the shoreline
        if (seaSide(x, z) && d > 0) { float ds = polyEdgeDist(dSea, DSEA_N, x, z); h = SEA_Y - 0.3f - fminf(ds * 0.12f, 6.0f); }
        else if (d > 0 && cd < 40 && coastSigned(x, z) > 4.6f) h = SEA_Y + 0.3f + clampf((polyEdgeDist(dSea, DSEA_N, x, z) - 2) * 0.03f, 0, 0.5f);
        if (inRiverRaw(x, z)) h = RIVER_Y - 1.0f;   // the river bed between its walls
        // the touge: road height blended into the slope (weighted by every nearby stretch so the rows terrace smoothly)
        {   float sw = 0, sy = 0, wmax = 0;
            for (int s = 0; s + 1 < nTouge; s += 1) {
                float ax = tougeP[s].x, az = tougeP[s].z, dx = tougeP[s + 1].x - ax, dz = tougeP[s + 1].z - az, L2 = dx * dx + dz * dz;
                if (fabsf(x - ax) > 20 || fabsf(z - az) > 20) continue;
                float t = L2 > 0 ? clampf(((x - ax) * dx + (z - az) * dz) / L2, 0, 1) : 0;
                float q = sqrtf((ax + dx * t - x) * (ax + dx * t - x) + (az + dz * t - z) * (az + dz * t - z));
                float ww = 1 - clampf((q - 4.5f) / 9, 0, 1); ww = ww * ww * (3 - 2 * ww); if (ww <= 0) continue;
                float yy = tougeP[s].y + (tougeP[s + 1].y - tougeP[s].y) * t;
                sw += ww; sy += ww * yy; if (ww > wmax) wmax = ww;
            }
            for (int s = 0; s + 1 < nLane; s++) { float y; float q = pathDist(laneP, nLane, x, z, &y); float ww = 1 - clampf((q - 3) / 6, 0, 1); if (ww > 0) { sw += ww; sy += ww * y; if (ww > wmax) wmax = ww; } break; }
            if (sw > 0) h = Lerp(h, sy / sw - 0.06f, wmax);
        }
        for (int p = 0; p < nPad; p++) { float q = sqrtf((x - pads[p].x) * (x - pads[p].x) + (z - pads[p].z) * (z - pads[p].z));
            float ww = 1 - clampf((q - pads[p].r) / pads[p].blend, 0, 1); ww = ww * ww * (3 - 2 * ww); h = Lerp(h, pads[p].y - 0.05f, ww); }
        // ridges over the tunnels so every bore stays underground
        for (int b = 0; b < nBore; b++) { Bore *br = &bores[b]; float a, l; boreLocal(br, x, z, &a, &l);
            if (a < boreDepth(br) - 2 || a > br->len + 30) continue;
            float c = br->fy + br->h + 3.5f, wl = 1 - clampf((fabsf(l) - (br->w / 2 + 5)) / 12, 0, 1); wl = wl * wl * (3 - 2 * wl);
            float wa = clampf((a - (boreDepth(br) - 2)) / 2, 0, 1);
            if (c > h) h = Lerp(h, c, wl * wa); }
        if (inCore(x, z)) h = SIDEH - 0.01f;   // the town: flat paving; streets are drawn on top of it
        hf[j * N + i] = h;
    }
    hfReady = true;
}
static bool terrainSkip(float ax, float az) {   // terrain cells inside a headwall, over the bore mouth
    float cx = ax + HF_S / 2, cz = az + HF_S / 2;
    for (int b = 0; b < nBore; b++) { float a, l; boreLocal(&bores[b], cx, cz, &a, &l);
        if (a > 0 && a < boreDepth(&bores[b]) && fabsf(l) < bores[b].w / 2 + 1.6f) return true; }
    return false;
}

// ---------------------------------------------------------------- road ribbons
static void ribbon(int t, const Vector3 *p, int n, float oA, float oB, float yA, float yB, Color c, bool (*skip)(Vector3 q)) {
    for (int i = 0; i + 1 < n; i++) {
        Vector2 n0 = pnorm(p, n, i), n1 = pnorm(p, n, i + 1);
        if (skip) { Vector3 m = Vector3Lerp(p[i], p[i + 1], 0.5f); Vector2 nm = Vector2Scale(Vector2Add(n0, n1), 0.5f); if (skip(V3(m.x + nm.x * (oA + oB) / 2, m.y, m.z + nm.y * (oA + oB) / 2))) continue; }
        Vector3 a0 = V3(p[i].x + n0.x * oA, p[i].y + yA, p[i].z + n0.y * oA), a1 = V3(p[i + 1].x + n1.x * oA, p[i + 1].y + yA, p[i + 1].z + n1.y * oA);
        Vector3 b0 = V3(p[i].x + n0.x * oB, p[i].y + yB, p[i].z + n0.y * oB), b1 = V3(p[i + 1].x + n1.x * oB, p[i + 1].y + yB, p[i + 1].z + n1.y * oB);
        quad(t, a0, a1, b1, b0, V3(0, 1, 0), c); quad(t, b0, b1, a1, a0, V3(0, 1, 0), c);   // both windings: never culled
    }
}
static void wallRibbon(int t, const Vector3 *p, int n, float o, float y0, float y1, Color c, bool (*skip)(Vector3 q)) {
    for (int i = 0; i + 1 < n; i++) {
        Vector2 n0 = pnorm(p, n, i), n1 = pnorm(p, n, i + 1);
        Vector3 m = Vector3Lerp(p[i], p[i + 1], 0.5f); if (skip && skip(V3(m.x + n0.x * o, m.y, m.z + n0.y * o))) continue;
        Vector3 a0 = V3(p[i].x + n0.x * o, p[i].y + y0, p[i].z + n0.y * o), a1 = V3(p[i + 1].x + n1.x * o, p[i + 1].y + y0, p[i + 1].z + n1.y * o);
        Vector3 b0 = V3(a0.x, p[i].y + y1, a0.z), b1 = V3(a1.x, p[i + 1].y + y1, a1.z);
        Vector3 nn = V3(n0.x, 0, n0.y);
        quad(t, a0, a1, b1, b0, nn, c); quad(t, b0, b1, a1, a0, Vector3Negate(nn), c);
    }
}
static void dashes(const Vector3 *p, int n, float o, float y, float w, int on, int off, Color c) {
    for (int i = 0; i + 1 < n; i++) { if (off > 0 && (i % (on + off)) >= on) continue;
        Vector3 seg[2] = {p[i], p[i + 1]}; ribbon(T_PLAIN, seg, 2, o - w / 2, o + w / 2, y, y, c, NULL); }
}
static void railSolids(const Vector3 *p, int n, float o, float h, float th) {   // collision chain along an offset line
    for (int i = 0; i + 1 < n; i++) {
        Vector2 n0 = pnorm(p, n, i);
        float x0 = p[i].x + n0.x * o, z0 = p[i].z + n0.y * o, x1 = p[i + 1].x + n0.x * o, z1 = p[i + 1].z + n0.y * o;
        float y = fminf(p[i].y, p[i + 1].y);
        addSolid(V3(fminf(x0, x1) - th, y - 0.4f, fminf(z0, z1) - th), V3(fmaxf(x0, x1) + th, fmaxf(p[i].y, p[i + 1].y) + h, fmaxf(z0, z1) + th));
    }
}
static bool onCityLand(Vector3 q) { return cityLand(q.x, q.z); }

// ---------------------------------------------------------------- small structures
static void torii(float x, float y, float z, bool alongX, float s, Color red) {
    float hw = 1.5f * s, h = 3.1f * s, t = 0.14f * s;
    if (alongX) {   // posts on the z axis, passage along x
        box(T_PLAIN, V3(x - t, y - 0.3f, z - hw - t), V3(x + t, y + h, z - hw + t), red, true, ALL);
        box(T_PLAIN, V3(x - t, y - 0.3f, z + hw - t), V3(x + t, y + h, z + hw + t), red, true, ALL);
        box(T_PLAIN, V3(x - t * 1.3f, y + h, z - hw - 0.6f * s), V3(x + t * 1.3f, y + h + 0.24f * s, z + hw + 0.6f * s), rgb(40, 30, 30), false, ALLB);
        box(T_PLAIN, V3(x - t, y + h - 0.6f * s, z - hw - 0.2f * s), V3(x + t, y + h - 0.45f * s, z + hw + 0.2f * s), red, false, ALLB);
    } else {
        box(T_PLAIN, V3(x - hw - t, y - 0.3f, z - t), V3(x - hw + t, y + h, z + t), red, true, ALL);
        box(T_PLAIN, V3(x + hw - t, y - 0.3f, z - t), V3(x + hw + t, y + h, z + t), red, true, ALL);
        box(T_PLAIN, V3(x - hw - 0.6f * s, y + h, z - t * 1.3f), V3(x + hw + 0.6f * s, y + h + 0.24f * s, z + t * 1.3f), rgb(40, 30, 30), false, ALLB);
        box(T_PLAIN, V3(x - hw - 0.2f * s, y + h - 0.6f * s, z - t), V3(x + hw + 0.2f * s, y + h - 0.45f * s, z + t), red, false, ALLB);
    }
}
static void hokora(float x, float y, float z) {   // small shrine with an offering box; a permanent ward
    box(T_CONCRETE, V3(x - 1.0f, y - 0.4f, z - 0.8f), V3(x + 1.0f, y + 0.4f, z + 0.8f), rgb(160, 160, 150), true, ALL);
    box(T_WOOD, V3(x - 0.6f, y + 0.4f, z - 0.5f), V3(x + 0.6f, y + 1.5f, z + 0.5f), rgb(150, 110, 80), true, ALL);
    box(T_ROOF, V3(x - 0.85f, y + 1.5f, z - 0.75f), V3(x + 0.85f, y + 1.7f, z + 0.75f), rgb(90, 80, 75), false, ALLB);
    box(T_CLOTH, V3(x - 0.62f, y + 1.3f, z + 0.5f), V3(x + 0.62f, y + 1.42f, z + 0.56f), rgb(225, 205, 145), false, ALLB);
    addCont(CK_OFFERING, V3(x, y + 0.4f, z + 0.9f), 0, 0, true);
    addLight(V3(x, y + 1.2f, z + 1.0f), V3(1.0f, 0.55f, 0.25f), 6, 1);
    addWard(V3(x, y, z), 5.5f, 0, 2);
}
static void oldHouse(float x, float y, float z, float w, float d, int face) {   // dark wooden minka on a stone base; enterable
    Color wood = rgb(110, 82, 62), dark = rgb(70, 52, 40);
    float x0 = x - w / 2, x1 = x + w / 2, z0 = z - d / 2, z1 = z + d / 2, H = 2.6f, T = 0.18f;
    box(T_CONCRETE, V3(x0 - 0.3f, y - 3.5f, z0 - 0.3f), V3(x1 + 0.3f, y + 0.3f, z1 + 0.3f), rgb(120, 118, 110), true, ALL);
    box(T_TATAMI, V3(x0 + T, y + 0.3f, z0 + T), V3(x1 - T, y + 0.34f, z1 - T), rgb(190, 180, 130), false, 16);
    float yb = y + 0.3f;
    for (int s = 0; s < 4; s++) {
        bool alongX = s == 0 || s == 2; float a0 = alongX ? x0 : z0, a1 = alongX ? x1 : z1, mid = (a0 + a1) / 2;
        float c = s == 0 ? z0 : s == 2 ? z1 - T : s == 1 ? x1 - T : x0;
        float gap0 = s == face ? mid - 0.6f : mid - 0.5f, gap1 = s == face ? mid + 0.6f : mid + 0.5f;   // door on the face side, a window elsewhere
        float gy0 = s == face ? yb : yb + 1.0f, gy1 = s == face ? yb + 2.0f : yb + 1.7f;
        #define WB(A0, A1, Y0, Y1) do { if ((A1) - (A0) > 0.05f) { if (alongX) box(T_WOOD, V3(A0, Y0, c), V3(A1, Y1, c + T), wood, true, ALL); else box(T_WOOD, V3(c, Y0, A0), V3(c + T, Y1, A1), wood, true, ALL); } } while (0)
        WB(a0, gap0, yb, yb + H); WB(gap1, a1, yb, yb + H); WB(gap0, gap1, gy1, yb + H); if (gy0 > yb) WB(gap0, gap1, yb, gy0);
        #undef WB
    }
    for (int k = 0; k < 3; k++) {   // stepped roof
        float o = 0.6f - k * 0.55f, yy = yb + H + k * 0.32f;
        box(T_ROOF, V3(x0 - o, yy, z0 - o), V3(x1 + o, yy + 0.34f, z1 + o), dark, k == 0, ALLB);
    }
    int ck = ri(3) == 0 ? CK_CABINET : ri(2) ? CK_WARDROBE : CK_DESK;
    float cx = face == 0 || face == 2 ? x : (face == 1 ? x0 + 0.9f : x1 - 0.9f), cz = face == 1 || face == 3 ? z : (face == 0 ? z1 - 0.7f : z0 + 0.7f);
    addCont(ck, V3(cx, yb + 0.04f, cz), face == 0 ? PI : face == 2 ? 0 : face == 1 ? PI / 2 : -PI / 2, 1, true);
    if (fr(0, 1) < 0.6f) addLight(V3(x, yb + 2.2f, z), V3(1.0f, 0.6f, 0.3f), 6, ri(3));
    if (nMapFoot < MAX_MAPFOOT) { mapFootH[nMapFoot] = 3.4f; mapFootY[nMapFoot] = y; mapFoot[nMapFoot++] = (Rectangle){x0, z0, w, d}; }
}
static void cedar(float x, float y, float z, float h) {
    box(T_WOOD, V3(x - 0.2f, y - 0.5f, z - 0.2f), V3(x + 0.2f, y + h * 0.45f, z + 0.2f), rgb(90, 70, 55), true, ALL);
    for (int k = 0; k < 3; k++) { float r = (1.6f - k * 0.45f) * h / 9, yy = y + h * (0.3f + k * 0.22f);
        box(T_HEDGE, V3(x - r, yy, z - r), V3(x + r, yy + h * 0.28f, z + r), rgb(55, 95, 60), false, ALLB); }
}
static void tetrapod(float x, float y, float z) {
    Color c = rgb(170, 168, 160);
    box(T_CONCRETE, V3(x - 1.0f, y, z - 0.3f), V3(x + 1.0f, y + 0.6f, z + 0.3f), c, false, ALLB);
    box(T_CONCRETE, V3(x - 0.3f, y, z - 1.0f), V3(x + 0.3f, y + 0.6f, z + 1.0f), c, false, ALLB);
    box(T_CONCRETE, V3(x - 0.3f, y - 0.2f, z - 0.3f), V3(x + 0.3f, y + 1.3f, z + 0.3f), c, false, ALLB);
}

// ---------------------------------------------------------------- tunnels
static void boreBox(Bore *b, float a0, float a1, float l0, float l1, float y0, float y1, int t, Color c, bool solid, int mask) {
    float ax0 = fminf(a0, a1), ax1 = fmaxf(a0, a1), lx0 = fminf(l0, l1), lx1 = fmaxf(l0, l1);
    Vector3 mn, mx;
    switch (b->dir) {
    case 0: mn = V3(b->px + ax0, y0, b->pz + lx0); mx = V3(b->px + ax1, y1, b->pz + lx1); break;
    case 1: mn = V3(b->px - ax1, y0, b->pz + lx0); mx = V3(b->px - ax0, y1, b->pz + lx1); break;
    case 2: mn = V3(b->px + lx0, y0, b->pz + ax0); mx = V3(b->px + lx1, y1, b->pz + ax1); break;
    default: mn = V3(b->px + lx0, y0, b->pz - ax1); mx = V3(b->px + lx1, y1, b->pz - ax0); break;
    }
    box(t, mn, mx, c, solid, mask);
}
static Vector3 boreAt(Bore *b, float a, float l, float y) {
    switch (b->dir) { case 0: return V3(b->px + a, y, b->pz + l); case 1: return V3(b->px - a, y, b->pz + l); case 2: return V3(b->px + l, y, b->pz + a); default: return V3(b->px + l, y, b->pz - a); }
}
static void buildBore(Bore *b) {
    bool old = b->kind == 1; float W = b->w, H = b->h, y = b->fy, L = b->len, D = boreDepth(b);
    Color wallC = old ? rgb(150, 80, 60) : rgb(200, 205, 200), ceilC = old ? rgb(120, 70, 55) : rgb(150, 150, 145);
    int wallT = old ? T_BLOCKWALL : T_TILEWALL;
    // headwall: as tall as the ground anywhere in its footprint
    // headwall in 2 m columns, each as tall as the ground behind it, so it steps down with the hillside
    float FW = W / 2 + 14, OW = W / 2 + 0.6f;
    Color hw = old ? rgb(140, 75, 55) : rgb(185, 185, 178);
    float edges[64]; int ne = 0;
    for (float l = -FW; l < -OW - 0.01f; l += 2) edges[ne++] = l;
    edges[ne++] = -OW; edges[ne++] = OW;
    for (float l = OW + 2; l < FW - 0.01f; l += 2) edges[ne++] = l;
    edges[ne++] = FW;
    for (int e = 0; e + 1 < ne; e++) {
        float l0 = edges[e], l1 = edges[e + 1], top = y + H + 1.5f;
        for (float a = 0; a <= D; a += 1) for (float l = l0; l <= l1 + 0.01f; l += 1) { Vector3 q = boreAt(b, a, l, 0); top = fmaxf(top, terrainH(q.x, q.z) + 0.6f); }
        bool open = l0 >= -OW - 0.01f && l1 <= OW + 0.01f;
        boreBox(b, 0, D, l0, l1, open ? y + H + 0.5f : y - 1, top, old ? T_BLOCKWALL : T_CONCRETE, hw, true, ALLB);
    }
    boreBox(b, -0.4f, 0, -W / 2 - 1.2f, W / 2 + 1.2f, y + H + 0.5f, y + H + 1.3f, T_CONCRETE, old ? rgb(110, 60, 45) : rgb(215, 215, 208), false, ALLB);   // portal frame
    boreBox(b, -0.4f, 0, -W / 2 - 1.2f, -W / 2 - 0.6f, y, y + H + 0.5f, T_CONCRETE, old ? rgb(110, 60, 45) : rgb(215, 215, 208), false, ALLB);
    boreBox(b, -0.4f, 0, W / 2 + 0.6f, W / 2 + 1.2f, y, y + H + 0.5f, T_CONCRETE, old ? rgb(110, 60, 45) : rgb(215, 215, 208), false, ALLB);
    {   // name plate with the tunnel's name in Japanese
        int face = b->dir == 0 ? 3 : b->dir == 1 ? 1 : b->dir == 2 ? 0 : 2;
        Vector3 p0 = boreAt(b, -0.46f, -2.4f, y + H + 0.55f), p1 = boreAt(b, -0.4f, 2.4f, y + H + 1.35f);
        if (nSign < MAX_SIGN) signs[nSign++] = (Sign){Vector3Min(p0, p1), Vector3Max(p0, p1), old ? rgb(70, 55, 45) : rgb(30, 90, 60), -1, T_SIGNH, SGN_TUN0 + (int)(b - bores), face, true};
    }
    // interior
    boreBox(b, 0, L + 6, -W / 2 - 0.6f, W / 2 + 0.6f, y - 0.6f, y, old ? T_CONCRETE : T_ASPHALT, old ? rgb(90, 85, 80) : WHITE, true, 16);
    boreBox(b, D, L + 6, -W / 2 - 0.6f, -W / 2, y, y + H, wallT, wallC, true, ALL);
    boreBox(b, D, L + 6, W / 2, W / 2 + 0.6f, y, y + H, wallT, wallC, true, ALL);
    boreBox(b, D, L + 6, -W / 2 - 0.6f, W / 2 + 0.6f, y + H, y + H + 0.5f, T_CONCRETE, ceilC, true, ALLB);
    if (!old) {
        boreBox(b, 0, L + 6, -W / 2, -W / 2 + 0.9f, y, y + 0.22f, T_SIDEWALK, rgb(170, 170, 165), true, ALL);
        boreBox(b, 0, L + 6, W / 2 - 0.9f, W / 2, y, y + 0.22f, T_SIDEWALK, rgb(170, 170, 165), true, ALL);
        for (float a = 1; a < L; a += 6) boreBox(b, a, a + 3, -0.08f, 0.08f, y, y + 0.012f, T_PLAIN, rgb(230, 230, 220), false, 16);
        for (float a = 8; a < L - 4; a += 12) {   // sodium lights, many dead
            boreBox(b, a - 0.6f, a + 0.6f, -0.3f, 0.3f, y + H - 0.15f, y + H, T_METAL, rgb(80, 80, 80), false, ALLB);
            float r = fr(0, 1); Vector3 lp = boreAt(b, a, 0, y + H - 0.4f); addLight(lp, V3(1.0f, 0.6f, 0.25f), 10, r < 0.35f ? 0 : r < 0.6f ? 1 : 2);
        }
        for (float a = 30; a < L - 8; a += 45) { Vector3 cp = boreAt(b, a, (ri(2) ? 1 : -1) * (W / 2 - 0.3f), y + 0.22f); addCont(CK_FIRECAB, cp, b->dir < 2 ? (cp.z > b->pz ? PI : 0) : (cp.x > b->px ? -PI / 2 : PI / 2), 0, true); }
        for (int k = 0; k < 3; k++) { float a = fr(14, L - 14), l = (ri(2) ? 1 : -1) * 1.9f; Vector3 q = boreAt(b, a, l, 0); car(q.x, q.z, b->dir >= 2);
            if (fr(0, 1) < 0.6f) { Vector3 tq = boreAt(b, a + 2.6f, l, y); addCont(CK_TRUNK, tq, 0, 0, false); } }
    } else {
        for (float a = D + 2; a < L; a += 3.5f) boreBox(b, a, a + 0.5f, -W / 2, W / 2, y + H - 0.25f, y + H, T_BLOCKWALL, rgb(100, 55, 45), false, ALLB);   // arch ribs
        // the seal: concrete blocks two rows high; a gap in the top row lets you climb over
        float sa = D + 1.0f;
        for (float l = -W / 2; l < W / 2 - 0.01f; l += 1.0f) boreBox(b, sa, sa + 1.0f, l, l + 0.98f, y, y + 1.0f, T_CONCRETE, rgb(190, 188, 180), true, ALL);
        for (float l = -W / 2; l < W / 2 - 0.01f; l += 1.0f) if (l < -0.6f || l > 0.4f) boreBox(b, sa, sa + 1.0f, l, l + 0.98f, y + 1.0f, y + 2.0f, T_CONCRETE, rgb(180, 178, 170), true, ALL);
        {   int face = b->dir == 0 ? 3 : b->dir == 1 ? 1 : b->dir == 2 ? 0 : 2;   // a notice pasted on the blocks
            Vector3 p0 = boreAt(b, sa - 0.04f, -2.0f, y + 0.2f), p1 = boreAt(b, sa, -0.2f, y + 0.92f);
            if (nSign < MAX_SIGN) signs[nSign++] = (Sign){Vector3Min(p0, p1), Vector3Max(p0, p1), rgb(232, 224, 205), -1, T_SIGNH, SGN_SEAL, face, true}; }
        Vector3 op = boreAt(b, sa + 14, 0, y); addCont(CK_OFFERING, op, 0, 0, true);
        for (int k = 0; k < 2; k++) { Vector3 q = boreAt(b, sa + 24 + k * 16, (k ? 1 : -1) * 1.2f, y); addCrate(q); }
        addLight(boreAt(b, sa + 30, 0, y + 2), V3(0.6f, 0.75f, 1.0f), 6, 1);   // something glows down there
    }
    // the collapse: a slope of rubble that fills the bore
    for (int k = 0; k < 28; k++) {
        float a = L - 6 + fr(0, 10), l = fr(-W / 2, W / 2), s = fr(0.6f, 2.0f);
        float hgt = clampf((a - (L - 6)) / 6, 0.1f, 1) * (H + 0.4f);
        boreBox(b, a, a + s, l, fminf(l + s, W / 2), y, y + fmaxf(0.5f, hgt * fr(0.6f, 1.0f)), T_CONCRETE, rgb(fr(110, 150), fr(105, 140), fr(95, 125)), true, ALL);
    }
    boreBox(b, L, L + 6, -W / 2, W / 2, y, y + H, T_CONCRETE, rgb(115, 110, 100), true, ALL);
    boreBox(b, L - 3, L + 1, -W / 2, 0.5f, y + H - 1.2f, y + H - 0.6f, T_CONCRETE, rgb(140, 138, 130), true, ALL);   // a fallen ceiling slab
    for (int k = 0; k < 6; k++) { float a = L - 4 + fr(0, 4); float l = fr(-W / 2 + 0.5f, W / 2 - 0.5f); boreBox(b, a, a + 0.06f, l, l + 0.06f, y + fr(0.5f, 3), y + fr(3, H), T_RUST, WHITE, false, ALLB); }   // rebar
}

// ---------------------------------------------------------------- the rest of the outside world
static bool coastSkipVerge(Vector3 q) { return cityLand(q.x, q.z) || roadAt(q.x, q.z) || seaSide(q.x, q.z) || !inCore(q.x, q.z); }   // gaps where streets meet the coast road
static bool coastSkipPave(Vector3 q) { return cityLand(q.x, q.z) || seaDepth(q.x, q.z) > 0; }
static void genLayoutWorld(void) {
    // coast road: two lanes, pavement on the land side, seawall on the sea side
    Color asp = rgb(255, 255, 255);
    ribbon(T_ASPHALT, coastP, nCoast, -3.7f, 3.7f, SIDEH + 0.012f, SIDEH + 0.012f, asp, NULL);
    dashes(coastP, nCoast, 0, SIDEH + 0.025f, 0.15f, 2, 2, rgb(235, 235, 225));
    dashes(coastP, nCoast, -3.3f, SIDEH + 0.025f, 0.12f, 1, 0, rgb(235, 235, 225)); dashes(coastP, nCoast, 3.3f, SIDEH + 0.025f, 0.12f, 1, 0, rgb(235, 235, 225));
    ribbon(T_SIDEWALK, coastP, nCoast, -6.2f, -3.7f, 0.2f, 0.2f, WHITE, coastSkipPave); wallRibbon(T_CONCRETE, coastP, nCoast, -3.7f, 0.0f, 0.2f, rgb(170, 170, 165), coastSkipPave);
    ribbon(T_SIDEWALK, coastP, nCoast, 3.7f, 4.6f, SIDEH + 0.05f, SIDEH + 0.05f, WHITE, NULL);
    ribbon(T_CONCRETE, coastP, nCoast, 4.6f, 5.2f, 1.05f, 1.05f, rgb(215, 212, 200), NULL);
    wallRibbon(T_CONCRETE, coastP, nCoast, 4.6f, SIDEH, 1.05f, rgb(200, 198, 188), NULL); wallRibbon(T_CONCRETE, coastP, nCoast, 5.2f, -4.4f, 1.05f, rgb(185, 183, 172), NULL);   // the seawall: about 4 m down to the beach
    railSolids(coastP, nCoast, 4.9f, 1.05f, 0.25f);
    for (int i = 10; i + 10 < nCoast; i += 1) {   // bus stops, vending machines, poles; breakwater blocks out in the water
        Vector2 nn = pnorm(coastP, nCoast, i); Vector3 p = coastP[i];
        if (i % 38 == 19) { float x = p.x - nn.x * 5.3f, z = p.z - nn.y * 5.3f; if (pointFree(x, z, 1.0f, SIDEH + 0.1f)) {
            box(T_METAL, V3(x - 0.06f, 0.2f, z - 0.06f), V3(x + 0.06f, 2.5f, z + 0.06f), rgb(80, 80, 85), true, ALL);
            box(T_PLAIN, V3(x - 0.3f, 2.2f, z - 0.3f), V3(x + 0.3f, 2.6f, z + 0.3f), rgb(60, 120, 170), false, ALLB);
            box(T_WOOD, V3(x - 0.9f, 0.6f, z - 0.25f), V3(x + 0.9f, 0.7f, z + 0.25f), rgb(140, 110, 80), true, ALL); } }
        if (i % 38 == 30) { float x = p.x - nn.x * 6.2f, z = p.z - nn.y * 6.2f; if (pointFree(x, z, 0.8f, SIDEH + 0.1f)) vending(x, z, fabsf(nn.x) > fabsf(nn.y) ? (nn.x > 0 ? 1 : 3) : (nn.y > 0 ? 2 : 0)); }
        if (i % 12 == 6) { float x = p.x - nn.x * 6.6f, z = p.z - nn.y * 6.6f; if (pointFree(x, z, 0.4f, SIDEH + 0.1f)) {
            box(T_CONCRETE, V3(x - 0.14f, -0.2f, z - 0.14f), V3(x + 0.14f, 8.6f, z + 0.14f), rgb(150, 150, 140), true, ALL);
            if (i % 24 == 6) addLight(V3(x + nn.x * 1.3f, 6.0f, z + nn.y * 1.3f), V3(1.0f, 0.62f, 0.28f), 12, ri(3)); } }
        if ((i / 30) % 2 == 1 && i % 3 == 0) { float o = fr(14, 20), x = p.x + nn.x * o, z = p.z + nn.y * o; if (seaDepth(x, z) > 0.2f) tetrapod(x + fr(-1, 1), SEA_Y - 0.6f, z + fr(-1, 1)); }
    }
    // the strip between the coast road and the hills: pines and benches
    for (int k = 0; k < 140; k++) { float x = fr(-60, 520), z = fr(150, 640);
        if (cityLand(x, z) || inCore(x, z) || seaSide(x, z) || coast_d(x, z) < 8 || coast_d(x, z) > 34 || !pointFree(x, z, 1.2f, 0.1f)) continue;
        float y = terrainH(x, z); if (y < -0.5f || y > 6) continue;
        if (k % 3 == 0) { box(T_WOOD, V3(x, y + 0.4f, z), V3(x + 1.8f, y + 0.5f, z + 0.5f), WHITE, true, ALL); }
        else { float h = fr(3.5f, 6); box(T_WOOD, V3(x - 0.18f, y - 0.3f, z - 0.18f), V3(x + 0.18f, y + h, z + 0.18f), rgb(110, 85, 70), true, ALL);
            box(T_HEDGE, V3(x - 1.8f, y + h - 0.4f, z - 1.4f), V3(x + 1.8f, y + h + 0.5f, z + 1.4f), rgb(70, 120, 80), false, ALLB); }
    }
    // paving inside the town's curved outline wherever the street grid does not reach
    {   int N = HF_N;
        for (int j = 0; j + 1 < N; j++) for (int i = 0; i + 1 < N; i++) { float x0 = HF_X0 + i * HF_S, z0 = HF_X0 + j * HF_S;
            if (!inCore(x0 + 0.1f, z0 + 0.1f) || !inCore(x0 + HF_S - 0.1f, z0 + 0.1f) || !inCore(x0 + 0.1f, z0 + HF_S - 0.1f) || !inCore(x0 + HF_S - 0.1f, z0 + HF_S - 0.1f)) continue;
            float y00 = hf[j * N + i], y10 = hf[j * N + i + 1], y01 = hf[(j + 1) * N + i], y11 = hf[(j + 1) * N + i + 1];
            quad(T_SIDEWALK, V3(x0, y01, z0 + HF_S), V3(x0 + HF_S, y11, z0 + HF_S), V3(x0 + HF_S, y10, z0), V3(x0, y00, z0), V3(0, 1, 0), rgb(205, 202, 194)); } }
    // street ends carried on to the main roads
    for (int e = 0; e < nExt; e++) { Rectangle r = extR[e]; tileBox(T_ASPHALT, r, -0.5f, 0.02f, WHITE, 16);
        bool alongZ = r.height > r.width; float c = alongZ ? r.x + r.width / 2 : r.y + r.height / 2, a0 = alongZ ? r.y : r.x, a1 = alongZ ? r.y + r.height : r.x + r.width;
        for (float u = a0 + 1; u < a1 - 2; u += 3) { if (alongZ) box(T_PLAIN, V3(c - 0.07f, 0.02f, u), V3(c + 0.07f, 0.07f, u + 1.5f), rgb(190, 190, 180), false, 16); else box(T_PLAIN, V3(u, 0.02f, c - 0.07f), V3(u + 1.5f, 0.07f, c + 0.07f), rgb(190, 190, 180), false, 16); } }
    // Omotesando-style walkway edging along every main road and extension: low hedge planters, wooden bollards, zelkova trees
    {   Vector3 *paths[6] = {ringP[0], ringP[1], ringP[2], ringP[3], avenueP, coastP}; int np[6] = {nRing[0], nRing[1], nRing[2], nRing[3], nAvenue, nCoast}; float off[6] = {6.0f, 6.0f, 6.0f, 6.0f, 8.6f, 6.4f};
        for (int p = 0; p < 6; p++) for (int sg = -1; sg <= 1; sg += 2) for (int i = 1; i + 1 < np[p]; i++) {
            Vector2 nn = pnorm(paths[p], np[p], i); float x = paths[p][i].x + nn.x * off[p] * sg, z = paths[p][i].z + nn.y * off[p] * sg;
            if (!inCore(x, z) || cityLand(x, z) || roadAt(x, z) || cityLand(x + nn.x * sg * 2, z + nn.y * sg * 2)) continue;
            float y = terrainH(x, z); int k = i % 9;
            if (k <= 4) { box(T_CONCRETE, V3(x - 0.75f, y - 0.1f, z - 0.75f), V3(x + 0.75f, y + 0.35f, z + 0.75f), rgb(150, 148, 140), true, ALL);
                box(T_HEDGE, V3(x - 0.7f, y + 0.35f, z - 0.7f), V3(x + 0.7f, y + 0.95f, z + 0.7f), rgb(64, 118, 58), false, ALLB); }
            else if (k == 5 || k == 8) box(T_WOOD, V3(x - 0.13f, y, z - 0.13f), V3(x + 0.13f, y + 0.9f, z + 0.13f), rgb(196, 150, 80), true, ALL);
            else if (k == 7) { box(T_WOOD, V3(x - 0.22f, y - 0.2f, z - 0.22f), V3(x + 0.22f, y + 4.2f, z + 0.22f), rgb(80, 66, 54), true, ALL);
                box(T_HEDGE, V3(x - 2.6f, y + 3.6f, z - 2.6f), V3(x + 2.6f, y + 5.4f, z + 2.6f), rgb(120, 140, 50), false, ALLB);
                box(T_HEDGE, V3(x - 1.8f, y + 5.4f, z - 1.8f), V3(x + 1.8f, y + 6.5f, z + 1.8f), rgb(150, 150, 55), false, ALLB); }
        }
        for (int e = 0; e < nExt; e++) { Rectangle r = extR[e]; bool alongZ = r.height > r.width; float a0 = alongZ ? r.y : r.x, a1 = alongZ ? r.y + r.height : r.x + r.width;
            for (float u = a0 + 3; u < a1 - 3; u += 7) for (int sg = 0; sg < 2; sg++) { float x = alongZ ? (sg ? r.x + r.width + 1.6f : r.x - 1.6f) : u, z = alongZ ? u : (sg ? r.y + r.height + 1.6f : r.y - 1.6f);
                if (!inCore(x, z) || cityLand(x, z) || roadAt(x, z)) continue; float y = terrainH(x, z);
                box(T_WOOD, V3(x - 0.2f, y - 0.2f, z - 0.2f), V3(x + 0.2f, y + 3.8f, z + 0.2f), rgb(80, 66, 54), true, ALL);
                box(T_HEDGE, V3(x - 2.2f, y + 3.2f, z - 2.2f), V3(x + 2.2f, y + 4.9f, z + 2.2f), rgb(110, 138, 52), false, ALLB); } }
    }
    // the ring road along the edge, and the avenue cutting across the grid
    for (int r = 0; r < 4; r++) if (nRing[r] > 1) {
        ribbon(T_ASPHALT, ringP[r], nRing[r], -4.4f, 4.4f, 0.03f, 0.03f, asp, NULL); dashes(ringP[r], nRing[r], 0, 0.045f, 0.15f, 2, 2, rgb(235, 235, 225));
        for (int i = 6; i + 6 < nRing[r]; i += 14) { Vector2 nn = pnorm(ringP[r], nRing[r], i); float x = ringP[r][i].x + nn.x * 5.6f, z = ringP[r][i].z + nn.y * 5.6f;
            if (cityLand(x, z) || !pointFree(x, z, 0.3f, 0.1f)) { x = ringP[r][i].x - nn.x * 5.6f; z = ringP[r][i].z - nn.y * 5.6f; if (cityLand(x, z) || !pointFree(x, z, 0.3f, 0.1f)) continue; }
            box(T_CONCRETE, V3(x - 0.14f, -0.2f, z - 0.14f), V3(x + 0.14f, 8.6f, z + 0.14f), rgb(150, 150, 140), true, ALL);
            if ((i / 14) % 2 == 0) addLight(V3(x, 6.0f, z), V3(1.0f, 0.62f, 0.28f), 12, ri(3)); } }
    if (nAvenue > 1) {
        ribbon(T_ASPHALT, avenueP, nAvenue, -7.0f, 7.0f, 0.04f, 0.04f, asp, NULL);
        dashes(avenueP, nAvenue, -0.25f, 0.055f, 0.12f, 1, 0, rgb(230, 190, 60)); dashes(avenueP, nAvenue, 0.25f, 0.055f, 0.12f, 1, 0, rgb(230, 190, 60));
        dashes(avenueP, nAvenue, -3.5f, 0.055f, 0.12f, 2, 2, rgb(235, 235, 225)); dashes(avenueP, nAvenue, 3.5f, 0.055f, 0.12f, 2, 2, rgb(235, 235, 225));
        for (int i = 5; i + 5 < nAvenue; i += 12) for (int sg = -1; sg <= 1; sg += 2) { Vector2 nn = pnorm(avenueP, nAvenue, i); float x = avenueP[i].x + nn.x * 7.8f * sg, z = avenueP[i].z + nn.y * 7.8f * sg;
            if (!pointFree(x, z, 0.3f, 0.1f)) continue;
            box(T_METAL, V3(x - 0.1f, 0, z - 0.1f), V3(x + 0.1f, 7.5f, z + 0.1f), rgb(110, 115, 120), true, ALL);
            if ((i / 12 + (sg > 0)) % 2 == 0) addLight(V3(x - nn.x * 1.2f * sg, 7.2f, z - nn.y * 1.2f * sg), V3(0.9f, 0.95f, 1.0f), 13, ri(3)); }
    }
    // a planted verge behind the land-side sidewalk: grass on a low curb, clipped hedges, pines; open where streets meet the road
    ribbon(T_GRASS, coastP, nCoast, -9.4f, -6.2f, SIDEH + 0.06f, SIDEH + 0.06f, rgb(150, 190, 120), coastSkipVerge);
    wallRibbon(T_CONCRETE, coastP, nCoast, -6.2f, SIDEH - 0.02f, SIDEH + 0.08f, rgb(170, 170, 162), coastSkipVerge); wallRibbon(T_CONCRETE, coastP, nCoast, -9.4f, SIDEH - 0.02f, SIDEH + 0.08f, rgb(170, 170, 162), coastSkipVerge);
    for (int i = 2; i + 2 < nCoast; i++) { Vector2 nn = pnorm(coastP, nCoast, i); float x = coastP[i].x - nn.x * 7.8f, z = coastP[i].z - nn.y * 7.8f;
        if (coastSkipVerge(V3(x, 0, z))) continue;
        if (i % 10 == 0) { float h = fr(4.5f, 6.5f); box(T_WOOD, V3(x - 0.18f, SIDEH, z - 0.18f), V3(x + 0.18f, SIDEH + h, z + 0.18f), rgb(110, 85, 70), true, ALL);
            box(T_HEDGE, V3(x - 1.9f, SIDEH + h - 0.5f, z - 1.9f), V3(x + 1.9f, SIDEH + h + 0.6f, z + 1.9f), rgb(62, 112, 70), false, ALLB); }
        else if (i % 10 >= 3 && i % 10 <= 7) box(T_HEDGE, V3(x - 0.75f, SIDEH, z - 0.75f), V3(x + 0.75f, SIDEH + 0.9f, z + 0.75f), rgb(70, 125, 72), true, ALL); }
    // the coast road is lined with houses and shops on its land side: a front row facing the road behind the sidewalk,
    // then rows behind it with 2.5 m alleys, until the grid or the edge of town
    for (int row = 0; row < 4; row++) {
        float acc = 99, depth0 = 6.6f + row * 12.5f;
        for (int k = 0; k + 1 < nCoast; k++) {
            acc += Vector3Distance(coastP[k], coastP[k + 1]); if (acc < 12.5f) continue;
            Vector2 nn = pnorm(coastP, nCoast, k); bool alongX = fabsf(nn.x) > fabsf(nn.y);
            float w = fr(9, 11.5f), d = 10, off = depth0 + 3.4f + d / 2;   // behind the green verge
            float cx = coastP[k].x - nn.x * off, cz = coastP[k].z - nn.y * off;
            Rectangle r = {cx - (alongX ? d : w) / 2, cz - (alongX ? w : d) / 2, alongX ? d : w, alongX ? w : d};
            bool ok = true;
            for (int q = 0; q < 9 && ok; q++) { float x = r.x + r.width * (q % 3) / 2, z = r.y + r.height * (q / 3) / 2;
                if (!inCore(x, z) || cityLand(x, z) || roadAt(x, z) || seaSide(x, z) || coast_d(x, z) < 9.6f) ok = false; }
            for (int q = 0; q < 4 && ok; q++) { float x = q & 1 ? r.x - 2.6f : r.x + r.width + 2.6f, z = q & 2 ? r.y - 2.6f : r.y + r.height + 2.6f; if (cityLand(x, z) && !roadAt(x, z)) { float dd = 0; (void)dd; } }
            if (!ok || !boxFree(r.x - 1.2f, r.y - 1.2f, r.x + r.width + 1.2f, r.y + r.height + 1.2f, SIDEH)) continue;
            acc = 0;
            int face = alongX ? (nn.x > 0 ? 1 : 3) : (nn.y > 0 ? 2 : 0);   // toward the road
            nFoot = 0; building(r, row == 0 ? (ri(3) ? 2 : 0) : (ri(3) ? 0 : 1), row == 0 ? 1 + ri(2) : 2 + ri(2), face);
            for (int f = 0; f < nFoot && nMapFoot < MAX_MAPFOOT; f++) { mapFootH[nMapFoot] = footH[f]; mapFootY[nMapFoot] = 0; mapFoot[nMapFoot++] = foots[f]; }
        }
    }
    // infill: every open stretch of town paving left between the coast-road rows and the grid gets old-town lots
    // (small houses, shops and apartments on a 13 m pitch with 2.5-3 m alleys between them)
    for (float z = -60; z < CITY + 120; z += 13) for (float x = -60; x < CITY + 120; x += 13) {
        float w = fr(8.5f, 10.5f), d = fr(8.5f, 10.5f), ox = fr(0, 13 - w - 2.5f), oz = fr(0, 13 - d - 2.5f);
        Rectangle r = {x + ox, z + oz, w, d};
        bool ok = true;
        for (int q = 0; q < 9 && ok; q++) { float px = r.x + r.width * (q % 3) / 2, pz = r.y + r.height * (q / 3) / 2;
            if (!inCore(px, pz) || cityLand(px, pz) || roadAt(px, pz) || seaSide(px, pz) || coast_d(px, pz) < 7.5f || fabsf(terrainH(px, pz) - SIDEH) > 0.3f) ok = false; }
        for (int q = 0; q < 4 && ok; q++) { float px = q & 1 ? r.x - 2.5f : r.x + r.width + 2.5f, pz = q & 2 ? r.y - 2.5f : r.y + r.height + 2.5f;   // keep an alley to the grid's blocks
            if (cityLand(px, pz) && !roadAt(px, pz)) ok = false; }
        if (ok && !boxFree(r.x - 1.3f, r.y - 1.3f, r.x + r.width + 1.3f, r.y + r.height + 1.3f, SIDEH)) ok = false;
        if (!ok) continue;
        // face the nearest road: the coast road or a street of the grid
        float cx = r.x + w / 2, cz = r.y + d / 2, best = 1e9f; int face = 0;
        for (int f = 0; f < 4; f++) { float dx = f == 1 ? 1 : f == 3 ? -1 : 0, dz = f == 2 ? 1 : f == 0 ? -1 : 0;
            for (float t = 4; t < 40; t += 2) if (roadAt(cx + dx * t, cz + dz * t) || (cityLand(cx + dx * t, cz + dz * t) && fmodf(cx + dx * t + 1000, PITCH) < ROADW)) { if (t < best) { best = t; face = f; } break; } }
        float rr = fr(0, 1); int kind = rr < 0.5f ? 0 : rr < 0.8f ? 2 : 1;
        nFoot = 0; building(r, kind, kind == 1 ? 2 + ri(2) : 1 + ri(2), face);
        for (int f = 0; f < nFoot && nMapFoot < MAX_MAPFOOT; f++) { mapFootH[nMapFoot] = footH[f]; mapFootY[nMapFoot] = 0; mapFoot[nMapFoot++] = foots[f]; }
        if (fr(0, 1) < 0.25f) { float lx = r.x + w + 1.2f, lz = r.y + d / 2; if (pointFree(lx, lz, 0.4f, SIDEH + 0.05f)) {   // paper lanterns in the alleys
            box(T_PAPER, V3(lx - 0.25f, SIDEH + 2.6f, lz - 0.25f), V3(lx + 0.25f, SIDEH + 3.2f, lz + 0.25f), rgb(230, 120, 70), false, ALLB);
            if (ri(2)) addLight(V3(lx, SIDEH + 2.8f, lz), V3(1.0f, 0.5f, 0.25f), 7, ri(3) ? 1 : 0); } }
    }
    // ================= the town (concept map 6, mirrored Atami): streets, river, railway, buildings, shrines, castle, parks
    for (int s2 = 0; s2 < nStreet; s2++) {   // streets on top of the paving: asphalt (stairs in concrete), edge lines on the wider ones
        const Vector3 *p = stPts + dStreets[s2][0]; int n = dStreets[s2][1]; float w = dStreets[s2][2] / 10.0f; int k = dStreets[s2][3];
        ribbon(k == 2 ? T_CONCRETE : T_ASPHALT, p, n, -w / 2, w / 2, SIDEH + 0.012f, SIDEH + 0.012f, k == 2 ? rgb(170, 168, 160) : WHITE, NULL);
        if (k == 1) { dashes(p, n, 0, SIDEH + 0.025f, 0.15f, 2, 2, rgb(235, 235, 225)); dashes(p, n, -w / 2 + 0.4f, SIDEH + 0.025f, 0.12f, 1, 0, rgb(235, 235, 225)); dashes(p, n, w / 2 - 0.4f, SIDEH + 0.025f, 0.12f, 1, 0, rgb(235, 235, 225)); }
        if (k == 3) {   // a bridge: deck edges and railings, a beam under the deck
            wallRibbon(T_CONCRETE, p, n, -w / 2, SIDEH, SIDEH + 1.0f, rgb(200, 198, 190), NULL); wallRibbon(T_CONCRETE, p, n, w / 2, SIDEH, SIDEH + 1.0f, rgb(200, 198, 190), NULL);
            railSolids(p, n, -w / 2, SIDEH + 1.0f, 0.12f); railSolids(p, n, w / 2, SIDEH + 1.0f, 0.12f);
            wallRibbon(T_CONCRETE, p, n, -w / 2, RIVER_Y + 0.2f, SIDEH, rgb(150, 150, 145), NULL); wallRibbon(T_CONCRETE, p, n, w / 2, RIVER_Y + 0.2f, SIDEH, rgb(150, 150, 145), NULL); }
        else for (int i = 3; i + 3 < n; i += 8) {   // utility poles on one side, a street lamp on every other one
            Vector2 nn = pnorm(p, n, i); float x = p[i].x + nn.x * (w / 2 + 0.9f), z = p[i].z + nn.y * (w / 2 + 0.9f);
            if (!cityLand(x, z) || !pointFree(x, z, 0.3f, SIDEH + 0.1f)) continue;
            box(T_CONCRETE, V3(x - 0.13f, 0, z - 0.13f), V3(x + 0.13f, 8.2f, z + 0.13f), rgb(150, 150, 140), true, ALL);
            box(T_WOOD, V3(x - 0.9f, 7.4f, z - 0.06f), V3(x + 0.9f, 7.55f, z + 0.06f), rgb(90, 80, 70), false, ALLB);
            if ((i / 8) % 2 == 0) addLight(V3(x - nn.x * 1.0f, 5.6f, z - nn.y * 1.0f), V3(1.0f, 0.68f, 0.36f), 11, ri(4) ? 1 : (ri(2) ? 0 : 2));
            if ((i / 8) % 5 == 2) { float vx = p[i].x - nn.x * (w / 2 + 0.9f), vz = p[i].z - nn.y * (w / 2 + 0.9f); if (cityLand(vx, vz) && pointFree(vx, vz, 0.8f, SIDEH + 0.1f)) vending(vx, vz, fabsf(nn.x) > fabsf(nn.y) ? (nn.x > 0 ? 3 : 1) : (nn.y > 0 ? 0 : 2)); }
        }
    }
    {   // the river: concrete walls down to the water on both banks, a railing along the top
        static Vector3 rv[MAX_PATH]; int nr = loadPath(dRiverLine, DRIVERLINE_N, rv);
        for (int sg = -1; sg <= 1; sg += 2) {
            wallRibbon(T_CONCRETE, rv, nr, 4.0f * sg, RIVER_Y - 1.1f, SIDEH, rgb(150, 150, 142), NULL);
            for (int i = 0; i + 1 < nr; i += 2) { Vector2 nn = pnorm(rv, nr, i); float x = rv[i].x + nn.x * 4.25f * sg, z = rv[i].z + nn.y * 4.25f * sg;
                bool onBr = false; for (int s2 = 0; s2 < nStreet && !onBr; s2++) if (dStreets[s2][3] == 3 && pathDist(stPts + dStreets[s2][0], dStreets[s2][1], x, z, NULL) < dStreets[s2][2] / 20.0f + 0.6f) onBr = true;
                if (!cityLand(x, z) || onBr) continue;   // no railing across a bridge deck
                box(T_METAL, V3(x - 0.04f, SIDEH, z - 0.04f), V3(x + 0.04f, SIDEH + 1.0f, z + 0.04f), rgb(120, 125, 130), false, ALLB);
                Vector3 seg[2] = {rv[i], rv[i + 2 < nr ? i + 2 : nr - 1]}; wallRibbon(T_METAL, seg, 2, 4.25f * sg, SIDEH + 0.9f, SIDEH + 1.0f, rgb(130, 135, 140), NULL); railSolids(seg, 2, 4.25f * sg, SIDEH + 1.0f, 0.06f); } }
    }
    if (nRail > 1) {   // the railway on its shelf above the town: ballast, sleepers, two rails, fences, the station platform
        ribbon(T_CONCRETE, railP, nRail, -2.6f, 2.6f, SIDEH + 0.05f, SIDEH + 0.05f, rgb(120, 116, 108), NULL);
        for (int i = 0; i + 1 < nRail; i++) { Vector2 nn = pnorm(railP, nRail, i); for (float t = 0; t < 1; t += 0.33f) { Vector3 c = Vector3Lerp(railP[i], railP[i + 1], t);
            Vector3 a = V3(c.x - nn.x * 1.25f, 0, c.z - nn.y * 1.25f), b = V3(c.x + nn.x * 1.25f, 0, c.z + nn.y * 1.25f); Vector3 sl[2] = {a, b}; ribbon(T_WOOD, sl, 2, -0.12f, 0.12f, SIDEH + 0.09f, SIDEH + 0.09f, rgb(90, 70, 55), NULL); } }
        for (int sg = -1; sg <= 1; sg += 2) { wallRibbon(T_METAL, railP, nRail, 0.75f * sg, SIDEH + 0.05f, SIDEH + 0.22f, rgb(170, 170, 175), NULL); ribbon(T_METAL, railP, nRail, 0.72f * sg, 0.78f * sg, SIDEH + 0.22f, SIDEH + 0.22f, rgb(200, 200, 205), NULL); }
        float stT = 1e9f; int stI = 0; for (int i = 0; i < nRail; i++) { float d = (railP[i].x - STN_X) * (railP[i].x - STN_X) + (railP[i].z - STN_Z) * (railP[i].z - STN_Z); if (d < stT) { stT = d; stI = i; } }
        int p0 = stI - 14 > 0 ? stI - 14 : 0, p1 = stI + 14 < nRail - 1 ? stI + 14 : nRail - 1;   // the platform: 56 m on the town side
        ribbon(T_TILEWALL, railP + p0, p1 - p0 + 1, 2.0f, 5.0f, 1.0f, 1.0f, rgb(200, 205, 200), NULL); wallRibbon(T_CONCRETE, railP + p0, p1 - p0 + 1, 2.0f, SIDEH, 1.0f, rgb(170, 170, 165), NULL);
        ribbon(T_PLAIN, railP + p0, p1 - p0 + 1, 2.05f, 2.35f, 1.01f, 1.01f, rgb(220, 190, 60), NULL);
        for (int i = p0; i < p1; i += 3) { Vector2 nn = pnorm(railP, nRail, i); float x = railP[i].x + nn.x * 3.5f, z = railP[i].z + nn.y * 3.5f;
            addSolid(V3(x - 1.5f, SIDEH, z - 1.5f), V3(x + 1.5f, 1.0f, z + 1.5f)); }
        for (int sg = -1; sg <= 1; sg += 2) for (int i = 0; i + 1 < nRail; i++) { if (sg > 0 && i >= p0 - 1 && i <= p1) continue;   // fences (open at the platform)
            Vector3 seg[2] = {railP[i], railP[i + 1]}; wallRibbon(T_METAL, seg, 2, 3.4f * sg, SIDEH, SIDEH + 1.6f, rgb(110, 118, 120), NULL); railSolids(seg, 2, 3.4f * sg, SIDEH + 1.6f, 0.06f); }
    }
    for (int l = 0; l < DLOTS_N; l++) {   // every building of the town, as drawn: kind, storeys and the side that faces the street
        Rectangle r = {dLots[l][0], dLots[l][1], dLots[l][2], dLots[l][3]}; int kind = (int)dLots[l][4], floors = (int)dLots[l][5], face = (int)dLots[l][6];
        nFoot = 0; building(r, kind, floors, face);
        for (int f = 0; f < nFoot && nMapFoot < MAX_MAPFOOT; f++) { mapFootH[nMapFoot] = footH[f]; mapFootY[nMapFoot] = 0; mapFoot[nMapFoot++] = foots[f]; }
    }
    for (int k = 0; k < DTOWNSHRINES_N; k++) shrine((Rectangle){dTownShrines[k][0] - 3.5f, dTownShrines[k][1] - 3.5f, 7, 7});
    for (int k = 0; k < DPARKS_N; k++) {   // parks: grass, trees, benches
        float cx = dParks[k][0], cz = dParks[k][1], R = dParks[k][2];
        for (float z = cz - R; z < cz + R; z += 2) for (float x = cx - R; x < cx + R; x += 2) if ((x - cx) * (x - cx) + (z - cz) * (z - cz) < R * R && cityLand(x, z))
            quad(T_GRASS, V3(x, SIDEH + 0.01f, z + 2), V3(x + 2, SIDEH + 0.01f, z + 2), V3(x + 2, SIDEH + 0.01f, z), V3(x, SIDEH + 0.01f, z), V3(0, 1, 0), rgb(150, 190, 120));
        for (int t = 0; t < (int)(R * R / 30); t++) { float a = fr(0, 2 * PI), rr = fr(0, R - 2), x = cx + cosf(a) * rr, z = cz + sinf(a) * rr; if (!pointFree(x, z, 1.5f, SIDEH + 0.1f)) continue;
            if (t % 3 == 0) box(T_WOOD, V3(x, SIDEH + 0.4f, z), V3(x + 1.8f, SIDEH + 0.5f, z + 0.5f), WHITE, true, ALL);
            else { float h = fr(3.5f, 5.5f); box(T_WOOD, V3(x - 0.18f, SIDEH, z - 0.18f), V3(x + 0.18f, SIDEH + h, z + 0.18f), rgb(110, 85, 70), true, ALL); box(T_HEDGE, V3(x - 1.8f, SIDEH + h - 0.4f, z - 1.8f), V3(x + 1.8f, SIDEH + h + 0.8f, z + 1.8f), rgb(70, 125, 75), false, ALLB); } }
    }
    {   // the castle on its hill: stone base, three white storeys under dark stepped roofs
        float x = CASTLE_X, z = CASTLE_Z, y = SIDEH;
        box(T_CONCRETE, V3(x - 7, y, z - 6), V3(x + 7, y + 3.2f, z + 6), rgb(150, 146, 136), true, ALL);
        float sz[3] = {5.5f, 4.2f, 3.0f}, hh[3] = {3.2f, 2.8f, 2.6f}; float yy = y + 3.2f;
        for (int t = 0; t < 3; t++) { box(T_PLASTER, V3(x - sz[t], yy, z - sz[t] * 0.85f), V3(x + sz[t], yy + hh[t], z + sz[t] * 0.85f), rgb(236, 234, 226), true, ALL); yy += hh[t];
            box(T_ROOF, V3(x - sz[t] - 1.0f, yy, z - sz[t] * 0.85f - 1.0f), V3(x + sz[t] + 1.0f, yy + 0.5f, z + sz[t] * 0.85f + 1.0f), rgb(60, 66, 72), true, ALL); yy += 0.5f; }
        box(T_PLAIN, V3(x - 0.3f, yy, z - 0.3f), V3(x + 0.3f, yy + 0.9f, z + 0.3f), rgb(200, 170, 70), false, ALLB);
        addLight(V3(x, y + 4, z + 7), V3(1.0f, 0.75f, 0.45f), 14, 1);
    }
    for (int k = 0, placed = 0; k < 400 && placed < 26; k++) {   // parked cars on straight stretches of street, along the kerb
        int s2 = ri(nStreet); if (dStreets[s2][3] == 2 || dStreets[s2][3] == 3) continue;
        const Vector3 *p = stPts + dStreets[s2][0]; int n = dStreets[s2][1]; if (n < 3) continue; int i = ri(n - 1);
        float dx = p[i + 1].x - p[i].x, dz = p[i + 1].z - p[i].z; bool alongZ = fabsf(dz) > fabsf(dx) * 3; if (!alongZ && fabsf(dx) < fabsf(dz) * 3) continue;
        float w = dStreets[s2][2] / 10.0f; if (w < 6.5f) continue; Vector2 nn = pnorm(p, n, i);
        float x = p[i].x + nn.x * (w / 2 - 1.2f), z = p[i].z + nn.y * (w / 2 - 1.2f); int before = nCar; car(x, z, alongZ); if (nCar > before) placed++; }
    // north approach + tunnels
    ribbon(T_ASPHALT, northP, nNorth, -4.4f, 4.4f, 0.05f, 0.05f, asp, NULL); dashes(northP, nNorth, 0, 0.065f, 0.15f, 2, 2, rgb(235, 235, 225));
    for (int b = 0; b < nBore; b++) buildBore(&bores[b]);
    // the touge: narrow asphalt, guardrails, a yellow centre line
    ribbon(T_ASPHALT, tougeP, nTouge, -3.4f, 3.4f, 0.06f, 0.06f, rgb(235, 235, 235), NULL);
    dashes(tougeP, nTouge, 0, 0.075f, 0.14f, 1, 0, rgb(230, 190, 60));
    dashes(tougeP, nTouge, -3.1f, 0.075f, 0.1f, 1, 0, rgb(235, 235, 225)); dashes(tougeP, nTouge, 3.1f, 0.075f, 0.1f, 1, 0, rgb(235, 235, 225));
    static Vector3 tr[MAX_PATH]; int ntr = 0; for (int i = 3; i < nTouge - 2; i++) tr[ntr++] = tougeP[i];
    wallRibbon(T_METAL, tr, ntr, -3.9f, 0.55f, 0.85f, rgb(220, 220, 215), NULL); wallRibbon(T_METAL, tr, ntr, 3.9f, 0.55f, 0.85f, rgb(220, 220, 215), NULL);
    for (int i = 3; i < nTouge - 2; i += 2) { Vector2 nn = pnorm(tougeP, nTouge, i); for (int sgn = -1; sgn <= 1; sgn += 2) { float x = tougeP[i].x + nn.x * 3.9f * sgn, z = tougeP[i].z + nn.y * 3.9f * sgn;
        if (Vector3Distance(V3(x, 0, z), V3(laneP[0].x, 0, laneP[0].z)) < 5) continue;   // gap for the hamlet lane
        box(T_METAL, V3(x - 0.06f, tougeP[i].y - 0.3f, z - 0.06f), V3(x + 0.06f, tougeP[i].y + 0.9f, z + 0.06f), rgb(200, 200, 195), false, ALLB); } }
    {   // rail collision with a gap at the hamlet lane
        int s0 = 3, cut = -1; for (int i = 3; i < nTouge - 2; i++) if (Vector3Distance(V3(tougeP[i].x, 0, tougeP[i].z), V3(laneP[0].x, 0, laneP[0].z)) < 3) { cut = i; break; }
        int lo = cut > 0 ? cut - s0 - 3 : ntr, hi = cut > 0 ? cut - s0 + 3 : ntr;   // both rails open where the lane leaves
        if (lo > 1) { railSolids(tr, lo, -3.9f, 1.0f, 0.12f); railSolids(tr, lo, 3.9f, 1.0f, 0.12f); }
        if (hi < ntr - 1) { railSolids(tr + hi, ntr - hi, -3.9f, 1.0f, 0.12f); railSolids(tr + hi, ntr - hi, 3.9f, 1.0f, 0.12f); } }
    for (int i = 20; i < nTouge - 10; i += 55) { Vector2 nn = pnorm(tougeP, nTouge, i); float x = tougeP[i].x - nn.x * 5, z = tougeP[i].z - nn.y * 5;   // curve mirrors and dead lamps
        box(T_METAL, V3(x - 0.05f, tougeP[i].y, z - 0.05f), V3(x + 0.05f, tougeP[i].y + 2.6f, z + 0.05f), rgb(200, 80, 40), false, ALL);
        box(T_GLASS, V3(x - 0.35f, tougeP[i].y + 2.3f, z - 0.35f), V3(x + 0.35f, tougeP[i].y + 3.0f, z + 0.35f), rgb(200, 220, 230), false, ALLB); }
    // hamlet: lane, a little square, old houses, a shrine
    ribbon(T_CONCRETE, laneP, nLane, -2.4f, 2.4f, 0.06f, 0.06f, rgb(170, 165, 150), NULL);
    for (int h = 0; h < DHOUSES_N; h++) {   // old wooden houses along the pass and in the hamlet, doors facing the nearest road
        float x = dHouses[h][0], z = dHouses[h][1], w = dHouses[h][2], d = dHouses[h][3];
        Pad *pd = &pads[1 + h]; Vector3 q = tougeP[0]; float best = 1e9f;
        for (int i = 0; i < nTouge; i++) { float dd = (tougeP[i].x - x) * (tougeP[i].x - x) + (tougeP[i].z - z) * (tougeP[i].z - z); if (dd < best) { best = dd; q = tougeP[i]; } }
        for (int i = 0; i < nLane; i++) { float dd = (laneP[i].x - x) * (laneP[i].x - x) + (laneP[i].z - z) * (laneP[i].z - z); if (dd < best) { best = dd; q = laneP[i]; } }
        float dx = q.x - x, dz = q.z - z; int face = fabsf(dx) > fabsf(dz) ? (dx > 0 ? 1 : 3) : (dz > 0 ? 2 : 0);
        if (face == 1 || face == 3) { float t = w; w = d; d = t; }
        oldHouse(x, pd->y, z, w, d, face); }
    for (int k = 0; k < DSHRINES_N; k++) { Pad *pd = &pads[1 + DHOUSES_N + k]; hokora(pd->x, pd->y, pd->z); }
    {   float hy = pads[0].y; torii(HAMLET_X, hy, HAMLET_Z - 4, false, 0.8f, rgb(190, 50, 35)); addLight(V3(HAMLET_X, hy + 3, HAMLET_Z), V3(1.0f, 0.6f, 0.3f), 9, 1); }
    for (int k = 0; k < DTORII_N; k++) { float x = dTorii[k][0], z = dTorii[k][1]; float dx = k + 1 < DTORII_N ? dTorii[k + 1][0] - x : x - dTorii[k - 1][0], dz = k + 1 < DTORII_N ? dTorii[k + 1][1] - z : z - dTorii[k - 1][1];
        torii(x, terrainH(x, z), z, fabsf(dx) > fabsf(dz), 0.75f, rgb(200, 60, 40)); }
    // cedar forest on the mountain
    for (int k = 0; k < 220; k++) {
        float a = fr(0, 2 * PI), r = fr(0, MT_R * 1.15f), x = MT_X + cosf(a) * r, z = MT_Z + sinf(a) * r;
        if (inCore(x, z) || cityDist(x, z) < 10 || touge_d(x, z) < 7 || pathDist(laneP, nLane, x, z, NULL) < 5 || coast_d(x, z) < 10 || seaSide(x, z)) continue;
        bool nearPad = false; for (int p = 0; p < nPad; p++) if (Vector2Distance((Vector2){x, z}, (Vector2){pads[p].x, pads[p].z}) < pads[p].r + 2) nearPad = true;
        bool overBore = false; for (int b = 0; b < nBore; b++) { float aa, ll; boreLocal(&bores[b], x, z, &aa, &ll); if (aa > -4 && aa < boreDepth(&bores[b]) + 2 && fabsf(ll) < bores[b].w / 2 + 16) overBore = true; }
        if (nearPad || overBore) continue;
        cedar(x, terrainH(x, z), z, fr(7, 12));
    }
    // the sea surface
    int N = HF_N;
    for (int j = 0; j + 1 < N; j++) for (int i = 0; i + 1 < N; i++) {
        float mn = fminf(fminf(hf[j * N + i], hf[j * N + i + 1]), fminf(hf[(j + 1) * N + i], hf[(j + 1) * N + i + 1]));
        float x0 = HF_X0 + i * HF_S, z0 = HF_X0 + j * HF_S;
        bool rv = inRiverRaw(x0 + 1, z0 + 1); float wy = rv ? RIVER_Y : SEA_Y;
        if ((!rv && mn > SEA_Y) || cityLand(x0 + 1, z0 + 1) || inBore(x0 + 1, z0 + 1, 0)) continue;
        quad(T_PLAIN, V3(x0, wy, z0 + HF_S), V3(x0 + HF_S, wy, z0 + HF_S), V3(x0 + HF_S, wy, z0), V3(x0, wy, z0), V3(0, 1, 0), rv ? rgb(60, 112, 120) : rgb(46, 98, 128));
    }
}
static void spawnTunnels(void) {
    for (int b = 0; b < nBore; b++) { Bore *br = &bores[b]; int n = br->kind ? 2 : 3;
        for (int k = 0; k < n; k++) { float a = fr(boreDepth(br) + 10, br->len - 8); Vector3 p = boreAt(br, a, fr(-1.5f, 1.5f), br->fy);
            int t = br->kind ? (k == 0 ? 8 : 5) : (ri(3) == 0 ? 2 : ri(2) ? 0 : 5); spawnAt(p, t, false); } }
}

// ---------------------------------------------------------------- plaza blocks (the avenue cuts through them)
static void genPlaza(float x0, float z0, int bi, int bj) {
    (void)bi; (void)bj;
    tileBox(T_SIDEWALK, (Rectangle){x0, z0, BLOCK, BLOCK}, -0.5f, 0, WHITE, 16);
    nFoot = 0;
    // shophouses along the avenue: narrow fronts every ~7 m, set just behind its sidewalk so the row follows the road;
    // a second row behind; whatever is left of the block becomes a pocket square
    for (int row = 0; row < 2; row++) for (int k = 0; k + 1 < nAvenue; k++) {
        Vector2 nn = pnorm(avenueP, nAvenue, k); bool alongX = fabsf(nn.x) > fabsf(nn.y);
        for (int sg = -1; sg <= 1; sg += 2) {
            float w = fr(5.5f, 8.0f), d = row ? fr(7, 9) : fr(8, 10), off = 9.4f + row * 10.6f + d / 2;
            float cx = avenueP[k].x + nn.x * off * sg, cz = avenueP[k].z + nn.y * off * sg;
            if (cx < x0 || cx > x0 + BLOCK || cz < z0 || cz > z0 + BLOCK) continue;   // only this block's share
            Rectangle r = {cx - (alongX ? d : w) / 2, cz - (alongX ? w : d) / 2, alongX ? d : w, alongX ? w : d};
            if (r.x < x0 + 0.4f || r.y < z0 + 0.4f || r.x + r.width > x0 + BLOCK - 0.4f || r.y + r.height > z0 + BLOCK - 0.4f) continue;
            float m = 1e9f; for (int q = 0; q < 9; q++) m = fminf(m, pathDist(avenueP, nAvenue, r.x + r.width * (q % 3) / 2, r.y + r.height * (q / 3) / 2, NULL));
            if (m < 8.4f || !boxFree(r.x - 0.3f, r.y - 0.3f, r.x + r.width + 0.3f, r.y + r.height + 0.3f, SIDEH)) continue;
            int face = alongX ? (nn.x * sg > 0 ? 3 : 1) : (nn.y * sg > 0 ? 0 : 2);   // toward the avenue
            float rr = fr(0, 1); building(r, row == 0 ? (rr < 0.7f ? 2 : 1) : (rr < 0.5f ? 0 : 1), row == 0 ? 1 + ri(3) : 2 + ri(2), face);
        }
    }
    for (int k = 0; k < 16; k++) { float x = x0 + fr(2, BLOCK - 2), z = z0 + fr(2, BLOCK - 2);
        if (pathDist(avenueP, nAvenue, x, z, NULL) < 8.5f || !pointFree(x, z, 1.4f, 0.1f)) continue;
        bool inF = false; for (int f = 0; f < nFoot; f++) if (x > foots[f].x - 1.5f && x < foots[f].x + foots[f].width + 1.5f && z > foots[f].y - 1.5f && z < foots[f].y + foots[f].height + 1.5f) inF = true;
        if (inF) continue;
        if (k % 4 == 0) box(T_WOOD, V3(x, 0.4f, z), V3(x + 1.8f, 0.5f, z + 0.5f), WHITE, true, ALL);
        else if (k % 4 == 1) vending(x, z, ri(4));
        else { box(T_WOOD, V3(x - 0.15f, 0, z - 0.15f), V3(x + 0.15f, 3.2f, z + 0.15f), rgb(100, 80, 60), true, ALL); box(T_HEDGE, V3(x - 1.4f, 2.6f, z - 1.4f), V3(x + 1.4f, 4.4f, z + 1.4f), rgb(70, 120, 80), false, ALLB); }
    }
    for (int f = 0; f < nFoot && nMapFoot < MAX_MAPFOOT; f++) { mapFootH[nMapFoot] = footH[f]; mapFootY[nMapFoot] = 0; mapFoot[nMapFoot++] = foots[f]; }
}

// ---------------------------------------------------------------- old town and fish market blocks
static void genOldTown(float x0, float z0, int bi, int bj) {
    // a cross of narrow alleys; small one- and two-storey houses and shops crowd every quadrant; lanterns, a shrine
    float A = 2.2f, half = (BLOCK - A) / 2;
    int shrineQ = ((bi + bj) % 3 == 0) ? ri(4) : -1;
    for (int q = 0; q < 4; q++) {
        float qx = x0 + (q & 1 ? half + A : 0), qz = z0 + (q & 2 ? half + A : 0);
        if (q == shrineQ) { shrine((Rectangle){qx + 1, qz + 1, half - 2, half - 2}); continue; }
        for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++) {
            float lw = half / 2, lx = qx + a * lw, lz = qz + b * lw;
            Rectangle lot = {lx + 0.5f, lz + 0.5f, lw - 1.0f, lw - 1.0f};
            if (inHole(lot.x + lot.width / 2, lot.y + lot.height / 2) || inHole(lot.x, lot.y) || inHole(lot.x + lot.width, lot.y + lot.height)) continue;
            // face the outer street when the lot touches it, else the alley
            int street = (q & 2) ? 2 : 0; if (!(b == ((q & 2) ? 1 : 0))) street = (q & 1) ? 1 : 3;
            if (fr(0, 1) < 0.12f) { blockWall(V3(lot.x, 0, lot.y), V3(lot.x + lot.width, 0, lot.y), 1.4f); continue; }   // a small yard
            int kind = fr(0, 1) < 0.3f ? 2 : 0;
            building(lot, kind, 1 + (fr(0, 1) < 0.4f ? 1 : 0), street);
        }
    }
    for (int k = 0; k < 4; k++) {   // paper lanterns over the alleys
        float x = x0 + half + A / 2, z = z0 + 4 + k * 8.5f;
        box(T_PAPER, V3(x - 0.25f, SIDEH + 2.6f, z - 0.25f), V3(x + 0.25f, SIDEH + 3.2f, z + 0.25f), rgb(230, 120, 70), false, ALLB);
        if (k % 2 == 0) addLight(V3(x, SIDEH + 2.8f, z), V3(1.0f, 0.5f, 0.25f), 7, ri(3) ? 1 : 0);
    }
    if (fr(0, 1) < 0.6f) vending(x0 + 1.6f, z0 + fr(4, BLOCK - 4), 3);
}
static void genMarket(float x0, float z0) {
    // the fish market: a long open hall with counters, fridges and crates
    building((Rectangle){x0 + 2.5f, z0 + 2.5f, BLOCK - 5, 13}, 2, 1, 0);
    for (int k = 0; k < 5; k++) { float x = x0 + 4 + k * 6.2f;
        addCont(k % 2 ? CK_FRIDGE : CK_COUNTER, V3(x, SIDEH, z0 + 21), 0, 2, true);
        box(T_CLOTH, V3(x - 2.2f, SIDEH + 2.4f, z0 + 18.5f), V3(x + 2.2f, SIDEH + 2.5f, z0 + 23.5f), k % 2 ? rgb(60, 110, 160) : rgb(200, 80, 60), false, ALLB);
        for (int c = 0; c < 2; c++) { box(T_METAL, V3(x - 2.1f + c * 4.1f, SIDEH, z0 + 18.6f), V3(x - 2.0f + c * 4.1f, SIDEH + 2.4f, z0 + 18.7f), rgb(90, 90, 95), true, ALL); } }
    for (int k = 0; k < 8; k++) addCrate(V3(fr(x0 + 3, x0 + BLOCK - 3), SIDEH, fr(z0 + 25, z0 + BLOCK - 2)));
    addLight(V3(x0 + BLOCK / 2, 3.5f, z0 + 21), V3(0.8f, 0.9f, 1.0f), 12, 1);
}
