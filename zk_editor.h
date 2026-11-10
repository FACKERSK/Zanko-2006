// zk_editor.h — the Mod Maker (ModMaker.exe: this source built with -DZK_MODMAKER).
// A mod project is a folder Mods/<name>/ that the game loads like any folder mod:
//   mod.ini, Data/map.zmap (the map editor), Textures/*.png and Data/Models/**.obj (overrides).
// Tabs: Map (fly through the town in game graphics, drag the points of roads, coast, sea, river, rail and the spots),
//       Textures, Models, Mod (info, save, export a .zip to share).
typedef struct { const char *label, *arr; int kind; Color col; } EdLayer;   // kind 0 line, 1 closed outline, 2 streets, 3 places (x, z first), 4 bridges (two ends), 5 spots
static const EdLayer edLayers[] = {
    {"Coast road", "dCoast", 0, {120, 200, 255, 255}}, {"Streets", "dStreetPts", 2, {255, 210, 90, 255}}, {"Railway", "dRail", 0, {200, 140, 255, 255}},
    {"Touge road", "dTouge", 0, {255, 150, 90, 255}}, {"Hamlet lane", "dLane", 0, {230, 180, 120, 255}}, {"River course", "dRiverLine", 0, {90, 170, 255, 255}},
    {"River banks", "dRiver", 1, {70, 140, 230, 255}}, {"Sea", "dSea", 1, {40, 110, 220, 255}}, {"Town outline", "dCore", 1, {150, 230, 150, 255}},
    {"Bridges", "dBridges", 4, {240, 240, 240, 255}}, {"Building lots", "dLots", 3, {200, 200, 200, 255}}, {"Hamlet houses", "dHouses", 3, {220, 170, 140, 255}},
    {"Parks", "dParks", 3, {110, 210, 110, 255}}, {"Torii", "dTorii", 3, {230, 80, 60, 255}}, {"Town shrines", "dTownShrines", 3, {230, 120, 90, 255}},
    {"Spots", NULL, 5, {255, 90, 200, 255}},
};
#define ED_NL ((int)(sizeof edLayers / sizeof edLayers[0]))
static struct { const char *name; float *x, *z; } edSpots[] = {
    {"Player spawn", &SPAWN_X_v, &SPAWN_Z_v}, {"Station", &STN_X_v, &STN_Z_v}, {"Castle", &CASTLE_X_v, &CASTLE_Z_v}, {"Hamlet", &HAMLET_X_v, &HAMLET_Z_v},
    {"Old tunnel", &OLDT_X_v, &OLDT_Z_v}, {"Mountain centre", &DMT_X_v, &DMT_Z_v}, {"Coast road west end", &WEST_PX_v, &WEST_PZ_v},
    {"Coast road south end", &SOUTH_PX_v, &SOUTH_PZ_v}, {"Railway east portal", &RAIL_E_X_v, &RAIL_E_Z_v}, {"Railway west portal", &RAIL_W_X_v, &RAIL_W_Z_v},
};
#define ED_NSPOT ((int)(sizeof edSpots / sizeof edSpots[0]))
static char edName[64] = "", edDir[700] = "";
static int edLayer = 1, edSel = -1, edTab = 0; static float edBrush = 8;
static bool edDirty = false, edDragging = false; static char edMsg[160] = ""; static float edMsgT = 0;
static Vector3 edPos = {228, 70, 60}; static float edYaw = 0.0f, edPitch = -0.75f, edSpeed = 30;
static float edDragY; static Vector2 edDragStart; static int edDragN = 0; static int edDragIdx[4096]; static float edDragW[4096]; static Vector2 edDragOrig[4096];
static TextBox edTB[4];   // mod name, author, version, description
static bool edNoPush = false;   // inside a compound change (one undo step)
static void edUndo(bool redo); static void edPush(void);
static void edSay(const char *m) { snprintf(edMsg, sizeof edMsg, "%s", m); edMsgT = 4; }
// ---- the points of a layer
static int edCount(int L) {
    const EdLayer *e = &edLayers[L]; if (e->kind == 5) return ED_NSPOT;
    MapArr *a = mapArr(e->arr); if (!a) return 0; int n = mapRows(a); return e->kind == 4 ? n * 2 : n;
}
static Vector2 edGet(int L, int j) {
    const EdLayer *e = &edLayers[L]; if (e->kind == 5) return (Vector2){*edSpots[j].x, *edSpots[j].z};
    MapArr *a = mapArr(e->arr); if (e->kind == 4) return (Vector2){(float)mapGet(a, j / 2, (j % 2) * 2), (float)mapGet(a, j / 2, (j % 2) * 2 + 1)};
    return (Vector2){(float)mapGet(a, j, 0), (float)mapGet(a, j, 1)};
}
static void edSetP(int L, int j, Vector2 p) {
    const EdLayer *e = &edLayers[L]; if (e->kind == 5) { *edSpots[j].x = p.x; *edSpots[j].z = p.y; return; }
    MapArr *a = mapArr(e->arr);
    if (e->kind == 4) { mapSet(a, j / 2, (j % 2) * 2, p.x); mapSet(a, j / 2, (j % 2) * 2 + 1, p.y); return; }
    if (e->kind == 3) { Vector2 o = edGet(L, j); float dx = p.x - o.x, dz = p.y - o.y;   // a lot's other corner points move with it
        if (!strcmp(e->arr, "dLots")) for (int c = 2; c + 1 < 7; c += 2) { mapSet(a, j, c, mapGet(a, j, c) + dx); mapSet(a, j, c + 1, mapGet(a, j, c + 1) + dz); } }
    mapSet(a, j, 0, p.x); mapSet(a, j, 1, p.y);
}
static int edStreetOf(int j) { for (int s2 = 0; s2 < DSTREETS_N; s2++) if (j >= dStreets[s2][0] && j < dStreets[s2][0] + dStreets[s2][1]) return s2; return -1; }
static bool edSameGroup(int L, int a, int b) { if (edLayers[L].kind == 2) return edStreetOf(a) == edStreetOf(b); if (edLayers[L].kind == 3 || edLayers[L].kind == 4 || edLayers[L].kind == 5) return a == b; return true; }
static float edGroundY(float x, float z) { float g = cityLand(x, z) ? SIDEH : terrainH(x, z); if (g < SEA_Y) g = SEA_Y; return g; }
static void edInsert(int L, int j) {   // a new point after j, halfway to the next
    if (!edNoPush) edPush();
    const EdLayer *e = &edLayers[L]; if (e->kind >= 4 || j < 0) return;
    MapArr *a = mapArr(e->arr); int n = mapRows(a); if (!a->n || n + 1 >= a->cap) { edSay("This layer is full."); return; }
    for (int r = n; r > j + 1; r--) for (int c = 0; c < a->cols; c++) mapSet(a, r, c, mapGet(a, r - 1, c));
    for (int c = 0; c < a->cols; c++) mapSet(a, j + 1, c, mapGet(a, j, c));
    Vector2 p = edGet(L, j), q = p;
    bool hasNext = j + 1 < n && edSameGroup(L, j, j + 2) ; if (e->kind == 2) { int s2 = edStreetOf(j); hasNext = j + 1 < dStreets[s2][0] + dStreets[s2][1]; }
    if (e->kind == 3) q = (Vector2){p.x + 4, p.y + 4}; else if (hasNext) { Vector2 nx = edGet(L, j + 2); q = Vector2Lerp(p, nx, 0.5f); } else q = (Vector2){p.x + 3, p.y};
    *a->n = n + 1; edSetP(L, j + 1, q);
    if (e->kind == 2) { int s2 = edStreetOf(j); dStreets[s2][1]++; for (int t = 0; t < DSTREETS_N; t++) if (t != s2 && dStreets[t][0] > j) dStreets[t][0]++; }
    edSel = j + 1; edDirty = true;
}
static void edDelete(int L, int j) {
    if (!edNoPush) edPush();
    const EdLayer *e = &edLayers[L]; if (e->kind >= 4 || j < 0) return;
    MapArr *a = mapArr(e->arr); int n = mapRows(a);
    if (e->kind == 2) { int s2 = edStreetOf(j); if (dStreets[s2][1] <= 2) { edSay("A street needs two points (delete the street with Shift+Del)."); return; } }
    else if (n <= (e->kind == 1 ? 3 : e->kind == 3 ? 0 : 2)) { edSay("Too few points left."); return; }
    for (int r = j; r + 1 < n; r++) for (int c = 0; c < a->cols; c++) mapSet(a, r, c, mapGet(a, r + 1, c));
    *a->n = n - 1;
    if (e->kind == 2) { int s2 = edStreetOf(j); dStreets[s2][1]--; for (int t = 0; t < DSTREETS_N; t++) if (t != s2 && dStreets[t][0] > j) dStreets[t][0]--; }
    edSel = j < *a->n ? j : *a->n - 1; edDirty = true;
}
static void edDeleteStreet(int j) {
    if (!edNoPush) edPush();
    int s2 = edStreetOf(j); if (s2 < 0) return; int st = dStreets[s2][0], cnt = dStreets[s2][1];
    MapArr *a = mapArr("dStreetPts"); int n = mapRows(a);
    for (int r = st; r + cnt < n; r++) for (int c = 0; c < 2; c++) mapSet(a, r, c, mapGet(a, r + cnt, c));
    *a->n = n - cnt;
    for (int t = s2; t + 1 < DSTREETS_N; t++) for (int c = 0; c < 4; c++) dStreets[t][c] = dStreets[t + 1][c];
    DSTREETS_N_v--; for (int t = 0; t < DSTREETS_N; t++) if (dStreets[t][0] > st) dStreets[t][0] -= cnt;
    edSel = -1; edDirty = true;
}
static void edNewStreet(Vector2 at) {   // a short new street at the cursor, ready to be dragged out
    if (!edNoPush) edPush();
    MapArr *a = mapArr("dStreetPts"); int n = mapRows(a); if (n + 2 >= a->cap || DSTREETS_N + 1 >= 676) { edSay("No room for more streets."); return; }
    mapSet(a, n, 0, at.x); mapSet(a, n, 1, at.y); mapSet(a, n + 1, 0, at.x + 12); mapSet(a, n + 1, 1, at.y); *a->n = n + 2;
    dStreets[DSTREETS_N][0] = n; dStreets[DSTREETS_N][1] = 2; dStreets[DSTREETS_N][2] = 80; dStreets[DSTREETS_N][3] = 0; DSTREETS_N_v++;
    edSel = n + 1; edDirty = true; edSay("New street: drag its end, Insert adds points.");
}
// ---- snapping and joining roads
static bool edSnapOn = true; static bool edSnapHit = false; static Vector2 edSnapAt; static int edSnapStreet = -1, edSnapEnd = -1;   // what the dragged end has caught
static bool edIsEnd(int L, int j) {
    if (edLayers[L].kind == 2) { int s2 = edStreetOf(j); return s2 >= 0 && (j == dStreets[s2][0] || j == dStreets[s2][0] + dStreets[s2][1] - 1); }
    if (edLayers[L].kind == 0) return j == 0 || j == edCount(L) - 1;
    return false;
}
static bool edRoadNearest(int L, int j, Vector2 p, Vector2 *out, int *tStreet, int *tEnd) {   // the closest road line to p (other than the dragged one); ends win when close
    const int roads[4] = {0, 1, 3, 4}; float best = 6.0f, bestEnd = 7.0f; bool got = false, gotEnd = false; int myS = edLayers[L].kind == 2 ? edStreetOf(j) : -1;
    *tStreet = -1; *tEnd = -1;
    for (int r = 0; r < 4; r++) { int RL = roads[r], c = edCount(RL);
        for (int k = 0; k + 1 < c || (k < c && c == 1); k++) {
            if (RL == L && edLayers[L].kind == 0) break;   // a line does not snap onto itself
            if (RL == 1 && edStreetOf(k) == myS && myS >= 0) continue;
            bool endK = edIsEnd(RL, k); Vector2 a = edGet(RL, k);
            if (endK && Vector2Distance(a, p) < bestEnd) { bestEnd = Vector2Distance(a, p); *out = a; gotEnd = true; *tStreet = RL == 1 ? edStreetOf(k) : -1; *tEnd = RL == 1 ? k : -1; }
            if (gotEnd || k + 1 >= c || !edSameGroup(RL, k, k + 1)) continue;
            Vector2 b = edGet(RL, k + 1), d = Vector2Subtract(b, a); float L2 = Vector2DotProduct(d, d); if (L2 < 1e-4f) continue;
            float t = clampf(Vector2DotProduct(Vector2Subtract(p, a), d) / L2, 0, 1); Vector2 q = Vector2Add(a, Vector2Scale(d, t)); float dist = Vector2Distance(q, p);
            if (dist < best) { best = dist; if (!gotEnd) { *out = q; got = true; } } } }
    return got || gotEnd;
}
static void edStreetPts(int s2, Vector2 *buf, int *n) { *n = dStreets[s2][1]; for (int k = 0; k < *n; k++) buf[k] = edGet(1, dStreets[s2][0] + k); }
static void edRemoveStreetRaw(int s2) {   // the points and the record, no undo step
    int st = dStreets[s2][0], cnt = dStreets[s2][1]; MapArr *a = mapArr("dStreetPts"); int n = mapRows(a);
    for (int r = st; r + cnt < n; r++) for (int c = 0; c < 2; c++) mapSet(a, r, c, mapGet(a, r + cnt, c));
    *a->n = n - cnt; for (int t = s2; t + 1 < DSTREETS_N; t++) for (int c = 0; c < 4; c++) dStreets[t][c] = dStreets[t + 1][c];
    DSTREETS_N_v--; for (int t = 0; t < DSTREETS_N; t++) if (dStreets[t][0] > st) dStreets[t][0] -= cnt;
}
static int edAddStreetRaw(const Vector2 *pts, int n, int w, int kind) {
    MapArr *a = mapArr("dStreetPts"); int base = mapRows(a); if (base + n >= a->cap || DSTREETS_N + 1 >= 676) return -1;
    for (int k = 0; k < n; k++) { mapSet(a, base + k, 0, pts[k].x); mapSet(a, base + k, 1, pts[k].y); } *a->n = base + n;
    dStreets[DSTREETS_N][0] = base; dStreets[DSTREETS_N][1] = n; dStreets[DSTREETS_N][2] = w; dStreets[DSTREETS_N][3] = kind; DSTREETS_N_v++; return base;
}
static void edJoinStreets(int j, int other, int otherEnd) {   // street with end j and street "other" (end otherEnd) become one street
    int sa = edStreetOf(j); if (sa < 0 || other < 0 || sa == other) return;
    static Vector2 A[4096], B[4096], C[8192]; int na, nb; edStreetPts(sa, A, &na); edStreetPts(other, B, &nb);
    if (na + nb > 4000) { edSay("Too long to join."); return; }
    bool aEndsHere = (j == dStreets[sa][0] + na - 1), bStartsHere = (otherEnd == dStreets[other][0]);
    int n = 0; for (int k = 0; k < na; k++) C[n++] = aEndsHere ? A[k] : A[na - 1 - k];
    for (int k = 1; k < nb; k++) C[n++] = bStartsHere ? B[k] : B[nb - 1 - k];
    int w = dStreets[sa][2], kind = dStreets[sa][3];
    if (sa > other) { edRemoveStreetRaw(sa); edRemoveStreetRaw(other); } else { edRemoveStreetRaw(other); edRemoveStreetRaw(sa); }
    int base = edAddStreetRaw(C, n, w, kind); edSel = base >= 0 ? base + (aEndsHere ? na - 1 : na - 1) : -1; edDirty = true;
    edSay("Streets joined into one.");
}
static void edSplitStreet(int j) {   // one street becomes two at point j
    int s2 = edStreetOf(j); if (s2 < 0 || edIsEnd(1, j)) { edSay("Pick a point in the middle of a street."); return; } edPush();
    static Vector2 P[4096]; int n; edStreetPts(s2, P, &n); int at = j - dStreets[s2][0], w = dStreets[s2][2], kind = dStreets[s2][3];
    edRemoveStreetRaw(s2); edAddStreetRaw(P, at + 1, w, kind); int b = edAddStreetRaw(P + at, n - at, w, kind); edSel = b; edDirty = true; edSay("Street split in two.");
}
// ---- files
static void edPath(char *out, int n, const char *rel) { snprintf(out, n, "%s/%s", edDir, rel); }
static void edEnsureDirs(const char *rel) {   // make every folder on the way to rel
    char b[800]; edPath(b, sizeof b, rel);
    for (char *p = b + strlen(edDir) + 1; *p; p++) if (*p == '/') { *p = 0; MakeDirectory(b); *p = '/'; }
}
static void edLoadInfo(void) {
    char p[800]; edPath(p, sizeof p, "mod.ini"); for (int i = 0; i < 4; i++) { edTB[i].len = 0; edTB[i].buf[0] = 0; edTB[i].max = i == 3 ? 200 : 40; }
    snprintf(edTB[0].buf, 256, "%s", edName); snprintf(edTB[2].buf, 256, "1.0");
    char *t = LoadFileText(p); if (t) { char *line = strtok(t, "\n");
        while (line) { const char *k[4] = {"name=", "author=", "version=", "description="}; for (int i = 0; i < 4; i++) if (!strncmp(line, k[i], strlen(k[i]))) { snprintf(edTB[i].buf, 256, "%s", line + strlen(k[i])); char *cr = strchr(edTB[i].buf, '\r'); if (cr) *cr = 0; }
            line = strtok(NULL, "\n"); } UnloadFileText(t); }
    for (int i = 0; i < 4; i++) edTB[i].len = (int)strlen(edTB[i].buf);
}
static void edSaveInfo(void) {
    MakeDirectory(edDir); char p[800]; edPath(p, sizeof p, "mod.ini"); FILE *f = fopen(p, "w"); if (!f) return;
    fprintf(f, "name=%s\nauthor=%s\nversion=%s\ndescription=%s\n", edTB[0].buf, edTB[1].buf, edTB[2].buf, edTB[3].buf); fclose(f);
}
static bool edSaveMap(void) { edEnsureDirs("Data/map.zmap"); char p[800]; edPath(p, sizeof p, "Data/map.zmap"); edSaveInfo(); bool ok = mapSave(p); edPath(p, sizeof p, "Data/objects.zobj"); ok &= mobjSave(p); edPath(p, sizeof p, "Data/items.ini"); ok &= itemsSave(p); edPath(p, sizeof p, "Data/terrain.zter"); ok &= terrSave(p); edDirty = false; return ok; }
static void edRebuild(void) { mapGen++; double t0 = GetTime(); genWorld(12345); char b[96]; snprintf(b, sizeof b, "World rebuilt in %.1f s.", GetTime() - t0); edSay(b); }
// ---- a plain .zip writer (stored, no compression) for sharing a mod
static unsigned int edCrc(const unsigned char *d, size_t n) { unsigned int c = 0xFFFFFFFFu; for (size_t i = 0; i < n; i++) { c ^= d[i]; for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1))); } return ~c; }
static void edW16(FILE *f, unsigned v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); }
static void edW32(FILE *f, unsigned v) { edW16(f, v & 0xFFFF); edW16(f, v >> 16); }
static int edExportZip(const char *zipPath) {
    FilePathList fl = LoadDirectoryFilesEx(edDir, NULL, true); FILE *f = fopen(zipPath, "wb"); if (!f) { UnloadDirectoryFiles(fl); return -1; }
    static unsigned off[4096], crc[4096], sz[4096]; static char nm[4096][260]; int n = 0; size_t base = strlen(edDir) + 1;
    for (unsigned i = 0; i < fl.count && n < 4096; i++) { if (!IsPathFile(fl.paths[i])) continue; int len = 0; unsigned char *d = LoadFileData(fl.paths[i], &len); if (!d) continue;
        snprintf(nm[n], 260, "%s/%s", edName, fl.paths[i] + base); for (char *c = nm[n]; *c; c++) if (*c == '\\') *c = '/';
        off[n] = (unsigned)ftell(f); crc[n] = edCrc(d, len); sz[n] = len; unsigned L = (unsigned)strlen(nm[n]);
        edW32(f, 0x04034b50); edW16(f, 20); edW16(f, 0); edW16(f, 0); edW16(f, 0); edW16(f, 0x21); edW32(f, crc[n]); edW32(f, sz[n]); edW32(f, sz[n]); edW16(f, L); edW16(f, 0);
        fwrite(nm[n], 1, L, f); fwrite(d, 1, len, f); UnloadFileData(d); n++; }
    unsigned cd = (unsigned)ftell(f);
    for (int i = 0; i < n; i++) { unsigned L = (unsigned)strlen(nm[i]);
        edW32(f, 0x02014b50); edW16(f, 20); edW16(f, 20); edW16(f, 0); edW16(f, 0); edW16(f, 0); edW16(f, 0x21); edW32(f, crc[i]); edW32(f, sz[i]); edW32(f, sz[i]);
        edW16(f, L); edW16(f, 0); edW16(f, 0); edW16(f, 0); edW16(f, 0); edW32(f, 0); edW32(f, off[i]); fwrite(nm[i], 1, L, f); }
    unsigned cdl = (unsigned)ftell(f) - cd;
    edW32(f, 0x06054b50); edW16(f, 0); edW16(f, 0); edW16(f, n); edW16(f, n); edW32(f, cdl); edW32(f, cd); edW16(f, 0);
    fclose(f); UnloadDirectoryFiles(fl); return n;
}
// ---- textures and models
static int edTexSel = 0, edModelSel = 0, edScrollT = 0, edScrollM = 0; static int edPrevFor = -1;
static FilePathList edModels = {0}; static bool edModelsOk = false;
static void edReloadTexture(int i) {
    char rel[128], mp[800]; snprintf(rel, sizeof rel, "Textures/%s", texNames[i]); edPath(mp, sizeof mp, rel);
    Texture2D t = FileExists(mp) ? LoadTexture(mp) : LoadTexture(P(rel)); if (!t.id) return;
    SetTextureFilter(t, TEXTURE_FILTER_POINT); SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
    UnloadTexture(tex[i]); tex[i] = t; cube[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = t; statSetTex(i, t);
    edPrevFor = -1;
}
static void edDropped(void) {   // files dragged onto the window: a .png replaces the selected texture, an .obj the selected model
    FilePathList d = LoadDroppedFiles();
    for (unsigned i = 0; i < d.count; i++) {
        const char *f = d.paths[i];
        if (IsFileExtension(f, ".png") && edTab == 1) { char rel[128], dst[800]; snprintf(rel, sizeof rel, "Textures/%s", texNames[edTexSel]); edEnsureDirs(rel); edPath(dst, sizeof dst, rel);
            Image im = LoadImage(f); if (im.data) { ExportImage(im, dst); UnloadImage(im); edReloadTexture(edTexSel); char b[160]; snprintf(b, sizeof b, "%s replaced.", texNames[edTexSel]); edSay(b); } }
        else if (IsFileExtension(f, ".obj") && edTab == 2 && edModelsOk && edModelSel < (int)edModels.count) {
            const char *src = edModels.paths[edModelSel]; const char *rel = strstr(src, "Data/Models/"); if (!rel) continue;
            edEnsureDirs(rel); char dst[800]; edPath(dst, sizeof dst, rel); int len = 0; unsigned char *b = LoadFileData(f, &len); if (b) { SaveFileData(dst, b, len); UnloadFileData(b); }
            char mtl[700]; snprintf(mtl, sizeof mtl, "%s", f); char *dot = strrchr(mtl, '.'); if (dot) { strcpy(dot, ".mtl"); if (FileExists(mtl)) { char dm[800]; snprintf(dm, sizeof dm, "%s", dst); char *d2 = strrchr(dm, '.'); strcpy(d2, ".mtl"); b = LoadFileData(mtl, &len); if (b) { SaveFileData(dm, b, len); UnloadFileData(b); } } }
            edSay("Model replaced: the game uses it next start."); }
        else edSay(edTab == 1 ? "Drop a .png here." : edTab == 2 ? "Drop an .obj here." : "Open the Textures or Models tab to drop files.");
    }
    UnloadDroppedFiles(d);
}
// ---- the Build tab: parts placed in 3D, moved, turned and sized with handles (like a block-building studio)
static int boSel = -1, boTool = 0, boAxis = -1; static bool boSnap = true, boAuto = true; static MObj boOrig; static Vector2 boMouse0; static Vector3 boOff;
static const Color boCols[8] = {{235, 235, 230, 255}, {150, 150, 145, 255}, {70, 72, 76, 255}, {190, 60, 50, 255}, {220, 190, 90, 255}, {80, 150, 90, 255}, {80, 120, 190, 255}, {150, 110, 80, 255}};
static float edStepMove = 0.5f, edStepSize = 0.25f, edStepAng = 15, edStepMap = 1.0f;   // editable snap and nudge steps
static float boSnapV(float v, float g) { return boSnap && g > 0 ? roundf(v / g) * g : v; }
static Vector3 boAxisW(MObj *o, int a, bool local) {   // world direction of an axis (move: world axes; size: the part's own)
    if (a == 1) return V3(0, 1, 0); float y = local ? o->yaw * DEG2RAD : 0;
    return a == 0 ? V3(cosf(y), 0, sinf(y)) : V3(-sinf(y), 0, cosf(y));
}
static Vector3 boShowSize(MObj *o) { return o->type == MO_BLOCK ? o->s : (o->type == MO_TREE || o->type == MO_CHERRY || o->type == MO_PINE || o->type == MO_CEDAR) ? V3(o->s.y * 0.5f, o->s.y, o->s.y * 0.5f) : o->s; }
static Matrix boM(MObj *o) { Vector3 z = boShowSize(o); return MatrixMultiply(MatrixMultiply(MatrixScale(z.x, z.y, z.z), moRot(o)), MatrixTranslate(o->p.x, o->p.y + z.y / 2, o->p.z)); }
static float boPick(Ray ray, MObj *o) {   // distance along the ray to the part, or -1 (in the part's own turned frame)
    Vector3 z = boShowSize(o), c = V3(o->p.x, o->p.y + z.y / 2, o->p.z); Matrix Ri = MatrixTranspose(moRot(o));
    Ray L = {Vector3Transform(Vector3Subtract(ray.position, c), Ri), Vector3Transform(ray.direction, Ri)};
    RayCollision h = GetRayCollisionBox(L, (BoundingBox){Vector3Scale(z, -0.5f), Vector3Scale(z, 0.5f)}); return h.hit ? h.distance : -1;
}

static void boDraw(void) {   // the parts, drawn live while they are edited
    for (int i = 0; i < nMobj; i++) { MObj *o = &mobj[i];
        if (o->type == MO_BUILDING) continue;
        if (o->type == MO_BLOCK) drawM(&cube[o->tex], boM(o), o->col);
        else { Color pc = o->type == MO_VENDING ? (Color){90, 140, 200, 255} : o->type == MO_CAR ? (Color){170, 60, 50, 255} : o->type == MO_LAMP ? (Color){160, 160, 150, 255} : (Color){70, 130, 70, 255}; drawM(&cube[T_PLAIN], boM(o), pc); } }
}
static void boHandles(MObj *o, Vector3 *O, Vector3 *E, float *len) {
    Vector3 z = boShowSize(o); *O = V3(o->p.x, o->p.y + z.y / 2, o->p.z);
    for (int a = 0; a < 3; a++) { float half = (a == 0 ? z.x : a == 1 ? z.y : z.z) / 2; len[a] = half + 1.6f; E[a] = Vector3Add(*O, Vector3Scale(boAxisW(o, a, boTool == 2), len[a])); }
}
static float boSegDist(Vector2 p, Vector2 a, Vector2 b) { Vector2 d = Vector2Subtract(b, a); float L2 = Vector2DotProduct(d, d); float t = L2 > 0 ? clampf(Vector2DotProduct(Vector2Subtract(p, a), d) / L2, 0, 1) : 0; return Vector2Distance(p, Vector2Add(a, Vector2Scale(d, t))); }
static void boInsert(int type) {
    if (nMobj >= MAX_MOBJ) { edSay("Too many parts."); return; } edPush();
    Vector3 f = Vector3Subtract(cam.target, cam.position), q = cam.position;   // where the view meets the ground
    for (int k = 0; k < 400; k++) { q = Vector3Add(q, Vector3Scale(f, 1.0f)); if (q.y < edGroundY(q.x, q.z)) break; }
    MObj o = {type, V3(boSnapV(q.x, edStepMove), edGroundY(q.x, q.z), boSnapV(q.z, edStepMove)), moDefaultSize(type), 0, T_CONCRETE, boCols[0], 1};
    mobj[nMobj] = o; boSel = nMobj++; edDirty = true;
}
// ---- undo / redo (Ctrl+Z, Ctrl+Y): whole snapshots of the map tables and the parts
#define ED_UNDO 40
typedef struct { void *arr[64]; int n[64]; double val[64]; MObj *obj; int nObj; float *ter; } EdSnap;
static EdSnap edU[ED_UNDO]; static int edUN = 0, edUPos = 0;   // snapshots [0, edUN); edUPos = how many are "done"
static void edSnapFree(EdSnap *u) { for (int i = 0; i < 64; i++) { free(u->arr[i]); u->arr[i] = NULL; } free(u->obj); u->obj = NULL; free(u->ter); u->ter = NULL; }
static void edSnapTake(EdSnap *u) {
    for (int i = 0; i < MAP_NARR && i < 64; i++) { MapArr *a = &mapArrs[i]; size_t sz = (size_t)mapRows(a) * a->cols * mapElem(a->type); u->arr[i] = malloc(sz ? sz : 1); memcpy(u->arr[i], a->p, sz); u->n[i] = mapRows(a); }
    for (int i = 0; i < MAP_NVAL && i < 64; i++) u->val[i] = mapVals[i].isInt ? *(int *)mapVals[i].p : *(float *)mapVals[i].p;
    u->obj = malloc(sizeof(MObj) * (nMobj ? nMobj : 1)); memcpy(u->obj, mobj, sizeof(MObj) * nMobj); u->nObj = nMobj;
    u->ter = NULL; if (terrEdit) { u->ter = malloc((size_t)HF_N * HF_N * sizeof(float)); memcpy(u->ter, terrEdit, (size_t)HF_N * HF_N * sizeof(float)); }
}
static void edSnapPut(EdSnap *u) {
    for (int i = 0; i < MAP_NARR && i < 64; i++) { MapArr *a = &mapArrs[i]; memcpy(a->p, u->arr[i], (size_t)u->n[i] * a->cols * mapElem(a->type)); if (a->n) *a->n = u->n[i]; }
    for (int i = 0; i < MAP_NVAL && i < 64; i++) { if (mapVals[i].isInt) *(int *)mapVals[i].p = (int)u->val[i]; else *(float *)mapVals[i].p = (float)u->val[i]; }
    memcpy(mobj, u->obj, sizeof(MObj) * u->nObj); nMobj = u->nObj;
    if (u->ter) { if (!terrEdit) terrEdit = calloc((size_t)HF_N * HF_N, sizeof(float)); memcpy(terrEdit, u->ter, (size_t)HF_N * HF_N * sizeof(float)); } else if (terrEdit) memset(terrEdit, 0, (size_t)HF_N * HF_N * sizeof(float));
}
static void edPush(void) {   // call before a change
    for (int i = edUPos; i < edUN; i++) edSnapFree(&edU[i]); edUN = edUPos;
    if (edUN == ED_UNDO) { edSnapFree(&edU[0]); memmove(edU, edU + 1, sizeof(EdSnap) * (ED_UNDO - 1)); memset(&edU[ED_UNDO - 1], 0, sizeof(EdSnap)); edUN--; }
    edSnapTake(&edU[edUN]); edUN++; edUPos = edUN;
}
static void edRebuild(void);
static void edUndo(bool redo) {
    if (!redo) { if (edUPos <= 0) { edSay("Nothing to undo."); return; }
        if (edUPos == edUN) { edSnapTake(&edU[edUN]); edUN++; }   // keep the present so it can be redone
        edUPos--; edSnapPut(&edU[edUPos]); edSay("Undone."); }
    else { if (edUPos + 1 >= edUN) { edSay("Nothing to redo."); return; } edUPos++; edSnapPut(&edU[edUPos]); edSay("Redone."); }
    edSel = -1; boSel = -1; edDirty = true; if (edTab == 0 || edTab == 6 || edTab == 7) edRebuild();
}
// ---- the model viewer: turn it with the left mouse, zoom with the wheel, the game's own model or the mod's replacement
static Model edMV = {0}; static bool edMVOk = false; static int edMVFor = -1, edMVTris = 0, edMVVerts = 0; static bool edMVMod = false, edMVWire = false, edMVShowOrig = false;
static float edMVYaw = 0.7f, edMVPitch = 0.35f, edMVDist = 0; static BoundingBox edMVBox; static char edMVFilter[64] = ""; static TextBox edMVSearch = {0};
static const char *edModelRel(int i) { const char *rel = strstr(edModels.paths[i], "Data/Models/"); if (!rel) rel = strstr(edModels.paths[i], "Data\\Models\\"); return rel ? rel + 12 : GetFileName(edModels.paths[i]); }
static void edMVLoad(void) {
    if (edMVOk) { UnloadModel(edMV); edMVOk = false; } edMVFor = edModelSel; if (!edModelsOk || edModelSel < 0 || edModelSel >= (int)edModels.count) return;
    char full[300], mp[800]; snprintf(full, sizeof full, "Data/Models/%s", edModelRel(edModelSel)); edPath(mp, sizeof mp, full);
    edMVMod = FileExists(mp) && !edMVShowOrig; edMV = LoadModel(edMVMod ? mp : edModels.paths[edModelSel]);
    if (!edMV.meshCount) return; edMVOk = true; edMVTris = edMVVerts = 0;
    for (int k = 0; k < edMV.meshCount; k++) { edMVTris += edMV.meshes[k].triangleCount; edMVVerts += edMV.meshes[k].vertexCount; }
    edMVBox = GetModelBoundingBox(edMV); Vector3 sz = Vector3Subtract(edMVBox.max, edMVBox.min); edMVDist = fmaxf(0.3f, Vector3Length(sz) * 1.6f);
}
static void edModelView(Rectangle vr) {   // draws into the area right of the panel
    if (edMVFor != edModelSel) edMVLoad();
    Vector2 m = GetMousePosition(); bool in = CheckCollisionPointRec(m, vr);
    if (in && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { Vector2 d = GetMouseDelta(); edMVYaw -= d.x * 0.01f; edMVPitch = clampf(edMVPitch + d.y * 0.01f, -1.4f, 1.4f); }
    if (in) edMVDist = clampf(edMVDist * (1 - GetMouseWheelMove() * 0.12f), 0.05f, 400);
    DrawRectangleRec(vr, (Color){46, 54, 58, 255});
    if (!edMVOk) { aTextC("This model could not be loaded.", vr.x + vr.width / 2, vr.y + vr.height / 2, 18, WHITE, true); return; }
    Vector3 c = Vector3Scale(Vector3Add(edMVBox.min, edMVBox.max), 0.5f);
    Camera3D vc = {Vector3Add(c, V3(sinf(edMVYaw) * cosf(edMVPitch) * edMVDist, sinf(edMVPitch) * edMVDist, cosf(edMVYaw) * cosf(edMVPitch) * edMVDist)), c, {0, 1, 0}, 40, CAMERA_PERSPECTIVE};
    BeginScissorMode((int)vr.x, (int)vr.y, (int)vr.width, (int)vr.height);
    rlViewport((int)vr.x, (int)(GetScreenHeight() - vr.y - vr.height), (int)vr.width, (int)vr.height);
    rlMatrixMode(RL_PROJECTION); rlPushMatrix(); rlLoadIdentity(); double asp = vr.width / vr.height, top = 0.01 * tan(40 * 0.5 * DEG2RAD); rlFrustum(-top * asp, top * asp, -top, top, 0.01, 2000);
    rlMatrixMode(RL_MODELVIEW); rlLoadIdentity(); Matrix mv = MatrixLookAt(vc.position, vc.target, vc.up); rlMultMatrixf(MatrixToFloat(mv)); rlEnableDepthTest();
    float g = powf(10, floorf(log10f(fmaxf(edMVDist, 0.1f)))) * 0.1f; DrawGrid(20, g); 
    if (edMVWire) DrawModelWires(edMV, V3(0, 0, 0), 1, (Color){111, 214, 208, 255}); else DrawModel(edMV, V3(0, 0, 0), 1, WHITE);
    DrawBoundingBox(edMVBox, (Color){255, 255, 255, 60});
    rlDrawRenderBatchActive(); rlMatrixMode(RL_PROJECTION); rlPopMatrix(); rlMatrixMode(RL_MODELVIEW); rlLoadIdentity(); rlViewport(0, 0, GetScreenWidth(), GetScreenHeight()); rlDisableDepthTest();
    EndScissorMode();
    float s = aU; Vector3 sz = Vector3Subtract(edMVBox.max, edMVBox.min); char b[200];
    snprintf(b, sizeof b, "%s   %s", edModelRel(edModelSel), edMVMod ? "(this mod's replacement)" : "(the game's own)"); aText(b, vr.x + 14 * s, vr.y + 12 * s, 16, WHITE, true);
    snprintf(b, sizeof b, "%.2f x %.2f x %.2f m    %d triangles    %d vertices    %d materials", sz.x, sz.y, sz.z, edMVTris, edMVVerts, edMV.materialCount); aText(b, vr.x + 14 * s, vr.y + 36 * s, 13, C_BONE, false);
    aText("Drag to turn, wheel to zoom. Drop an .obj (and its .mtl) onto the window to replace it.", vr.x + 14 * s, vr.y + vr.height - 26 * s, 13, C_BONE2, false);
}
// ---- items: the stats the game reads from Data/items.ini
static int edItemSel = 0, edScrollI = 0;
static bool edStep(float x, float y, float w, const char *label, float *v, float step, float lo, float hi, const char *fmt) {   // label  [-] value [+]
    float s = aU; bool ch = false; aText(label, x, y + 4 * s, 13, C_BONE, false);
    if (aButton((Rectangle){x + w - 120 * s, y, 26 * s, 24 * s}, "-", *v > lo)) { *v = fmaxf(lo, *v - step); ch = true; }
    char b[32]; snprintf(b, sizeof b, fmt, *v); aTextC(b, x + w - 62 * s, y + 4 * s, 13, WHITE, true);
    if (aButton((Rectangle){x + w - 26 * s, y, 26 * s, 24 * s}, "+", *v < hi)) { *v = fminf(hi, *v + step); ch = true; }
    return ch;
}
// ---- the terrain brush
static int edTerTool = 0; static float edTerR = 10, edTerS = 4, edTerFlatY = 0, edTerT = 0; static bool edTerHit = false, edTerPainted = false; static Vector3 edTerAt;
static const char *edTerNames[5] = {"Raise", "Lower", "Flatten", "Smooth", "Erase"};
static void edTerPaint(float dt) {
    int N = HF_N, ci = (int)((edTerAt.x - HF_X0) / HF_S), cj = (int)((edTerAt.z - HF_X0) / HF_S), rr = (int)(edTerR / HF_S) + 1;
    for (int j = cj - rr; j <= cj + rr; j++) for (int i = ci - rr; i <= ci + rr; i++) { if (i < 1 || j < 1 || i >= N - 1 || j >= N - 1) continue;
        float x = HF_X0 + i * HF_S, z = HF_X0 + j * HF_S, d = sqrtf((x - edTerAt.x) * (x - edTerAt.x) + (z - edTerAt.z) * (z - edTerAt.z)); if (d > edTerR) continue;
        if (inCore(x, z)) continue;   // the town stays flat paving
        float w = 1 - d / edTerR; w = w * w * (3 - 2 * w); int k = j * N + i; float before = hf[k], ch = 0;
        if (edTerTool == 0) ch = edTerS * w * dt;
        else if (edTerTool == 1) ch = -edTerS * w * dt;
        else if (edTerTool == 2) ch = (edTerFlatY - before) * fminf(1, w * edTerS * dt);
        else if (edTerTool == 3) { float avg = (hf[k - 1] + hf[k + 1] + hf[k - N] + hf[k + N]) * 0.25f; ch = (avg - before) * fminf(1, w * edTerS * 0.5f * dt); }
        else { ch = -terrEdit[k] * fminf(1, w * edTerS * 0.5f * dt); }
        terrEdit[k] += ch; hf[k] += ch; }
    edTerPainted = true; edDirty = true;
}
// ---- buildings
static int edBSel = 0, edBScroll = 0, edBCat = 0; static TextBox edBSearch = {0};
static const char *edBCats[6] = {"All", "Houses", "Flats", "Shops, food", "Public", "Work"};
static int edBCatOf(int i) { const BPreset *b = &bPresets[i]; if (b->kind == 0) return 1; if (b->kind == 1) return (b->tex == T_RUST || b->tex == T_METAL || b->tex == T_BLOCKWALL || b->tex == T_CONCRETE || b->tex == T_PLASTER) && b->tex >= 0 ? 5 : 2; if (b->kind == 2 || b->kind == 3) return 3; return 4; }
static void edPlaceBuilding(int pi) {
    boInsert(MO_BUILDING); if (boSel < 0) return; MObj *o = &mobj[boSel]; const BPreset *b = &bPresets[pi];
    o->tex = pi; o->solid = 0; o->col = (Color){0, 0, 0, 255}; o->s = V3(b->w, b->floors * FH + 1, b->d); if (boAuto) edRebuild();
}
// ---- typed number fields: [-] value [+]; click the value to type one, Enter to apply
static int edNumAct = -1; static TextBox edNumTB;
static bool edNum(int id, float x, float y, float w, const char *lab, float *v, float step, bool en) {
    float s = aU; bool ch = false; aText(lab, x, y + 4 * s, 13, en ? C_BONE : C_BONE2, false);
    Rectangle vb = {x + w - 118 * s, y, 86 * s, 24 * s};
    if (aButton((Rectangle){x + w - 146 * s, y, 26 * s, 24 * s}, "-", en)) { *v -= step; ch = true; edNumAct = -1; }
    if (aButton((Rectangle){x + w - 28 * s, y, 26 * s, 24 * s}, "+", en)) { *v += step; ch = true; edNumAct = -1; }
    if (edNumAct == id) { edNumTB.active = true; edNumTB.max = 12; aTextBox(vb, "", &edNumTB);
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || !edNumTB.active) { char *e; float nv = strtof(edNumTB.buf, &e); if (e != edNumTB.buf) { *v = nv; ch = true; } edNumAct = -1; } }
    else { DrawRectangleRec(vb, (Color){20, 26, 28, 255}); char b[32]; snprintf(b, sizeof b, "%.2f", *v); aTextC(b, vb.x + vb.width / 2, y + 4 * s, 13, en ? WHITE : C_BONE2, true);
        if (en && aMPressed() && CheckCollisionPointRec(GetMousePosition(), vb)) { edNumAct = id; snprintf(edNumTB.buf, sizeof edNumTB.buf, "%.2f", *v); edNumTB.len = (int)strlen(edNumTB.buf); } }
    return ch;
}
static void edPropsPanel(float W, float H) {   // right-hand panel: exact position, size and turn of what is selected, and the snap steps
    float s = aU; bool obj = (edTab == 4 || edTab == 6) && boSel >= 0 && boSel < nMobj, pt = edTab == 0 && edSel >= 0 && edSel < edCount(edLayer);
    float pw = 300 * s, x = W - pw - 10 * s, y = 10 * s; int rows = obj ? 13 : pt ? 6 : 6;
    Rectangle pr = {x, y, pw, (rows * 30 + 56) * s}; aPanel(pr); x += 14 * s; y += 14 * s; float w = pw - 28 * s; int id = 0;
    if (obj) { MObj *o = &mobj[boSel]; MObj before = *o; bool blk = o->type == MO_BLOCK, bld = o->type == MO_BUILDING; Vector3 p = o->p, sz = o->s; float yw = o->yaw, pi = o->pitch, ro = o->roll;
        aText(bld ? bPresets[o->tex].name : moNames[o->type], x, y, 15, C_CYAN, true); y += 26 * s;
        edNum(id++, x, y, w, "Position X", &p.x, edStepMove, true); y += 28 * s;
        edNum(id++, x, y, w, "Position Y (height)", &p.y, edStepMove, !bld); y += 28 * s;
        edNum(id++, x, y, w, "Position Z", &p.z, edStepMove, true); y += 28 * s;
        edNum(id++, x, y, w, "Size X", &sz.x, edStepSize, blk); y += 28 * s;
        edNum(id++, x, y, w, "Size Y", &sz.y, edStepSize, !bld); y += 28 * s;
        edNum(id++, x, y, w, "Size Z", &sz.z, edStepSize, blk); y += 28 * s;
        edNum(id++, x, y, w, "Turn (yaw)", &yw, edStepAng, true); y += 28 * s;
        edNum(id++, x, y, w, "Tilt (pitch)", &pi, edStepAng, blk); y += 28 * s;
        edNum(id++, x, y, w, "Lean (roll)", &ro, edStepAng, blk); y += 34 * s;
        if (memcmp(&p, &o->p, sizeof p) || memcmp(&sz, &o->s, sizeof sz) || yw != o->yaw || pi != o->pitch || ro != o->roll) {
            edPush(); o->p = p; o->s = V3(fmaxf(0.05f, sz.x), fmaxf(0.05f, sz.y), fmaxf(0.05f, sz.z)); o->yaw = yw; o->pitch = pi; o->roll = ro; edDirty = true;
            if (bld && boAuto) edRebuild(); (void)before; } }
    else if (pt) { Vector2 q = edGet(edLayer, edSel), q0 = q;
        aText(edLayers[edLayer].kind == 5 ? edSpots[edSel].name : TextFormat("%s, point %d", edLayers[edLayer].label, edSel), x, y, 15, C_CYAN, true); y += 26 * s;
        edNum(id++, x, y, w, "X", &q.x, edStepMap, true); y += 28 * s;
        float gy = edGroundY(q.x, q.y); edNum(id++, x, y, w, "Y (the ground)", &gy, 0, false); y += 28 * s;
        edNum(id++, x, y, w, "Z", &q.y, edStepMap, true); y += 34 * s;
        if (q.x != q0.x || q.y != q0.y) { edPush(); edSetP(edLayer, edSel, q); edDirty = true; if (boAuto) edRebuild(); } }
    else { aText("Nothing selected", x, y, 15, C_BONE2, true); y += 26 * s; aText("Arrow keys and PgUp/PgDn nudge a selection.", x, y, 12, C_BONE2, false); y += 26 * s; }
    aText("Steps (snap and nudge)", x, y, 13, C_BONE2, true); y += 22 * s;
    if (edTab == 0) { edNum(40, x, y, w, "Point step (m)", &edStepMap, 0.1f, true); if (edStepMap < 0.05f) edStepMap = 0.05f; }
    else { edNum(41, x, y, w, "Move step (m)", &edStepMove, 0.05f, true); y += 28 * s; edNum(42, x, y, w, "Size step (m)", &edStepSize, 0.05f, true); y += 28 * s; edNum(43, x, y, w, "Angle step (deg)", &edStepAng, 1, true);
        if (edStepMove < 0.01f) edStepMove = 0.01f; if (edStepSize < 0.01f) edStepSize = 0.01f; if (edStepAng < 0.5f) edStepAng = 0.5f; }
}
// ---- the project picker
static bool edPickProject(void) {
    if (getenv("KIRI_EDPROJ")) { snprintf(edName, sizeof edName, "%s", getenv("KIRI_EDPROJ")); snprintf(edDir, sizeof edDir, "%s/%s", P("Mods"), edName); MakeDirectory(edDir); return true; }   // test hook
    TextBox nb = {0}; nb.max = 31; FilePathList dirs = LoadDirectoryFiles(P("Mods"));
    while (!WindowShouldClose()) {
        BeginDrawing(); ClearBackground((Color){18, 21, 22, 255}); aeroBegin(720); float s = aU, W = GetScreenWidth();
        Rectangle win = {W / 2 - 300 * s, 60 * s, 600 * s, 600 * s}; aPanel(win);
        aTextC("Zank\xC5\x8D 2006 Mod Maker", W / 2, win.y + 24 * s, 30, C_BONE, true);
        aText("Open a mod project (a folder in Mods/):", win.x + 30 * s, win.y + 80 * s, 16, C_BONE2, false);
        float y = win.y + 110 * s; int shown = 0;
        for (unsigned i = 0; i < dirs.count && shown < 9; i++) { if (IsPathFile(dirs.paths[i])) continue; const char *nm = GetFileName(dirs.paths[i]); if (nm[0] == '.') continue;
            if (aButton((Rectangle){win.x + 30 * s, y, 540 * s, 34 * s}, nm, true)) { snprintf(edName, sizeof edName, "%s", nm); snprintf(edDir, sizeof edDir, "%s", dirs.paths[i]); EndDrawing(); UnloadDirectoryFiles(dirs); return true; }
            y += 40 * s; shown++; }
        if (!shown) { aText("(none yet)", win.x + 30 * s, y, 15, C_BONE2, false); y += 30 * s; }
        aTextBox((Rectangle){win.x + 30 * s, win.y + 500 * s, 380 * s, 34 * s}, "New mod name", &nb);
        bool ok = nb.len > 0; for (int i = 0; i < nb.len; i++) if (strchr("\\/:*?\"<>|", nb.buf[i])) ok = false;
        if (aButton((Rectangle){win.x + 420 * s, win.y + 500 * s, 150 * s, 34 * s}, "Create", ok)) {
            snprintf(edName, sizeof edName, "%s", nb.buf); snprintf(edDir, sizeof edDir, "%s/%s", P("Mods"), nb.buf); MakeDirectory(edDir);
            edLoadInfo(); edSaveInfo(); EndDrawing(); UnloadDirectoryFiles(dirs); return true; }
        EndDrawing();
    }
    UnloadDirectoryFiles(dirs); return false;
}
// ---- the editor
static int editorMain(void) {
    SetWindowTitle("Zank\xC5\x8D 2006 Mod Maker"); EnableCursor();
    if (!edPickProject()) { CloseWindow(); return 0; }
    edLoadInfo();
    { char mp[800]; edPath(mp, sizeof mp, "Data/map.zmap"); if (FileExists(mp)) mapLoadFile(mp); else mapReset(); edPath(mp, sizeof mp, "Data/objects.zobj"); nMobj = 0; if (FileExists(mp)) mobjLoadFile(mp); itemsReset(); edPath(mp, sizeof mp, "Data/items.ini"); if (FileExists(mp)) itemsLoadFile(mp); if (terrEdit) { free(terrEdit); terrEdit = NULL; } edPath(mp, sizeof mp, "Data/terrain.zter"); if (FileExists(mp)) terrLoadFile(mp); }
    mobjLive = true;
    for (int i = 0; i < T_COUNT; i++) edReloadTexture(i);
    edRebuild();
    edModels = LoadDirectoryFilesEx(P("Data/Models"), ".obj", true); edModelsOk = true;
    float fogc[3] = {0.47f, 0.50f, 0.53f}; Color fogC = {120, 128, 135, 255};
    SetExitKey(0); static bool askQuit = false, help = false;
    while (true) {
        if (WindowShouldClose()) { if (!edDirty) break; askQuit = true; }   // unsaved work: ask first
        float dt = fminf(GetFrameTime(), 0.05f); aeroBegin(720); float s = aU, W = GetScreenWidth(), H = GetScreenHeight();
        if (IsFileDropped()) edDropped();
        Rectangle side = {0, 0, 330 * s, H}; Vector2 m = GetMousePosition(); bool inView = m.x > side.width && m.y > 44 * s && !((edTab == 0 || edTab == 4 || edTab == 6) && m.x > W - 312 * s && m.y < 480 * s);   // not over the properties panel
        bool typing = edNumAct >= 0 || edMVSearch.active || edBSearch.active; for (int i = 0; i < 4; i++) typing |= edTB[i].active;
        // camera: hold the right mouse button to look, WASD to fly, Q/E down/up, Shift faster, wheel sets the speed
        if (edTab == 0 || edTab == 4 || edTab == 6 || edTab == 7) {
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && inView) { Vector2 d = GetMouseDelta(); edYaw -= d.x * 0.004f; edPitch = clampf(edPitch - d.y * 0.004f, -1.5f, 1.5f); }
            if (inView) edSpeed = clampf(edSpeed * (1 + GetMouseWheelMove() * 0.15f), 4, 300);
            if (!typing) { Vector3 f = {sinf(edYaw) * cosf(edPitch), sinf(edPitch), cosf(edYaw) * cosf(edPitch)}, r = {-cosf(edYaw), 0, sinf(edYaw)};
                float sp = edSpeed * dt * (IsKeyDown(KEY_LEFT_SHIFT) ? 4 : 1); Vector3 v = {0};
                if (IsKeyDown(KEY_W)) v = Vector3Add(v, f); if (IsKeyDown(KEY_S)) v = Vector3Subtract(v, f); if (IsKeyDown(KEY_D)) v = Vector3Add(v, r); if (IsKeyDown(KEY_A)) v = Vector3Subtract(v, r);
                if (IsKeyDown(KEY_E)) v.y += 1; if (IsKeyDown(KEY_Q)) v.y -= 1;
                edPos = Vector3Add(edPos, Vector3Scale(v, sp)); }
            if (!(edPos.x == edPos.x) || !(edPos.y == edPos.y) || !(edPos.z == edPos.z)) edPos = V3(228, 70, 60);   // never lose the camera
            edPos.x = clampf(edPos.x, HF_X0 - 200, HF_X0 + HF_N * HF_S + 200); edPos.z = clampf(edPos.z, HF_X0 - 200, HF_X0 + HF_N * HF_S + 200); edPos.y = clampf(edPos.y, -30, 420);
        }
        cam.position = edPos; cam.target = Vector3Add(edPos, V3(sinf(edYaw) * cosf(edPitch), sinf(edPitch), cosf(edYaw) * cosf(edPitch))); cam.up = V3(0, 1, 0); cam.fovy = 70; cam.projection = CAMERA_PERSPECTIVE;
        // picking and dragging points of the chosen layer
        int n = edCount(edLayer); int hover = -1;
        if (edTab == 0 && inView) { float bd = 14; for (int j = 0; j < n; j++) { Vector2 p = edGet(edLayer, j); Vector3 w = V3(p.x, edGroundY(p.x, p.y) + 0.4f, p.y);
                if (Vector3DotProduct(Vector3Subtract(w, edPos), Vector3Subtract(cam.target, edPos)) <= 0) continue;
                Vector2 sc = GetWorldToScreen(w, cam); float d = Vector2Distance(sc, m); if (d < bd) { bd = d; hover = j; } } }
        Ray ray = GetScreenToWorldRay(m, cam);
        if (edTab == 0 && inView && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (hover >= 0) { edPush(); edNoPush = true; }
            if (hover >= 0 && edLayers[edLayer].kind <= 2 && edBrush > 0) {   // fill in points around it so the line bends as a smooth curve
                Vector2 c0 = edGet(edLayer, hover); for (int j = 0; j + 1 < edCount(edLayer) && j < 20000; j++) {
                    if (!edSameGroup(edLayer, j, hover) || !edSameGroup(edLayer, j + 1, hover)) continue;
                    Vector2 a = edGet(edLayer, j), b = edGet(edLayer, j + 1); if (Vector2Distance(a, b) <= 2.5f || fminf(Vector2Distance(a, c0), Vector2Distance(b, c0)) > edBrush + 4) continue;
                    MapArr *ma = mapArr(edLayers[edLayer].arr); if (mapRows(ma) + 2 >= ma->cap) break;
                    edInsert(edLayer, j); if (hover > j) hover++; j--; }
                n = edCount(edLayer); }
            edNoPush = false;
            if (hover >= 0) { edSel = hover; Vector2 p = edGet(edLayer, hover); edDragY = edGroundY(p.x, p.y) + 0.4f; edDragStart = p; edDragging = true; edDragN = 0;
                int kind = edLayers[edLayer].kind; float R = (kind <= 2) ? edBrush : 0;
                for (int j = 0; j < n && edDragN < 4096; j++) { if (!edSameGroup(edLayer, j, hover)) continue; Vector2 q = edGet(edLayer, j); float d = Vector2Distance(q, p);
                    if (j != hover && (R <= 0 || d >= R)) continue; float w = j == hover ? 1 : 1 - d / R; w = w * w * (3 - 2 * w);
                    edDragIdx[edDragN] = j; edDragW[edDragN] = w; edDragOrig[edDragN] = q; edDragN++; } }
            else edSel = -1; }
        if (edDragging) {
            if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { edDragging = false; if (edSnapHit && edSnapStreet >= 0 && edSnapEnd >= 0 && edLayers[edLayer].kind == 2 && dStreets[edSnapStreet][3] == dStreets[edStreetOf(edSel)][3]) edJoinStreets(edSel, edSnapStreet, edSnapEnd); else if (edSnapHit) edSay("Snapped onto the road: a junction."); edSnapHit = false; if (boAuto && edDirty) edRebuild(); }   // the town follows the line as soon as it is let go
            else if (fabsf(ray.direction.y) > 1e-4f) { float t = (edDragY - ray.position.y) / ray.direction.y; if (t > 0) { Vector2 hit = {ray.position.x + ray.direction.x * t, ray.position.z + ray.direction.z * t}, d = Vector2Subtract(hit, edDragStart);
                for (int k = 0; k < edDragN; k++) edSetP(edLayer, edDragIdx[k], Vector2Add(edDragOrig[k], Vector2Scale(d, edDragW[k]))); edDirty = true;
                edSnapHit = false; edSnapStreet = edSnapEnd = -1;
                if (edSnapOn && !IsKeyDown(KEY_LEFT_ALT) && edSel >= 0 && edIsEnd(edLayer, edSel)) { Vector2 q; int ts, te;   // an end catches the nearest road (Alt: no snapping)
                    if (edRoadNearest(edLayer, edSel, edGet(edLayer, edSel), &q, &ts, &te)) { edSetP(edLayer, edSel, q); edSnapHit = true; edSnapAt = q; if (edLayers[edLayer].kind == 2) { edSnapStreet = ts; edSnapEnd = te; } } } } } }
        if (edTab == 0 && !typing) {
            if (IsKeyPressed(KEY_INSERT) && edSel >= 0) edInsert(edLayer, edSel);
            if (IsKeyPressed(KEY_DELETE) && edSel >= 0) { if (IsKeyDown(KEY_LEFT_SHIFT) && edLayers[edLayer].kind == 2) edDeleteStreet(edSel); else edDelete(edLayer, edSel); }
            if (IsKeyPressed(KEY_F5)) edRebuild();
            if (IsKeyPressed(KEY_N) && edLayers[edLayer].kind == 2 && fabsf(ray.direction.y) > 1e-4f) { float t = (SIDEH - ray.position.y) / ray.direction.y; if (t > 0) edNewStreet((Vector2){ray.position.x + ray.direction.x * t, ray.position.z + ray.direction.z * t}); }
        }
        if (edTab == 4 || edTab == 6) {   // Build: pick a part, drag its handles (Move: world arrows, Size: its own sides), Turn: drag sideways
            Vector3 O, E[3]; float len[3]; int hAx = -1;
            if (boSel >= 0 && boSel < nMobj && boTool != 1) { boHandles(&mobj[boSel], &O, E, len); Vector2 so = GetWorldToScreen(O, cam); float bd = 10;
                for (int a = 0; a < 3; a++) { float d = boSegDist(m, so, GetWorldToScreen(E[a], cam)); if (d < bd) { bd = d; hAx = a; } } }
            if (inView && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (hAx >= 0) { edPush(); boAxis = hAx; boOrig = mobj[boSel]; boMouse0 = m; }
                else { int best = -1; float bt = 1e9f; for (int i = 0; i < nMobj; i++) { float t = boPick(ray, &mobj[i]); if (t >= 0 && t < bt) { bt = t; best = i; } }
                    boSel = best; boAxis = -1;
                    if (best >= 0) { edPush(); boOrig = mobj[best]; boMouse0 = m; boAxis = 3;
                        float t = fabsf(ray.direction.y) > 1e-4f ? (boOrig.p.y - ray.position.y) / ray.direction.y : -1; Vector3 hit = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
                        boOff = t > 0 ? Vector3Subtract(boOrig.p, hit) : V3(0, 0, 0); } } }
            if (boAxis >= 0 && boSel >= 0 && boSel < nMobj) { MObj *o = &mobj[boSel];
                if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { boAxis = -1; if (o->type == MO_BUILDING && boAuto && edDirty) edRebuild(); }   // a building is rebuilt where it was let go
                else if (boTool == 1) { float da = (m.x - boMouse0.x) * 0.5f; float *ang = &o->yaw, base = boOrig.yaw;   // drag: yaw; Shift: pitch; Ctrl: roll (blocks tilt)
                    if (o->type == MO_BLOCK && IsKeyDown(KEY_LEFT_SHIFT)) { ang = &o->pitch; base = boOrig.pitch; } else if (o->type == MO_BLOCK && IsKeyDown(KEY_LEFT_CONTROL)) { ang = &o->roll; base = boOrig.roll; }
                    *ang = boSnapV(base + da, edStepAng); edDirty = true; }
                else if (boAxis == 3) { if (boTool == 0 && fabsf(ray.direction.y) > 1e-4f) { float t = (boOrig.p.y - ray.position.y) / ray.direction.y; if (t > 0) {
                    Vector3 hit = Vector3Add(Vector3Add(ray.position, Vector3Scale(ray.direction, t)), boOff); o->p.x = boSnapV(hit.x, edStepMove); o->p.z = boSnapV(hit.z, edStepMove); edDirty = true; } } }
                else { boHandles(&boOrig, &O, E, len); Vector2 so = GetWorldToScreen(O, cam), se = GetWorldToScreen(E[boAxis], cam), d = Vector2Subtract(se, so); float L = Vector2Length(d);
                    if (L > 2) { float tw = Vector2DotProduct(Vector2Subtract(m, boMouse0), Vector2Scale(d, 1 / L)) / L * len[boAxis]; Vector3 ax = boAxisW(&boOrig, boAxis, boTool == 2);
                        if (boTool == 0) { o->p = Vector3Add(boOrig.p, Vector3Scale(ax, tw)); o->p.x = boSnapV(o->p.x, edStepMove); o->p.y = boSnapV(o->p.y, edStepMove); o->p.z = boSnapV(o->p.z, edStepMove); }
                        else if (boOrig.type == MO_BLOCK || (boAxis == 1 && boOrig.type != MO_BUILDING)) { float *sz = boAxis == 0 ? &o->s.x : boAxis == 1 ? &o->s.y : &o->s.z, base = boAxis == 0 ? boOrig.s.x : boAxis == 1 ? boOrig.s.y : boOrig.s.z;
                            *sz = fmaxf(0.1f, boSnapV(base + tw * (boAxis == 1 ? 1 : 2), edStepSize)); }
                        edDirty = true; } } }
            if (!typing && boSel >= 0 && boSel < nMobj) {
                if (IsKeyPressed(KEY_DELETE)) { edPush(); mobj[boSel] = mobj[--nMobj]; boSel = -1; edDirty = true; }
                else if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_D) && nMobj < MAX_MOBJ) { edPush(); mobj[nMobj] = mobj[boSel]; mobj[nMobj].p.x += 1; boSel = nMobj++; edDirty = true; } }
            if (!typing && IsKeyPressed(KEY_ONE)) boTool = 0; if (!typing && IsKeyPressed(KEY_TWO)) boTool = 1; if (!typing && IsKeyPressed(KEY_THREE)) boTool = 2;
        }
        if (edTab == 7) {   // terrain brush: hold the left mouse on the ground
            edTerHit = false; Vector3 q = ray.position;
            for (int k = 0; k < 1500; k++) { q = Vector3Add(q, Vector3Scale(ray.direction, 0.5f)); if (q.y < terrainH(q.x, q.z)) { edTerHit = true; edTerAt = q; break; } }
            if (inView && edTerHit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { edPush(); edTerFlatY = terrainH(edTerAt.x, edTerAt.z); if (!terrEdit) terrEdit = calloc((size_t)HF_N * HF_N, sizeof(float)); }
            if (inView && edTerHit && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && terrEdit) { edTerPaint(dt); edTerT += dt; if (edTerT > 0.12f) { genTerrain(); edTerT = 0; } }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && edTerPainted) { genTerrain(); edTerPainted = false; if (boAuto) edRebuild(); }
        }
        if (!typing && edNumAct < 0) {   // nudge the selection: arrows on the ground, PgUp/PgDn up and down
            float nx = (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) - (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)), nz = (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) - (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)), ny = (IsKeyPressed(KEY_PAGE_UP) || IsKeyPressedRepeat(KEY_PAGE_UP)) - (IsKeyPressed(KEY_PAGE_DOWN) || IsKeyPressedRepeat(KEY_PAGE_DOWN));
            if (nx || nz || ny) {
                if ((edTab == 4 || edTab == 6) && boSel >= 0 && boSel < nMobj) { edPush(); MObj *o = &mobj[boSel]; o->p.x += nx * edStepMove; o->p.z += nz * edStepMove; if (o->type != MO_BUILDING) o->p.y += ny * edStepMove; edDirty = true; }
                else if (edTab == 0 && edSel >= 0 && edSel < edCount(edLayer) && (nx || nz)) { edPush(); Vector2 q = edGet(edLayer, edSel); edSetP(edLayer, edSel, (Vector2){q.x + nx * edStepMap, q.y + nz * edStepMap}); edDirty = true; } } }
        if (!typing && IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Z) && (edTab == 0 || edTab == 4 || edTab == 6 || edTab == 7)) edUndo(IsKeyDown(KEY_LEFT_SHIFT));
        if (!typing && IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Y) && (edTab == 0 || edTab == 4 || edTab == 6 || edTab == 7)) edUndo(true);
        if (!typing && IsKeyPressed(KEY_HOME)) { edPos = V3(228, 70, 60); edYaw = 0; edPitch = -0.75f; edSpeed = 30; }
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) edSay(edSaveMap() ? "Saved." : "Could not save.");
        // ---- draw
        float amb = 0.82f * gs.bright, fogD = 0.0025f, zero = 0, one = 1, snap[2] = {16384, 16384}; int nl = 0;
        SetShaderValue(sh, locFog, fogc, SHADER_UNIFORM_VEC3); SetShaderValue(sh, locView, &cam.position, SHADER_UNIFORM_VEC3); SetShaderValue(sh, locFlashOn, &zero, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, locMuzzle, &zero, SHADER_UNIFORM_FLOAT); SetShaderValue(sh, locAmb, &amb, SHADER_UNIFORM_FLOAT); SetShaderValue(sh, locFogD, &fogD, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, locSnap, snap, SHADER_UNIFORM_VEC2); SetShaderValue(sh, locNL, &nl, SHADER_UNIFORM_INT); SetShaderValue(sh, locDesat, &zero, SHADER_UNIFORM_FLOAT); SetShaderValue(sh, locRage, &zero, SHADER_UNIFORM_FLOAT); (void)one;
        BeginDrawing(); ClearBackground(fogC);
        if (edTab == 4 || edTab == 6 || edTab == 7) {
            BeginMode3D(cam); drawWorld(cam.position, (float)GetTime()); boDraw(); rlDrawRenderBatchActive();
            if (edTab == 7 && edTerHit) { rlDisableDepthTest(); for (int k = 0; k < 48; k++) { float a0 = k * 2 * PI / 48, a1 = (k + 1) * 2 * PI / 48; Vector3 p0 = V3(edTerAt.x + cosf(a0) * edTerR, 0, edTerAt.z + sinf(a0) * edTerR), p1 = V3(edTerAt.x + cosf(a1) * edTerR, 0, edTerAt.z + sinf(a1) * edTerR);
                p0.y = terrainH(p0.x, p0.z) + 0.3f; p1.y = terrainH(p1.x, p1.z) + 0.3f; DrawLine3D(p0, p1, (Color){111, 214, 208, 255}); } DrawSphere(V3(edTerAt.x, edTerAt.y + 0.3f, edTerAt.z), 0.3f, WHITE); rlDrawRenderBatchActive(); rlEnableDepthTest(); }
            if (edTab != 7 && boSel >= 0 && boSel < nMobj) { MObj *o = &mobj[boSel]; Vector3 z = boShowSize(o); rlPushMatrix(); rlTranslatef(o->p.x, o->p.y + z.y / 2, o->p.z); rlMultMatrixf(MatrixToFloat(moRot(o)));
                DrawCubeWires(V3(0, 0, 0), z.x + 0.05f, z.y + 0.05f, z.z + 0.05f, (Color){111, 214, 208, 255}); rlPopMatrix(); rlDrawRenderBatchActive(); rlDisableDepthTest();
                if (boTool != 1) { Vector3 O, E[3]; float len[3]; boHandles(o, &O, E, len); Color hc[3] = {{230, 70, 60, 255}, {90, 220, 90, 255}, {80, 140, 240, 255}};
                    for (int a = 0; a < 3; a++) { DrawLine3D(O, E[a], hc[a]); if (boTool == 0) DrawSphere(E[a], 0.28f, hc[a]); else DrawCube(E[a], 0.45f, 0.45f, 0.45f, hc[a]); } }
                rlDrawRenderBatchActive(); rlEnableDepthTest(); }
            EndMode3D();
            for (int i = 0; i < nMobj; i++) if (mobj[i].type != MO_BLOCK && mobj[i].type != MO_BUILDING && Vector3DistanceSqr(mobj[i].p, edPos) < 80 * 80) { Vector2 sp = GetWorldToScreen(V3(mobj[i].p.x, mobj[i].p.y + boShowSize(&mobj[i]).y + 0.5f, mobj[i].p.z), cam); if (sp.x > side.width) aTextC(moNames[mobj[i].type], sp.x, sp.y, 12, WHITE, false); }
        }
        if (edTab == 0) {
            BeginMode3D(cam); drawWorld(cam.position, (float)GetTime()); boDraw(); rlDrawRenderBatchActive(); rlDisableDepthTest();
            for (int L = 0; L < ED_NL; L++) { const EdLayer *e = &edLayers[L]; int c = edCount(L); bool act = L == edLayer; Color lc = act ? e->col : aA(e->col, 90);
                if (e->kind == 5 && !act) continue;
                for (int j = 0; j < c; j++) { Vector2 p = edGet(L, j); Vector3 a = V3(p.x, edGroundY(p.x, p.y) + 0.4f, p.y);
                    bool link = (e->kind <= 2 && j + 1 < c && edSameGroup(L, j, j + 1)) || (e->kind == 4 && j % 2 == 0);
                    if (link) { Vector2 q = edGet(L, j + 1); DrawLine3D(a, V3(q.x, edGroundY(q.x, q.y) + 0.4f, q.y), lc); }
                    if (e->kind == 1 && j == c - 1 && c > 2) { Vector2 q = edGet(L, 0); DrawLine3D(a, V3(q.x, edGroundY(q.x, q.y) + 0.4f, q.y), lc); }
                    if (act && Vector3DistanceSqr(a, edPos) < 260 * 260) { float r = j == edSel ? 0.9f : j == hover ? 0.7f : 0.35f; DrawCube(a, r, r, r, j == edSel ? WHITE : lc); } } }
            if (edSnapHit && edDragging) { float gy = edGroundY(edSnapAt.x, edSnapAt.y) + 0.6f; Color sc = edSnapEnd >= 0 ? (Color){120, 255, 140, 255} : (Color){111, 214, 208, 255}; for (int k = 0; k < 3; k++) DrawCircle3D(V3(edSnapAt.x, gy, edSnapAt.y), 1.2f + k * 0.5f, V3(1, 0, 0), 90, sc); }
            if (edSel >= 0 && edLayers[edLayer].kind <= 2 && edSel < n) { Vector2 p = edGet(edLayer, edSel); DrawCircle3D(V3(p.x, edGroundY(p.x, p.y) + 0.5f, p.y), edBrush, V3(1, 0, 0), 90, aA(WHITE, 160)); }
            rlDrawRenderBatchActive(); rlEnableDepthTest(); EndMode3D();
            if (hover >= 0 && edLayers[edLayer].kind == 5) aText(edSpots[hover].name, m.x + 14, m.y - 6, 15, WHITE, true);
        }
        // ---- panel
        aPanel(side);
        float y = 14 * s; aText("Mod Maker", 18 * s, y, 22, C_BONE, true); y += 30 * s;
        { char b[96]; snprintf(b, sizeof b, "Project: %s%s     F1 help", edName, edDirty ? "  (unsaved)" : ""); aText(b, 18 * s, y, 13, C_CYAN, false); } y += 26 * s;
        {   // tabs, in three groups and an "All" view
            static int cat = 0; const char *cats[4] = {"World", "Objects", "Assets", "All"};
            static const char *tn[8] = {"Map", "Terrain", "Build", "Buildings", "Items", "Textures", "Models", "Mod"}; static const int tid[8] = {0, 7, 4, 6, 5, 1, 2, 3}, tcat[8] = {0, 0, 1, 1, 1, 2, 2, 2};
            for (int i = 0; i < 8; i++) if (tid[i] == edTab && cat != 3) cat = tcat[i];
            for (int c = 0; c < 4; c++) if (aButton((Rectangle){(14 + c * 76) * s, y, 72 * s, 26 * s}, cats[c], cat != c)) { cat = c; if (c != 3) for (int i = 0; i < 8; i++) if (tcat[i] == c) { edTab = tid[i]; break; } }
            y += 32 * s; int k = 0;
            for (int i = 0; i < 8; i++) { if (cat != 3 && tcat[i] != cat) continue; int per = cat == 3 ? 4 : 3; float bw = (300.0f - (per - 1) * 4) / per;
                if (aButton((Rectangle){(14 + (k % per) * (bw + 4)) * s, y + (k / per) * 34 * s, bw * s, 30 * s}, tn[i], edTab != tid[i])) edTab = tid[i]; k++; }
            y += ((k + (cat == 3 ? 3 : 2)) / (cat == 3 ? 4 : 3)) * 34 * s - 34 * s;
        }
        y += 44 * s;
        if (edTab == 0) {
            aText("Layer", 18 * s, y, 15, C_BONE2, true); y += 22 * s;
            for (int L = 0; L < ED_NL; L++) { if (aButton((Rectangle){(14 + (L % 2) * 152) * s, y, 148 * s, 25 * s}, edLayers[L].label, L != edLayer)) { edLayer = L; edSel = -1; } if (L % 2) y += 28 * s; }
            if (ED_NL % 2) y += 28 * s; y += 26 * s; edBrush = aSlider((Rectangle){18 * s, y, 290 * s, 18 * s}, "Soft drag radius (m)", edBrush, 0, 60, "%.0f"); y += 30 * s;
            if (aButton((Rectangle){14 * s, y, 146 * s, 30 * s}, "Add point", edSel >= 0 && edLayers[edLayer].kind < 4)) edInsert(edLayer, edSel);
            if (aButton((Rectangle){168 * s, y, 146 * s, 30 * s}, "Delete point", edSel >= 0 && edLayers[edLayer].kind < 4)) edDelete(edLayer, edSel); y += 36 * s;
            if (aButton((Rectangle){14 * s, y, 146 * s, 30 * s}, "Rebuild (F5)", true)) edRebuild();
            if (aButton((Rectangle){168 * s, y, 146 * s, 30 * s}, "Save (Ctrl+S)", true)) edSay(edSaveMap() ? "Saved." : "Could not save."); y += 36 * s;
            boAuto = aToggle(18 * s, y, "Rebuild when a point is let go", boAuto); y += 32 * s;
            edSnapOn = aToggle(18 * s, y, "Snap road ends to roads (Alt: off)", edSnapOn); y += 32 * s;
            if (aButton((Rectangle){14 * s, y, 300 * s, 28 * s}, "Reset to the built-in map", true)) { mapReset(); edSel = -1; edDirty = true; edRebuild(); } y += 36 * s;
            if (edLayers[edLayer].kind == 2 && edSel >= 0) { int s2 = edStreetOf(edSel); if (s2 >= 0) { char b[96];
                snprintf(b, sizeof b, "Street %d: width %.1f m, %s", s2, dStreets[s2][2] / 10.0f, (const char *[]){"street", "avenue (lines)", "stairs", "bridge"}[dStreets[s2][3] & 3]); aText(b, 18 * s, y, 13, C_BONE, false); y += 22 * s;
                if (aButton((Rectangle){14 * s, y, 70 * s, 26 * s}, "Wider", true)) { dStreets[s2][2] += 5; edDirty = true; }
                if (aButton((Rectangle){90 * s, y, 70 * s, 26 * s}, "Narrower", dStreets[s2][2] > 30)) { dStreets[s2][2] -= 5; edDirty = true; }
                if (aButton((Rectangle){166 * s, y, 148 * s, 26 * s}, "Change type", true)) { dStreets[s2][3] = (dStreets[s2][3] + 1) % 4; edDirty = true; } y += 32 * s;
                if (aButton((Rectangle){14 * s, y, 300 * s, 26 * s}, "Split the street at this point", !edIsEnd(1, edSel))) { edSplitStreet(edSel); if (boAuto) edRebuild(); } y += 32 * s; } }
            if (y < H - 40 * s) aText("F1: all controls", 18 * s, H - 30 * s, 12, C_BONE2, false);
        } else if (edTab == 4) {
            const char *tools[3] = {"Move (1)", "Turn (2)", "Size (3)"};
            for (int i = 0; i < 3; i++) if (aButton((Rectangle){(14 + i * 101) * s, y, 97 * s, 30 * s}, tools[i], boTool != i)) boTool = i; y += 40 * s;
            boSnap = aToggle(18 * s, y, "Snap to the steps (right panel)", boSnap); y += 32 * s;
            aText("Insert", 18 * s, y, 15, C_BONE2, true); y += 22 * s;
            for (int t = 0; t < MO_BUILDING; t++) { if (aButton((Rectangle){(14 + (t % 2) * 152) * s, y, 148 * s, 25 * s}, moNames[t], true)) boInsert(t); if (t % 2) y += 28 * s; }   // buildings come from the Buildings tab
            y += 10 * s;
            if (boSel >= 0 && boSel < nMobj) { MObj *o = &mobj[boSel]; char b[128];
                snprintf(b, sizeof b, "%s  %.2f x %.2f x %.2f m, %.0f deg", moNames[o->type], o->s.x, o->s.y, o->s.z, o->yaw); aText(b, 18 * s, y, 13, C_BONE, true); y += 24 * s;
                if (o->type == MO_BLOCK) {
                    if (aButton((Rectangle){14 * s, y, 40 * s, 26 * s}, "<", true)) { o->tex = (o->tex + T_COUNT - 1) % T_COUNT; edDirty = true; }
                    if (aButton((Rectangle){274 * s, y, 40 * s, 26 * s}, ">", true)) { o->tex = (o->tex + 1) % T_COUNT; edDirty = true; }
                    aTextC(texNames[o->tex], 164 * s, y + 4 * s, 13, C_BONE, false); y += 32 * s;
                    for (int c = 0; c < 8; c++) { Rectangle sw = {(16 + c * 37) * s, y, 32 * s, 24 * s}; DrawRectangleRec(sw, boCols[c]); if (o->col.r == boCols[c].r && o->col.g == boCols[c].g && o->col.b == boCols[c].b) DrawRectangleLinesEx(aGrow(sw, 2), 2, C_CYAN);
                        if (aMPressed() && CheckCollisionPointRec(m, sw)) { o->col = boCols[c]; edDirty = true; } } y += 34 * s;
                    bool so = aToggle(18 * s, y, "Solid (you collide with it)", o->solid != 0); if (so != (o->solid != 0)) { o->solid = so; edDirty = true; } y += 32 * s; }
                if (aButton((Rectangle){14 * s, y, 146 * s, 28 * s}, "Duplicate", nMobj < MAX_MOBJ)) { mobj[nMobj] = *o; mobj[nMobj].p.x += 1; boSel = nMobj++; edDirty = true; }
                if (aButton((Rectangle){168 * s, y, 146 * s, 28 * s}, "Delete", true)) { mobj[boSel] = mobj[--nMobj]; boSel = -1; edDirty = true; } y += 36 * s; }
            else { aText(TextFormat("%d parts. Click one to select it.", nMobj), 18 * s, y, 13, C_BONE2, false); y += 24 * s; }
            if (aButton((Rectangle){14 * s, y, 146 * s, 28 * s}, "Preview in game", true)) { mobjLive = false; edRebuild(); mobjLive = true; edSay("Props shown as in the game until the next rebuild."); }
            if (aButton((Rectangle){168 * s, y, 146 * s, 28 * s}, "Save (Ctrl+S)", true)) edSay(edSaveMap() ? "Saved." : "Could not save.");
            aText("Drag a part to move it, drag the arrows / cubes", 18 * s, H - 50 * s, 12, C_BONE2, false);
            aText("Ctrl+D duplicate  Del delete  RMB look  WASD fly", 18 * s, H - 34 * s, 12, C_BONE2, false);
        } else if (edTab == 6) {
            for (int c = 0; c < 6; c++) if (aButton((Rectangle){(14 + (c % 3) * 101) * s, y + (c / 3) * 30 * s, 97 * s, 26 * s}, edBCats[c], edBCat != c)) { edBCat = c; edBScroll = 0; }
            y += 64 * s; y += 18 * s; edBSearch.max = 30; aTextBox((Rectangle){14 * s, y, 300 * s, 26 * s}, "Search buildings", &edBSearch); y += 32 * s;
            static int vis[NBPRESET]; int nv = 0;
            for (int i = 0; i < NBPRESET; i++) { if (edBCat && edBCatOf(i) != edBCat) continue;
                if (edBSearch.len) { char a[64], q[40]; snprintf(a, sizeof a, "%s", bPresets[i].name); snprintf(q, sizeof q, "%s", edBSearch.buf); for (char *c = a; *c; c++) *c = (char)tolower((unsigned char)*c); for (char *c = q; *c; c++) *c = (char)tolower((unsigned char)*c); if (!strstr(a, q)) continue; }
                vis[nv++] = i; }
            int rows = (int)((H - y - 250 * s) / (24 * s)); if (rows < 4) rows = 4;
            if (GetMouseWheelMove() != 0 && m.x < side.width) edBScroll = (int)clampf(edBScroll - GetMouseWheelMove() * 3, 0, fmaxf(0, nv - rows)); if (edBScroll > nv) edBScroll = 0;
            for (int k = edBScroll; k < nv && k < edBScroll + rows; k++) { int i = vis[k]; char b[80]; snprintf(b, sizeof b, "%s  (%dF)", bPresets[i].name, bPresets[i].floors);
                if (aButton((Rectangle){14 * s, y, 300 * s, 22 * s}, b, i != edBSel)) edBSel = i; y += 24 * s; }
            aText(TextFormat("%d of %d presets", nv, NBPRESET), 18 * s, y + 2 * s, 12, C_BONE2, false); y += 22 * s;
            if (aButton((Rectangle){14 * s, y, 300 * s, 30 * s}, TextFormat("Place: %s", bPresets[edBSel].name), true)) edPlaceBuilding(edBSel); y += 38 * s;
            if (boSel >= 0 && boSel < nMobj && mobj[boSel].type == MO_BUILDING) { MObj *o = &mobj[boSel]; const BPreset *b = &bPresets[o->tex]; int fl = o->solid > 0 ? o->solid : b->floors;
                aText(TextFormat("%s, %d storeys, %.0f deg", b->name, fl, o->yaw), 18 * s, y, 13, C_BONE, true); y += 24 * s;
                if (aButton((Rectangle){14 * s, y, 70 * s, 26 * s}, "Fewer", fl > 1)) { edPush(); o->solid = fl - 1; edDirty = true; if (boAuto) edRebuild(); }
                if (aButton((Rectangle){90 * s, y, 70 * s, 26 * s}, "More", fl < 14)) { edPush(); o->solid = fl + 1; edDirty = true; if (boAuto) edRebuild(); }
                if (aButton((Rectangle){166 * s, y, 148 * s, 26 * s}, "Use the picked preset", o->tex != edBSel)) { edPush(); o->tex = edBSel; o->solid = 0; o->s = V3(bPresets[edBSel].w, bPresets[edBSel].floors * FH + 1, bPresets[edBSel].d); edDirty = true; if (boAuto) edRebuild(); } y += 32 * s;
                for (int c = 0; c < 8; c++) { Rectangle sw = {(16 + c * 37) * s, y, 32 * s, 22 * s}; DrawRectangleRec(sw, boCols[c]); if (aMPressed() && CheckCollisionPointRec(m, sw)) { edPush(); o->col = boCols[c]; edDirty = true; if (boAuto) edRebuild(); } } y += 28 * s;
                if (aButton((Rectangle){14 * s, y, 96 * s, 26 * s}, "Preset colour", o->col.r || o->col.g || o->col.b)) { edPush(); o->col = (Color){0, 0, 0, 255}; edDirty = true; if (boAuto) edRebuild(); }
                if (aButton((Rectangle){116 * s, y, 96 * s, 26 * s}, "Duplicate", nMobj < MAX_MOBJ)) { edPush(); mobj[nMobj] = *o; mobj[nMobj].p.x += b->w + 2; boSel = nMobj++; edDirty = true; if (boAuto) edRebuild(); }
                if (aButton((Rectangle){218 * s, y, 96 * s, 26 * s}, "Delete", true)) { edPush(); mobj[boSel] = mobj[--nMobj]; boSel = -1; edDirty = true; if (boAuto) edRebuild(); } y += 32 * s; }
            boAuto = aToggle(18 * s, H - 64 * s, "Rebuild when let go", boAuto);
            aText("Drag a building to move it; 2 then drag to turn", 18 * s, H - 30 * s, 12, C_BONE2, false);
        } else if (edTab == 7) {
            for (int t = 0; t < 5; t++) if (aButton((Rectangle){(14 + t * 61) * s, y, 58 * s, 30 * s}, edTerNames[t], edTerTool != t)) edTerTool = t; y += 62 * s;
            edTerR = aSlider((Rectangle){18 * s, y, 290 * s, 18 * s}, "Brush radius (m)", edTerR, 2, 60, "%.0f"); y += 44 * s;
            edTerS = aSlider((Rectangle){18 * s, y, 290 * s, 18 * s}, "Strength", edTerS, 0.5f, 20, "%.1f"); y += 40 * s;
            boAuto = aToggle(18 * s, y, "Rebuild the town when let go", boAuto); y += 34 * s;
            if (aButton((Rectangle){14 * s, y, 300 * s, 30 * s}, "Rebuild (F5)", true)) edRebuild(); y += 36 * s;
            if (aButton((Rectangle){14 * s, y, 300 * s, 30 * s}, "Clear all terrain changes", terrEdit != NULL)) { edPush(); free(terrEdit); terrEdit = NULL; edDirty = true; edRebuild(); } y += 40 * s;
            aText("Hold the left mouse on the ground to paint.", 18 * s, y, 13, C_BONE2, false); y += 20 * s;
            aText("Flatten levels to the height you first click.", 18 * s, y, 13, C_BONE2, false); y += 20 * s;
            aText("The town's paving stays flat.", 18 * s, y, 13, C_BONE2, false);
        } else if (edTab == 5) {
            int rows = (int)((H - y - 20 * s) / (25 * s));
            if (GetMouseWheelMove() != 0 && m.x < side.width) edScrollI = (int)clampf(edScrollI - GetMouseWheelMove() * 3, 0, fmaxf(0, IT_N - rows));
            for (int t = edScrollI; t < IT_N && t < edScrollI + rows; t++) { char b[64]; snprintf(b, sizeof b, "%s%s", idef[t].name, memcmp(&idef[t], &idefBase[t], sizeof idef[t]) ? "  *" : "");
                if (aButton((Rectangle){14 * s, y, 300 * s, 23 * s}, b, t != edItemSel)) edItemSel = t; y += 25 * s; }
        } else if (edTab == 1) {
            aText("Pick a texture, then drop a .png onto the window.", 18 * s, y, 13, C_BONE2, false); y += 24 * s;
            int rows = (int)((H - y - 230 * s) / (25 * s)); if (rows < 4) rows = 4;
            if (GetMouseWheelMove() != 0 && m.x < side.width) edScrollT = (int)clampf(edScrollT - GetMouseWheelMove() * 3, 0, fmaxf(0, T_COUNT - rows));
            for (int i = edScrollT; i < T_COUNT && i < edScrollT + rows; i++) { char rel[128], mp[800]; snprintf(rel, sizeof rel, "Textures/%s", texNames[i]); edPath(mp, sizeof mp, rel);
                char b[96]; snprintf(b, sizeof b, "%s%s", texNames[i], FileExists(mp) ? "  *" : ""); if (aButton((Rectangle){14 * s, y, 300 * s, 23 * s}, b, i != edTexSel)) edTexSel = i; y += 25 * s; }
            float ps = 190 * s; DrawTexturePro(tex[edTexSel], (Rectangle){0, 0, (float)tex[edTexSel].width, (float)tex[edTexSel].height}, (Rectangle){70 * s, H - ps - 30 * s, ps, ps}, (Vector2){0}, 0, WHITE);
            { char rel[128], mp[800]; snprintf(rel, sizeof rel, "Textures/%s", texNames[edTexSel]); edPath(mp, sizeof mp, rel);
              if (FileExists(mp) && aButton((Rectangle){14 * s, H - 26 * s, 300 * s, 22 * s}, "Remove this override", true)) { remove(mp); edReloadTexture(edTexSel); edSay("Override removed."); } }
        } else if (edTab == 2) {
            y += 20 * s; edMVSearch.max = 40; aTextBox((Rectangle){14 * s, y, 300 * s, 28 * s}, "Search models", &edMVSearch); y += 36 * s;
            static int vis[4096]; int nv = 0; int cnt = (int)edModels.count;
            for (int i = 0; i < cnt && nv < 4096; i++) { if (edMVSearch.len) { char a[128], q[64]; snprintf(a, sizeof a, "%s", edModelRel(i)); snprintf(q, sizeof q, "%s", edMVSearch.buf); for (char *c = a; *c; c++) *c = (char)tolower((unsigned char)*c); for (char *c = q; *c; c++) *c = (char)tolower((unsigned char)*c); if (!strstr(a, q)) continue; } vis[nv++] = i; }
            int rows = (int)((H - y - 140 * s) / (25 * s)); if (rows < 3) rows = 3;
            if (GetMouseWheelMove() != 0 && m.x < side.width) edScrollM = (int)clampf(edScrollM - GetMouseWheelMove() * 3, 0, fmaxf(0, nv - rows)); if (edScrollM > nv) edScrollM = 0;
            for (int k = edScrollM; k < nv && k < edScrollM + rows; k++) { int i = vis[k]; char mp[800], full[300]; snprintf(full, sizeof full, "Data/Models/%s", edModelRel(i)); edPath(mp, sizeof mp, full);
                char b[120]; snprintf(b, sizeof b, "%s%s", edModelRel(i), FileExists(mp) ? "  *" : ""); if (aButton((Rectangle){14 * s, y, 300 * s, 23 * s}, b, i != edModelSel)) { edModelSel = i; edMVShowOrig = false; } y += 25 * s; }
            aText(TextFormat("%d of %d models  (* = replaced in this mod)", nv, cnt), 18 * s, y + 4 * s, 12, C_BONE2, false); y = H - 108 * s;
            { bool w = aToggle(18 * s, y, "Wireframe", edMVWire); edMVWire = w; } y += 30 * s;
            { char full[300], mp[800]; snprintf(full, sizeof full, "Data/Models/%s", edModelRel(edModelSel)); edPath(mp, sizeof mp, full); bool has = FileExists(mp);
              if (aButton((Rectangle){14 * s, y, 146 * s, 28 * s}, edMVShowOrig ? "Show replacement" : "Show original", has)) { edMVShowOrig = !edMVShowOrig; edMVFor = -1; }
              if (aButton((Rectangle){168 * s, y, 146 * s, 28 * s}, "Remove replacement", has)) { remove(mp); char *d = strrchr(mp, '.'); if (d) { strcpy(d, ".mtl"); remove(mp); } edMVFor = -1; edSay("Replacement removed."); } y += 34 * s;
              if (aButton((Rectangle){14 * s, y, 300 * s, 28 * s}, "Export the original (to edit in Blender)", true)) { ensureDir("Exports"); char dst[800]; snprintf(dst, sizeof dst, "%s/%s", P("Exports"), GetFileName(edModels.paths[edModelSel]));
                  int len = 0; unsigned char *bb = LoadFileData(edModels.paths[edModelSel], &len); if (bb) { SaveFileData(dst, bb, len); UnloadFileData(bb); }
                  char sm[800], dm[800]; snprintf(sm, sizeof sm, "%s", edModels.paths[edModelSel]); char *d = strrchr(sm, '.'); if (d) strcpy(d, ".mtl"); snprintf(dm, sizeof dm, "%s", dst); d = strrchr(dm, '.'); if (d) strcpy(d, ".mtl");
                  if (FileExists(sm)) { bb = LoadFileData(sm, &len); if (bb) { SaveFileData(dm, bb, len); UnloadFileData(bb); } }
                  edSay(TextFormat("Exported to Exports/%s", GetFileName(dst))); } }
        } else {
            const char *lab[4] = {"Name", "Author", "Version", "Description"};
            for (int i = 0; i < 4; i++) { y += 28 * s; aTextBox((Rectangle){18 * s, y, 290 * s, 30 * s}, lab[i], &edTB[i]); y += 40 * s; }
            y += 10 * s; if (aButton((Rectangle){14 * s, y, 300 * s, 32 * s}, "Save mod", true)) { edSaveInfo(); if (edDirty) edSaveMap(); edSay("Mod saved. Enable it in the launcher's Mods tab."); } y += 40 * s;
            if (aButton((Rectangle){14 * s, y, 300 * s, 32 * s}, "Export .zip to share", true)) { edSaveInfo(); if (edDirty) edSaveMap(); ensureDir("Exports"); char zp[800]; snprintf(zp, sizeof zp, "%s/%s.zip", P("Exports"), edName);
                int k = edExportZip(zp); char b[160]; snprintf(b, sizeof b, k >= 0 ? "Exported %d files to Exports/%s.zip" : "Export failed.", k, edName); edSay(b); } y += 44 * s;
            aText("The game loads this folder mod directly:", 18 * s, y, 13, C_BONE2, false); y += 20 * s;
            aText(TextFormat("Mods/%s/", edName), 18 * s, y, 13, C_CYAN, false); y += 20 * s;
            aText("Its map replaces the town; players joining", 18 * s, y, 13, C_BONE2, false); y += 18 * s;
            aText("a multiplayer game need the same map mod.", 18 * s, y, 13, C_BONE2, false);
        }
        if (edTab == 2) edModelView((Rectangle){side.width, 0, W - side.width, H});
        if (edTab == 5) {   // item stats
            Rectangle vr = {side.width + 20 * s, 20 * s, fminf(640 * s, W - side.width - 40 * s), H - 40 * s}; aPanel(vr); ItemDef *d = &idef[edItemSel], *o = &idefBase[edItemSel];
            float x = vr.x + 24 * s, w = vr.width - 48 * s, yy = vr.y + 20 * s; bool ch = false;
            aText(d->name, x, yy, 24, C_BONE, true); yy += 34 * s; aText(d->desc ? d->desc : "", x, yy, 13, C_BONE2, false); yy += 30 * s;
            { float v = d->weight; if (edStep(x, yy, w, "Weight (kg)", &v, 0.05f, 0, 50, "%.2f")) { edPush(); d->weight = v; ch = true; } yy += 32 * s; }
            { float v = (float)d->stack; if (edStep(x, yy, w, "Stack size", &v, 1, 1, 999, "%.0f")) { edPush(); d->stack = (int)v; ch = true; } yy += 32 * s; }
            if (d->cat == CAT_MELEE) {
                { float v = d->dmg; if (edStep(x, yy, w, "Damage per hit", &v, 1, 0, 500, "%.0f")) { edPush(); d->dmg = v; ch = true; } yy += 32 * s; }
                { float v = d->cd; if (edStep(x, yy, w, "Swing time (s)", &v, 0.05f, 0.1f, 5, "%.2f")) { edPush(); d->cd = v; ch = true; } yy += 32 * s; }
                { float v = d->range; if (edStep(x, yy, w, "Reach (m)", &v, 0.1f, 0.5f, 6, "%.1f")) { edPush(); d->range = v; ch = true; } yy += 32 * s; }
                { float v = d->stam; if (edStep(x, yy, w, "Stamina per swing", &v, 1, 0, 100, "%.0f")) { edPush(); d->stam = v; ch = true; } yy += 32 * s; }
                { float v = d->knock; if (edStep(x, yy, w, "Knockback (m)", &v, 0.1f, 0, 10, "%.1f")) { edPush(); d->knock = v; ch = true; } yy += 32 * s; } }
            if (d->cat == CAT_BAG) { float v = (float)d->bag; if (edStep(x, yy, w, "Extra bag slots", &v, 1, 0, 30, "%.0f")) { edPush(); d->bag = (int)v; ch = true; } yy += 32 * s; }
            yy += 10 * s; if (aButton((Rectangle){x, yy, 220 * s, 30 * s}, "Back to the game's values", memcmp(d, o, sizeof *d) != 0)) { *d = *o; ch = true; }
            if (ch) edDirty = true;
            aText("Changes are saved with the mod (Data/items.ini). Multiplayer needs the same mod on both sides.", x, vr.y + vr.height - 30 * s, 12, C_BONE2, false);
        }
        if (edTab == 0 || edTab == 4 || edTab == 6) edPropsPanel(W, H);
        if (IsKeyPressed(KEY_F1)) help = !help;
        if (help) { Rectangle hb = {W / 2 - 300 * s, H / 2 - 230 * s, 600 * s, 460 * s}; aPanel(hb); float hy = hb.y + 20 * s;
            const char *hl[] = {"Camera: hold right mouse to look, WASD fly, Q/E down/up, Shift faster, wheel speed, Home resets the view",
                "Map: pick a layer, drag points (they bend as a curve within the drag radius), Ins add, Del delete",
                "Streets: N new street, Shift+Del removes it; dragged ends snap to roads, end on end joins (Alt: no snap); Split",
                "Build: Insert parts, drag a part to move it, arrows move along one axis, 2 turns, 3 sizes with the cubes",
                "Build: Ctrl+D duplicates, Del deletes; Turn: drag = yaw, Shift+drag = tilt, Ctrl+drag = lean (blocks)",
                "Items: change weight, stack, damage, swing time, reach, stamina, knockback, bag slots",
                "Textures / Models: pick one and drop a .png / .obj onto the window; the model viewer turns with the mouse",
                "Ctrl+Z undo, Ctrl+Y (or Ctrl+Shift+Z) redo, Ctrl+S save, F5 rebuild the town, F1 this help",
                "Right panel: type exact X/Y/Z, size and angles; set the snap steps; arrows and PgUp/PgDn nudge"};
            aTextC("Mod Maker help", W / 2, hy, 22, C_BONE, true); hy += 40 * s;
            for (int i = 0; i < 9; i++) { aText(hl[i], hb.x + 20 * s, hy, 13, C_BONE, false); hy += 34 * s; }
            if (aButton((Rectangle){W / 2 - 60 * s, hb.y + hb.height - 46 * s, 120 * s, 32 * s}, "Close", true)) help = false; }
        if (askQuit) { Rectangle qb = {W / 2 - 240 * s, H / 2 - 80 * s, 480 * s, 160 * s}; aPanel(qb);
            aTextC("Save your changes before closing?", W / 2, qb.y + 30 * s, 18, C_BONE, true);
            if (aButton((Rectangle){qb.x + 20 * s, qb.y + 100 * s, 140 * s, 34 * s}, "Save", true)) { edSaveMap(); EndDrawing(); break; }
            if (aButton((Rectangle){qb.x + 170 * s, qb.y + 100 * s, 140 * s, 34 * s}, "Don't save", true)) { EndDrawing(); break; }
            if (aButton((Rectangle){qb.x + 320 * s, qb.y + 100 * s, 140 * s, 34 * s}, "Cancel", true)) askQuit = false; }
        if (edMsgT > 0) { edMsgT -= dt; aTextC(edMsg, side.width + (W - side.width) / 2, H - 40 * s, 17, WHITE, true); }
        if (edTab == 0 && edSel >= 0) { Vector2 p = edGet(edLayer, edSel); char b[96]; snprintf(b, sizeof b, "%s #%d   x %.1f  z %.1f", edLayers[edLayer].kind == 5 ? edSpots[edSel].name : edLayers[edLayer].label, edSel, p.x, p.y); aText(b, side.width + 14 * s, 10 * s, 15, WHITE, true); }
        EndDrawing();
        if (getenv("KIRI_EDSNAP")) { static int fs = 0; if (++fs == 30) { TakeScreenshot("ed_snap.png"); break; } }
        if (getenv("KIRI_EDSHOT")) {   // test hook: a few edits and a picture of each tab
            static int fr2 = 0; fr2++;
            if (fr2 == 5) { edPos = V3(195, 45, 300); edYaw = 0.6f; edPitch = -0.7f; edLayer = 0; edSel = 170; }
            if (fr2 == 20) { TakeScreenshot("ed_map.png"); Vector2 p = edGet(0, 170); edSetP(0, 170, (Vector2){p.x - 6, p.y - 6}); edInsert(1, 10); edDelete(1, 12); edRebuild(); edSaveMap(); }
            if (fr2 == 30) TakeScreenshot("ed_map2.png");
            if (fr2 == 31) { edTab = 1; edTexSel = 3; } if (fr2 == 40) TakeScreenshot("ed_tex.png");
            if (fr2 == 41) edTab = 2; if (fr2 == 50) TakeScreenshot("ed_models.png");
            if (fr2 == 51) edTab = 3; if (fr2 == 60) { TakeScreenshot("ed_mod.png"); ensureDir("Exports"); char zp[800]; snprintf(zp, sizeof zp, "%s/%s.zip", P("Exports"), edName); TraceLog(LOG_INFO, "EDTEST export %d files", edExportZip(zp)); }
            if (fr2 == 62) { edTab = 4; edPos = V3(222, 12, 100); edYaw = 0.2f; edPitch = -0.45f; nMobj = 0; }
            if (fr2 == 66) { boInsert(MO_BLOCK); mobj[0].s = V3(6, 0.4f, 3); mobj[0].yaw = 30; mobj[0].pitch = 0; mobj[0].roll = 18; mobj[0].tex = T_BRICK; boInsert(MO_BLOCK); mobj[1].p.x += 4; mobj[1].col = boCols[3]; boInsert(MO_TREE); mobj[2].p.x -= 5; boInsert(MO_VENDING); mobj[3].p.z += 3; boSel = 0; boTool = 0; }
            if (fr2 == 72) TakeScreenshot("ed_build.png");
            if (fr2 == 73) { boTool = 2; } if (fr2 == 80) TakeScreenshot("ed_build2.png");
            if (fr2 == 81) { edSaveMap(); mobjLive = false; edRebuild(); mobjLive = true; } if (fr2 == 90) TakeScreenshot("ed_build3.png");
            if (fr2 == 91) { int before = nMobj; edUndo(false); TraceLog(LOG_INFO, "EDTEST undo parts %d -> %d", before, nMobj); edUndo(true); TraceLog(LOG_INFO, "EDTEST redo parts %d", nMobj);
                edTab = 2; for (int i = 0; i < (int)edModels.count; i++) if (strstr(edModels.paths[i], "revolver4_body") || strstr(edModels.paths[i], "revolver4.obj")) { edModelSel = i; break; } }
            if (fr2 == 100) TakeScreenshot("ed_models.png");
            if (fr2 == 101) { edTab = 5; edItemSel = IT_KATANA; idef[IT_KATANA].dmg += 10; } if (fr2 == 108) { TakeScreenshot("ed_items.png"); edSaveMap(); }
            if (fr2 == 109) { int n0 = DSTREETS_N; edNewStreet((Vector2){300, 150}); int ea = edSel; edNewStreet((Vector2){330, 150}); int sb = DSTREETS_N - 1;
                edSetP(1, ea, (Vector2){329, 151}); Vector2 q; int ts, te; bool hit = edRoadNearest(1, ea, edGet(1, ea), &q, &ts, &te);
                TraceLog(LOG_INFO, "EDTEST snap hit %d at %.1f %.1f street %d (new %d) end %d", hit, q.x, q.y, ts, sb, te);
                if (hit && ts >= 0 && te >= 0) { edSetP(1, ea, q); edJoinStreets(ea, ts, te); } int sj = DSTREETS_N - 1;
                TraceLog(LOG_INFO, "EDTEST join: streets %d -> %d, joined has %d points from %.0f,%.0f to %.0f,%.0f", n0 + 2, DSTREETS_N, dStreets[sj][1], edGet(1, dStreets[sj][0]).x, edGet(1, dStreets[sj][0]).y, edGet(1, dStreets[sj][0] + dStreets[sj][1] - 1).x, edGet(1, dStreets[sj][0] + dStreets[sj][1] - 1).y);
                edInsert(1, dStreets[sj][0]); edSplitStreet(dStreets[sj][0] + 1); TraceLog(LOG_INFO, "EDTEST split: streets %d", DSTREETS_N);
                Vector2 q2; hit = edRoadNearest(1, dStreets[DSTREETS_N - 1][0], (Vector2){236, 118}, &q2, &ts, &te); TraceLog(LOG_INFO, "EDTEST tee snap %d -> %.1f %.1f", hit, q2.x, q2.y); edTab = 0; edLayer = 1; edSel = dStreets[DSTREETS_N - 1][0]; edRebuild(); edPos = V3(321, 30, 120); edYaw = 0; edPitch = -0.9f;
                edSnapHit = true; edDragging = true; edSnapAt = (Vector2){330, 150}; edSnapEnd = 1; }
            if (fr2 == 109 + 0) {}
            if (fr2 == 118) { TakeScreenshot("ed_join.png"); edDragging = false; edSnapHit = false; }
            if (fr2 == 121) { if (!terrEdit) terrEdit = calloc((size_t)HF_N * HF_N, sizeof(float));   // showcase: a flattened field with a dozen presets
                for (int j = 0; j < HF_N; j++) for (int i = 0; i < HF_N; i++) { float x = HF_X0 + i * HF_S, z = HF_X0 + j * HF_S; if (x > 560 && x < 680 && z > 250 && z < 380 && !inCore(x, z)) terrEdit[j * HF_N + i] = SIDEH - hf[j * HF_N + i] - (terrEdit[j * HF_N + i]); }
                int pick[12] = {0, 5, 7, 16, 17, 28, 32, 35, 48, 55, 66, 71}; edTab = 6; boAuto = false;
                for (int k = 0; k < 12; k++) { edPos = V3(575 + (k % 4) * 28, 30, 265 + (k / 4) * 34); cam.position = edPos; cam.target = Vector3Add(edPos, V3(0, -1, 0.001f)); edPlaceBuilding(pick[k]); if (boSel >= 0) { mobj[boSel].p = V3(575 + (k % 4) * 28, SIDEH, 265 + (k / 4) * 34); mobj[boSel].yaw = (k % 3) * 15; } }
                edRebuild(); edPos = V3(620, 45, 215); edYaw = 0; edPitch = -0.5f; boSel = -1; }
            if (fr2 == 130) TakeScreenshot("ed_bld.png");
            if (fr2 == 131) { edPos = V3(590, 8, 250); edPitch = -0.12f; edYaw = 0.3f; } if (fr2 == 138) TakeScreenshot("ed_bld2.png");
            if (fr2 == 139) { edTab = 7; edTerTool = 0; edTerR = 25; edTerS = 30; edTerAt = V3(520, 0, 330); edTerHit = true; edPush(); for (int k = 0; k < 30; k++) edTerPaint(0.1f); genTerrain(); edPos = V3(470, 60, 260); edYaw = 0.6f; edPitch = -0.45f; }
            if (fr2 == 148) { TakeScreenshot("ed_ter.png"); edSaveMap(); }
            if (fr2 == 150) { TraceLog(LOG_INFO, "EDTEST done, map hash %08x, %d parts", mapHash(), nMobj); break; } }
    }
    CloseWindow(); return 0;
}
