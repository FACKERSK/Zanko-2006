// zk_net.h — multiplayer: co-op with an optional PvP switch, one player hosts, up to 8 join by IP (UDP, net.c).
// The host runs the creatures and is the referee; every machine builds the same town from the host's seed.
// World changes (doors, windows, barricades, junk piles, containers, builds, broken solids) are found by comparing
// against a shadow copy a few times a second and sent reliably; players and creatures stream unreliably.
int netOpen(int port); void netClose(void); int netResolve(const char *host, int port, NetAddr *out);
int netSend(const NetAddr *to, const void *buf, int len); int netRecv(void *buf, int max, NetAddr *from); void netAddrStr(const NetAddr *a, char *out, int max);

#define MP_PORT 27015
#define MP_PROTO 3
enum { P_JOIN = 1, P_WELCOME, P_REJECT, P_STATE, P_PLAYERS, P_CRS, P_REL, P_ACK, P_BYE };
enum { R_OP = 1, R_DEB, R_VEND, R_CRATE, R_CONT, R_BUILD, R_SOL, R_HIT, R_PDMG, R_SAY };
static NetAddr hostAddr; static double netJoinT = 0, netSendT = 0, netCrT = 0, netScanT = 0; static bool netJoining = false;
static char netStatus[160] = ""; static unsigned int netSeed = 0;
static TextBox mpIP = {"127.0.0.1", 9, false, false, 63}, mpPort = {"27015", 5, false, false, 5};

// ---- byte writer / reader
typedef struct { unsigned char b[1400]; int n; } WB;
static void wb(WB *w, const void *p, int n) { if (w->n + n <= (int)sizeof w->b) { memcpy(w->b + w->n, p, n); w->n += n; } }
#define WV(w, v) do { __typeof__(v) _t = (v); wb((w), &_t, sizeof _t); } while (0)
typedef struct { const unsigned char *b; int n, i; bool bad; } RB;
static void rbr(RB *r, void *p, int n) { if (r->i + n > r->n) { r->bad = true; memset(p, 0, n); return; } memcpy(p, r->b + r->i, n); r->i += n; }
#define RV(r, T) ({ T _t; rbr((r), &_t, sizeof _t); _t; })

// ---- reliable channel: per peer a queue of messages resent until acknowledged
#define RQN 2048
typedef struct { bool used; unsigned short seq; short len; double sent; int tries; unsigned char d[300]; } RMsg;
static RMsg rq[MP_MAX][RQN]; static unsigned short rseq[MP_MAX]; static unsigned char rseen[MP_MAX][8192];
static unsigned short ackq[MP_MAX][256]; static int nAck[MP_MAX];
static NetAddr *peerAddr(int k) { return netMode == NM_CLIENT ? &hostAddr : &rps[k].addr; }
static void relSend(int peer, const WB *m) {   // queue one inner message (type byte + payload) for a peer
    for (int i = 0; i < RQN; i++) if (!rq[peer][i].used) { RMsg *q = &rq[peer][i]; q->used = true; q->seq = ++rseq[peer]; q->len = (short)m->n; memcpy(q->d, m->b, m->n); q->sent = -1; q->tries = 0; return; }
}
static void relAll(const WB *m, int except) {   // the host to every client but one; a client to the host
    if (netMode == NM_CLIENT) { relSend(0, m); return; }
    for (int k = 0; k < MP_MAX; k++) if (k != mySlot && k != except && rps[k].on) relSend(k, m);
}
static void relPump(double now) {
    for (int k = 0; k < MP_MAX; k++) {
        if (netMode == NM_HOST && (k == mySlot || !rps[k].on)) continue;
        if (netMode == NM_CLIENT && k != 0) continue;
        int sent = 0;
        for (int i = 0; i < RQN && sent < 90; i++) { RMsg *q = &rq[k][i]; if (!q->used || (q->sent >= 0 && now - q->sent < 0.3)) continue;
            if (q->tries > 80) { q->used = false; continue; }
            WB w = {0}; WV(&w, (unsigned char)P_REL); WV(&w, q->seq); wb(&w, q->d, q->len); netSend(peerAddr(k), w.b, w.n); q->sent = now; q->tries++; sent++; }
        if (nAck[k]) { WB w = {0}; WV(&w, (unsigned char)P_ACK); WV(&w, (unsigned short)nAck[k]); wb(&w, ackq[k], nAck[k] * 2); netSend(peerAddr(k), w.b, w.n); nAck[k] = 0; }
    }
}
static bool relSeen(int k, unsigned short seq) {   // true when this sequence number was already handled
    unsigned short old = (unsigned short)(seq + 32768); rseen[k][old >> 3] &= (unsigned char)~(1 << (old & 7));   // forget the far half of the window
    bool was = rseen[k][seq >> 3] & (1 << (seq & 7)); rseen[k][seq >> 3] |= (unsigned char)(1 << (seq & 7)); return was;
}
static void netResetChannels(void) { memset(rq, 0, sizeof rq); memset(rseq, 0, sizeof rseq); memset(rseen, 0, sizeof rseen); memset(nAck, 0, sizeof nAck); }

