// zk_surv.h — v0.4 survival fundamentals: traits, hints, day/night clock, water, sickness, infection, fractures,
// weapon wear, crafting, traps, gardens, loot restock, the map, and save / load.

// ============================================================ traits (one strength, one weakness)
#define TR_N 6
static const char *trSName[TR_N] = {"Athlete", "Field Medic", "Scavenger", "Carpenter", "Iron Stomach", "Night Owl"};
static const char *trSDesc[TR_N] = {"Sprinting and swinging use 20% less stamina", "Starts with First Aid level 2", "Starts with Scavenging level 2",
                                    "Starts with Carpentry level 2, a hammer and nails", "70% less likely to get sick from bad water", "Flashlight batteries last 65% longer"};
static const char *trWName[TR_N] = {"Hemophiliac", "Brittle Bones", "Big Appetite", "Insomniac", "Faint-Hearted", "Clumsy"};
static const char *trWDesc[TR_N] = {"Wounds bleed 50% faster", "Bones break 2.5x as easily", "Hunger grows 30% faster",
                                    "Tiredness grows 40% faster", "Sanity drops 30% faster", "Sprinting is heard 40% further away"};
// which: 0 stamina 1 sickness chance 2 fracture chance 3 food drain 4 battery drain 5 bleed 6 noise 7 rest drain 8 sanity loss
static float trMul(int which) {
    int s = pl.trS - 1, w = pl.trW - 1;
    switch (which) {
    case 0: return s == 0 ? 0.8f : 1; case 1: return s == 4 ? 0.3f : 1; case 4: return s == 5 ? 0.6f : 1;
    case 5: return w == 0 ? 1.5f : 1; case 2: return w == 1 ? 2.5f : 1; case 3: return w == 2 ? 1.3f : 1;
    case 7: return w == 3 ? 1.4f : 1; case 8: return w == 4 ? 1.3f : 1; case 6: return w == 5 ? 1.4f : 1;
    }
    return 1;
}
static void applyTraits(void) {
    int s = pl.trS - 1;
    if (s == 1) pl.sk[SK_MED] = skNeed[2];
    if (s == 2) pl.sk[SK_SCAV] = skNeed[2];
    if (s == 3) { pl.sk[SK_CARP] = skNeed[2]; invAdd(IT_HAMMER, 1); invAdd(IT_NAILS, 12); }
}

// ============================================================ first-time hints (stored per profile)
#define HINT_N 12
static const char *hintTxt[HINT_N] = {
    "Hold E on furniture to search it. Tab: inventory and crafting. M: map.",
    "Bleeding: take a bandage or rags in hand (right-click it in Tab) and click to apply. Wounds left open can get infected.",
    "Night is falling. More spirits walk at night. Get under a roof, light a fire, save batteries.",
    "Thirsty: rain barrels (blue drums) and fountains give unboiled water. Boil it at a fire (Tab > Crafting).",
    "Sick: you lose food and water faster. Charcoal (craft at a fire from a plank) cuts it short.",
    "Broken leg: craft a splint (Tab > Crafting: 1 plank + 2 rags), then use it.",
    "Spirits ignore weapons. Seal them with an ofuda, pour salt, or build wards inside a base.",
    "The hills drain sanity fast. The springs are clean, but do not stay long.",
    "Your weapon is wearing out. Repair it: Tab > Crafting (scrap + nails, hammer).",
    "The world saves when you sleep, every 2 minutes, and when you leave to the menu.",
    "Infected wound: use disinfectant. A fever stops your blood from recovering.",
    "Build a Kamidana to claim a base. Garden beds grow daikon. Spike traps and can alarms guard the edges."};
static int hintQ[8], hintN; static float hintT;
static void showHint(int id) {
    if (!gs.hints || id < 0 || id >= HINT_N || (prof.hints & (1u << id))) return;
    prof.hints |= 1u << id; profSave();
    if (hintN < 8) hintQ[hintN++] = id;
}
static void drawHint(float W, float H) {
    if (hintN <= 0) return;
    float s = aU; hintT += GetFrameTime();
    if (hintT > 10) { for (int i = 1; i < hintN; i++) hintQ[i - 1] = hintQ[i]; hintN--; hintT = 0; if (hintN <= 0) return; }
    const char *tx = hintTxt[hintQ[0]];
    Vector2 m = aMeasure(tx, 14, false);
    Rectangle r = {W / 2 - m.x / 2 - 22 * s, 118 * s, m.x + 44 * s, 40 * s};
    aGadget(r);
    aText("TIP", r.x + 12 * s, r.y + 12 * s, 11, C_AMBER, true);
    aText(tx, r.x + 40 * s, r.y + 11 * s, 14, C_BONE, false);
    (void)H;
}

// ============================================================ conditions
static void getSick(float amount, const char *why) { pl.sick = fminf(100, pl.sick + amount); pl.vomitT = fr(15, 30); say(why); showHint(4); }
static void breakLeg(const char *why) { pl.fracture = 1; pl.fracT = 0; say(why); play(sBone, 1.0f); pl.shake = fmaxf(pl.shake, 0.6f); pl.sanity -= 6; showHint(5); }
static float weaponLife(int t) {   // hits until it breaks
    switch (t) { case IT_AXE: return 140; case IT_KATANA: return 70; case IT_PIPE: return 220; case IT_BAT: return 110; case IT_CROWBAR: return 260;
                 case IT_KNIFE: return 90; case IT_SLEDGE: return 180; case IT_HAMMER: return 140; case IT_NAILBAT: return 80; default: return 0; }
}
static void weaponWear(float k) {   // wear is stored 0-1000 in Slot.w
    int t = pl.hands.t; if (t < 0 || idef[t].cat != CAT_MELEE) return;
    float life = weaponLife(t); if (life <= 0) return;
    float add = 1000.0f / life * k; int w = pl.hands.w + (int)add + (fr(0, 1) < add - (int)add ? 1 : 0);
    pl.hands.w = (short)(w > 1000 ? 1000 : w);
    if (pl.hands.w > 700) showHint(8);
    if (pl.hands.w >= 1000) {
        say(TextFormat("The %s breaks.", idef[t].name)); play(sClang, 0.9f);
        spawnParts(Vector3Add(pl.pos, V3(sinf(pl.yaw) * 0.6f, 1.2f, cosf(pl.yaw) * 0.6f)), 8, T_METAL, rgb(150, 150, 150), 1.5f, 0.05f);
        for (int q = 0; q < 6; q++) if (pl.quick[q] == t && invCount(t) <= 1) pl.quick[q] = -1;
        pl.hands.t = -1; pl.hands.n = 0; pl.hands.w = 0;
        if (t == IT_NAILBAT) giveBack(IT_NAILS, 3);
    }
}

