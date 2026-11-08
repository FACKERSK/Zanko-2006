// zk_cont.h — searchable containers (furniture, lockers, car trunks, dropped piles) and their loot tables

enum { CK_PILE, CK_CABINET, CK_FRIDGE, CK_WARDROBE, CK_DESK, CK_SHELF, CK_LOCKER, CK_BOX, CK_TOOLBOX, CK_TRUNK, CK_OFFERING, CK_COUNTER, CK_CHEST, CK_GUNLOCKER, CK_FIRECAB, CK_N };
static const char *ckName[CK_N] = {"Dropped items", "Kitchen cabinet", "Refrigerator", "Wardrobe", "Desk", "Store shelf", "Locker", "Cardboard boxes", "Toolbox", "Car trunk",
                                   "Offering box", "Counter", "Storage chest", "Police locker", "Fire cabinet"};
static const int ckSlots[CK_N] = {16, 6, 6, 8, 4, 8, 6, 4, 4, 8, 4, 6, 16, 6, 3};
// footprint (w along the facing wall, h, d into the room)
static const Vector3 ckSize[CK_N] = {{0.5f, 0.2f, 0.5f}, {1.0f, 0.9f, 0.5f}, {0.7f, 1.7f, 0.65f}, {1.2f, 1.9f, 0.55f}, {1.2f, 0.75f, 0.6f}, {1.6f, 1.6f, 0.45f}, {0.5f, 1.8f, 0.5f},
                                      {0.7f, 0.5f, 0.5f}, {0.5f, 0.25f, 0.3f}, {1.2f, 0.4f, 0.6f}, {0.6f, 0.55f, 0.4f}, {1.4f, 1.0f, 0.6f}, {1.0f, 0.6f, 0.6f}, {0.6f, 1.8f, 0.5f}, {0.6f, 0.8f, 0.25f}};
#define MAX_CONT 12000
#define CSLOTS 16
typedef struct { Vector3 pos; float yaw; int kind, region, sid; bool on, filled, searched; Slot s[CSLOTS]; } Cont;
static Cont conts[MAX_CONT]; static int nCont;
static void slotsClear(Slot *s, int n) { for (int i = 0; i < n; i++) { s[i].t = -1; s[i].n = 0; s[i].w = 0; } }
// add n of type t into a slot array; returns how many did not fit
static int slotsAdd(Slot *s, int ns, int t, int n) {
    int st = idef[t].stack;
    for (int i = 0; i < ns && n > 0; i++) if (s[i].t == t && s[i].n < st) { int k = st - s[i].n; if (k > n) k = n; s[i].n += k; n -= k; }
    for (int i = 0; i < ns && n > 0; i++) if (s[i].t < 0) { int k = n < st ? n : st; s[i].t = (short)t; s[i].n = (short)k; s[i].w = 0; n -= k; }
    return n;
}
static int contAt(Vector3 p, float r, int kind) {
    for (int i = 0; i < nCont; i++) if (conts[i].on && conts[i].kind == kind && Vector3Distance(conts[i].pos, p) < r) return i;
    return -1;
}
static int addCont(int kind, Vector3 p, float yaw, int region, bool solid) {
    int slot = -1;
    for (int i = 0; i < nCont; i++) if (!conts[i].on) { slot = i; break; }
    if (slot < 0) { if (nCont >= MAX_CONT) return -1; slot = nCont++; }
    Cont *c = &conts[slot]; memset(c, 0, sizeof *c);
    c->pos = xfP(curXf, p); c->yaw = yaw + xfYaw(curXf); c->kind = kind; c->region = region; c->on = true; c->sid = -1;   // stored in the world; the solid stays in the building's frame
    slotsClear(c->s, CSLOTS);
    if (kind == CK_PILE || kind == CK_CHEST) c->filled = true;
    if (solid) {
        Vector3 sz = ckSize[kind]; bool rot = fabsf(sinf(yaw)) > 0.5f;
        float hx = (rot ? sz.z : sz.x) / 2, hz = (rot ? sz.x : sz.z) / 2;
        c->sid = addSolid(V3(p.x - hx, p.y, p.z - hz), V3(p.x + hx, p.y + sz.y, p.z + hz));
    }
    return slot;
}
// floor items no longer exist: anything "dropped" in the world goes into a pile container
static void addItem(Vector3 p, int type, int amount) {
    int sv = curXf; p = xfP(sv, p); curXf = 0;   // piles live in the world
    int c = contAt(p, 1.2f, CK_PILE);
    if (c < 0) c = addCont(CK_PILE, p, 0, 0, false);
    if (c < 0) { curXf = sv; return; }
    int left = slotsAdd(conts[c].s, CSLOTS, type, amount);
    if (left > 0) { int c2 = addCont(CK_PILE, Vector3Add(p, V3(0.6f, 0, 0)), 0, 0, false); if (c2 >= 0) slotsAdd(conts[c2].s, CSLOTS, type, left); }
    curXf = sv;
}
static bool contEmpty(Cont *c) { for (int i = 0; i < CSLOTS; i++) if (c->s[i].t >= 0) return false; return true; }

