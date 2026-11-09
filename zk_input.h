// zk_input.h — every player action goes through here: two rebindable keys / mouse buttons per action plus one gamepad
// button, saved in Data/keybinds.ini. Gamepad: left stick moves, right stick looks; in menus the left stick moves the
// cursor and A clicks, B / Start go back.
enum { A_FWD, A_BACK, A_LEFT, A_RIGHT, A_JUMP, A_SPRINT, A_CROUCH, A_USE, A_ATTACK, A_GUARD, A_RELOAD, A_INSPECT, A_FLASH,
       A_INV, A_MAP, A_HOLSTER, A_BARR, A_BUILD, A_SLEEP, A_Q1, A_Q2, A_Q3, A_Q4, A_Q5, A_Q6, A_N };
static const char *actName[A_N] = {"Move forward", "Move back", "Move left", "Move right", "Jump / climb / vault", "Sprint", "Sneak",
    "Use / hold to search", "Attack / fire / place", "Guard / parry", "Reload / next build piece", "Inspect", "Flashlight", "Inventory", "Map",
    "Holster (fists)", "Barricade / repair", "Build mode", "Sleep / wake", "Quick slot 1", "Quick slot 2", "Quick slot 3", "Quick slot 4", "Quick slot 5", "Quick slot 6"};
static const char *actIni[A_N] = {"forward", "back", "left", "right", "jump", "sprint", "sneak", "use", "attack", "guard", "reload", "inspect", "flashlight",
    "inventory", "map", "holster", "barricade", "build", "sleep", "quick1", "quick2", "quick3", "quick4", "quick5", "quick6"};
#define BM(b) (600 + (b))   // mouse buttons are stored as 600 + raylib button
static const int actDefK[A_N][2] = {{KEY_W, KEY_UP}, {KEY_S, KEY_DOWN}, {KEY_A, KEY_LEFT}, {KEY_D, KEY_RIGHT}, {KEY_SPACE, 0}, {KEY_LEFT_SHIFT, 0}, {KEY_LEFT_CONTROL, 0},
    {KEY_E, 0}, {BM(0), 0}, {BM(1), 0}, {KEY_R, 0}, {KEY_I, 0}, {KEY_F, 0}, {KEY_TAB, 0}, {KEY_M, 0}, {KEY_H, 0}, {KEY_G, 0}, {KEY_B, 0}, {KEY_Z, 0},
    {KEY_ONE, 0}, {KEY_TWO, 0}, {KEY_THREE, 0}, {KEY_FOUR, 0}, {KEY_FIVE, 0}, {KEY_SIX, 0}};
static const int actDefP[A_N] = {0, 0, 0, 0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, GAMEPAD_BUTTON_LEFT_THUMB, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
    GAMEPAD_BUTTON_RIGHT_FACE_LEFT, GAMEPAD_BUTTON_RIGHT_TRIGGER_2, GAMEPAD_BUTTON_LEFT_TRIGGER_2, GAMEPAD_BUTTON_RIGHT_TRIGGER_1, GAMEPAD_BUTTON_LEFT_FACE_DOWN,
    GAMEPAD_BUTTON_LEFT_TRIGGER_1, GAMEPAD_BUTTON_RIGHT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_LEFT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
    GAMEPAD_BUTTON_MIDDLE_LEFT, GAMEPAD_BUTTON_RIGHT_THUMB, 0, 0, 0, 0, 0, 0};
static int bindK[A_N][2], bindP[A_N];
static float padLookSens = 1.0f; static bool padInvertY = false, padVibe = true;