// ============================================================ crafting
enum { RQ_FIRE = 1, RQ_HAMMER = 2, RQ_SHRINE = 4, RQ_WORN = 8 };
typedef struct { const char *name; short in[3][2]; short out, outN; int need; } Recipe;
static const Recipe recipes[] = {
    {"Bandage", {{IT_RAG, 3}, {-1, 0}, {-1, 0}}, IT_BANDAGE, 1, 0},
    {"Splint", {{IT_PLANKS, 1}, {IT_RAG, 2}, {-1, 0}}, IT_SPLINT, 1, 0},
    {"Boil water", {{IT_DIRTYWATER, 1}, {-1, 0}, {-1, 0}}, IT_WATER, 1, RQ_FIRE},
    {"Charcoal", {{IT_PLANKS, 1}, {-1, 0}, {-1, 0}}, IT_CHARCOAL, 2, RQ_FIRE},
    {"Ofuda (blessed)", {{IT_MANGA, 1}, {IT_SALT, 1}, {-1, 0}}, IT_OFUDA, 3, RQ_SHRINE},
    {"Nail Bat", {{IT_BAT, 1}, {IT_NAILS, 12}, {-1, 0}}, IT_NAILBAT, 1, RQ_HAMMER},
    {"Repair held weapon", {{IT_SCRAP, 1}, {IT_NAILS, 4}, {-1, 0}}, -1, 0, RQ_HAMMER | RQ_WORN},
};
#define N_RECIPE ((int)(sizeof recipes / sizeof recipes[0]))
static bool nearShrine(void) { return baseShrine >= 0 && builds[baseShrine].alive && Vector3Distance(builds[baseShrine].mn, pl.pos) < 4.5f; }
static const char *recipeMissing(int i) {   // NULL when it can be crafted now
    const Recipe *r = &recipes[i];
    for (int k = 0; k < 3; k++) if (r->in[k][0] >= 0 && invCount(r->in[k][0]) < r->in[k][1]) return "missing materials";
    if ((r->need & RQ_FIRE) && !pl.nearFire) return "needs a fire";
    if ((r->need & RQ_HAMMER) && !invHas(IT_HAMMER)) return "needs a hammer";
    if ((r->need & RQ_SHRINE) && !nearShrine()) return "needs your Kamidana";
    if ((r->need & RQ_WORN) && (pl.hands.t < 0 || idef[pl.hands.t].cat != CAT_MELEE || pl.hands.w < 50)) return "hold a worn weapon";
    return NULL;
}
static const char *recipeText(int i) {
    static char b[128]; b[0] = 0; const Recipe *r = &recipes[i];
    for (int k = 0; k < 3; k++) if (r->in[k][0] >= 0) { char p[48]; snprintf(p, sizeof p, "%s%d %s", b[0] ? " + " : "", r->in[k][1], idef[r->in[k][0]].name); strncat(b, p, sizeof b - strlen(b) - 1); }
    if (r->out >= 0) { char p[48]; snprintf(p, sizeof p, "  >  %d %s", r->outN, idef[r->out].name); strncat(b, p, sizeof b - strlen(b) - 1); }
    else strncat(b, "  >  -60% wear", sizeof b - strlen(b) - 1);
    return b;
}
static void craft(int i) {
    if (recipeMissing(i)) return;
    const Recipe *r = &recipes[i];
    for (int k = 0; k < 3; k++) if (r->in[k][0] >= 0) invTake(r->in[k][0], r->in[k][1]);
    if (r->out >= 0) giveBack(r->out, r->outN);
    else { pl.hands.w = (short)(pl.hands.w > 600 ? pl.hands.w - 600 : 0); }
    if (r->in[0][0] == IT_DIRTYWATER) { /* the bottle comes back full */ }
    play((r->need & RQ_HAMMER) ? sPlace : sPick, 0.6f);
    say(TextFormat("Crafted: %s.", r->name));
    skGain(r->out == IT_BANDAGE || r->out == IT_SPLINT ? SK_MED : SK_CARP, 2);
}
static void drawCrafting(Rectangle cp) {
    float s = aU;
    aWindow(cp, "Crafting");
    float y = cp.y + 52 * s;
    aText(pl.nearFire ? "At a fire" : "No fire nearby", cp.x + 18 * s, y, 11, pl.nearFire ? C_AMBER : C_BONE2, false);
    aText(invHas(IT_HAMMER) ? "Hammer: yes" : "Hammer: no", cp.x + 130 * s, y, 11, C_BONE2, false); y += 22 * s;
    for (int i = 0; i < N_RECIPE; i++) {
        const char *miss = recipeMissing(i);
        aText(recipes[i].name, cp.x + 18 * s, y, 14, miss ? (Color){150, 146, 136, 255} : (Color){236, 230, 216, 255}, true);
        aText(recipeText(i), cp.x + 18 * s, y + 18 * s, 10, C_BONE2, false);
        if (miss) aText(miss, cp.x + 18 * s, y + 32 * s, 10, (Color){210, 120, 90, 255}, false);
        if (aButton((Rectangle){cp.x + cp.width - 92 * s, y + 4 * s, 74 * s, 30 * s}, "Craft", miss == NULL)) craft(i);
        y += 50 * s;
    }
}

