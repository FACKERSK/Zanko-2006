// zk_extra.h — 0.9.1: thrown things and fire pools, the extinguisher and the umbrella, rain, generators and lamps, bodies.
// Thrown things fly on the thrower's machine; what they start (a fire pool, a crackle) is announced so every player sees it,
// and damage to creatures always goes through the host. Rain is decided by the host and streamed with the players.

// ============================================================ thrown things
typedef struct { Vector3 p, v; int t; float age, fuse, spin; bool on, landed; } Throw;
#define MAX_THROW 32
static Throw throws[MAX_THROW];
typedef struct { Vector3 p; float life, r, tick; int light; bool on, mine; } Blaze;   // a burning pool from a molotov
#define MAX_BLAZE 16
static Blaze blazes[MAX_BLAZE];
static float rainK = 0, rainTgt = 0, rainT = 240;   // rain 0-1, where it is heading, seconds until the weather turns

static bool pointInSolid(Vector3 p) {
    query(p.x - 0.05f, p.z - 0.05f, p.x + 0.05f, p.z + 0.05f);
    for (int k = 0; k < qn; k++) { Solid *s = &sol[qb[k]]; if (s->off) continue; Vector3 L = solL(s, p);
        if (L.x > s->mn.x && L.x < s->mx.x && L.y > s->mn.y && L.y < s->mx.y && L.z > s->mn.z && L.z < s->mx.z) return true; }
    return false;
}
static void crDamage(int i, float d, bool alert) {   // a creature hurt by something of mine (fire, a rock): the host applies it
    if (i < 0 || i >= nCr) return; Cr *c = &crs[i]; if (!c->on || c->state == CS_DEAD || c->phantom) return;
    if (netMode == NM_CLIENT) { netCreatureHit(i, d); c->hurt = 0.2f; return; }
    c->hp -= cht.on[CH_ONEHIT] ? 9999 : d; c->hurt = 0.2f;
    if (c->hp <= 0) { c->state = CS_DEAD; c->deadT = 0; pl.kills++; playAt(sScreech, c->pos, 40, 0.6f); }
    else if (alert && c->state == CS_WANDER) { c->state = CS_CHASE; c->lastKnown = pl.pos; c->losT = 0; }
}
static void startBlaze(Vector3 p, bool mine) {
    int s = -1; for (int i = 0; i < MAX_BLAZE; i++) if (!blazes[i].on) { s = i; break; } if (s < 0) return;
    float g = groundAt(V3(p.x, p.y + 0.5f, p.z), 0.3f, 1.5f); if (g > -1e8f) p.y = g;
    int li = blazes[s].light; if (li <= 0 || li >= nLight) li = addLight(V3(p.x, p.y + 0.7f, p.z), V3(1.0f, 0.5f, 0.15f), 11, 1);
    else { lights[li].pos = V3(p.x, p.y + 0.7f, p.z); lights[li].mode = 1; }
    blazes[s] = (Blaze){p, 14, 2.1f, 0, li, true, mine};
    spawnParts(V3(p.x, p.y + 0.3f, p.z), 14, T_GLASS, rgb(120, 140, 120), 2.2f, 0.04f);
    playAt(sFireW, p, 45, 1.0f);
}
static void netFx(int kind, Vector3 p);   // tell the other players: 1 fire pool, 2 crackle
static void throwLand(Throw *T, int hitC) {
    Vector3 p = T->p;
    if (T->t == IT_ROCK) {
        playAt(sWood, p, 30, 0.6f); alertAll(p, 24 * (1 - 0.3f * rainK));
        if (hitC >= 0) { crDamage(hitC, idef[IT_ROCK].dmg, true); playAt(sHit, p, 20, 0.6f); }
        float g = groundAt(V3(p.x, p.y + 0.3f, p.z), 0.2f, 2.0f); addItem(V3(p.x, (g > -1e8f ? g : p.y) + 0.02f, p.z), IT_ROCK, 1);   // pick it up again
        T->on = false;
    } else if (T->t == IT_MOLOTOV) {
        startBlaze(p, true); netFx(1, p); alertAll(p, 20); if (hitC >= 0) crDamage(hitC, 2.0f, true); T->on = false;
    } else { T->landed = true; T->v = V3(0, 0, 0); playAt(sClick, p, 15, 0.5f); }   // firecrackers lie there fizzing
}
static void throwHeld(Vector3 eye, Vector3 aim) {
    int t = pl.hands.t; if (t < 0 || idef[t].cat != CAT_THROW || pl.hands.n <= 0) return;
    int s = -1; for (int i = 0; i < MAX_THROW; i++) if (!throws[i].on) { s = i; break; } if (s < 0) return;
    float spd = t == IT_MOLOTOV ? 12.5f : t == IT_FIRECRACKER ? 13.5f : 17;
    throws[s] = (Throw){Vector3Add(eye, Vector3Add(Vector3Scale(aim, 0.45f), V3(0, -0.1f, 0))), Vector3Add(Vector3Scale(aim, spd), V3(0, 2.4f, 0)), t, 0, t == IT_FIRECRACKER ? 4.4f : 0, fr(-12, 12), true, false};
    pl.hands.n--;
    if (pl.hands.n <= 0) { pl.hands = (Slot){-1, 0, 0}; for (int i = 0; i < invNSlots(); i++) if (pl.sl[i].t == t) { pl.hands = pl.sl[i]; pl.sl[i] = (Slot){-1, 0, 0}; break; } }
    play(sThrow, 0.7f); pl.swingDur = pl.swingT = 0.5f; pl.swingCd = 0.6f; pl.hitDone = true; netSwingN++;
    if (t == IT_FIRECRACKER) { play(sClick, 0.4f); say("The fuse fizzes."); }
}
static void updateThrows(float dt) {
    for (int i = 0; i < MAX_THROW; i++) { Throw *T = &throws[i]; if (!T->on) continue;
        T->age += dt;
        if (!T->landed) {
            for (int sub = 0; sub < 3 && T->on && !T->landed; sub++) { float h = dt / 3;
                T->v.y -= 9.8f * h; Vector3 np = Vector3Add(T->p, Vector3Scale(T->v, h));
                int hitC = -1; for (int k = 0; k < nCr; k++) { Cr *c = &crs[k]; if (!c->on || c->state == CS_DEAD || c->phantom || c->type == 4) continue; if (Vector3Distance(crCenter(c), np) < 0.55f) { hitC = k; break; } }
                float g = groundAt(V3(np.x, np.y + 0.3f, np.z), 0.1f, 0.6f);
                if (hitC >= 0 || pointInSolid(np) || (g > -1e8f && np.y <= g) || T->age > 6) {
                    if (pointInSolid(np)) { np = T->p; }   // stop short of the wall
                    T->p = np; throwLand(T, hitC);
                } else T->p = np;
            }
            if (T->landed && T->t == IT_FIRECRACKER) { float g = groundAt(V3(T->p.x, T->p.y + 0.3f, T->p.z), 0.2f, 2.0f); if (g > -1e8f) T->p.y = g + 0.03f; }
        }
        if (T->t == IT_FIRECRACKER) {   // fuse, then a long string of bangs that everything comes to
            float before = T->fuse; T->fuse -= dt;
            if (before > 3.2f && T->fuse <= 3.2f) { playAt(sCrack, T->p, 60, 1.0f); netFx(2, T->p); }
            if (T->fuse <= 3.2f) {
                if ((int)(before * 2.5f) != (int)(T->fuse * 2.5f)) { alertAll(T->p, 45 * (1 - 0.25f * rainK)); spawnParts(V3(T->p.x, T->p.y + 0.1f, T->p.z), 3, T_PLAIN, rgb(255, 220, 140), 1.6f, 0.025f); }
            }
            if (T->fuse <= 0) T->on = false;
        }
    }
}

