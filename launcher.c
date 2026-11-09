// Zankō 2006 — Launcher: local accounts, settings, update check, play
#include "raylib.h"
#include "aero.h"
#include "silent.h"
#include "modzip.h"
#include <time.h>
#include <stdint.h>
#include "raymath.h"

#if defined(_WIN32)
typedef void *HANDLE;
__declspec(dllimport) long __stdcall URLDownloadToFileA(void *, const char *, const char *, unsigned long, void *);
__declspec(dllimport) HANDLE __stdcall CreateThread(void *, size_t, unsigned long(__stdcall *)(void *), void *, unsigned long, unsigned long *);
__declspec(dllimport) int __stdcall MoveFileExA(const char *, const char *, unsigned long);
__declspec(dllimport) void *__stdcall ShellExecuteA(void *, const char *, const char *, const char *, const char *, int);
__declspec(dllimport) int __stdcall DeleteFileA(const char *);
__declspec(dllimport) unsigned long __stdcall GetTempPathA(unsigned long, char *);
typedef struct { unsigned long len, load; unsigned long long totalPhys, availPhys, totalPage, availPage, totalVirt, availVirt, availExt; } MemStatEx;
__declspec(dllimport) int __stdcall GlobalMemoryStatusEx(MemStatEx *);
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
static int systemRamMB(void) {
#if defined(_WIN32)
    MemStatEx m; memset(&m, 0, sizeof m); m.len = sizeof m;
    if (GlobalMemoryStatusEx(&m)) return (int)(m.totalPhys / (1024ull * 1024ull));
    return 0;
#else
    long pg = sysconf(_SC_PHYS_PAGES), ps = sysconf(_SC_PAGE_SIZE);
    return pg > 0 && ps > 0 ? (int)((long long)pg * ps / (1024ll * 1024ll)) : 0;
#endif
}

