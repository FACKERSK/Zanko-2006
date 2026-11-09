// zk_map.h — the map as a file. Data/map.zmap (or the first enabled mod's Data/map.zmap) is loaded over the built-in
// defaults in zk_design.h before the world is generated. The Mod Maker edits the same tables live and saves them here.
// Format (text): "val NAME value" lines, and "arr NAME rows cols" followed by one row of numbers per line.
#include "zk_mapdesc.h"
static int mapGen = 0;   // bumped whenever the map tables change (Mod Maker): caches built from them start over
#define MAP_NARR ((int)(sizeof mapArrs / sizeof mapArrs[0]))
#define MAP_NVAL ((int)(sizeof mapVals / sizeof mapVals[0]))
static void *mapDef[64]; static int mapDefN[64]; static double mapDefV[64]; static bool mapDefOk = false;
static char mapSource[700] = "built-in";
static int mapElem(int type) { return type == 0 ? 4 : type == 1 ? 4 : 1; }
static int mapRows(MapArr *a) { return a->n ? *a->n : a->cap; }
static void mapSnapshot(void) {   // the built-in map, to return to before loading another
    if (mapDefOk) return;
    for (int i = 0; i < MAP_NARR && i < 64; i++) { size_t sz = (size_t)mapArrs[i].cap * mapArrs[i].cols * mapElem(mapArrs[i].type); mapDef[i] = malloc(sz); memcpy(mapDef[i], mapArrs[i].p, sz); mapDefN[i] = mapRows(&mapArrs[i]); }
    for (int i = 0; i < MAP_NVAL && i < 64; i++) mapDefV[i] = mapVals[i].isInt ? *(int *)mapVals[i].p : *(float *)mapVals[i].p;
    mapDefOk = true;
}
static void mapReset(void) {
    mapSnapshot();
    for (int i = 0; i < MAP_NARR && i < 64; i++) { memcpy(mapArrs[i].p, mapDef[i], (size_t)mapArrs[i].cap * mapArrs[i].cols * mapElem(mapArrs[i].type)); if (mapArrs[i].n) *mapArrs[i].n = mapDefN[i]; }
    for (int i = 0; i < MAP_NVAL && i < 64; i++) { if (mapVals[i].isInt) *(int *)mapVals[i].p = (int)mapDefV[i]; else *(float *)mapVals[i].p = (float)mapDefV[i]; }
    snprintf(mapSource, sizeof mapSource, "built-in");
}
static double mapGet(MapArr *a, int r, int c) {
    int k = r * a->cols + c;
    return a->type == 0 ? ((float *)a->p)[k] : a->type == 1 ? ((int *)a->p)[k] : ((unsigned char *)a->p)[k];
}
static void mapSet(MapArr *a, int r, int c, double v) {
    int k = r * a->cols + c;
    if (a->type == 0) ((float *)a->p)[k] = (float)v; else if (a->type == 1) ((int *)a->p)[k] = (int)lround(v); else ((unsigned char *)a->p)[k] = (unsigned char)lround(v);
}
static MapArr *mapArr(const char *name) { for (int i = 0; i < MAP_NARR; i++) if (!strcmp(mapArrs[i].name, name)) return &mapArrs[i]; return NULL; }
static bool mapSave(const char *path) {
    FILE *f = fopen(path, "w"); if (!f) return false;
    fprintf(f, "# Zanko 2006 map v1 (world metres; made by the Mod Maker)\n");
    for (int i = 0; i < MAP_NVAL; i++) { if (mapVals[i].isInt) fprintf(f, "val %s %d\n", mapVals[i].name, *(int *)mapVals[i].p); else fprintf(f, "val %s %.9g\n", mapVals[i].name, *(float *)mapVals[i].p); }
    for (int i = 0; i < MAP_NARR; i++) { MapArr *a = &mapArrs[i]; int rows = mapRows(a);
        fprintf(f, "arr %s %d %d\n", a->name, rows, a->cols);
        for (int r = 0; r < rows; r++) { for (int c = 0; c < a->cols; c++) { double v = mapGet(a, r, c); if (a->type == 0) fprintf(f, c ? " %.9g" : "%.9g", v); else fprintf(f, c ? " %d" : "%d", (int)v); } fputc('\n', f); } }
    fclose(f); return true;
}
static bool mapLoadFile(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) return false;
    mapReset();
    char line[4096], nm[64]; MapArr *cur = NULL; int rows = 0, cols = 0, r = 0;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (cur && r < rows) {   // a row of the open table
            char *p = line;
            for (int c = 0; c < cols; c++) { char *e; double v = strtod(p, &e); if (e == p) break; p = e; if (r < cur->cap && c < cur->cols) mapSet(cur, r, c, v); }
            if (++r >= rows) cur = NULL; continue; }
        if (!strncmp(line, "val ", 4)) { double v; if (sscanf(line + 4, "%63s %lf", nm, &v) == 2) for (int i = 0; i < MAP_NVAL; i++) if (!strcmp(mapVals[i].name, nm)) { if (mapVals[i].isInt) *(int *)mapVals[i].p = (int)v; else *(float *)mapVals[i].p = (float)v; } }
        else if (!strncmp(line, "arr ", 4)) { if (sscanf(line + 4, "%63s %d %d", nm, &rows, &cols) == 3) { cur = mapArr(nm); r = 0;
            if (cur) { if (cur->n) *cur->n = rows < cur->cap ? rows : cur->cap; else if (rows > cur->cap) rows = cur->cap; }
            else { for (int k = 0; k < rows && fgets(line, sizeof line, f); k++) {} }   // an unknown table: skip it
            if (rows <= 0) cur = NULL; } }
    }
    fclose(f);
    snprintf(mapSource, sizeof mapSource, "%s", path);
    return true;
}
static unsigned int mapHash(void) {   // multiplayer: both sides must have generated the same town
    unsigned int h = 2166136261u;
    for (int i = 0; i < MAP_NARR; i++) { MapArr *a = &mapArrs[i]; int rows = mapRows(a); const unsigned char *b = a->p; size_t sz = (size_t)rows * a->cols * mapElem(a->type);
        for (size_t k = 0; k < sz; k++) { h ^= b[k]; h *= 16777619u; } h ^= (unsigned)rows; h *= 16777619u; }
    for (int i = 0; i < MAP_NVAL; i++) { const unsigned char *b = mapVals[i].p; for (int k = 0; k < 4; k++) { h ^= b[k]; h *= 16777619u; } }
    return h;
}
static bool modFind(const char *rel, char *out, int outSize);
static void mapLoad(void) {   // at start: a mod's map wins over Data/map.zmap, which wins over the built-in one
    mapSnapshot();
    char mp[800];
    if (modFind("Data/map.zmap", mp, sizeof mp) && mapLoadFile(mp)) { TraceLog(LOG_INFO, "MAP: %s (from a mod), hash %08x", mp, mapHash()); return; }
    if (FileExists(P("Data/map.zmap")) && mapLoadFile(P("Data/map.zmap"))) { TraceLog(LOG_INFO, "MAP: Data/map.zmap, hash %08x", mapHash()); return; }
    mapReset(); TraceLog(LOG_INFO, "MAP: built-in, hash %08x", mapHash());
}