// ---- gamepad state, sampled once per frame
static int padId = -1; static bool padNow[20], padPrev[20], padActive = false, padSprintLatch = false;
static Vector2 padMoveV = {0, 0}, padLookV = {0, 0};
static Vector2 padDz(float x, float y) {   // radial dead zone, rescaled so the edge of the dead zone is 0
    float m = sqrtf(x * x + y * y); if (m < 0.18f) return (Vector2){0, 0};
    float k = fminf(1, (m - 0.18f) / 0.82f) / m; return (Vector2){x * k, y * k};
}
static void inputUpdate(void) {
    memcpy(padPrev, padNow, sizeof padNow); memset(padNow, 0, sizeof padNow); padMoveV = padLookV = (Vector2){0, 0};
    padId = -1; for (int i = 0; i < 4; i++) if (IsGamepadAvailable(i)) { padId = i; break; }
    if (padId < 0) { padActive = false; aPadClick = aPadDown = false; return; }
    for (int b = 1; b < 18; b++) padNow[b] = IsGamepadButtonDown(padId, b);
    float lt = GetGamepadAxisMovement(padId, GAMEPAD_AXIS_LEFT_TRIGGER), rt = GetGamepadAxisMovement(padId, GAMEPAD_AXIS_RIGHT_TRIGGER);
    if (lt > 0.3f) padNow[GAMEPAD_BUTTON_LEFT_TRIGGER_2] = true;   // triggers are axes (-1 at rest on most drivers)
    if (rt > 0.3f) padNow[GAMEPAD_BUTTON_RIGHT_TRIGGER_2] = true;
    padMoveV = padDz(GetGamepadAxisMovement(padId, GAMEPAD_AXIS_LEFT_X), GetGamepadAxisMovement(padId, GAMEPAD_AXIS_LEFT_Y));
    padLookV = padDz(GetGamepadAxisMovement(padId, GAMEPAD_AXIS_RIGHT_X), GetGamepadAxisMovement(padId, GAMEPAD_AXIS_RIGHT_Y));
    bool any = Vector2Length(padMoveV) > 0 || Vector2Length(padLookV) > 0; for (int b = 1; b < 18; b++) any |= padNow[b];
    if (any) padActive = true;
    Vector2 md = GetMouseDelta(); if (fabsf(md.x) + fabsf(md.y) > 3 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) padActive = false;
    if (Vector2Length(padMoveV) < 0.3f) padSprintLatch = false;
    // menus: the left stick drives the mouse cursor, A clicks
    aPadClick = aPadDown = false;
    if (!IsCursorHidden()) {
        float dt = fminf(GetFrameTime(), 0.05f), sp = 900 * (GetScreenHeight() / 720.0f);
        Vector2 v = padMoveV;
        if (Vector2Length(v) > 0) { Vector2 m = GetMousePosition(); float c = Vector2Length(v);
            m.x = clampf(m.x + v.x * c * sp * dt, 0, (float)GetScreenWidth() - 1); m.y = clampf(m.y + v.y * c * sp * dt, 0, (float)GetScreenHeight() - 1); SetMousePosition((int)m.x, (int)m.y); }
        aPadClick = padNow[GAMEPAD_BUTTON_RIGHT_FACE_DOWN] && !padPrev[GAMEPAD_BUTTON_RIGHT_FACE_DOWN]; aPadDown = padNow[GAMEPAD_BUTTON_RIGHT_FACE_DOWN];
    }
}
static bool padP(int b) { return b > 0 && b < 20 && padNow[b] && !padPrev[b]; }
// back / pause: Esc, Start, or B while a cursor is up
static bool escP(void) { return KP(KEY_ESCAPE) || padP(GAMEPAD_BUTTON_MIDDLE_RIGHT) || (!IsCursorHidden() && padP(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)); }

static bool inputLocked;   // something else has the controls (insanity)
static bool bindDown(int c) { if (c <= 0) return false; if (c >= 600) { int b = c - 600; return vin.on ? (b == 0 && vin.lmb) : IsMouseButtonDown(b); } return vin.on ? vin.down[c] : IsKeyDown(c); }
static bool bindPressed(int c) { if (c <= 0) return false; if (c >= 600) { int b = c - 600; return vin.on ? (b == 0 && vin.lmbP) : IsMouseButtonPressed(b); } return vin.on ? vin.pressed[c] : IsKeyPressed(c); }
static bool actD(int a) {
    if (inputLocked) return false;
    if (bindDown(bindK[a][0]) || bindDown(bindK[a][1])) return true;
    if (vin.on || padId < 0) return false;
    if (a == A_SPRINT && padSprintLatch) return true;
    return bindP[a] > 0 && padNow[bindP[a]];
}
static bool sprintLatch = false, sneakLatch = false;   // "Toggle" sprint / sneak in Settings > Gameplay
static bool actHeld(int a) {   // sprint and sneak, honouring the hold / toggle setting
    if (a == A_SPRINT && gs.sprintTgl) return sprintLatch || (padId >= 0 && padSprintLatch);
    if (a == A_CROUCH && gs.sneakTgl) return sneakLatch;
    return actD(a);
}
static bool actP(int a) {
    if (inputLocked) return false;
    if (bindPressed(bindK[a][0]) || bindPressed(bindK[a][1])) return true;
    if (vin.on || padId < 0) return false;
    return padP(bindP[a]);
}
// analog movement: x right, y forward (keys give -1 / 0 / 1, the stick anything between)
static Vector2 actMove(void) {
    if (inputLocked) return (Vector2){0, 0};
    Vector2 m = {(actD(A_RIGHT) ? 1.0f : 0) - (actD(A_LEFT) ? 1.0f : 0), (actD(A_FWD) ? 1.0f : 0) - (actD(A_BACK) ? 1.0f : 0)};
    if (!vin.on && IsCursorHidden()) { m.x += padMoveV.x; m.y -= padMoveV.y; }
    if (bindP[A_SPRINT] > 0 && padP(bindP[A_SPRINT])) padSprintLatch = !padSprintLatch;
    if (gs.sprintTgl && (bindPressed(bindK[A_SPRINT][0]) || bindPressed(bindK[A_SPRINT][1]))) { sprintLatch = !sprintLatch; if (sprintLatch) sneakLatch = false; }
    if (gs.sneakTgl && (bindPressed(bindK[A_CROUCH][0]) || bindPressed(bindK[A_CROUCH][1]) || padP(bindP[A_CROUCH]))) { sneakLatch = !sneakLatch; if (sneakLatch) sprintLatch = false; }
    if (Vector2Length(m) < 0.1f && fabsf(padMoveV.x) + fabsf(padMoveV.y) < 0.1f) sprintLatch = false;   // stopping ends a toggled sprint
    return m;
}
// the right stick, as mouse pixels this frame (so mouse sensitivity and aim lag apply the same way)
static Vector2 padLookPx(void) {
    if (vin.on || padId < 0 || !IsCursorHidden()) return (Vector2){0, 0};
    float dt = fminf(GetFrameTime(), 0.05f), sx = padLookV.x * fabsf(padLookV.x), sy = padLookV.y * fabsf(padLookV.y);   // squared response: fine aim near the centre
    float px = 1500.0f * padLookSens * dt * (0.0022f / fmaxf(cfg.sens, 0.0004f));
    return (Vector2){sx * px, sy * px * (padInvertY ? -1 : 1) * 0.75f};
}

