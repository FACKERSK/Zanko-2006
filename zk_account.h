// zk_account.h - online accounts for the launcher (Firebase Authentication, REST).
// The account is made on the website (zanko2006.net/account.html): username, e-mail, password; the e-mail is confirmed by a link.
// The launcher signs in with the e-mail and the password; the username becomes the profile name.
// ZK_FIREBASE_KEY is the project's *web API key*: it is public by design (it only names the project), not a secret.
#ifndef ZK_FIREBASE_KEY
#define ZK_FIREBASE_KEY "AIzaSyCBCgN0SFahqYz-JveAgzYQbEcZ_-OB9Rc"
#endif
#define ZK_ACCOUNT_URL "https://zanko2006.net/account.html"

#if defined(_WIN32)
__declspec(dllimport) void *__stdcall InternetOpenA(const char *, unsigned long, const char *, const char *, unsigned long);
__declspec(dllimport) void *__stdcall InternetConnectA(void *, const char *, unsigned short, const char *, const char *, unsigned long, unsigned long, size_t);
__declspec(dllimport) void *__stdcall HttpOpenRequestA(void *, const char *, const char *, const char *, const char *, const char **, unsigned long, size_t);
__declspec(dllimport) int __stdcall HttpSendRequestA(void *, const char *, unsigned long, void *, unsigned long);
__declspec(dllimport) int __stdcall InternetReadFile(void *, void *, unsigned long, unsigned long *);
__declspec(dllimport) int __stdcall InternetCloseHandle(void *);
// POST body to https://host/path; the answer (whatever the status) into out. false: no connection.
static bool httpsPost(const char *host, const char *path, const char *ctype, const char *body, char *out, int outSize) {
    out[0] = 0; bool ok = false;
    void *net = InternetOpenA("ZankoLauncher", 0, NULL, NULL, 0); if (!net) return false;
    void *con = InternetConnectA(net, host, 443, NULL, NULL, 3 /* INTERNET_SERVICE_HTTP */, 0, 0);
    if (con) { void *req = HttpOpenRequestA(con, "POST", path, NULL, NULL, NULL, 0x00800000 /* SECURE */ | 0x80000000 /* RELOAD */ | 0x04000000 /* NO_CACHE_WRITE */, 0);
        if (req) { char hdr[160]; snprintf(hdr, sizeof hdr, "Content-Type: %s\r\n", ctype);
            if (HttpSendRequestA(req, hdr, (unsigned long)strlen(hdr), (void *)body, (unsigned long)strlen(body))) { int n = 0; unsigned long got = 0; ok = true;
                while (n < outSize - 1 && InternetReadFile(req, out + n, (unsigned long)(outSize - 1 - n), &got) && got > 0) n += (int)got;
                out[n] = 0; }
            InternetCloseHandle(req); }
        InternetCloseHandle(con); }
    InternetCloseHandle(net);
    return ok;
}
#else
static bool httpsPost(const char *host, const char *path, const char *ctype, const char *body, char *out, int outSize) {   // (test build: curl)
    out[0] = 0; char tmp[64]; snprintf(tmp, sizeof tmp, "/tmp/zk_post_%d.txt", (int)getpid()); FILE *f = fopen(tmp, "w"); if (!f) return false; fputs(body, f); fclose(f);
    char cmd[1600]; snprintf(cmd, sizeof cmd, "curl -sS -m 15 -X POST -H \"Content-Type: %s\" --data-binary @%s \"https://%s%s\" 2>/dev/null", ctype, tmp, host, path);
    FILE *p = popen(cmd, "r"); if (!p) { remove(tmp); return false; } int n = (int)fread(out, 1, (size_t)outSize - 1, p); out[n > 0 ? n : 0] = 0; pclose(p); remove(tmp);
    return n > 0;
}
#endif