// ============================================================ fire pools
static void updateBlazes(float dt) {
    for (int i = 0; i < MAX_BLAZE; i++) { Blaze *B = &blazes[i]; if (!B->on) continue;
        B->life -= dt * (1 + rainK * 1.5f);   // rain drowns it faster
        if (B->life <= 0) { B->on = false; if (B->light > 0 && B->light < nLight) lights[B->light].mode = 2; continue; }
        B->tick -= dt;
        if (B->tick <= 0) { B->tick = 0.5f;
            if (B->mine || netMode != NM_CLIENT) for (int k = 0; k < nCr; k++) { Cr *c = &crs[k];   // the one who threw it (or the host) burns what stands in it
                if (!c->on || c->state == CS_DEAD || c->type == 4 || c->type == 17 || c->phantom) continue;
                if (Vector3Distance(V3(c->pos.x, 0, c->pos.z), V3(B->p.x, 0, B->p.z)) < B->r && fabsf(c->pos.y - B->p.y) < 1.5f) { if (B->mine || netMode == NM_OFF) crDamage(k, 1.3f, false); c->stun = fmaxf(c->stun, 0.25f); } }
            if (pl.hp > 0 && Vector3Distance(V3(pl.pos.x, 0, pl.pos.z), V3(B->p.x, 0, B->p.z)) < B->r * 0.85f && fabsf(pl.pos.y - B->p.y) < 1.2f && !cht.on[CH_GOD]) { hurtPlayer(5, 0); pl.warmth = fminf(100, pl.warmth + 6); say("You're on fire!"); }
        }
        if (Vector3Distance(B->p, pl.pos) < 4) pl.nearFire = true;
    }
}