// ============================================================ world interaction: water, gardens
static int facingWater(Vector3 aim) {
    Vector3 f = Vector3Normalize(V3(aim.x, 0, aim.z));
    for (int i = 0; i < nWs; i++) {
        Vector3 d = V3(wsrc[i].pos.x - pl.pos.x, 0, wsrc[i].pos.z - pl.pos.z); float L = Vector3Length(d);
        float reach = wsrc[i].kind == 1 ? 2.3f : 1.7f;
        if (L < reach && fabsf(wsrc[i].pos.y - pl.pos.y) < 1.5f && (L < 0.6f || Vector3DotProduct(Vector3Scale(d, 1 / L), f) > 0.4f)) return i;
    }
    return -1;
}
static bool survPrompt(Vector3 eye, Vector3 aim, bool act) {
    int w = facingWater(aim);
    if (w >= 0) {
        const char *nm = wsrc[w].kind == 0 ? "rain barrel" : wsrc[w].kind == 1 ? "fountain" : "spring";
        bool clean = wsrc[w].kind == 2;
        if (!act) { snprintf(prompt, sizeof prompt, invHas(IT_BOTTLE) ? "[E] Fill a bottle at the %s%s" : "[E] Drink from the %s%s", nm, clean ? " (clean)" : " (unboiled)"); return true; }
        if (invHas(IT_BOTTLE)) { invTake(IT_BOTTLE, 1); giveBack(clean ? IT_WATER : IT_DIRTYWATER, 1); play(sPick, 0.5f); say(clean ? "Bottle filled. The spring water is clear." : "Bottle filled. Boil it before drinking."); }
        else {
            pl.water = fminf(100, pl.water + 20); play(sPick, 0.4f);
            if (!clean && fr(0, 1) < 0.35f * trMul(1)) getSick(35, "You drink. Your stomach turns.");
            else say(clean ? "Cold, clean water." : "You drink straight from it.");
        }
        return true;
    }
    int g = facingBuild(eye, aim, 2.4f, BK_GARDEN);
    if (g >= 0) {
        Build *b = &builds[g];
        if (!b->open) {
            if (!act) { snprintf(prompt, sizeof prompt, invHas(IT_SEEDS) ? "[E] Plant daikon seeds" : "Garden bed (empty) - needs daikon seeds"); return true; }
            if (invTake(IT_SEEDS, 1)) { b->open = true; b->hp = 0; play(sPick, 0.5f); say("Planted."); }
            return true;
        }
        if (b->hp >= b->maxhp) {
            if (!act) { snprintf(prompt, sizeof prompt, "[E] Harvest daikon"); return true; }
            int n = 2 + ri(2); giveBack(IT_DAIKON, n); if (fr(0, 1) < 0.6f) giveBack(IT_SEEDS, 1 + ri(2));
            b->open = false; b->hp = 0; play(sPick, 0.6f); say(TextFormat("Harvested %d daikon.", n)); skGain(SK_SCAV, 2);
            return true;
        }
        if (!act) { snprintf(prompt, sizeof prompt, "Daikon growing - %d%%", (int)(100 * b->hp / b->maxhp)); return true; }
        return true;
    }
    return false;
}

// ============================================================ map
#define MAX_MARK 8
static Vector2 marks[MAX_MARK]; static int nMark;
// ---- the 3D map: a miniature of the town in the reference palette (blue ground, peach roads, glassy blue blocks, white pins)
static RenderTexture2D mapRT; static bool mapRTok = false; static float mapYaw = 0.6f, mapZoom = 1.0f, mapPitch = 0.95f; static Vector2 mapDrag; static bool mapDragging, mapMoved;
static Camera3D mapCamera(void) {
    Vector3 tgt = V3(220, 0, 280); float d = 520 * mapZoom;   // the city and the east mountain
    return (Camera3D){Vector3Add(tgt, V3(sinf(mapYaw) * cosf(mapPitch) * d, sinf(mapPitch) * d, cosf(mapYaw) * cosf(mapPitch) * d)), tgt, V3(0, 1, 0), 38, CAMERA_PERSPECTIVE};
}
static int mapXf = 0;   // frame of the building being drawn on the map
static void mapQuadC(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color ca, Color cb, Color cc, Color cd) {
    if (mapXf) { a = xfP(mapXf, a); b = xfP(mapXf, b); c = xfP(mapXf, c); d = xfP(mapXf, d); }
    rlColor4ub(ca.r, ca.g, ca.b, 255); rlVertex3f(a.x, a.y, a.z); rlColor4ub(cb.r, cb.g, cb.b, 255); rlVertex3f(b.x, b.y, b.z); rlColor4ub(cc.r, cc.g, cc.b, 255); rlVertex3f(c.x, c.y, c.z);
    rlColor4ub(ca.r, ca.g, ca.b, 255); rlVertex3f(a.x, a.y, a.z); rlColor4ub(cc.r, cc.g, cc.b, 255); rlVertex3f(c.x, c.y, c.z); rlColor4ub(cd.r, cd.g, cd.b, 255); rlVertex3f(d.x, d.y, d.z);
}
// map palette: the 5b concept map (green hills, cedar slopes, bare rock, blue bay, dark asphalt, pale sidewalks, beige buildings, brown old houses)
static const Color MP_HILL_LO = {96, 170, 70, 255}, MP_HILL_HI = {150, 205, 95, 255}, MP_FOREST = {40, 92, 52, 255}, MP_ROCK = {128, 124, 112, 255},
    MP_SEA = {40, 110, 160, 255}, MP_DEEP = {18, 55, 98, 255}, MP_ROAD = {70, 72, 76, 255}, MP_WALK = {176, 174, 166, 255}, MP_BLD = {218, 200, 190, 255},
    MP_BLD2 = {205, 210, 220, 255}, MP_OLD = {112, 82, 64, 255}, MP_PARK = {92, 150, 78, 255}, MP_DASH = {240, 240, 230, 255}, MP_TUN = {240, 200, 80, 255},
    MP_TUNK = {40, 40, 45, 255}, MP_X = {200, 40, 30, 255}, MP_SUB = {230, 120, 40, 255}, MP_EDGE = {60, 58, 56, 255};
