// zk_cheat.h — the cheat menu. Off unless an enabled mod provides cheats.ini (Mods/CheatMenu.zip).
// cheats.ini (in the mod's root): key=F8 opens the menu; god=1, stamina=1, ... set what is on when a game starts.

static const char *chtKeyNames[12] = {"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F12", "INSERT"};
static const int chtKeyCodes[12] = {KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F12, KEY_INSERT};
static bool chtDefault[CH_N]; static float chtSpeedDefault = 1;
static const char *chtIni[CH_N] = {"god", "stamina", "needs", "ammo", "unbreakable", "onehit", "noclip", "peaceful", "freezetime", "light"};
static const char *chtLabel[CH_N] = {"God mode (no damage, no injuries)", "Infinite stamina", "No hunger, thirst, cold, tiredness or fear",
    "Infinite revolver ammo", "Weapons never wear out", "One-hit kills", "Noclip (Space up, Ctrl down, Shift fast)",
    "Peaceful (no creatures)", "Freeze the clock", "Flashlight battery never runs out"};

static void cheatsLoad(void) {   // called after loadMods(): the top enabled mod with cheats.ini switches the menu on
    char path[700]; cht.avail = false; cht.key = KEY_F8; snprintf(cht.mod, sizeof cht.mod, "%s", "");
    if (!modFind("cheats.ini", path, sizeof path)) return;
    char *txt = LoadFileText(path); if (!txt) return;
    cht.avail = true;
    for (int i = 0; i < nMods; i++) { char p2[700]; snprintf(p2, sizeof p2, "%s/cheats.ini", modDir[i]); if (FileExists(p2)) { snprintf(cht.mod, sizeof cht.mod, "%s", modName[i]); break; } }
    for (char *line = strtok(txt, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        while (*line == ' ' || *line == '\t') line++;
        if (*line == '#' || *line == ';' || !*line) continue;
        char *eq = strchr(line, '='); if (!eq) continue; *eq = 0; char *k = line, *v = eq + 1;
        for (char *e = eq - 1; e >= k && (*e == ' ' || *e == '\t'); e--) *e = 0;
        while (*v == ' ' || *v == '\t') v++;
        if (!strcmp(k, "key")) { for (int i = 0; i < 12; i++) if (!strcasecmp(v, chtKeyNames[i])) cht.key = chtKeyCodes[i]; continue; }
        if (!strcmp(k, "speed")) { chtSpeedDefault = clampf((float)atof(v), 0.5f, 4); continue; }
        for (int i = 0; i < CH_N; i++) if (!strcmp(k, chtIni[i])) chtDefault[i] = atoi(v) != 0;
    }
    UnloadFileText(txt);
    TraceLog(LOG_INFO, "CHEATS: menu available from mod '%s'", cht.mod);
}
static void cheatsReset(void) {   // a new game or a loaded save starts with the mod's defaults
    for (int i = 0; i < CH_N; i++) cht.on[i] = cht.avail && chtDefault[i];
    cht.speed = cht.avail ? chtSpeedDefault : 1; cht.menu = false; cht.frozenTod = pl.tod;
}
static void cheatHeal(void) {
    pl.hp = 100; pl.wounds = 0; pl.bleedAcc = 0; pl.fracture = 0; pl.fracT = 0; pl.sick = 0; pl.infect = 0; pl.woundT = 0; pl.vomitT = 0; pl.turned = false;
    pl.stam = 100; pl.exhausted = false; pl.food = 100; pl.water = 100; pl.warmth = 100; pl.rest = 100; pl.sanity = 100; pl.boredom = 0; pl.corrupt = 0;
}
static void cheatTick(float dt) {   // after the survival tick: whatever is switched on holds every frame
    (void)dt;
    if (!cht.avail) return;
    if (cht.on[CH_GOD]) { pl.hp = 100; pl.wounds = 0; pl.bleedAcc = 0; pl.fracture = 0; pl.sick = 0; pl.infect = 0; pl.turned = false; }
    if (cht.on[CH_STAM]) { pl.stam = 100; pl.exhausted = false; }
    if (cht.on[CH_NEEDS]) { pl.food = 100; pl.water = 100; pl.warmth = 100; pl.rest = 100; pl.sanity = 100; pl.boredom = 0; }
    if (cht.on[CH_AMMO] && pl.weapon == 1) pl.mag = 6;
    if (cht.on[CH_WEAR] && pl.hands.t >= 0) pl.hands.w = 0;
    if (cht.on[CH_PEACE]) for (int i = 0; i < nCr; i++) crs[i].on = false;
    if (cht.on[CH_TIME]) pl.tod = cht.frozenTod; else cht.frozenTod = pl.tod;
    if (cht.on[CH_LIGHT]) pl.batt = 100;
}
static void cheatTeleport(float x, float z) {
    float y = fmaxf(terrainH(x, z), cityLand(x, z) ? SIDEH : -1e9f);
    pl.pos = V3(x, y + 0.3f, z); pl.vy = 0;
    float g = groundAt(V3(x, y + 30, z), 0.3f, 40); if (g > -1e8f) pl.pos.y = g + 0.05f;   // stand on whatever is there (a roof, a floor)
}
static void cheatGive(const int *ids, const int *ns, int n, const char *what) {
    int left = 0; for (int i = 0; i < n; i++) left += invAdd(ids[i], ns[i]);
    char b[96]; snprintf(b, sizeof b, left ? "%s (some did not fit)" : "%s", what); say(b);
}
static void cheatMenu(float t) {
    p3Begin(t); p3FitMargin = 8 * aU;
    Rectangle p = p3Panel(5, 0, 0, 700, 640, true); float s = aU;
    char title[120]; snprintf(title, sizeof title, "Cheats  -  %s", cht.mod[0] ? cht.mod : "mod");
    if (aWindow(p, title)) { cht.menu = false; DisableCursor(); }
    float x = p.x + 30 * s, y = p.y + 62 * s;
    aText("Toggles", x, y, 16, C_CYAN, true); y += 30 * s;
    for (int i = 0; i < CH_N; i++) {
        bool v = aToggle(x, y, chtLabel[i], cht.on[i]);
        if (v != cht.on[i]) { cht.on[i] = v; if (i == CH_TIME) cht.frozenTod = pl.tod; if (i == CH_NOCLIP && !v) pl.vy = 0; }
        y += 32 * s; }
    y += 34 * s;
    cht.speed = aSlider((Rectangle){x, y, 320 * s, 10 * s}, "Move speed", cht.speed, 0.5f, 4, "x%.1f");
    y += 40 * s;
    char st[160]; snprintf(st, sizeof st, "Day %d  %02d:%02d   pos %.0f, %.0f, %.0f", pl.day, (int)pl.tod, (int)(fmodf(pl.tod, 1) * 60), pl.pos.x, pl.pos.y, pl.pos.z);
    aText(st, x, y, 13, C_BONE2, false);
    float bx = p.x + 400 * s, bw = 270 * s, bh = 32 * s, by = p.y + 62 * s, g2 = 38 * s;
    aText("Actions", bx, by, 16, C_CYAN, true); by += 30 * s;
    if (aButton((Rectangle){bx, by, bw, bh}, "Heal fully", true)) { cheatHeal(); say("Healed."); } by += g2;
    if (aButton((Rectangle){bx, by, bw, bh}, "Kill creatures within 60 m", true)) { int k = 0;
        for (int i = 0; i < nCr; i++) { Cr *c = &crs[i]; if (!c->on || c->state == CS_DEAD || Vector3Distance(c->pos, pl.pos) > 60) continue; c->hp = 0; c->state = CS_DEAD; c->deadT = 0; k++; }
        char b[64]; snprintf(b, sizeof b, "%d creatures put down.", k); say(b); } by += g2;
    float hw = (bw - 8 * s) / 2;
    if (aButton((Rectangle){bx, by, hw, bh}, "Morning", true)) { pl.tod = 7; cht.frozenTod = 7; }
    if (aButton((Rectangle){bx + hw + 8 * s, by, hw, bh}, "Noon", true)) { pl.tod = 12; cht.frozenTod = 12; } by += g2;
    if (aButton((Rectangle){bx, by, hw, bh}, "Dusk", true)) { pl.tod = 18.5f; cht.frozenTod = 18.5f; }
    if (aButton((Rectangle){bx + hw + 8 * s, by, hw, bh}, "Midnight", true)) { pl.tod = 0; cht.frozenTod = 0; } by += g2;
    if (aButton((Rectangle){bx, by, bw, bh}, "Teleport to last map marker", nMark > 0)) { cheatTeleport(marks[nMark - 1].x, marks[nMark - 1].y); cht.menu = false; DisableCursor(); } by += g2;
    if (aButton((Rectangle){bx, by, bw, bh}, "Teleport to the station", true)) { cheatTeleport(STN_X, STN_Z); cht.menu = false; DisableCursor(); } by += g2 + 10 * s;
    aText("Give", bx, by, 16, C_CYAN, true); by += 30 * s;
    if (aButton((Rectangle){bx, by, hw, bh}, "Katana", true)) { int a[] = {IT_KATANA}, n[] = {1}; cheatGive(a, n, 1, "Katana."); }
    if (aButton((Rectangle){bx + hw + 8 * s, by, hw, bh}, "Revolver", true)) { int a[] = {IT_REVOLVER, IT_AMMO}, n[] = {1, 24}; cheatGive(a, n, 2, "Revolver and 24 rounds."); } by += g2;
    if (aButton((Rectangle){bx, by, hw, bh}, "Medicine", true)) { int a[] = {IT_BANDAGE, IT_SPLINT, IT_DISINFECT, IT_PAINKILLER}, n[] = {5, 2, 2, 3}; cheatGive(a, n, 4, "Medicine."); }
    if (aButton((Rectangle){bx + hw + 8 * s, by, hw, bh}, "Food & water", true)) { int a[] = {IT_CANNED, IT_WATER, IT_FOOD}, n[] = {5, 5, 5}; cheatGive(a, n, 3, "Food and water."); } by += g2;
    if (aButton((Rectangle){bx, by, hw, bh}, "Building kit", true)) { int a[] = {IT_HAMMER, IT_PLANKS, IT_NAILS}, n[] = {1, 20, 40}; cheatGive(a, n, 3, "Hammer, planks and nails."); }
    if (aButton((Rectangle){bx + hw + 8 * s, by, hw, bh}, "Ofuda & salt", true)) { int a[] = {IT_OFUDA, IT_SALT}, n[] = {5, 5}; cheatGive(a, n, 2, "Ofuda and salt."); } by += g2;
    if (aButton((Rectangle){bx, by, hw, bh}, "Hiking pack", true)) { int a[] = {IT_HIKEPACK}, n[] = {1}; cheatGive(a, n, 1, "Hiking pack."); }
    if (aButton((Rectangle){bx + hw + 8 * s, by, hw, bh}, "Batteries", true)) { int a[] = {IT_BATTERY}, n[] = {4}; cheatGive(a, n, 1, "Batteries."); } by += g2;
    char kb[64]; snprintf(kb, sizeof kb, "[%s] or Esc closes", chtKeyNames[0]);
    for (int i = 0; i < 12; i++) if (chtKeyCodes[i] == cht.key) snprintf(kb, sizeof kb, "[%s] or Esc closes. The game is paused.", chtKeyNames[i]);
    aText(kb, p.x + 30 * s, p.y + p.height - 34 * s, 13, C_BONE2, false);
    p3End(5);
    p3Render(pl.stage); p3FitMargin = 0;
}
