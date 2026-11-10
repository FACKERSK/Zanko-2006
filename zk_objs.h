// zk_objs.h — placed objects (the Mod Maker's Build tab): blocks with any texture and colour, turned and sized freely,
// and ready-made props. Stored in Data/objects.zobj (a mod's wins), one object per line:
//   type x y z sizeX sizeY sizeZ yawDegrees texture r g b solid      (x, z: centre; y: bottom)
enum { MO_BLOCK, MO_TREE, MO_CHERRY, MO_PINE, MO_CEDAR, MO_VENDING, MO_CAR, MO_LAMP, MO_BUILDING, MO_N };   // a building: tex = preset, solid = storeys (0: the preset's), col = wall colour (black: the preset's)
static const char *moNames[MO_N] = {"Block", "Tree", "Cherry tree", "Garden pine", "Cedar", "Vending machine", "Car", "Lamp post", "Building"};
typedef struct { int type; Vector3 p, s; float yaw; int tex; Color col; int solid; float pitch, roll; } MObj;   // angles in degrees; pitch and roll tilt blocks
#define MAX_MOBJ 4096
static MObj mobj[MAX_MOBJ]; static int nMobj = 0;
static bool mobjLive = false;
static void mobjClear(void) { nMobj = 0; }   // the Mod Maker draws objects itself while they are being edited
static Vector3 moDefaultSize(int type) {
    switch (type) { case MO_BLOCK: return V3(4, 1, 2); case MO_VENDING: return V3(1.1f, 1.9f, 0.8f); case MO_CAR: return V3(1.8f, 1.4f, 4.2f);
    case MO_LAMP: return V3(0.3f, 6, 0.3f); case MO_CEDAR: return V3(3, 10, 3); case MO_BUILDING: return V3(8, 7, 7); default: return V3(3, 5, 3); }
}
static bool mobjLoadFile(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) return false; nMobj = 0; char line[256];
    while (fgets(line, sizeof line, f) && nMobj < MAX_MOBJ) { if (line[0] == '#') continue; MObj o; int r, g, b;
        o.pitch = o.roll = 0; int nr = sscanf(line, "%d %f %f %f %f %f %f %f %d %d %d %d %d %f %f", &o.type, &o.p.x, &o.p.y, &o.p.z, &o.s.x, &o.s.y, &o.s.z, &o.yaw, &o.tex, &r, &g, &b, &o.solid, &o.pitch, &o.roll);
        if (nr >= 13 && o.type >= 0 && o.type < MO_N) {
            o.tex = o.tex < 0 || o.tex >= T_COUNT ? T_CONCRETE : o.tex; o.col = (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255}; mobj[nMobj++] = o; } }
    fclose(f); return true;
}
static bool mobjSave(const char *path) {
    FILE *f = fopen(path, "w"); if (!f) return false;
    fprintf(f, "# Zanko 2006 placed objects: type x y z sizeX sizeY sizeZ yaw texture r g b solid pitch roll\n");
    for (int i = 0; i < nMobj; i++) { MObj *o = &mobj[i]; fprintf(f, "%d %.3f %.3f %.3f %.3f %.3f %.3f %.2f %d %d %d %d %d %.2f %.2f\n", o->type, o->p.x, o->p.y, o->p.z, o->s.x, o->s.y, o->s.z, o->yaw, o->tex, o->col.r, o->col.g, o->col.b, o->solid, o->pitch, o->roll); }
    fclose(f); return true;
}
static unsigned int mobjHash(unsigned int h) { const unsigned char *b = (const unsigned char *)mobj; size_t n = sizeof(MObj) * nMobj; for (size_t k = 0; k < n; k++) { h ^= b[k]; h *= 16777619u; } return h ^ (unsigned)nMobj; }
static Matrix moRot(const MObj *o) { return MatrixRotateXYZ(V3(o->pitch * DEG2RAD, -o->yaw * DEG2RAD, o->roll * DEG2RAD)); }   // the same turn the Mod Maker shows
static void tiltedBlock(const MObj *o) {   // a block with pitch or roll: six turned faces, and (if solid) a walkable stack of slabs along it
    Matrix R = moRot(o); Vector3 c = V3(o->p.x, o->p.y + o->s.y / 2, o->p.z), h = Vector3Scale(o->s, 0.5f);
    Vector3 P[8]; for (int k = 0; k < 8; k++) P[k] = Vector3Add(c, Vector3Transform(V3((k & 1) ? h.x : -h.x, (k & 2) ? h.y : -h.y, (k & 4) ? h.z : -h.z), R));
    static const int F[6][4] = {{0, 2, 6, 4}, {1, 5, 7, 3}, {0, 4, 5, 1}, {2, 3, 7, 6}, {0, 1, 3, 2}, {4, 6, 7, 5}};
    static const Vector3 N[6] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
    int keep = curXf; curXf = 0;
    for (int f = 0; f < 6; f++) { Vector3 n = Vector3Transform(N[f], R); quad(o->tex, P[F[f][0]], P[F[f][1]], P[F[f][2]], P[F[f][3]], n, o->col); quad(o->tex, P[F[f][3]], P[F[f][2]], P[F[f][1]], P[F[f][0]], Vector3Negate(n), o->col); }
    curXf = keep;
    if (!o->solid) return;
    int ax = h.x >= h.z ? 0 : 2, ns = (int)fminf(48, fmaxf(1, (ax == 0 ? o->s.x : o->s.z) / 0.4f)); int f = newXf(o->p.x, o->p.z, -o->yaw * DEG2RAD);   // slabs in the block's own (turned) frame
    Matrix T = MatrixRotateXYZ(V3(o->pitch * DEG2RAD, 0, o->roll * DEG2RAD));
    for (int k = 0; k < ns; k++) { float u0 = -1 + 2.0f * k / ns, u1 = -1 + 2.0f * (k + 1) / ns; Vector3 mn = V3(1e9f, 1e9f, 1e9f), mx = V3(-1e9f, -1e9f, -1e9f);
        for (int q = 0; q < 8; q++) { float u = (q & 1) ? u1 : u0; Vector3 lp = ax == 0 ? V3(u * h.x, (q & 2) ? h.y : -h.y, (q & 4) ? h.z : -h.z) : V3((q & 4) ? h.x : -h.x, (q & 2) ? h.y : -h.y, u * h.z);
            Vector3 w = Vector3Add(V3(o->p.x, c.y, o->p.z), Vector3Transform(lp, T)); mn = Vector3Min(mn, w); mx = Vector3Max(mx, w); }
        addSolidF(mn, mx, f); }
}
static void genObjs(void) {   // into the world, at the end of the town
    for (int i = 0; i < nMobj; i++) { MObj *o = &mobj[i]; float y = o->p.y; if (mobjLive && o->type != MO_BUILDING) continue;   // buildings are always built for real (the Mod Maker shows them as they are)
        switch (o->type) {
        case MO_BLOCK: if (o->pitch != 0 || o->roll != 0) { tiltedBlock(o); break; } { int f = newXf(o->p.x, o->p.z, -o->yaw * DEG2RAD); int keep = curXf; curXf = f;
            box(o->tex, V3(o->p.x - o->s.x / 2, y, o->p.z - o->s.z / 2), V3(o->p.x + o->s.x / 2, y + o->s.y, o->p.z + o->s.z / 2), o->col, o->solid != 0, ALL); curXf = keep; } break;
        case MO_TREE: townTree(o->p.x, o->p.z, y, o->s.y, 0); break;
        case MO_CHERRY: townTree(o->p.x, o->p.z, y, o->s.y, 1); break;
        case MO_PINE: townTree(o->p.x, o->p.z, y, o->s.y, 2); break;
        case MO_CEDAR: cedar(o->p.x, y, o->p.z, o->s.y); break;
        case MO_VENDING: vending(o->p.x, o->p.z, ((int)lroundf(o->yaw / 90) % 4 + 4) % 4); break;
        case MO_CAR: car(o->p.x, o->p.z, fabsf(sinf(o->yaw * DEG2RAD)) < 0.7f); break;
        case MO_BUILDING: { int pi = o->tex >= 0 && o->tex < NBPRESET ? o->tex : 0; const BPreset *bp = &bPresets[pi]; BPreset tmp = *bp;
            if (o->col.r || o->col.g || o->col.b) tmp.wall = o->col;
            int floors = o->solid > 0 ? o->solid : bp->floors; bP = &tmp;
            Rectangle r = {o->p.x - bp->w / 2, o->p.z - bp->d / 2, bp->w, bp->d}; int keep = curXf; curXf = newXf(o->p.x, o->p.z, -o->yaw * DEG2RAD); nFoot = 0;
            building(r, bp->kind, floors, 0);
            for (int f = 0; f < nFoot && nMapFoot < MAX_MAPFOOT; f++) { mapFootH[nMapFoot] = footH[f]; mapFootY[nMapFoot] = 0; mapFootF[nMapFoot] = (short)curXf; mapFoot[nMapFoot++] = foots[f]; }
            bP = NULL; curXf = keep; } break;
        case MO_LAMP: box(T_CONCRETE, V3(o->p.x - 0.13f, y, o->p.z - 0.13f), V3(o->p.x + 0.13f, y + o->s.y, o->p.z + 0.13f), rgb(150, 150, 140), o->solid != 0, ALL);
            addLight(V3(o->p.x, y + o->s.y - 0.5f, o->p.z), V3(1.0f, 0.75f, 0.45f), 12, 1); break;
        } }
}