// ---- shadows of the world state (what every other machine is known to have)
static unsigned char opS[MAX_OPEN][2]; static float opH[MAX_OPEN]; static bool debS[MAX_DEB], vendS[MAX_VEND], crateS[MAX_CRATE];
static unsigned int contS[MAX_CONT]; static Build buildS[MAX_BUILD]; static unsigned char *solS; static int solSN, nContS, nBuildS;
static unsigned char opP[MAX_OPEN], *solP; static unsigned int contP[MAX_CONT]; static int nContP;   // the town as built (what a joining player starts from)
static unsigned int fnv(const void *p, int n) { const unsigned char *b = p; unsigned int h = 2166136261u; for (int i = 0; i < n; i++) { h ^= b[i]; h *= 16777619u; } return h; }
static unsigned int contHash(const Cont *c) { Cont t = *c; t.sid = 0; return fnv(&t, sizeof t); }
static void shadowAll(void) {   // after a world is built or loaded: everything as it is now counts as known
    for (int i = 0; i < nOps; i++) { opS[i][0] = (unsigned char)ops[i].state; opS[i][1] = 1; opH[i] = ops[i].hp; }
    for (int i = 0; i < nDeb; i++) debS[i] = debs[i].alive;
    for (int i = 0; i < nVend; i++) vendS[i] = vends[i].broken;
    for (int i = 0; i < nCrate; i++) crateS[i] = crates[i].alive;
    for (int i = 0; i < nCont; i++) contS[i] = contHash(&conts[i]); nContS = nCont;
    for (int i = 0; i < nBuild; i++) { buildS[i] = builds[i]; buildS[i].sid = 0; } nBuildS = nBuild;
    free(solS); solSN = worldSolids; solS = calloc((size_t)solSN + 1, 1); for (int i = 0; i < solSN; i++) solS[i] = sol[i].off;
}
// ---- messages for each kind of change
static void msgOp(WB *w, int i) { Open *o = &ops[i]; WV(w, (unsigned char)R_OP); WV(w, i); WV(w, (unsigned char)o->state); WV(w, o->hp); WV(w, o->mn); WV(w, o->mx); }
static void msgCont(WB *w, int i) { WV(w, (unsigned char)R_CONT); WV(w, i); WV(w, conts[i]); }
static void msgBuild(WB *w, int i) { WV(w, (unsigned char)R_BUILD); WV(w, i); WV(w, builds[i]); }
static void applyOp(int i, int st, float hp, Vector3 mn, Vector3 mx) {
    if (i < 0 || i >= nOps) return; Open *o = &ops[i];
    o->mn = mn; o->mx = mx; o->hp = hp; o->state = st;
    bool closed = (o->type == OP_WINDOW && st == 0) || (o->type == OP_DOOR && st == 1);
    if (o->sid >= 0) sol[o->sid].off = !closed;
    if (st == 2 && o->bsid < 0) o->bsid = addSolidF(mn, mx, o->f);
    if (st != 2 && o->bsid >= 0) { sol[o->bsid].off = true; o->bsid = -1; }
    opS[i][0] = (unsigned char)st; opH[i] = hp;
}
static void applyBuild(int i, Build b) {
    if (i < 0 || i >= MAX_BUILD) return;
    Build *L = &builds[i]; int sid = (i < nBuild) ? L->sid : -1;
    *L = b; L->sid = sid; if (i >= nBuild) nBuild = i + 1;
    bool solid = b.kind != BK_SHRINE && b.kind != BK_WARD && b.kind != BK_GARDEN && b.kind != BK_SPIKES && b.kind != BK_ALARM;
    if (solid && L->sid < 0 && b.alive) L->sid = addSolidF(b.mn, b.mx, 0);
    if (L->sid >= 0) sol[L->sid].off = !b.alive || (b.kind == BK_DOOR && b.open);
    buildS[i] = *L; buildS[i].sid = 0; if (nBuild > nBuildS) nBuildS = nBuild;
}
static void netScan(void) {   // what changed here since the last look goes out
    WB w;
    for (int i = 0; i < nOps; i++) if (ops[i].state != opS[i][0] || fabsf(ops[i].hp - opH[i]) > 0.01f) { w.n = 0; msgOp(&w, i); relAll(&w, -1); opS[i][0] = (unsigned char)ops[i].state; opH[i] = ops[i].hp; }
    for (int i = 0; i < nDeb; i++) if (debs[i].alive != debS[i]) { w.n = 0; WV(&w, (unsigned char)R_DEB); WV(&w, i); WV(&w, (unsigned char)debs[i].alive); relAll(&w, -1); debS[i] = debs[i].alive; }
    for (int i = 0; i < nVend; i++) if (vends[i].broken != vendS[i]) { w.n = 0; WV(&w, (unsigned char)R_VEND); WV(&w, i); WV(&w, (unsigned char)vends[i].broken); relAll(&w, -1); vendS[i] = vends[i].broken; }
    for (int i = 0; i < nCrate; i++) if (crates[i].alive != crateS[i]) { w.n = 0; WV(&w, (unsigned char)R_CRATE); WV(&w, i); WV(&w, (unsigned char)crates[i].alive); relAll(&w, -1); crateS[i] = crates[i].alive; }
    for (int i = 0; i < nCont; i++) { unsigned int h = contHash(&conts[i]); if (i >= nContS || h != contS[i]) { w.n = 0; msgCont(&w, i); relAll(&w, -1); contS[i] = h; } } if (nCont > nContS) nContS = nCont;
    for (int i = 0; i < nBuild; i++) { Build t = builds[i]; t.sid = 0; if (i >= nBuildS || memcmp(&t, &buildS[i], sizeof t)) { w.n = 0; msgBuild(&w, i); relAll(&w, -1); buildS[i] = t; } } if (nBuild > nBuildS) nBuildS = nBuild;
    for (int i = 0; i < solSN && i < nSol; i++) if (sol[i].off != solS[i]) { w.n = 0; WV(&w, (unsigned char)R_SOL); WV(&w, i); WV(&w, (unsigned char)sol[i].off); relAll(&w, -1); solS[i] = sol[i].off; }
}
static void netSendWorldTo(int k) {   // a player who just joined gets every change made since the town was built
    WB w;
    for (int i = 0; i < nOps; i++) if (ops[i].state != opP[i]) { w.n = 0; msgOp(&w, i); relSend(k, &w); }
    for (int i = 0; i < nDeb; i++) if (!debs[i].alive) { w.n = 0; WV(&w, (unsigned char)R_DEB); WV(&w, i); WV(&w, (unsigned char)0); relSend(k, &w); }
    for (int i = 0; i < nVend; i++) if (vends[i].broken) { w.n = 0; WV(&w, (unsigned char)R_VEND); WV(&w, i); WV(&w, (unsigned char)1); relSend(k, &w); }
    for (int i = 0; i < nCrate; i++) if (!crates[i].alive) { w.n = 0; WV(&w, (unsigned char)R_CRATE); WV(&w, i); WV(&w, (unsigned char)0); relSend(k, &w); }
    for (int i = 0; i < nCont; i++) if (i >= nContP || contHash(&conts[i]) != contP[i]) { w.n = 0; msgCont(&w, i); relSend(k, &w); }
    for (int i = 0; i < nBuild; i++) { w.n = 0; msgBuild(&w, i); relSend(k, &w); }
    for (int i = 0; i < solSN && i < nSol; i++) if (sol[i].off != solP[i]) { w.n = 0; WV(&w, (unsigned char)R_SOL); WV(&w, i); WV(&w, (unsigned char)sol[i].off); relSend(k, &w); }
}
static void netSay(const char *s, int except) { WB w = {0}; WV(&w, (unsigned char)R_SAY); char b[96] = {0}; snprintf(b, sizeof b, "%s", s); wb(&w, b, sizeof b); relAll(&w, except); }