// ---- loot tables: {type, weight}
typedef struct { short t; float w; } LootW;
#define LT(...) (LootW[]){__VA_ARGS__}, (int)(sizeof((LootW[]){__VA_ARGS__}) / sizeof(LootW))
static int lootPick(const LootW *tb, int n) {
    float sum = 0; for (int i = 0; i < n; i++) sum += tb[i].w;
    float r = fr(0, sum);
    for (int i = 0; i < n; i++) { r -= tb[i].w; if (r <= 0) return tb[i].t; }
    return tb[n - 1].t;
}
static int lootQty(int t) {
    switch (t) { case IT_PLANKS: return 2 + ri(3); case IT_NAILS: return 6 + ri(12); case IT_AMMO: return 2 + ri(5); case IT_RAG: return 1 + ri(3);
                 case IT_SCRAP: return 1 + ri(2); case IT_OFUDA: return 1 + ri(2); case IT_SEEDS: return 2 + ri(3); case IT_CHARCOAL: return 1 + ri(2); default: return 1; }
}
static void lootFill0(Cont *c);
static void lootFill(Cont *c) {   // what a container holds depends only on the world and the container: every player in a session finds the same
    if (c->filled) return;
    unsigned int save = rs; rs = (worldSeed * 2654435761u) ^ ((unsigned int)(c - conts) * 40503u + 12345u); if (!rs) rs = 1;
    lootFill0(c); rs = save;
}
static void lootFill0(Cont *c) {
    if (c->filled) return;
    c->filled = true;
    int rolls = 0; const LootW *tb = NULL; int n = 0;
    bool sub = c->region == 9;
    switch (c->kind) {
    case CK_CABINET: rolls = 1 + ri(3); { static LootW t[] = {{IT_CANNED, 3}, {IT_RAMEN, 3}, {IT_FOOD, 2}, {IT_DRINK, 2}, {IT_WATER, 2}, {IT_KNIFE, 0.8f}, {IT_RAG, 2}, {IT_BATTERY, 1}, {IT_SALT, 0.8f}, {IT_SEEDS, 0.7f}, {IT_DISINFECT, 0.5f}, {IT_BOTTLE, 1.2f}}; tb = t; n = 12; } break;
    case CK_FRIDGE: rolls = 1 + ri(2); { static LootW t[] = {{IT_DRINK, 4}, {IT_WATER, 3}, {IT_FOOD, 3}, {IT_CANNED, 1}}; tb = t; n = 4; } break;
    case CK_WARDROBE: rolls = 1 + ri(3); { static LootW t[] = {{IT_RAG, 5}, {IT_SCHOOLBAG, 0.9f}, {IT_DUFFEL, 0.6f}, {IT_BAT, 0.9f}, {IT_MANGA, 1.2f}, {IT_PAINKILLER, 0.8f}, {IT_BANDAGE, 0.8f}, {IT_KATANA, 0.07f}, {IT_VHS, 0.6f}}; tb = t; n = 9; } break;
    case CK_DESK: rolls = ri(3); { static LootW t[] = {{IT_MANGA, 3}, {IT_VHS, 2}, {IT_BATTERY, 2}, {IT_PAINKILLER, 1}, {IT_OFUDA, 0.5f}, {IT_KNIFE, 0.4f}, {IT_NAILS, 1}}; tb = t; n = 7; } break;
    case CK_SHELF: rolls = 1 + ri(3); { static LootW t[] = {{IT_FOOD, 3}, {IT_DRINK, 3}, {IT_RAMEN, 2}, {IT_CANNED, 2}, {IT_BATTERY, 2}, {IT_BANDAGE, 1}, {IT_SALT, 1}, {IT_RAG, 1}, {IT_WATER, 2}, {IT_SEEDS, 1}, {IT_CHARCOAL, 0.6f}, {IT_DISINFECT, 0.6f}}; tb = t; n = 12; } break;
    case CK_LOCKER: rolls = 1 + ri(2) + (sub ? 1 : 0);
        if (sub) { static LootW t[] = {{IT_RAG, 2}, {IT_BANDAGE, 2.5f}, {IT_PAINKILLER, 1.5f}, {IT_AMMO, 1.4f}, {IT_PIPE, 1}, {IT_CROWBAR, 1}, {IT_KATANA, 0.2f}, {IT_SLEDGE, 0.25f}, {IT_HIKEPACK, 0.35f}, {IT_DUFFEL, 0.6f}, {IT_WATER, 1}}; tb = t; n = 11; }
        else { static LootW t[] = {{IT_RAG, 2}, {IT_BANDAGE, 2}, {IT_PAINKILLER, 1}, {IT_BAT, 1}, {IT_PIPE, 1}, {IT_CROWBAR, 0.8f}, {IT_DUFFEL, 0.8f}, {IT_AMMO, 0.4f}, {IT_HIKEPACK, 0.25f}, {IT_DISINFECT, 0.6f}, {IT_SPLINT, 0.4f}}; tb = t; n = 11; } break;
    case CK_BOX: rolls = ri(3); { static LootW t[] = {{IT_PLANKS, 3}, {IT_NAILS, 2}, {IT_SCRAP, 2}, {IT_RAG, 1}, {IT_FOOD, 1}, {IT_SLEDGE, 0.05f}, {IT_PIPE, 0.8f}, {IT_HAMMER, 0.4f}}; tb = t; n = 8; } break;
    case CK_TOOLBOX: rolls = 1 + ri(2); { static LootW t[] = {{IT_HAMMER, 3}, {IT_NAILS, 4}, {IT_CROWBAR, 1}, {IT_SCRAP, 2}}; tb = t; n = 4; } break;
    case CK_TRUNK: rolls = ri(3); { static LootW t[] = {{IT_SCRAP, 2}, {IT_RAG, 1}, {IT_WATER, 1}, {IT_CROWBAR, 0.8f}, {IT_HAMMER, 0.7f}, {IT_DUFFEL, 0.5f}, {IT_BAT, 0.5f}, {IT_PLANKS, 1}}; tb = t; n = 8; } break;
    case CK_OFFERING: rolls = 1 + ri(2); { static LootW t[] = {{IT_OFUDA, 4}, {IT_SALT, 2}, {IT_ROPE, 2}, {IT_FOOD, 1}}; tb = t; n = 4; } break;
    case CK_COUNTER: rolls = ri(3); { static LootW t[] = {{IT_BATTERY, 2}, {IT_PAINKILLER, 1}, {IT_BANDAGE, 1}, {IT_MANGA, 1}, {IT_VHS, 1}, {IT_RAG, 1}, {IT_CHARCOAL, 0.8f}, {IT_DISINFECT, 0.8f}}; tb = t; n = 8; } break;
    case CK_GUNLOCKER: slotsAdd(c->s, CSLOTS, IT_REVOLVER, 1); slotsAdd(c->s, CSLOTS, IT_AMMO, 4 + ri(4));
        rolls = 1; { static LootW t[] = {{IT_BANDAGE, 2}, {IT_CROWBAR, 1}, {IT_AMMO, 1}}; tb = t; n = 3; } break;
    case CK_FIRECAB: if (fr(0, 1) < 0.6f) slotsAdd(c->s, CSLOTS, IT_AXE, 1); rolls = ri(2); { static LootW t[] = {{IT_BANDAGE, 1}, {IT_RAG, 1}}; tb = t; n = 2; } break;
    default: break;
    }
    if (tb) for (int k = 0; k < rolls; k++) { int t = lootPick(tb, n); slotsAdd(c->s, ckSlots[c->kind] < CSLOTS ? ckSlots[c->kind] : CSLOTS, t, lootQty(t)); }
    for (int i = 0; i < CSLOTS; i++) if (c->s[i].t >= 0 && idef[c->s[i].t].cat == CAT_MELEE && c->s[i].w == 0) c->s[i].w = (short)(50 + ri(500));   // found weapons are already worn
}