static Color mpLerp(Color a, Color b, float t) { t = clampf(t, 0, 1); return (Color){(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t), (unsigned char)(a.b + (b.b - a.b) * t), 255}; }
static void mapBlock(float x0, float z0, float x1, float z1, float y0, float h) {   // an extruded building: lit top, shaded sides, dark outline at the eaves
    bool old = h < 4; Color lid = old ? MP_OLD : (((int)(x0 * 7 + z0 * 3) & 1) ? MP_BLD : MP_BLD2), side = aMul(lid, 0.78f), foot = aMul(lid, 0.55f);
    float y1 = y0 + h, e = 0.25f;
    mapQuadC(V3(x0 - e, y1, z0 - e), V3(x0 - e, y1, z1 + e), V3(x1 + e, y1, z1 + e), V3(x1 + e, y1, z0 - e), MP_EDGE, MP_EDGE, MP_EDGE, MP_EDGE);
    mapQuadC(V3(x0, y1 + 0.05f, z0), V3(x0, y1 + 0.05f, z1), V3(x1, y1 + 0.05f, z1), V3(x1, y1 + 0.05f, z0), lid, lid, lid, lid);
    mapQuadC(V3(x0, y0, z1), V3(x1, y0, z1), V3(x1, y1, z1), V3(x0, y1, z1), foot, foot, side, side);
    mapQuadC(V3(x1, y0, z0), V3(x0, y0, z0), V3(x0, y1, z0), V3(x1, y1, z0), aMul(foot, 0.85f), aMul(foot, 0.85f), aMul(side, 0.85f), aMul(side, 0.85f));
    mapQuadC(V3(x1, y0, z1), V3(x1, y0, z0), V3(x1, y1, z0), V3(x1, y1, z1), aMul(foot, 0.92f), aMul(foot, 0.92f), aMul(side, 0.92f), aMul(side, 0.92f));
    mapQuadC(V3(x0, y0, z0), V3(x0, y0, z1), V3(x0, y1, z1), V3(x0, y1, z0), aMul(foot, 0.78f), aMul(foot, 0.78f), aMul(side, 0.78f), aMul(side, 0.78f));
}
static void mapPin(Vector3 p, Color c, float s) {
    DrawCylinderEx(V3(p.x, p.y, p.z), V3(p.x, p.y + 9 * s, p.z), 0.4f * s, 1.6f * s, 6, c);
    DrawSphere(V3(p.x, p.y + 11 * s, p.z), 3.2f * s, c); DrawSphere(V3(p.x, p.y + 11 * s, p.z), 1.4f * s, (Color){30, 34, 38, 255});
}
static void mapRibbon(const Vector3 *p, int n, float w, float y, Color c) {
    for (int i = 0; i + 1 < n; i++) { Vector2 a = pnorm(p, n, i), b = pnorm(p, n, i + 1);
        float ya = (p[i].y > 0.5f ? p[i].y * 0.5f : 0) + y, yb = (p[i + 1].y > 0.5f ? p[i + 1].y * 0.5f : 0) + y;
        mapQuadC(V3(p[i].x + a.x * w, ya, p[i].z + a.y * w), V3(p[i + 1].x + b.x * w, yb, p[i + 1].z + b.y * w), V3(p[i + 1].x - b.x * w, yb, p[i + 1].z - b.y * w), V3(p[i].x - a.x * w, ya, p[i].z - a.y * w), c, c, c, c); }
}
static void mapDashes(const Vector3 *p, int n, float w, float y, float on, float off, Color c) {   // centre-line dashes along a road path
    float acc = 0;
    for (int i = 0; i + 1 < n; i++) { Vector3 A = p[i], B = p[i + 1]; float L = sqrtf((B.x - A.x) * (B.x - A.x) + (B.z - A.z) * (B.z - A.z)); if (L < 1e-3f) continue;
        Vector2 d = {(B.x - A.x) / L, (B.z - A.z) / L}, nn = {-d.y * w, d.x * w};
        for (float s0 = 0; s0 < L; ) { float ph = fmodf(acc + s0, on + off), seg = ph < on ? fminf(on - ph, L - s0) : fminf(on + off - ph, L - s0);
            if (ph < on) { float s1 = s0 + seg; float ya = (A.y > 0.5f ? Lerp(A.y, B.y, s0 / L) * 0.5f : 0) + y, yb = (A.y > 0.5f ? Lerp(A.y, B.y, s1 / L) * 0.5f : 0) + y;
                Vector3 a0 = V3(A.x + d.x * s0, ya, A.z + d.y * s0), a1 = V3(A.x + d.x * s1, yb, A.z + d.y * s1);
                mapQuadC(V3(a0.x + nn.x, a0.y, a0.z + nn.y), V3(a1.x + nn.x, a1.y, a1.z + nn.y), V3(a1.x - nn.x, a1.y, a1.z - nn.y), V3(a0.x - nn.x, a0.y, a0.z - nn.y), c, c, c, c); }
            s0 += fmaxf(seg, 0.01f); }
        acc += L; }
}
static void renderMapScene(float t) {
    if (!mapRTok) { mapRT = LoadRenderTexture(1100, 820); SetTextureFilter(mapRT.texture, TEXTURE_FILTER_BILINEAR); mapRTok = true; }
    Camera3D cam3 = mapCamera();
    BeginTextureMode(mapRT); ClearBackground(MP_DEEP);
    rlSetClipPlanes(4.0, 3000.0);   // depth range sized for a 500 m view: streets 0.2 m above the ground stay on top
    BeginMode3D(cam3);
    rlDisableBackfaceCulling(); rlBegin(RL_TRIANGLES);
    const int st = 4; int N = HF_N;   // terrain every 8 m, heights halved; sea in deep blue
    for (int j = 0; j + st < N; j += st) for (int i = 0; i + st < N; i += st) {
        float x0 = HF_X0 + i * HF_S, z0 = HF_X0 + j * HF_S, x1 = x0 + st * HF_S, z1 = z0 + st * HF_S;
        float h[4] = {hf[j * N + i], hf[j * N + i + st], hf[(j + st) * N + i + st], hf[(j + st) * N + i]}; Color cc[4];
        int vi[4] = {i, i + st, i + st, i}, vj[4] = {j, j, j + st, j + st};
        for (int q = 0; q < 4; q++) {
            if (h[q] < SEA_Y + 0.1f || (h[q] < RIVER_Y && inRiverRaw(HF_X0 + vi[q] * HF_S, HF_X0 + vj[q] * HF_S))) { cc[q] = mpLerp(MP_SEA, MP_DEEP, (SEA_Y - h[q]) / 6); h[q] = -0.4f; continue; }
            int ii = vi[q], jj = vj[q]; float hx = hf[jj * N + (ii + st < N ? ii + st : ii)] - hf[jj * N + (ii >= st ? ii - st : ii)], hz = hf[(jj + st < N ? jj + st : jj) * N + ii] - hf[(jj >= st ? jj - st : jj) * N + ii];
            float shade = clampf(0.92f + (-hx * 0.6f - hz * 0.4f) * 0.035f, 0.55f, 1.15f);   // light from the north-west, like the concept map
            Color g = mpLerp(MP_HILL_LO, MP_HILL_HI, h[q] / 18);
            g = mpLerp(g, MP_FOREST, (h[q] - 14) / 14); g = mpLerp(g, MP_ROCK, (h[q] - 46) / 12);
            if (h[q] < 1.2f) g = mpLerp((Color){226, 208, 160, 255}, g, h[q] / 1.2f);   // sand at the waterline
            cc[q] = aMul(g, shade); h[q] *= 0.5f; }
        for (int q = 0; q < 4; q++) { float qx = q == 1 || q == 2 ? x1 : x0, qz = q >= 2 ? z1 : z0; if (inCore(qx, qz) || cityLand(qx, qz)) { cc[q] = MP_WALK; h[q] = 0.0f; } }
        mapQuadC(V3(x0, h[0], z0), V3(x1, h[1], z0), V3(x1, h[2], z1), V3(x0, h[3], z1), cc[0], cc[1], cc[2], cc[3]);
    }
    Color road = MP_ROAD;
    for (int i = 0; i <= NB; i++) for (int j = 0; j <= NB; j++) {   // city: pale sidewalk blocks (green parks), dark asphalt streets between them
        if (blockOn(i, j)) { float x0 = ROADW + i * PITCH, z0 = ROADW + j * PITCH; Color bc = blockKindA[i][j] == BK_PARK ? MP_PARK : MP_WALK;
            mapQuadC(V3(x0 - 1, 0.1f, z0 - 1), V3(x0 + BLOCK + 1, 0.1f, z0 - 1), V3(x0 + BLOCK + 1, 0.1f, z0 + BLOCK + 1), V3(x0 - 1, 0.1f, z0 + BLOCK + 1), bc, bc, bc, bc); }
        #define RQ(X, Z, WW, HH) mapQuadC(V3(X, 0.2f, Z), V3((X) + (WW), 0.2f, Z), V3((X) + (WW), 0.2f, (Z) + (HH)), V3(X, 0.2f, (Z) + (HH)), road, road, road, road)
        if (nodeOn(i, j)) RQ(i * PITCH + 1, j * PITCH + 1, ROADW - 2, ROADW - 2);
        if (segV(i, j)) RQ(i * PITCH + 1, ROADW + j * PITCH - 1, ROADW - 2, BLOCK + 2);
        if (segH(i, j)) RQ(ROADW + i * PITCH - 1, j * PITCH + 1, BLOCK + 2, ROADW - 2);
        #undef RQ
    }
    for (int i = 0; i + 1 < nCoast; i++) { Vector2 nn = pnorm(coastP, nCoast, i); Vector3 q = V3(coastP[i].x - nn.x * 7.8f, 0, coastP[i].z - nn.y * 7.8f); if (coastSkipVerge(q)) continue;   // the green verge
        Vector3 seg[2] = {coastP[i], coastP[i + 1]}; Vector3 sg2[2] = {V3(seg[0].x - nn.x * 7.8f, 0, seg[0].z - nn.y * 7.8f), V3(seg[1].x - nn.x * 7.8f, 0, seg[1].z - nn.y * 7.8f)}; mapRibbon(sg2, 2, 1.6f, 0.3f, MP_PARK); }
    mapRibbon(coastP, nCoast, 5.6f, 0.25f, MP_WALK); mapRibbon(coastP, nCoast, 4.5f, 0.3f, road); mapRibbon(northP, nNorth, 4.5f, 0.3f, road);
    mapRibbon(tougeP, nTouge, 4.4f, 0.35f, MP_WALK); mapRibbon(tougeP, nTouge, 3.6f, 0.4f, road); mapRibbon(laneP, nLane, 2.5f, 0.4f, road);
    for (int r = 0; r < 4; r++) if (nRing[r] > 1) mapRibbon(ringP[r], nRing[r], 4.4f, 0.3f, road);
    for (int s2 = 0; s2 < nStreet; s2++) mapRibbon(stPts + dStreets[s2][0], dStreets[s2][1], dStreets[s2][2] / 20.0f, 0.3f, road);   // every street
    if (nRail > 1) mapRibbon(railP, nRail, 2.2f, 0.35f, (Color){120, 116, 108, 255});
    for (int e = 0; e < nExt; e++) { Rectangle r = extR[e]; mapQuadC(V3(r.x, 0.25f, r.y), V3(r.x + r.width, 0.25f, r.y), V3(r.x + r.width, 0.25f, r.y + r.height), V3(r.x, 0.25f, r.y + r.height), road, road, road, road); }
    if (nAvenue > 1) { mapRibbon(avenueP, nAvenue, 7.0f, 0.32f, road); mapDashes(avenueP, nAvenue, 0.35f, 0.4f, 4, 4, MP_TUN); }
    mapDashes(coastP, nCoast, 0.35f, 0.38f, 4, 4, MP_DASH); mapDashes(northP, nNorth, 0.35f, 0.38f, 4, 4, MP_DASH); mapDashes(tougeP, nTouge, 0.3f, 0.48f, 3, 3, MP_DASH);
    for (int i = 0; i < nMapFoot; i++) { Rectangle r = mapFoot[i]; float y0 = mapFootY[i] > 0.5f ? mapFootY[i] * 0.5f : 0.1f; mapXf = mapFootF[i]; mapBlock(r.x + 0.3f, r.y + 0.3f, r.x + r.width - 0.3f, r.y + r.height - 0.3f, y0, mapFootH[i]);  mapXf = 0; }
    rlEnd(); rlEnableBackfaceCulling();
    for (int b = 0; b < nBore; b++) { Bore *br = &bores[b];   // tunnels: yellow-on-black dashes, ending in a red cross
        for (float a = 0; a < br->len; a += 6) { Vector3 p0 = boreAt(br, a, 0, br->fy * 0.5f + 1), p1 = boreAt(br, fminf(a + 6, br->len), 0, br->fy * 0.5f + 1);
            DrawCylinderEx(p0, p1, 1.5f, 1.5f, 4, MP_TUNK); if (((int)(a / 6)) % 2 == 0) DrawCylinderEx(Vector3Add(p0, V3(0, 0.9f, 0)), Vector3Add(p1, V3(0, 0.9f, 0)), 0.7f, 0.7f, 4, br->kind ? (Color){190, 120, 80, 255} : MP_TUN); }
        Vector3 e = boreAt(br, br->len, 0, br->fy * 0.5f + 2), dd = Vector3Normalize(Vector3Subtract(boreAt(br, 1, 0, 0), boreAt(br, 0, 0, 0))); Vector3 sd = V3(-dd.z, 0, dd.x);
        Vector3 k1 = Vector3Scale(Vector3Add(dd, sd), 4.2f), k2 = Vector3Scale(Vector3Subtract(dd, sd), 4.2f);
        DrawCylinderEx(Vector3Subtract(e, k1), Vector3Add(e, k1), 1.1f, 1.1f, 4, MP_X); DrawCylinderEx(Vector3Subtract(e, k2), Vector3Add(e, k2), 1.1f, 1.1f, 4, MP_X); }
    for (int i = 0; i < nHole; i++) { Vector3 c = V3(holes[i].x + holes[i].width / 2, 2.0f, holes[i].y + holes[i].height / 2); DrawCylinder(c, 3.2f, 3.2f, 1.6f, 12, MP_EDGE); DrawCylinder(V3(c.x, c.y + 0.2f, c.z), 2.5f, 2.5f, 1.6f, 12, MP_SUB); }
    for (int i = 0; i < nWs; i++) DrawSphere(V3(wsrc[i].pos.x, fmaxf(0, wsrc[i].pos.y * 0.5f) + 1.5f, wsrc[i].pos.z), 1.6f, (Color){120, 210, 250, 255});
    if (baseShrine >= 0 && builds[baseShrine].alive) { Vector3 c = builds[baseShrine].mn; mapPin(V3(c.x, fmaxf(0, c.y * 0.5f), c.z), (Color){220, 60, 50, 255}, 1.2f); }
    for (int i = 0; i < nMark; i++) mapPin(V3(marks[i].x, terrainH(marks[i].x, marks[i].y) > 0 ? terrainH(marks[i].x, marks[i].y) * 0.5f : 0, marks[i].y), (Color){250, 205, 70, 255}, 1.2f);
    {   Vector3 pp = V3(pl.pos.x, fmaxf(0, pl.pos.y * 0.5f) + sinf(t * 3) * 0.8f, pl.pos.z); mapPin(pp, WHITE, 1.6f);   // you, facing the way you look
        Vector3 d = V3(sinf(pl.yaw), 0, cosf(pl.yaw)); DrawCylinderEx(Vector3Add(pp, V3(0, 0.6f, 0)), Vector3Add(pp, Vector3Add(V3(0, 0.6f, 0), Vector3Scale(d, 12))), 1.6f, 0.1f, 4, WHITE); }
    EndMode3D(); EndTextureMode();
    rlSetClipPlanes(0.08, 700.0);   // back to the game's own range
}
static void drawMap(float W, float H) {
    float t = (float)GetTime();
    DrawRectangle(0, 0, (int)W, (int)H, (Color){6, 8, 9, 150});
    { float wh = GetMouseWheelMove(); if (wh != 0) mapZoom = clampf(mapZoom * (wh > 0 ? 0.88f : 1.13f), 0.35f, 1.6f); }
    {   // gamepad: right stick turns and tilts, RT / LT zoom in / out
        float dt = fminf(GetFrameTime(), 0.05f);
        mapYaw -= padLookV.x * 2.2f * dt; mapPitch = clampf(mapPitch + padLookV.y * 1.2f * dt, 0.45f, 1.45f);
        if (padNow[GAMEPAD_BUTTON_RIGHT_TRIGGER_2]) mapZoom = clampf(mapZoom * (1 - 1.2f * dt), 0.35f, 1.6f);
        if (padNow[GAMEPAD_BUTTON_LEFT_TRIGGER_2]) mapZoom = clampf(mapZoom * (1 + 1.2f * dt), 0.35f, 1.6f); }
    renderMapScene(t);
    p3Begin(t);
    Rectangle mp = p3Panel(4, 0, 0, 800, 640, true); float s = aU;
    if (aWindow(mp, "Map  -  Kirimachi")) { pl.mapOpen = false; DisableCursor(); }
    if (jpOk) DrawTextEx(jpFont, jpMapTitle, (Vector2){mp.x + 210 * s, mp.y + 12 * s}, 18 * s, 1, (Color){236, 230, 216, 255});
    Rectangle mr = {mp.x + 14 * s, mp.y + 46 * s, mp.width - 28 * s, mp.height - 100 * s};
    DrawTexturePro(mapRT.texture, (Rectangle){0, 0, (float)mapRT.texture.width, -(float)mapRT.texture.height}, mr, (Vector2){0, 0}, 0, WHITE);
    DrawRectangleLinesEx(mr, 2 * s, (Color){8, 10, 10, 255});
    float ly = mr.y + mr.height + 10 * s, lx = mr.x;
    struct { Color c; const char *t; } leg[] = {{MP_BLD, "Buildings"}, {MP_OLD, "Old houses"}, {MP_SEA, "Bay"}, {MP_HILL_LO, "Hills"}, {MP_ROAD, "Roads"}, {{120, 116, 108, 255}, "Railway"},
        {MP_TUN, "Road tunnel"}, {MP_X, "Collapse"}, {{120, 210, 250, 255}, "Water"}, {{250, 205, 70, 255}, "Markers"}};
    for (int i = 0; i < 10; i++) { DrawRectangle((int)lx, (int)(ly + 3 * s), (int)(12 * s), (int)(12 * s), leg[i].c); aText(leg[i].t, lx + 16 * s, ly, 12, C_BONE2, false); lx += (aMeasure(leg[i].t, 12, false).x + 34 * s); }
    aText(padActive ? "A marker   X remove nearest   Right stick rotate   RT / LT zoom   B close" : TextFormat("LMB marker   RMB drag rotate / click remove   Wheel zoom   %s / Esc close", actKey(A_MAP)), mr.x, ly + 20 * s, 12, C_BONE2, false);
    Vector2 m = aMousePos(); bool over = CheckCollisionPointRec(m, mr);
    if (over) {
        Vector2 rtp = {(m.x - mr.x) / mr.width * mapRT.texture.width, (m.y - mr.y) / mr.height * mapRT.texture.height};
        Ray ray = GetScreenToWorldRayEx(rtp, mapCamera(), mapRT.texture.width, mapRT.texture.height);
        float tt = ray.direction.y < -1e-4f ? -ray.position.y / ray.direction.y : -1; Vector2 wp = {ray.position.x + ray.direction.x * tt, ray.position.z + ray.direction.z * tt};
        if (tt > 0 && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || aPadClick)) { play(sClick, 0.35f); if (nMark < MAX_MARK) marks[nMark++] = wp; else { for (int i = 1; i < MAX_MARK; i++) marks[i - 1] = marks[i]; marks[MAX_MARK - 1] = wp; } play(sClick, 0.4f); }
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { mapDragging = true; mapMoved = false; mapDrag = GetMousePosition(); }
        if (((mapDragging && IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && !mapMoved) || padP(GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) && tt > 0 && nMark > 0) {
            int best = 0; float bd = 1e9f; for (int i = 0; i < nMark; i++) { float dd = Vector2Distance(marks[i], wp); if (dd < bd) { bd = dd; best = i; } }
            for (int i = best + 1; i < nMark; i++) marks[i - 1] = marks[i]; nMark--; play(sClick, 0.3f); }
    }
    if (mapDragging) { Vector2 g = GetMousePosition(); float dx = g.x - mapDrag.x, dy = g.y - mapDrag.y; if (fabsf(dx) + fabsf(dy) > 3) mapMoved = true;
        mapYaw -= dx * 0.006f; mapPitch = clampf(mapPitch + dy * 0.004f, 0.45f, 1.45f); mapDrag = g; if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) mapDragging = false; }
    p3End(4);
    p3Render(pl.stage);
}