// ---- damage between machines
static void netPlayerDamage(int slot, float dmg, float bleed) {
    if (slot == mySlot) { hurtPlayer(dmg, bleed); return; }
    WB w = {0}; WV(&w, (unsigned char)R_PDMG); WV(&w, (unsigned char)slot); WV(&w, dmg); WV(&w, bleed);
    if (netMode == NM_HOST) relSend(slot, &w); else relSend(0, &w);
}
static void netCreatureHit(int idx, float dmg) { WB w = {0}; WV(&w, (unsigned char)R_HIT); WV(&w, (unsigned short)idx); WV(&w, dmg); relSend(0, &w); }

// ---- one inner reliable message, from peer k
static void netApply(RB *r, int k) {
    unsigned char t = RV(r, unsigned char); WB fw = {0}; bool fwd = netMode == NM_HOST; wb(&fw, r->b + r->i - 1, r->n - r->i + 1);
    if (t == R_OP) { int i = RV(r, int); unsigned char st = RV(r, unsigned char); float hp = RV(r, float); Vector3 mn = RV(r, Vector3), mx = RV(r, Vector3); if (r->bad) return; applyOp(i, st, hp, mn, mx); }
    else if (t == R_DEB) { int i = RV(r, int); unsigned char a = RV(r, unsigned char); if (r->bad || i < 0 || i >= nDeb) return; debs[i].alive = a; debS[i] = a; if (debs[i].sid >= 0) { sol[debs[i].sid].off = !a; if (debs[i].sid < solSN) solS[debs[i].sid] = !a; } }
    else if (t == R_VEND) { int i = RV(r, int); unsigned char b = RV(r, unsigned char); if (r->bad || i < 0 || i >= nVend) return; vends[i].broken = b; vendS[i] = b; if (b && vends[i].light >= 0) lights[vends[i].light].mode = 2; }
    else if (t == R_CRATE) { int i = RV(r, int); unsigned char a = RV(r, unsigned char); if (r->bad || i < 0 || i >= nCrate) return; crates[i].alive = a; crateS[i] = a; if (crates[i].sid >= 0) { sol[crates[i].sid].off = !a; if (crates[i].sid < solSN) solS[crates[i].sid] = !a; } }
    else if (t == R_CONT) { int i = RV(r, int); Cont c = RV(r, Cont); if (r->bad || i < 0 || i >= MAX_CONT) return;
        int sid = i < nCont ? conts[i].sid : c.sid; conts[i] = c; conts[i].sid = sid; if (i >= nCont) nCont = i + 1; contS[i] = contHash(&conts[i]); if (nCont > nContS) nContS = nCont; }
    else if (t == R_BUILD) { int i = RV(r, int); Build b = RV(r, Build); if (r->bad) return; applyBuild(i, b); }
    else if (t == R_SOL) { int i = RV(r, int); unsigned char o = RV(r, unsigned char); if (r->bad || i < 0 || i >= solSN) return; sol[i].off = o; solS[i] = o; }
    else if (t == R_HIT) { fwd = false; unsigned short i = RV(r, unsigned short); float d = RV(r, float); if (r->bad || netMode != NM_HOST || i >= nCr) return; Cr *c = &crs[i]; if (!c->on || c->state == CS_DEAD) return;
        c->hp -= d; c->hurt = 0.18f; c->wind = 0; if (c->hp <= 0) { c->state = CS_DEAD; c->deadT = 0; } else if (c->state == CS_WANDER) { c->state = CS_CHASE; c->lastKnown = rps[k].pos; } }
    else if (t == R_PDMG) { fwd = false; unsigned char v = RV(r, unsigned char); float d = RV(r, float), b = RV(r, float); if (r->bad) return;
        if (v == mySlot) { if (pl.hp > 0) hurtPlayer(d, b); }
        else if (netMode == NM_HOST && v < MP_MAX && rps[v].on) { WB w = {0}; WV(&w, (unsigned char)R_PDMG); WV(&w, v); WV(&w, d); WV(&w, b); relSend(v, &w); } }
    else if (t == R_SAY) { char b[96]; rbr(r, b, sizeof b); if (r->bad) return; b[95] = 0; say(b); }
    else return;
    if (fwd) relAll(&fw, k);   // the host passes what a client changed on to everyone else
}

