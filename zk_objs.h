// zk_objs.h — placed objects (the Mod Maker's Build tab): blocks with any texture and colour, turned and sized freely,
// and ready-made props. Stored in Data/objects.zobj (a mod's wins), one object per line:
//   type x y z sizeX sizeY sizeZ yawDegrees texture r g b solid      (x, z: centre; y: bottom)
enum { MO_BLOCK, MO_TREE, MO_CHERRY, MO_PINE, MO_CEDAR, MO_VENDING, MO_CAR, MO_LAMP, MO_N };
static const char *moNames[MO_N] = {"Block", "Tree", "Cherry tree", "Garden pine", "Cedar", "Vending machine", "Car", "Lamp post"};
typedef struct { int type; Vector3 p, s; float yaw; int tex; Color col; int solid; } MObj;
#define MAX_MOBJ 4096
static MObj mobj[MAX_MOBJ]; static int nMobj = 0;
static bool mobjLive = false;
static void mobjClear(void) { nMobj = 0; }   // the Mod Maker draws objects itself while they are being edited
static Vector3 moDefaultSize(int type) {
    switch (type) { case MO_BLOCK: return V3(4, 1, 2); case MO_VENDING: return V3(1.1f, 1.9f, 0.8f); case MO_CAR: return V3(1.8f, 1.4f, 4.2f);
    case MO_LAMP: return V3(0.3f, 6, 0.3f); case MO_CEDAR: return V3(3, 10, 3); default: return V3(3, 5, 3); }
}
static bool mobjLoadFile(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) return false; nMobj = 0; char line[256];
    while (fgets(line, sizeof line, f) && nMobj < MAX_MOBJ) { if (line[0] == '#') continue; MObj o; int r, g, b;
        if (sscanf(line, "%d %f %f %f %f %f %f %f %d %d %d %d %d", &o.type, &o.p.x, &o.p.y, &o.p.z, &o.s.x, &o.s.y, &o.s.z, &o.yaw, &o.tex, &r, &g, &b, &o.solid) == 13 && o.type >= 0 && o.type < MO_N) {
            o.tex = o.tex < 0 || o.tex >= T_COUNT ? T_CONCRETE : o.tex; o.col = (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255}; mobj[nMobj++] = o; } }
    fclose(f); return true;
}
static bool mobjSave(const char *path) {
    FILE *f = fopen(path, "w"); if (!f) return false;
    fprintf(f, "# Zanko 2006 placed objects: type x y z sizeX sizeY sizeZ yaw texture r g b solid\n");
    for (int i = 0; i < nMobj; i++) { MObj *o = &mobj[i]; fprintf(f, "%d %.3f %.3f %.3f %.3f %.3f %.3f %.2f %d %d %d %d %d\n", o->type, o->p.x, o->p.y, o->p.z, o->s.x, o->s.y, o->s.z, o->yaw, o->tex, o->col.r, o->col.g, o->col.b, o->solid); }
    fclose(f); return true;
}
static unsigned int mobjHash(unsigned int h) { const unsigned char *b = (const unsigned char *)mobj; size_t n = sizeof(MObj) * nMobj; for (size_t k = 0; k < n; k++) { h ^= b[k]; h *= 16777619u; } return h ^ (unsigned)nMobj; }
static void genObjs(void) {   // into the world, at the end of the town
    if (mobjLive) return;
    for (int i = 0; i < nMobj; i++) { MObj *o = &mobj[i]; float y = o->p.y;
        switch (o->type) {
        case MO_BLOCK: { int f = newXf(o->p.x, o->p.z, -o->yaw * DEG2RAD); int keep = curXf; curXf = f;
            box(o->tex, V3(o->p.x - o->s.x / 2, y, o->p.z - o->s.z / 2), V3(o->p.x + o->s.x / 2, y + o->s.y, o->p.z + o->s.z / 2), o->col, o->solid != 0, ALL); curXf = keep; } break;
        case MO_TREE: townTree(o->p.x, o->p.z, y, o->s.y, 0); break;
        case MO_CHERRY: townTree(o->p.x, o->p.z, y, o->s.y, 1); break;
        case MO_PINE: townTree(o->p.x, o->p.z, y, o->s.y, 2); break;
        case MO_CEDAR: cedar(o->p.x, y, o->p.z, o->s.y); break;
        case MO_VENDING: vending(o->p.x, o->p.z, ((int)lroundf(o->yaw / 90) % 4 + 4) % 4); break;
        case MO_CAR: car(o->p.x, o->p.z, fabsf(sinf(o->yaw * DEG2RAD)) < 0.7f); break;
        case MO_LAMP: box(T_CONCRETE, V3(o->p.x - 0.13f, y, o->p.z - 0.13f), V3(o->p.x + 0.13f, y + o->s.y, o->p.z + 0.13f), rgb(150, 150, 140), o->solid != 0, ALL);
            addLight(V3(o->p.x, y + o->s.y - 0.5f, o->p.z), V3(1.0f, 0.75f, 0.45f), 12, 1); break;
        } }
}