// ============================================================ the extinguisher and the umbrella
static bool sprayOn = false;
static void updateHeldTools(float dt, Vector3 eye, Vector3 aim) {
    bool can = state == ST_PLAY && !pl.invOpen && !pl.mapOpen && !pl.build && pl.hp > 0;
    // umbrella: [R] opens and closes it; swinging closes it
    if (pl.hands.t == IT_UMBRELLA) { if (can && actP(A_RELOAD)) { pl.umbOpen = !pl.umbOpen; play(sUmb, 0.7f); } if (pl.swingT > 0) pl.umbOpen = false; }
    else pl.umbOpen = false;
    pl.umbK += ((pl.umbOpen ? 1.0f : 0.0f) - pl.umbK) * fminf(1, dt * 9);
    // extinguisher: hold [R] to spray
    sprayOn = (can && pl.hands.t == IT_FIREEXT && actD(A_RELOAD) && pl.swingT <= 0 && pl.hands.w < 1000) || (vmSpray && pl.hands.t == IT_FIREEXT);
    if (!sprayOn) { pl.sprayT = 0; if (audioOk && IsSoundPlaying(sSpray) && pl.hands.t == IT_FIREEXT) StopSound(sSpray); return; }
    pl.sprayT += dt;
    if (audioOk && !IsSoundPlaying(sSpray)) { SetSoundVolume(sSpray, 0.55f * gs.sfx); PlaySound(sSpray); }
    { float add = dt * 1000.0f / 16.0f; int w = pl.hands.w + (int)add + (fr(0, 1) < add - (int)add ? 1 : 0); pl.hands.w = (short)(w > 1000 ? 1000 : w); }
    if (pl.hands.w >= 1000) { say("The extinguisher coughs and runs dry."); StopSound(sSpray); return; }
    Vector3 noz = Vector3Add(eye, Vector3Add(Vector3Scale(aim, 1.1f), V3(0, -0.3f, 0)));
    for (int k = 0; k < 3; k++) for (int i = 0; i < MAX_PART; i++) if (parts[i].life <= 0) {   // the white cloud pours forward
        Vector3 v = Vector3Add(Vector3Scale(spreadDir(aim, 0.18f), fr(5, 8)), V3(0, fr(-0.2f, 0.4f), 0));
        parts[i] = (Part){noz, v, fr(0.4f, 0.8f), fr(0.035f, 0.07f), noz.y - 2.0f, T_PLAIN, rgb(235, 238, 240)}; break; }
    for (int i = 0; i < nCr; i++) { Cr *c = &crs[i]; if (!c->on || c->state == CS_DEAD || c->phantom || c->type == 4) continue;   // blinded and choking: they stagger and lose you
        Vector3 d = Vector3Subtract(crCenter(c), eye); float L = Vector3Length(d); if (L > 5.5f || L < 0.01f || Vector3DotProduct(Vector3Scale(d, 1 / L), aim) < 0.82f) continue;
        if (netMode == NM_CLIENT) { static float st = 0; st -= dt; if (st <= 0) { st = 0.25f; netCreatureStun(i, 1.0f); } }
        else { c->stun = fmaxf(c->stun, 1.0f); c->seeT = 0; if (c->state == CS_CHASE && fr(0, 1) < dt * 2) c->state = CS_WANDER; } }
    for (int i = 0; i < MAX_BLAZE; i++) if (blazes[i].on) { Vector3 d = Vector3Subtract(blazes[i].p, eye); float L = Vector3Length(d);
        if (L < 6.5f && Vector3DotProduct(Vector3Scale(d, 1 / fmaxf(L, 0.01f)), aim) > 0.75f) blazes[i].life -= dt * 6; }
    for (int i = 0; i < nFire; i++) if (fires[i].on) { Vector3 d = Vector3Subtract(fires[i].pos, eye); float L = Vector3Length(d);
        if (L < 5 && Vector3DotProduct(Vector3Scale(d, 1 / fmaxf(L, 0.01f)), aim) > 0.75f) { fires[i].life -= dt * 40; } }
}