// ---- names
static const char *keyName(int c) {
    static char b[4][24]; static int k = 0; k = (k + 1) & 3; char *o = b[k];
    if (c <= 0) return "-";
    if (c >= 600) { const char *mn[] = {"LMB", "RMB", "MMB", "Mouse 4", "Mouse 5", "Mouse 6", "Mouse 7"}; return c - 600 < 7 ? mn[c - 600] : "Mouse"; }
    if (c >= KEY_A && c <= KEY_Z) { o[0] = (char)c; o[1] = 0; return o; }
    if (c >= KEY_ZERO && c <= KEY_NINE) { o[0] = (char)c; o[1] = 0; return o; }
    if (c >= KEY_F1 && c <= KEY_F12) { snprintf(o, 24, "F%d", c - KEY_F1 + 1); return o; }
    if (c >= KEY_KP_0 && c <= KEY_KP_9) { snprintf(o, 24, "Num %d", c - KEY_KP_0); return o; }
    switch (c) {
    case KEY_SPACE: return "Space"; case KEY_LEFT_SHIFT: return "L Shift"; case KEY_RIGHT_SHIFT: return "R Shift"; case KEY_LEFT_CONTROL: return "L Ctrl";
    case KEY_RIGHT_CONTROL: return "R Ctrl"; case KEY_LEFT_ALT: return "L Alt"; case KEY_RIGHT_ALT: return "R Alt"; case KEY_TAB: return "Tab"; case KEY_ENTER: return "Enter";
    case KEY_BACKSPACE: return "Backspace"; case KEY_UP: return "Up"; case KEY_DOWN: return "Down"; case KEY_LEFT: return "Left"; case KEY_RIGHT: return "Right";
    case KEY_CAPS_LOCK: return "Caps"; case KEY_GRAVE: return "`"; case KEY_MINUS: return "-"; case KEY_EQUAL: return "="; case KEY_LEFT_BRACKET: return "[";
    case KEY_RIGHT_BRACKET: return "]"; case KEY_BACKSLASH: return "\\"; case KEY_SEMICOLON: return ";"; case KEY_APOSTROPHE: return "'"; case KEY_COMMA: return ",";
    case KEY_PERIOD: return "."; case KEY_SLASH: return "/"; case KEY_INSERT: return "Insert"; case KEY_DELETE: return "Delete"; case KEY_HOME: return "Home";
    case KEY_END: return "End"; case KEY_PAGE_UP: return "PgUp"; case KEY_PAGE_DOWN: return "PgDn";
    }
    snprintf(o, 24, "Key %d", c); return o;
}
static const char *padName(int b) {
    static const char *n[20] = {"-", "D-pad Up", "D-pad Right", "D-pad Down", "D-pad Left", "Y", "B", "A", "X", "LB", "LT", "RB", "RT", "Back", "Guide", "Start", "L3", "R3", "-", "-"};
    return b >= 0 && b < 20 ? n[b] : "-";
}
// the label for an action in on-screen hints: the first key, or the pad button while the pad is in use
static const char *actKey(int a) { if (padActive && bindP[a] > 0) return padName(bindP[a]); int k = bindK[a][0] > 0 ? bindK[a][0] : bindK[a][1]; return k > 0 ? keyName(k) : bindP[a] > 0 ? padName(bindP[a]) : "unbound"; }