// ---------------------------------------------------------------- SHA-256
typedef struct { uint32_t s[8]; uint64_t len; uint8_t buf[64]; int n; } Sha;
static const uint32_t K[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d,
    0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
static void shaBlock(Sha *h, const uint8_t *p) {
    uint32_t w[64], a, b, c, d, e, f, g, k, t1, t2;
    for (int i = 0; i < 16; i++) w[i] = (uint32_t)p[i * 4] << 24 | (uint32_t)p[i * 4 + 1] << 16 | (uint32_t)p[i * 4 + 2] << 8 | p[i * 4 + 3];
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^ (w[i - 15] >> 3), s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    a = h->s[0]; b = h->s[1]; c = h->s[2]; d = h->s[3]; e = h->s[4]; f = h->s[5]; g = h->s[6]; k = h->s[7];
    for (int i = 0; i < 64; i++) {
        t1 = k + (ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25)) + ((e & f) ^ (~e & g)) + K[i] + w[i];
        t2 = (ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        k = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    h->s[0] += a; h->s[1] += b; h->s[2] += c; h->s[3] += d; h->s[4] += e; h->s[5] += f; h->s[6] += g; h->s[7] += k;
}
static void shaInit(Sha *h) {
    uint32_t iv[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    memcpy(h->s, iv, sizeof iv); h->len = 0; h->n = 0;
}
static void shaUpdate(Sha *h, const void *data, size_t len) {
    const uint8_t *p = data;
    for (size_t i = 0; i < len; i++) { h->buf[h->n++] = p[i]; h->len++; if (h->n == 64) { shaBlock(h, h->buf); h->n = 0; } }
}
static void shaFinal(Sha *h, uint8_t out[32]) {
    uint64_t bits = h->len * 8; uint8_t pad = 0x80, z = 0;
    shaUpdate(h, &pad, 1);
    while (h->n != 56) shaUpdate(h, &z, 1);
    uint8_t L[8]; for (int i = 0; i < 8; i++) L[i] = (uint8_t)(bits >> (56 - i * 8));
    shaUpdate(h, L, 8);
    for (int i = 0; i < 8; i++) { out[i * 4] = h->s[i] >> 24; out[i * 4 + 1] = h->s[i] >> 16; out[i * 4 + 2] = h->s[i] >> 8; out[i * 4 + 3] = h->s[i]; }
}
static void hashPassword(const char *salt, const char *pw, char hex[65]) {   // iterated salted SHA-256 (20000 rounds)
    uint8_t d[32]; Sha h;
    shaInit(&h); shaUpdate(&h, salt, strlen(salt)); shaUpdate(&h, pw, strlen(pw)); shaFinal(&h, d);
    for (int i = 0; i < 20000; i++) { shaInit(&h); shaUpdate(&h, d, 32); shaUpdate(&h, salt, strlen(salt)); shaFinal(&h, d); }
    for (int i = 0; i < 32; i++) sprintf(hex + i * 2, "%02x", d[i]);
    hex[64] = 0;
}

// ---------------------------------------------------------------- launcher state
enum { L_BOOT, L_LOGIN, L_MAIN, L_SETTINGS, L_MODS };
static int lstate = L_BOOT;
static int ramMB = 0;                 // 0 = automatic (no working-set request)
static char launchArgs[256] = "";    // additional free-form parameters
static int pWindowed = 0, pNewGame = 0, pNoMods = 0, pLog = 0, pNoVsync = 0, pFps = 0, pRender = 0; static char pSeed[24] = "";
static char status[160] = "";
static char updStatus[200] = "";
#define DEFAULT_UPDATE_URL "https://raw.githubusercontent.com/FACKERSK/Zanko-2006/main/update/{channel}.txt"
static char updateUrl[512] = DEFAULT_UPDATE_URL;
static char instUrl[600] = "", newsUrl[600] = "";   // from the manifest: the installer of the new version, and the news feed
static volatile int updQuit = 0; static int autoInstallTried = 0;
// ---- version selector: every released version can be played; older ones are downloaded into Versions/<ver>/ with their own data and saves
#define VMAX 48
typedef struct { char ver[24], channel[12], url[400], date[16], title[80]; } VerEntry;
static VerEntry vers[VMAX]; static int nVers = 0;
static char verSel[24] = "";          // "" = the installed (latest) version
static volatile int verBusy = 0; static char verStatus[200] = ""; static char verPending[24] = ""; static volatile int verLaunch = 0;
static bool verPopup = false; static Rectangle verPopR; static volatile int versReload = 0; static bool verHoldClick = false;
static char lastUser[32] = "";
static int closeOnPlay = 1;
static volatile int updBusy = 0, updAvailable = 0, updDone = 0;
static char remoteVer[32] = "";
static char localVer[32] = GAME_VERSION;

static void launcherIniLoad(void) {
    FILE *f = fopen(P("Data/launcher.ini"), "r");
    if (!f) return;
    char line[700], k[64], v[600];
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == ';') continue;
        v[0] = 0;
        if (sscanf(line, "%63[^=]=%599[^\r\n]", k, v) < 1) continue;
        if (!strcmp(k, "update_url")) { if (v[0]) { strncpy(updateUrl, v, 511); updateUrl[511] = 0; } }   // empty keeps the GitHub default
        else if (!strcmp(k, "lastUser")) { strncpy(lastUser, v, 31); lastUser[31] = 0; }
        else if (!strcmp(k, "closeOnPlay")) closeOnPlay = atoi(v);
        else if (!strcmp(k, "ram_mb")) ramMB = atoi(v);
        else if (!strcmp(k, "p_windowed")) pWindowed = atoi(v);
        else if (!strcmp(k, "p_newgame")) pNewGame = atoi(v);
        else if (!strcmp(k, "p_nomods")) pNoMods = atoi(v);
        else if (!strcmp(k, "p_log")) pLog = atoi(v);
        else if (!strcmp(k, "p_novsync")) pNoVsync = atoi(v);
        else if (!strcmp(k, "p_fps")) pFps = atoi(v);
        else if (!strcmp(k, "p_render")) pRender = atoi(v);
        else if (!strcmp(k, "p_seed")) snprintf(pSeed, sizeof pSeed, "%s", v);
        else if (!strcmp(k, "launch_args")) { snprintf(launchArgs, sizeof launchArgs, "%s", v); }
        else if (!strcmp(k, "version")) { snprintf(verSel, sizeof verSel, "%s", v); }
    }
    fclose(f);
}
static void launcherIniSave(void) {
    ensureDir("Data");
    FILE *f = fopen(P("Data/launcher.ini"), "w");
    if (!f) return;
    fprintf(f, "# Zanko 2006 launcher settings\n");
    fprintf(f, "# update_url: URL of the plain-text update manifest (empty = the GitHub repository FACKERSK/Zanko-2006).\n");
    fprintf(f, "#   Manifest format:\n#     version=0.6.0\n#     installer=https://github.com/.../Zanko2006_Setup_v0.6.0.exe   (preferred: run silently, then the launcher restarts)\n#     news=https://raw.githubusercontent.com/.../news.txt\n#     file=Root/Zanko.exe|https://example.com/Zanko.exe          (fallback: single files)\n");
    fprintf(f, "#   {channel} in the URL is replaced with the selected update channel (stable/beta).\n");
    fprintf(f, "# ram_mb: memory reserved for the game process in MB (0 = automatic). Passed as --ram.\n");
    fprintf(f, "# launch_args: extra startup arguments passed to Zanko.exe.\n");
    fprintf(f, "update_url=%s\nlastUser=%s\ncloseOnPlay=%d\nram_mb=%d\nlaunch_args=%s\n", updateUrl, lastUser, closeOnPlay, ramMB, launchArgs);
    fprintf(f, "# version: the game version PLAY starts (empty = the installed, newest version)\nversion=%s\n", verSel);
    fprintf(f, "# Parameters tab\np_windowed=%d\np_newgame=%d\np_nomods=%d\np_log=%d\np_novsync=%d\np_fps=%d\np_render=%d\np_seed=%s\n", pWindowed, pNewGame, pNoMods, pLog, pNoVsync, pFps, pRender, pSeed);
    fclose(f);
}
static void readLocalVersion(void) {
    char *t = LoadFileText(P("Data/version.txt"));
    if (t) { sscanf(t, "%31s", localVer); UnloadFileText(t); }
}
static int verCmp(const char *a, const char *b) {
    int x[3] = {0}, y[3] = {0};
    sscanf(a, "%d.%d.%d", &x[0], &x[1], &x[2]); sscanf(b, "%d.%d.%d", &y[0], &y[1], &y[2]);
    for (int i = 0; i < 3; i++) if (x[i] != y[i]) return x[i] < y[i] ? -1 : 1;
    return 0;
}
static void buildUrl(char *out, size_t n) {
    const char *ph = strstr(updateUrl, "{channel}");
    if (ph) snprintf(out, n, "%.*s%s%s", (int)(ph - updateUrl), updateUrl, cfg.channel, ph + 9);
    else snprintf(out, n, "%s", updateUrl);
}

#if defined(_WIN32)
static int download(const char *url, const char *path) { return URLDownloadToFileA(NULL, url, path, 0, NULL) == 0; }
#else
static int download(const char *url, const char *path) { char cmd[1400]; snprintf(cmd, sizeof cmd, "curl -fsSL \"%s\" -o \"%s\"", url, path); return system(cmd) == 0; }
#endif

static int verCmp(const char *a, const char *b);
static void versLoad(void) {   // Data/versions.txt: one line per release, newest first:  version|channel|zip url|date|title
    nVers = 0; char *t = LoadFileText(P("Data/versions.txt")); if (!t) return;
    char *line = strtok(t, "\r\n");
    while (line && nVers < VMAX) {
        if (line[0] && line[0] != '#') { VerEntry *v = &vers[nVers]; memset(v, 0, sizeof *v);
            if (sscanf(line, "%23[^|]|%11[^|]|%399[^|]|%15[^|]|%79[^\n]", v->ver, v->channel, v->url, v->date, v->title) >= 3) nVers++; }
        line = strtok(NULL, "\r\n"); }
    UnloadFileText(t);
    for (int i = 0; i < nVers; i++) for (int j = i + 1; j < nVers; j++) if (verCmp(vers[j].ver, vers[i].ver) > 0) { VerEntry x = vers[i]; vers[i] = vers[j]; vers[j] = x; }
}
static void versFetch(void) {   // the list sits next to the update manifest: .../update/versions.txt
    char base[600]; snprintf(base, sizeof base, "%s", updateUrl); char *sl = strrchr(base, '/'); if (!sl) return; sl[1] = 0;
    char url[700], tmp[900]; snprintf(url, sizeof url, "%sversions.txt?v=%ld", base, (long)time(NULL)); snprintf(tmp, sizeof tmp, "%s.new", P("Data/versions.txt"));
    if (download(url, tmp)) { remove(P("Data/versions.txt")); rename(tmp, P("Data/versions.txt")); }
}
static const char *verDir(char *b, const char *v) { snprintf(b, 300, "Versions/%s", v); return b; }   // caller buffer: used from the download thread too
static bool verInstalled(const char *v) {
    if (!v[0] || !strcmp(v, localVer)) return true;
    char vb[300], rel[400]; snprintf(rel, sizeof rel, "%s/Zanko2006/Root/%s", verDir(vb, v),
#if defined(_WIN32)
        "Zanko.exe"
#else
        "zanko_linux"
#endif
    ); return FileExists(P(rel));
}
#if defined(_WIN32)
static unsigned long __stdcall checkThread(void *arg)
#else
static void *checkThread(void *arg)
#endif
{
    (void)arg;
    char url0[600], url[640]; buildUrl(url0, sizeof url0);
    snprintf(url, sizeof url, "%s%sv=%ld", url0, strchr(url0, '?') ? "&" : "?", (long)time(NULL));   // skip the system download cache: always the live manifest
    const char *mf = P("Data/update_manifest.txt");
    if (!download(url, mf)) { snprintf(updStatus, sizeof updStatus, "Update check failed (could not reach update source)."); updBusy = 0; return 0; }
    versFetch(); versReload = 1;
    char *t = LoadFileText(mf);
    remoteVer[0] = 0;
    instUrl[0] = newsUrl[0] = 0;
    if (t) { char *v = strstr(t, "version="); if (v) sscanf(v + 8, "%31s", remoteVer);
        char *iu = strstr(t, "installer="); if (iu) sscanf(iu + 10, "%599s", instUrl);
        char *nu = strstr(t, "news="); if (nu) sscanf(nu + 5, "%599s", newsUrl);
        UnloadFileText(t); }
    if (newsUrl[0]) { char tmp[900]; snprintf(tmp, sizeof tmp, "%s.new", P("Data/news.txt")); if (download(newsUrl, tmp)) { remove(P("Data/news.txt")); rename(tmp, P("Data/news.txt")); } }
    if (!remoteVer[0]) snprintf(updStatus, sizeof updStatus, "Update manifest unreadable.");
    else if (verCmp(remoteVer, localVer) > 0) { updAvailable = 1; snprintf(updStatus, sizeof updStatus, "Update available: v%s (installed v%s)", remoteVer, localVer); }
    else snprintf(updStatus, sizeof updStatus, "Up to date (v%s).", localVer);
    updBusy = 0;
    return 0;
}
#if defined(_WIN32)
static unsigned long __stdcall installThread(void *arg)
#else
static void *installThread(void *arg)
#endif
{
    (void)arg;
    if (instUrl[0]) {   // the whole new version as an installer: download, run it silently, close so it can replace the launcher
#if defined(_WIN32)
        char tmpDir[600]; unsigned long n = GetTempPathA(sizeof tmpDir, tmpDir); if (!n) snprintf(tmpDir, sizeof tmpDir, ".\\");
        char path[700]; snprintf(path, sizeof path, "%sZanko2006_Setup_v%s.exe", tmpDir, remoteVer);
        snprintf(updStatus, sizeof updStatus, "Downloading v%s ...", remoteVer);
        if (!download(instUrl, path)) { snprintf(updStatus, sizeof updStatus, "Update download failed. Check the connection and try again."); updBusy = 0; return 0; }
        snprintf(updStatus, sizeof updStatus, "Installing v%s. The launcher will reopen.", remoteVer);
        if ((intptr_t)ShellExecuteA(NULL, "open", path, "/S /UPDATE", NULL, 1) <= 32) { snprintf(updStatus, sizeof updStatus, "Could not start the installer (%s).", path); updBusy = 0; return 0; }
        updQuit = 1; updBusy = 0; return 0;
#else
        snprintf(updStatus, sizeof updStatus, "v%s is out: the installer is for Windows (%s).", remoteVer, instUrl); updBusy = 0; return 0;
#endif
    }
    char *t = LoadFileText(P("Data/update_manifest.txt"));
    if (!t) { snprintf(updStatus, sizeof updStatus, "Manifest missing."); updBusy = 0; return 0; }
    int ok = 1, count = 0, selfUpdated = 0;
    char *line = strtok(t, "\r\n");
    while (line) {
        if (!strncmp(line, "file=", 5)) {
            char rel[300], url[700]; char *bar = strchr(line + 5, '|');
            if (bar) {
                snprintf(rel, sizeof rel, "%.*s", (int)(bar - (line + 5)), line + 5); snprintf(url, sizeof url, "%s", bar + 1);
                if (strstr(rel, "..")) { line = strtok(NULL, "\r\n"); continue; }   // never write outside the game folder
                char dst[900], tmp[920]; snprintf(dst, sizeof dst, "%s", P(rel)); snprintf(tmp, sizeof tmp, "%s.new", dst);
                snprintf(updStatus, sizeof updStatus, "Downloading %s ...", rel);
                if (!download(url, tmp)) { ok = 0; break; }
#if defined(_WIN32)
                if (strstr(rel, "Launcher.exe")) { char old[940]; snprintf(old, sizeof old, "%s.old", dst); DeleteFileA(old); MoveFileExA(dst, old, 1); selfUpdated = 1; }
                if (!MoveFileExA(tmp, dst, 1)) { ok = 0; break; }
#else
                if (rename(tmp, dst) != 0) { ok = 0; break; }
#endif
                count++;
            }
        }
        line = strtok(NULL, "\r\n");
    }
    UnloadFileText(t);
    if (ok) {
        FILE *f = fopen(P("Data/version.txt"), "w"); if (f) { fprintf(f, "%s\n", remoteVer); fclose(f); }
        snprintf(localVer, sizeof localVer, "%s", remoteVer);
        snprintf(updStatus, sizeof updStatus, selfUpdated ? "Updated %d file(s) to v%s. Restart the launcher." : "Updated %d file(s) to v%s.", count, remoteVer);
        updAvailable = 0; updDone = 1;
    } else snprintf(updStatus, sizeof updStatus, "Update failed. Game files were left unchanged where possible.");
    updBusy = 0;
    return 0;
}
#if !defined(_WIN32)
#include <pthread.h>
#endif
#if defined(_WIN32)
static unsigned long __stdcall verThread(void *arg)
#else
static void *verThread(void *arg)
#endif
{
    (void)arg;
    VerEntry *v = NULL; for (int i = 0; i < nVers; i++) if (!strcmp(vers[i].ver, verPending)) v = &vers[i];
    if (!v) { snprintf(verStatus, sizeof verStatus, "v%s is not in the version list.", verPending); verBusy = 0; return 0; }
    ensureDir("Versions");
    char vb[300], zip[700], dir[700]; snprintf(zip, sizeof zip, "%s.zip", P(verDir(vb, v->ver))); snprintf(dir, sizeof dir, "%s", P(verDir(vb, v->ver)));
    snprintf(verStatus, sizeof verStatus, "Downloading v%s ...", v->ver);
    if (!download(v->url, zip)) { snprintf(verStatus, sizeof verStatus, "Could not download v%s.", v->ver); remove(zip); verBusy = 0; return 0; }
    snprintf(verStatus, sizeof verStatus, "Unpacking v%s ...", v->ver);
    if (!DirectoryExists(dir)) MakeDirectory(dir);
    int n = mzUnzipTo(zip, dir); remove(zip);
#if !defined(_WIN32)
    { char ex[800]; snprintf(ex, sizeof ex, "%s/Zanko2006/Root/zanko_linux", dir); chmod(ex, 0755); }
#endif
    if (n <= 0 || !verInstalled(v->ver)) { snprintf(verStatus, sizeof verStatus, "v%s could not be unpacked.", v->ver); verBusy = 0; return 0; }
    snprintf(verStatus, sizeof verStatus, "v%s installed.", v->ver);
    verLaunch = 1; verBusy = 0; return 0;
}
static void startVerThread(void) {
    if (verBusy) return; verBusy = 1;
#if defined(_WIN32)
    CreateThread(NULL, 0, verThread, NULL, 0, NULL);
#else
    pthread_t th; pthread_create(&th, NULL, verThread, NULL); pthread_detach(th);
#endif
}
static void startThread(int install) {
    if (updBusy) return;
    updBusy = 1;
#if defined(_WIN32)
    CreateThread(NULL, 0, install ? installThread : checkThread, NULL, 0, NULL);
#else
    pthread_t th; pthread_create(&th, NULL, install ? installThread : checkThread, NULL); pthread_detach(th);
#endif
}
static void checkUpdates(void) {
    if (!updateUrl[0]) { snprintf(updStatus, sizeof updStatus, "No update source configured (Data/launcher.ini: update_url)."); return; }
    snprintf(updStatus, sizeof updStatus, "Checking for updates ...");
    startThread(0);
}

// ---------------------------------------------------------------- accounts
static bool validName(const char *n) {
    int l = (int)strlen(n);
    if (l < 3 || l > 16) return false;
    for (int i = 0; i < l; i++) { char c = n[i]; if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) return false; }
    return strcmp(n, "Guest") != 0;
}
static bool findAccount(const char *name, char *salt, char *hash) {
    FILE *f = fopen(P("Data/Userdata/accounts.dat"), "r");
    if (!f) return false;
    char line[256], n[40], s[40], h[80];
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "%39s %39s %79s", n, s, h) == 3 && !strcmp(n, name)) { strcpy(salt, s); strcpy(hash, h); fclose(f); return true; }
    }
    fclose(f);
    return false;
}
static bool createAccount(const char *name, const char *pw) {
    char s[40], h[80];
    if (findAccount(name, s, h)) { snprintf(status, sizeof status, "That username is taken."); return false; }
    char salt[33]; for (int i = 0; i < 16; i++) sprintf(salt + i * 2, "%02x", GetRandomValue(0, 255) ^ (int)((time(NULL) >> (i % 4 * 8)) & 0xFF));
    char hex[65]; hashPassword(salt, pw, hex);
    ensureDir("Data"); ensureDir("Data/Userdata");
    FILE *f = fopen(P("Data/Userdata/accounts.dat"), "a");
    if (!f) { snprintf(status, sizeof status, "Could not write account file."); return false; }
    fprintf(f, "%s %s %s\n", name, salt, hex);
    fclose(f);
    profLoad(name); profSave();
    return true;
}
static bool login(const char *name, const char *pw) {
    char s[40], h[80];
    if (!findAccount(name, s, h)) { snprintf(status, sizeof status, "Unknown username."); return false; }
    char hex[65]; hashPassword(s, pw, hex);
    if (strcmp(hex, h) != 0) { snprintf(status, sizeof status, "Wrong password."); return false; }
    return true;
}
static void enterMain(const char *name) {
    profLoad(name);
    snprintf(lastUser, sizeof lastUser, "%s", strcmp(name, "Guest") ? name : "");
    launcherIniSave();
    lstate = L_MAIN; status[0] = 0;
}
static void buildArgs(char *args, int sz) {   // full command line from the Parameters tab
    int n = snprintf(args, sz, "--user %s", prof.name[0] ? prof.name : "Guest");
    if (ramMB >= 256) n += snprintf(args + n, sz - n, " --ram %d", ramMB);
    if (pWindowed) n += snprintf(args + n, sz - n, " --windowed");
    if (pNewGame) n += snprintf(args + n, sz - n, " --newgame");
    if (pNoMods) n += snprintf(args + n, sz - n, " --nomods");
    if (pLog) n += snprintf(args + n, sz - n, " --log");
    if (pNoVsync) n += snprintf(args + n, sz - n, " --novsync");
    if (pFps > 0) n += snprintf(args + n, sz - n, " --fps %d", pFps);
    if (pRender > 0) n += snprintf(args + n, sz - n, " --render %d", pRender);
    if (pSeed[0]) n += snprintf(args + n, sz - n, " --seed %s", pSeed);
    if (launchArgs[0] && n < sz - 2) snprintf(args + n, sz - n, " %s", launchArgs);
}
static void play(void) {
    cfgSave();
    char args[512]; buildArgs(args, sizeof args);
    char root[400] = "Root";
    if (verSel[0] && strcmp(verSel, localVer)) {   // an older version: its own folder; download it first when it is not there yet
        if (!verInstalled(verSel)) { snprintf(verPending, sizeof verPending, "%s", verSel); startVerThread(); snprintf(status, sizeof status, "Getting v%s ...", verSel); return; }
        char vb[300]; snprintf(root, sizeof root, "%s/Zanko2006/Root", verDir(vb, verSel));
    }
    char rexe[460];
#if defined(_WIN32)
    snprintf(rexe, sizeof rexe, "%s/Zanko.exe", root);
    const char *exe = P(rexe);
    if (!FileExists(exe)) { snprintf(status, sizeof status, "%s not found.", rexe); return; }
    ShellExecuteA(NULL, "open", exe, args, P(root), 1);
#else
    snprintf(rexe, sizeof rexe, "%s/zanko_linux", root);
    char cmd[1400]; snprintf(cmd, sizeof cmd, "\"%s\" %s &", P(rexe), args);
    if (system(cmd) != 0) { snprintf(status, sizeof status, "Could not start the game."); return; }
#endif
    snprintf(status, sizeof status, "Game started.");
}