// ============================================================ rain
static float wetDry(void) {   // how much of the rain is kept off: the best coat, or an open umbrella
    float d = 0; for (int i = 0; i < WS_N; i++) if (pl.wear[i].t >= 0) d = fmaxf(d, idef[pl.wear[i].t].dry);
    if (pl.hands.t == IT_UMBRELLA && pl.umbOpen) d = fmaxf(d, idef[IT_UMBRELLA].dry);
    return d;
}
static void updateRain(float dt) {
    static int force = -1; if (force < 0) force = getenv("KIRI_RAIN") ? 1 : 0;
    if (force) { rainK = rainTgt = (float)atof(getenv("KIRI_RAIN")); if (rainK <= 0) rainK = rainTgt = 1; }
    else if (netMode != NM_CLIENT) {   // the host (or a lone player) decides the weather
        rainT -= dt;
        if (rainT <= 0) { if (rainTgt > 0) { rainTgt = 0; rainT = fr(420, 1000); } else if (fr(0, 1) < 0.5f * gRainK) { rainTgt = fminf(1, fr(0.35f, 1.0f) * fmaxf(gRainK, 0.5f)); rainT = fr(200, 520); } else rainT = fr(240, 600); }
        rainK += (rainTgt - rainK) * fminf(1, dt * 0.04f); if (rainK < 0.002f) rainK = 0;
    }
    if (!gameActive) return;
    // soaked through, unless sheltered, dried by a fire, or kept dry
    bool out = !pl.roof && pl.pos.y > -5;
    if (out && rainK > 0.05f) { float k = rainK * 4.5f * (1 - wetDry()); pl.wet = fminf(100, pl.wet + dt * k); if (pl.wet > 70 && pl.wet - dt * k <= 70) say("You're soaked to the skin. Get dry before the cold gets in."); }
    else pl.wet = fmaxf(0, pl.wet - dt * (pl.nearFire ? 5.0f : pl.roof ? 0.7f : 0.3f));
    // the sound: loud outside, a drum on the roof inside
    if (audioOk) { float v = rainK * (out ? 0.75f : 0.32f) * gs.amb;
        if (v > 0.01f) { if (!IsSoundPlaying(sRain)) PlaySound(sRain); SetSoundVolume(sRain, v); SetSoundPitch(sRain, out ? 1.0f : 0.7f); } else if (IsSoundPlaying(sRain)) StopSound(sRain); }
}
static void drawRain(Vector3 eye, float t) {
    if (rainK < 0.03f || eye.y < -5) return;
    static float cellT = 0; static bool open[10][10]; static int cx0 = 1 << 30, cz0 = 1 << 30;   // which 2 m columns around you are open to the sky
    int gx = (int)floorf(eye.x / 2) - 5, gz = (int)floorf(eye.z / 2) - 5; cellT -= GetFrameTime();
    if (cellT <= 0 || gx != cx0 || gz != cz0) { cellT = 0.4f; cx0 = gx; cz0 = gz;
        for (int a = 0; a < 10; a++) for (int b = 0; b < 10; b++) { Vector3 q = V3((gx + a) * 2 + 1, eye.y - 1.2f, (gz + b) * 2 + 1); open[a][b] = ceilAt(q, 0.4f) > eye.y + 30; } }
    int n = (int)(380 * rainK); setEmis(0.6f);
    for (int i = 0; i < n; i++) {
        float hx = hash2(i, 1, 51), hz = hash2(i, 2, 52), hp = hash2(i, 3, 53);
        float x = (gx + hx * 10) * 2, z = (gz + hz * 10) * 2;
        int a = (int)(hx * 10), b = (int)(hz * 10); if (a < 0 || a > 9 || b < 0 || b > 9 || !open[a][b]) continue;
        float y = eye.y + 9 - fmodf(t * 15 + hp * 18, 18);
        drawBox(T_PLAIN, V3(x - 0.008f, y, z - 0.008f), V3(x + 0.008f, y + 0.55f, z + 0.008f), rgb(150, 165, 170));
    }
    setEmis(0);
}

