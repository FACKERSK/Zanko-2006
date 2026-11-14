// zk_ui.h — HUD (vitals, bleeding, quickbar, build panel) and the inventory screen (equipment, pockets, bag, container, skills)
static float hudCalmT = 0; static float hudPrev[6];
static Color catColor(int t) {
    switch (idef[t].cat) {
    case CAT_FOOD: return (Color){200, 150, 70, 255}; case CAT_DRINK: return (Color){80, 140, 190, 255}; case CAT_MED: return (Color){190, 60, 55, 255};
    case CAT_MELEE: return (Color){170, 175, 180, 255}; case CAT_GUN: return (Color){120, 120, 130, 255}; case CAT_AMMO: return (Color){180, 150, 70, 255};
    case CAT_MAT: return (Color){160, 120, 80, 255}; case CAT_SPIRIT: return (Color){225, 210, 160, 255}; case CAT_COMFORT: return (Color){150, 120, 200, 255};
    case CAT_BAG: return (Color){90, 130, 90, 255}; default: return (Color){150, 150, 150, 255};
    }
}
// one item cell: dark well, category gel chip, name, count
static void itemCell(Rectangle r, Slot sl, bool hot, bool sel) {
    float s = aU;
    aRR(aGrow(r, 1.5f * s), 6 * s, (Color){8, 10, 10, 255});
    aGrad(r, aRnd(r, 5 * s), (Color){30, 40, 42, 235}, (Color){14, 19, 20, 240}, 3);
    if (hot) aRRL(aGrow(r, 1 * s), 6 * s, 1, C_CYAN);
    if (sel) aRRL(aGrow(r, 2.5f * s), 7 * s, 1, C_AMBER);
    if (sl.t < 0) return;
    Color c = catColor(sl.t);
    if (iconOk) {   // the item's own model as an icon, on a soft category-coloured glow
        DrawCircleGradient((int)(r.x + r.width / 2), (int)(r.y + r.height * 0.42f), r.width * 0.36f, (Color){c.r, c.g, c.b, 90}, (Color){c.r, c.g, c.b, 0});
        float isz = r.width * 0.78f;
        DrawTexturePro(icons[sl.t].texture, (Rectangle){0, 0, ICON_PX, -ICON_PX}, (Rectangle){r.x + (r.width - isz) / 2, r.y + 1 * s, isz, isz}, (Vector2){0, 0}, 0, WHITE);
        DrawRectangle((int)(r.x + 4 * s), (int)(r.y + 3 * s), (int)(r.width - 8 * s), (int)(2 * s), aMul(c, 1.2f));
        char nm2[24]; snprintf(nm2, sizeof nm2, "%s", idef[sl.t].name);
        while (strlen(nm2) > 3 && aMeasure(nm2, 9.5f, false).x > r.width - 8 * s) nm2[strlen(nm2) - 1] = 0;
        aText(nm2, r.x + 4 * s, r.y + r.height - 14 * s, 9.5f, C_BONE, false);
        if (idef[sl.t].cat == CAT_MELEE && sl.w > 0) { float k = 1 - sl.w / 1000.0f; DrawRectangleRec((Rectangle){r.x + 5 * s, r.y + r.height - 3 * s, r.width - 10 * s, 2 * s}, (Color){20, 20, 20, 255});
            DrawRectangleRec((Rectangle){r.x + 5 * s, r.y + r.height - 3 * s, (r.width - 10 * s) * k, 2 * s}, k > 0.3f ? (Color){150, 190, 140, 255} : (Color){210, 90, 60, 255}); }
        if (idef[sl.t].stack > 1 || sl.n > 1) { char b[8]; snprintf(b, sizeof b, "%d", sl.n); Vector2 m = aMeasure(b, 12, true); aText(b, r.x + r.width - m.x - 5 * s, r.y + 4 * s, 12, C_CYAN, true); }
        return;
    }
    Rectangle chip = {r.x + 5 * s, r.y + 5 * s, r.width - 10 * s, 12 * s};
    aGrad(chip, 1.0f, aMul(c, 1.25f), aMul(c, 0.6f), 3);
    aRR((Rectangle){chip.x + 4 * s, chip.y + 1.5f * s, chip.width - 8 * s, 4 * s}, 2 * s, (Color){255, 255, 255, 50});
    char nm[40]; snprintf(nm, sizeof nm, "%s", idef[sl.t].name);
    // two short lines of name
    char l1[24] = "", l2[24] = ""; int L = (int)strlen(nm);
    if (aMeasure(nm, 11, false).x <= r.width - 8 * s) snprintf(l1, sizeof l1, "%s", nm);
    else { int cut = L / 2; for (int i = L / 2; i < L; i++) if (nm[i] == ' ') { cut = i; break; } snprintf(l1, sizeof l1, "%.*s", cut, nm); snprintf(l2, sizeof l2, "%s", nm + cut + (nm[cut] == ' ')); }
    aText(l1, r.x + 5 * s, r.y + 21 * s, 11, C_BONE, false);
    if (l2[0]) aText(l2, r.x + 5 * s, r.y + 34 * s, 11, C_BONE, false);
    if (idef[sl.t].cat == CAT_MELEE && sl.w > 0) { float k = 1 - sl.w / 1000.0f; Rectangle wb = {r.x + 5 * s, r.y + r.height - 7 * s, (r.width - 10 * s) * k, 3 * s};
        DrawRectangleRec((Rectangle){r.x + 5 * s, r.y + r.height - 7 * s, r.width - 10 * s, 3 * s}, (Color){20, 20, 20, 255}); DrawRectangleRec(wb, k > 0.3f ? (Color){150, 190, 140, 255} : (Color){210, 90, 60, 255}); }
    if (idef[sl.t].stack > 1 || sl.n > 1) { char b[8]; snprintf(b, sizeof b, "%d", sl.n); Vector2 m = aMeasure(b, 12, true); aText(b, r.x + r.width - m.x - 5 * s, r.y + r.height - 17 * s, 12, C_CYAN, true); }
}
// put a whole slot (keeps weapon wear) into hands or a free inventory slot
static bool invPutSlot(Slot *a) {
    if (idef[a->t].stack > 1) return false;
    if ((idef[a->t].cat == CAT_MELEE || idef[a->t].cat == CAT_GUN) && pl.hands.t < 0) { pl.hands = *a; quickAuto(a->t); a->t = -1; a->n = 0; return true; }
    for (int i = 0; i < invNSlots(); i++) if (pl.sl[i].t < 0) { pl.sl[i] = *a; quickAuto(a->t); a->t = -1; a->n = 0; return true; }
    return false;
}
// ---- slot addressing: region 0 hands, 1 back, 2 inventory index, 3 open container index, 4 worn clothes (WS_*)
static Slot *slotRef(int id) {
    int reg = id >> 8, i = id & 255;
    if (reg == 0) return &pl.hands;
    if (reg == 1) return &pl.back;
    if (reg == 4) return i < WS_N ? &pl.wear[i] : NULL;
    if (reg == 2) return i < invNSlots() ? &pl.sl[i] : NULL;
    if (reg == 3 && pl.openC >= 0) return i < (ckSlots[conts[pl.openC].kind] < CSLOTS ? ckSlots[conts[pl.openC].kind] : CSLOTS) ? &conts[pl.openC].s[i] : NULL;
    return NULL;
}
static bool slotAccepts(int id, int t) {
    int reg = id >> 8;
    if (t < 0) return true;
    if (reg == 0) return idef[t].cat == CAT_MELEE || idef[t].cat == CAT_GUN;
    if (reg == 1) return idef[t].cat == CAT_BAG;
    if (reg == 4) return idef[t].cat == CAT_CLOTHES && idef[t].wslot == (id & 255);
    return true;
}
static bool bagHasItems(void) { for (int i = POCK; i < invNSlots(); i++) if (pl.sl[i].t >= 0) return true; return false; }
static void moveSlot(int from, int to) {
    Slot *a = slotRef(from), *b = slotRef(to);
    if (!a || !b || a == b || a->t < 0) return;
    if ((from >> 8) == 1 && bagHasItems() && !(b->t >= 0 && idef[b->t].cat == CAT_BAG)) { say("Empty the bag before taking it off."); return; }
    if (!slotAccepts(to, a->t) || !slotAccepts(from, b->t)) { say(to >> 8 == 0 ? "Only weapons go in your hands." : to >> 8 == 1 ? "Only bags go on your back." : to >> 8 == 4 ? TextFormat("Only %s clothes go there.", wsName[to & 255]) : "That doesn't fit there."); return; }
    if (b->t == a->t && idef[a->t].stack > 1) { int room = idef[a->t].stack - b->n, k = a->n < room ? a->n : room; b->n += k; a->n -= k; if (a->n <= 0) { a->t = -1; a->n = 0; } }
    else { Slot tmp = *a; *a = *b; *b = tmp; }
    if ((to >> 8) == 0 || (from >> 8) == 0) { pl.swingT = 0; pl.reloadT = 0; }
    if ((to >> 8) == 2 || (to >> 8) == 0) quickAuto(pl.hands.t >= 0 ? pl.hands.t : (slotRef(to)->t >= 0 ? slotRef(to)->t : 0));
    play(sClick, 0.4f);
}
static void quickTransfer(int from) {   // right click: inventory <-> container, or use/equip
    Slot *a = slotRef(from); if (!a || a->t < 0) return;
    int reg = from >> 8;
    if (reg == 3 && idef[a->t].stack == 1 && idef[a->t].cat != CAT_BAG) { if (invPutSlot(a)) play(sPick, 0.4f); else say("No room."); return; }
    if (reg == 3) { int left = invAdd(a->t, a->n); if (left < a->n) play(sPick, 0.4f); if (left) say("No room."); a->n = (short)left; if (!a->n) a->t = -1; return; }
    if (reg == 2 && pl.openC >= 0) { Cont *c = &conts[pl.openC]; int ns = ckSlots[c->kind] < CSLOTS ? ckSlots[c->kind] : CSLOTS; int left = slotsAdd(c->s, ns, a->t, a->n); if (left) say("Container is full."); a->n = (short)left; if (!a->n) a->t = -1; play(sClick, 0.4f); return; }
    if (reg == 2) {
        int t = a->t, cat = idef[t].cat;
        if (cat == CAT_MELEE || cat == CAT_GUN) { equipFromSlot(from & 255); return; }
        if (cat == CAT_BAG) { if (pl.back.t < 0) { pl.back = *a; a->t = -1; a->n = 0; say(TextFormat("Wearing the %s.", idef[t].name)); } else moveSlot(from, 1 << 8); return; }
        if (cat == CAT_CLOTHES) { int ws = idef[t].wslot; moveSlot(from, (4 << 8) | ws); play(sCloth, 0.6f); if (pl.wear[ws].t == t) say(TextFormat("Wearing the %s.", idef[t].name)); return; }
        useItem(t); return;
    }
    if (reg == 4) { for (int i = 0; i < invNSlots(); i++) if (pl.sl[i].t < 0) { pl.sl[i] = *a; a->t = -1; a->n = 0; play(sCloth, 0.5f); return; } say("No room to take it off."); return; }
    if (reg == 0) holster();
}
static void dropSlot(int from) {
    Slot *a = slotRef(from); if (!a || a->t < 0) return;
    if ((from >> 8) == 1 && bagHasItems()) { say("Empty the bag before dropping it."); return; }
    if (idef[a->t].stack == 1 && a->w > 0) {   // keep the wear on a dropped weapon
        Vector3 dp = V3(pl.pos.x + sinf(pl.yaw) * 0.6f, pl.pos.y + 0.02f, pl.pos.z + cosf(pl.yaw) * 0.6f); int c = contAt(dp, 1.2f, CK_PILE); if (c < 0) c = addCont(CK_PILE, dp, 0, 0, false);
        if (c >= 0) for (int i = 0; i < CSLOTS; i++) if (conts[c].s[i].t < 0) { conts[c].s[i] = *a; break; }
    } else addItem(V3(pl.pos.x + sinf(pl.yaw) * 0.6f, pl.pos.y + 0.02f, pl.pos.z + cosf(pl.yaw) * 0.6f), a->t, a->n);
    for (int q = 0; q < 6; q++) if (pl.quick[q] == a->t && !invHas(a->t) && (from >> 8) != 3) {}
    a->t = -1; a->n = 0; play(sPick, 0.3f);
}
static int uiHot = -1;
// grid of n slots starting at slot-id base; returns hovered id
static void slotGrid(float x, float y, int cols, int n, int base, float cell) {
    float s = aU; Vector2 m = aMousePos();
    for (int i = 0; i < n; i++) {
        Rectangle r = {x + (i % cols) * (cell + 8 * s), y + (i / cols) * (cell + 8 * s), cell, cell};
        Slot *sl = slotRef(base + i); if (!sl) continue;
        bool hot = CheckCollisionPointRec(m, r);
        if (hot) uiHot = base + i;
        Slot shown = *sl;
        if (pl.dragFrom == base + i && pl.drag.t >= 0) shown.t = -1;
        itemCell(r, shown, hot, false);
        if ((base >> 8) == 2 && i == POCK && n > POCK) aLine(r.x - 4 * s, r.y - 4 * s, r.x - 4 * s, r.y + cell + 4 * s, 2, C_AMBER);
    }
}
// tooltip for the hovered slot, drawn inside the panel the mouse is over
static void slotTooltip(Rectangle panel) {
    if (uiHot < 0) return;
    Slot *sl = slotRef(uiHot); if (!sl || sl->t < 0) return;
    float s = aU; const ItemDef *d = &idef[sl->t];
    Vector2 m = aMousePos(); Rectangle tp = {m.x + 18 * s, m.y + 10 * s, 290 * s, 92 * s};
    if (tp.x + tp.width > panel.width) tp.x = m.x - tp.width - 10 * s; if (tp.y + tp.height > panel.height) tp.y = panel.height - tp.height - 4 * s;
    aGadget(tp);
    aText(d->name, tp.x + 12 * s, tp.y + 10 * s, 15, (Color){236, 230, 216, 255}, true);
    aText(TextFormat("%.2f kg each", d->weight), tp.x + 12 * s, tp.y + 30 * s, 11, C_BONE2, false);
    if (d->cat == CAT_MELEE) aText(TextFormat("Damage %.1f   Swing %.2fs   Reach %.1fm   Condition %d%%", d->dmg, d->cd, d->range, 100 - sl->w / 10), tp.x + 12 * s, tp.y + 46 * s, 11, C_CYAN, false);
    else if (d->cat == CAT_BAG) aText(TextFormat("+%d slots", d->bag), tp.x + 12 * s, tp.y + 46 * s, 11, C_CYAN, false);
    else if (d->cat == CAT_CLOTHES) aText(TextFormat("%s   Protection %d%%   Warmth +%d%s", wsName[d->wslot], (int)lroundf(d->armor * 100), (int)lroundf(d->warm), d->stam > 0 ? TextFormat("   Stamina -%d%%", (int)d->stam) : ""), tp.x + 12 * s, tp.y + 46 * s, 11, C_CYAN, false);
    aText(d->desc ? d->desc : "", tp.x + 12 * s, tp.y + 64 * s, 11, C_BONE, false);
}
static bool invOverPanel; static int invQHot;
static void dragGhost(float cell) {   // the item under the cursor, drawn in whichever panel the cursor is over
    Vector2 m = aMousePos(); if (m.x < -9000) return;
    invOverPanel = true;
    if (pl.dragFrom >= 0 && pl.drag.t >= 0) { Rectangle r = {m.x - cell / 2, m.y - cell / 2, cell, cell}; itemCell(r, pl.drag, true, false); }
}
// the inventory: four 3D glass panels on the sphere around you, like the menus
static void drawInventory(void) {
    float W = sw, H = shh;
    uiHot = -1; invOverPanel = false; invQHot = -1;
    DrawRectangle(0, 0, (int)W, (int)H, (Color){6, 8, 9, 150});
    { float asp = (float)GetScreenWidth() / GetScreenHeight(); p3Fov = fmaxf(45, 2 * atanf(0.98f / asp) * RAD2DEG); }   // the three panels fit at any aspect
    p3Begin((float)GetTime()); p3FitMargin = 8 * aU;   // every inventory slab stays fully on screen, whatever the window shape
    float s, cell;
    // ---- survivor: equipment, load, skills
    float pxDeg = 2 * tanf(22.5f * DEG2RAD) / 720.0f * RAD2DEG;   // degrees per reference px on the panel sphere
    float halfIn = (6 * 70 + 36) / 2.0f + 46;   // a clear gap: neighbouring slabs never cover each other's text, even as the view sways with the mouse   // side panels hug the pockets panel instead of drifting off-screen
    Rectangle ep = p3Panel(0, -(halfIn + 150) * pxDeg, 0, 300, 650, true); s = aU; cell = 62 * s;
    aWindow(ep, "Survivor");
    aText("Hands", ep.x + 22 * s, ep.y + 52 * s, 13, C_BONE2, false);
    aText("Back", ep.x + 160 * s, ep.y + 52 * s, 13, C_BONE2, false);
    slotGrid(ep.x + 22 * s, ep.y + 72 * s, 1, 1, 0 << 8, cell * 1.4f);
    slotGrid(ep.x + 160 * s, ep.y + 72 * s, 1, 1, 1 << 8, cell * 1.4f);
    float w = invWeight(), y = ep.y + 72 * s + cell * 1.4f + 14 * s;
    {   // worn clothes: head, body, legs, feet
        float wc = (ep.width - 44 * s - 3 * 8 * s) / 4;
        for (int k = 0; k < WS_N; k++) aText(wsName[k], ep.x + 22 * s + k * (wc + 8 * s), y, 11, C_BONE2, false);
        slotGrid(ep.x + 22 * s, y + 16 * s, 4, WS_N, 4 << 8, wc);
        y += 16 * s + wc + 8 * s;
        aText(TextFormat("Protection %d%%   Warmth +%d", (int)lroundf(wearArmor() * 100), (int)lroundf(wearWarm())), ep.x + 22 * s, y, 11, C_CYAN, false);
        y += 22 * s; }
    aText(TextFormat("Load  %.1f kg", w), ep.x + 22 * s, y, 15, C_BONE, true);
    aProgress((Rectangle){ep.x + 22 * s, y + 24 * s, ep.width - 44 * s, 10 * s}, w / 30, w > 18 ? (Color){190, 70, 50, 255} : w > 8 ? (Color){200, 150, 60, 255} : (Color){80, 160, 150, 255});
    aText(TextFormat("Speed %d%%   Swing %d%%   Stamina %d%%", (int)(encSpeed() * 100), (int)(100 / encSwing()), (int)(encStam() * 100)), ep.x + 22 * s, y + 42 * s, 11, C_BONE2, false);
    y += 74 * s;
    aText("Skills", ep.x + 22 * s, y, 15, C_CYAN, true); y += 26 * s;
    for (int k = 0; k < SK_N; k++) {
        int lv = skLvl(k); float a0 = skNeed[lv], a1 = lv < 5 ? skNeed[lv + 1] : skNeed[5];
        aText(TextFormat("%s  %d", skName[k], lv), ep.x + 22 * s, y, 13, C_BONE, false);
        aProgress((Rectangle){ep.x + 150 * s, y + 4 * s, ep.width - 172 * s, 8 * s}, lv >= 5 ? 1 : (pl.sk[k] - a0) / (a1 - a0), (Color){110, 150, 120, 255});
        aText(skDesc[k], ep.x + 22 * s, y + 17 * s, 10, (Color){170, 166, 156, 240}, false);
        y += 38 * s;
    }
    slotTooltip(ep); dragGhost(cell);
    p3End(0);
    // ---- pockets + bag
    int ns = invNSlots(), cols = 6;
    float iwRef = cols * 70 + 36;
    Rectangle ip = p3Panel(1, 0, 4, iwRef, 470, true); s = aU; cell = 62 * s;
    aWindow(ip, pl.back.t >= 0 ? TextFormat("Pockets + %s", idef[pl.back.t].name) : "Pockets");
    slotGrid(ip.x + 18 * s, ip.y + 56 * s, cols, ns, 2 << 8, cell);
    if (pl.back.t < 0) aText("Find a bag (school bag, duffel, hiking pack) for more space.", ip.x + 18 * s, ip.y + 56 * s + cell + 20 * s, 12, C_BONE2, false);
    if (pl.wounds > 0) aText(TextFormat("Bleeding from %d wound%s: right-click a bandage or rags to hold it, then click.", pl.wounds, pl.wounds > 1 ? "s" : ""), ip.x + 18 * s, ip.y + ip.height - 70 * s, 13, (Color){230, 90, 70, 255}, true);
    aText("Drag to move   Right-click: hold / equip / take   Drop outside the panels to leave it", ip.x + 18 * s, ip.y + ip.height - 42 * s, 11, (Color){170, 166, 156, 240}, false);
    aText("Hover an item and press 1-6 to put it on the quickbar.   Tab closes.", ip.x + 18 * s, ip.y + ip.height - 24 * s, 11, (Color){170, 166, 156, 240}, false);
    slotTooltip(ip); dragGhost(cell);
    p3End(1);
    // ---- container, or the crafting bench when nothing is open
    if (pl.openC >= 0) {
        Cont *c = &conts[pl.openC]; int cn = ckSlots[c->kind] < CSLOTS ? ckSlots[c->kind] : CSLOTS;
        Rectangle cp = p3Panel(2, (halfIn + (4 * 70 + 36) / 2.0f) * pxDeg, 1, 4 * 70 + 36, 470, true); s = aU; cell = 62 * s;
        if (aWindow(cp, ckName[c->kind])) pl.openC = -1;
        else {
            slotGrid(cp.x + 18 * s, cp.y + 56 * s, 4, cn, 3 << 8, cell);
            if (aButton((Rectangle){cp.x + 18 * s, cp.y + cp.height - 56 * s, cp.width - 36 * s, 36 * s}, "Take All", true)) {
                for (int i = 0; i < cn; i++) if (c->s[i].t >= 0 && idef[c->s[i].t].stack == 1 && idef[c->s[i].t].cat != CAT_BAG) invPutSlot(&c->s[i]);
                for (int i = 0; i < cn; i++) if (c->s[i].t >= 0) { int left = invAdd(c->s[i].t, c->s[i].n); c->s[i].n = (short)left; if (!left) c->s[i].t = -1; }
                play(sPick, 0.5f); animPlay(ANIM_TAKE);
            }
        }
        slotTooltip(cp); dragGhost(cell);
        p3End(2);
    } else {
        Rectangle cp = p3Panel(2, (halfIn + 175) * pxDeg, 0, 350, 560, true); s = aU; cell = 62 * s;
        drawCrafting(cp); dragGhost(cell);
        p3End(2);
    }
    // ---- quickbar
    {
        Rectangle qp = p3Panel(3, 0, -27, 6 * 70 + 40, 112, true); s = aU; cell = 62 * s;
        aWindow(qp, "Quickbar  (drag in, reorder, drag out to clear)");
        float qx = qp.x + 20 * s, qy = qp.y + 42 * s;
        for (int q = 0; q < 6; q++) {
            Rectangle r = {qx + q * (cell + 8 * s), qy, cell, cell};
            bool hot = CheckCollisionPointRec(aMousePos(), r);
            if (hot) invQHot = q;
            Slot sl = {(short)pl.quick[q], (short)(pl.quick[q] >= 0 ? invCount(pl.quick[q]) : 0), 0};
            if (pl.dragFrom == ((4 << 8) | q)) sl.t = -1;
            itemCell(r, sl, hot, pl.quick[q] >= 0 && pl.quick[q] == pl.hands.t);
            aText(TextFormat("%d", q + 1), r.x + 5 * s, r.y + 4 * s, 11, C_AMBER, true);   // top-left: the item name runs along the bottom
        }
        dragGhost(cell);
        p3End(3);
    }
    // ---- hotkeys and drag and drop (slot ids are panel independent)
    if (uiHot >= 0) { Slot *sl = slotRef(uiHot); for (int q = 0; q < 6; q++) if (actP(A_Q1 + q) && sl && sl->t >= 0) { pl.quick[q] = sl->t; play(sClick, 0.4f); } }
    if (aMPressed() && uiHot >= 0) { Slot *sl = slotRef(uiHot); if (sl && sl->t >= 0) { pl.dragFrom = uiHot; pl.drag = *sl; } }
    else if (aMPressed() && invQHot >= 0 && pl.quick[invQHot] >= 0) { pl.dragFrom = (4 << 8) | invQHot; pl.drag.t = (short)pl.quick[invQHot]; pl.drag.n = (short)invCount(pl.quick[invQHot]); }
    if ((IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || padP(GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) && uiHot >= 0) quickTransfer(uiHot);
    if (pl.dragFrom >= 0 && !aMDown()) {
        if ((pl.dragFrom >> 8) == 4) {   // quickbar slot: reorder, or clear when dropped elsewhere
            int q0 = pl.dragFrom & 255;
            if (invQHot >= 0) { int tmp = pl.quick[invQHot]; pl.quick[invQHot] = pl.quick[q0]; pl.quick[q0] = tmp; } else pl.quick[q0] = -1;
            play(sClick, 0.4f);
        }
        else if (invQHot >= 0) { for (int q = 0; q < 6; q++) if (pl.quick[q] == pl.drag.t) pl.quick[q] = -1; pl.quick[invQHot] = pl.drag.t; play(sClick, 0.4f); }
        else if (uiHot >= 0 && uiHot != pl.dragFrom) moveSlot(pl.dragFrom, uiHot);
        else if (!invOverPanel) dropSlot(pl.dragFrom);
        pl.dragFrom = -1; pl.drag.t = -1;
    }
    p3Render(pl.stage); p3Fov = 45; p3FitMargin = 0;
    if (!invOverPanel && pl.dragFrom >= 0 && pl.drag.t >= 0) { Vector2 m = GetMousePosition(); float c2 = 62 * (shh / 720.0f); itemCell((Rectangle){m.x - c2 / 2, m.y - c2 / 2, c2, c2}, pl.drag, true, false); }
}
// ---- the bleeding symbol: a blood drop, one ring per open wound
static void bleedIcon(float x, float y, float sz, int wounds, float t) {
    float s = aU, p = 0.6f + 0.4f * sinf(t * 6);
    Color c = (Color){(unsigned char)(150 + 90 * p), 20, 18, 255};
    DrawCircle((int)x, (int)(y + sz * 0.25f), sz * 0.5f, (Color){8, 4, 4, 255});
    DrawTriangle((Vector2){x, y - sz * 0.75f}, (Vector2){x - sz * 0.47f, y + sz * 0.15f}, (Vector2){x + sz * 0.47f, y + sz * 0.15f}, (Color){8, 4, 4, 255});
    DrawCircle((int)x, (int)(y + sz * 0.25f), sz * 0.44f, c);
    DrawTriangle((Vector2){x, y - sz * 0.62f}, (Vector2){x - sz * 0.41f, y + sz * 0.15f}, (Vector2){x + sz * 0.41f, y + sz * 0.15f}, c);
    DrawEllipse((int)(x - sz * 0.15f), (int)(y + sz * 0.05f), sz * 0.1f, sz * 0.16f, (Color){255, 200, 190, 160});
    if (wounds > 1) aText(TextFormat("x%d", wounds), x + sz * 0.55f, y - sz * 0.1f, 13, (Color){240, 120, 100, 255}, true);
    (void)s;
}
static void drawHUD(Vector2 cross, float t) {
    float W = sw, H = shh, s = aU;
    aCorrupt = pl.stage;
    drawEyelids(W, H);
    if (pl.sleeping && pl.eyeK > 0.9f) { char b2[64]; snprintf(b2, sizeof b2, "Sleeping...  rest %d%%   [Z] wake", (int)pl.rest); keySubst(b2, sizeof b2); aTextC(b2, W / 2, H * 0.47f, 22, (Color){200, 196, 186, (unsigned char)(255 * (pl.eyeK - 0.9f) * 10)}, false); }
    if (pl.watchT > 0 && pl.watchKind == 2 && mangaStep < 4 && MANGA_T - pl.watchT > 1.2f) { char b2[128]; snprintf(b2, sizeof b2, "[%s] %s     [%s] Put it down", actKey(A_ATTACK), mangaStep < 3 ? "Turn the page" : "Close it", actKey(A_GUARD));
        aTextC(b2, W / 2, H * 0.8f, 16, (Color){226, 220, 206, 230}, false); }
    if (pl.watchT > 0 && pl.watchKind != 2) { const char *w = pl.watchKind == 1 ? "Watching anime..." : "Watching the broadcast...";
        DrawRectangle(0, 0, (int)W, (int)H, (Color){0, 0, 0, 90}); aTextC(w, W / 2, H * 0.12f, 18, C_BONE, false); }
    // ---- blood loss shows on screen, not as a number: grey edges that close in
    if (pl.hp < 75) { float k = (75 - pl.hp) / 75.0f;
        for (int b = 0; b < 5; b++) { unsigned char al = (unsigned char)(k * 120 * (5 - b) / 5); int bw = (int)(W * 0.035f), bh = (int)(H * 0.035f);
            DrawRectangle(0, b * bh, (int)W, bh, (Color){10, 6, 6, al}); DrawRectangle(0, (int)H - (b + 1) * bh, (int)W, bh, (Color){10, 6, 6, al});
            DrawRectangle(b * bw, 0, bw, (int)H, (Color){10, 6, 6, al}); DrawRectangle((int)W - (b + 1) * bw, 0, bw, (int)H, (Color){10, 6, 6, al}); } }
    // ---- vitals gadget: fades to 25% when nothing changes
    float cur[6] = {pl.stam, pl.food, pl.water, pl.warmth, pl.rest, pl.sanity};
    bool changed = pl.hurtT > 0 || pl.wounds > 0 || pl.food < 20 || pl.water < 20 || pl.warmth < 20 || pl.stage >= 3 || pl.hp < 50;
    for (int i = 0; i < 6; i++) if (fabsf(cur[i] - hudPrev[i]) > 2.5f && i != 0) { changed = true; hudPrev[i] = cur[i]; }
    if (pl.stam < 60) changed = true;
    if (changed) hudCalmT = 0; else hudCalmT += GetFrameTime();
    float fade = hudCalmT < 6 ? 1.0f : fmaxf(0.25f, 1.0f - (hudCalmT - 6) * 1.5f); (void)fade;
    float wk = wildK(pl.pos);
    // ==== 3D HUD: every gadget is a glass slab on the panel sphere, turned to face you; the set swings with your head
    float Wr = 720.0f * W / H;   // screen width in 720p reference px
    bool hud3 = !pl.invOpen && !pl.mapOpen && !cht.menu;   // the inventory, the map and the cheat menu take the whole view
    p3HudScale = 0.8f * gs.hudScale;   // the HUD size setting
    p3Hud = true; p3Fov = 54; p3HudLook = (Vector2){-pl.swayX * 0.5f, pl.swayY * 0.5f - pl.landK * 0.04f};
    float f = p3HudScale * tanf(22.5f * DEG2RAD) / tanf(p3Fov / 2 * DEG2RAD), mg = 30;   // slab size on screen per reference px; edge margin
    if (hud3) {
    p3Begin(t);
    #define HUDP(ID, X, Y, WW, HH) ({ Vector2 a_ = p3At((X), (Y)); Rectangle r_ = p3Panel((ID), a_.x, a_.y, (WW), (HH), 2); s = aU; r_; })
    {   // vitals vials + sanity orb
        Rectangle vp = HUDP(8, -Wr / 2 + mg + 214 * f, 360 - mg - 104 * f, 428, 208);
        aGadget(vp);
        struct { float v; Color c; const char *l; } vi[5] = {
            {pl.stam, pl.exhausted ? (Color){90, 92, 90, 255} : (Color){90, 150, 120, 255}, "STAMINA"}, {pl.food, (Color){200, 140, 60, 255}, "FOOD"}, {pl.water, (Color){60, 120, 170, 255}, "WATER"},
            {pl.warmth, pl.wet > 40 ? (Color){90, 140, 170, 255} : (Color){190, 90, 44, 255}, pl.wet > 40 ? "SOAKED" : "WARMTH"}, {pl.rest, (Color){120, 110, 170, 255}, "REST"}};
        for (int k = 0; k < 5; k++) {
            float x = vp.x + 26 * s + k * 46 * s;
            aVial((Rectangle){x, vp.y + 18 * s, 22 * s, 138 * s}, vi[k].v / 100, vi[k].c);
            Vector2 m = aMeasure(vi[k].l, 10, false);
            aText(vi[k].l, x + 11 * s - m.x / 2, vp.y + 166 * s, 10, vi[k].v < 25 ? (Color){220, 90, 70, 255} : C_BONE2, false);
        }
        float ox = vp.x + 336 * s, oy = vp.y + 88 * s;
        aOrb(ox, oy, 44 * s, pl.sanity, pl.stage, t);
        const char *st = pl.stage == 0 ? (pl.sanity > 60 ? "Clear" : pl.sanity > 35 ? "Overcast" : "Rain") : pl.stage == 1 ? "Sadness" : pl.stage == 2 ? "Depression" : "Insanity";
        Vector2 m = aMeasure(st, 13, true);
        aText(st, ox - m.x / 2, vp.y + 162 * s, 13, pl.stage >= 3 ? (Color){220, 80, 60, 255} : pl.stage ? C_TALLOW : C_BONE2, true);
        if (pl.boredom > 70) aText("Bored", ox - 18 * s, vp.y + 182 * s, 11, C_BONE2, false);
        p3End(8); p3Fit(8, 12 * s);
    }
    {   // quickbar
        float cell = 46 * s;
        Rectangle qp = HUDP(11, 0, 360 - mg * 0.6f - 34 * f, 6 * 52 + 18, 68); cell = 46 * s;
        aGadget(qp);
        for (int q = 0; q < 6; q++) {
            Rectangle r = {qp.x + 12 * s + q * 52 * s, qp.y + 11 * s, cell, cell};
            int it = pl.quick[q]; Slot sl = {(short)it, (short)(it >= 0 ? invCount(it) : 0)};
            if (it >= 0 && sl.n == 0) sl.t = -1;
            itemCell(r, sl, false, it >= 0 && it == pl.hands.t);
            aText(TextFormat("%d", q + 1), r.x + 5 * s, r.y + 4 * s, 10, C_AMBER, true);
        }
        p3End(11); p3Fit(11, 12 * s);
    }
    {   // held weapon
        Rectangle wp = HUDP(9, Wr / 2 - mg - 143 * f, 360 - mg - 52 * f, 286, 104);
        aGadget(wp);
        aText(weaponName(), wp.x + 18 * s, wp.y + 14 * s, 20, (Color){236, 230, 216, 255}, true);
        if (pl.hands.t == IT_REVOLVER) aText(TextFormat("%d / %d rounds%s", pl.mag, invCount(IT_AMMO), pl.reloadT > 0 ? "  reloading" : ""), wp.x + 18 * s, wp.y + 44 * s, 14, C_CYAN, false);
        else if (pl.hands.t == IT_PISTOL || pl.hands.t == IT_SHOTGUN) aText(TextFormat("%d / %d %s%s", *gunMag(), invCount(idef[pl.hands.t].ammo), pl.hands.t == IT_SHOTGUN ? "shells" : "rounds", pl.reloadT > 0 ? "  reloading" : ""), wp.x + 18 * s, wp.y + 44 * s, 14, C_CYAN, false);
        else if (pl.hands.t >= 0 && idef[pl.hands.t].cat == CAT_THROW) aText(TextFormat("%d in hand, %d more   %s throws", pl.hands.n, invCount(pl.hands.t) - pl.hands.n, actKey(A_ATTACK)), wp.x + 18 * s, wp.y + 44 * s, 12, C_BONE2, false);
        else { const ItemDef *d = weaponDef(); aText(TextFormat("dmg %.1f  swing %.2fs  load %.1f kg", d->dmg, meleeCd(), invWeight()), wp.x + 18 * s, wp.y + 44 * s, 12, C_BONE2, false);
            if (pl.hands.t >= 0 && idef[pl.hands.t].cat == CAT_MELEE) { float k = 1 - pl.hands.w / 1000.0f; aText("Cond.", wp.x + 190 * s, wp.y + 18 * s, 10, C_BONE2, false);
                aProgress((Rectangle){wp.x + 222 * s, wp.y + 20 * s, 44 * s, 8 * s}, k, k > 0.3f ? (Color){150, 190, 140, 255} : (Color){210, 90, 60, 255}); } }
        aText(pl.flash ? "Light" : "Light off", wp.x + 18 * s, wp.y + 70 * s, 11, C_BONE2, false);
        aProgress((Rectangle){wp.x + 80 * s, wp.y + 72 * s, 90 * s, 9 * s}, pl.batt / 100, pl.flash ? (Color){200, 180, 90, 255} : (Color){100, 96, 80, 255});
        aText(TextFormat("%s  inventory", actKey(A_INV)), wp.x + 184 * s, wp.y + 70 * s, 11, (Color){150, 146, 136, 230}, false);
        p3End(9); p3Fit(9, 12 * s);
    }
    if (pl.build) {   // build list
        float bh = BK_N * 26 + 92;
        Rectangle bp = HUDP(10, Wr / 2 - mg - 143 * f, 360 - mg - (104 + 14 + bh / 2) * f, 286, bh);
        aGadget(bp);
        aText(TextFormat("Build  (%s next piece, wheel turns %.0f deg, %s place, %s exit)", actKey(A_RELOAD), bkRot, actKey(A_ATTACK), actKey(A_BUILD)), bp.x + 16 * s, bp.y + 14 * s, 11, C_CYAN, false);
        for (int k = 0; k < BK_N; k++) {
            float yy = bp.y + 36 * s + k * 26 * s;
            bool sel = k == pl.buildKind, ok = bkAfford(k) && (!bkHammer[k] || invHas(IT_HAMMER));
            if (sel) aRR((Rectangle){bp.x + 10 * s, yy - 3 * s, bp.width - 20 * s, 24 * s}, 6 * s, (Color){47, 93, 94, 200});
            aText(bkName[k], bp.x + 18 * s, yy, 13, ok ? (Color){236, 230, 216, 255} : (Color){140, 136, 128, 255}, sel);
            if (sel) aText(bkCostText(k), bp.x + 16 * s, bp.y + bp.height - 48 * s, 12, ok ? C_CYAN : (Color){220, 120, 90, 255}, false);
        }
        if (baseShrine >= 0 && builds[baseShrine].alive) {
            Vector3 sc = builds[baseShrine].mn; float d = Vector3Distance(V3(sc.x, 0, sc.z), V3(pl.pos.x, 0, pl.pos.z));
            aText(d < BASE_R ? "Inside your base: wards allowed" : TextFormat("Base %.0f m away", d), bp.x + 16 * s, bp.y + bp.height - 28 * s, 11, d < BASE_R ? C_CYAN : C_BONE2, false);
        }
        p3End(10); p3Fit(10, 12 * s);
    }
    {   // compass
        Rectangle cp = HUDP(12, 0, -360 + mg * 0.5f + 62 * f, 124, 124);
        aCompass(cp.x + cp.width / 2, cp.y + cp.height / 2, 48 * s, pl.yaw, pl.stage, t);
        p3End(12); p3Fit(12, 12 * s);
    }
    {   // who and where
        const char *where = pl.pos.y < -40 ? "Subway" : inBore(pl.pos.x, pl.pos.z, pl.pos.y) ? (bores[inBore(pl.pos.x, pl.pos.z, pl.pos.y) - 1].kind == 1 ? "Old tunnel" : bores[inBore(pl.pos.x, pl.pos.z, pl.pos.y) - 1].kind == 2 ? "Railway tunnel" : "Road tunnel") : touge_d(pl.pos.x, pl.pos.z) < 40 ? "The mountain pass" : coast_d(pl.pos.x, pl.pos.z) < 30 && !cityLand(pl.pos.x, pl.pos.z) ? "Coast road" : wk > 0.3f ? "The hills" : "Nanashi no Gonbei";
        Rectangle np = HUDP(13, -Wr / 2 + mg + 130 * f, -360 + mg * 0.7f + 30 * f, 260, 60);
        aGadget(np);
        aText(prof.name, np.x + 16 * s, np.y + 11 * s, 15, (Color){236, 230, 216, 255}, false);
        aText(where, np.x + 16 * s, np.y + 32 * s, 12, C_BONE2, false);
        p3End(13); p3Fit(13, 12 * s);
        const char *ck = TextFormat("Day %d   %02d:%02d   Kills %d", pl.day, (int)pl.tod, (int)((pl.tod - (int)pl.tod) * 60), pl.kills);
        Rectangle tp = HUDP(14, Wr / 2 - mg - 125 * f, -360 + mg * 0.7f + 22 * f, 250, 44);
        aGadget(tp);
        Vector2 m = aMeasure(ck, 15, false);
        aText(ck, tp.x + (tp.width - m.x) / 2, tp.y + (tp.height - m.y) / 2, 15, dayK() < 0.2f ? (Color){150, 170, 210, 255} : (Color){236, 230, 216, 255}, false);
        p3End(14); p3Fit(14, 12 * s);
    }
    if (prompt[0]) {   // what E does here
        keySubst(prompt, sizeof prompt);
        float mw = aMeasure(prompt, 17, false).x / aU;
        Rectangle pr = HUDP(15, 0, H * 0.68f / H * 720 - 360 + 26, mw + 48, pl.searchC >= 0 ? 66 : 50);
        aGadget(pr);
        aTextC(prompt, pr.x + pr.width / 2, pr.y + 15 * s, 17, (Color){236, 230, 216, 255}, false);
        if (pl.searchC >= 0) aProgress((Rectangle){pr.x + 14 * s, pr.y + 44 * s, pr.width - 28 * s, 8 * s}, pl.searchT / fmaxf(0.01f, searchTime(conts[pl.searchC].kind)), (Color){80, 160, 150, 255});
        p3End(15); p3Fit(15, 12 * s);
    }
    #undef HUDP
    p3Render(pl.stage);
    }
    p3Hud = false; p3Fov = 45; s = aU;
    // ==== flat overlays: status words, warnings, hints, messages
    aLayerBegin();
    float sx = (mg + 6) * s, sy = H - (mg + 208 * f + 22) * s;
    if (pl.wounds > 0) { bleedIcon(sx + 14 * s, sy + 6 * s, 26 * s, pl.wounds, t); aText("Bleeding", sx + 46 * s, sy - 2 * s, 14, (Color){230, 100, 80, 255}, true); sx += 150 * s; }
    if (pl.hp < 50) { aText(pl.hp < 25 ? "Fading" : "Weak", sx, sy - 2 * s, 14, (Color){200, 170, 160, (unsigned char)(160 + 90 * sinf(t * 3))}, true); sx += 80 * s; }
    if (pl.painT > 0) { aText("Painkillers", sx, sy - 2 * s, 12, C_BONE2, false); sx += 90 * s; }
    if (pl.sick > 0) { aText(pl.sick > 30 ? "Sick" : "Queasy", sx, sy - 2 * s, 14, (Color){170, 200, 90, 255}, true); sx += 64 * s; }
    if (pl.infect > 0) { aText(pl.infect >= 35 ? "Fever" : "Infected", sx, sy - 2 * s, 14, (Color){230, 150, 60, 255}, true); sx += 80 * s; }
    if (pl.fracture) { aText(pl.fracture == 1 ? "Broken leg" : "Splinted", sx, sy - 2 * s, 14, pl.fracture == 1 ? (Color){230, 100, 80, 255} : C_BONE2, true); sx += 100 * s; }
    if (pl.roof || pl.nearFire) aText(pl.nearFire ? "Fire" : "Sheltered", sx, sy - 2 * s, 12, C_AMBER, false);
    if (wk > 0.15f) aTextC("This place should not exist.", W / 2, H * 0.17f, 15, (Color){230, 240, 255, (unsigned char)(255 * clampf(wk * 1.5f, 0, 1))}, false);
    drawHint(W, H);
    if (msgT > 0 && !pl.invOpen && !pl.mapOpen) aTextC(msg, W / 2, H * 0.78f, 16, (Color){226, 220, 206, (unsigned char)(255 * clampf(msgT, 0, 1))}, false);
    aLayerEnd(1);
    // ---- crosshair (GoldenEye-style: moves, view lags behind): the only signal-cyan in view
    float cs = 9 * s, gp = 4 * s + (pl.swingT > 0 ? 3 * s : 0);
    Color cc = crossCols[gs.crossCol < 0 || gs.crossCol > 4 ? 0 : gs.crossCol];
    for (int i = 0; i < nCr; i++) if (crs[i].on && crs[i].state != CS_DEAD) {
        Vector3 d = Vector3Subtract(crCenter(&crs[i]), cam.position); float dist = Vector3Length(d);
        if (dist < 30 && Vector3DotProduct(Vector3Scale(d, 1 / dist), dirOf(pl.tyaw, pl.tpitch)) > 0.995f && lineOfSight(cam.position, crCenter(&crs[i]))) { cc = (Color){200, 50, 40, 240}; break; }   // only what you can actually see
    }
    if (pl.hitStopT > 0) { cc = (Color){255, 240, 230, 255}; gp *= 0.5f; }
    int px = (int)fmaxf(2, roundf(2 * s));
    if (gs.crossStyle == 0 || gs.crossStyle == 2) {
    DrawRectangle((int)(cross.x - cs - gp), (int)(cross.y - px / 2), (int)cs, px, cc); DrawRectangle((int)(cross.x + gp), (int)(cross.y - px / 2), (int)cs, px, cc);
    DrawRectangle((int)(cross.x - px / 2), (int)(cross.y - cs - gp), px, (int)cs, cc); DrawRectangle((int)(cross.x - px / 2), (int)(cross.y + gp), px, (int)cs, cc); }
    if (gs.crossStyle == 1 || gs.crossStyle == 2) { int d = px + (int)fmaxf(1, s); DrawRectangle((int)(cross.x - d / 2), (int)(cross.y - d / 2), d, d, cc); }
    if (hitMarkT > 0 && gs.hitMarks) {   // hit marker: four ticks that pop out and fade; white body, yellow head, red kill
        float life = hitMarkKind == 2 ? 0.42f : 0.28f, k = clampf(hitMarkT / life, 0, 1), pop = 1 + 0.6f * powf(k, 6);
        Color hc = hitMarkKind == 2 ? (Color){235, 50, 40, 255} : hitMarkKind == 1 ? (Color){255, 214, 90, 255} : (Color){245, 245, 240, 255}; hc.a = (unsigned char)(255 * fminf(1, k * 1.6f));
        float r0 = (6 + (hitMarkKind == 2 ? 2 : 0)) * s * pop, r1 = r0 + (hitMarkKind == 2 ? 10 : 7) * s, th = (hitMarkKind == 2 ? 3.0f : 2.2f) * s;
        for (int q = 0; q < 4; q++) { float ax = (q & 1) ? 1.0f : -1.0f, ay = (q & 2) ? 1.0f : -1.0f; const float d = 0.7071f;
            DrawLineEx((Vector2){cross.x + ax * d * r0, cross.y + ay * d * r0}, (Vector2){cross.x + ax * d * r1, cross.y + ay * d * r1}, th, hc); }
    }
    // ---- impact flash: one bright frame, then a red after-image
    if (pl.hitFlash > 0 && !(gs.reduceFlash && pl.hitFlash > 0.06f)) { float k = pl.hitFlash; DrawRectangle(0, 0, (int)W, (int)H, k > 0.06f ? (Color){255, 245, 235, 70} : (Color){150, 20, 10, (unsigned char)(90 * k / 0.06f)}); }
    // ---- damage flash
    if (pl.hurtT > 0) DrawRectangle(0, 0, (int)W, (int)H, (Color){120, 10, 8, (unsigned char)(130 * clampf(pl.hurtT / 0.6f, 0, 1) * gs.dmgOverlay)});
    if (pl.invOpen) drawInventory();
    if (pl.mapOpen) drawMap(W, H);
    if (msgT > 0 && (pl.invOpen || pl.mapOpen)) {   // over the inventory or the map: on a dark strip on top of everything, so no panel cuts it off
        float al = clampf(msgT, 0, 1), fs = 16 * s, tw = aMeasure(msg, 16, false).x + 28 * s, ty = H - 26 * s;
        DrawRectangleRounded((Rectangle){W / 2 - tw / 2, ty - fs * 0.35f, tw, fs * 1.7f}, 0.4f, 6, (Color){12, 12, 14, (unsigned char)(220 * al)});
        aTextC(msg, W / 2, ty, 16, (Color){226, 220, 206, (unsigned char)(255 * al)}, false); }
}
