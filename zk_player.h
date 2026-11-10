static bool forceGuard;   // test hook (shot mode)
static bool lineOfSight(Vector3 a, Vector3 b);
static bool survPrompt(Vector3 eye, Vector3 aim, bool act);
static void breakLeg(const char *why);
static void weaponWear(float k);
static int *gunMag(void) { return pl.hands.t == IT_SHOTGUN ? &pl.magS : &pl.magP; }   // rounds loaded in the pistol / shotgun
static float gunReloadT(void) { return pl.hands.t == IT_SHOTGUN ? 2.1f : 1.7f; }
static void showHint(int id);
// zk_player.h — player reset, combat feel, building, interaction and the per-frame player update
static void spawnUnderground(void);
static void resetPlayer(void) {
    selfShot = false; inputLocked = false;
    memset(&pl, 0, sizeof pl);
    pl.pos = V3(SPAWN_X, 0, SPAWN_Z); pl.yaw = pl.tyaw = 0.6f;
    pl.hp = 100; pl.stam = 100; pl.food = 70; pl.water = 65; pl.batt = 60; pl.flash = true;
    pl.sanity = 75; pl.warmth = 55; pl.rest = 75; pl.boredom = 10;
    pl.tod = 16.5f; pl.day = 1;
    invReset();
    invAdd(IT_RAG, 2); invAdd(IT_FOOD, 1); invAdd(IT_DRINK, 1);
}
static void newGame(void) {
    unsigned int seed = getenv("KIRI_VIDEO") ? 2003u : gSeedArg ? gSeedArg : ((unsigned int)time(NULL) ^ 0x9E3779B9u);
    resetPlayer();
    genWorld(seed);
    for (int i = 0; i < 26; i++) spawnCreature(pl.pos, 38, 0, false);
    for (int i = 0; i < 26; i++) spawnIndoor(pl.pos, 30);
    spawnUnderground();
    spawnTunnels();
    evacOn = false; evacT = -1;
    gameActive = true; cheatsReset();
    prof.sessions++; profSave();
    memset(splats, 0, sizeof splats);
    say("Japan, 2006. No rescue arrives. Search furniture for supplies [hold E].");
    showHint(0);
}