// ============================================================ generators and lamps
#define POWER_R 40.0f
static bool genRunning(const Build *b) { return b->alive && b->kind == BK_GENERATOR && b->open && b->fuel > 0; }
static unsigned char *lightPw; static unsigned char *lightSave; static int lightPwN;   // lights a generator has switched on, and what they were
static int nearestRunningGen(Vector3 p, float r) { int best = -1; float bd = r; for (int i = 0; i < nBuild; i++) if (genRunning(&builds[i])) { float d = Vector3Distance(buildC(&builds[i]), p); if (d < bd) { bd = d; best = i; } } return best; }
static void updatePower(float dt) {
    if (lightPwN != nLight) { lightPw = realloc(lightPw, (size_t)nLight + 1); lightSave = realloc(lightSave, (size_t)nLight + 1); for (int i = lightPwN; i < nLight; i++) lightPw[i] = 0; lightPwN = nLight; }
    static float fuelT = 0, scanT = 0, noiseT = 0; fuelT += dt; scanT -= dt; noiseT -= dt;
    if (fuelT >= 5) { if (netMode != NM_CLIENT) for (int i = 0; i < nBuild; i++) if (genRunning(&builds[i])) { builds[i].fuel = fmaxf(0, builds[i].fuel - fuelT); if (builds[i].fuel <= 0) builds[i].open = false; } fuelT = 0; }   // burns in 5 s steps (each one a world change)
    int any = 0; for (int i = 0; i < nBuild; i++) if (genRunning(&builds[i])) any++;
    if (scanT <= 0) { scanT = 0.5f;   // lights in reach of a running generator come on steady; the rest go back to how they were
        for (int i = 0; i < nLight; i++) {
            Light *L = &lights[i]; bool fire = L->col.x > 0.95f && L->col.y < 0.6f && L->col.z < 0.3f;
            bool want = false; if (any && !fire) for (int b = 0; b < nBuild && !want; b++) if (genRunning(&builds[b]) && Vector3Distance(buildC(&builds[b]), L->pos) < POWER_R) want = true;
            if (want && !lightPw[i]) { lightPw[i] = 1; lightSave[i] = (unsigned char)L->mode; L->mode = 0; }
            else if (!want && lightPw[i]) { lightPw[i] = 0; L->mode = lightSave[i]; }
        }
    }
    int g = nearestRunningGen(pl.pos, 34);
    if (g >= 0) { float d = Vector3Distance(buildC(&builds[g]), pl.pos);
        if (audioOk) { if (!IsSoundPlaying(sGen)) PlaySound(sGen); SetSoundVolume(sGen, 0.6f * powf(1 - d / 34, 1.4f) * gs.sfx); }
        if (pl.roof && d < POWER_R) pl.sanity = fminf(100, pl.sanity + dt * 0.12f); }   // a lit room
    else if (audioOk && IsSoundPlaying(sGen)) StopSound(sGen);
    if (noiseT <= 0) { noiseT = 4; if (netMode != NM_CLIENT) for (int i = 0; i < nBuild; i++) if (genRunning(&builds[i])) alertAll(buildC(&builds[i]), 24); }   // the noise carries
}
static bool extraPrompt(Vector3 eye, Vector3 aim) {   // a generator: start, stop, fuel it ([E] and [G])
    int g = facingBuild(eye, aim, 2.4f, BK_GENERATOR);
    if (g < 0) return false;
    Build *b = &builds[g]; int m = (int)b->fuel / 60, s = (int)b->fuel % 60;
    const char *fuelTxt = invHas(IT_FUEL) && b->fuel < 1500 ? "   [G] Pour in petrol" : "";
    if (b->fuel <= 0) snprintf(prompt, sizeof prompt, "Generator - no fuel%s", invHas(IT_FUEL) ? fuelTxt : " (needs a petrol can)");
    else snprintf(prompt, sizeof prompt, "[E] %s generator (%d:%02d of fuel)%s", b->open ? "Stop" : "Start", m, s, fuelTxt);
    if (actP(A_USE) && b->fuel > 0) { b->open = !b->open; play(b->open ? sGen : sClick, 0.6f); if (b->open) { play(sReload, 0.5f); say("The generator coughs and catches. Lights come on nearby - and it is loud."); } }
    if (actP(A_BARR) && invHas(IT_FUEL) && b->fuel < 1500) { invTake(IT_FUEL, 1); b->fuel += 600; play(sDrink, 0.5f); say("Petrol in. About ten more minutes."); }
    return true;
}
static void drawPowerPieces(void) { }   // generators and lamps are drawn with the other base pieces (zk_draw.h)

