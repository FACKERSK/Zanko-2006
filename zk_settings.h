// zk_settings.h — game-only settings (Data/game.ini). The launcher shares Data/config.ini (resolution, sensitivity, FOV,
// volumes); everything else the game adds lives here so an older launcher saving config.ini never drops it.
typedef struct {
    int fpsCap;          // 0 unlimited
    int smooth;          // upscale the low-res image smoothly instead of sharp pixels
    float bright, fog;   // brightness (ambient light) and fog multipliers
    int wobble;          // PS1 vertex snapping
    int showFps;
    float sfx, amb, ui;  // effects, ambience (static, drone, broadcasts), interface volumes
    float bob, sway, shake;
    int sprintTgl, sneakTgl, hints, hitMarks, crossStyle, crossCol, disturb;
    int invertY;
    int reduceFlash; float dmgOverlay, insanityFx;
    int autoReload, pauseUnfocus, muteUnfocus, centered;
} GameSet;
static GameSet gs;
static void gsDefaults(void) {
    gs = (GameSet){.fpsCap = 144, .smooth = 0, .bright = 1, .fog = 1, .wobble = 1, .showFps = 0, .sfx = 1, .amb = 1, .ui = 1,
        .bob = 1, .sway = 1, .shake = 1, .sprintTgl = 0, .sneakTgl = 0, .hints = 1, .hitMarks = 1, .crossStyle = 0, .crossCol = 0, .disturb = 1,
        .invertY = 0, .reduceFlash = 0, .dmgOverlay = 1, .insanityFx = 1, .autoReload = 1, .pauseUnfocus = 1, .muteUnfocus = 0, .centered = 0};
}
#define GS_INTS X(fpsCap) X(smooth) X(wobble) X(showFps) X(sprintTgl) X(sneakTgl) X(hints) X(hitMarks) X(crossStyle) X(crossCol) X(disturb) X(invertY) X(reduceFlash) X(autoReload) X(pauseUnfocus) X(muteUnfocus) X(centered)
#define GS_FLOATS X(bright) X(fog) X(sfx) X(amb) X(ui) X(bob) X(sway) X(shake) X(dmgOverlay) X(insanityFx)
static void gsSave(void) {
    ensureDir("Data"); FILE *f = fopen(P("Data/game.ini"), "w"); if (!f) return;
    fprintf(f, "# Zanko 2006 game settings (the launcher keeps config.ini)\n");
#define X(n) fprintf(f, #n "=%d\n", gs.n);
    GS_INTS
#undef X
#define X(n) fprintf(f, #n "=%.3f\n", gs.n);
    GS_FLOATS
#undef X
    fclose(f);
}
static void gsLoad(void) {
    gsDefaults();
    FILE *f = fopen(P("Data/game.ini"), "r"); if (!f) return;
    char line[256], k[64], v[64];
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || sscanf(line, "%63[^=]=%63s", k, v) != 2) continue;
#define X(n) if (!strcmp(k, #n)) gs.n = atoi(v);
        GS_INTS
#undef X
#define X(n) if (!strcmp(k, #n)) gs.n = (float)atof(v);
        GS_FLOATS
#undef X
    }
    fclose(f);
    gs.bright = fminf(fmaxf(gs.bright, 0.5f), 1.8f); gs.fog = fminf(fmaxf(gs.fog, 0.3f), 1.6f);
}
static const int fpsChoices[] = {30, 60, 90, 120, 144, 165, 240, 0};
static const char *crossStyleNames[] = {"Lines", "Dot", "Lines + dot", "Off"};
static const char *crossColNames[] = {"Cyan", "White", "Green", "Yellow", "Magenta"};
static const Color crossCols[] = {{111, 214, 208, 230}, {240, 240, 236, 230}, {120, 230, 110, 230}, {245, 220, 90, 230}, {230, 110, 220, 230}};