// ============================================================ player helpers
static Vector3 dirOf(float yaw, float pitch) { return V3(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch)); }
static float crHeight(int type) {
    switch (type) { case 0: return 1.2f; case 1: return 1.8f; case 2: return 0.45f; case 3: return 0.65f; case 5: return 1.55f; case 6: return 1.25f; case 7: return 1.25f; case 8: return 0.5f; case 9: return 1.75f; default: return 1.3f; }
}
static Vector3 crCenter(Cr *c) { return Vector3Add(c->pos, V3(0, c->hanging ? -1.0f : crHeight(c->type), 0)); }
// creature damage: blood loss plus a chance of an open wound
static void hurtPlayer(float dmg, float bleed) {
    if (cht.on[CH_GOD]) return;   // cheat
    if (pl.blocking && pl.stam > 2) {   // guard: only what comes at you from the front
        Vector3 fw = V3(sinf(pl.yaw), 0, cosf(pl.yaw)); Cr *att = NULL; float bd = 3.4f;
        for (int i = 0; i < nCr; i++) { Cr *c = &crs[i]; if (!c->on || c->state == CS_DEAD || c->phantom || c->type == 4) continue;
            Vector3 d = V3(c->pos.x - pl.pos.x, 0, c->pos.z - pl.pos.z); float L = Vector3Length(d);
            if (L < bd && L > 0.01f && Vector3DotProduct(Vector3Scale(d, 1 / L), fw) > 0.3f) { bd = L; att = c; } }
        if (att) {
            bool parry = pl.blockT < 0.28f, armed = pl.hands.t >= 0 && idef[pl.hands.t].cat == CAT_MELEE;
            float red = parry ? 0.95f : armed ? 0.75f : 0.4f;
            dmg *= 1 - red; bleed *= parry ? 0 : 0.35f;
            pl.stam -= parry ? 4 : 10 + dmg * 2; weaponWear(parry ? 0.5f : 1.2f);
            Vector3 push = Vector3Normalize(V3(att->pos.x - pl.pos.x, 0, att->pos.z - pl.pos.z));
            att->stun = fmaxf(att->stun, parry ? 1.4f : 0.35f); att->kb = Vector3Add(att->kb, Vector3Scale(push, parry ? 7 : 3)); att->flinch = 1;
            pl.shake = fmaxf(pl.shake, parry ? 0.35f : 0.25f); pl.hitStopT = parry ? 0.09f : 0.04f; pl.punch = fmaxf(pl.punch, 0.3f);
            play(armed ? sClang : sImpact, parry ? 1.0f : 0.7f);
            if (parry) say("Parried."); 
            if (pl.stam <= 0) { pl.stam = 0; pl.blocking = false; pl.exhausted = true; say("Your guard breaks."); }
            if (dmg < 0.5f) return;
        }
    }
    { float ar = wearArmor(); dmg *= 1 - ar; bleed *= 1 - fminf(0.8f, ar * 1.5f); }   // what you wear takes some of it
    pl.hp -= dmg; pl.hurtT = 0.6f; pl.shake = fmaxf(pl.shake, 0.4f); pl.punch = fmaxf(pl.punch, 0.6f);
    pl.sanity -= 3; pl.sleeping = false; pl.watchT = 0;
    if (fr(0, 1) < bleed) { addWound(1); say(pl.wounds > 1 ? "Another wound. You are bleeding badly." : "You are bleeding. Bandage it."); showHint(1);
        if (pl.infect <= 0 && fr(0, 1) < 0.12f) pl.infect = 1; }   // bites carry filth
    if (dmg >= 18 && pl.fracture == 0 && fr(0, 1) < 0.12f * trMul(2)) breakLeg("The blow snaps something in your leg.");
    play(sHurt, 0.8f);
}
static void addSplat(Vector3 p, float r) {
    splats[splatI] = (Splat){p, r, 90}; splatI = (splatI + 1) % MAX_SPLAT;
}
// the feel of a hit: freeze frames, camera kick, knockback, stagger, blood
static void impactFX(Cr *c, float heavy, bool kill, Vector3 dir) {
    heavy = clampf(heavy, 0.15f, 1.3f);
    // freeze frames: long enough to feel the weapon stop inside the body
    pl.hitStopT = 0.075f + heavy * 0.075f + (kill ? 0.09f : 0);
    pl.shake = fmaxf(pl.shake, 0.45f + heavy * 0.45f); pl.punch = fmaxf(pl.punch, 0.7f + heavy * 0.5f);
    pl.roll = (fr(0, 1) < 0.5f ? -1.0f : 1.0f) * (0.05f + heavy * 0.06f);
    pl.hitFlash = kill ? 0.16f : 0.09f;
    hitMarkT = kill ? 0.42f : 0.28f; hitMarkKind = kill ? 2 : 0; play(sTick, kill ? 1.0f : 0.6f);
    pl.tpitch -= 0.03f + heavy * 0.035f; pl.tyaw += fr(-1, 1) * 0.02f * heavy;
    c->kb = Vector3Add(c->kb, Vector3Scale(dir, 6.0f + heavy * 9.0f));
    c->stun = fmaxf(c->stun, 0.35f + heavy * 0.45f);
    c->flinch = 1.0f; c->hurt = 0.22f;
    for (int k = 0; k < 6; k++) c->tw[k] = fr(-1.2f, 1.2f);   // the body jerks
    Vector3 cc = crCenter(c);
    int np = 22 + (int)(heavy * 26) + (kill ? 20 : 0);
    for (int k = 0; k < np; k++) for (int i = 0; i < MAX_PART; i++) if (parts[i].life <= 0) {
        Vector3 v = Vector3Add(Vector3Scale(dir, fr(2.0f, 6.5f)), V3(fr(-2.0f, 2.0f), fr(0.8f, 4.0f), fr(-2.0f, 2.0f)));
        parts[i] = (Part){cc, v, fr(0.6f, 1.6f), fr(0.035f, 0.11f), c->pos.y + 0.02f, T_FLESH, rgb(fr(80, 150), 6, 6)};
        break;
    }
    for (int s = 0; s < 2 + (int)(heavy * 2); s++) addSplat(V3(c->pos.x + dir.x * fr(0.3f, 1.8f) + fr(-0.3f, 0.3f), c->pos.y + 0.02f, c->pos.z + dir.z * fr(0.3f, 1.8f) + fr(-0.3f, 0.3f)), fr(0.25f, 0.6f) + heavy * 0.25f);
    if (kill) addSplat(V3(c->pos.x, c->pos.y + 0.021f, c->pos.z), 1.0f + heavy * 0.5f);
    if (audioOk) { SetSoundPitch(sImpact, 1.1f - heavy * 0.35f + fr(-0.05f, 0.05f)); SetSoundVolume(sImpact, gs.sfx); PlaySound(sImpact);
        SetSoundPitch(sBone, fr(0.85f, 1.05f) - heavy * 0.15f); SetSoundVolume(sBone, (kill ? 1.0f : 0.55f) * gs.sfx); PlaySound(sBone); }
    play(sSplat, 0.75f);
}
static void meleeHit(Vector3 eye, Vector3 aim) {
    const ItemDef *w = weaponDef();
    float heavy = clampf((w->weight - 0.3f) / 4.0f + w->knock * 0.25f, 0, 1);
    int best = -1, ghost = -1; float bd = meleeRange(), gd = meleeRange() + 0.45f;
    if (netMode != NM_OFF && netPvp) {   // PvP: another player in reach takes the blow
        int pv = -1; float pd = meleeRange() + 0.3f;
        for (int k = 0; k < MP_MAX; k++) { RPlayer *r = &rps[k]; if (k == mySlot || !r->on || !r->alive) continue;
            Vector3 d = Vector3Subtract(Vector3Add(r->pos, V3(0, 1.1f, 0)), eye); float dist = Vector3Length(d);
            if (dist < pd && dist > 0.01f && Vector3DotProduct(Vector3Scale(d, 1 / dist), aim) > 0.6f && lineOfSight(eye, Vector3Add(r->pos, V3(0, 1.1f, 0)))) { pd = dist; pv = k; } }
        if (pv >= 0) { netPlayerDamage(pv, meleeDmg() * 12, 0.5f); play(sSplat, 0.75f); pl.hitStopT = 0.04f; weaponWear(1); return; }
    }
    for (int i = 0; i < nCr; i++) {
        Cr *c = &crs[i]; if (!c->on || c->state == CS_DEAD) continue;
        Vector3 d = Vector3Subtract(crCenter(c), eye); float dist = Vector3Length(d);
        if (dist > meleeRange() + 0.45f) continue;
        float dot = Vector3DotProduct(Vector3Scale(d, 1 / dist), aim);
        if (dot < 0.6f) continue;
        if (!lineOfSight(eye, crCenter(c))) continue;   // nothing is struck through a wall
        if (c->phantom || c->type == 4) { if (dist < gd) { gd = dist; ghost = i; } continue; }
        if (dist > bd + 0.45f) continue;
        bd = dist; best = i;
    }
    if (best < 0 && ghost >= 0) {   // only an incorporeal thing in reach
        Cr *c = &crs[ghost];
        if (c->phantom) { c->on = false; say("Nothing there."); }
        else { say("The blow passes through the spirit. Only an ofuda harms it."); c->hurt = 0.25f; pl.shake = fmaxf(pl.shake, 0.08f); }
        return;
    }
    if (best >= 0) {
        Cr *c = &crs[best];
        float dmg = meleeDmg();
        if (c->type == 7) dmg *= 1.4f;
        if (c->type == 15) dmg *= 0.55f;   // riot armour
        if (cht.on[CH_ONEHIT]) dmg = 9999;   // mannequins crack easily
        c->hp -= dmg; c->hurt = 0.18f; c->wind = 0; c->atkCd = fmaxf(c->atkCd, 0.7f);
        if (netMode == NM_CLIENT) netCreatureHit(best, dmg);
        Vector3 push = Vector3Normalize(V3(c->pos.x - pl.pos.x, 0, c->pos.z - pl.pos.z));
        if (c->type == 9) heavy *= 0.4f;   // the attendant barely flinches
        bool kill = c->hp <= 0;
        if (c->hanging) { c->hanging = false; c->pos.y = groundAt(c->pos, 0.3f, 4.0f); }
        c->state = kill ? CS_DEAD : CS_CHASE; c->lastKnown = pl.pos; c->losT = 0;
        impactFX(c, heavy * (c->type == 9 ? 1 : w->knock + 0.4f), kill, push);
        pl.sanity -= kill ? 2.5f : 1.0f;
        if (kill) { c->deadT = 0; pl.kills++; playAt(sScreech, c->pos, 30, 0.5f); }
        skGain(SK_STR, kill ? 3 : 1);
        weaponWear(1.0f);
        return;
    }
    // world targets
    Vector3 tip = Vector3Add(eye, Vector3Scale(aim, 1.6f));
    for (int i = 0; i < nDeb; i++) {
        Debris *d = &debs[i]; if (!d->alive) continue;
        Vector3 c = xfP(d->f, Vector3Scale(Vector3Add(d->mn, d->mx), 0.5f));
        if (Vector3Distance(c, tip) > 1.6f) continue;
        float dm = meleeDmg() * (handsType() == IT_CROWBAR || handsType() == IT_SLEDGE || handsType() == IT_PICKAXE || handsType() == IT_TETSUBO || handsType() == IT_WRENCH ? 2.2f : 1.0f);
        d->hp -= dm; play(sWood, 0.9f); spawnParts(c, 6, T_WOOD, WHITE, 1.6f, 0.08f); alertAll(c, 14); weaponWear(0.5f);
        pl.shake = fmaxf(pl.shake, 0.15f); pl.hitStopT = 0.03f;
        if (d->hp <= 0) { d->alive = false; sol[d->sid].off = true; say("The junk pile collapses."); addItem(V3(c.x, d->mn.y, c.z), IT_PLANKS, 1 + ri(2)); if (fr(0, 1) < 0.5f) addItem(V3(c.x + 0.3f, d->mn.y, c.z), IT_ROCK, 1 + ri(2)); }
        else say("Junk piled up here. Keep hitting it.");
        return;
    }
    for (int i = 0; i < nOps; i++) {
        Open *o = &ops[i];
        Vector3 c = opCenter(o);
        if (Vector3Distance(c, tip) > 1.4f) continue;
        if (o->type == OP_WINDOW && o->state == 0) { breakWindow(o); return; }
        if (o->type == OP_DOOR && o->state == 1) {
            o->hp -= meleeDmg(); play(sWood, 0.8f); spawnParts(c, 5, T_WOOD, WHITE, 1.5f, 0.08f); alertAll(c, 15); pl.hitStopT = 0.03f; weaponWear(0.5f);
            if (o->hp <= 0) { o->state = 0; o->locked = false; sol[o->sid].off = true; say("Door forced open."); }
            return;
        }
    }
    for (int i = 0; i < nVend; i++) {
        Vend *v = &vends[i]; if (v->broken) continue;
        Vector3 c = xfP(v->f, Vector3Scale(Vector3Add(v->mn, v->mx), 0.5f));
        if (Vector3Distance(c, tip) > 1.3f) continue;
        v->hp -= meleeDmg(); play(sClang, 0.8f); alertAll(c, 20); pl.hitStopT = 0.03f; weaponWear(0.8f);
        spawnParts(c, 5, T_GLASS, rgb(200, 220, 255), 1.6f, 0.05f);
        if (v->hp <= 0) {
            v->broken = true; if (v->light >= 0) lights[v->light].mode = 2;
            Vector3 out = xfD(v->f, v->face == 0 ? V3(0, 0, -1) : v->face == 2 ? V3(0, 0, 1) : v->face == 1 ? V3(1, 0, 0) : V3(-1, 0, 0));
            Vector3 dp = Vector3Add(V3(c.x, v->mn.y, c.z), Vector3Scale(out, 0.9f));
            addItem(dp, fr(0, 1) < 0.5f ? IT_DRINK : IT_WATER, 1 + ri(2));
            playAt(sGlass, c, 30, 0.8f);
        }
        return;
    }
    for (int i = 0; i < nCrate; i++) {
        Crate *k = &crates[i]; if (!k->alive) continue;
        if (Vector3Distance(Vector3Add(k->pos, V3(0, k->s / 2, 0)), tip) > 1.3f) continue;
        k->hp -= meleeDmg(); play(sWood, 0.8f); spawnParts(Vector3Add(k->pos, V3(0, k->s / 2, 0)), 6, T_WOOD, WHITE, 1.6f, 0.08f); pl.hitStopT = 0.03f;
        if (k->hp <= 0) { k->alive = false; sol[k->sid].off = true; addItem(k->pos, IT_PLANKS, 2 + ri(2)); if (fr(0, 1) < 0.35f) addItem(k->pos, IT_NAILS, 4 + ri(6)); }
        return;
    }
}
static void shootEx(Vector3 eye, Vector3 aim, float dmg, float range, bool fx) {   // one bullet or pellet: dmg to the body, double to the head
    Ray ray = {eye, aim};
    float best = range; int hitC = -1; bool head = false;
    query(fminf(eye.x, eye.x + aim.x * range), fminf(eye.z, eye.z + aim.z * range), fmaxf(eye.x, eye.x + aim.x * range), fmaxf(eye.z, eye.z + aim.z * range));
    for (int k = 0; k < qn; k++) {
        Solid *s = &sol[qb[k]]; Ray lr = s->f ? (Ray){solL(s, eye), xfDI(s->f, aim)} : ray;
        RayCollision rc = GetRayCollisionBox(lr, (BoundingBox){s->mn, s->mx});
        if (rc.hit && rc.distance < best) best = rc.distance;
    }
    for (int i = 0; i < nCr; i++) {
        Cr *c = &crs[i]; if (!c->on || c->state == CS_DEAD || c->type == 4 || c->phantom) continue;
        Vector3 cc = crCenter(c);
        Vector3 hc = Vector3Add(cc, V3(0, c->type == 2 || c->type == 8 ? 0.1f : c->type == 1 ? 0.9f : 0.55f, 0));
        RayCollision rh = GetRayCollisionSphere(ray, hc, 0.22f);
        if (rh.hit && rh.distance < best) { best = rh.distance; hitC = i; head = true; }
        RayCollision rb = GetRayCollisionSphere(ray, cc, 0.42f);
        if (rb.hit && rb.distance < best) { best = rb.distance; hitC = i; head = false; }
    }
    int hitP = -1;
    if (netMode != NM_OFF && netPvp) for (int k = 0; k < MP_MAX; k++) { RPlayer *r = &rps[k]; if (k == mySlot || !r->on || !r->alive) continue;   // PvP
        RayCollision rp = GetRayCollisionSphere(ray, Vector3Add(r->pos, V3(0, 1.0f, 0)), 0.45f); RayCollision rh = GetRayCollisionSphere(ray, Vector3Add(r->pos, V3(0, 1.62f, 0)), 0.2f);
        if (rh.hit && rh.distance < best) { best = rh.distance; hitP = k; hitC = -1; head = true; } else if (rp.hit && rp.distance < best) { best = rp.distance; hitP = k; hitC = -1; head = false; } }
    if (hitP >= 0) { netPlayerDamage(hitP, (head ? 70 : 34) * dmg / 3.5f, head ? 0 : 1.0f); hitMarkT = 0.3f; hitMarkKind = head ? 1 : 0; play(sTick, 0.7f); }
    for (int i = 0; i < nOps; i++) {
        Open *o = &ops[i]; if (o->type != OP_WINDOW || o->state != 0) continue;
        RayCollision rc = GetRayCollisionBox(o->f ? (Ray){xfI(o->f, eye), xfDI(o->f, aim)} : ray, (BoundingBox){o->mn, o->mx});
        if (rc.hit && rc.distance <= best + 0.01f) breakWindow(o);
    }
    if (hitC >= 0) {
        Cr *c = &crs[hitC];
        { float gd = cht.on[CH_ONEHIT] ? 9999 : (head ? dmg * 2 : dmg); c->hp -= gd; if (netMode == NM_CLIENT) netCreatureHit(hitC, gd); } c->hurt = 0.2f; c->state = c->hp <= 0 ? CS_DEAD : CS_CHASE; c->wind = 0;
        if (c->hanging) { c->hanging = false; c->pos.y = groundAt(c->pos, 0.3f, 4.0f); }
        impactFX(c, 0.7f, c->state == CS_DEAD, aim); if (head && hitMarkKind < 2) hitMarkKind = 1;
        if (c->state == CS_DEAD) { c->deadT = 0; pl.kills++; playAt(sScreech, c->pos, 40, 0.6f); }
    } else if (best < range && fx) spawnParts(Vector3Add(eye, Vector3Scale(aim, best - 0.05f)), 4, T_CONCRETE, rgb(150, 150, 150), 1.2f, 0.04f);
}
static void shoot(Vector3 eye, Vector3 aim) { shootEx(eye, aim, 3.5f, 120, true); alertAll(pl.pos, 70); }
static Vector3 spreadDir(Vector3 aim, float sp) {   // a direction within a cone around aim
    Vector3 up = fabsf(aim.y) < 0.95f ? V3(0, 1, 0) : V3(1, 0, 0), rt = Vector3Normalize(Vector3CrossProduct(aim, up)), u2 = Vector3CrossProduct(rt, aim);
    float a = fr(0, 2 * PI), r = sqrtf(fr(0, 1)) * sp;
    return Vector3Normalize(Vector3Add(aim, Vector3Add(Vector3Scale(rt, cosf(a) * r), Vector3Scale(u2, sinf(a) * r)))); }