// ---- sessions
static void netStop(const char *why) {
    if (netMode == NM_OFF) return;
    WB w = {0}; WV(&w, (unsigned char)P_BYE); WV(&w, (unsigned char)mySlot);
    if (netMode == NM_CLIENT) netSend(&hostAddr, w.b, w.n); else for (int k = 0; k < MP_MAX; k++) if (k != mySlot && rps[k].on) netSend(&rps[k].addr, w.b, w.n);
    netClose(); netMode = NM_OFF; netJoining = false; memset(rps, 0, sizeof rps); mySlot = 0;
    if (why) snprintf(netStatus, sizeof netStatus, "%s", why);
}
static bool netHost(int port) {
    netStop(NULL); netResetChannels(); memset(rps, 0, sizeof rps);
    if (!netOpen(port)) { snprintf(netStatus, sizeof netStatus, "Could not open UDP port %d (in use?)", port); return false; }
    netMode = NM_HOST; mySlot = 0; netSeed = worldSeed; shadowAll();
    for (int i = 0; i < nOps; i++) opP[i] = (unsigned char)ops[i].state; for (int i = 0; i < nCont; i++) contP[i] = contS[i]; nContP = nCont; free(solP); solP = malloc((size_t)solSN + 1); memcpy(solP, solS, (size_t)solSN);
    snprintf(netStatus, sizeof netStatus, "Hosting on UDP port %d. Others join with this computer's IP.", port); return true;
}
static bool netJoin(const char *host, int port) {
    netStop(NULL); netResetChannels(); memset(rps, 0, sizeof rps);
    if (!netResolve(host, port, &hostAddr)) { snprintf(netStatus, sizeof netStatus, "Unknown address: %s", host); return false; }
    if (!netOpen(0)) { snprintf(netStatus, sizeof netStatus, "Could not open a UDP socket"); return false; }
    netMode = NM_CLIENT; netJoining = true; netJoinT = GetTime() - 10; snprintf(netStatus, sizeof netStatus, "Connecting to %s:%d ...", host, port); return true;
}
static int slotOf(const NetAddr *a) { for (int k = 0; k < MP_MAX; k++) if (k != mySlot && rps[k].on && rps[k].addr.ip == a->ip && rps[k].addr.port == a->port) return k; return -1; }
static void startClientWorld(unsigned int seed);
static void netRecvAll(double now) {
    unsigned char buf[1500]; NetAddr from;
    for (int guard = 0; guard < 2000; guard++) {
        int n = netRecv(buf, sizeof buf, &from); if (n <= 0) break;
        RB r = {buf, n, 0, false}; unsigned char t = RV(&r, unsigned char);
        if (netMode == NM_HOST) {
            int k = slotOf(&from);
            if (t == P_JOIN) {
                unsigned char proto = RV(&r, unsigned char); char name[16]; rbr(&r, name, 16); name[15] = 0;
                WB w = {0};
                if (proto != MP_PROTO) { WV(&w, (unsigned char)P_REJECT); char m[64] = "Different game version"; wb(&w, m, 64); netSend(&from, w.b, w.n); continue; }
                if (k < 0) { for (int s = 0; s < MP_MAX; s++) if (s != mySlot && !rps[s].on) { k = s; break; }
                    if (k < 0) { WV(&w, (unsigned char)P_REJECT); char m[64] = "The game is full (8 players)"; wb(&w, m, 64); netSend(&from, w.b, w.n); continue; }
                    memset(rq[k], 0, sizeof rq[k]); rseq[k] = 0; memset(rseen[k], 0, sizeof rseen[k]); nAck[k] = 0;
                    rps[k] = (RPlayer){0}; rps[k].on = true; rps[k].alive = true; rps[k].addr = from; rps[k].pos = rps[k].shown = V3(SPAWN_X, SIDEH, SPAWN_Z); rps[k].hp = 100; snprintf(rps[k].name, 16, "%s", name);
                    netSendWorldTo(k); char b[96]; snprintf(b, sizeof b, "%s joined.", rps[k].name); say(b); netSay(b, k); }
                rps[k].heard = now;
                WV(&w, (unsigned char)P_WELCOME); WV(&w, (unsigned char)k); WV(&w, netSeed); WV(&w, (unsigned char)netPvp); netSend(&from, w.b, w.n);
                continue;
            }
            if (k < 0) continue;
            rps[k].heard = now;
            if (t == P_STATE) { Vector3 p = RV(&r, Vector3); float yw = RV(&r, float), pt = RV(&r, float), hp = RV(&r, float); unsigned char fl = RV(&r, unsigned char); short held = RV(&r, short);
                if (r.bad) continue; rps[k].pos = p; rps[k].yaw = yw; rps[k].pitch = pt; rps[k].hp = hp; rps[k].flash = fl & 1; rps[k].alive = (fl & 2) != 0; rps[k].held = held; }
            else if (t == P_BYE) { char b[96]; snprintf(b, sizeof b, "%s left.", rps[k].name); say(b); rps[k].on = false; netSay(b, k); }
        } else if (netMode == NM_CLIENT) {
            if (from.ip != hostAddr.ip || from.port != hostAddr.port) continue;
            rps[0].heard = now;
            if (t == P_WELCOME && netJoining) { mySlot = RV(&r, unsigned char); unsigned int seed = RV(&r, unsigned int); netPvp = RV(&r, unsigned char); if (r.bad) continue;
                netJoining = false; snprintf(netStatus, sizeof netStatus, "Connected as player %d.", mySlot + 1); startClientWorld(seed); continue; }
            if (t == P_REJECT) { char m[64]; rbr(&r, m, 64); m[63] = 0; netStop(NULL); snprintf(netStatus, sizeof netStatus, "Refused: %s", m); continue; }
            if (t == P_BYE) { netStop("The host closed the game."); if (state == ST_PLAY || state == ST_PAUSE || state == ST_DEAD) { state = ST_MENU; EnableCursor(); gameActive = false; } say("The host closed the game."); continue; }
            if (netJoining) continue;
            if (t == P_PLAYERS) { float tod = RV(&r, float); int day = RV(&r, int); netPvp = RV(&r, unsigned char); unsigned char cnt = RV(&r, unsigned char); if (r.bad) continue;
                pl.tod = tod; pl.day = day; bool seen[MP_MAX] = {0};
                for (int e = 0; e < cnt; e++) { unsigned char s = RV(&r, unsigned char), fl = RV(&r, unsigned char); Vector3 p = RV(&r, Vector3); float yw = RV(&r, float), pt = RV(&r, float), hp = RV(&r, float); short held = RV(&r, short); char nm[16]; rbr(&r, nm, 16); nm[15] = 0;
                    if (r.bad || s >= MP_MAX || s == mySlot) continue; RPlayer *q = &rps[s]; if (!q->on) { q->shown = p; q->shownYaw = yw; }
                    q->on = true; seen[s] = true; q->alive = fl & 2; q->flash = fl & 1; q->pos = p; q->yaw = yw; q->pitch = pt; q->hp = hp; q->held = held; memcpy(q->name, nm, 16); }
                for (int s = 0; s < MP_MAX; s++) if (s != mySlot && !seen[s]) rps[s].on = false; }
            else if (t == P_CRS) { unsigned short cnt = RV(&r, unsigned short);
                for (int e = 0; e < cnt; e++) { unsigned short id = RV(&r, unsigned short); unsigned char ty = RV(&r, unsigned char), st = RV(&r, unsigned char), fl = RV(&r, unsigned char); Vector3 p = RV(&r, Vector3); float yw = RV(&r, float), hp = RV(&r, float);
                    if (r.bad || id >= MAX_CR) break; Cr *c = &crs[id]; if (id >= nCr) nCr = id + 1;
                    if (!c->on || c->type != ty || c->phantom) { memset(c, 0, sizeof *c); c->pos = p; c->yaw = yw; c->var = id; }
                    c->on = true; c->type = ty; c->state = st; c->hanging = fl & 1; c->tgt = p; c->hp = hp; c->deadT = st == CS_DEAD ? fmaxf(c->deadT, 0) : 0; c->lastD = (float)now; c->kb = V3(yw, 0, 0); } }
        }
        if (t == P_REL) { unsigned short seq = RV(&r, unsigned short); if (r.bad) continue; int k = netMode == NM_CLIENT ? 0 : slotOf(&from); if (k < 0) continue;
            if (nAck[k] < 256) ackq[k][nAck[k]++] = seq; if (relSeen(k, seq)) continue; netApply(&r, k); }
        else if (t == P_ACK) { unsigned short cnt = RV(&r, unsigned short); int k = netMode == NM_CLIENT ? 0 : slotOf(&from); if (k < 0) continue;
            for (int a = 0; a < cnt; a++) { unsigned short s = RV(&r, unsigned short); if (r.bad) break; for (int i = 0; i < RQN; i++) if (rq[k][i].used && rq[k][i].seq == s) { rq[k][i].used = false; break; } } }
    }
}
static void netUpdate(float dt) {   // every frame, in every state
    (void)dt; if (netMode == NM_OFF) return;
    double now = GetTime();
    netRecvAll(now);
    if (netMode == NM_OFF) return;
    if (netMode == NM_CLIENT && netJoining) {
        if (now - netJoinT > 1.0) { netJoinT = now; WB w = {0}; WV(&w, (unsigned char)P_JOIN); WV(&w, (unsigned char)MP_PROTO); char nm[16] = {0}; snprintf(nm, 16, "%s", prof.name); wb(&w, nm, 16); netSend(&hostAddr, w.b, w.n); }
        static double first = 0; if (first == 0) first = now; if (now - first > 12) { first = 0; netStop("No answer from the host (check the IP, the port and the host's firewall)."); }
        return;
    }
    if (netMode == NM_CLIENT && now - rps[0].heard > 10 && rps[0].heard > 0) { netStop("Lost the connection to the host."); state = ST_MENU; EnableCursor(); gameActive = false; return; }
    if (netMode == NM_HOST) for (int k = 0; k < MP_MAX; k++) if (k != mySlot && rps[k].on && now - rps[k].heard > 10) { char b[96]; snprintf(b, sizeof b, "%s lost connection.", rps[k].name); say(b); rps[k].on = false; netSay(b, k); }
    if (now - netScanT > 0.2) { netScanT = now; if (gameActive) netScan(); }
    if (now - netSendT > 0.05) { netSendT = now;
        unsigned char fl = (pl.flash ? 1 : 0) | (pl.hp > 0 && state != ST_DEAD ? 2 : 0);
        if (netMode == NM_CLIENT) { WB w = {0}; WV(&w, (unsigned char)P_STATE); WV(&w, pl.pos); WV(&w, pl.yaw); WV(&w, pl.pitch); WV(&w, pl.hp); WV(&w, fl); WV(&w, (short)pl.hands.t); netSend(&hostAddr, w.b, w.n); }
        else {
            WB w = {0}; WV(&w, (unsigned char)P_PLAYERS); WV(&w, pl.tod); WV(&w, pl.day); WV(&w, (unsigned char)netPvp);
            unsigned char cnt = 1; for (int k = 0; k < MP_MAX; k++) if (k != mySlot && rps[k].on) cnt++; WV(&w, cnt);
            char nm[16] = {0}; snprintf(nm, 16, "%s", prof.name);
            WV(&w, (unsigned char)mySlot); WV(&w, fl); WV(&w, pl.pos); WV(&w, pl.yaw); WV(&w, pl.pitch); WV(&w, pl.hp); WV(&w, (short)pl.hands.t); wb(&w, nm, 16);
            for (int k = 0; k < MP_MAX; k++) if (k != mySlot && rps[k].on) { RPlayer *q = &rps[k]; unsigned char f2 = (q->flash ? 1 : 0) | (q->alive ? 2 : 0);
                WV(&w, (unsigned char)k); WV(&w, f2); WV(&w, q->pos); WV(&w, q->yaw); WV(&w, q->pitch); WV(&w, q->hp); WV(&w, q->held); wb(&w, q->name, 16); }
            for (int k = 0; k < MP_MAX; k++) if (k != mySlot && rps[k].on) netSend(&rps[k].addr, w.b, w.n);
        }
    }
    if (netMode == NM_HOST && now - netCrT > 0.1) { netCrT = now;   // creatures near each client
        for (int k = 0; k < MP_MAX; k++) { if (k == mySlot || !rps[k].on) continue;
            WB w = {0}; int cntAt = 0; unsigned short cnt = 0;
            for (int i = 0; i <= nCr; i++) {
                if (i == nCr || (w.n > 1300)) { if (cnt) { memcpy(w.b + cntAt, &cnt, 2); netSend(&rps[k].addr, w.b, w.n); } if (i == nCr) break; w.n = 0; cnt = 0; }
                if (w.n == 0) { WV(&w, (unsigned char)P_CRS); cntAt = w.n; WV(&w, (unsigned short)0); }
                Cr *c = &crs[i]; if (!c->on || c->phantom || Vector3DistanceSqr(c->pos, rps[k].pos) > 95 * 95) continue;
                WV(&w, (unsigned short)i); WV(&w, (unsigned char)c->type); WV(&w, (unsigned char)c->state); WV(&w, (unsigned char)(c->hanging ? 1 : 0)); WV(&w, c->pos); WV(&w, c->yaw); WV(&w, c->hp); cnt++;
            }
        }
    }
    relPump(now);
}
// ---- the client's creatures follow the host's
static void netClientCreatures(float dt) {
    double now = GetTime();
    for (int i = 0; i < nCr; i++) { Cr *c = &crs[i]; if (!c->on || c->phantom) continue;
        if (now - c->lastD > 1.0) { c->on = false; continue; }   // out of range, or gone
        c->t += dt; c->hurt -= dt; c->flinch = fmaxf(0, c->flinch - dt * 3.5f); if (c->state == CS_DEAD) c->deadT += dt;
        c->pos = Vector3Lerp(c->pos, c->tgt, fminf(1, dt * 10)); c->yaw += wrapA(c->kb.x - c->yaw) * fminf(1, dt * 10);
        c->twT -= dt; if (c->twT <= 0) { for (int k = 0; k < 6; k++) c->tw[k] = fr(-0.3f, 0.3f); c->twT = fr(0.1f, 0.4f); } }
}
// ---- other players in the world: a simple figure and a name
static void drawRemotePlayers(float dt) {
    if (netMode == NM_OFF) return;
    for (int k = 0; k < MP_MAX; k++) { RPlayer *q = &rps[k]; if (k == mySlot || !q->on) continue;
        q->shown = Vector3Lerp(q->shown, q->pos, fminf(1, dt * 12)); q->shownYaw += wrapA(q->yaw - q->shownYaw) * fminf(1, dt * 12);
        Matrix M = TRS(q->shown, q->shownYaw, V3(1, 1, 1));
        if (!q->alive) M = MatrixMultiply(MatrixRotateX(-PI / 2), TRS(V3(q->shown.x, q->shown.y + 0.15f, q->shown.z), q->shownYaw, V3(1, 1, 1)));
        ADRAW("player/survivor3", M, WHITE, BX(T_PLAIN, -0.11f, 0.42f, 0, 0.16f, 0.84f, 0.18f, ((Color){36, 40, 54, 255})), BX(T_PLAIN, 0.11f, 0.42f, 0, 0.16f, 0.84f, 0.18f, ((Color){36, 40, 54, 255})),
              BX(T_PLAIN, -0.11f, 0.04f, 0.04f, 0.17f, 0.08f, 0.26f, ((Color){28, 26, 24, 255})), BX(T_PLAIN, 0.11f, 0.04f, 0.04f, 0.17f, 0.08f, 0.26f, ((Color){28, 26, 24, 255})),
              BX(T_PLAIN, 0, 1.18f, 0, 0.46f, 0.68f, 0.26f, ((Color){44, 54, 42, 255})), BX(T_PLAIN, 0, 1.4f, 0.133f, 0.16f, 0.18f, 0.01f, ((Color){200, 196, 186, 255})),
              BX(T_PLAIN, -0.29f, 1.14f, 0, 0.12f, 0.62f, 0.14f, ((Color){44, 54, 42, 255})), BX(T_PLAIN, 0.29f, 1.14f, 0, 0.12f, 0.62f, 0.14f, ((Color){44, 54, 42, 255})),
              BX(T_PLAIN, -0.29f, 0.79f, 0, 0.1f, 0.1f, 0.1f, ((Color){150, 110, 88, 255})), BX(T_PLAIN, 0.29f, 0.79f, 0, 0.1f, 0.1f, 0.1f, ((Color){150, 110, 88, 255})),
              BX(T_PLAIN, 0, 1.68f, 0, 0.24f, 0.28f, 0.24f, ((Color){150, 110, 88, 255})), BX(T_PLAIN, 0, 1.84f, -0.02f, 0.27f, 0.08f, 0.28f, ((Color){30, 24, 20, 255})), BX(T_PLAIN, 0, 1.72f, -0.13f, 0.27f, 0.22f, 0.04f, ((Color){30, 24, 20, 255})),
              BX(T_PLAIN, -0.055f, 1.71f, 0.121f, 0.04f, 0.03f, 0.01f, ((Color){20, 20, 20, 255})), BX(T_PLAIN, 0.055f, 1.71f, 0.121f, 0.04f, 0.03f, 0.01f, ((Color){20, 20, 20, 255})));
        if (q->flash && q->alive) { Vector3 d = V3(sinf(q->yaw) * cosf(q->pitch), sinf(q->pitch), cosf(q->yaw) * cosf(q->pitch)); DrawCube(Vector3Add(Vector3Add(q->shown, V3(0, 1.3f, 0)), Vector3Scale(d, 0.35f)), 0.07f, 0.07f, 0.07f, (Color){255, 245, 210, 255}); }
    }
}
static void drawNameTags(void) {
    if (netMode == NM_OFF) return;
    for (int k = 0; k < MP_MAX; k++) { RPlayer *q = &rps[k]; if (k == mySlot || !q->on) continue;
        Vector3 head = Vector3Add(q->shown, V3(0, 2.15f, 0)); float d = Vector3Distance(head, cam.position);
        if (d > 45 || Vector3DotProduct(Vector3Subtract(head, cam.position), Vector3Subtract(cam.target, cam.position)) <= 0) continue;
        if (!lineOfSight(cam.position, Vector3Subtract(head, V3(0, 0.3f, 0)))) continue;
        Vector2 sp = GetWorldToScreenEx(head, cam, GetScreenWidth(), GetScreenHeight());
        char b[48]; snprintf(b, sizeof b, q->alive ? "%s" : "%s (down)", q->name);
        aTextC(b, sp.x, sp.y, d < 15 ? 16 : 13, q->alive ? (Color){230, 226, 210, 255} : (Color){220, 90, 70, 255}, true);
    }
}
// ---- the multiplayer menu
static void mpPanel(void) {
    Rectangle p = p3Panel(3, 0, 0, 560, 520, true); float s = aU;
    if (aWindow(p, "Multiplayer")) state = ST_MENU;
    float x = p.x + 36 * s, w = 488 * s, y = p.y + 64 * s;
    aText(TextFormat("Playing as %s", prof.name), x, y, 16, C_BONE, true); y += 40 * s;
    aText("Host a game", x, y, 16, C_CYAN, true); y += 30 * s;
    netPvp = aToggle(x, y, "Players can hurt each other (PvP)", netPvp); y += 36 * s;
    aTextBox((Rectangle){x, y + 18 * s, 150 * s, 34 * s}, "Port (UDP)", &mpPort); y += 66 * s;
    if (aButton((Rectangle){x, y, w, 42 * s}, "Host a new game (up to 8 players)", netMode == NM_OFF)) {
        int port = atoi(mpPort.buf); if (port <= 0 || port > 65535) port = MP_PORT;
        gSeedArg = 0; newGame(); if (netHost(port)) { state = ST_PLAY; DisableCursor(); } }
    y += 64 * s;
    aText("Join a game", x, y, 16, C_CYAN, true); y += 30 * s;
    aTextBox((Rectangle){x, y + 18 * s, 300 * s, 34 * s}, "Host IP or name", &mpIP); y += 66 * s;
    if (aButton((Rectangle){x, y, w, 42 * s}, netJoining ? "Connecting..." : "Join", netMode == NM_OFF)) { int port = atoi(mpPort.buf); if (port <= 0 || port > 65535) port = MP_PORT; netJoin(mpIP.buf, port); }
    y += 56 * s;
    if (netMode != NM_OFF && aButton((Rectangle){x, y, w, 36 * s}, "Disconnect", true)) netStop("Disconnected.");
    aText(netStatus, x, p.y + p.height - 70 * s, 13, C_BONE2, false);
    aText("Over the internet the host forwards the UDP port, or both use a VPN such as Radmin VPN.", x, p.y + p.height - 44 * s, 12, C_BONE2, false);
    p3End(3);
}
static void startClientWorld(unsigned int seed) {   // the host's town, an empty street: creatures come from the host
    gSeedArg = seed; newGame(); gSeedArg = 0;
    for (int i = 0; i < nCr; i++) crs[i].on = false; nCr = 0;
    shadowAll(); state = ST_PLAY; DisableCursor(); say("Connected. Same town, same night.");
}