// ============================================================ bodies
static void bodyFromCreature(int i) {   // a creature that dies leaves something to search
    Cr *c = &crs[i]; if (c->type == 4 || c->phantom || netMode == NM_CLIENT) return;
    int k = addCont(CK_BODY, V3(c->pos.x, c->pos.y + 0.02f, c->pos.z), c->yaw, c->type, false);
    if (k >= 0) conts[k].var = (short)(100 + i);
}
static void dropPlayerBody(void) {   // everything you carried stays with you where you fell
    int k = addCont(CK_BODY, V3(pl.pos.x, pl.pos.y + 0.02f, pl.pos.z), pl.yaw, 0, false); if (k < 0) return;
    Cont *c = &conts[k]; c->var = 4; c->filled = true;
    int n = 0;
    Slot all[6 + 18 + 6]; all[n++] = pl.hands; all[n++] = pl.back; for (int j = 0; j < 4; j++) all[n++] = pl.wear[j]; for (int j = 0; j < invNSlots(); j++) all[n++] = pl.sl[j];
    for (int j = 0; j < n; j++) if (all[j].t >= 0 && all[j].n > 0) { int left = slotsAdd(c->s, CSLOTS, all[j].t, all[j].n); if (left > 0) addItem(pl.pos, all[j].t, left); }
    pl.hands = pl.back = (Slot){-1, 0, 0}; for (int j = 0; j < 4; j++) pl.wear[j] = (Slot){-1, 0, 0}; for (int j = 0; j < 24; j++) pl.sl[j] = (Slot){-1, 0, 0};
}
static void drawBodyCont(Cont *c) {
    if (c->var >= 100) { int i = c->var - 100; if (i < nCr && crs[i].on && crs[i].state == CS_DEAD && Vector3Distance(crs[i].pos, c->pos) < 1.5f) return; }   // the creature's own corpse is still lying there
    Matrix M = TRS(c->pos, c->yaw, V3(1, 1, 1)); Color dim = c->searched ? (Color){170, 165, 160, 255} : WHITE;
    int ty = c->var >= 100 ? c->region : -1;
    if (ty == 3 || ty == 13) { ADRAW("bodies/beast", M, dim, BX(T_FLESH, 0, 0.18f, 0, 0.42f, 0.32f, 0.95f, ((Color){70, 50, 45, 255})), BX(T_FLESH, 0, 0.14f, 0.6f, 0.26f, 0.24f, 0.34f, ((Color){80, 55, 50, 255})),
                                     BX(T_FLESH, 0.3f, 0.05f, 0.2f, 0.4f, 0.08f, 0.1f, ((Color){60, 45, 40, 255})), BX(T_FLESH, -0.3f, 0.05f, -0.25f, 0.4f, 0.08f, 0.1f, ((Color){60, 45, 40, 255}))); return; }
    Color cloth = c->var == 2 ? (Color){40, 50, 80, 255} : c->var == 3 ? (Color){215, 220, 222, 255} : c->var == 4 ? (Color){44, 54, 42, 255} : ty == 7 ? (Color){235, 235, 230, 255} : ty == 15 ? (Color){28, 30, 36, 255}
                : ty == 17 ? (Color){40, 30, 26, 255} : ty >= 0 ? (Color){90, 80, 74, 255} : ((Color){(unsigned char)(60 + (int)(c->pos.x * 13) % 60), 62, (unsigned char)(70 + (int)(c->pos.z * 7) % 50), 255});
    Color skin = ty == 7 ? (Color){240, 240, 236, 255} : ty == 17 ? (Color){50, 30, 24, 255} : ty >= 0 ? (Color){150, 130, 120, 255} : (Color){175, 150, 135, 255};
    ADRAW("bodies/person", M, dim, BX(T_CLOTH, 0, 0.12f, 0.1f, 0.44f, 0.22f, 0.62f, cloth), BX(T_FLESH, 0, 0.11f, 0.56f, 0.22f, 0.2f, 0.24f, skin),
          BX(T_CLOTH, -0.11f, 0.08f, -0.52f, 0.16f, 0.15f, 0.78f, ((Color){36, 40, 50, 255})), BX(T_CLOTH, 0.12f, 0.08f, -0.5f, 0.16f, 0.15f, 0.76f, ((Color){36, 40, 50, 255})),
          BX(T_CLOTH, -0.32f, 0.07f, 0.18f, 0.12f, 0.12f, 0.56f, cloth), BX(T_CLOTH, 0.36f, 0.07f, 0.05f, 0.12f, 0.12f, 0.56f, cloth),
          BX(T_PLAIN, 0.05f, 0.012f, 0.2f, 0.9f, 0.01f, 0.9f, ((Color){60, 18, 16, 255})));   // the stain under it
}