static void shootGun(int t, Vector3 eye, Vector3 aim) {   // the pistol and the shotgun
    const ItemDef *g = &idef[t];
    if (t == IT_SHOTGUN) { for (int k = 0; k < 9; k++) shootEx(eye, spreadDir(aim, 0.075f), g->dmg, g->range, k < 3); alertAll(pl.pos, 95); }
    else { shootEx(eye, spreadDir(aim, 0.006f), g->dmg, g->range, true); alertAll(pl.pos, 55); }
}

// ============================================================ building
enum { BK_WALL, BK_DOOR, BK_CRATE, BK_FLOOR, BK_FIRE, BK_CHEST, BK_BED, BK_SHRINE, BK_WARD, BK_GARDEN, BK_SPIKES, BK_ALARM, BK_GENERATOR, BK_LAMP, BK_N };
static const char *bkName[BK_N] = {"Plank Wall", "Door Wall", "Step Crate", "Floor Platform", "Campfire", "Storage Chest", "Bed", "Kamidana (base shrine)", "Spirit Ward",
                                   "Garden Bed", "Spike Trap", "Can Alarm", "Generator", "Work Lamp"};
typedef struct { int t, n; } Cost;
static const Cost bkCost[BK_N][3] = {
    {{IT_PLANKS, 3}, {IT_NAILS, 4}, {-1, 0}}, {{IT_PLANKS, 4}, {IT_NAILS, 6}, {-1, 0}}, {{IT_PLANKS, 2}, {-1, 0}, {-1, 0}}, {{IT_PLANKS, 3}, {IT_NAILS, 4}, {-1, 0}},
    {{IT_PLANKS, 2}, {-1, 0}, {-1, 0}}, {{IT_PLANKS, 4}, {IT_NAILS, 4}, {-1, 0}}, {{IT_PLANKS, 3}, {IT_RAG, 2}, {-1, 0}},
    {{IT_PLANKS, 2}, {IT_OFUDA, 1}, {IT_ROPE, 1}}, {{IT_PLANKS, 2}, {IT_ROPE, 1}, {IT_OFUDA, 1}},
    {{IT_PLANKS, 3}, {IT_SEEDS, 1}, {-1, 0}}, {{IT_PLANKS, 3}, {IT_NAILS, 12}, {-1, 0}}, {{IT_SCRAP, 1}, {IT_NAILS, 2}, {IT_BOTTLE, 1}}, {{IT_SCRAP, 5}, {IT_NAILS, 8}, {IT_BATTERY, 2}}, {{IT_SCRAP, 1}, {IT_PLANKS, 1}, {IT_BATTERY, 1}}};
