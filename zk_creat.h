// zk_creat.h — creature perception (line of sight), behaviour per type, structure attacks, population
// types: 0 bound, 1 long-neck, 2 crawler, 3 dog, 4 spirit, 5 stalker, 6 screamer, 7 mannequin, 8 hanger, 9 station attendant
static bool solidAt(Vector3 p) {
    query(p.x - 0.01f, p.z - 0.01f, p.x + 0.01f, p.z + 0.01f);
    for (int k = 0; k < qn; k++) { Solid *s = &sol[qb[k]]; Vector3 L = solL(s, p); if (L.x > s->mn.x && L.x < s->mx.x && p.y > s->mn.y && p.y < s->mx.y && L.z > s->mn.z && L.z < s->mx.z) return true; }
    return false;
}
// exact segment test against every solid along the way (sampling could step straight over a 12 cm wall)
static bool segHitsBox(Vector3 a, Vector3 d, Vector3 mn, Vector3 mx) {
    float t0 = 0, t1 = 1;
    float o[3] = {a.x, a.y, a.z}, v[3] = {d.x, d.y, d.z}, lo[3] = {mn.x, mn.y, mn.z}, hi[3] = {mx.x, mx.y, mx.z};
    for (int i = 0; i < 3; i++) {
        if (fabsf(v[i]) < 1e-6f) { if (o[i] <= lo[i] || o[i] >= hi[i]) return false; continue; }
        float ta = (lo[i] - o[i]) / v[i], tb = (hi[i] - o[i]) / v[i];
        if (ta > tb) { float tt = ta; ta = tb; tb = tt; }
        if (ta > t0) t0 = ta; if (tb < t1) t1 = tb;
        if (t0 >= t1) return false;
    }
    return true;
}
static bool lineOfSight(Vector3 a, Vector3 b) {
    Vector3 d = Vector3Subtract(b, a); float L = Vector3Length(d);
    if (L < 0.05f) return true;
    int chunks = (int)(L / 4.0f) + 1;
    for (int ch = 0; ch < chunks; ch++) {
        Vector3 p0 = Vector3Add(a, Vector3Scale(d, (float)ch / chunks)), p1 = Vector3Add(a, Vector3Scale(d, (float)(ch + 1) / chunks)), sd = Vector3Subtract(p1, p0);
        query(fminf(p0.x, p1.x) - 0.05f, fminf(p0.z, p1.z) - 0.05f, fmaxf(p0.x, p1.x) + 0.05f, fmaxf(p0.z, p1.z) + 0.05f);
        for (int k = 0; k < qn; k++) { Solid *s = &sol[qb[k]]; if (s->off) continue; if (segHitsBox(solL(s, p0), s->f ? xfDI(s->f, sd) : sd, s->mn, s->mx)) return false; }
    }
    return true;
}
static float stingCd = 0;
static void sting(Vector3 p) { if (stingCd <= 0 && audioOk) { SetSoundVolume(sStinger, 0.75f * gs.sfx); SetSoundPitch(sStinger, fr(0.9f, 1.1f)); PlaySound(sStinger); stingCd = 9; } (void)p; }
static int crDamageOf(int type) { switch (type) { case 1: return 18; case 2: return 7; case 3: return 9; case 5: return 22; case 6: return 14; case 7: return 15; case 8: return 12; case 9: return 26; case 10: return 20; case 11: return 9; case 12: return 10; case 13: return 24; case 14: return 13; case 15: return 19; case 16: return 6; case 17: return 14; default: return 11; } }
static float crBleedOf(int type) { switch (type) { case 2: return 0.6f; case 3: return 0.7f; case 5: return 0.6f; case 8: return 0.7f; case 6: return 0.3f; case 10: return 0.85f; case 11: return 0.6f; case 13: return 0.75f; case 14: return 0.9f; case 15: return 0.15f; case 16: return 0.5f; case 17: return 0.35f; default: return 0.45f; } }
static float crSightOf(int type) { switch (type) { case 3: return 22; case 5: return 34; case 6: return 20; case 9: return 18; case 7: return 14; case 10: return 26; case 13: return 24; case 15: return 22; case 11: return 12; default: return 16; } }
// nearest stair waypoint that moves a creature toward the player's floor
static Vector3 navGoal(Cr *c, Vector3 target) {
    float dy = target.y - c->pos.y;
    if (fabsf(dy) < 1.6f) return target;
    int best = -1; float bd = 1e9f;
    for (int i = 0; i < nStair; i++) {
        Stair *s = &stairs[i];
        Vector3 from = dy > 0 ? s->bot : s->top;
        if (fabsf(from.y - c->pos.y) > 1.2f) continue;
        float d = Vector3Distance(V3(from.x, 0, from.z), V3(c->pos.x, 0, c->pos.z)) + Vector3Distance(V3(from.x, 0, from.z), V3(target.x, 0, target.z)) * 0.5f;
        if (d < bd) { bd = d; best = i; }
    }
    if (best < 0) return target;
    Stair *s = &stairs[best]; Vector3 from = dy > 0 ? s->bot : s->top, to = dy > 0 ? s->top : s->bot;
    if (Vector3Distance(V3(from.x, 0, from.z), V3(c->pos.x, 0, c->pos.z)) < 0.9f || (c->pos.y > fminf(from.y, to.y) + 0.3f && c->pos.y < fmaxf(from.y, to.y) - 0.3f)) return to;
    return from;
}
static void attackStructureNear(Cr *c) {
    float r = 1.9f; float dmg = c->type == 1 || c->type == 9 || c->type == 13 ? 2.5f : c->type == 15 ? 2.2f : c->type == 6 || c->type == 10 ? 2 : c->type == 16 ? 0.7f : 1;
    for (int i = 0; i < nBuild; i++) {
        Build *b = &builds[i]; if (!b->alive || b->kind == BK_SHRINE || b->kind == BK_WARD || b->sid < 0 || (b->kind == BK_DOOR && b->open)) continue;
        Vector3 cc = Vector3Scale(Vector3Add(b->mn, b->mx), 0.5f);
        if (Vector3Distance(V3(cc.x, 0, cc.z), V3(c->pos.x, 0, c->pos.z)) > r + 0.8f || fabsf(cc.y - c->pos.y) > 2.5f) continue;
        b->hp -= dmg; playAt(sWood, cc, 30, 0.8f); spawnParts(cc, 4, T_WOOD, WHITE, 1.4f, 0.07f);
        if (b->hp <= 0) { b->alive = false; if (b->sid >= 0) sol[b->sid].off = true; if (b->kind == BK_CHEST && b->cont >= 0) conts[b->cont].kind = CK_PILE; }
        return;
    }
    for (int i = 0; i < nDeb; i++) {
        Debris *d = &debs[i]; if (!d->alive) continue;
        Vector3 cc = xfP(d->f, Vector3Scale(Vector3Add(d->mn, d->mx), 0.5f));
        if (Vector3Distance(V3(cc.x, 0, cc.z), V3(c->pos.x, 0, c->pos.z)) > r + 0.5f || fabsf(cc.y - c->pos.y) > 2.0f) continue;
        d->hp -= dmg * 0.5f; playAt(sWood, cc, 25, 0.6f);
        if (d->hp <= 0) { d->alive = false; sol[d->sid].off = true; }
        return;
    }
    for (int i = 0; i < nOps; i++) {
        Open *o = &ops[i];
        if (!(o->state == 2 || (o->type == OP_DOOR && o->state == 1))) continue;
        Vector3 cc = opCenter(o);
        if (Vector3Distance(V3(cc.x, 0, cc.z), V3(c->pos.x, 0, c->pos.z)) > r || fabsf(o->fy - c->pos.y) > 1.5f) continue;
        o->hp -= dmg; playAt(sWood, cc, 30, 0.8f); spawnParts(cc, 4, T_WOOD, WHITE, 1.4f, 0.07f);
        if (o->hp <= 0) {
            if (o->state == 2) { sol[o->bsid].off = true; o->bsid = -1; o->state = o->type == OP_WINDOW ? 1 : 0; }
            else { sol[o->sid].off = true; o->state = 0; }
        }
        return;
    }
    for (int i = 0; i < nOps; i++) {
        Open *o = &ops[i]; if (o->type != OP_WINDOW || o->state != 0) continue;
        Vector3 cc = opCenter(o);
        if (Vector3Distance(V3(cc.x, 0, cc.z), V3(c->pos.x, 0, c->pos.z)) < 1.3f && fabsf(o->fy - c->pos.y) < 1.5f) { breakWindow(o); return; }
    }
}
static bool inWard(Vector3 p, float extra) {
    for (int w = 0; w < nWard; w++) if (wards[w].on && Vector3Distance(V3(p.x, 0, p.z), V3(wards[w].pos.x, 0, wards[w].pos.z)) < wards[w].r + extra && fabsf(p.y - wards[w].pos.y) < 12) return true;
    return false;
}
static float wildK(Vector3 p);
static void updateCreatures(float dt, Vector3 aim) {
    float nearest = 1e9f;
    int aliveTop = 0, spirits = 0, stalkers = 0, chasing = 0;
    stingCd -= dt;
    Vector3 eye = Vector3Add(pl.pos, V3(0, 1.6f, 0));
    float stealth = (1 - 0.08f * skLvl(SK_STEALTH)) * (actHeld(A_CROUCH) ? 0.55f : 1.0f);
    for (int w = 0; w < nWard; w++) if (wards[w].on && wards[w].kind == 0) { wards[w].life -= dt; if (wards[w].life <= 0) wards[w].on = false; }
    for (int i = 0; i < nCr; i++) {
        Cr *c = &crs[i]; if (!c->on) continue;
        c->t += dt; c->hurt -= dt; c->flinch = fmaxf(0, c->flinch - dt * 3.5f);
        if (c->state == CS_DEAD) {
            if (c->type == 12 && c->deadT == 0) {   // the bloated one bursts: a spray of filth that sickens and hurts anyone close
                Vector3 cc = Vector3Add(c->pos, V3(0, 1.0f, 0)); spawnParts(cc, 30, T_GORE, (Color){150, 170, 120, 255}, 4.0f, 0.14f); playAt(sSplat, cc, 40, 1.0f);
                if (Vector3Distance(cc, pl.pos) < 3.6f && !cht.on[CH_GOD]) { hurtPlayer(12, 0.2f); pl.sick = fminf(100, pl.sick + 30); pl.shake = fmaxf(pl.shake, 0.6f); say("It bursts over you. The stink is unbearable."); }
                for (int j = 0; j < nCr; j++) if (j != i && crs[j].on && crs[j].state != CS_DEAD && Vector3Distance(crs[j].pos, c->pos) < 3.6f) { crs[j].hp -= 3; crs[j].hurt = 0.2f; if (crs[j].hp <= 0) { crs[j].state = CS_DEAD; crs[j].deadT = 0; } } }
            c->deadT += dt; if (c->deadT > (c->type == 4 ? 1.2f : 30)) c->on = false; continue; }
        c->revealT -= dt;
        if (c->phantom) {   // hallucination: drifts toward you, vanishes up close
            Vector3 to = V3(pl.pos.x - c->pos.x, 0, pl.pos.z - c->pos.z); float dist = Vector3Length(to);
            if (dist < 3.0f || dist > 60 || (pl.stage < 3 && wildK(pl.pos) < 0.3f)) { c->on = false; continue; }
            c->yaw = atan2f(to.x, to.z);
            float sp = c->spd * 0.35f * (0.55f + 0.45f * fabsf(sinf(c->t * 4.5f + i)));
            c->pos.x += to.x / dist * sp * dt; c->pos.z += to.z / dist * sp * dt;
            for (int k = 0; k < 6; k++) if (fr(0, 1) < dt * 6) c->tw[k] = fr(-0.6f, 0.6f);
            continue;
        }
        // ---- the target: the nearest living player (the local one, or a remote one in multiplayer)
        int tgt = -1; Vector3 tp = pl.pos; bool tfl = pl.flash; Vector3 teye = eye, taim = aim; float tstealth = stealth, tclimb = pl.climbT;
        {   float bd = pl.hp > 0 ? Vector3DistanceSqr(c->pos, pl.pos) : 1e12f;
            if (netMode == NM_HOST) for (int k = 0; k < MP_MAX; k++) { RPlayer *r = &rps[k]; if (k == mySlot || !r->on || !r->alive) continue; float d2 = Vector3DistanceSqr(c->pos, r->pos);
                if (d2 < bd) { bd = d2; tgt = k; tp = r->pos; tfl = r->flash; teye = Vector3Add(r->pos, V3(0, 1.6f, 0)); taim = V3(sinf(r->yaw) * cosf(r->pitch), sinf(r->pitch), cosf(r->yaw) * cosf(r->pitch)); tstealth = 1; tclimb = 0; } } }
        Vector3 to = V3(tp.x - c->pos.x, 0, tp.z - c->pos.z);
        float dist = Vector3Length(to), dist3 = Vector3Distance(c->pos, tp);
        bool lookedAt = Vector3DotProduct(Vector3Normalize(Vector3Subtract(crCenter(c), teye)), taim) > 0.8f && (tfl || dist < 12);
        // ---- line of sight, checked a few times a second
        c->losCheck -= dt;
        if (c->losCheck <= 0) { c->losCheck = fr(0.18f, 0.3f); c->seen = dist3 < 45 && lineOfSight(Vector3Add(crCenter(c), V3(0, 0.3f, 0)), teye); }
        if (c->type == 4) {   // ---- spirit: drifts through walls, hunts only what it can see, forgets when sight is lost
            spirits++;
            if (dist < nearest) nearest = dist;
            Vector3 fwdS = V3(sinf(c->yaw), 0, cosf(c->yaw));
            bool sees = c->seen && dist < 24 * tstealth && (Vector3DotProduct(fwdS, Vector3Scale(to, 1 / fmaxf(dist, 0.01f))) > -0.2f || dist < 5);
            // after giving up it leaves: heads away from you, ignores you for a while, and fades out once far enough
            c->calm -= dt;
            if (c->calm > 0 && dist > 2.5f) sees = false;
            if (c->state == CS_CHASE) c->chaseT += dt;
            bool giveUp = false;
            if (sees) { c->losT = 0; c->lastKnown = tp; if (c->state != CS_CHASE) { c->state = CS_CHASE; c->chaseT = 0; sting(c->pos); } }
            else if (c->state == CS_CHASE) { c->losT += dt; if (c->losT > 3.0f) giveUp = true; }
            if (c->state == CS_CHASE && c->chaseT > 40) giveUp = true;   // no spirit hunts forever
            if (inWard(tp, 0) && c->state == CS_CHASE) giveUp = true;
            if (giveUp) { Vector3 away = dist > 0.1f ? Vector3Scale(to, -1 / dist) : V3(1, 0, 0);
                c->state = CS_WANDER; c->calm = 30; c->chaseT = 0; c->t = 0;
                c->tgt = Vector3Add(c->pos, Vector3Add(Vector3Scale(away, 45), V3(fr(-10, 10), 0, fr(-10, 10)))); }
            if (c->calm > 0 && c->state == CS_WANDER && dist > 30) { c->on = false; continue; }   // gone
            Vector3 goal = c->state == CS_CHASE ? (sees ? tp : c->lastKnown) : c->tgt;
            if (c->state == CS_WANDER && c->calm <= 0 && (Vector3Distance(V3(goal.x, 0, goal.z), V3(c->pos.x, 0, c->pos.z)) < 1.5f || c->t > 14)) { c->tgt = Vector3Add(c->pos, V3(fr(-18, 18), 0, fr(-18, 18))); c->t = 0; }
            Vector3 dv = V3(goal.x - c->pos.x, 0, goal.z - c->pos.z); float dl = Vector3Length(dv);
            float spd = c->state == CS_CHASE ? (lookedAt && tfl ? 1.2f : 3.6f) : c->calm > 0 ? 2.2f : 0.9f;
            if (dl > 0.3f) { c->pos.x += dv.x / dl * spd * dt; c->pos.z += dv.z / dl * spd * dt; c->yaw += wrapA(atan2f(dv.x, dv.z) - c->yaw) * fminf(1, dt * 3); }
            for (int w = 0; w < nWard; w++) if (wards[w].on && fabsf(c->pos.y - wards[w].pos.y) < 12) {
                Vector3 d = V3(c->pos.x - wards[w].pos.x, 0, c->pos.z - wards[w].pos.z); float L = Vector3Length(d);
                float rr = wards[w].r + 0.4f;
                if (L < rr) { if (L < 0.01f) { d = V3(1, 0, 0); L = 1; } c->pos.x = wards[w].pos.x + d.x / L * rr; c->pos.z = wards[w].pos.z + d.z / L * rr; if (c->state == CS_CHASE && inWard(tp, 0)) { c->state = CS_WANDER; c->tgt = Vector3Add(c->pos, Vector3Scale(V3(d.x / L, 0, d.z / L), 25)); } }
            }
            float gy = c->state == CS_CHASE ? tp.y : groundAt(V3(c->pos.x, c->pos.y + 1, c->pos.z), 0.3f, 1.2f);
            c->pos.y = Lerp(c->pos.y, gy + 0.25f + sinf(c->t * 1.3f) * 0.12f, fminf(1, dt * 2));
            if (dist < 1.1f && fabsf(tp.y - c->pos.y) < 2.0f && c->state == CS_CHASE && c->seen) {
                if (tgt < 0) { pl.hp -= 9 * dt; pl.sanity -= 4 * dt; pl.hurtT = fmaxf(pl.hurtT, 0.25f); pl.shake = fmaxf(pl.shake, 0.12f);
                    if (fr(0, 1) < dt * 0.6f) addWound(1); }
                else netPlayerDamage(tgt, 9 * dt, 0);
            }
            for (int k = 0; k < 6; k++) if (fr(0, 1) < dt * 3) c->tw[k] = fr(-0.4f, 0.4f);
            if (dist > 140) c->on = false;
            continue;
        }
        if (c->pos.y > -40) aliveTop++;
        if (c->type == 5) stalkers++;
        if (dist < nearest) nearest = dist;
        // twitch
        c->twT -= dt;
        if (c->twT <= 0) {
            bool violent = fr(0, 1) < (c->state == CS_CHASE ? 0.35f : 0.12f);
            float a = violent ? 0.9f : 0.15f;
            for (int k = 0; k < 6; k++) c->tw[k] = fr(-a, a);
            c->twT = violent ? fr(0.03f, 0.08f) : fr(0.12f, 0.4f);
        }
        // ---- knockback + stagger
        if (Vector3Length(c->kb) > 0.05f) {
            c->pos.x += c->kb.x * dt; c->pos.z += c->kb.z * dt; c->kb = Vector3Scale(c->kb, expf(-dt * 9));
            resolve(&c->pos, 0.35f, 1.6f, 0.42f);
        }
        if (c->stun > 0) { c->stun -= dt; c->wind = 0; continue; }
        // ---- hanging: waits on the ceiling until you pass beneath
        if (c->hanging) {
            if (dist < 2.2f && tp.y < c->pos.y && c->pos.y - tp.y < 4.5f) {
                c->hanging = false; c->state = CS_CHASE; c->lastKnown = tp; sting(c->pos); playAt(sScreech, c->pos, 30, 0.9f);
                c->pos.y = groundAt(V3(c->pos.x, c->pos.y - 0.5f, c->pos.z), 0.3f, 0.6f);
            }
            continue;
        }
        // ---- perception
        float sight = crSightOf(c->type) * tstealth * (tfl ? 1.25f : 1.0f);
        bool sees = c->seen && (dist3 < sight || (tfl && dist3 < sight * 1.8f && lookedAt)) && fabsf(tp.y - c->pos.y) < 6;
        if (c->type == 7 && dist3 > 13) sees = false;
        if (sees) { c->losT = 0; c->lastKnown = tp; }
        if (c->state == CS_WANDER) {
            if (sees) {
                c->state = CS_CHASE; c->lastKnown = tp;
                if (c->type != 5 && c->type != 7) playAt(sScreech, c->pos, 35, 0.5f);
                if (dist < 18) sting(c->pos);
                if (c->type == 6) c->scream = 0;
                c->chaseT = 0;   // (the kuchisake stares first)
            }
        } else {
            if (!sees) { c->losT += dt; if (c->losT > (c->type == 9 ? 14 : 8) && Vector3Distance(c->pos, c->lastKnown) < 2.5f) { c->state = CS_WANDER; c->tgt = c->pos; } }
            if (dist > 90 && !evacOn) c->state = CS_WANDER;
        }
        if (c->state == CS_CHASE && dist < 20) chasing++;
        // ---- screamer: calls the street
        if (c->type == 6 && c->state == CS_CHASE && sees) {
            c->scream -= dt;
            if (c->scream <= 0) { c->scream = 12; playAt(sScream, c->pos, 80, 1.0f); c->wind = 0; c->stun = 1.4f;
                for (int j = 0; j < nCr; j++) if (crs[j].on && crs[j].state == CS_WANDER && crs[j].type != 4 && Vector3Distance(crs[j].pos, c->pos) < 55) { crs[j].state = CS_CHASE; crs[j].lastKnown = tp; } }
        }
        // ---- movement
        Vector3 goal; float spd;
        bool frozen = false;
        if (c->state == CS_CHASE) {
            goal = navGoal(c, sees ? tp : c->lastKnown); spd = c->spd;
            // stutter-dash: they stop dead for a heartbeat, then come at you twice as fast
            c->dashT -= dt;
            if ((c->type == 0 || c->type == 6 || c->type == 2) && dist < 22) {
                if (c->dashT < -1.6f && fr(0, 1) < dt * 1.2f) c->dashT = 0.9f;
                if (c->dashT > 0.55f) { spd = 0; for (int k = 0; k < 6; k++) c->tw[k] *= 0.3f; }
                else if (c->dashT > 0) spd *= 1.9f;
            }
            if (c->sideT > 0) { c->sideT -= dt; goal = c->tgt; }
            if (c->type == 1 && dist < 4.5f && dist > 1.6f) spd = 6.2f;                 // long-neck lunge
            if (c->type == 10) { c->chaseT += dt; if (c->chaseT < 1.1f) spd = 0; else spd = 7.2f; }   // kuchisake: a long stare, then she runs
            if (c->type == 11 && dist < 7) spd = 6.0f;                                   // crawler: scuttles in close
            if (c->type == 13 && dist < 9 && dist > 2.0f) spd = 5.8f;                    // tsuchigumo: closes fast
            if (c->type == 14 && fr(0, 1) < dt * 2.5f) c->sideT = 0.25f;                  // nurse: jerks aside
            if (c->type == 5) {                                                          // stalker: only moves when unwatched
                if (lookedAt && c->seen) { frozen = true; }
                else if (dist < 6) spd = 8.5f; else if (dist > 18) spd = 4.5f; else spd = 2.4f;
            }
            if (c->type == 7) { if (lookedAt && c->seen) frozen = true; else spd = 6.5f; }  // mannequin: moves in the dark of your back
        } else {
            if (c->type == 7 || c->type == 8) { goal = c->pos; spd = 0; }
            else {
                if (Vector3Distance(V3(c->tgt.x, 0, c->tgt.z), V3(c->pos.x, 0, c->pos.z)) < 1.0f || c->t > 9) {
                    float r2 = c->pos.y < -40 ? 18 : 14;
                    c->tgt = V3(c->pos.x + fr(-r2, r2), c->pos.y, c->pos.z + fr(-r2, r2)); c->t = 0;
                }
                goal = c->tgt; spd = c->spd * 0.32f;
            }
        }
        if (frozen) { c->wind = 0; for (int k = 0; k < 6; k++) c->tw[k] *= 0.5f; if (fr(0, 1) < dt * 2) c->tw[ri(6)] = fr(-1.2f, 1.2f); }
        float pulse = c->type == 9 ? 1.0f : 0.55f + 0.45f * fabsf(sinf(c->t * (c->type == 2 ? 9 : 4.5f) + i));
        Vector3 dir = V3(goal.x - c->pos.x, 0, goal.z - c->pos.z);
        float dl = Vector3Length(dir);
        if (c->wind > 0) {   // attack wind-up
            c->wind -= dt;
            if (c->wind <= 0) {
                if (dist < 2.1f && fabsf(tp.y - c->pos.y) < 1.6f && tclimb <= 0 && lineOfSight(Vector3Add(crCenter(c), V3(0, 0.2f, 0)), teye)) { if (tgt < 0) hurtPlayer((float)crDamageOf(c->type), crBleedOf(c->type)); else netPlayerDamage(tgt, (float)crDamageOf(c->type), crBleedOf(c->type)); c->revealT = 3.0f; }
                c->atkCd = c->type == 9 ? 1.8f : 1.2f;
            }
        } else if (!frozen && dl > 0.05f && spd > 0) {
            dir = Vector3Scale(dir, 1 / dl);
            float ty = atan2f(dir.x, dir.z);
            c->yaw += wrapA(ty - c->yaw) * fminf(1, dt * 6);
            Vector3 before = c->pos;
            c->pos.x += dir.x * spd * pulse * dt; c->pos.z += dir.z * spd * pulse * dt;
            float h = c->type == 1 || c->type == 5 ? 2.4f : c->type == 2 || c->type == 8 || c->type == 11 ? 0.8f : c->type == 16 ? 1.3f : c->type == 13 ? 1.6f : 1.8f;
            resolve(&c->pos, 0.35f, h, 0.45f);
            float gg = groundAt(c->pos, 0.35f, 0.45f);
            c->pos.y = c->pos.y > gg + 0.5f ? c->pos.y - dt * 9 : Lerp(c->pos.y, gg, fminf(1, dt * 12));
            if (c->pos.y < gg) c->pos.y = gg;
            float moved = Vector3Distance(V3(before.x, 0, before.z), V3(c->pos.x, 0, c->pos.z));
            if (c->state == CS_CHASE && dist > 1.6f && moved < spd * pulse * dt * 0.3f) c->stuckT += dt; else c->stuckT = fmaxf(0, c->stuckT - dt);
            if (c->stuckT > 0.7f) {
                c->stuckT = 0;
                attackStructureNear(c);
                if (fr(0, 1) < 0.5f) { c->sideT = 1.2f; c->tgt = Vector3Add(c->pos, V3(fr(-5, 5), 0, fr(-5, 5))); }
            }
        }
        c->atkCd -= dt;
        if (c->state == CS_CHASE && !frozen && c->seen && dist < (c->type == 1 || c->type == 5 ? 1.9f : 1.5f) && c->atkCd <= 0 && c->wind <= 0 && fabsf(tp.y - c->pos.y) < 1.6f) {
            c->wind = c->type == 2 || c->type == 3 || c->type == 11 || c->type == 16 ? 0.22f : c->type == 9 || c->type == 12 || c->type == 15 ? 0.6f : 0.38f; c->atkCd = 1.2f;
        }
        // ---- sound
        c->groanT -= dt;
        if (c->groanT <= 0) {
            c->groanT = fr(5, 13);
            if (c->type == 5 || c->type == 7) { if (dist < 12 && audioOk && !IsSoundPlaying(sBreath)) { SetSoundVolume(sBreath, 0.6f * (1 - dist / 12) * gs.sfx); PlaySound(sBreath); } }
            else if (dist < 40 && audioOk) for (int k = 0; k < 8; k++) if (!IsSoundPlaying(groanPool[k])) {
                SetSoundVolume(groanPool[k], 0.8f * powf(1 - dist / 40, 1.5f) * gs.sfx); SetSoundPitch(groanPool[k], c->type == 2 ? 1.5f : c->type == 1 || c->type == 9 ? 0.7f : c->type == 6 ? 0.85f : fr(0.9f, 1.1f));
                PlaySound(groanPool[k]); break;
            }
        }
    }
    // heartbeat while hunted, radio static near creatures
    static float heartT = 0; heartT -= dt;
    if (chasing > 0 && heartT <= 0) { play(sHeart, 0.55f); heartT = chasing > 2 ? 0.55f : 0.85f; }
    if (audioOk) {
        float v = nearest < 28 ? powf(1 - nearest / 28, 1.3f) * 0.55f : 0;
        if (!IsSoundPlaying(sStatic)) PlaySound(sStatic);
        SetSoundVolume(sStatic, v * gs.amb); SetSoundVolume(sDrone, 0.35f * gs.amb);
        if (!IsSoundPlaying(sDrone)) PlaySound(sDrone);
    }
    // ---- population upkeep
    if (netMode == NM_HOST) { int k = ri(MP_MAX); if (k != mySlot && rps[k].on && rps[k].alive) { static float rsp = 0; rsp -= dt; if (rsp <= 0) { rsp = 3.0f; int n = 0; for (int j = 0; j < nCr; j++) if (crs[j].on && crs[j].state != CS_DEAD && Vector3Distance(crs[j].pos, rps[k].pos) < 50) n++; if (n < 14 && wildK(rps[k].pos) < 0.2f) spawnCreature(rps[k].pos, 40, 0, false); } } }   // remote players get their own share of the town's creatures
    float wk = wildK(pl.pos);
    static float spawnT = 0, spiritT = 40, ugT = 20; spawnT -= dt; spiritT -= dt; ugT -= dt;
    { bool dist = inDistrict(pl.pos.x, pl.pos.z);   // the high-rise district draws more of them, and faster
      if (pl.pos.y > -40 && wk < 0.2f && aliveTop < (dayK() > 0.5f ? 32 : 44) * (dist ? 1.7f : 1.0f) && spawnT <= 0) { if (fr(0, 1) < (dist ? 0.5f : 0.35f)) spawnIndoor(pl.pos, 35); else spawnCreature(pl.pos, 45, 0, false); spawnT = dist ? 1.4f : 3.0f; } }
    if (stalkers < 1 && pl.time > 240 && fr(0, 1) < dt * 0.004f) spawnCreatureT(pl.pos, 40, 60, false, 5);
    int maxSpirits = wk > 0.3f ? 4 + (int)(wk * 4) : dayK() > 0.5f ? 0 : (pl.time > 300 ? 3 : 2);   // the city's spirits walk at night
    if (spiritT <= 0) { if (spirits < maxSpirits) spawnCreatureT(pl.pos, wk > 0.3f ? 14 : 30, wk > 0.3f ? 30 : 45, wk > 0.5f, 4); spiritT = wk > 0.3f ? fr(4, 8) : fr(35, 70); }
    if (pl.pos.y < -40 && ugT <= 0) { int ug = 0; for (int i = 0; i < nCr; i++) if (crs[i].on && crs[i].state != CS_DEAD && crs[i].pos.y < -40) ug++; if (ug < 12) spawnUnderground(); ugT = 45; }
    static float hallT = 5; hallT -= dt;
    if ((pl.stage == 3 || wk > 0.4f) && hallT <= 0) {   // things that aren't there
        int before = nCr; int freeSlot = -1; for (int i = 0; i < nCr; i++) if (!crs[i].on) { freeSlot = i; break; }
        spawnCreatureT(pl.pos, 8, 22, false, ri(2) ? 4 : ri(4));
        int idx = freeSlot >= 0 ? freeSlot : (nCr > before ? nCr - 1 : -1);
        if (idx >= 0 && crs[idx].on) { crs[idx].phantom = true; crs[idx].hp = 999; }
        hallT = fr(4, 14) * (1.3f - fmaxf(pl.corrupt / 100.0f, wk));
    }
}
