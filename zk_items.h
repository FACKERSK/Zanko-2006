// zk_items.h — item definitions (weights, stacks, weapon stats, bag sizes)
enum { IT_FOOD, IT_DRINK, IT_BANDAGE, IT_BATTERY, IT_AMMO, IT_PLANKS, IT_OFUDA, IT_SALT, IT_ROPE, IT_MANGA, IT_VHS, IT_RAMEN, IT_AXE, IT_KATANA, IT_REVOLVER,
       IT_PIPE, IT_BAT, IT_CROWBAR, IT_KNIFE, IT_SLEDGE, IT_HAMMER, IT_NAILS, IT_SCRAP, IT_RAG, IT_PAINKILLER, IT_CANNED, IT_WATER,
       IT_BOTTLE, IT_DIRTYWATER, IT_SPLINT, IT_CHARCOAL, IT_DISINFECT, IT_SEEDS, IT_DAIKON, IT_NAILBAT,
       IT_SCHOOLBAG, IT_DUFFEL, IT_HIKEPACK,
       IT_WAKIZASHI, IT_NAGINATA, IT_BOKKEN, IT_GOLFCLUB, IT_SHOVEL, IT_MACHETE, IT_CLEAVER, IT_SICKLE, IT_WRENCH, IT_BATON, IT_TETSUBO, IT_PICKAXE, IT_SCALPEL, IT_FIREEXT, IT_UMBRELLA, IT_STOPSIGN,
       IT_HARDHAT, IT_RIOTHELM, IT_BEANIE, IT_COAT, IT_LEATHER, IT_STABVEST, IT_GAKURAN, IT_HANTEN, IT_WORKPANTS, IT_BIKEPANTS, IT_BOOTS, IT_RUBBERBOOTS,
       IT_PISTOL, IT_SHOTGUN, IT_ACP, IT_SHELLS, IT_ROCK, IT_FIRECRACKER, IT_MOLOTOV, IT_KEY, IT_FUEL, IT_RAINCOAT, IT_N };