// ============================================================ save / load
static const char *savePath(void) { static char rel[160]; snprintf(rel, sizeof rel, "Data/Userdata/%s/save.dat", prof.name[0] ? prof.name : "Guest"); return P(rel); }
static bool saveExists(void) { return FileExists(savePath()); }
static void deleteSave(void) { if (saveExists()) remove(savePath()); }
#define SAVE_MAGIC 0x3034485Au   // "ZH04"
#define WR(x) fwrite(&(x), sizeof(x), 1, f)
#define WRA(a, n) do { int _n = (n); fwrite(&_n, sizeof _n, 1, f); fwrite((a), sizeof((a)[0]), _n, f); } while (0)

static bool saveGame(void) {
    if (netMode == NM_CLIENT) return false;   // a guest in someone else's game keeps no save
    if (!gameActive || pl.hp <= 0) return false;
    ensureDir("Data"); ensureDir("Data/Userdata");
    { char rel[128]; snprintf(rel, sizeof rel, "Data/Userdata/%s", prof.name[0] ? prof.name : "Guest"); ensureDir(rel); }
    FILE *f = fopen(savePath(), "wb"); if (!f) return false;
    unsigned int magic = SAVE_MAGIC, ver = 5; WR(magic); WR(ver); WR(worldSeed); WR(worldSolids);
    Player p = pl; p.invOpen = false; p.mapOpen = false; p.openC = -1; p.searchC = -1; p.dragFrom = -1; p.drag.t = -1; p.build = false; WR(p);
    unsigned char *bits = calloc((size_t)worldSolids, 1); for (int i = 0; i < worldSolids; i++) bits[i] = sol[i].off; fwrite(bits, 1, (size_t)worldSolids, f); free(bits);
    WRA(ops, nOps); WRA(debs, nDeb); WRA(crates, nCrate); WRA(vends, nVend); WRA(builds, nBuild); WRA(conts, nCont); WRA(crs, nCr); WRA(wards, nWard); WRA(fires, nFire); WRA(marks, nMark);
    WR(baseShrine); WR(evacOn); WR(evacT);
    fclose(f);
    TraceLog(LOG_INFO, "SAVE: written (day %d %.1fh)", pl.day, pl.tod);
    return true;
}
#define RD(x) (fread(&(x), sizeof(x), 1, f) == 1)
static bool rdArr(FILE *f, void *a, size_t el, int max, int *n, bool exact) {
    int k; if (fread(&k, sizeof k, 1, f) != 1 || k < 0 || k > max || (exact && k != *n)) return false;
    if (fread(a, el, (size_t)k, f) != (size_t)k) return false; *n = k; return true;
}
static bool loadGame(void) {
    FILE *f = fopen(savePath(), "rb"); if (!f) return false;
    unsigned int magic, ver, seed; int ws;
    if (!RD(magic) || magic != SAVE_MAGIC || !RD(ver) || ver != 5 || !RD(seed) || !RD(ws)) { fclose(f); return false; }
    genWorld(seed);
    if (ws != worldSolids) { fclose(f); TraceLog(LOG_WARNING, "SAVE: world mismatch"); return false; }
    Player p; if (!RD(p)) { fclose(f); return false; }
    unsigned char *bits = malloc((size_t)ws); bool ok = fread(bits, 1, (size_t)ws, f) == (size_t)ws;
    if (ok) for (int i = 0; i < ws; i++) sol[i].off = bits[i];
    free(bits);
    ok = ok && rdArr(f, ops, sizeof ops[0], MAX_OPEN, &nOps, true) && rdArr(f, debs, sizeof debs[0], MAX_DEB, &nDeb, true) && rdArr(f, crates, sizeof crates[0], MAX_CRATE, &nCrate, true)
            && rdArr(f, vends, sizeof vends[0], MAX_VEND, &nVend, true) && rdArr(f, builds, sizeof builds[0], MAX_BUILD, &nBuild, false) && rdArr(f, conts, sizeof conts[0], MAX_CONT, &nCont, false)
            && rdArr(f, crs, sizeof crs[0], MAX_CR, &nCr, false) && rdArr(f, wards, sizeof wards[0], MAX_WARD, &nWard, false) && rdArr(f, fires, sizeof fires[0], MAX_FIRE, &nFire, false)
            && rdArr(f, marks, sizeof marks[0], MAX_MARK, &nMark, false) && RD(baseShrine) && RD(evacOn) && RD(evacT);
    fclose(f);
    if (!ok) { TraceLog(LOG_WARNING, "SAVE: corrupt"); return false; }
    pl = p;
    // rebuild what lives on top of the generated world
    for (int i = 0; i < nVend; i++) if (vends[i].broken && vends[i].light >= 0) lights[vends[i].light].mode = 2;
    for (int i = 0; i < nOps; i++) if (ops[i].state == 2) ops[i].bsid = addSolidF(ops[i].mn, ops[i].mx, ops[i].f); else ops[i].bsid = -1;
    for (int i = 0; i < nBuild; i++) { Build *b = &builds[i]; b->sid = -1;
        if (b->alive && b->kind != BK_SHRINE && b->kind != BK_WARD && b->kind != BK_GARDEN && b->kind != BK_SPIKES && b->kind != BK_ALARM) { buildAddSolid(b); if (b->sid >= 0 && b->kind == BK_DOOR && b->open) sol[b->sid].off = true; } }
    for (int i = 0; i < nFire; i++) fires[i].light = fires[i].on ? addLight(V3(fires[i].pos.x, fires[i].pos.y + 0.8f, fires[i].pos.z), V3(1.0f, 0.55f, 0.2f), 10, 1) : -1;
    pl.invOpen = pl.mapOpen = pl.sleeping = pl.build = false; pl.watchT = 0; pl.climbT = 0; pl.swingT = 0; pl.hitStopT = 0; pl.searchC = -1; pl.openC = -1; pl.dragFrom = -1; pl.drag.t = -1;
    gameActive = true; evacOn = evacOn && evacT > 0;
    memset(splats, 0, sizeof splats);
    say(TextFormat("Day %d. You are where you left off.", pl.day));
    TraceLog(LOG_INFO, "SAVE: loaded (day %d %.1fh, %d builds, %d creatures)", pl.day, pl.tod, nBuild, nCr);
    cheatsReset(); return true;
}

