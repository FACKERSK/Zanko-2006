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
    int s2 = edStreetOf(j); if (s2 < 0) return; int st = dStreets[s2][0], cnt = dStreets[s2][1];
    MapArr *a = mapArr("dStreetPts"); int n = mapRows(a);
    for (int r = st; r + cnt < n; r++) for (int c = 0; c < 2; c++) mapSet(a, r, c, mapGet(a, r + cnt, c));
    *a->n = n - cnt;
    for (int t = s2; t + 1 < DSTREETS_N; t++) for (int c = 0; c < 4; c++) dStreets[t][c] = dStreets[t + 1][c];
    DSTREETS_N_v--; for (int t = 0; t < DSTREETS_N; t++) if (dStreets[t][0] > st) dStreets[t][0] -= cnt;
    edSel = -1; edDirty = true;
}
static void edNewStreet(Vector2 at) {   // a short new street at the cursor, ready to be dragged out
    MapArr *a = mapArr("dStreetPts"); int n = mapRows(a); if (n + 2 >= a->cap || DSTREETS_N + 1 >= 676) { edSay("No room for more streets."); return; }
    mapSet(a, n, 0, at.x); mapSet(a, n, 1, at.y); mapSet(a, n + 1, 0, at.x + 12); mapSet(a, n + 1, 1, at.y); *a->n = n + 2;
    dStreets[DSTREETS_N][0] = n; dStreets[DSTREETS_N][1] = 2; dStreets[DSTREETS_N][2] = 80; dStreets[DSTREETS_N][3] = 0; DSTREETS_N_v++;
    edSel = n + 1; edDirty = true; edSay("New street: drag its end, Insert adds points.");
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
static bool edSaveMap(void) { edEnsureDirs("Data/map.zmap"); char p[800]; edPath(p, sizeof p, "Data/map.zmap"); edSaveInfo(); bool ok = mapSave(p); edDirty = false; return ok; }
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
    UnloadTexture(tex[i]); tex[i] = t; cube[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = t; if (statOk[i]) stat[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = t;
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
    { char mp[800]; edPath(mp, sizeof mp, "Data/map.zmap"); if (FileExists(mp)) mapLoadFile(mp); else mapReset(); }
    for (int i = 0; i < T_COUNT; i++) edReloadTexture(i);
    edRebuild();
    edModels = LoadDirectoryFilesEx(P("Data/Models"), ".obj", true); edModelsOk = true;
    float fogc[3] = {0.47f, 0.50f, 0.53f}; Color fogC = {120, 128, 135, 255};
    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f); aeroBegin(720); float s = aU, W = GetScreenWidth(), H = GetScreenHeight();
        if (IsFileDropped()) edDropped();
        Rectangle side = {0, 0, 330 * s, H}; Vector2 m = GetMousePosition(); bool inView = m.x > side.width && m.y > 44 * s;
        bool typing = false; for (int i = 0; i < 4; i++) typing |= edTB[i].active;
        // camera: hold the right mouse button to look, WASD to fly, Q/E down/up, Shift faster, wheel sets the speed
        if (edTab == 0) {
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && inView) { Vector2 d = GetMouseDelta(); edYaw -= d.x * 0.004f; edPitch = clampf(edPitch - d.y * 0.004f, -1.5f, 1.5f); }
            if (inView) edSpeed = clampf(edSpeed * (1 + GetMouseWheelMove() * 0.15f), 4, 300);
            if (!typing) { Vector3 f = {sinf(edYaw) * cosf(edPitch), sinf(edPitch), cosf(edYaw) * cosf(edPitch)}, r = {-cosf(edYaw), 0, sinf(edYaw)};
                float sp = edSpeed * dt * (IsKeyDown(KEY_LEFT_SHIFT) ? 4 : 1); Vector3 v = {0};
                if (IsKeyDown(KEY_W)) v = Vector3Add(v, f); if (IsKeyDown(KEY_S)) v = Vector3Subtract(v, f); if (IsKeyDown(KEY_D)) v = Vector3Add(v, r); if (IsKeyDown(KEY_A)) v = Vector3Subtract(v, r);
                if (IsKeyDown(KEY_E)) v.y += 1; if (IsKeyDown(KEY_Q)) v.y -= 1;
                edPos = Vector3Add(edPos, Vector3Scale(v, sp)); }
        }
        cam.position = edPos; cam.target = Vector3Add(edPos, V3(sinf(edYaw) * cosf(edPitch), sinf(edPitch), cosf(edYaw) * cosf(edPitch))); cam.up = V3(0, 1, 0); cam.fovy = 70; cam.projection = CAMERA_PERSPECTIVE;
        // picking and dragging points of the chosen layer
        int n = edCount(edLayer); int hover = -1;
        if (edTab == 0 && inView) { float bd = 14; for (int j = 0; j < n; j++) { Vector2 p = edGet(edLayer, j); Vector3 w = V3(p.x, edGroundY(p.x, p.y) + 0.4f, p.y);
                if (Vector3DotProduct(Vector3Subtract(w, edPos), Vector3Subtract(cam.target, edPos)) <= 0) continue;
                Vector2 sc = GetWorldToScreen(w, cam); float d = Vector2Distance(sc, m); if (d < bd) { bd = d; hover = j; } } }
        Ray ray = GetScreenToWorldRay(m, cam);
        if (edTab == 0 && inView && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (hover >= 0) { edSel = hover; Vector2 p = edGet(edLayer, hover); edDragY = edGroundY(p.x, p.y) + 0.4f; edDragStart = p; edDragging = true; edDragN = 0;
                int kind = edLayers[edLayer].kind; float R = (kind <= 2) ? edBrush : 0;
                for (int j = 0; j < n && edDragN < 4096; j++) { if (!edSameGroup(edLayer, j, hover)) continue; Vector2 q = edGet(edLayer, j); float d = Vector2Distance(q, p);
                    if (j != hover && (R <= 0 || d >= R)) continue; float w = j == hover ? 1 : 1 - d / R; w = w * w * (3 - 2 * w);
                    edDragIdx[edDragN] = j; edDragW[edDragN] = w; edDragOrig[edDragN] = q; edDragN++; } }
            else edSel = -1; }
        if (edDragging) {
            if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) edDragging = false;
            else if (fabsf(ray.direction.y) > 1e-4f) { float t = (edDragY - ray.position.y) / ray.direction.y; if (t > 0) { Vector2 hit = {ray.position.x + ray.direction.x * t, ray.position.z + ray.direction.z * t}, d = Vector2Subtract(hit, edDragStart);
                for (int k = 0; k < edDragN; k++) edSetP(edLayer, edDragIdx[k], Vector2Add(edDragOrig[k], Vector2Scale(d, edDragW[k]))); edDirty = true; } } }
        if (edTab == 0 && !typing) {
            if (IsKeyPressed(KEY_INSERT) && edSel >= 0) edInsert(edLayer, edSel);
            if (IsKeyPressed(KEY_DELETE) && edSel >= 0) { if (IsKeyDown(KEY_LEFT_SHIFT) && edLayers[edLayer].kind == 2) edDeleteStreet(edSel); else edDelete(edLayer, edSel); }
            if (IsKeyPressed(KEY_F5)) edRebuild();
            if (IsKeyPressed(KEY_N) && edLayers[edLayer].kind == 2 && fabsf(ray.direction.y) > 1e-4f) { float t = (SIDEH - ray.position.y) / ray.direction.y; if (t > 0) edNewStreet((Vector2){ray.position.x + ray.direction.x * t, ray.position.z + ray.direction.z * t}); }
        }
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) edSay(edSaveMap() ? "Saved." : "Could not save.");
        // ---- draw
        float amb = 0.82f * gs.bright, fogD = 0.006f, zero = 0, one = 1, snap[2] = {16384, 16384}; int nl = 0;
        SetShaderValue(sh, locFog, fogc, SHADER_UNIFORM_VEC3); SetShaderValue(sh, locView, &cam.position, SHADER_UNIFORM_VEC3); SetShaderValue(sh, locFlashOn, &zero, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, locMuzzle, &zero, SHADER_UNIFORM_FLOAT); SetShaderValue(sh, locAmb, &amb, SHADER_UNIFORM_FLOAT); SetShaderValue(sh, locFogD, &fogD, SHADER_UNIFORM_FLOAT);
        SetShaderValue(sh, locSnap, snap, SHADER_UNIFORM_VEC2); SetShaderValue(sh, locNL, &nl, SHADER_UNIFORM_INT); SetShaderValue(sh, locDesat, &zero, SHADER_UNIFORM_FLOAT); SetShaderValue(sh, locRage, &zero, SHADER_UNIFORM_FLOAT); (void)one;
        BeginDrawing(); ClearBackground(fogC);
        if (edTab == 0) {
            BeginMode3D(cam); drawWorld(cam.position, (float)GetTime()); rlDrawRenderBatchActive(); rlDisableDepthTest();
            for (int L = 0; L < ED_NL; L++) { const EdLayer *e = &edLayers[L]; int c = edCount(L); bool act = L == edLayer; Color lc = act ? e->col : aA(e->col, 90);
                if (e->kind == 5 && !act) continue;
                for (int j = 0; j < c; j++) { Vector2 p = edGet(L, j); Vector3 a = V3(p.x, edGroundY(p.x, p.y) + 0.4f, p.y);
                    bool link = (e->kind <= 2 && j + 1 < c && edSameGroup(L, j, j + 1)) || (e->kind == 4 && j % 2 == 0);
                    if (link) { Vector2 q = edGet(L, j + 1); DrawLine3D(a, V3(q.x, edGroundY(q.x, q.y) + 0.4f, q.y), lc); }
                    if (e->kind == 1 && j == c - 1 && c > 2) { Vector2 q = edGet(L, 0); DrawLine3D(a, V3(q.x, edGroundY(q.x, q.y) + 0.4f, q.y), lc); }
                    if (act && Vector3DistanceSqr(a, edPos) < 260 * 260) { float r = j == edSel ? 0.9f : j == hover ? 0.7f : 0.35f; DrawCube(a, r, r, r, j == edSel ? WHITE : lc); } } }
            if (edSel >= 0 && edLayers[edLayer].kind <= 2 && edSel < n) { Vector2 p = edGet(edLayer, edSel); DrawCircle3D(V3(p.x, edGroundY(p.x, p.y) + 0.5f, p.y), edBrush, V3(1, 0, 0), 90, aA(WHITE, 160)); }
            rlDrawRenderBatchActive(); rlEnableDepthTest(); EndMode3D();
            if (hover >= 0 && edLayers[edLayer].kind == 5) aText(edSpots[hover].name, m.x + 14, m.y - 6, 15, WHITE, true);
        }
        // ---- panel
        aPanel(side);
        float y = 14 * s; aText("Mod Maker", 18 * s, y, 22, C_BONE, true); y += 30 * s;
        { char b[96]; snprintf(b, sizeof b, "Project: %s%s", edName, edDirty ? "  (unsaved)" : ""); aText(b, 18 * s, y, 13, C_CYAN, false); } y += 26 * s;
        const char *tabs[4] = {"Map", "Textures", "Models", "Mod"};
        for (int i = 0; i < 4; i++) if (aButton((Rectangle){(14 + i * 76) * s, y, 72 * s, 30 * s}, tabs[i], edTab != i)) { edTab = i; edTexSel = edTexSel; }
        y += 44 * s;
        if (edTab == 0) {
            aText("Layer", 18 * s, y, 15, C_BONE2, true); y += 22 * s;
            for (int L = 0; L < ED_NL; L++) { if (aButton((Rectangle){(14 + (L % 2) * 152) * s, y, 148 * s, 25 * s}, edLayers[L].label, L != edLayer)) { edLayer = L; edSel = -1; } if (L % 2) y += 28 * s; }
            if (ED_NL % 2) y += 28 * s; y += 26 * s; edBrush = aSlider((Rectangle){18 * s, y, 290 * s, 18 * s}, "Soft drag radius (m)", edBrush, 0, 60, "%.0f"); y += 30 * s;
            if (aButton((Rectangle){14 * s, y, 146 * s, 30 * s}, "Add point", edSel >= 0 && edLayers[edLayer].kind < 4)) edInsert(edLayer, edSel);
            if (aButton((Rectangle){168 * s, y, 146 * s, 30 * s}, "Delete point", edSel >= 0 && edLayers[edLayer].kind < 4)) edDelete(edLayer, edSel); y += 36 * s;
            if (aButton((Rectangle){14 * s, y, 146 * s, 30 * s}, "Rebuild (F5)", true)) edRebuild();
            if (aButton((Rectangle){168 * s, y, 146 * s, 30 * s}, "Save (Ctrl+S)", true)) edSay(edSaveMap() ? "Saved." : "Could not save."); y += 36 * s;
            if (aButton((Rectangle){14 * s, y, 300 * s, 28 * s}, "Reset to the built-in map", true)) { mapReset(); edSel = -1; edDirty = true; edRebuild(); } y += 36 * s;
            if (edLayers[edLayer].kind == 2 && edSel >= 0) { int s2 = edStreetOf(edSel); if (s2 >= 0) { char b[96];
                snprintf(b, sizeof b, "Street %d: width %.1f m, %s", s2, dStreets[s2][2] / 10.0f, (const char *[]){"street", "avenue (lines)", "stairs", "bridge"}[dStreets[s2][3] & 3]); aText(b, 18 * s, y, 13, C_BONE, false); y += 22 * s;
                if (aButton((Rectangle){14 * s, y, 70 * s, 26 * s}, "Wider", true)) { dStreets[s2][2] += 5; edDirty = true; }
                if (aButton((Rectangle){90 * s, y, 70 * s, 26 * s}, "Narrower", dStreets[s2][2] > 30)) { dStreets[s2][2] -= 5; edDirty = true; }
                if (aButton((Rectangle){166 * s, y, 148 * s, 26 * s}, "Change type", true)) { dStreets[s2][3] = (dStreets[s2][3] + 1) % 4; edDirty = true; } y += 32 * s; } }
            aText("RMB look  WASD fly  Q/E down/up  wheel speed", 18 * s, H - 66 * s, 12, C_BONE2, false);
            aText("LMB drag a point  Ins add  Del delete", 18 * s, H - 50 * s, 12, C_BONE2, false);
            aText("Streets: N new street, Shift+Del remove street", 18 * s, H - 34 * s, 12, C_BONE2, false);
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
            aText("Pick a model, then drop an .obj (and its .mtl) onto the window.", 18 * s, y, 12, C_BONE2, false); y += 24 * s;
            int rows = (int)((H - y - 20 * s) / (25 * s)); int cnt = (int)edModels.count;
            if (GetMouseWheelMove() != 0 && m.x < side.width) edScrollM = (int)clampf(edScrollM - GetMouseWheelMove() * 3, 0, fmaxf(0, cnt - rows));
            for (int i = edScrollM; i < cnt && i < edScrollM + rows; i++) { const char *rel = strstr(edModels.paths[i], "Data/Models/"); rel = rel ? rel + 12 : GetFileName(edModels.paths[i]);
                char mp[800], full[300]; snprintf(full, sizeof full, "Data/Models/%s", rel); edPath(mp, sizeof mp, full);
                char b[120]; snprintf(b, sizeof b, "%s%s", rel, FileExists(mp) ? "  *" : ""); if (aButton((Rectangle){14 * s, y, 300 * s, 23 * s}, b, i != edModelSel)) edModelSel = i; y += 25 * s; }
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
        if (edMsgT > 0) { edMsgT -= dt; aTextC(edMsg, side.width + (W - side.width) / 2, H - 40 * s, 17, WHITE, true); }
        if (edTab == 0 && edSel >= 0) { Vector2 p = edGet(edLayer, edSel); char b[96]; snprintf(b, sizeof b, "%s #%d   x %.1f  z %.1f", edLayers[edLayer].kind == 5 ? edSpots[edSel].name : edLayers[edLayer].label, edSel, p.x, p.y); aText(b, side.width + 14 * s, 10 * s, 15, WHITE, true); }
        EndDrawing();
        if (getenv("KIRI_EDSHOT")) {   // test hook: a few edits and a picture of each tab
            static int fr2 = 0; fr2++;
            if (fr2 == 5) { edPos = V3(195, 45, 300); edYaw = 0.6f; edPitch = -0.7f; edLayer = 0; edSel = 170; }
            if (fr2 == 20) { TakeScreenshot("ed_map.png"); Vector2 p = edGet(0, 170); edSetP(0, 170, (Vector2){p.x - 6, p.y - 6}); edInsert(1, 10); edDelete(1, 12); edRebuild(); edSaveMap(); }
            if (fr2 == 30) TakeScreenshot("ed_map2.png");
            if (fr2 == 31) { edTab = 1; edTexSel = 3; } if (fr2 == 40) TakeScreenshot("ed_tex.png");
            if (fr2 == 41) edTab = 2; if (fr2 == 50) TakeScreenshot("ed_models.png");
            if (fr2 == 51) edTab = 3; if (fr2 == 60) { TakeScreenshot("ed_mod.png"); ensureDir("Exports"); char zp[800]; snprintf(zp, sizeof zp, "%s/%s.zip", P("Exports"), edName); TraceLog(LOG_INFO, "EDTEST export %d files", edExportZip(zp)); }
            if (fr2 == 62) { TraceLog(LOG_INFO, "EDTEST done, map hash %08x", mapHash()); break; } }
    }
    CloseWindow(); return 0;
}