// on-screen hints are written with the default keys; show the player's own bindings instead
static void keySubst(char *buf, size_t n) {
    static const struct { const char *from; int a; const char *fmt; } R[] = {{"[hold E]", A_USE, "[hold %s]"}, {"[E]", A_USE, "[%s]"}, {"press E", A_USE, "press %s"},
        {"[Z]", A_SLEEP, "[%s]"}, {"[SPACE]", A_JUMP, "[%s]"}, {"[G]", A_BARR, "[%s]"}};
    char out[512]; size_t o = 0; const char *p = buf;
    while (*p && o < sizeof out - 1) {
        bool hit = false;
        for (int i = 0; i < (int)(sizeof R / sizeof R[0]); i++) { size_t L = strlen(R[i].from);
            if (!strncmp(p, R[i].from, L)) { char t[48]; snprintf(t, sizeof t, R[i].fmt, actKey(R[i].a)); for (char *q = t; *q && o < sizeof out - 1; q++) out[o++] = *q; p += L; hit = true; break; } }
        if (!hit) out[o++] = *p++;
    }
    out[o] = 0; snprintf(buf, n, "%s", out);
}
// ---- load / save
static void keysDefaults(void) { for (int a = 0; a < A_N; a++) { bindK[a][0] = actDefK[a][0]; bindK[a][1] = actDefK[a][1]; bindP[a] = actDefP[a]; } padLookSens = 1; padInvertY = false; padVibe = true; }
static void keysSave(void) {
    ensureDir("Data"); FILE *f = fopen(P("Data/keybinds.ini"), "w"); if (!f) return;
    fprintf(f, "# Zanko 2006 controls. key = raylib key code, mouse = 600 + button (600 LMB, 601 RMB, 602 MMB), 0 = none.\n# pad = raylib gamepad button (7 A, 6 B, 8 X, 5 Y, 9 LB, 11 RB, 10 LT, 12 RT, 13 Back, 15 Start, 16 L3, 17 R3, 1-4 D-pad).\n");
    for (int a = 0; a < A_N; a++) fprintf(f, "%s=%d,%d,%d\n", actIni[a], bindK[a][0], bindK[a][1], bindP[a]);
    fprintf(f, "padLook=%.2f\npadInvertY=%d\npadVibrate=%d\n", padLookSens, padInvertY, padVibe);
    fclose(f);
}
static void keysLoad(void) {
    keysDefaults();
    FILE *f = fopen(P("Data/keybinds.ini"), "r"); if (!f) return;
    char line[256], k[64], v[128];
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || sscanf(line, "%63[^=]=%127s", k, v) != 2) continue;
        if (!strcmp(k, "padLook")) { padLookSens = clampf((float)atof(v), 0.2f, 3); continue; }
        if (!strcmp(k, "padInvertY")) { padInvertY = atoi(v) != 0; continue; }
        if (!strcmp(k, "padVibrate")) { padVibe = atoi(v) != 0; continue; }
        for (int a = 0; a < A_N; a++) if (!strcmp(k, actIni[a])) { int x = 0, y = 0, z = 0; if (sscanf(v, "%d,%d,%d", &x, &y, &z) >= 1) { bindK[a][0] = x; bindK[a][1] = y; bindP[a] = z; } }
    }
    fclose(f);
}

// ---- the Controls screen: click a slot, then press a key, mouse button or pad button. Esc cancels, Backspace / Delete clears.
static int capA = -1, capS = 0, capFrame = 0;
static void bindSet(int a, int slot, int code) {
    for (int o = 0; o < A_N; o++) {   // one input, one action: take it off whatever had it
        if (slot < 2) { for (int s = 0; s < 2; s++) if (bindK[o][s] == code && !(o == a && s == slot)) bindK[o][s] = 0; }
        else if (bindP[o] == code && o != a) bindP[o] = 0;
    }
    if (slot < 2) bindK[a][slot] = code; else bindP[a] = code;
    keysSave();
}
static void bindCapture(int frameNo) {
    if (capA < 0 || frameNo - capFrame < 2) return;
    int k = GetKeyPressed();
    if (k == KEY_ESCAPE) { capA = -1; return; }
    if (k == KEY_BACKSPACE || k == KEY_DELETE) { if (capS < 2) bindK[capA][capS] = 0; else bindP[capA] = 0; keysSave(); capA = -1; return; }
    for (int b = 1; b < 18; b++) if (padP(b) && b != GAMEPAD_BUTTON_MIDDLE_RIGHT) { bindSet(capA, 2, b); capA = -1; return; }
    if (capS == 2) return;
    if (k > 0 && k < 512) { bindSet(capA, capS, k); capA = -1; return; }
    for (int b = 0; b < 5; b++) if (IsMouseButtonPressed(b)) { bindSet(capA, capS, BM(b)); capA = -1; return; }
}
