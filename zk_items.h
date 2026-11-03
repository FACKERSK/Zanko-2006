// zk_items.h — item definitions (weights, stacks, weapon stats, bag sizes)
enum { IT_FOOD, IT_DRINK, IT_BANDAGE, IT_BATTERY, IT_AMMO, IT_PLANKS, IT_OFUDA, IT_SALT, IT_ROPE, IT_MANGA, IT_VHS, IT_RAMEN, IT_AXE, IT_KATANA, IT_REVOLVER,
       IT_PIPE, IT_BAT, IT_CROWBAR, IT_KNIFE, IT_SLEDGE, IT_HAMMER, IT_NAILS, IT_SCRAP, IT_RAG, IT_PAINKILLER, IT_CANNED, IT_WATER,
       IT_BOTTLE, IT_DIRTYWATER, IT_SPLINT, IT_CHARCOAL, IT_DISINFECT, IT_SEEDS, IT_DAIKON, IT_NAILBAT,
       IT_SCHOOLBAG, IT_DUFFEL, IT_HIKEPACK, IT_N };
enum { CAT_FOOD, CAT_DRINK, CAT_MED, CAT_TOOL, CAT_MELEE, CAT_GUN, CAT_AMMO, CAT_MAT, CAT_SPIRIT, CAT_COMFORT, CAT_BAG };
typedef struct {
    const char *name; float weight; int stack, cat;
    float dmg, cd, range, stam, knock;   // melee: damage per hit, swing time (s), reach (m), stamina per swing, knockback (m)
    int bag;                              // bag: extra slots
    const char *desc;
} ItemDef;
static const ItemDef idef[IT_N] = {
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
};
#define itName_(t) (idef[(t)].name)
typedef struct { short t, n, w; } Slot;   // t = item type or -1, w = wear 0-100 (weapons break at 100)