// ============================================================ per frame, and drawing
static void updateExtras(float dt, Vector3 eye, Vector3 aim) {
    updateRain(dt);
    if (!gameActive) return;
    updateThrows(dt); updateBlazes(dt); updatePower(dt);
    if (state == ST_PLAY) updateHeldTools(dt, eye, aim);
}
static void drawExtras(Vector3 eye, float t) {
    for (int i = 0; i < MAX_THROW; i++) { Throw *T = &throws[i]; if (!T->on) continue;
        Matrix M = MatrixMultiply(MatrixMultiply(MatrixRotateX(T->landed ? PI / 2 : T->age * T->spin), MatrixRotateY(T->age * 3)), MatrixTranslate(T->p.x, T->p.y, T->p.z));
        setEmis(0); drawItemType(T->t, M);
        if ((T->t == IT_MOLOTOV && !T->landed) || (T->t == IT_FIRECRACKER && T->fuse > 3.2f)) { setEmis(1); drawBox(T_PLAIN, Vector3Subtract(T->p, V3(0.03f, 0.03f, 0.03f)), Vector3Add(T->p, V3(0.03f, 0.03f, 0.03f)), rgb(255, 180, 60)); }   // the lit rag, the fizzing fuse
        if (T->t == IT_FIRECRACKER && T->fuse <= 3.2f && hash2((int)(t * 30), i, 5) > 0.5f) { setEmis(1); Vector3 q = Vector3Add(T->p, V3(fr(-0.3f, 0.3f), fr(0, 0.3f), fr(-0.3f, 0.3f)));
            drawBox(T_PLAIN, Vector3Subtract(q, V3(0.06f, 0.06f, 0.06f)), Vector3Add(q, V3(0.06f, 0.06f, 0.06f)), rgb(255, 240, 200)); }
        setEmis(0); }
    for (int i = 0; i < MAX_BLAZE; i++) { Blaze *B = &blazes[i]; if (!B->on) continue; float k = fminf(1, B->life / 3);
        setEmis(0); drawBox(T_PLAIN, V3(B->p.x - B->r * 0.8f, B->p.y + 0.005f, B->p.z - B->r * 0.8f), V3(B->p.x + B->r * 0.8f, B->p.y + 0.015f, B->p.z + B->r * 0.8f), rgb(30, 22, 18));
        setEmis(1);
        for (int f = 0; f < 9; f++) { float a = f * 2.4f + i, rr = B->r * (0.25f + 0.6f * hash2(f, i, 9)), h = (0.3f + 0.6f * fabsf(sinf(t * (6 + f) + f))) * k, w = 0.22f + 0.1f * hash2(f, i, 3);
            Vector3 q = V3(B->p.x + cosf(a) * rr, B->p.y, B->p.z + sinf(a) * rr);
            ADRAW("props/campfire_flame", TRS(q, t * 2 + f, V3(w, h, w)), f % 3 ? rgb(255, 150, 40) : rgb(255, 230, 120), BX(T_PLAIN, 0, 0.5f, 0, 1, 1, 1, CWH)); }
        setEmis(0); }
    drawRain(eye, t);
}
