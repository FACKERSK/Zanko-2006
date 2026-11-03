// modzip.h — mod discovery, load order (Data/mods.ini) and zip reading, shared by the launcher and the game.
#ifndef MODZIP_H
#define MODZIP_H
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static unsigned int mzRd16(const unsigned char *p) { return p[0] | p[1] << 8; }
static unsigned int mzRd32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (unsigned int)p[3] << 24; }
static bool mzSafeRel(const char *n) {   // reject absolute paths and traversal in archive entries
    if (!n[0] || n[0] == '/' || n[0] == '\\' || strchr(n, ':')) return false;
    if (strstr(n, "..")) return false;
    return true;
}
// Walk a zip's central directory. cb(name, method, csize, usize, data, user) gets each file entry (data = compressed bytes).
// Returns number of entries, or -1 when the file is not a readable zip.
typedef bool (*MzEntryFn)(const char *name, unsigned int method, unsigned int csize, unsigned int usize, const unsigned char *data, void *user);
static int mzWalk(const char *zipPath, MzEntryFn cb, void *user) {
    int size = 0; unsigned char *z = LoadFileData(zipPath, &size);
    if (!z || size < 22) { if (z) UnloadFileData(z); return -1; }
    int eocd = -1;
    for (int i = size - 22; i >= 0 && i >= size - 65557; i--) if (mzRd32(z + i) == 0x06054b50) { eocd = i; break; }
    if (eocd < 0) { UnloadFileData(z); return -1; }
    unsigned int count = mzRd16(z + eocd + 10), cd = mzRd32(z + eocd + 16); int files = 0;
    for (unsigned int e = 0; e < count; e++) {
        if (cd + 46 > (unsigned)size || mzRd32(z + cd) != 0x02014b50) break;
        unsigned int method = mzRd16(z + cd + 10), csize = mzRd32(z + cd + 20), usize = mzRd32(z + cd + 24);
        unsigned int nlen = mzRd16(z + cd + 28), xlen = mzRd16(z + cd + 30), clen = mzRd16(z + cd + 32), lho = mzRd32(z + cd + 42);
        char name[512]; int n = nlen < 511 ? nlen : 511; memcpy(name, z + cd + 46, n); name[n] = 0;
        for (char *c = name; *c; c++) if (*c == '\\') *c = '/';
        cd += 46 + nlen + xlen + clen;
        if (!mzSafeRel(name) || name[strlen(name) - 1] == '/') continue;
        if (lho + 30 > (unsigned)size || mzRd32(z + lho) != 0x04034b50) continue;
        unsigned int dataOff = lho + 30 + mzRd16(z + lho + 26) + mzRd16(z + lho + 28);
        if (dataOff + csize > (unsigned)size) continue;
        files++;
        if (cb && !cb(name, method, csize, usize, z + dataOff, user)) break;
    }
    UnloadFileData(z);
    return files;
}
static unsigned char *mzInflate(unsigned int method, unsigned int csize, unsigned int usize, const unsigned char *data, int *outLen) {
    if (method == 0) { unsigned char *o = (unsigned char *)MemAlloc(csize + 1); memcpy(o, data, csize); o[csize] = 0; *outLen = (int)csize; return o; }
    if (method == 8) { int n = 0; unsigned char *o = DecompressData(data, (int)csize, &n); if (!o || (unsigned)n != usize) { if (o) MemFree(o); return NULL; } *outLen = n; return o; }
    return NULL;
}
typedef struct { const char *outDir; int files; } MzUnzip;
static bool mzUnzipCb(const char *name, unsigned int method, unsigned int csize, unsigned int usize, const unsigned char *data, void *user) {
    MzUnzip *u = (MzUnzip *)user; int len = 0;
    unsigned char *o = mzInflate(method, csize, usize, data, &len);
    if (!o) return true;
    char path[1200]; snprintf(path, sizeof path, "%s/%s", u->outDir, name);
    char dir[1200]; snprintf(dir, sizeof dir, "%s", GetDirectoryPath(path));
    // create nested directories one level at a time
    for (char *p = dir + 1; *p; p++) if (*p == '/') { *p = 0; if (!DirectoryExists(dir)) MakeDirectory(dir); *p = '/'; }
    if (!DirectoryExists(dir)) MakeDirectory(dir);
    SaveFileData(path, o, len); MemFree(o); u->files++;
    return true;
}
static int mzUnzipTo(const char *zipPath, const char *outDir) { MzUnzip u = {outDir, 0}; return mzWalk(zipPath, mzUnzipCb, &u) < 0 ? -1 : u.files; }
typedef struct { const char *want; unsigned char *out; int len; } MzFind;
static bool mzFindCb(const char *name, unsigned int method, unsigned int csize, unsigned int usize, const unsigned char *data, void *user) {
    MzFind *f = (MzFind *)user;
    const char *base = strrchr(name, '/'); base = base ? base + 1 : name;
    if (strcmp(name, f->want) && !(strchr(name, '/') == strrchr(name, '/') && strchr(name, '/') && !strcmp(base, f->want))) return true;   // allow one wrapping folder
    f->out = mzInflate(method, csize, usize, data, &f->len);
    return false;
}
static char *mzReadText(const char *zipPath, const char *entry) {   // MemAlloc'd, NUL-terminated
    MzFind f = {entry, NULL, 0}; mzWalk(zipPath, mzFindCb, &f);
    if (f.out && f.len >= 0) { unsigned char *t = (unsigned char *)MemRealloc(f.out, f.len + 1); t[f.len] = 0; return (char *)t; }
    return NULL;
}

