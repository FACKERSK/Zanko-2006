// zk_inv.h — player inventory (hands, back, pockets, bag), quickbar, weight, skills, wounds, item use, searching
enum { SK_STR, SK_FIT, SK_CARP, SK_SCAV, SK_STEALTH, SK_MED, SK_N };
static const char *skName[SK_N] = {"Strength", "Fitness", "Carpentry", "Scavenging", "Stealth", "First Aid"};
static const char *skDesc[SK_N] = {"+7% melee damage per level", "-7% stamina use per level", "+12% structure strength per level",
                                   "-12% search time per level, more loot", "-8% detection range per level", "Bandages and rags work better"};
static const float skNeed[6] = {0, 12, 40, 95, 180, 320};
static int skLvl(int s) { int l = 0; for (int i = 1; i < 6; i++) if (pl.sk[s] >= skNeed[i]) l = i; return l; }
static void skGain(int s, float xp) {
    int before = skLvl(s); pl.sk[s] += xp; int after = skLvl(s);
    if (after > before) { char b[64]; snprintf(b, sizeof b, "%s reached level %d.", skName[s], after); say(b); play(sPick, 0.7f); }
}
#define POCK 6
static int invNSlots(void) { return POCK + (pl.back.t >= 0 ? idef[pl.back.t].bag : 0); }
static int invCount(int t) { int n = 0; for (int i = 0; i < invNSlots(); i++) if (pl.sl[i].t == t) n += pl.sl[i].n; if (pl.hands.t == t) n += pl.hands.n; return n; }
static bool invHas(int t) { return invCount(t) > 0; }
static int invTake(int t, int n) {   // returns how many were taken
    int got = 0;
    for (int i = invNSlots() - 1; i >= 0 && got < n; i--) if (pl.sl[i].t == t) {
        int k = pl.sl[i].n < n - got ? pl.sl[i].n : n - got; pl.sl[i].n -= k; got += k; if (pl.sl[i].n <= 0) { pl.sl[i].t = -1; pl.sl[i].n = 0; } }
    if (got < n && pl.hands.t == t) { int k = pl.hands.n < n - got ? pl.hands.n : n - got; pl.hands.n -= k; got += k; if (pl.hands.n <= 0) pl.hands.t = -1; }
    return got;
}
static void quickAuto(int t) {   // weapons and medicine get a quickbar key automatically
    int c = idef[t].cat; if (!(c == CAT_MELEE || c == CAT_GUN || c == CAT_THROW || c == CAT_MED || c == CAT_FOOD || c == CAT_DRINK)) return;
    for (int i = 0; i < 6; i++) if (pl.quick[i] == t) return;
    for (int i = 0; i < 6; i++) if (pl.quick[i] < 0) { pl.quick[i] = t; return; }
}
static int invAdd(int t, int n) {   // returns leftover that did not fit
    if (idef[t].cat == CAT_BAG && pl.back.t < 0 && n > 0) { pl.back.t = (short)t; pl.back.n = 1; n--; say(TextFormat("Wearing the %s.", idef[t].name)); }
    if ((idef[t].cat == CAT_MELEE || idef[t].cat == CAT_GUN) && pl.hands.t < 0 && n > 0) { pl.hands.t = (short)t; pl.hands.n = 1; pl.hands.w = 0; n--; }
    if (idef[t].cat == CAT_CLOTHES && n > 0) { int ws = idef[t].wslot; if (ws >= 0 && ws < WS_N && pl.wear[ws].t < 0) { pl.wear[ws].t = (short)t; pl.wear[ws].n = 1; pl.wear[ws].w = 0; n--; say(TextFormat("Wearing the %s.", idef[t].name)); } }
    int left = slotsAdd(pl.sl, invNSlots(), t, n);
    if (left < n || pl.hands.t == t) quickAuto(t);
    return left;
}
static float slotW(Slot s) { return s.t >= 0 ? idef[s.t].weight * s.n : 0; }
static float invWeight(void) { float w = slotW(pl.hands) + slotW(pl.back); for (int i = 0; i < WS_N; i++) w += slotW(pl.wear[i]) * 0.5f; for (int i = 0; i < invNSlots(); i++) w += slotW(pl.sl[i]); return w; }
// encumbrance: up to 8 kg is free; heavier loads slow walking, swings and recovery
static float encSpeed(void) { float w = invWeight(); return clampf(1.0f - fmaxf(0, w - 8) / 38.0f, 0.55f, 1.0f); }
static float encSwing(void) { float w = invWeight(); return 1.0f + fmaxf(0, w - 10) / 30.0f; }
// clothes: worn weight counts half; armour and warmth add up across what you wear
static float wearArmor(void) { float a = 0; for (int i = 0; i < WS_N; i++) if (pl.wear[i].t >= 0) a += idef[pl.wear[i].t].armor; return fminf(a, 0.55f); }
static float wearWarm(void) { float a = 0; for (int i = 0; i < WS_N; i++) if (pl.wear[i].t >= 0) a += idef[pl.wear[i].t].warm; return a; }
static float wearStam(void) { float a = 0; for (int i = 0; i < WS_N; i++) if (pl.wear[i].t >= 0) a += idef[pl.wear[i].t].stam; return a / 100.0f; }
static float encStam(void) { float w = invWeight(); return (1.0f + fmaxf(0, w - 10) / 22.0f) * (1 + wearStam()); }
static int handsType(void) { return pl.hands.t; }
static const ItemDef *weaponDef(void) {   // bare fists when nothing melee is held
    static const ItemDef fists = {"Fists", 0, 1, CAT_MELEE, 0.45f, 0.45f, 1.6f, 5, 0.25f};
    int t = handsType();
    return (t >= 0 && idef[t].cat == CAT_MELEE) ? &idef[t] : &fists;
}
static float meleeDmg(void) { float d = weaponDef()->dmg * (1 + 0.07f * skLvl(SK_STR)); return pl.stage == 3 ? d * 1.5f : d; }   // insanity: rage
static float meleeCd(void) { return weaponDef()->cd * encSwing(); }
static float meleeRange(void) { return weaponDef()->range; }
static const char *weaponName(void) { return pl.hands.t >= 0 ? idef[pl.hands.t].name : "Fists"; }
// move hands <-> slot i (equip / swap)
static bool isConsumable(int t) { if (t < 0) return false; int c = idef[t].cat; return c == CAT_FOOD || c == CAT_DRINK || c == CAT_MED; }   // eaten, drunk or applied from the hand only
static void equipFromSlot(int i) {
    Slot s = pl.sl[i];
    if (s.t < 0) return;
    if (!(idef[s.t].cat == CAT_MELEE || idef[s.t].cat == CAT_GUN || idef[s.t].cat == CAT_THROW || isConsumable(s.t))) return;
    pl.sl[i] = pl.hands; pl.hands = s; pl.swingT = 0; pl.reloadT = 0; pl.build = false; animPlay(ANIM_EQUIP);
    if (s.t == IT_KATANA) play(sBlade, 0.55f); else if (s.t == IT_REVOLVER) play(sCloth, 0.5f); else { play(sCloth, 0.4f); play(sClick, 0.3f); }
}
static void holster(void) {   // put the held weapon away (fists)
    if (pl.hands.t < 0) return;
    int left = slotsAdd(pl.sl, invNSlots(), pl.hands.t, pl.hands.n);
    if (left == 0) { pl.hands.t = -1; pl.hands.n = 0; play(sCloth, 0.45f); animPlay(ANIM_EQUIP); } else say("No room to put it away.");
}
// ---- wounds and blood (pl.hp is blood, 0-100; there is no health bar)
static void addWound(int n) { pl.wounds += n; if (pl.wounds > 6) pl.wounds = 6; }
static void stopBleeding(int t) {
    if (pl.wounds <= 0) { say("You are not bleeding."); return; }
    int med = skLvl(SK_MED);
    if (t == IT_BANDAGE) { int k = 2 + (med >= 3 ? 1 : 0); pl.wounds -= k; if (pl.wounds < 0) pl.wounds = 0; say(pl.wounds ? "Bandaged. Still bleeding somewhere." : "Bleeding stopped."); pl.regenT = 0; }
    else { if (fr(0, 1) < 0.55f + med * 0.08f) { pl.wounds--; say(pl.wounds ? "The rag holds. More wounds left." : "The rag holds. Bleeding stopped."); } else say("The rag soaks through. It didn't hold.");
        if (pl.infect <= 0 && fr(0, 1) < 0.25f - med * 0.03f) pl.infect = 1; }   // dirty cloth: the wound can turn
    skGain(SK_MED, 6);
}
// ---- use an item from the inventory (consumables) or equip it (weapons)
static void spiritSeal(void);
static float trMul(int which);
static void getSick(float amount, const char *why);
static void giveBack(int t, int n) { int left = invAdd(t, n); if (left) addItem(V3(pl.pos.x, pl.pos.y + 0.02f, pl.pos.z), t, left); }
static const char *noNeed(int t) {   // why using it now would do nothing (NULL: it would help)
    switch (t) {
    case IT_FOOD: case IT_CANNED: case IT_RAMEN: case IT_DAIKON: return pl.food >= 95 ? "You are full." : NULL;
    case IT_DRINK: case IT_WATER: case IT_DIRTYWATER: return pl.water >= 95 ? "You are not thirsty." : NULL;
    case IT_PAINKILLER: return (pl.hp >= 99 && pl.fracture == 0 && pl.painT <= 0) ? "Nothing hurts." : NULL;
    case IT_SPLINT: return pl.fracture != 1 ? (pl.fracture == 2 ? "The leg is already splinted." : "Nothing is broken.") : NULL;
    case IT_CHARCOAL: return pl.sick <= 0 ? "Your stomach is fine." : NULL;
    case IT_DISINFECT: return (pl.infect <= 0 && pl.wounds <= 0) ? "No wound to clean." : NULL;
    case IT_BANDAGE: case IT_RAG: return pl.wounds <= 0 ? "You are not bleeding." : NULL;
    }
    return NULL;
}
static void consumeItem(int t);
static void useItem(int t) {   // inventory right-click and the quickbar: weapons and consumables go to the hand, other things are used
    if (t < 0 || !invHas(t)) { if (t >= 0) say(TextFormat("No %s.", idef[t].name)); return; }
    int cat = idef[t].cat;
    if (cat == CAT_MELEE || cat == CAT_GUN || cat == CAT_THROW || isConsumable(t)) {
        if (pl.hands.t == t) { holster(); return; }
        for (int i = 0; i < invNSlots(); i++) if (pl.sl[i].t == t) { equipFromSlot(i); if (pl.hands.t == t && isConsumable(t)) say(TextFormat("%s in hand. %s to %s.", idef[t].name, actKey(A_ATTACK), cat == CAT_FOOD ? "eat" : cat == CAT_DRINK ? "drink" : "use")); return; }
        return;
    }
    consumeItem(t);
}
static void useHeld(void) {   // the attack button with a consumable in hand
    int t = pl.hands.t; if (!isConsumable(t) || animBusy()) return;
    const char *why = noNeed(t); if (why) { say(why); if (aErrOk) { SetSoundVolume(aErr, 0.5f * gs.ui); PlaySound(aErr); } return; }
    consumeItem(t);
}
static void consumeItem(int t) {
    if (t < 0 || !invHas(t)) return;
    { const char *why = noNeed(t); if (why) { say(why); return; } }   // nothing to gain: no animation, nothing used
    { int a = 0; switch (t) { case IT_FOOD: case IT_CANNED: case IT_RAMEN: case IT_DAIKON: a = ANIM_EAT; break; case IT_DRINK: case IT_WATER: case IT_DIRTYWATER: a = ANIM_DRINK; break;
        case IT_PAINKILLER: case IT_CHARCOAL: a = ANIM_PILL; break; case IT_BANDAGE: case IT_RAG: case IT_SPLINT: case IT_DISINFECT: a = ANIM_BANDAGE; break; }
      if (a) animUse(a, t); }
    switch (t) {
    case IT_FOOD: invTake(t, 1); pl.food = fminf(100, pl.food + 30); say("Ate an onigiri."); break;
    case IT_CANNED: invTake(t, 1); pl.food = fminf(100, pl.food + 50); pl.water = fmaxf(0, pl.water - 5); say("Canned mackerel. Salty."); break;
    case IT_DRINK: invTake(t, 1); pl.water = fminf(100, pl.water + 35); say("Drank green tea."); giveBack(IT_BOTTLE, 1); break;
    case IT_WATER: invTake(t, 1); pl.water = fminf(100, pl.water + 55); say("Drank water."); giveBack(IT_BOTTLE, 1); break;
    case IT_DIRTYWATER: invTake(t, 1); pl.water = fminf(100, pl.water + 40); giveBack(IT_BOTTLE, 1);
        if (fr(0, 1) < 0.45f * trMul(1)) getSick(45, "The water tasted wrong."); else say("Unboiled water. You got lucky this time."); break;
    case IT_DAIKON: invTake(t, 1); pl.food = fminf(100, pl.food + 25); pl.water = fminf(100, pl.water + 10); pl.sanity += 4; say("Fresh daikon. Something you grew yourself."); break;
    case IT_SPLINT: if (pl.fracture != 1) { say(pl.fracture == 2 ? "The leg is already splinted." : "Nothing is broken."); return; } invTake(t, 1); pl.fracture = 2; pl.fracT = 0; say("Leg splinted. It will heal over the next half day."); skGain(SK_MED, 6); break;
    case IT_CHARCOAL: if (pl.sick <= 0) { say("Your stomach is fine."); return; } invTake(t, 1); pl.sick = fmaxf(0, pl.sick - 55); say("Charcoal. The nausea fades."); break;
    case IT_DISINFECT: if (pl.infect <= 0 && pl.wounds <= 0) { say("No wound to clean."); return; } invTake(t, 1); pl.infect = 0; pl.woundT = 0; say(pl.wounds ? "Wounds cleaned. They still need bandaging." : "Infection cleaned out. The fever breaks."); skGain(SK_MED, 4); break;
    case IT_SEEDS: say("Plant seeds in a Garden Bed (B: build menu)."); return;
    case IT_BOTTLE: say("Fill it at a rain barrel or fountain [E]."); return;
    case IT_RAMEN:
        invTake(t, 1);
        if (pl.nearFire) { pl.food = fminf(100, pl.food + 45); pl.water = fminf(100, pl.water + 10); pl.sanity += 20; pl.warmth = fminf(100, pl.warmth + 25); say("A hot meal. Warmth spreads through you."); }
        else { pl.food = fminf(100, pl.food + 22); say("Eaten dry. It fills you, nothing more."); }
        break;
    case IT_BANDAGE: case IT_RAG: if (pl.wounds <= 0) { say("You are not bleeding."); return; } invTake(t, 1); stopBleeding(t); break;
    case IT_PAINKILLER: invTake(t, 1); pl.painT = 120; say("Painkillers. The shaking eases."); break;
    case IT_BATTERY: invTake(t, 1); pl.batt = 100; say("Battery replaced."); break;
    case IT_MANGA: invTake(t, 1); { static int readN = 0; mangaRegen(readN++); } pl.watchKind = 2; pl.watchT = MANGA_T; pl.invOpen = false; break;
    case IT_VHS: say("Find a TV and press E to watch it."); return;
    case IT_SALT: invTake(t, 1); addWard(V3(pl.pos.x, pl.pos.y, pl.pos.z), 1.9f, 120, 0); say("Salt circle poured. Spirits cannot cross it for a while."); break;
    case IT_OFUDA: spiritSeal(); return;
    default: say(idef[t].desc); return;
    }
    switch (t) {   // what using it sounds like
    case IT_FOOD: case IT_CANNED: case IT_RAMEN: case IT_DAIKON: play(sEat, 0.6f); break;
    case IT_DRINK: case IT_WATER: case IT_DIRTYWATER: play(sDrink, 0.6f); break;
    case IT_PAINKILLER: case IT_CHARCOAL: play(sPill, 0.6f); break;
    case IT_BANDAGE: case IT_RAG: case IT_SPLINT: case IT_DISINFECT: play(sBandage, 0.6f); break;
    case IT_MANGA: play(sPaper, 0.5f); break;
    default: play(sPick, 0.4f);
    }
}
static void quickUse(int k) { if (pl.quick[k] >= 0) useItem(pl.quick[k]); }
static void invReset(void) {
    pl.hands.t = pl.back.t = -1; pl.hands.n = pl.back.n = 0; slotsClear(pl.wear, WS_N);
    slotsClear(pl.sl, POCK + 18);
    for (int i = 0; i < 6; i++) pl.quick[i] = -1;
    pl.openC = -1; pl.searchC = -1; pl.dragFrom = -1; pl.drag.t = -1;
}
