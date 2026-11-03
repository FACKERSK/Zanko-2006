// zk_layout.h — v0.5 city layout (concept 5b): bay in the south-west, old town on the shore, a two-lane coast road that
// runs along the bay into tunnels at both ends, a north road tunnel, and the east mountain with a hairpin pass (touge),
// a hamlet, shrines and a sealed old brick tunnel at the top. Tunnels are explorable up to a collapse.

// ---------------------------------------------------------------- paths
#define MAX_PATH 1200
static Vector3 coastP[MAX_PATH]; static int nCoast;     // y = road height
static Vector3 tougeP[MAX_PATH]; static int nTouge;
static Vector3 northP[MAX_PATH]; static int nNorth;
static Vector3 laneP[MAX_PATH]; static int nLane;      // hamlet lane
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
static unsigned char *townMask; static bool townReady = false;   // baked per heightfield cell: 1 = town ground (paved, flat)
static float coastSigned(float x, float z);
static bool seaSide(float x, float z);
static bool inTownRaw(float x, float z) {   // the drawn outline, plus the land side of the coast road where it passes the town
    if (inPoly(dCore, DCORE_N, x, z)) return true;
    float cs = coastSigned(x, z); if (cs > -4.5f || cs < -48) return false;
    return polyEdgeDist(dCore, DCORE_N, x, z) < 46 && !seaSide(x, z);
}
static bool inCore(float x, float z) {
    if (!townReady) return inTownRaw(x, z);
    int i = (int)((x - HF_X0) / HF_S + 0.5f), j = (int)((z - HF_X0) / HF_S + 0.5f);
    if (i < 0 || j < 0 || i >= HF_N || j >= HF_N) return false;
    return townMask[j * HF_N + i] != 0;
}
#define MAX_EXT 96
static Rectangle extR[MAX_EXT]; static int nExt;   // street ends carried on to the ring road / avenue / coast road
static float pathDist(const Vector3 *p, int n, float x, float z, float *yOut);
static bool mainRoadAt(float x, float z, float m) {   // on the ring road, the avenue or the coast road (m: extra margin)
    for (int r = 0; r < 4; r++) if (nRing[r] > 1 && pathDist(ringP[r], nRing[r], x, z, NULL) < 4.4f + m) return true;
    if (nAvenue > 1 && pathDist(avenueP, nAvenue, x, z, NULL) < 7.0f + m) return true;
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
        if (!inCore(px, pz) && !mainRoadAt(px, pz, 0)) return;
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
    for (int i = 0; i < NB; i++) for (int j = 0; j < NB; j++) if (blockKindA[i][j] != BK_GONE) {   // no block may sit on the coast road or its sidewalk
        float x0 = ROADW + i * PITCH, z0 = ROADW + j * PITCH, best = 1e9f;
        for (int k = 0; k < nCoast; k++) { float dx = fmaxf(fmaxf(x0 - coastP[k].x, 0), coastP[k].x - (x0 + BLOCK)), dz = fmaxf(fmaxf(z0 - coastP[k].z, 0), coastP[k].z - (z0 + BLOCK)); best = fminf(best, dx * dx + dz * dz); }
        if (sqrtf(best) < 7.0f) blockKindA[i][j] = BK_GONE; }
    addBore(WEST_PX, WEST_PZ, 0, 3, 0, 84);    // west: heads north into the hills, collapses after 84 m
    addBore(EAST_PX, EAST_PZ, 0, 0, 0, 110);   // east: heads east
    {   Vector2 cp[2] = {{NORTH_X, NORTH_Z0}, {NORTH_X, NORTH_PORTAL_Z}}; nNorth = resample(cp, 2, northP, MAX_PATH, 2.0f); for (int i = 0; i < nNorth; i++) northP[i].y = 0; }
    addBore(NORTH_X, NORTH_PORTAL_Z, 0, 3, 0, 80);
    nAvenue = loadPath(dAvenue, DAVENUE_N, avenueP);
    nRing[0] = loadPath(dRing0, DRING0_N, ringP[0]); nRing[1] = loadPath(dRing1, DRING1_N, ringP[1]); nRing[2] = loadPath(dRing2, DRING2_N, ringP[2]); nRing[3] = loadPath(dRing3, DRING3_N, ringP[3]);
    layoutExtensions();
    // the touge: from the avenue's east end up the mountain in hairpins to the old tunnel
    nTouge = loadPath(dTouge, DTOUGE_N, tougeP);
    {   float total = 0; for (int i = 1; i < nTouge; i++) total += Vector3Distance(V3(tougeP[i].x, 0, tougeP[i].z), V3(tougeP[i - 1].x, 0, tougeP[i - 1].z));
        float acc = 0; tougeP[0].y = 0;
        for (int i = 1; i < nTouge; i++) { acc += Vector3Distance(V3(tougeP[i].x, 0, tougeP[i].z), V3(tougeP[i - 1].x, 0, tougeP[i - 1].z)); float t = acc / total; tougeP[i].y = 34.0f * (t * t * 0.35f + t * 0.65f); } }
    addBore(tougeP[nTouge - 1].x, tougeP[nTouge - 1].z, tougeP[nTouge - 1].y, 3, 1, 62);   // the old brick tunnel at the top
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
        float cd = coast_d(x, z), w = 1 - clampf((cd - 8) / 12, 0, 1); w = w * w * (3 - 2 * w); h = Lerp(h, -0.05f, w);
        float nd = pathDist(northP, nNorth, x, z, NULL); w = 1 - clampf((nd - 6) / 12, 0, 1); w = w * w * (3 - 2 * w); h = Lerp(h, -0.05f, w);
        // the bay
        if (seaSide(x, z) && d > 0 && cd > 7) {   // depth grows with distance from the shoreline of the drawn bay
            float ds = polyEdgeDist(dSea, DSEA_N, x, z) + 0.6f;
            float tgt = ds < 7 ? -0.15f - ds * 0.14f : -1.15f - fminf((ds - 7) * 0.1f, 5); float sw = clampf(ds / 4, 0, 1); h = Lerp(h, tgt, sw * sw * (3 - 2 * sw));
        }
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
        if (d <= 0 && !cityLand(x, z)) h = roadAt(x, z) ? -0.02f : SIDEH - 0.01f;   // inside the town's outline: roads at street level, raised walkways elsewhere
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
static bool coastSkipPave(Vector3 q) { return cityLand(q.x, q.z) || seaDepth(q.x, q.z) > 0; }
static void genLayoutWorld(void) {
    // coast road: two lanes, pavement on the land side, seawall on the sea side
    Color asp = rgb(255, 255, 255);
    ribbon(T_ASPHALT, coastP, nCoast, -3.7f, 3.7f, 0.05f, 0.05f, asp, NULL);
    dashes(coastP, nCoast, 0, 0.065f, 0.15f, 2, 2, rgb(235, 235, 225));
    dashes(coastP, nCoast, -3.3f, 0.065f, 0.12f, 1, 0, rgb(235, 235, 225)); dashes(coastP, nCoast, 3.3f, 0.065f, 0.12f, 1, 0, rgb(235, 235, 225));
    ribbon(T_SIDEWALK, coastP, nCoast, -6.2f, -3.7f, 0.2f, 0.2f, WHITE, coastSkipPave); wallRibbon(T_CONCRETE, coastP, nCoast, -3.7f, 0.0f, 0.2f, rgb(170, 170, 165), coastSkipPave);
    ribbon(T_SIDEWALK, coastP, nCoast, 3.7f, 4.6f, 0.2f, 0.2f, WHITE, NULL);
    ribbon(T_CONCRETE, coastP, nCoast, 4.6f, 5.2f, 1.05f, 1.05f, rgb(215, 212, 200), NULL);
    wallRibbon(T_CONCRETE, coastP, nCoast, 4.6f, 0.2f, 1.05f, rgb(200, 198, 188), NULL); wallRibbon(T_CONCRETE, coastP, nCoast, 5.2f, -1.6f, 1.05f, rgb(185, 183, 172), NULL);
    railSolids(coastP, nCoast, 4.9f, 1.05f, 0.25f);
    for (int i = 10; i + 10 < nCoast; i += 1) {   // bus stops, vending machines, poles; breakwater blocks out in the water
        Vector2 nn = pnorm(coastP, nCoast, i); Vector3 p = coastP[i];
        if (i % 38 == 19) { float x = p.x - nn.x * 5.3f, z = p.z - nn.y * 5.3f; if (!cityLand(x, z)) {
            box(T_METAL, V3(x - 0.06f, 0.2f, z - 0.06f), V3(x + 0.06f, 2.5f, z + 0.06f), rgb(80, 80, 85), true, ALL);
            box(T_PLAIN, V3(x - 0.3f, 2.2f, z - 0.3f), V3(x + 0.3f, 2.6f, z + 0.3f), rgb(60, 120, 170), false, ALLB);
            box(T_WOOD, V3(x - 0.9f, 0.6f, z - 0.25f), V3(x + 0.9f, 0.7f, z + 0.25f), rgb(140, 110, 80), true, ALL); } }
        if (i % 38 == 30) { float x = p.x - nn.x * 6.2f, z = p.z - nn.y * 6.2f; if (!cityLand(x, z) && pointFree(x, z, 0.8f, 0.3f)) vending(x, z, fabsf(nn.x) > fabsf(nn.y) ? (nn.x > 0 ? 1 : 3) : (nn.y > 0 ? 2 : 0)); }
        if (i % 12 == 6) { float x = p.x - nn.x * 6.6f, z = p.z - nn.y * 6.6f; if (!cityLand(x, z)) {
            box(T_CONCRETE, V3(x - 0.14f, -0.2f, z - 0.14f), V3(x + 0.14f, 8.6f, z + 0.14f), rgb(150, 150, 140), true, ALL);
            if (i % 24 == 6) addLight(V3(x + nn.x * 1.3f, 6.0f, z + nn.y * 1.3f), V3(1.0f, 0.62f, 0.28f), 12, ri(3)); } }
        if ((i / 30) % 2 == 1 && i % 3 == 0) { float o = fr(14, 20), x = p.x + nn.x * o, z = p.z + nn.y * o; if (seaDepth(x, z) > 0.2f) tetrapod(x + fr(-1, 1), -1.4f, z + fr(-1, 1)); }
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
            if (cityLand(x0 + 1, z0 + 1)) continue;
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
    // the coast road is lined with houses and shops on its land side: a front row facing the road behind the sidewalk,
    // then rows behind it with 2.5 m alleys, until the grid or the edge of town
    for (int row = 0; row < 4; row++) {
        float acc = 99, depth0 = 6.6f + row * 12.5f;
        for (int k = 0; k + 1 < nCoast; k++) {
            acc += Vector3Distance(coastP[k], coastP[k + 1]); if (acc < 12.5f) continue;
            Vector2 nn = pnorm(coastP, nCoast, k); bool alongX = fabsf(nn.x) > fabsf(nn.y);
            float w = fr(9, 11.5f), d = 10, off = depth0 + d / 2;
            float cx = coastP[k].x - nn.x * off, cz = coastP[k].z - nn.y * off;
            Rectangle r = {cx - (alongX ? d : w) / 2, cz - (alongX ? w : d) / 2, alongX ? d : w, alongX ? w : d};
            bool ok = true;
            for (int q = 0; q < 9 && ok; q++) { float x = r.x + r.width * (q % 3) / 2, z = r.y + r.height * (q / 3) / 2;
                if (!inCore(x, z) || cityLand(x, z) || roadAt(x, z) || seaSide(x, z) || coast_d(x, z) < 6.3f) ok = false; }
            for (int q = 0; q < 4 && ok; q++) { float x = q & 1 ? r.x - 2.6f : r.x + r.width + 2.6f, z = q & 2 ? r.y - 2.6f : r.y + r.height + 2.6f; if (cityLand(x, z) && !roadAt(x, z)) { float dd = 0; (void)dd; } }
            if (!ok || !boxFree(r.x - 1.2f, r.y - 1.2f, r.x + r.width + 1.2f, r.y + r.height + 1.2f, SIDEH)) continue;
            acc = 0;
            int face = alongX ? (nn.x > 0 ? 1 : 3) : (nn.y > 0 ? 2 : 0);   // toward the road
            nFoot = 0; building(r, row == 0 ? (ri(3) ? 2 : 0) : (ri(3) ? 0 : 1), row == 0 ? 1 + ri(2) : 2 + ri(2), face);
            for (int f = 0; f < nFoot && nMapFoot < MAX_MAPFOOT; f++) { mapFootH[nMapFoot] = footH[f]; mapFootY[nMapFoot] = 0; mapFoot[nMapFoot++] = foots[f]; }
        }
    }
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
        if (mn > -0.8f || cityLand(x0 + 1, z0 + 1) || inBore(x0 + 1, z0 + 1, 0)) continue;
        quad(T_PLAIN, V3(x0, -0.8f, z0 + HF_S), V3(x0 + HF_S, -0.8f, z0 + HF_S), V3(x0 + HF_S, -0.8f, z0), V3(x0, -0.8f, z0), V3(0, 1, 0), rgb(46, 98, 128));
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
    for (int c = 0; c < 4; c++) {   // a small shop or kiosk in each corner the avenue leaves free
        float w = 11, d = 10, cx = c & 1 ? x0 + BLOCK - w - 1.5f : x0 + 1.5f, cz = c & 2 ? z0 + BLOCK - d - 1.5f : z0 + 1.5f;
        float m = 1e9f; for (int k = 0; k < 9; k++) m = fminf(m, pathDist(avenueP, nAvenue, cx + w * (k % 3) / 2, cz + d * (k / 3) / 2, NULL));
        if (m < 9.5f) continue;
        int street = c & 2 ? 2 : 0; building((Rectangle){cx, cz, w, d}, ri(2) ? 2 : 0, 1 + ri(2), street);
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