enum { CAT_FOOD, CAT_DRINK, CAT_MED, CAT_TOOL, CAT_MELEE, CAT_GUN, CAT_AMMO, CAT_MAT, CAT_SPIRIT, CAT_COMFORT, CAT_BAG, CAT_CLOTHES, CAT_THROW };
enum { WS_HEAD, WS_BODY, WS_LEGS, WS_FEET, WS_N };
static const char *wsName[WS_N] = {"Head", "Body", "Legs", "Feet"};
typedef struct {
    const char *name; float weight; int stack, cat;
    float dmg, cd, range, stam, knock;   // melee: damage per hit, swing time (s), reach (m), stamina per swing, knockback (m)
    int bag;                              // bag: extra slots
    int style;                            // melee swing: 0 by type, 1 blade, 2 stab, 3 blunt, 4 heavy two-handed, 5 polearm, 6 hook, 7 spray
    int wslot; float armor, warm;         // clothes: where worn (WS_*), share of damage stopped, warmth added; stam = % more stamina used
    float dry; int cap, ammo;             // rain kept off (0-1, worn or held); guns: rounds held and the round they take
    const char *desc;
} ItemDef;
static ItemDef idef[IT_N] = {   // mutable: Data/items.ini (or a mod's) changes stats at start (see itemsLoad)
    [IT_FOOD]      = {"Onigiri", 0.15f, 5, CAT_FOOD, .desc = "Rice ball. Food +30."},
    [IT_DRINK]     = {"Green Tea", 0.5f, 3, CAT_DRINK, .desc = "Bottled tea. Water +35."},
    [IT_BANDAGE]   = {"Bandage", 0.1f, 5, CAT_MED, .desc = "Stops up to 2 bleeding wounds."},
    [IT_BATTERY]   = {"Battery", 0.1f, 4, CAT_TOOL, .desc = "Flashlight battery."},
    [IT_AMMO]      = {".38 Rounds", 0.02f, 30, CAT_AMMO, .desc = "Revolver ammunition."},
    [IT_PLANKS]    = {"Wood Planks", 1.2f, 10, CAT_MAT, .desc = "Building and barricades."},
    [IT_OFUDA]     = {"Ofuda", 0.02f, 10, CAT_SPIRIT, .desc = "Seals a spirit within reach. Base shrines and wards need it."},
    [IT_SALT]      = {"Salt", 0.5f, 3, CAT_SPIRIT, .desc = "Pour a temporary circle spirits cannot cross."},
    [IT_ROPE]      = {"Shimenawa Rope", 0.8f, 3, CAT_SPIRIT, .desc = "Sacred rope. Needed for the base shrine and spirit wards."},
    [IT_MANGA]     = {"Manga", 0.3f, 3, CAT_COMFORT, .desc = "Reading restores sanity."},
    [IT_VHS]       = {"Anime VHS", 0.3f, 3, CAT_COMFORT, .desc = "Watch on a TV. Big sanity boost."},
    [IT_RAMEN]     = {"Instant Ramen", 0.2f, 4, CAT_FOOD, .desc = "Cook at a fire for warmth and sanity."},
    [IT_AXE]       = {"Fire Axe", 2.6f, 1, CAT_MELEE, 2.6f, 0.95f, 2.4f, 13, 0.9f, .desc = "Heavy, slow, brutal."},
    [IT_KATANA]    = {"Katana", 1.3f, 1, CAT_MELEE, 2.1f, 0.52f, 2.7f, 8, 0.45f, .desc = "Fast and long. Fragile hands make it sing."},
    [IT_REVOLVER]  = {"Revolver", 0.9f, 1, CAT_GUN, .desc = "Six rounds. Loud enough to call the whole street."},
    [IT_PIPE]      = {"Steel Pipe", 1.4f, 1, CAT_MELEE, 1.25f, 0.62f, 2.3f, 9, 0.5f, .desc = "Reliable blunt weapon."},
    [IT_BAT]       = {"Baseball Bat", 1.0f, 1, CAT_MELEE, 1.4f, 0.66f, 2.4f, 9, 0.7f, .desc = "Good knockback."},
    [IT_CROWBAR]   = {"Crowbar", 1.6f, 1, CAT_MELEE, 1.6f, 0.7f, 2.2f, 10, 0.55f, .desc = "Also pries debris apart faster."},
    [IT_KNIFE]     = {"Kitchen Knife", 0.3f, 1, CAT_MELEE, 0.95f, 0.36f, 1.8f, 5, 0.15f, .desc = "Quick, short, causes deep cuts."},
    [IT_SLEDGE]    = {"Sledgehammer", 5.5f, 1, CAT_MELEE, 4.2f, 1.4f, 2.5f, 20, 1.6f, .desc = "Crushes anything. Exhausting."},
    [IT_HAMMER]    = {"Claw Hammer", 0.7f, 1, CAT_MELEE, 1.0f, 0.5f, 1.9f, 6, 0.3f, .desc = "Needed to build walls, doors, chests, beds and floors."},
    [IT_NAILS]     = {"Nails", 0.01f, 50, CAT_MAT, .desc = "Box of nails for building."},
    [IT_SCRAP]     = {"Scrap Metal", 1.0f, 8, CAT_MAT, .desc = "Reinforces walls (hold G on a wall)."},
    [IT_RAG]       = {"Rags", 0.05f, 10, CAT_MED, .desc = "Improvised bandage: stops 1 wound, may fail."},
    [IT_PAINKILLER]= {"Painkillers", 0.05f, 6, CAT_MED, .desc = "Calms shaking and speeds blood recovery for a while."},
    [IT_CANNED]    = {"Canned Mackerel", 0.4f, 4, CAT_FOOD, .desc = "Food +50."},
    [IT_WATER]     = {"Water Bottle", 1.0f, 2, CAT_DRINK, .desc = "Water +55."},
    [IT_BOTTLE]    = {"Empty Bottle", 0.1f, 4, CAT_TOOL, .desc = "Fill it at a rain barrel or fountain [E]."},
    [IT_DIRTYWATER]= {"Unboiled Water", 1.0f, 2, CAT_DRINK, .desc = "Water +40, may make you sick. Boil it at a fire (Crafting)."},
    [IT_SPLINT]    = {"Splint", 0.4f, 2, CAT_MED, .desc = "Binds a broken leg so it can heal."},
    [IT_CHARCOAL]  = {"Charcoal", 0.1f, 6, CAT_MED, .desc = "Settles the stomach. Cuts sickness sharply."},
    [IT_DISINFECT] = {"Disinfectant", 0.3f, 3, CAT_MED, .desc = "Cleans infected wounds and breaks a fever."},
    [IT_SEEDS]     = {"Daikon Seeds", 0.02f, 10, CAT_MAT, .desc = "Plant in a garden bed (Build menu)."},
    [IT_DAIKON]    = {"Daikon", 0.6f, 6, CAT_FOOD, .desc = "Home-grown radish. Food +25, Water +10."},
    [IT_NAILBAT]   = {"Nail Bat", 1.2f, 1, CAT_MELEE, 1.9f, 0.68f, 2.4f, 10, 0.65f, .desc = "Bat studded with nails. Tears flesh, wears fast."},
    [IT_SCHOOLBAG] = {"Randoseru", 0.8f, 1, CAT_BAG, .bag = 6, .desc = "Leather school bag. +6 slots."},
    [IT_DUFFEL]    = {"Duffel Bag", 1.2f, 1, CAT_BAG, .bag = 10, .desc = "Gym duffel. +10 slots."},
    [IT_HIKEPACK]  = {"Hiking Pack", 1.8f, 1, CAT_BAG, .bag = 16, .desc = "Framed backpack. +16 slots."},
    [IT_WAKIZASHI] = {"Wakizashi", 0.9f, 1, CAT_MELEE, 1.6f, 0.48f, 2.1f, 8, 0.3f, .style = 1, .desc = "Short companion sword. Quick, keen, forgiving."},
    [IT_NAGINATA]  = {"Naginata", 2.4f, 1, CAT_MELEE, 2.0f, 0.85f, 3.0f, 14, 0.7f, .style = 5, .desc = "Curved blade on a long shaft. Keeps them at arm's length and then some."},
    [IT_BOKKEN]    = {"Bokken", 0.7f, 1, CAT_MELEE, 1.1f, 0.5f, 2.2f, 7, 0.6f, .style = 1, .desc = "Oak practice sword. Light, bruising, never needs sharpening."},
    [IT_GOLFCLUB]  = {"Golf Club", 0.6f, 1, CAT_MELEE, 1.2f, 0.6f, 2.4f, 9, 0.7f, .style = 3, .desc = "A driver from a bag left in a car. Whippy and loud."},
    [IT_SHOVEL]    = {"Shovel", 1.8f, 1, CAT_MELEE, 1.5f, 0.8f, 2.5f, 13, 0.9f, .style = 4, .desc = "Spade from a garden shed. Digs into anything."},
    [IT_MACHETE]   = {"Machete", 0.8f, 1, CAT_MELEE, 1.7f, 0.55f, 2.0f, 9, 0.35f, .style = 1, .desc = "Brush knife. Wide blade, chops through limbs."},
    [IT_CLEAVER]   = {"Meat Cleaver", 0.7f, 1, CAT_MELEE, 1.45f, 0.45f, 1.6f, 7, 0.3f, .style = 3, .desc = "From a butcher's block. Short but it bites deep."},
    [IT_SICKLE]    = {"Kama Sickle", 0.5f, 1, CAT_MELEE, 1.35f, 0.42f, 1.7f, 6, 0.2f, .style = 6, .desc = "Farmer's sickle. Hooks and pulls."},
    [IT_WRENCH]    = {"Pipe Wrench", 1.6f, 1, CAT_MELEE, 1.4f, 0.65f, 1.9f, 10, 0.8f, .style = 3, .desc = "Heavy plumber's wrench. Cracks skulls and padlocks."},
    [IT_BATON]     = {"Police Baton", 0.6f, 1, CAT_MELEE, 1.0f, 0.38f, 1.8f, 5, 0.5f, .style = 3, .desc = "Extending steel baton from the koban. Fast and tireless."},
    [IT_TETSUBO]   = {"Tetsubo", 4.2f, 1, CAT_MELEE, 2.4f, 1.05f, 2.4f, 18, 1.2f, .style = 4, .desc = "Iron-studded war club from the castle museum. Two hands, every ounce of you."},
    [IT_PICKAXE]   = {"Pickaxe", 3.0f, 1, CAT_MELEE, 2.1f, 0.95f, 2.2f, 16, 0.8f, .style = 4, .desc = "Mining pick. Punches through bone and doors alike."},
    [IT_SCALPEL]   = {"Scalpel", 0.05f, 1, CAT_MELEE, 0.9f, 0.3f, 1.3f, 3, 0.05f, .style = 2, .desc = "Surgical steel. Barely any reach; barely any effort."},
    [IT_FIREEXT]   = {"Fire Extinguisher", 4.5f, 1, CAT_MELEE, 1.8f, 1.0f, 1.8f, 15, 1.3f, .style = 7, .desc = "Red cylinder. Swing it to knock them flat, or hold [R] to spray: a white cloud that blinds and puts out fire."},
    [IT_UMBRELLA]  = {"Umbrella", 0.5f, 1, CAT_MELEE, 0.6f, 0.4f, 2.2f, 4, 0.4f, .style = 3, .dry = 1.0f, .desc = "A clear convenience-store umbrella. [R] opens it to keep the rain off; closed, it is a poor club."},
    [IT_STOPSIGN]  = {"Stop Sign", 3.4f, 1, CAT_MELEE, 1.9f, 0.9f, 2.8f, 15, 1.1f, .style = 4, .desc = "Tomare. Pulled out of the pavement, post and all."},
    [IT_HARDHAT]   = {"Hard Hat", 0.4f, 1, CAT_CLOTHES, .stam = 0, .wslot = 0, .armor = 0.1f, .warm = 2.0f, .desc = "Construction helmet. Glancing blows to the head stay glancing."},
    [IT_RIOTHELM]  = {"Riot Helmet", 1.1f, 1, CAT_CLOTHES, .stam = 2, .wslot = 0, .armor = 0.18f, .warm = 3.0f, .desc = "Police riot helmet with a visor. Heavy, close, safe."},
    [IT_BEANIE]    = {"Wool Beanie", 0.1f, 1, CAT_CLOTHES, .stam = 0, .wslot = 0, .armor = 0.02f, .warm = 8.0f, .desc = "Knitted cap. Most of your warmth goes out through your head."},
    [IT_COAT]      = {"Winter Coat", 1.6f, 1, CAT_CLOTHES, .stam = 3, .wslot = 1, .armor = 0.06f, .dry = 0.35f, .warm = 18.0f, .desc = "Down parka. Stays warm through the coldest night outdoors."},
    [IT_LEATHER]   = {"Leather Jacket", 1.4f, 1, CAT_CLOTHES, .stam = 1, .wslot = 1, .armor = 0.12f, .dry = 0.3f, .warm = 9.0f, .desc = "Biker's jacket. Teeth and claws struggle with it."},
    [IT_STABVEST]  = {"Stab Vest", 2.6f, 1, CAT_CLOTHES, .stam = 6, .wslot = 1, .armor = 0.24f, .warm = 4.0f, .desc = "Police body armour. Turns most bites, slows you down."},
    [IT_GAKURAN]   = {"Gakuran", 0.9f, 1, CAT_CLOTHES, .stam = 0, .wslot = 1, .armor = 0.03f, .warm = 6.0f, .desc = "Black school uniform jacket with brass buttons. Someone's son."},
    [IT_HANTEN]    = {"Hanten", 1.0f, 1, CAT_CLOTHES, .stam = 0, .wslot = 1, .armor = 0.04f, .warm = 14.0f, .desc = "Padded winter house jacket. Smells of someone's grandmother."},
    [IT_WORKPANTS] = {"Work Trousers", 0.9f, 1, CAT_CLOTHES, .stam = 0, .wslot = 2, .armor = 0.06f, .warm = 5.0f, .desc = "Tobi carpenter's trousers, baggy at the knee. Thick canvas."},
    [IT_BIKEPANTS] = {"Motorcycle Pants", 1.5f, 1, CAT_CLOTHES, .stam = 2, .wslot = 2, .armor = 0.12f, .warm = 6.0f, .desc = "Armoured riding trousers. Knee pads built in."},
    [IT_BOOTS]     = {"Work Boots", 1.2f, 1, CAT_CLOTHES, .stam = 1, .wslot = 3, .armor = 0.06f, .warm = 4.0f, .desc = "Steel toecaps. Fewer broken bones when you fall."},
    [IT_RAINCOAT]  = {"Raincoat", 0.7f, 1, CAT_CLOTHES, .stam = 0, .wslot = 1, .armor = 0.02f, .warm = 5.0f, .dry = 0.85f, .desc = "Clear vinyl from a konbini rack. Keeps the rain off, not much else."},
    [IT_PISTOL]    = {"Police Pistol", 0.6f, 1, CAT_GUN, 2.6f, 0.2f, 90, .cap = 8, .ammo = IT_ACP, .desc = "SIG P230 from a koban. Eight rounds, quick follow-ups, weak per shot."},
    [IT_SHOTGUN]   = {"Hunting Shotgun", 3.2f, 1, CAT_GUN, 1.3f, 0.6f, 45, .cap = 2, .ammo = IT_SHELLS, .desc = "Over-under 12 gauge from a hunter's cabinet. Two shells, nine pellets each. Deafening."},
    [IT_ACP]       = {".32 Rounds", 0.015f, 40, CAT_AMMO, .desc = "Pistol ammunition."},
    [IT_SHELLS]    = {"12ga Shells", 0.04f, 20, CAT_AMMO, .desc = "Buckshot for the shotgun."},
    [IT_ROCK]      = {"Rock", 0.4f, 8, CAT_THROW, 1.5f, .desc = "Throw it [LMB] to make a noise somewhere else. Hurts a little if it lands on one."},
    [IT_FIRECRACKER] = {"Firecrackers", 0.1f, 6, CAT_THROW, .desc = "A string of summer hanabi. Thrown, it crackles for seconds and pulls everything to it."},
    [IT_MOLOTOV]   = {"Molotov", 0.7f, 3, CAT_THROW, 6.0f, .desc = "Bottle, rag, petrol. Bursts into a pool of fire that burns whatever walks through it."},
    [IT_KEY]       = {"Spare Key", 0.02f, 10, CAT_TOOL, .desc = "Someone's spare. Opens one locked door [E] without breaking it."},
    [IT_FUEL]      = {"Petrol Can", 2.2f, 4, CAT_MAT, .desc = "Runs a generator for about ten minutes. Also goes into molotovs."},
    [IT_RUBBERBOOTS] = {"Rubber Boots", 0.9f, 1, CAT_CLOTHES, .stam = 0, .wslot = 3, .armor = 0.03f, .warm = 5.0f, .desc = "Fisherman's wellingtons from the market. Dry feet."},
};
static ItemDef idefBase[IT_N]; static bool idefBaseOk = false; static char idefName[IT_N][32], idefDesc[IT_N][160];
#define ITEM_FIELDS X(weight, f) X(stack, i) X(dmg, f) X(cd, f) X(range, f) X(stam, f) X(knock, f) X(bag, i) X(armor, f) X(warm, f) X(dry, f) X(cap, i)
static void itemsReset(void) { if (!idefBaseOk) { memcpy(idefBase, idef, sizeof idef); idefBaseOk = true; } memcpy(idef, idefBase, sizeof idef); }
static const char *itemKey(int t) { static char k[40]; int j = 0; for (const char *p = idefBase[t].name; *p && j < 38; p++) k[j++] = (*p == ' ') ? '_' : (char)tolower((unsigned char)*p); k[j] = 0; return k; }
static bool itemsLoadFile(const char *path) {   // [item_key] then name=, desc=, weight=, stack=, dmg=, cd=, range=, stam=, knock=, bag=
    FILE *f = fopen(path, "r"); if (!f) return false; char line[300]; int cur = -1;
    while (fgets(line, sizeof line, f)) { char *e = line + strlen(line); while (e > line && (e[-1] == '\n' || e[-1] == '\r')) *--e = 0;
        if (line[0] == '[') { char *c = strchr(line, ']'); if (c) *c = 0; cur = -1; for (int t = 0; t < IT_N; t++) if (!strcmp(itemKey(t), line + 1)) cur = t; continue; }
        char *eq = strchr(line, '='); if (cur < 0 || !eq || line[0] == '#') continue; *eq = 0; const char *k = line, *v = eq + 1;
        if (!strcmp(k, "name")) { snprintf(idefName[cur], 32, "%s", v); idef[cur].name = idefName[cur]; }
        else if (!strcmp(k, "desc")) { snprintf(idefDesc[cur], 160, "%s", v); idef[cur].desc = idefDesc[cur]; }
#define X(n, ty) else if (!strcmp(k, #n)) idef[cur].n = (#ty[0] == 'f') ? (float)atof(v) : atoi(v);
        ITEM_FIELDS
#undef X
    }
    fclose(f); return true;
}
static bool itemsSave(const char *path) {   // only what differs from the game's own values
    FILE *f = fopen(path, "w"); if (!f) return false; fprintf(f, "# Zanko 2006 item changes (Mod Maker). Sections are item names in lower case.\n");
    for (int t = 0; t < IT_N; t++) { ItemDef *a = &idef[t], *b = &idefBase[t]; bool any = false;
#define HDR if (!any) { fprintf(f, "\n[%s]\n", itemKey(t)); any = true; }
        if (strcmp(a->name, b->name)) { HDR fprintf(f, "name=%s\n", a->name); }
        if (strcmp(a->desc ? a->desc : "", b->desc ? b->desc : "")) { HDR fprintf(f, "desc=%s\n", a->desc ? a->desc : ""); }
#define X(n, ty) if (a->n != b->n) { HDR if (#ty[0] == 'f') fprintf(f, #n "=%g\n", (double)a->n); else fprintf(f, #n "=%d\n", (int)a->n); }
        ITEM_FIELDS
#undef X
#undef HDR
    }
    fclose(f); return true;
}

#define itName_(t) (idef[(t)].name)
typedef struct { short t, n, w; } Slot;   // t = item type or -1, w = wear 0-100 (weapons break at 100)