// ---------------------------------------------------------------- mod list + load order
#define MZ_MAX 48
#define MZ_MAXENT 400
typedef struct {
    char file[128];          // name inside Mods/ (zip file or folder)
    char path[700];
    bool isZip, enabled, valid;
    char name[64], author[64], version[24], desc[256];
    int nfiles; long bytes;
    int nent; char (*ent)[96];   // relative paths, lower-case (only when entries were requested)
} ModEntry;
static void mzIniField(const char *txt, const char *key, char *out, int n) {
    char k[40]; snprintf(k, sizeof k, "%s=", key);
    for (const char *p = txt; p && *p; ) {
        const char *e = strchr(p, '\n'); int len = e ? (int)(e - p) : (int)strlen(p);
        if (!strncmp(p, k, strlen(k))) { int l = len - (int)strlen(k); if (l > n - 1) l = n - 1; memcpy(out, p + strlen(k), l); out[l] = 0; char *cr = strchr(out, '\r'); if (cr) *cr = 0; return; }
        p = e ? e + 1 : NULL;
    }
}
typedef struct { ModEntry *m; } MzScan;
static bool mzScanCb(const char *name, unsigned int method, unsigned int csize, unsigned int usize, const unsigned char *data, void *user) {
    (void)method; (void)csize; (void)data;
    ModEntry *m = ((MzScan *)user)->m;
    m->nfiles++; m->bytes += usize;
    if (m->ent && m->nent < MZ_MAXENT) { int i = 0; for (; name[i] && i < 95; i++) m->ent[m->nent][i] = (char)tolower((unsigned char)name[i]); m->ent[m->nent][i] = 0; m->nent++; }
    return true;
}
static void mzReadMeta(ModEntry *m, bool wantEntries) {
    snprintf(m->name, sizeof m->name, "%s", GetFileNameWithoutExt(m->file));
    m->author[0] = m->version[0] = m->desc[0] = 0; m->nfiles = 0; m->bytes = 0; m->nent = 0;
    if (wantEntries && !m->ent) m->ent = (char (*)[96])MemAlloc(MZ_MAXENT * 96);
    char *ini = NULL;
    if (m->isZip) {
        MzScan s = {m}; m->valid = mzWalk(m->path, mzScanCb, &s) >= 0;
        if (m->valid) ini = mzReadText(m->path, "mod.ini");
    } else {
        m->valid = true;
        FilePathList fl = LoadDirectoryFilesEx(m->path, NULL, true);
        size_t base = strlen(m->path) + 1;
        for (unsigned int i = 0; i < fl.count; i++) {
            m->nfiles++; m->bytes += GetFileLength(fl.paths[i]);
            if (m->ent && m->nent < MZ_MAXENT && strlen(fl.paths[i]) > base) { const char *r = fl.paths[i] + base; int k = 0; for (; r[k] && k < 95; k++) m->ent[m->nent][k] = (char)(r[k] == '\\' ? '/' : tolower((unsigned char)r[k])); m->ent[m->nent][k] = 0; m->nent++; }
        }
        UnloadDirectoryFiles(fl);
        char p[760]; snprintf(p, sizeof p, "%s/mod.ini", m->path);
        if (FileExists(p)) { char *t = LoadFileText(p); if (t) { int n = (int)strlen(t); ini = (char *)MemAlloc(n + 1); memcpy(ini, t, n + 1); UnloadFileText(t); } }
    }
    if (ini) {
        mzIniField(ini, "name", m->name, sizeof m->name); mzIniField(ini, "author", m->author, sizeof m->author);
        mzIniField(ini, "version", m->version, sizeof m->version); mzIniField(ini, "description", m->desc, sizeof m->desc);
        MemFree(ini);
    }
}
// modsDir / iniPath are absolute. Fills list in load-order (top = highest priority). Returns count.
static int modsScan(const char *modsDir, const char *iniPath, ModEntry *list, int max, bool wantEntries) {
    for (int i = 0; i < max; i++) { if (list[i].ent) MemFree(list[i].ent); }
    memset(list, 0, sizeof(ModEntry) * max);
    int n = 0;
    if (!DirectoryExists(modsDir)) return 0;
    FilePathList fl = LoadDirectoryFiles(modsDir);
    for (unsigned int a = 0; a < fl.count; a++) for (unsigned int b = a + 1; b < fl.count; b++) if (strcmp(fl.paths[a], fl.paths[b]) > 0) { char *t = fl.paths[a]; fl.paths[a] = fl.paths[b]; fl.paths[b] = t; }
    ModEntry found[MZ_MAX]; int nf = 0; memset(found, 0, sizeof found);
    for (unsigned int i = 0; i < fl.count && nf < MZ_MAX; i++) {
        const char *f = fl.paths[i], *fn = GetFileName(f);
        if (fn[0] == '.' || !strcmp(fn, "README.txt")) continue;
        bool zip = IsPathFile(f) && IsFileExtension(f, ".zip");
        if (IsPathFile(f) && !zip) continue;
        ModEntry *m = &found[nf++];
        snprintf(m->file, sizeof m->file, "%s", fn); snprintf(m->path, sizeof m->path, "%s", f); m->isZip = zip; m->enabled = true;
    }
    UnloadDirectoryFiles(fl);
    bool used[MZ_MAX] = {0};
    char *ini = FileExists(iniPath) ? LoadFileText(iniPath) : NULL;
    if (ini) {
        for (char *p = ini; p && *p; ) {
            char *e = strchr(p, '\n'); if (e) *e = 0;
            char fileN[128] = ""; int en = 1;
            if (!strncmp(p, "mod=", 4)) {
                char *bar = strrchr(p + 4, '|');
                if (bar) { *bar = 0; en = atoi(bar + 1); }
                snprintf(fileN, sizeof fileN, "%s", p + 4); char *cr = strchr(fileN, '\r'); if (cr) *cr = 0;
                for (int i = 0; i < nf; i++) if (!used[i] && !strcmp(found[i].file, fileN) && n < max) { list[n] = found[i]; list[n].enabled = en != 0; used[i] = true; n++; break; }
            }
            p = e ? e + 1 : NULL;
        }
        UnloadFileText(ini);
    }
    for (int i = 0; i < nf && n < max; i++) if (!used[i]) list[n++] = found[i];   // new mods: enabled, lowest priority
    for (int i = 0; i < n; i++) { list[i].ent = NULL; mzReadMeta(&list[i], wantEntries); }
    return n;
}
static void modsSave(const char *iniPath, ModEntry *list, int n) {
    FILE *f = fopen(iniPath, "w"); if (!f) return;
    fprintf(f, "# Zanko 2006 mod load order, written by the launcher's Mod Manager.\n");
    fprintf(f, "# Top = highest priority: when two mods replace the same file, the higher one wins.\n");
    fprintf(f, "# mod=<zip or folder name inside Mods>|<1 enabled, 0 disabled>\n");
    for (int i = 0; i < n; i++) fprintf(f, "mod=%s|%d\n", list[i].file, list[i].enabled ? 1 : 0);
    fclose(f);
}
#endif