// ---------------------------------------------------------------- UI: desktop-launcher layout (sidebar, content, bottom bar) in the Silent Aero skin
static TextBox tbUser, tbPass, tbArgs, tbSeed;
static const int resList[][2] = {{1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}};
static float bootT = 0;
enum { TAB_PLAY, TAB_MODS, TAB_SERVERS, TAB_PARAMS, TAB_OPTIONS };
static int tab = TAB_PLAY;
// ---------------------------------------------------------------- Mod Manager
static ModEntry mods[MZ_MAX]; static int nModsL = 0, modSel = 0, modScroll = 0; static bool modsDirty = false, modsLoaded = false;
static char modMsg[200] = "";
static void modsReload(void) {
    char md[700], mi[700]; snprintf(md, sizeof md, "%s", P("Mods")); snprintf(mi, sizeof mi, "%s", P("Data/mods.ini"));
    nModsL = modsScan(md, mi, mods, MZ_MAX, true); modsLoaded = true; modsDirty = false;
    if (modSel >= nModsL) modSel = nModsL - 1; if (modSel < 0) modSel = 0;
}
static void modsWrite(void) { char mi[700]; snprintf(mi, sizeof mi, "%s", P("Data/mods.ini")); modsSave(mi, mods, nModsL); modsDirty = false; }
static void openFolder(const char *path) {
#if defined(_WIN32)
    ShellExecuteA(NULL, "open", path, NULL, NULL, 1);
#else
    char cmd[900]; snprintf(cmd, sizeof cmd, "xdg-open \"%s\" >/dev/null 2>&1 &", path); if (system(cmd)) {}
#endif
}
static bool isMeta(const char *e) { const char *b = strrchr(e, '/'); b = b ? b + 1 : e; return !strcmp(b, "mod.ini") || !strcmp(b, "readme.txt"); }
static int sharedFiles(ModEntry *a, ModEntry *b) {
    int n = 0;
    for (int i = 0; i < a->nent; i++) { if (isMeta(a->ent[i])) continue; for (int j = 0; j < b->nent; j++) if (!strcmp(a->ent[i], b->ent[j])) { n++; break; } }
    return n;
}
static void wrapText(const char *t, float x, float y, float w, float size, Color c, int maxLines) {
    char line[256] = ""; int lines = 0; const char *p = t;
    while (*p && lines < maxLines) {
        const char *e = p; while (*e && *e != ' ') e++;
        char test[300]; snprintf(test, sizeof test, "%s%s%.*s", line, line[0] ? " " : "", (int)(e - p), p);
        if (aMeasure(test, size, false).x > w && line[0]) { aText(line, x, y, size, c, false); y += size * 1.4f * aU; lines++; line[0] = 0; continue; }
        snprintf(line, sizeof line, "%s", test);
        p = *e ? e + 1 : e;
    }
    if (line[0] && lines < maxLines) aText(line, x, y, size, c, false);
}
static void installDropped(void) {
    if (!IsFileDropped()) return;
    FilePathList fl = LoadDroppedFiles(); int ok = 0, bad = 0;
    for (unsigned int i = 0; i < fl.count; i++) {
        const char *f = fl.paths[i];
        if (!IsFileExtension(f, ".zip")) { bad++; continue; }
        if (mzWalk(f, NULL, NULL) < 0) { bad++; continue; }
        int sz = 0; unsigned char *d = LoadFileData(f, &sz);
        if (d) { char dst[800]; snprintf(dst, sizeof dst, "%s/%s", P("Mods"), GetFileName(f)); if (SaveFileData(dst, d, sz)) ok++; else bad++; UnloadFileData(d); }
    }
    UnloadDroppedFiles(fl);
    if (modsDirty) modsWrite();
    modsReload();
    snprintf(modMsg, sizeof modMsg, "Installed %d mod(s)%s.", ok, bad ? TextFormat(", skipped %d file(s) that are not readable .zip archives", bad) : "");
}
// ---- small pieces
static void glassStrip(Rectangle r, int alpha) {   // flat glass bar (sidebar, bottom bar)
    float s = aU;
    aGrad(r, 0, (Color){62, 78, 81, (unsigned char)alpha}, (Color){22, 30, 31, (unsigned char)alpha}, 5);
    aScissor(r.x, r.y, r.width, fminf(r.height * 0.5f, 60 * s)); DrawRectangle((int)r.x, (int)r.y, (int)r.width, (int)fminf(r.height * 0.5f, 60 * s), (Color){255, 255, 255, 10}); aScissorEnd();
}
static void icon(int k, float cx, float cy, float r, Color c) {
    float s = aU;
    if (k == TAB_PLAY) DrawTriangle((Vector2){cx - r * 0.6f, cy - r}, (Vector2){cx - r * 0.6f, cy + r}, (Vector2){cx + r, cy}, c);
    else if (k == TAB_MODS) { for (int i = 0; i < 3; i++) DrawRectangleLinesEx((Rectangle){cx - r + i * 3 * s, cy - r + i * 3 * s, r * 1.3f, r * 1.3f}, 2 * s, c); }
    else if (k == TAB_SERVERS) { DrawRing((Vector2){cx, cy}, r - 2 * s, r, 0, 360, 24, c); aLine(cx - r, cy, cx + r, cy, 1, c); DrawEllipseLines((int)cx, (int)cy, r * 0.45f, r, c); }
    else if (k == TAB_PARAMS) { for (int i = 0; i < 3; i++) { float y = cy - r * 0.7f + i * r * 0.7f; aLine(cx - r, y, cx + r, y, 1, c); DrawCircle((int)(cx - r * 0.5f + i * r * 0.5f), (int)y, 3 * s, c); } }
    else { DrawRing((Vector2){cx, cy}, r * 0.45f, r * 0.75f, 0, 360, 24, c); for (int i = 0; i < 8; i++) { float a = i * PI / 4; aLine(cx + cosf(a) * r * 0.7f, cy + sinf(a) * r * 0.7f, cx + cosf(a) * r, cy + sinf(a) * r, 2, c); } }
}
static bool navItem(Rectangle r, int k, const char *label, bool sel, bool enabled) {
    float s = aU; bool hov = enabled && CheckCollisionPointRec(aMousePos(), r);
    if (sel) {
        aRR(aGrow(r, 1.5f * s), 8 * s, (Color){10, 14, 15, 255});
        aGrad(r, aRnd(r, 7 * s), aMul(C_GEL, 1.3f), aMul(C_GEL, 0.62f), 4);
        aRR((Rectangle){r.x + 8 * s, r.y + 3 * s, r.width - 16 * s, r.height * 0.4f}, 6 * s, (Color){255, 255, 255, 40});
        DrawRectangle((int)r.x, (int)(r.y + 8 * s), (int)(4 * s), (int)(r.height - 16 * s), C_CYAN);
    } else if (hov) aRR(r, 7 * s, (Color){111, 214, 208, 26});
    Color c = !enabled ? (Color){100, 104, 102, 255} : sel ? (Color){240, 236, 226, 255} : (Color){200, 194, 180, 255};
    icon(k, r.x + 30 * s, r.y + r.height / 2, 9 * s, sel ? C_CYAN : c);
    aText(label, r.x + 54 * s, r.y + (r.height - 17 * s) / 2, 17, c, sel);
    if (hov && !aClickUsed && aMPressed()) { aClickUsed = true; if (aBlipOk) PlaySound(aBlip); return true; }
    return false;
}
static void card(Rectangle r, const char *title) {   // captionless glass card with a heading
    float s = aU;
    aRR((Rectangle){r.x + 5 * s, r.y + 7 * s, r.width, r.height}, 9 * s, (Color){4, 6, 6, 130});
    aRR(aGrow(r, 1.5f * s), 10 * s, (Color){12, 16, 17, 240});
    aGrad(r, aRnd(r, 9 * s), (Color){34, 46, 48, 232}, (Color){12, 17, 18, 240}, 5);
    aScissor(r.x, r.y, r.width, 34 * s); aRR(r, 9 * s, (Color){120, 140, 142, 60}); aScissorEnd();
    aLine(r.x + 9 * s, r.y + 34 * s, r.x + r.width - 9 * s, r.y + 34 * s, 1, (Color){8, 10, 10, 255});
    aLine(r.x + 9 * s, r.y + 1.5f * s, r.x + r.width - 9 * s, r.y + 1.5f * s, 1, (Color){200, 214, 210, 120});
    if (title) aText(title, r.x + 16 * s, r.y + 8 * s, 15, (Color){236, 230, 216, 255}, true);
}