// ============================================================ the per-frame tick for all of the above
static float trapCd[MAX_BUILD];
static void restock(void) {   // each midnight: some far-away searched places have something in them again
    int n = 0;
    for (int i = 0; i < nCont; i++) { Cont *c = &conts[i];
        if (!c->on || !c->searched || c->kind == CK_PILE || c->kind == CK_CHEST || c->kind == CK_GUNLOCKER) continue;
        if (Vector3Distance(c->pos, pl.pos) < 60 || fr(0, 1) > 0.12f) continue;
        c->filled = false; c->searched = false; n++; }
    TraceLog(LOG_INFO, "RESTOCK: %d containers", n);
}
static void survivalTick(float dt) {
    // ---- clock: one in-game hour per 50 s; sleep runs it 12x faster
    float hours = dt / 50.0f * (pl.sleeping ? 12 : 1);
    pl.tod += hours;
    if (pl.tod >= 24) { pl.tod -= 24; pl.day++; restock(); }
    if (pl.tod > 18.3f && pl.tod < 19.5f) showHint(2);
    // ---- sickness
    if (pl.sick > 0) {
        pl.sick = fmaxf(0, pl.sick - dt * 100.0f / 300.0f * (pl.sleeping ? 2 : 1));
        pl.vomitT -= dt;
        if (pl.sick > 30 && pl.vomitT <= 0 && !pl.sleeping) {
            pl.vomitT = fr(35, 70); pl.water = fmaxf(0, pl.water - 12); pl.food = fmaxf(0, pl.food - 10); pl.sanity -= 3;
            pl.shake = fmaxf(pl.shake, 0.5f); pl.hurtT = fmaxf(pl.hurtT, 0.3f); play(sHurt, 0.6f); say("You double over and vomit.");
        }
        if (pl.sick <= 0) say("The sickness has passed.");
    }
    // ---- infection: wounds left open turn; fever blocks blood recovery and later drains it
    if (pl.wounds > 0 && pl.infect <= 0) { pl.woundT += dt; if (pl.woundT > 45) { pl.infect = 1; say("The wound is turning hot and red."); } }
    if (pl.wounds <= 0) pl.woundT = 0;
    if (pl.infect > 0) {
        if (pl.wounds == 0 && pl.food > 50 && pl.water > 50 && pl.infect < 35) pl.infect -= dt * 100.0f / 900.0f;   // a mild one clears by itself
        else pl.infect = fminf(100, pl.infect + dt * 100.0f / 600.0f);
        if (pl.infect >= 35) { showHint(10); pl.warmth += sinf(pl.time * 0.7f) * dt * 3; }
        if (pl.infect >= 70) pl.hp -= 0.12f * dt;
        if (pl.infect <= 0) { pl.infect = 0; say("The wound has closed clean."); }
    }
    // ---- broken leg heals only once splinted (12 in-game hours)
    if (pl.fracture == 2) { pl.fracT += hours; if (pl.fracT >= 12) { pl.fracture = 0; say("Your leg has healed."); } }
    // ---- gardens, traps
    for (int i = 0; i < nBuild; i++) { Build *b = &builds[i]; if (!b->alive) continue;
        if (b->kind == BK_GARDEN && b->open && b->hp < b->maxhp) { b->hp = fminf(b->maxhp, b->hp + hours * (dayK() > 0.3f ? 1.2f : 0.6f)); }
        if (b->kind == BK_SPIKES || b->kind == BK_ALARM) {
            trapCd[i] -= dt; if (trapCd[i] > 0) continue;
            Vector3 c = Vector3Scale(Vector3Add(b->mn, b->mx), 0.5f);
            for (int k = 0; k < nCr; k++) { Cr *cr = &crs[k];
                if (!cr->on || cr->state == CS_DEAD || cr->phantom || cr->type == 4 || cr->hanging) continue;
                if (Vector3Distance(V3(cr->pos.x, 0, cr->pos.z), V3(c.x, 0, c.z)) > (b->kind == BK_SPIKES ? 0.85f : 1.2f) || fabsf(cr->pos.y - b->mn.y) > 1.0f) continue;
                if (b->kind == BK_SPIKES) {
                    cr->hp -= 1.6f; cr->stun = fmaxf(cr->stun, 1.2f); cr->hurt = 0.3f; cr->flinch = 1;
                    spawnParts(V3(cr->pos.x, cr->pos.y + 0.4f, cr->pos.z), 12, T_FLESH, rgb(130, 10, 10), 1.6f, 0.06f); addSplat(V3(cr->pos.x, b->mn.y + 0.02f, cr->pos.z), 0.5f);
                    playAt(sBone, c, 30, 0.8f);
                    if (cr->hp <= 0) { cr->state = CS_DEAD; cr->deadT = 0; pl.kills++; }
                    b->hp -= 1; trapCd[i] = 1.2f;
                    if (b->hp <= 0) { b->alive = false; if (Vector3Distance(c, pl.pos) < 30) say("A spike trap broke apart."); }
                } else {
                    playAt(sClang, c, 60, 1.0f); trapCd[i] = 8;
                    float d = Vector3Distance(c, pl.pos);
                    if (d < 70) { float a = atan2f(c.x - pl.pos.x, c.z - pl.pos.z) - pl.yaw; a = wrapA(a);
                        say(TextFormat("A can alarm rattles %d m %s.", (int)d, fabsf(a) < 0.8f ? "ahead" : fabsf(a) > 2.3f ? "behind you" : a > 0 ? "to your left" : "to your right")); }
                }
                break;
            }
        }
    }
    // ---- situational hints
    if (pl.water < 40) showHint(3);
    if (wildK(pl.pos) > 0.3f) showHint(7);
    if (pl.time > 20) showHint(9);
    for (int i = 0; i < nCr; i++) if (crs[i].on && crs[i].type == 4 && crs[i].state != CS_DEAD && crs[i].seen && Vector3Distance(crs[i].pos, pl.pos) < 15) { showHint(6); break; }
    // ---- autosave
    static bool wasSleeping = false;
    pl.saveT += dt;
    if ((pl.saveT > 120 && !pl.sleeping && pl.searchC < 0) || (wasSleeping && !pl.sleeping)) { pl.saveT = 0; saveGame(); }
    wasSleeping = pl.sleeping;
}