static const bool bkHammer[BK_N] = {true, true, false, true, false, true, true, false, false, true, true, false, true, true};
#define BASE_R 22.0f
#define WARD_R 7.0f
static bool bkAfford(int k) { for (int i = 0; i < 3; i++) if (bkCost[k][i].t >= 0 && invCount(bkCost[k][i].t) < bkCost[k][i].n) return false; return true; }
static void bkPay(int k) { for (int i = 0; i < 3; i++) if (bkCost[k][i].t >= 0) invTake(bkCost[k][i].t, bkCost[k][i].n); }
static const char *bkCostText(int k) {
    static char b[96]; b[0] = 0;
    for (int i = 0; i < 3; i++) if (bkCost[k][i].t >= 0) { char p[40]; snprintf(p, sizeof p, "%s%d %s", b[0] ? ", " : "", bkCost[k][i].n, idef[bkCost[k][i].t].name); strncat(b, p, sizeof b - strlen(b) - 1); }
    if (bkHammer[k]) strncat(b, " (hammer)", sizeof b - strlen(b) - 1);
    return b;
}
static float bkRot = 0, bkYaw = 0;   // mouse-wheel rotation (degrees) on top of the automatic alignment; the ghost's final yaw (radians)
static float bkAlignYaw(void) {   // inside or beside a turned building, line up with its walls
    query(pl.pos.x - 4, pl.pos.z - 4, pl.pos.x + 4, pl.pos.z + 4);
    for (int k = 0; k < qn; k++) { Solid *s = &sol[qb[k]]; if (s->f) return xfYaw(s->f); }
    return 0;
}
static void buildAddSolid(Build *b) {   // the piece's collision box, turned with it
    float yw = buildYaw(b); int f = 0; if (yw != 0) { Vector3 c = buildC(b); f = newXf(c.x, c.z, yw); }
    b->sid = addSolidF(b->mn, b->mx, f);
}
static bool buildGhost(Vector3 *mn, Vector3 *mx) {
    int K = pl.buildKind;
    Vector3 f = Vector3Normalize(V3(sinf(pl.tyaw), 0, cosf(pl.tyaw)));
    {   float al = bkAlignYaw(), rel = wrapA(pl.tyaw - al); float q = roundf(rel / (PI / 2)) * (PI / 2);
        bkYaw = wrapA(al + q + bkRot * DEG2RAD); }
    Vector3 c = Vector3Add(pl.pos, Vector3Scale(f, (K == BK_WALL || K == BK_DOOR || K == BK_FLOOR) ? 1.9f : 1.4f));
    c.x = roundf(c.x * 2) / 2; c.z = roundf(c.z * 2) / 2;
    float g = K == BK_FLOOR ? pl.pos.y : groundAt(V3(c.x, pl.pos.y + 0.6f, c.z), 0.3f, 0.6f);
    bool ax = false;   // the box is laid out facing +z, then turned by bkYaw
    switch (K) {
    case BK_WALL: case BK_DOOR:
        if (ax) { *mn = V3(c.x - 0.1f, g, c.z - 1.0f); *mx = V3(c.x + 0.1f, g + 2.4f, c.z + 1.0f); } else { *mn = V3(c.x - 1.0f, g, c.z - 0.1f); *mx = V3(c.x + 1.0f, g + 2.4f, c.z + 0.1f); } break;
    case BK_CRATE: *mn = V3(c.x - 0.45f, g, c.z - 0.45f); *mx = V3(c.x + 0.45f, g + 0.9f, c.z + 0.45f); break;
    case BK_FLOOR: *mn = V3(c.x - 1.0f, g - 0.15f, c.z - 1.0f); *mx = V3(c.x + 1.0f, g, c.z + 1.0f); break;
    case BK_CHEST: *mn = V3(c.x - 0.5f, g, c.z - 0.3f); *mx = V3(c.x + 0.5f, g + 0.6f, c.z + 0.3f); break;
    case BK_BED: if (ax) { *mn = V3(c.x - 1.0f, g, c.z - 0.5f); *mx = V3(c.x + 1.0f, g + 0.45f, c.z + 0.5f); } else { *mn = V3(c.x - 0.5f, g, c.z - 1.0f); *mx = V3(c.x + 0.5f, g + 0.45f, c.z + 1.0f); } break;
    case BK_SHRINE: *mn = V3(c.x - 0.45f, g, c.z - 0.3f); *mx = V3(c.x + 0.45f, g + 1.3f, c.z + 0.3f); break;
    case BK_WARD: if (ax) { *mn = V3(c.x - 0.05f, g, c.z - 0.9f); *mx = V3(c.x + 0.05f, g + 1.6f, c.z + 0.9f); } else { *mn = V3(c.x - 0.9f, g, c.z - 0.05f); *mx = V3(c.x + 0.9f, g + 1.6f, c.z + 0.05f); } break;
    case BK_GARDEN: if (ax) { *mn = V3(c.x - 0.5f, g, c.z - 0.9f); *mx = V3(c.x + 0.5f, g + 0.3f, c.z + 0.9f); } else { *mn = V3(c.x - 0.9f, g, c.z - 0.5f); *mx = V3(c.x + 0.9f, g + 0.3f, c.z + 0.5f); } break;
    case BK_SPIKES: *mn = V3(c.x - 0.55f, g, c.z - 0.55f); *mx = V3(c.x + 0.55f, g + 0.25f, c.z + 0.55f); break;
    case BK_ALARM: *mn = V3(c.x - 0.2f, g, c.z - 0.2f); *mx = V3(c.x + 0.2f, g + 0.5f, c.z + 0.2f); break;
    case BK_GENERATOR: *mn = V3(c.x - 0.45f, g, c.z - 0.35f); *mx = V3(c.x + 0.45f, g + 0.6f, c.z + 0.35f); break;
    case BK_LAMP: *mn = V3(c.x - 0.25f, g, c.z - 0.25f); *mx = V3(c.x + 0.25f, g + 1.7f, c.z + 0.25f); break;
    default: *mn = V3(c.x - 0.4f, g, c.z - 0.4f); *mx = V3(c.x + 0.4f, g + 0.4f, c.z + 0.4f); break;
    }
    if (K == BK_WARD) {
        if (baseShrine < 0 || !builds[baseShrine].alive) return false;
        Vector3 sc = builds[baseShrine].mn; if (Vector3Distance(V3(sc.x, 0, sc.z), V3(c.x, 0, c.z)) > BASE_R) return false;
    }
    if (K == BK_FIRE || K == BK_WARD || K == BK_SPIKES || K == BK_ALARM) return true;
    Vector3 lmn = *mn, lmx = *mx;   // test the turned box by its world bounds
    {   float cx = (mn->x + mx->x) / 2, cz = (mn->z + mx->z) / 2, hx = (mx->x - mn->x) / 2, hz = (mx->z - mn->z) / 2, c = fabsf(cosf(bkYaw)), s2 = fabsf(sinf(bkYaw));
        float ex = hx * c + hz * s2, ez = hx * s2 + hz * c; *mn = V3(cx - ex, mn->y, cz - ez); *mx = V3(cx + ex, mx->y, cz + ez); }
    bool okk = true;
    query(mn->x, mn->z, mx->x, mx->z);
    for (int k = 0; k < qn; k++) {
        Solid *s = &sol[qb[k]];
        if (s->mx.y <= mn->y + 0.05f || s->mn.y >= mx->y) continue;
        float a0 = mn->x, a1 = mx->x, b0 = mn->z, b1 = mx->z;
        if (s->f) { a0 = b0 = 1e9f; a1 = b1 = -1e9f; for (int q = 0; q < 4; q++) { Vector3 L = solL(s, V3(q & 1 ? mx->x : mn->x, 0, q & 2 ? mx->z : mn->z)); a0 = fminf(a0, L.x); a1 = fmaxf(a1, L.x); b0 = fminf(b0, L.z); b1 = fmaxf(b1, L.z); } }
        if (a0 < s->mx.x && a1 > s->mn.x && b0 < s->mx.z && b1 > s->mn.z) { okk = false; break; }
    }
    float pr = 0.35f;
    if (okk && K != BK_FLOOR && pl.pos.x + pr > mn->x && pl.pos.x - pr < mx->x && pl.pos.z + pr > mn->z && pl.pos.z - pr < mx->z) okk = false;
    *mn = lmn; *mx = lmx;
    return okk;
}
static void placeBuild(void) {
    int K = pl.buildKind; Vector3 mn, mx;
    bool ok = buildGhost(&mn, &mx);
    if (K == BK_WARD && (baseShrine < 0 || !builds[baseShrine].alive)) { say("Spirit wards only work inside a base. Build a Kamidana first."); return; }
    if (K == BK_WARD && !ok) { say("Too far from your Kamidana. Wards must stand inside the base."); return; }
    if (!ok) { say("Can't build there."); return; }
    if (bkHammer[K] && !invHas(IT_HAMMER)) { say("You need a claw hammer to build that."); return; }
    if (!bkAfford(K)) { say(TextFormat("Needs %s.", bkCostText(K))); return; }
    if (K == BK_SHRINE && baseShrine >= 0 && builds[baseShrine].alive) { say("You already have a Kamidana. One base at a time."); return; }
    Vector3 c = Vector3Scale(Vector3Add(mn, mx), 0.5f);
    if (K == BK_FIRE) {
        int slot = -1; for (int i = 0; i < nFire; i++) if (!fires[i].on) { slot = i; break; }
        if (slot < 0 && nFire < MAX_FIRE) slot = nFire++;
        if (slot < 0) return;
        c.y = mn.y;
        int li = fires[slot].light;
        if (!(slot < nFire && li > 0 && li < nLight)) li = addLight(V3(c.x, c.y + 0.8f, c.z), V3(1.0f, 0.55f, 0.2f), 10, 1);
        else { lights[li].pos = V3(c.x, c.y + 0.8f, c.z); lights[li].mode = 1; }
        fires[slot] = (Fire){c, 300, li, true};
        bkPay(K); pl.boredom = fmaxf(0, pl.boredom - 10); play(sWood, 0.7f); say("Campfire lit."); return;
    }
    int bi = -1; for (int i = 0; i < nBuild; i++) if (!builds[i].alive) { bi = i; break; }
    if (bi < 0) { if (nBuild >= MAX_BUILD) { say("Too many structures."); return; } bi = nBuild++; }
    Build *b = &builds[bi]; memset(b, 0, sizeof *b);
    b->mn = mn; b->mx = mx; b->kind = K; b->alive = true; b->cont = -1; b->sid = -1;
    {   int deg = ((int)lroundf(bkYaw * RAD2DEG) % 360 + 360) % 360;
        if (deg % 90 == 0) {   // square to the world: keep a plain world box
            if (deg % 180 == 90) { float cx = (mn.x + mx.x) / 2, cz = (mn.z + mx.z) / 2, hx = (mx.x - mn.x) / 2, hz = (mx.z - mn.z) / 2;
                b->mn = V3(cx - hz, mn.y, cz - hx); b->mx = V3(cx + hz, mx.y, cz + hx); }
            b->axis = (b->mx.x - b->mn.x) < (b->mx.z - b->mn.z) ? 1 : 0;
        } else b->axis = 1000 + deg; }
    mn = b->mn; mx = b->mx;
    float carp = 1 + 0.12f * skLvl(SK_CARP);
    b->maxhp = b->hp = (K == BK_WALL ? 10 : K == BK_DOOR ? 9 : K == BK_FLOOR ? 12 : 6) * carp;
    if (K == BK_GARDEN) { b->hp = 0; b->maxhp = 36; b->open = true; say("Daikon planted. Ready to harvest in about a day and a half."); }   // hp = hours grown, open = planted
    if (K == BK_SPIKES) { b->hp = b->maxhp = 6; say("Spike trap set. Six bites before it needs rebuilding."); }
    if (K == BK_ALARM) { b->hp = b->maxhp = 1; say("Can alarm strung. It rattles when something walks into it."); }
    if (K != BK_SHRINE && K != BK_WARD && K != BK_GARDEN && K != BK_SPIKES && K != BK_ALARM) buildAddSolid(b);
    if (K == BK_CHEST) { b->cont = addCont(CK_CHEST, V3(c.x, mn.y, c.z), b->axis >= 1000 ? buildYaw(b) : b->axis ? PI / 2 : 0, 0, false); }
    if (K == BK_SHRINE) { baseShrine = bi; addWard(V3(c.x, mn.y, c.z), 11.0f, 0, 1); say("Kamidana raised. Spirits cannot come within 11 m of it; spirit wards extend the base (22 m)."); }
    if (K == BK_WARD) { addWard(V3(c.x, mn.y, c.z), WARD_R, 0, 1); say("Spirit ward hung. Spirits cannot enter its circle."); }
    if (K == BK_DOOR) say("Door wall built. [E] opens and closes it.");
    if (K == BK_BED) say("Bed built. Sleep here with [Z].");
    if (K == BK_GENERATOR) { b->fuel = 0; b->light = -1; b->hp = b->maxhp = 8; say("Generator built. Pour in petrol [G], start it [E]. Lights within 40 m come on - and it is loud."); }
    if (K == BK_LAMP) { b->light = addLight(V3(c.x, mx.y - 0.1f, c.z), V3(1.0f, 0.95f, 0.8f), 15, 2); say("Work lamp set up. It lights when a running generator is within 40 m."); }
    bkPay(K); play(sPlace, 0.9f); pl.boredom = fmaxf(0, pl.boredom - 15); animPlay(ANIM_BUILD);
    if (bkHammer[K]) skGain(SK_CARP, 4);
}
static int facingBuild(Vector3 eye, Vector3 aim, float maxD, int kind) {
    int best = -1; float bd = maxD;
    for (int i = 0; i < nBuild; i++) {
        Build *b = &builds[i]; if (!b->alive || (kind >= 0 && b->kind != kind)) continue;
        Vector3 c = Vector3Scale(Vector3Add(b->mn, b->mx), 0.5f);
        Vector3 d = Vector3Subtract(c, eye); float L = Vector3Length(d);
        if (L > bd || L < 0.01f) continue;
        if (Vector3DotProduct(Vector3Scale(d, 1 / L), aim) < 0.6f) continue;
        bd = L; best = i;
    }
    return best;
}
static void spiritSeal(void) {
    Vector3 eye = Vector3Add(pl.pos, V3(0, 1.6f, 0)), aim = dirOf(pl.tyaw, pl.tpitch);
    int best = -1; float bd = 5.0f;
    for (int i = 0; i < nCr; i++) { Cr *c = &crs[i]; if (!c->on || c->type != 4 || c->state == CS_DEAD) continue;
        Vector3 d = Vector3Subtract(crCenter(c), eye); float dist = Vector3Length(d);
        if (dist < bd && Vector3DotProduct(Vector3Scale(d, 1 / dist), aim) > 0.7f) { bd = dist; best = i; } }
    if (best >= 0) { invTake(IT_OFUDA, 1); crs[best].state = CS_DEAD; crs[best].deadT = 0; pl.kills++; play(sScreech, 0.7f);
        spawnParts(crCenter(&crs[best]), 12, T_PAPER, WHITE, 1.5f, 0.06f); say("The ofuda seals the spirit."); }
    else say("No spirit within reach.");
}
static int facingCont(Vector3 eye, Vector3 aim, float maxD) {
    int best = -1; float bd = maxD;
    for (int i = 0; i < nCont; i++) {
        Cont *c = &conts[i]; if (!c->on) continue;
        if ((c->kind == CK_PILE || (c->kind == CK_BODY && c->var >= 100 && c->searched)) && contEmpty(c)) { c->on = false; continue; }   // empty piles and picked-clean creature bodies go
        Vector3 p = Vector3Add(c->pos, V3(0, ckSize[c->kind].y * 0.5f, 0));
        Vector3 d = Vector3Subtract(p, eye); float L = Vector3Length(d);
        if (L > bd + (c->kind == CK_PILE ? 0.3f : 0) || L < 0.01f) continue;
        if (Vector3DotProduct(Vector3Scale(d, 1 / L), aim) < (L < 1.2f ? 0.35f : 0.75f)) continue;
        bd = L; best = i;
    }
    return best;
}
static float searchTime(int k) { return k == CK_PILE || k == CK_CHEST ? 0 : (k == CK_TRUNK || k == CK_BODY ? 2.0f : 1.4f) * (1 - 0.12f * skLvl(SK_SCAV)); }
static void openInventory(int contIdx) {
    pl.invOpen = true; pl.openC = contIdx; pl.dragFrom = -1; pl.drag.t = -1;
    EnableCursor();
}