// the string value of "key" in a JSON answer (first occurrence); false if absent
static bool jsonStr(const char *js, const char *key, char *out, int outSize) {
    char pat[64]; snprintf(pat, sizeof pat, "\"%s\"", key); const char *p = strstr(js, pat); out[0] = 0; if (!p) return false;
    p += strlen(pat); while (*p == ' ' || *p == ':' || *p == '\n' || *p == '\r' || *p == '\t') p++; if (*p != '"') return false; p++;
    int n = 0; while (*p && *p != '"' && n < outSize - 1) { if (*p == '\\' && p[1]) p++; out[n++] = *p++; } out[n] = 0; return true;
}
static bool jsonTrue(const char *js, const char *key) { char pat[64]; snprintf(pat, sizeof pat, "\"%s\"", key); const char *p = strstr(js, pat); if (!p) return false; p += strlen(pat); while (*p == ' ' || *p == ':') p++; return !strncmp(p, "true", 4); }
static void jsonEsc(const char *in, char *out, int outSize) { int n = 0; for (; *in && n < outSize - 3; in++) { unsigned char c = (unsigned char)*in; if (c == '"' || c == '\\') out[n++] = '\\'; if (c >= 32) out[n++] = (char)c; } out[n] = 0; }
static const char *accErrText(const char *js) {   // Firebase's error code in plain words
    char m[96]; if (!jsonStr(js, "message", m, sizeof m)) return "The account server gave no answer.";
    if (strstr(m, "INVALID_LOGIN_CREDENTIALS") || strstr(m, "INVALID_PASSWORD") || strstr(m, "EMAIL_NOT_FOUND")) return "Wrong e-mail or password.";
    if (strstr(m, "INVALID_EMAIL")) return "That is not an e-mail address.";
    if (strstr(m, "USER_DISABLED")) return "This account is disabled.";
    if (strstr(m, "TOO_MANY_ATTEMPTS")) return "Too many attempts. Try again later.";
    if (strstr(m, "API key not valid") || strstr(m, "API_KEY")) return "The account server is not set up in this build.";
    if (strstr(m, "TOKEN_EXPIRED") || strstr(m, "INVALID_REFRESH_TOKEN") || strstr(m, "USER_NOT_FOUND")) return "The saved login has expired. Log in again.";
    return "Login failed.";
}
// name: the account's username (3-16 of letters, digits, _ and -), made safe as a profile file name
static void accCleanName(const char *in, char *out) { int n = 0; for (; *in && n < 16; in++) { char c = *in; if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-') out[n++] = c; } out[n] = 0; if (n < 3 || !strcmp(out, "Guest")) strcpy(out, "Citizen"); }
// the account behind an id token: its username, and whether the e-mail is confirmed
static bool accLookup(const char *idToken, char *name, bool *verified, char *msg, int msgSize) {
    static char body[2600], ans[8192]; char path[200]; snprintf(path, sizeof path, "/v1/accounts:lookup?key=%s", ZK_FIREBASE_KEY); snprintf(body, sizeof body, "{\"idToken\":\"%s\"}", idToken);
    if (!httpsPost("identitytoolkit.googleapis.com", path, "application/json", body, ans, sizeof ans)) { snprintf(msg, msgSize, "No connection to the account server."); return false; }
    if (strstr(ans, "\"error\"")) { snprintf(msg, msgSize, "%s", accErrText(ans)); return false; }
    char dn[64]; jsonStr(ans, "displayName", dn, sizeof dn); accCleanName(dn, name); *verified = jsonTrue(ans, "emailVerified"); return true;
}
// e-mail + password -> the username and a refresh token (kept so the next start needs no password). false: msg says why.
static bool accLogin(const char *email, const char *pw, char *name, char *refresh, int refreshSize, char *msg, int msgSize) {
    if (!ZK_FIREBASE_KEY[0]) { snprintf(msg, msgSize, "Online accounts are not set up in this build."); return false; }
    static char body[900], ans[8192], tok[2400]; char e[300], p[300], path[200]; jsonEsc(email, e, sizeof e); jsonEsc(pw, p, sizeof p);
    snprintf(path, sizeof path, "/v1/accounts:signInWithPassword?key=%s", ZK_FIREBASE_KEY); snprintf(body, sizeof body, "{\"email\":\"%s\",\"password\":\"%s\",\"returnSecureToken\":true}", e, p);
    if (!httpsPost("identitytoolkit.googleapis.com", path, "application/json", body, ans, sizeof ans)) { snprintf(msg, msgSize, "No connection to the account server."); return false; }
    if (strstr(ans, "\"error\"") || !jsonStr(ans, "idToken", tok, sizeof tok)) { snprintf(msg, msgSize, "%s", accErrText(ans)); return false; }
    jsonStr(ans, "refreshToken", refresh, refreshSize); bool ver = false;
    if (!accLookup(tok, name, &ver, msg, msgSize)) return false;
    if (!ver) { refresh[0] = 0; snprintf(msg, msgSize, "Confirm the e-mail first: open the link sent to it."); return false; }
    return true;
}
// a saved refresh token -> still a valid, confirmed account? (start-up: straight in without the password)
static bool accResume(char *refresh, int refreshSize, char *name, char *msg, int msgSize) {
    if (!ZK_FIREBASE_KEY[0] || !refresh[0]) return false;
    static char body[1200], ans[8192], tok[2400]; char path[200]; snprintf(path, sizeof path, "/v1/token?key=%s", ZK_FIREBASE_KEY); snprintf(body, sizeof body, "grant_type=refresh_token&refresh_token=%s", refresh);
    if (!httpsPost("securetoken.googleapis.com", path, "application/x-www-form-urlencoded", body, ans, sizeof ans)) { snprintf(msg, msgSize, "No connection to the account server."); return false; }
    if (strstr(ans, "\"error\"") || !jsonStr(ans, "id_token", tok, sizeof tok)) { snprintf(msg, msgSize, "%s", accErrText(ans)); refresh[0] = 0; return false; }
    { char nr[1024]; if (jsonStr(ans, "refresh_token", nr, sizeof nr) && nr[0]) snprintf(refresh, refreshSize, "%s", nr); }
    bool ver = false; if (!accLookup(tok, name, &ver, msg, msgSize)) return false;
    return ver;
}