// ============================================================ new game: trait choice
static int selS = 0, selW = 0;
static void traitsPanel(void) {
    Rectangle p = p3Panel(3, 0, 0, 760, 560, true);
    if (aWindow(p, "New survivor  -  one strength, one weakness")) state = ST_MENU;
    float s = aU, x1 = p.x + 36 * s, x2 = p.x + p.width / 2 + 10 * s, y0 = p.y + 64 * s;
    aText("Strength", x1, y0, 17, C_CYAN, true); aText("Weakness", x2, y0, 17, (Color){220, 110, 90, 255}, true);
    for (int i = 0; i < TR_N; i++) {
        float y = y0 + 34 * s + i * 58 * s;
        if (aToggle(x1, y, trSName[i], selS == i) && selS != i) selS = i;
        aText(trSDesc[i], x1 + 4 * s, y + 26 * s, 11, C_BONE2, false);
        if (aToggle(x2, y, trWName[i], selW == i) && selW != i) selW = i;
        aText(trWDesc[i], x2 + 4 * s, y + 26 * s, 11, C_BONE2, false);
    }
    if (aButton((Rectangle){p.x + p.width / 2 - 230 * s, p.y + p.height - 66 * s, 200 * s, 44 * s}, "Back", true)) state = ST_MENU;
    if (aButton((Rectangle){p.x + p.width / 2 + 30 * s, p.y + p.height - 66 * s, 200 * s, 44 * s}, "Start", true)) {
        newGame(); pl.trS = selS + 1; pl.trW = selW + 1; applyTraits(); saveGame(); state = ST_PLAY; DisableCursor();
    }
    p3End(3);
}