// ---- login (separate screen, like a game launcher's sign-in)
static void drawBoot(float W, float H) {
    float s = aU, k = Clamp(bootT / 2.2f, 0, 1);
    aTextC("SEIRY\xC5\xAA OS", W / 2, H * 0.68f, 46, (Color){232, 226, 210, 255}, true);
    aTextC("Ministry of Environment  -  2006", W / 2, H * 0.68f + 58 * s, 15, (Color){150, 210, 205, 230}, false);
    aProgress((Rectangle){W / 2 - 220 * s, H * 0.68f + 90 * s, 440 * s, 12 * s}, k, (Color){80, 160, 150, 255});
    aTextC(updStatus[0] ? updStatus : "Starting ...", W / 2, H * 0.68f + 110 * s, 12, C_BONE2, false);
    if (bootT > 2.2f || (bootT > 0.3f && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)))) lstate = L_LOGIN;
}
static void drawLogin(float W, float H) {
    float s = aU;
    aLogo(W / 2, H * 0.08f, 62, "S E I R Y \xC5\xAA   O S   -   L A U N C H E R");
    Rectangle p = {W / 2 - 230 * s, H * 0.33f, 460 * s, 360 * s};
    aWindow(p, "Seiry\xC5\xAB - Citizen Registry");
    aTextBox((Rectangle){p.x + 40 * s, p.y + 86 * s, 380 * s, 40 * s}, "Citizen name", &tbUser);
    aTextBox((Rectangle){p.x + 40 * s, p.y + 160 * s, 380 * s, 40 * s}, "Password", &tbPass);
    bool doLogin = aButton((Rectangle){p.x + 40 * s, p.y + 220 * s, 180 * s, 42 * s}, "Log In", true) || (IsKeyPressed(KEY_ENTER) && tbPass.len > 0);
    bool doCreate = aButton((Rectangle){p.x + 240 * s, p.y + 220 * s, 180 * s, 42 * s}, "Register", true);
    bool doGuest = aButton((Rectangle){p.x + 120 * s, p.y + 274 * s, 220 * s, 38 * s}, "Enter as Guest", true);
    if (IsKeyPressed(KEY_TAB)) { bool u = tbUser.active; tbUser.active = !u; tbPass.active = u; }
    aTextC(status, W / 2, p.y + 322 * s, 15, (Color){255, 170, 140, 255}, true);
    DrawRectangle(0, (int)(H - 40 * s), (int)W, (int)(40 * s), (Color){8, 10, 11, 200});
    aTextC("Accounts are stored locally in Data/Userdata (salted SHA-256). Online accounts require the multiplayer server.", W / 2, H - 29 * s, 14, (Color){214, 208, 196, 255}, false);
    if (doLogin) { if (login(tbUser.buf, tbPass.buf)) enterMain(tbUser.buf); }
    if (doCreate) {
        if (!validName(tbUser.buf)) snprintf(status, sizeof status, "Name: 3-16 letters, digits, _ or -.");
        else if (tbPass.len < 4) snprintf(status, sizeof status, "Password: at least 4 characters.");
        else if (createAccount(tbUser.buf, tbPass.buf)) enterMain(tbUser.buf);
    }
    if (doGuest) enterMain("Guest");
}