// ============================================================ update
static void updatePlayer(float dt) {
    pl.time += dt;
    // ---- animation clock and springs
    inputLocked = animId == ANIM_TURNGUN;
    if (animId == ANIM_TURNGUN) {
        pl.tpitch += (0.02f - pl.tpitch) * fminf(1, dt * 2); pl.shake = fmaxf(pl.shake, 0.08f + 0.15f * clampf(animT / animDur, 0, 1));
        if (animT + dt >= animDur) { play(sShot, 1.0f); pl.muzzle = 0.07f; selfShot = true; pl.hp = 0; inputLocked = false; }
    }
    if (animId != ANIM_NONE) { animT += dt; if (animT >= animDur) { if (animId == ANIM_HOLSTER) {} animId = ANIM_NONE; animItem = -1; } }
    { Vector2 m = (pl.invOpen || pl.mapOpen) ? (Vector2){0, 0} : MDel(); float tx = clampf(-m.x * 0.0007f, -0.05f, 0.05f), ty = clampf(m.y * 0.0007f, -0.04f, 0.04f);
      // springs, slightly under-damped: the weapon lags behind fast turns, overshoots a touch and settles
      const float K = 210, C = 2 * 0.58f * 14.5f;
      vmVx += ((tx - pl.swayX) * K - vmVx * C) * dt; pl.swayX += vmVx * dt;
      vmVy += ((ty - pl.swayY) * K - vmVy * C) * dt; pl.swayY += vmVy * dt;
      vmRollV += ((vmStrafe - vmRoll) * 160 - vmRollV * 2 * 0.6f * 12.6f) * dt; vmRoll += vmRollV * dt;
      vmPushV += (-vmPushZ * 240 - vmPushV * 2 * 0.5f * 15.5f) * dt; vmPushZ += vmPushV * dt;
      hitMarkT -= dt;
      pl.landK *= expf(-dt * 7); pl.blockK += ((pl.blocking ? 1.0f : 0.0f) - pl.blockK) * fminf(1, dt * 14);
      pl.eyeK += ((pl.sleeping ? 1.0f : 0.0f) - pl.eyeK) * fminf(1, dt * (pl.sleeping ? 1.6f : 3.0f));
      pl.readFlip = 0; }   // (page turns: drawManga keeps its own time)
    pl.weapon = pl.hands.t == IT_REVOLVER ? 1 : pl.hands.t == IT_PISTOL ? 2 : pl.hands.t == IT_SHOTGUN ? 3 : 0;
    // ---- GoldenEye-style aim: crosshair leads, view follows with lag
    Vector2 md = (pl.invOpen || pl.mapOpen) ? (Vector2){0, 0} : MDel();
    pl.tyaw -= md.x * cfg.sens; pl.tpitch -= md.y * cfg.sens;
    if (pl.painT <= 0 && pl.hp < 50) { pl.tyaw += sinf(pl.time * 9) * (50 - pl.hp) * 0.00004f; pl.tpitch += cosf(pl.time * 7) * (50 - pl.hp) * 0.00003f; }   // blood loss: shaking aim
    pl.tpitch = clampf(pl.tpitch, -1.35f, 1.35f);
    float maxOffY = 0.42f, maxOffP = 0.30f;
    float dy = wrapA(pl.tyaw - pl.yaw);
    if (dy > maxOffY) { pl.yaw = pl.tyaw - maxOffY; } if (dy < -maxOffY) { pl.yaw = pl.tyaw + maxOffY; }
    float dp = pl.tpitch - pl.pitch;
    if (dp > maxOffP) pl.pitch = pl.tpitch - maxOffP; if (dp < -maxOffP) pl.pitch = pl.tpitch + maxOffP;
    float k = 1.0f - expf(-dt * cfg.aimLag);
    pl.yaw += wrapA(pl.tyaw - pl.yaw) * k; pl.pitch += (pl.tpitch - pl.pitch) * k;
    pl.yaw = wrapA(pl.yaw); pl.tyaw = pl.yaw + wrapA(pl.tyaw - pl.yaw);
    Vector3 aim = dirOf(pl.tyaw, pl.tpitch);
    if (pl.sleeping || pl.watchT > 0) {
        if (pl.sleeping) { pl.rest = fminf(100, pl.rest + dt * 100.0f / 40.0f); if (pl.rest >= 100 || actP(A_SLEEP)) { pl.sleeping = false; say("You wake up."); if (pl.rest >= 100) play(sPhone, 0.35f); else play(sSigh, 0.5f); }   /* the phone alarm, or you get up yourself */ }
        else {
            pl.watchT -= dt;
            if (pl.watchT <= 0) {
                if (pl.watchKind == 1) { pl.sanity += 30; pl.boredom = 0; say("For a while, the world felt normal."); }
                else if (pl.watchKind == 2) { pl.sanity += 18; pl.boredom = fmaxf(0, pl.boredom - 60); say("You finish the manga."); }
                else { pl.sanity += 4; pl.boredom = fmaxf(0, pl.boredom - 25); say("\"...a bright, green future.\""); }
            }
        }
        prompt[0] = 0;
        goto survival;
    }
    if (pl.stage == 3 && !pl.invOpen) {   // insanity: involuntary actions
        pl.involT -= dt;
        if (pl.involT <= 0) {
            pl.involT = fr(1.5f, 6.0f) * (1.25f - pl.corrupt / 100.0f);
            int kk = ri(3);
            if (kk == 0) { pl.tyaw += fr(-0.9f, 0.9f); pl.tpitch += fr(-0.3f, 0.3f); }
            else if (kk == 1 && pl.weapon == 0 && !isConsumable(pl.hands.t) && pl.swingCd <= 0) { pl.swingDur = pl.swingT = pl.swingCd = meleeCd(); pl.hitDone = false; play(sSwing, 0.6f); netSwingN++; }
            else { pl.pos.x += fr(-0.8f, 0.8f); pl.pos.z += fr(-0.8f, 0.8f); resolve(&pl.pos, 0.3f, 1.7f, 0.42f); }
        }
    }
    // ---- climb / vault animation
    if (pl.climbT > 0) {
        pl.climbT -= dt;
        float t = 1 - clampf(pl.climbT / pl.climbDur, 0, 1);
        Vector3 a = pl.climbFrom, b = pl.climbTo;
        if (pl.climbKind == 0) {
            float t1 = clampf(t / 0.6f, 0, 1), t2 = clampf((t - 0.6f) / 0.4f, 0, 1);
            pl.pos.y = Lerp(a.y, b.y + 0.05f, t1 * t1 * (3 - 2 * t1));
            pl.pos.x = Lerp(a.x, b.x, t2); pl.pos.z = Lerp(a.z, b.z, t2);
        } else { pl.pos = Vector3Lerp(a, b, t); pl.pos.y = Lerp(a.y, b.y, t) + sinf(t * PI) * 0.85f; }
        if (pl.climbT <= 0) { pl.pos = b; pl.vy = 0; pl.ground = true; }
        return;
    }
    // ---- movement (weight slows you; swinging heavy weapons slows you more)
    Vector3 fwd = V3(sinf(pl.yaw), 0, cosf(pl.yaw)), right = V3(-cosf(pl.yaw), 0, sinf(pl.yaw));
    Vector3 mv = {0};
    { Vector2 am = pl.mapOpen ? (Vector2){0, 0} : actMove(); mv = Vector3Add(Vector3Scale(fwd, am.y), Vector3Scale(right, am.x)); vmStrafe = clampf(am.x, -1, 1);
      static float lastFwd = 0; float fw = clampf(am.y, -1, 1); if (fabsf(fw - lastFwd) > 0.5f) vmPushV += (fw - lastFwd) * 0.35f; lastFwd = fw; }   // start / stop: the hands lag
    bool moving = Vector3Length(mv) > 0.1f;
    if (moving) { float ml = Vector3Length(mv); mv = Vector3Scale(mv, (ml > 1 ? 1 : ml < 0.45f ? 0.45f : ml) / ml); }   // a half-tilted stick walks slower
    bool sneak = actHeld(A_CROUCH);
    bool sprint = actHeld(A_SPRINT) && moving && !pl.exhausted && pl.stam > 0 && !pl.invOpen && pl.fracture == 0 && !pl.blocking;
    float spd = (sprint ? 6.6f : sneak ? 1.7f : 3.8f) * encSpeed() * cht.speed;
    if (pl.swingT > 0) spd *= 1.0f - 0.45f * clampf(weaponDef()->weight / 5.0f, 0.15f, 1);
    if (pl.stage >= 2) spd *= 0.9f;
    if (pl.hp < 30) spd *= 0.85f;
    if (pl.fracture == 1) spd *= 0.5f; else if (pl.fracture == 2) spd *= 0.78f;
    if (pl.blocking) spd *= 0.55f;
    pl.sprintK += ((sprint ? 1.0f : 0.0f) - pl.sprintK) * fminf(1, dt * 8);
    if (pl.sick > 40) spd *= 0.9f;
    float stamMul = encStam() * (1 - 0.07f * skLvl(SK_FIT)) * trMul(0);
    if (sprint) { pl.stam -= 18 * dt * stamMul; if (pl.stam <= 0) { pl.stam = 0; pl.exhausted = true; } alertAll(pl.pos, 13 * (1 - 0.08f * skLvl(SK_STEALTH)) * trMul(6)); skGain(SK_FIT, dt * 0.25f); }
    else pl.stam = fminf(100, pl.stam + (moving ? 8 : 15) * dt * (pl.stage >= 1 ? 0.75f : 1.0f) / encStam());
    if (sneak && moving) { bool near = false; for (int i = 0; i < nCr; i++) if (crs[i].on && crs[i].state == CS_WANDER && Vector3Distance(crs[i].pos, pl.pos) < 14) near = true; if (near) skGain(SK_STEALTH, dt * 0.35f); }
    if (pl.exhausted && pl.stam > 30) pl.exhausted = false;
    if (pl.food <= 0 || pl.water <= 0) spd *= 0.85f;
    if (cht.on[CH_NOCLIP]) {   // cheat: fly through everything where you look
        Vector3 f3 = dirOf(pl.tyaw, pl.tpitch), v = {0};
        { Vector2 am = pl.mapOpen ? (Vector2){0, 0} : actMove(); v = Vector3Add(Vector3Scale(f3, am.y), Vector3Scale(right, am.x)); }
        if (actD(A_JUMP)) v.y += 1; if (actD(A_CROUCH)) v.y -= 1;
        if (Vector3Length(v) > 0.01f) pl.pos = Vector3Add(pl.pos, Vector3Scale(Vector3Normalize(v), (actD(A_SPRINT) ? 26.0f : 9.0f) * cht.speed * dt));
        pl.vy = 0; pl.ground = false; moving = false;
        goto cheatMoved;
    }
    pl.pos.x += mv.x * spd * dt; pl.pos.z += mv.z * spd * dt;
    resolve(&pl.pos, 0.3f, 1.7f, 0.42f);
    pl.vy -= 18.0f * dt;
    pl.pos.y += pl.vy * dt;
    float g = groundAt(pl.pos, 0.3f, 0.42f);
    if (pl.pos.y <= g && pl.vy < -4) { pl.landK = fminf(1.5f, -pl.vy / 9); vmVy -= pl.landK * 0.5f; play(sLand, fminf(1, 0.3f + pl.landK * 0.4f)); }
    if (pl.pos.y <= g) { if (pl.vy < -12) { hurtPlayer((-pl.vy - 12) * 5, 0.5f); if (pl.fracture == 0 && fr(0, 1) < 0.55f * trMul(2) * (pl.wear[WS_FEET].t == IT_BOOTS ? 0.5f : 1.0f)) breakLeg("You land badly. Your leg is broken."); }
        else if (pl.vy < -9.5f && pl.fracture == 0 && fr(0, 1) < 0.08f * trMul(2) * (pl.wear[WS_FEET].t == IT_BOOTS ? 0.5f : 1.0f)) breakLeg("You land badly. Your leg is broken.");
        pl.pos.y = g; pl.vy = 0; pl.ground = true; }
    else if (pl.pos.y - g < 0.45f && pl.vy <= 0 && pl.ground) { pl.pos.y = g; pl.vy = 0; }
    else pl.ground = false;
    if (pl.pos.y < -60) { pl.pos = V3(SPAWN_X, 0, SPAWN_Z); pl.vy = 0; }
    float ceil = ceilAt(pl.pos, 0.3f);
    if (pl.pos.y + 1.75f > ceil && pl.vy > 0) { pl.vy = 0; pl.pos.y = ceil - 1.75f; }
    cheatMoved:
    if (moving && pl.ground) {
        pl.bob += dt * spd * 1.6f; pl.stepAcc += spd * dt;
        if (pl.stepAcc > (sprint ? 2.2f : 1.7f)) { pl.stepAcc = 0; play(sStep, sprint ? 0.5f : sneak ? 0.12f : 0.3f); }
    }
    // ---- interaction targets
    prompt[0] = 0;
    Vector3 eye = Vector3Add(pl.pos, V3(0, 1.6f, 0));
    int cont = pl.invOpen ? -1 : facingCont(eye, aim, 2.3f);
    int vaultOp = facingOpening(pl.pos, aim, 1.5f, OP_WINDOW, 1);
    int barrOp = -1, useDoor = -1;
    { int a = facingOpening(pl.pos, aim, 2.0f, OP_WINDOW, 1); int b = facingOpening(pl.pos, aim, 2.0f, OP_DOOR, 0); barrOp = a >= 0 ? a : b; }
    useDoor = facingOpening(pl.pos, aim, 2.0f, OP_DOOR, 1); if (useDoor < 0) useDoor = facingOpening(pl.pos, aim, 2.0f, OP_DOOR, 0);
    Vector3 ledge; bool hasLedge = findLedge(pl.pos, Vector3Normalize(V3(aim.x, 0, aim.z)), &ledge);
    int nearTV = -1;
    for (int i = 0; i < nTV; i++) { Vector3 d = V3(tvs[i].pos.x - pl.pos.x, 0, tvs[i].pos.z - pl.pos.z); float L = Vector3Length(d);
        if (L < 1.9f && fabsf(tvs[i].pos.y - pl.pos.y - 0.8f) < 1.0f && Vector3DotProduct(Vector3Scale(d, 1 / fmaxf(L, 0.01f)), Vector3Normalize(V3(aim.x, 0, aim.z))) > 0.5f) { nearTV = i; break; } }
    int door = facingBuild(eye, aim, 2.4f, BK_DOOR), myWall = -1;
    { int w = facingBuild(eye, aim, 2.4f, -1); if (w >= 0 && (builds[w].kind == BK_WALL || builds[w].kind == BK_DOOR || builds[w].kind == BK_FLOOR || builds[w].kind == BK_CRATE)) myWall = w; }
    int bed = facingBuild(eye, aim, 2.4f, BK_BED);
    if (pl.searchC >= 0) {
        Cont *c = &conts[pl.searchC];
        if (!c->on || Vector3Distance(c->pos, pl.pos) > 2.8f || !actD(A_USE)) { pl.searchC = -1; pl.searchT = 0; }
        else {
            { float before = pl.searchT; pl.searchT += dt; if ((int)(before / 0.45f) != (int)(pl.searchT / 0.45f) || before == 0) play(sRummage, 0.45f); }
            float need = searchTime(c->kind);
            snprintf(prompt, sizeof prompt, "Searching %s ...", ckName[c->kind]);
            if (pl.searchT >= need) {
                bool fresh = !c->filled; lootFill(c);
                if (fresh && skLvl(SK_SCAV) >= 2 && fr(0, 1) < 0.08f * skLvl(SK_SCAV)) slotsAdd(c->s, ckSlots[c->kind], IT_RAG, 1);
                if (!c->searched) { c->searched = true; skGain(SK_SCAV, 2); }
                openInventory(pl.searchC); pl.searchC = -1; pl.searchT = 0; play(sPick, 0.4f);
            }
        }
    } else if (!pl.invOpen) {
        if (cont < 0 && extraPrompt(eye, aim)) {}
        else if (cont < 0 && survPrompt(eye, aim, false)) {}
        else if (nearTV >= 0 && cont < 0) snprintf(prompt, sizeof prompt, invHas(IT_VHS) ? "[E] Watch anime (VHS)" : "[E] Watch TV");
        else if (cont >= 0) { Cont *c = &conts[cont]; snprintf(prompt, sizeof prompt, c->kind == CK_PILE || c->kind == CK_CHEST ? "[E] Open %s" : c->searched ? "[hold E] %s (searched)" : "[hold E] Search %s", ckName[c->kind]); }
        else if (door >= 0) snprintf(prompt, sizeof prompt, builds[door].open ? "[E] Close door" : "[E] Open door");
        else if (bed >= 0) snprintf(prompt, sizeof prompt, "[Z] Sleep in bed");
        else if (vaultOp >= 0) snprintf(prompt, sizeof prompt, "[SPACE] Vault through window%s", barrOp >= 0 ? "   [G] Barricade (2 planks)" : "");
        else if (barrOp >= 0) snprintf(prompt, sizeof prompt, "%s[G] Barricade (%d planks)", useDoor >= 0 && useDoor == barrOp ? "[E] Close door   " : "", ops[barrOp].type == OP_WINDOW ? 2 : 3);
        else if (myWall >= 0 && builds[myWall].hp < builds[myWall].maxhp) snprintf(prompt, sizeof prompt, "[G] Repair (1 plank)  -  %d%%", (int)(100 * builds[myWall].hp / builds[myWall].maxhp));
        else if (myWall >= 0 && invHas(IT_SCRAP)) snprintf(prompt, sizeof prompt, "[G] Reinforce with scrap metal");
        else if (hasLedge && pl.ground) snprintf(prompt, sizeof prompt, "[SPACE] Climb");
        else if (useDoor >= 0) snprintf(prompt, sizeof prompt, "%s", ops[useDoor].state == 0 ? "[E] Close door" : ops[useDoor].locked ? (invHas(IT_KEY) ? "[E] Unlock with a spare key" : "Locked - strike it to force it open") : "[E] Open door");
        else { int w = facingOpening(pl.pos, aim, 1.6f, OP_WINDOW, 0); if (w >= 0) snprintf(prompt, sizeof prompt, "Glass window - strike to break"); }
    }
    if (actP(A_USE) && !pl.invOpen && pl.searchC < 0) {
        if (cont >= 0) { Cont *c = &conts[cont]; if (searchTime(c->kind) <= 0) { lootFill(c); openInventory(cont); } else { pl.searchC = cont; pl.searchT = 0; } }
        else if (facingBuild(eye, aim, 2.4f, BK_GENERATOR) >= 0) {}   // handled by extraPrompt
        else if (survPrompt(eye, aim, true)) {}
        else if (nearTV >= 0) { if (invTake(IT_VHS, 1)) { pl.watchKind = 1; pl.watchT = 8; play(sTape, 0.6f); play(sTV, 0.35f); } else { pl.watchKind = 0; pl.watchT = 4; play(sTV, 0.3f); play(sJingle, 0.45f); } }
        else if (door >= 0) { animPlay(ANIM_DOOR); Build *b = &builds[door]; b->open = !b->open; sol[b->sid].off = b->open; play(sWood, 0.5f); }
        else if (useDoor >= 0) { Open *o = &ops[useDoor];   // a building's door: open or shut it (a locked one has to be forced)
            if (o->state == 1 && o->locked && invTake(IT_KEY, 1)) { o->locked = false; o->state = 0; if (o->sid >= 0) sol[o->sid].off = true; animPlay(ANIM_DOOR); play(sClick, 0.6f); play(sWood, 0.4f); say("One of the spare keys fits. The door swings open."); }
            else if (o->state == 1 && o->locked) { play(sClick, 0.5f); say("Locked."); }
            else { o->state = o->state == 1 ? 0 : 1; if (o->sid >= 0) sol[o->sid].off = o->state == 0; animPlay(ANIM_DOOR); play(sWood, 0.45f); alertAll(opCenter(o), 6); } }
    }
    if (actP(A_MAP) && !pl.invOpen) { pl.mapOpen = !pl.mapOpen; play(sPaper, 0.45f); if (pl.mapOpen) EnableCursor(); else DisableCursor(); }
    if (pl.mapOpen) goto survival_ui;
    if (actP(A_INV)) { play(sCloth, 0.45f); if (pl.invOpen) { pl.invOpen = false; pl.openC = -1; DisableCursor(); } else { int pile = contAt(pl.pos, 1.6f, CK_PILE); openInventory(pile); } }
    if (pl.invOpen && pl.openC >= 0 && Vector3Distance(conts[pl.openC].pos, pl.pos) > 3.0f) pl.openC = -1;
    if (pl.invOpen) goto survival_ui;
    if (actP(A_JUMP)) {
        if (vaultOp >= 0) {
            Open *o = &ops[vaultOp]; Vector3 c = opCenter(o), n = opNormal(o);
            float sd = Vector3DotProduct(V3(pl.pos.x - c.x, 0, pl.pos.z - c.z), n);
            Vector3 dest = Vector3Add(V3(c.x, o->fy, c.z), Vector3Scale(n, sd > 0 ? -0.95f : 0.95f));
            pl.climbFrom = pl.pos; pl.climbTo = dest; pl.climbDur = pl.climbT = 0.75f; pl.climbKind = 1; play(sStep, 0.6f);
        } else if (hasLedge && pl.fracture == 1) say("Not with a broken leg.");
        else if (hasLedge) {
            float h = ledge.y - pl.pos.y;
            pl.climbFrom = pl.pos; pl.climbTo = ledge; pl.climbDur = pl.climbT = 0.35f + h * 0.28f; pl.climbKind = 0;
            pl.stam -= 8; play(sStep, 0.6f);
        } else if (pl.ground && pl.fracture != 1) { pl.vy = 5.6f; pl.ground = false; pl.stam -= 6; vmVy -= 0.35f; play(sJump, 0.35f); }
    }
    if (actP(A_BARR)) {
        if (barrOp >= 0) {
            Open *o = &ops[barrOp]; int cost = o->type == OP_WINDOW ? 2 : 3;
            if (invCount(IT_PLANKS) >= cost) {
                invTake(IT_PLANKS, cost); o->state = 2; o->hp = (o->type == OP_WINDOW ? 6 : 8) * (1 + 0.12f * skLvl(SK_CARP));
                Vector3 mn = o->mn, mx = o->mx;
                if (o->type == OP_DOOR) { if (o->axis == 1) { float c = (mn.z + mx.z) / 2; mn.z = c - 0.06f; mx.z = c + 0.06f; } else { float c = (mn.x + mx.x) / 2; mn.x = c - 0.06f; mx.x = c + 0.06f; } }
                o->bsid = addSolidF(mn, mx, o->f); o->mn = mn; o->mx = mx;
                play(sPlace, 0.9f); say("Barricaded."); skGain(SK_CARP, 2);
            } else say("Not enough wood planks.");
        } else if (myWall >= 0) {
            Build *b = &builds[myWall];
            if (b->hp < b->maxhp) { if (invTake(IT_PLANKS, 1)) { b->hp = b->maxhp; play(sPlace, 0.7f); say("Repaired."); skGain(SK_CARP, 1); } else say("Repair needs a wood plank."); }
            else if (invTake(IT_SCRAP, 1)) { b->maxhp += 6; b->hp = b->maxhp; play(sClang, 0.7f); say("Reinforced with scrap metal."); skGain(SK_CARP, 2); }
        }
    }
    // ---- quickbar, tools
    for (int q = 0; q < 6; q++) if (actP(A_Q1 + q)) quickUse(q);
    if (actP(A_FLASH) && pl.batt > 0) { pl.flash = !pl.flash; play(sClick, 0.5f); }
    if (actP(A_HOLSTER)) holster();
    if (actP(A_SLEEP)) {
        bool threat = false; for (int i = 0; i < nCr; i++) if (crs[i].on && crs[i].state == CS_CHASE && Vector3Distance(crs[i].pos, pl.pos) < 25) threat = true;
        bool inBed = bed >= 0 || facingBuild(eye, V3(0, -1, 0), 2.0f, BK_BED) >= 0;
        if (threat) say("Something is hunting you. You can't sleep now.");
        else if (!inBed && !(pl.roof && pl.nearFire)) say("Sleep needs a bed, or a roof and a fire.");
        else { pl.sleeping = true; say("You close your eyes. [Z] to wake."); play(sSigh, 0.55f); }
    }
    if (actP(A_BUILD)) { pl.build = !pl.build; if (pl.build) { say(TextFormat("Build mode: %s place, wheel turns, %s next piece, %s exit", actKey(A_ATTACK), actKey(A_RELOAD), actKey(A_BUILD))); bkRot = 0; showHint(11); } }
    if (pl.build) {   // wheel turns the piece (15 degree steps, hold Shift for 5), reload picks the next piece
        float w = MWheel(); float step = IsKeyDown(KEY_LEFT_SHIFT) ? 5.0f : 15.0f;
        if (w > 0 || padP(GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) bkRot = fmodf(bkRot + step, 360); if (w < 0 || padP(GAMEPAD_BUTTON_LEFT_FACE_LEFT)) bkRot = fmodf(bkRot - step + 360, 360);
        if (actP(A_RELOAD)) pl.buildKind = (pl.buildKind + 1) % BK_N; if (padP(GAMEPAD_BUTTON_LEFT_FACE_DOWN)) pl.buildKind = (pl.buildKind + BK_N - 1) % BK_N; }
    // ---- attack / build
    pl.swingCd -= dt; pl.shootCd -= dt; pl.kick = fmaxf(0, pl.kick - dt * 6); pl.muzzle -= dt;
    if (pl.swingT > 0) {
        pl.swingT -= dt;
        if (!pl.hitDone && pl.swingT < pl.swingDur * 0.6f) { pl.hitDone = true; meleeHit(eye, aim); }
    }
    if (pl.reloadT > 0) {
        pl.reloadT -= dt;
        if (pl.reloadT <= 0) {
            if (pl.weapon == 1) { int need = 6 - pl.mag; pl.mag += invTake(IT_AMMO, need); }
            else if (pl.weapon >= 2) { int *m = gunMag(); const ItemDef *g = &idef[pl.hands.t]; *m += invTake(g->ammo, g->cap - *m); } }
    }
    {   // guard: hold the right mouse button with a melee weapon or fists
        bool want = !pl.invOpen && !pl.build && pl.weapon == 0 && !isConsumable(pl.hands.t) && pl.climbT <= 0 && (actD(A_GUARD) || forceGuard) && pl.swingT <= 0 && pl.stam > 2;
        if (want) { if (!pl.blocking) pl.blockT = 0; pl.blocking = true; pl.blockT += dt; } else pl.blocking = false;
    }
    if (actP(A_INSPECT) && !animBusy() && pl.swingT <= 0 && !pl.build && !isConsumable(pl.hands.t)) animPlay(ANIM_INSPECT);
    if (pl.build) { if (actP(A_ATTACK)) placeBuild(); }
    else if (isConsumable(pl.hands.t)) { if (actP(A_ATTACK)) useHeld(); }
    else if (actD(A_ATTACK) && !pl.blocking && !(animBusy() && animId != ANIM_EQUIP)) {
        if (pl.hands.t >= 0 && idef[pl.hands.t].cat == CAT_THROW) { if (actP(A_ATTACK) && pl.swingCd <= 0) throwHeld(eye, aim); }
        else if (pl.hands.t == IT_UMBRELLA && pl.umbOpen) { if (actP(A_ATTACK)) { pl.umbOpen = false; play(sUmb, 0.6f); } }   // fold it first
        else if (pl.weapon == 0 && pl.swingCd <= 0 && pl.stam > 6) {
            const ItemDef *w = weaponDef();
            pl.swingDur = pl.swingT = pl.swingCd = meleeCd(); pl.hitDone = false; pl.stam -= w->stam * (1 - 0.07f * skLvl(SK_FIT)) * encStam();
            if (audioOk) { SetSoundPitch(sSwing, 1.25f - clampf(w->weight / 5.0f, 0, 0.6f)); SetSoundVolume(sSwing, 0.6f * gs.sfx); PlaySound(sSwing); } netSwingN++;
        } else if (pl.weapon == 1 && pl.shootCd <= 0 && pl.reloadT <= 0 && actP(A_ATTACK)) {
            if (pl.mag > 0) {
                pl.mag--; revShotT = 0; netShotN++; pl.shootCd = 0.45f; pl.kick = 1; pl.muzzle = 0.07f; pl.tpitch += 0.05f; pl.shake = 0.2f; pl.punch = 0.5f;
                play(sShot, 1.0f); shoot(eye, aim);
            } else { play(sClick, 0.7f); revShotT = 0; pl.shootCd = 0.3f; }
        } else if (pl.weapon >= 2 && pl.shootCd <= 0 && pl.reloadT <= 0 && actP(A_ATTACK)) {
            int *m = gunMag(); const ItemDef *g = &idef[pl.hands.t];
            if (*m > 0 || cht.on[CH_AMMO]) {
                if (!cht.on[CH_AMMO]) (*m)--; revShotT = 0; netShotN++; pl.shootCd = g->cd; pl.muzzle = 0.07f;
                bool sg = pl.weapon == 3; pl.kick = sg ? 1.6f : 0.7f; pl.tpitch += sg ? 0.11f : 0.03f; pl.shake = sg ? 0.45f : 0.12f; pl.punch = sg ? 1.0f : 0.35f;
                play(sg ? sShotgun : sPistol, 1.0f); shootGun(pl.hands.t, eye, aim); weaponWear(0.3f);
            } else { play(sClick, 0.7f); revShotT = 0; pl.shootCd = 0.3f; }
        }
    }
    if ((actP(A_RELOAD) || (gs.autoReload && pl.mag == 0 && pl.shootCd <= 0 && !animBusy())) && !pl.build && pl.weapon == 1 && pl.mag < 6 && invHas(IT_AMMO) && pl.reloadT <= 0) { pl.reloadT = REV_RELOAD; animPlay(ANIM_RELOAD); }
    if (pl.weapon >= 2 && !pl.build && pl.reloadT <= 0) { int *m = gunMag(); const ItemDef *g = &idef[pl.hands.t];
        if ((actP(A_RELOAD) || (gs.autoReload && *m == 0 && pl.shootCd <= 0 && !animBusy())) && *m < g->cap && invHas(g->ammo)) { pl.reloadT = gunReloadT(); animPlay(ANIM_RELOAD); } }
    goto survival;
survival_ui:
    pl.swingT = fmaxf(0, pl.swingT - dt);
survival:
    // ---- survival: blood, wounds, hunger, thirst (harder than before)
    { float sk = pl.sick > 0 ? 1.8f : 1.0f; pl.food -= dt * 100.0f / 480.0f * sk * trMul(3); pl.water -= dt * 100.0f / 360.0f * sk; }
    pl.food = fmaxf(pl.food, 0); pl.water = fmaxf(pl.water, 0);
    for (int q = 0; q < 6; q++) if (pl.quick[q] >= 0 && invCount(pl.quick[q]) == 0 && pl.back.t != pl.quick[q]) pl.quick[q] = -1;   // used up: clear the slot
    if (pl.wounds > 0) {
        pl.hp -= pl.wounds * 0.55f * dt * trMul(5); pl.regenT = 0;
        pl.bleedAcc += dt * pl.wounds; if (pl.bleedAcc > 1.4f) { pl.bleedAcc = 0; addSplat(V3(pl.pos.x + fr(-0.3f, 0.3f), pl.pos.y + 0.022f, pl.pos.z + fr(-0.3f, 0.3f)), fr(0.08f, 0.16f)); }
    } else {
        pl.regenT += dt;
        if (pl.food > 40 && pl.water > 40 && pl.regenT > 20 && pl.hp < 100 && pl.infect < 40 && pl.sick < 50) pl.hp = fminf(100, pl.hp + (pl.painT > 0 ? 0.18f : 0.09f) * dt * (pl.sleeping ? 3 : 1));
    }
    if (pl.food <= 0 || pl.water <= 0) pl.hp -= 0.6f * dt;
    pl.painT -= dt;
    if (pl.flash) { pl.batt -= dt * 100.0f / 420.0f * trMul(4); if (pl.batt <= 0) { pl.batt = 0; pl.flash = false; say("Flashlight battery dead."); } }
    pl.hurtT -= dt; pl.shake = fmaxf(0, pl.shake - dt * 1.6f); pl.punch = fmaxf(0, pl.punch - dt * 3.0f);
    pl.roll *= expf(-dt * 7); pl.hitFlash -= GetFrameTime();
}