// ---- tabs
static int enabledMods(void) { int n = 0; for (int i = 0; i < nModsL; i++) if (mods[i].enabled && mods[i].valid) n++; return n; }
static void tabPlay(Rectangle c) {
    float s = aU;
    // hero text over the live backdrop
    aText("AFTERGLOW", c.x + 40 * s, c.y + 36 * s, 44, (Color){232, 226, 210, 255}, true);
    aText("Japan, 2006. The bright, green future did not arrive.", c.x + 42 * s, c.y + 94 * s, 16, (Color){206, 200, 186, 255}, false);
    aText("Open-world survival  -  no story but the one you make.", c.x + 42 * s, c.y + 116 * s, 14, (Color){150, 210, 205, 230}, false);
    // cards
    float cy = c.y + c.height - 300 * s, cw = (c.width - 80 * s - 40 * s) / 3;
    Rectangle n1 = {c.x + 40 * s, cy, cw * 1.6f, 270 * s};
    card(n1, "Patch notes");
    char *news = LoadFileText(P("Data/news.txt"));
    const char *txt = news ? news : "No news file (Data/news.txt).";
    float y = n1.y + 46 * s; const char *sp = txt;
    while (*sp && y < n1.y + n1.height - 20 * s) {
        const char *e = strchr(sp, '\n'); int len = e ? (int)(e - sp) : (int)strlen(sp);
        char ln[200]; snprintf(ln, sizeof ln, "%.*s", len > 199 ? 199 : len, sp);
        aText(ln, n1.x + 16 * s, y, 13, C_BONE, false);
        y += 19 * s; sp = e ? e + 1 : sp + len;
    }
    if (news) UnloadFileText(news);
    Rectangle n2 = {n1.x + n1.width + 20 * s, cy, c.x + c.width - 40 * s - (n1.x + n1.width + 20 * s), 270 * s};
    card(n2, "Survivor record");
    const char *lab[5] = {"Kills", "Deaths", "Runs", "Best survival", "Playtime"};
    char val[5][32];
    snprintf(val[0], 32, "%d", prof.kills); snprintf(val[1], 32, "%d", prof.deaths); snprintf(val[2], 32, "%d", prof.sessions);
    snprintf(val[3], 32, "%02d:%02d", (int)prof.best / 60, (int)prof.best % 60); snprintf(val[4], 32, "%.1f h", prof.playtime / 3600.0f);
    for (int i = 0; i < 5; i++) { aText(lab[i], n2.x + 16 * s, n2.y + (48 + i * 26) * s, 14, C_BONE2, false); Vector2 m = aMeasure(val[i], 14, false); aText(val[i], n2.x + n2.width - 16 * s - m.x, n2.y + (48 + i * 26) * s, 14, C_CYAN, false); }
    aText("Character: Nanashi no Gonbei", n2.x + 16 * s, n2.y + 180 * s, 12, (Color){150, 146, 136, 230}, false);
    aText(TextFormat("Mods enabled: %d of %d", enabledMods(), nModsL), n2.x + 16 * s, n2.y + 198 * s, 12, C_BONE2, false);
    if (aButton((Rectangle){n2.x + 16 * s, n2.y + n2.height - 46 * s, n2.width - 32 * s, 32 * s}, "Manage Mods", true)) tab = TAB_MODS;
}
static void tabMods(Rectangle c) {
    float s = aU;
    installDropped();
    aText("Mods", c.x + 40 * s, c.y + 28 * s, 30, (Color){232, 226, 210, 255}, true);
    aText("Load order: the top mod wins when two mods replace the same file. Drop .zip files onto the window to install.", c.x + 42 * s, c.y + 70 * s, 13, C_BONE2, false);
    Rectangle lst = {c.x + 40 * s, c.y + 100 * s, c.width * 0.58f, c.height - 130 * s};
    card(lst, NULL);
    // column header
    float hx = lst.x + 16 * s, hy = lst.y + 9 * s;
    aText("On", hx, hy, 12, C_CYAN, true); aText("Name", hx + 46 * s, hy, 12, C_CYAN, true); aText("Version", hx + lst.width * 0.50f, hy, 12, C_CYAN, true);
    aText("Files", hx + lst.width * 0.64f, hy, 12, C_CYAN, true); aText("Order", lst.x + lst.width - 110 * s, hy, 12, C_CYAN, true);
    float ly = lst.y + 44 * s, rowH = 42 * s; int rows = (int)((lst.height - 54 * s) / rowH);
    Vector2 mp = aMousePos();
    float wh = GetMouseWheelMove(); if (wh != 0 && CheckCollisionPointRec(mp, lst)) modScroll -= (int)wh;
    if (modScroll > nModsL - rows) modScroll = nModsL - rows; if (modScroll < 0) modScroll = 0;
    if (nModsL == 0) aText("No mods found. Drop .zip files here, or put zips / folders in the Mods folder.", hx, ly + 10 * s, 13, C_BONE2, false);
    int swapA = -1, swapB = -1;
    for (int r = 0; r < rows && r + modScroll < nModsL; r++) {
        int i = r + modScroll; ModEntry *m = &mods[i];
        Rectangle row = {lst.x + 8 * s, ly + r * rowH, lst.width - 16 * s, rowH - 4 * s};
        bool sel = i == modSel;
        if (sel) { aRR(row, 6 * s, (Color){47, 93, 94, 170}); aRRL(row, 6 * s, 1, C_CYAN); } else if (r % 2) aRR(row, 6 * s, (Color){255, 255, 255, 8});
        if (CheckCollisionPointRec(mp, (Rectangle){row.x + 40 * s, row.y, row.width - 150 * s, row.height}) && aMPressed() && !aClickUsed) { modSel = i; aClickUsed = true; }
        bool en = aToggle(row.x + 8 * s, row.y + 9 * s, "", m->enabled);
        if (en != m->enabled) { m->enabled = en; modsDirty = true; }
        Color tc = !m->valid ? (Color){220, 90, 70, 255} : m->enabled ? (Color){236, 230, 216, 255} : (Color){130, 128, 120, 255};
        aText(TextFormat("%d. %s", i + 1, m->name), row.x + 46 * s, row.y + 4 * s, 15, tc, false);
        aText(m->valid ? (m->isZip ? "zip archive" : "folder") : "unreadable archive", row.x + 46 * s, row.y + 22 * s, 11, C_BONE2, false);
        aText(m->version[0] ? m->version : "-", hx + lst.width * 0.50f, row.y + 11 * s, 13, C_BONE2, false);
        aText(TextFormat("%d", m->nfiles), hx + lst.width * 0.64f, row.y + 11 * s, 13, C_BONE2, false);
        if (aButton((Rectangle){row.x + row.width - 112 * s, row.y + 6 * s, 52 * s, 26 * s}, "Up", i > 0)) { swapA = i; swapB = i - 1; }
        if (aButton((Rectangle){row.x + row.width - 56 * s, row.y + 6 * s, 52 * s, 26 * s}, "Down", i < nModsL - 1)) { swapA = i; swapB = i + 1; }
    }
    if (swapA >= 0) { ModEntry t = mods[swapA]; mods[swapA] = mods[swapB]; mods[swapB] = t; if (modSel == swapA) modSel = swapB; else if (modSel == swapB) modSel = swapA; modsDirty = true; }
    // details
    Rectangle d = {lst.x + lst.width + 20 * s, lst.y, c.x + c.width - 40 * s - (lst.x + lst.width + 20 * s), lst.height};
    card(d, "Details");
    float dx = d.x + 16 * s, dy = d.y + 46 * s, dw = d.width - 32 * s;
    if (nModsL > 0) {
        ModEntry *m = &mods[modSel];
        aText(m->name, dx, dy, 19, (Color){236, 230, 216, 255}, true);
        aText(TextFormat("Version %s  -  %s", m->version[0] ? m->version : "?", m->author[0] ? m->author : "unknown author"), dx, dy + 28 * s, 12, C_BONE2, false);
        aText(TextFormat("Mods/%s", m->file), dx, dy + 46 * s, 12, C_BONE2, false);
        aText(TextFormat("%d files, %.1f KB  -  priority %d of %d", m->nfiles, m->bytes / 1024.0f, modSel + 1, nModsL), dx, dy + 64 * s, 12, C_BONE2, false);
        wrapText(m->desc[0] ? m->desc : "No description (add description= to mod.ini).", dx, dy + 90 * s, dw, 13, C_BONE, 4);
        aText("Conflicts", dx, dy + 172 * s, 13, C_CYAN, true);
        float cy = dy + 192 * s; int shown = 0;
        for (int j = 0; j < nModsL && shown < 4; j++) {
            if (j == modSel || !mods[j].enabled || !m->enabled) continue;
            int sh = sharedFiles(m, &mods[j]); if (!sh) continue;
            if (j > modSel) aText(TextFormat("Overrides %d file(s) of %s", sh, mods[j].name), dx, cy, 12, (Color){150, 210, 205, 255}, false);
            else aText(TextFormat("%d file(s) overridden by %s", sh, mods[j].name), dx, cy, 12, (Color){230, 160, 120, 255}, false);
            cy += 18 * s; shown++;
        }
        if (!shown) aText(m->enabled ? "None with other enabled mods." : "Mod is disabled.", dx, cy, 12, C_BONE2, false);
    }
    float bw = (dw - 10 * s) / 2, by = d.y + d.height - 96 * s;
    if (aButton((Rectangle){dx, by, bw, 34 * s}, "Open Folder", true)) openFolder(P("Mods"));
    if (aButton((Rectangle){dx + bw + 10 * s, by, bw, 34 * s}, "Refresh", true)) { if (modsDirty) modsWrite(); modsReload(); snprintf(modMsg, sizeof modMsg, "Rescanned the Mods folder."); }
    if (aButton((Rectangle){dx, by + 44 * s, bw, 34 * s}, "Enable All", nModsL > 0)) { for (int i = 0; i < nModsL; i++) mods[i].enabled = true; modsDirty = true; }
    if (aButton((Rectangle){dx + bw + 10 * s, by + 44 * s, bw, 34 * s}, "Disable All", nModsL > 0)) { for (int i = 0; i < nModsL; i++) mods[i].enabled = false; modsDirty = true; }
    if (modMsg[0]) aText(modMsg, lst.x, lst.y + lst.height + 8 * s, 12, C_BONE2, false);
    if (modsDirty) modsWrite();   // the load order is saved as it changes
}
static void tabServers(Rectangle c) {
    float s = aU;
    aText("Servers", c.x + 40 * s, c.y + 28 * s, 30, (Color){232, 226, 210, 255}, true);
    aText("The server browser opens with the multiplayer update.", c.x + 42 * s, c.y + 70 * s, 13, C_BONE2, false);
    Rectangle t = {c.x + 40 * s, c.y + 100 * s, c.width - 80 * s, c.height - 130 * s};
    card(t, NULL);
    const char *cols[5] = {"Name", "Map", "Players", "Mods", "Ping"}; float cx[5] = {0, 0.45f, 0.65f, 0.78f, 0.9f};
    for (int i = 0; i < 5; i++) aText(cols[i], t.x + 16 * s + (t.width - 32 * s) * cx[i], t.y + 9 * s, 12, C_CYAN, true);
    for (int r = 0; r < 6; r++) aRR((Rectangle){t.x + 8 * s, t.y + (44 + r * 40) * s, t.width - 16 * s, 34 * s}, 6 * s, (Color){255, 255, 255, (unsigned char)(r % 2 ? 6 : 12)});
    aTextC("No servers  -  multiplayer is not in this build", t.x + t.width / 2, t.y + t.height / 2, 16, (Color){150, 146, 136, 230}, false);
}
static void tabParams(Rectangle c) {
    float s = aU;
    aText("Parameters", c.x + 40 * s, c.y + 28 * s, 30, (Color){232, 226, 210, 255}, true);
    aText("Startup parameters passed to Zanko.exe when PLAY is pressed.", c.x + 42 * s, c.y + 70 * s, 13, C_BONE2, false);
    Rectangle a = {c.x + 40 * s, c.y + 100 * s, (c.width - 100 * s) / 2, 300 * s};
    card(a, "Memory");
    int sys = systemRamMB(); int maxMB = sys > 2048 ? ((sys - 1024) / 256) * 256 : 4096; if (maxMB > 16384) maxMB = 16384;
    float rv = aSlider((Rectangle){a.x + 20 * s, a.y + 80 * s, a.width - 40 * s, 10 * s}, "Memory allocation", (float)ramMB, 0, (float)maxMB, ramMB < 256 ? "Auto" : "%.0f MB");
    ramMB = ((int)(rv + 128) / 256) * 256; if (ramMB < 512) ramMB = ramMB < 256 ? 0 : 512;
    aText(TextFormat("System memory: %d MB", sys), a.x + 20 * s, a.y + 112 * s, 13, C_BONE2, false);
    wrapText("Reserves a resident working set of this size for the game process (--ram). Auto lets Windows manage it.", a.x + 20 * s, a.y + 138 * s, a.width - 40 * s, 12, (Color){150, 146, 136, 230}, 3);
    pNoVsync = aToggle(a.x + 20 * s, a.y + 196 * s, "Disable V-Sync (--novsync)", pNoVsync);
    float fv = aSlider((Rectangle){a.x + 20 * s, a.y + 262 * s, a.width - 40 * s, 10 * s}, "FPS limit (--fps)", (float)pFps, 0, 240, pFps ? "%.0f" : "Off");
    pFps = ((int)(fv + 5) / 10) * 10; if (pFps < 30) pFps = 0;
    Rectangle b = {a.x + a.width + 20 * s, a.y, a.width, 300 * s};
    card(b, "Game");
    pWindowed = aToggle(b.x + 20 * s, b.y + 50 * s, "Force windowed (--windowed)", pWindowed);
    pNewGame = aToggle(b.x + 20 * s, b.y + 80 * s, "Skip menu, start a new game (--newgame)", pNewGame);
    pNoMods = aToggle(b.x + 20 * s, b.y + 110 * s, "Start without mods (--nomods)", pNoMods);
    pLog = aToggle(b.x + 20 * s, b.y + 140 * s, "Write Data/log.txt (--log)", pLog);
    float rr = aSlider((Rectangle){b.x + 20 * s, b.y + 200 * s, b.width - 40 * s, 10 * s}, "Render height override (--render)", (float)pRender, 0, 480, pRender ? "%.0fp" : "Off");
    pRender = ((int)rr / 30) * 30; if (pRender < 180) pRender = 0;
    aTextBox((Rectangle){b.x + 20 * s, b.y + 252 * s, 160 * s, 32 * s}, "World seed (--seed)", &tbSeed);
    { int k = 0; for (int i = 0; i < tbSeed.len && k < 20; i++) if (tbSeed.buf[i] >= '0' && tbSeed.buf[i] <= '9') pSeed[k++] = tbSeed.buf[i]; pSeed[k] = 0; }
    aText(pSeed[0] ? "Fixed world" : "Random world each run", b.x + 196 * s, b.y + 260 * s, 12, C_BONE2, false);
    Rectangle e = {a.x, a.y + 320 * s, c.width - 80 * s, c.y + c.height - 30 * s - (a.y + 320 * s)};
    card(e, "Additional parameters");
    aTextBox((Rectangle){e.x + 20 * s, e.y + 48 * s, e.width - 40 * s, 34 * s}, NULL, &tbArgs);
    snprintf(launchArgs, sizeof launchArgs, "%s", tbArgs.buf);
    char args[512]; buildArgs(args, sizeof args);
    aText(TextFormat("Command line:  Zanko.exe %s", args), e.x + 20 * s, e.y + 92 * s, 12, (Color){150, 210, 205, 230}, false);
    static char last[512] = ""; if (strcmp(last, args)) { snprintf(last, sizeof last, "%s", args); launcherIniSave(); }   // saved as it changes
}
static void tabOptions(Rectangle c) {
    float s = aU;
    aText("Options", c.x + 40 * s, c.y + 28 * s, 30, (Color){232, 226, 210, 255}, true);
    aText("Shared with the in-game Settings (Data/config.ini).", c.x + 42 * s, c.y + 70 * s, 13, C_BONE2, false);
    float cw = (c.width - 100 * s) / 2;
    Rectangle a = {c.x + 40 * s, c.y + 100 * s, cw, 330 * s};
    card(a, "Display & audio");
    float lx = a.x + 20 * s, w = a.width - 40 * s, y = a.y + 50 * s;
    int ri = 0; for (int i = 0; i < 4; i++) if (resList[i][0] == cfg.resW && resList[i][1] == cfg.resH) ri = i;
    char b[96]; snprintf(b, sizeof b, "Resolution: %d x %d", cfg.resW, cfg.resH);
    if (aButton((Rectangle){lx, y, w, 34 * s}, b, true)) { ri = (ri + 1) % 4; cfg.resW = resList[ri][0]; cfg.resH = resList[ri][1]; }
    y += 48 * s;
    cfg.fullscreen = aToggle(lx, y, "Fullscreen (borderless)", cfg.fullscreen); y += 30 * s;
    cfg.vsync = aToggle(lx, y, "V-Sync", cfg.vsync); y += 58 * s;
    float rh = aSlider((Rectangle){lx, y, w, 10 * s}, "Render resolution (PS1 = 240p)", (float)cfg.renderH, 180, 480, "%.0fp"); cfg.renderH = ((int)rh / 30) * 30; y += 62 * s;
    cfg.volume = aSlider((Rectangle){lx, y, w, 10 * s}, "Master volume", cfg.volume, 0, 1, "%.2f");
    Rectangle g = {a.x + cw + 20 * s, a.y, cw, 330 * s};
    card(g, "Gameplay & launcher");
    float rx = g.x + 20 * s, y2 = g.y + 72 * s;
    cfg.sens = aSlider((Rectangle){rx, y2, w, 10 * s}, "Mouse sensitivity", cfg.sens * 1000, 0.5f, 6, "%.1f") / 1000; y2 += 58 * s;
    cfg.aimLag = aSlider((Rectangle){rx, y2, w, 10 * s}, "Aim lag (lower = heavier, GoldenEye-like)", cfg.aimLag, 2, 16, "%.1f"); y2 += 58 * s;
    cfg.fov = aSlider((Rectangle){rx, y2, w, 10 * s}, "Field of view", cfg.fov, 55, 95, "%.0f"); y2 += 36 * s;
    cfg.autoUpdate = aToggle(rx, y2, "Update automatically on start", cfg.autoUpdate); y2 += 28 * s;
    closeOnPlay = aToggle(rx, y2, "Close launcher when the game starts", closeOnPlay); y2 += 28 * s;
    bool beta = !strcmp(cfg.channel, "beta"); bool nb = aToggle(rx, y2, "Beta update channel", beta);
    if (nb != beta) strcpy(cfg.channel, nb ? "beta" : "stable");
    static Config lastCfg; static int lastClose = -1;
    if (memcmp(&lastCfg, &cfg, sizeof cfg) || lastClose != closeOnPlay) { lastCfg = cfg; lastClose = closeOnPlay; cfgSave(); launcherIniSave(); }   // saved as it changes
}
static void drawLauncher(float W, float H) {
    float s = aU, sbw = 236 * s, bbh = 92 * s;
    if (versReload) { versReload = 0; versLoad(); }
    verHoldClick = verPopup && CheckCollisionPointRec(aMousePos(), verPopR);
    if (verHoldClick) aClickUsed = true;   // the open list sits on top: widgets under it must not take the click
    // sidebar
    glassStrip((Rectangle){0, 0, sbw, H}, 236);
    aLine(sbw, 0, sbw, H, 1, (Color){8, 10, 10, 255}); aLine(sbw - 2 * s, 0, sbw - 2 * s, H, 1, (Color){200, 214, 210, 50});
    aText(aFontOwn ? "ZANK\xC5\x8C" : "ZANKO", 22 * s, 22 * s, 34, (Color){232, 226, 210, 255}, true);
    aText("2006", 22 * s, 60 * s, 34, (Color){232, 226, 210, 255}, true);
    if (aFontJOwn) aTextF("\xE6\xAE\x8B\xE5\x85\x89", 128 * s, 66 * s, 24, C_AMBER, 2);
    aText("A F T E R G L O W", 24 * s, 104 * s, 11, (Color){150, 210, 205, 230}, false);
    const char *names[5] = {"Play", "Mods", "Servers", "Parameters", "Options"};
    for (int i = 0; i < 5; i++) if (navItem((Rectangle){12 * s, (150 + i * 52) * s, sbw - 24 * s, 44 * s}, i, i == 1 && nModsL ? TextFormat("Mods  (%d)", enabledMods()) : names[i], tab == i, true)) tab = i;
    // account box
    Rectangle acc = {12 * s, H - 132 * s, sbw - 24 * s, 118 * s};
    card(acc, NULL);
    aText(prof.name, acc.x + 14 * s, acc.y + 10 * s, 17, (Color){236, 230, 216, 255}, true);
    aText(strcmp(prof.name, "Guest") ? "Local citizen account" : "Guest (stats not saved)", acc.x + 14 * s, acc.y + 34 * s, 11, C_BONE2, false);
    aText(TextFormat("v%s  -  %s", localVer, cfg.channel), acc.x + 14 * s, acc.y + 52 * s, 11, (Color){150, 146, 136, 230}, false);
    if (aButton((Rectangle){acc.x + 12 * s, acc.y + 76 * s, acc.width * 0.5f - 18 * s, 30 * s}, "Log Out", true)) { lstate = L_LOGIN; tbPass.len = 0; tbPass.buf[0] = 0; }
    if (aButton((Rectangle){acc.x + acc.width * 0.5f + 6 * s, acc.y + 76 * s, acc.width * 0.5f - 18 * s, 30 * s}, "Quit", true)) { if (modsDirty) modsWrite(); CloseWindow(); exit(0); }
    // content
    Rectangle c = {sbw, 0, W - sbw, H - bbh};
    if (tab != TAB_PLAY) aRR((Rectangle){c.x, c.y, c.width, c.height}, 0, (Color){10, 14, 15, 150});
    Rectangle vb = {c.x + c.width - 40 * s - 270 * s, c.y + 40 * s, 270 * s, 36 * s};
    if (tab == TAB_PLAY) {   // version picker: top right of the Play page
        tabPlay(c);
        aText("Version", vb.x, vb.y - 18 * s, 11, C_BONE2, false);
        bool latest = !verSel[0] || !strcmp(verSel, localVer);
        char lb[64]; snprintf(lb, sizeof lb, "v%s%s", latest ? localVer : verSel, latest ? "  (latest)" : "");
        if (aButton(vb, lb, !verBusy)) verPopup = !verPopup;
        float tx = vb.x + vb.width - 20 * s, ty = vb.y + vb.height / 2;   // the dropdown arrow
        if (verPopup) DrawTriangle((Vector2){tx - 5 * s, ty + 3 * s}, (Vector2){tx + 5 * s, ty + 3 * s}, (Vector2){tx, ty - 3 * s}, (Color){20, 30, 40, 255});
        else DrawTriangle((Vector2){tx - 5 * s, ty - 3 * s}, (Vector2){tx, ty + 3 * s}, (Vector2){tx + 5 * s, ty - 3 * s}, (Color){20, 30, 40, 255});
        if (verBusy || verStatus[0]) aText(verStatus, vb.x, vb.y + vb.height + 6 * s, 11, C_CYAN, false);
    } else verPopup = false;
    if (tab == TAB_PLAY) {} else if (tab == TAB_MODS) tabMods(c); else if (tab == TAB_SERVERS) tabServers(c); else if (tab == TAB_PARAMS) tabParams(c); else tabOptions(c);
    // bottom bar
    Rectangle bb = {sbw, H - bbh, W - sbw, bbh};
    glassStrip(bb, 245);
    aLine(bb.x, bb.y, bb.x + bb.width, bb.y, 1, (Color){8, 10, 10, 255}); aLine(bb.x, bb.y + 2 * s, bb.x + bb.width, bb.y + 2 * s, 1, (Color){200, 214, 210, 70});
    float bx = bb.x + 28 * s;
    aText("Build", bx, bb.y + 16 * s, 11, C_BONE2, false);
    aText(TextFormat("Zank\xC5\x8D 2006  v%s", localVer), bx, bb.y + 32 * s, 16, (Color){236, 230, 216, 255}, false);
    { char us[200]; snprintf(us, sizeof us, "%s", updStatus); while (strlen(us) > 4 && aMeasure(us, 11, false).x > 330 * s) { us[strlen(us) - 4] = 0; strcat(us, "..."); }
      aText(us, bx, bb.y + 58 * s, 11, updAvailable ? C_CYAN : (Color){150, 146, 136, 230}, false); }
    float mx = bx + 360 * s;
    aText("Session", mx, bb.y + 16 * s, 11, C_BONE2, false);
    aText(TextFormat("%d mod%s  -  memory %s", enabledMods(), enabledMods() == 1 ? "" : "s", ramMB >= 256 ? TextFormat("%d MB", ramMB) : "auto"), mx, bb.y + 34 * s, 13, (Color){206, 200, 186, 255}, false);
    if (updAvailable && !updBusy) { if (aButton((Rectangle){mx, bb.y + 52 * s, 170 * s, 28 * s}, "Install Update", true)) startThread(1); }
    else if (aButton((Rectangle){mx, bb.y + 52 * s, 170 * s, 28 * s}, updBusy ? "Working ..." : "Check for Updates", !updBusy)) checkUpdates();
    aText(status, mx + 190 * s, bb.y + 58 * s, 12, (Color){230, 160, 120, 255}, false);
    // PLAY
    Rectangle pr = {bb.x + bb.width - 300 * s, bb.y + 16 * s, 272 * s, 60 * s};
    aRR(aGrow(pr, 4 * s), 34 * s, (Color){111, 214, 208, 70});
    bool go = aButton(pr, verBusy ? "..." : "PLAY", !verBusy);
    if (verLaunch) { verLaunch = 0; go = true; }   // an older version just finished downloading: start it
    if (go) { if (modsDirty) modsWrite(); play(); if (closeOnPlay && !strcmp(status, "Game started.")) { profSave(); launcherIniSave(); CloseWindow(); exit(0); } }
    // the version list pops up above the picker
    if (verPopup) {
        if (verHoldClick) aClickUsed = false;
        int rows = 1; for (int i = 0; i < nVers; i++) if (strcmp(vers[i].ver, localVer) && (!strcmp(vers[i].channel, "stable") || !strcmp(cfg.channel, "beta"))) rows++;
        float rh = 40 * s, pw = 380 * s, ph = rows * rh + 46 * s; if (ph > H - 140 * s) ph = H - 140 * s;
        Rectangle pp = {vb.x + vb.width - pw, vb.y + vb.height + 8 * s, pw, ph}; verPopR = pp;
        aRR(aGrow(pp, 2 * s), 10 * s, (Color){8, 11, 12, 255}); aRR(pp, 9 * s, (Color){22, 30, 32, 255});   // solid: nothing behind shows through
        aLine(pp.x + 9 * s, pp.y + 1.5f * s, pp.x + pp.width - 9 * s, pp.y + 1.5f * s, 1, (Color){200, 214, 210, 160});
        aText("Choose the version PLAY starts", pp.x + 16 * s, pp.y + 14 * s, 12, C_BONE2, false);
        float y = pp.y + 38 * s; Vector2 m = aMousePos(); int pick = -2;
        for (int i = -1; i < nVers && y + rh <= pp.y + pp.height - 4 * s; i++) {
            const char *v = i < 0 ? localVer : vers[i].ver;
            if (i >= 0 && (!strcmp(v, localVer) || (strcmp(vers[i].channel, "stable") && strcmp(cfg.channel, "beta")))) continue;
            Rectangle r = {pp.x + 8 * s, y, pp.width - 16 * s, rh - 4 * s};
            bool sel = i < 0 ? (!verSel[0] || !strcmp(verSel, localVer)) : !strcmp(verSel, v), hov = CheckCollisionPointRec(m, r);
            if (sel || hov) aRR(r, 6 * s, sel ? (Color){47, 93, 94, 220} : (Color){60, 80, 84, 150});
            const char *title = i < 0 ? "Installed" : vers[i].title;
            aText(TextFormat("v%s", v), r.x + 12 * s, r.y + 5 * s, 15, (Color){236, 230, 216, 255}, true);
            aText(title, r.x + 88 * s, r.y + 7 * s, 12, C_BONE2, false);
            const char *tag = i < 0 ? "latest" : verInstalled(v) ? "installed" : "downloads on PLAY";
            Vector2 tm = aMeasure(tag, 11, false); aText(tag, r.x + r.width - tm.x - 10 * s, r.y + 9 * s, 11, i < 0 ? C_CYAN : verInstalled(v) ? C_BONE : (Color){150, 146, 136, 230}, false);
            if (i >= 0 && vers[i].date[0]) aText(vers[i].date, r.x + 88 * s, r.y + 22 * s, 10, (Color){130, 126, 118, 230}, false);
            if (hov && !aClickUsed && aMPressed()) { aClickUsed = true; pick = i; }
            y += rh;
        }
        if (nVers == 0) aText("No version list yet. It loads with the update check.", pp.x + 16 * s, y + 4 * s, 11, C_BONE2, false);
        if (pick >= -1) { if (pick < 0) verSel[0] = 0; else snprintf(verSel, sizeof verSel, "%s", vers[pick].ver); verPopup = false; verStatus[0] = 0; launcherIniSave(); }
        else if (aMPressed() && !aClickUsed && !CheckCollisionPointRec(m, pp)) verPopup = false;
    }
}

int main(void) {
    aeroInitPaths(); cfgLoad(); launcherIniLoad(); readLocalVersion();
    ensureDir("Data"); ensureDir("Data/Userdata"); ensureDir("Mods"); ensureDir("Textures");
    if (!FileExists(P("Data/launcher.ini"))) launcherIniSave();
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    const char *vdir = getenv("KIRI_VIDEO"); vidOn = vdir != NULL;
    InitWindow(1280, 720, "Zank\xC5\x8D 2006 - Launcher");
    SetWindowMinSize(1100, 620);
    if (FileExists(P("Data/UI/icon.png"))) { Image ico = LoadImage(P("Data/UI/icon.png")); if (ico.data) { SetWindowIcon(ico); UnloadImage(ico); } }
    SetExitKey(KEY_NULL);
    InitAudioDevice();
    if (IsAudioDeviceReady()) {
        int n = 22050 / 14; short *d = MemAlloc(n * 2);
        for (int i = 0; i < n; i++) { float t = (float)i / 22050; d[i] = (short)(sinf(t * 2 * PI * (900 + t * 3000)) * 0.22f * expf(-t * 30) * 30000); }
        Wave w = {(unsigned)n, 22050, 16, 1, d}; aBlip = LoadSoundFromWave(w); UnloadWave(w); aBlipOk = true; aMakeErr();
        SetMasterVolume(cfg.volume);
    }
    aeroLoadFont();
    saInit(P("Textures/ecoglass.png"), P("Textures/facade_a.png"));
    tbPass.mask = true; tbUser.max = 16; tbPass.max = 31; tbArgs.max = 250; tbSeed.max = 10;
    snprintf(tbArgs.buf, sizeof tbArgs.buf, "%s", launchArgs); tbArgs.len = (int)strlen(tbArgs.buf);
    snprintf(tbSeed.buf, sizeof tbSeed.buf, "%s", pSeed); tbSeed.len = (int)strlen(tbSeed.buf);
    if (vidOn) lastUser[0] = 0;
    if (lastUser[0]) { snprintf(tbUser.buf, sizeof tbUser.buf, "%s", lastUser); tbUser.len = (int)strlen(lastUser); tbPass.active = true; } else tbUser.active = true;
    versLoad();
    if (cfg.autoUpdate) checkUpdates(); else snprintf(updStatus, sizeof updStatus, "Automatic update check is off.");
    modsReload();
    const char *shot = getenv("KIRI_SHOT"); int frame = 0;
    while (!WindowShouldClose() && !updQuit) {
        frame++;
        if (cfg.autoUpdate && updAvailable && !updBusy && !autoInstallTried && instUrl[0] && !shot) { autoInstallTried = 1; startThread(1); }   // automatic update: install as soon as one is found
        float t = shot ? frame / 30.0f : (float)GetTime();
        bootT += GetFrameTime();
        float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
        aeroBegin(720);
        BeginDrawing();
        ClearBackground(BLACK);
        saBackdrop(t, 0, lstate == L_BOOT ? 1 : 0, Clamp(bootT / 2.2f, 0, 1), 1.0f);
        aCorrupt = 0;
        aLayerBegin();
        if (lstate == L_BOOT) drawBoot(W, H);
        else if (lstate == L_LOGIN) drawLogin(W, H);
        else drawLauncher(W, H);
        aLayerEnd(1);
        aDrawCursor();
        EndDrawing();
        if (shot) {
            if (frame == 30) TakeScreenshot("launcher_boot.png");
            if (frame == 31) lstate = L_LOGIN;
            if (frame == 45) TakeScreenshot("launcher_login.png");
            if (frame == 46) { if (!createAccount("Gonbei", "test1234")) login("Gonbei", "test1234"); enterMain("Gonbei"); }
            if (frame == 60) TakeScreenshot("launcher_play.png");
            if (frame == 61) tab = TAB_MODS;
            if (frame == 70) TakeScreenshot("launcher_mods.png");
            if (frame == 71) tab = TAB_PARAMS;
            if (frame == 80) TakeScreenshot("launcher_params.png");
            if (frame == 81) tab = TAB_OPTIONS;
            if (frame == 90) TakeScreenshot("launcher_options.png");
            if (frame == 91) tab = TAB_SERVERS;
            if (frame == 100) TakeScreenshot("launcher_servers.png");
            if (frame == 101) { tab = TAB_PLAY; verPopup = true; }
            if (frame == 110) TakeScreenshot("launcher_versions.png");
            if (frame == 111 && getenv("KIRI_VERTEST")) { verPopup = false; snprintf(verSel, sizeof verSel, "%s", getenv("KIRI_VERTEST")); play(); }
            if (frame > 111 && frame < 600 && getenv("KIRI_VERTEST") && (verBusy || verLaunch)) { if (verLaunch) { verLaunch = 0; play(); TraceLog(LOG_INFO, "VERTEST: %s | %s", verStatus, status); } continue; }
            if (frame == 112 && getenv("KIRI_VERTEST")) TraceLog(LOG_INFO, "VERTEST end: %s | %s", verStatus, status);
            if (frame >= 112) break;
        }
    }
    if (modsDirty) modsWrite();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
