// zk_anim.h — first-person hands and every viewmodel animation: idle breathing, sway, sprint, landing, swings, guard,
// weapon inspects (blade turn + flip, revolver trigger spin, knuckle crack), eating, drinking, bandaging, reading
// manga with turning pages, searching, taking, doors, climbing, reloading, building. Smooth eased curves throughout.

#ifdef _WIN32
__declspec(dllimport) void __stdcall glClear(unsigned int);
#else
extern void glClear(unsigned int);
#endif
static float ez(float t) { t = clampf(t, 0, 1); return t * t * (3 - 2 * t); }                 // smoothstep
static float ezOut(float t) { t = clampf(t, 0, 1); return 1 - (1 - t) * (1 - t) * (1 - t); }
static float ezBack(float t) { t = clampf(t, 0, 1); float c = 1.70158f; return 1 + (c + 1) * powf(t - 1, 3) + c * powf(t - 1, 2); }
static float ph(float u, float a, float b) { return ez((u - a) / (b - a)); }                     // eased progress of a phase
static float bell(float u, float a, float b) { float x = clampf((u - a) / (b - a), 0, 1); return sinf(x * PI); }

typedef struct { Vector3 p, r; int grip; bool vis; } HandP;   // grip 0 closed around a handle, 1 open, 2 fist
static Color skinC = {206, 172, 150, 255};

static void drawHandAsset(Matrix H, bool right, int grip) {
    float s = right ? 1.0f : -1.0f;
    if (grip == 0) {   // wrapped around a vertical handle through the origin
        if (right) ADRAW("viewmodels/hand_grip_R", H, skinC, BX(T_SKIN, 0.045f, 0, 0.012f, 0.03f, 0.095f, 0.085f, CWH),
                         BX(T_SKIN, 0.0f, 0.034f, -0.034f, 0.085f, 0.02f, 0.028f, CWH), BX(T_SKIN, 0.0f, 0.012f, -0.036f, 0.086f, 0.02f, 0.028f, CWH),
                         BX(T_SKIN, 0.0f, -0.010f, -0.035f, 0.084f, 0.02f, 0.027f, CWH), BX(T_SKIN, 0.0f, -0.031f, -0.032f, 0.078f, 0.018f, 0.025f, CWH),
                         BX(T_SKIN, -0.02f, 0.052f, 0.0f, 0.03f, 0.024f, 0.06f, CWH), BX(T_SKIN, 0.045f, -0.07f, 0.03f, 0.07f, 0.05f, 0.08f, CWH));
        else ADRAW("viewmodels/hand_grip_L", H, skinC, BX(T_SKIN, -0.045f, 0, 0.012f, 0.03f, 0.095f, 0.085f, CWH),
                   BX(T_SKIN, 0.0f, 0.034f, -0.034f, 0.085f, 0.02f, 0.028f, CWH), BX(T_SKIN, 0.0f, 0.012f, -0.036f, 0.086f, 0.02f, 0.028f, CWH),
                   BX(T_SKIN, 0.0f, -0.010f, -0.035f, 0.084f, 0.02f, 0.027f, CWH), BX(T_SKIN, 0.0f, -0.031f, -0.032f, 0.078f, 0.018f, 0.025f, CWH),
                   BX(T_SKIN, 0.02f, 0.052f, 0.0f, 0.03f, 0.024f, 0.06f, CWH), BX(T_SKIN, -0.045f, -0.07f, 0.03f, 0.07f, 0.05f, 0.08f, CWH));
    } else if (grip == 1) {   // open hand, palm down, fingers forward
        if (right) ADRAW("viewmodels/hand_open_R", H, skinC, BX(T_SKIN, 0, 0, 0, 0.085f, 0.026f, 0.09f, CWH),
                         BX(T_SKIN, -0.03f, 0, -0.072f, 0.017f, 0.018f, 0.058f, CWH), BX(T_SKIN, -0.01f, 0, -0.078f, 0.017f, 0.018f, 0.068f, CWH),
                         BX(T_SKIN, 0.01f, 0, -0.076f, 0.017f, 0.018f, 0.064f, CWH), BX(T_SKIN, 0.03f, 0, -0.068f, 0.016f, 0.017f, 0.05f, CWH),
                         BX(T_SKIN, -0.058f, -0.004f, -0.018f, 0.022f, 0.02f, 0.05f, CWH));
        else ADRAW("viewmodels/hand_open_L", H, skinC, BX(T_SKIN, 0, 0, 0, 0.085f, 0.026f, 0.09f, CWH),
                   BX(T_SKIN, 0.03f, 0, -0.072f, 0.017f, 0.018f, 0.058f, CWH), BX(T_SKIN, 0.01f, 0, -0.078f, 0.017f, 0.018f, 0.068f, CWH),
                   BX(T_SKIN, -0.01f, 0, -0.076f, 0.017f, 0.018f, 0.064f, CWH), BX(T_SKIN, -0.03f, 0, -0.068f, 0.016f, 0.017f, 0.05f, CWH),
                   BX(T_SKIN, 0.058f, -0.004f, -0.018f, 0.022f, 0.02f, 0.05f, CWH));
    } else {   // fist, knuckles forward
        if (right) ADRAW("viewmodels/fist_R", H, skinC, BX(T_SKIN, 0, 0, 0, 0.088f, 0.08f, 0.09f, CWH), BX(T_SKIN, 0, 0.012f, -0.05f, 0.09f, 0.05f, 0.022f, ((Color){226, 190, 168, 255})),
                         BX(T_SKIN, -0.048f, -0.01f, -0.02f, 0.022f, 0.045f, 0.05f, CWH));
        else ADRAW("viewmodels/fist_L", H, skinC, BX(T_SKIN, 0, 0, 0, 0.088f, 0.08f, 0.09f, CWH), BX(T_SKIN, 0, 0.012f, -0.05f, 0.09f, 0.05f, 0.022f, ((Color){226, 190, 168, 255})),
                   BX(T_SKIN, 0.048f, -0.01f, -0.02f, 0.022f, 0.045f, 0.05f, CWH));
    }
    (void)s;
}
// a sleeve from the wrist back to a shoulder anchor; both points in camera-local space
static void drawArm(Matrix B, Vector3 wrist, Vector3 shoulder) {
    Vector3 d = Vector3Subtract(shoulder, wrist); float L = Vector3Length(d); if (L < 0.01f) return;
    Vector3 z = Vector3Scale(d, 1 / L), x = Vector3Normalize(Vector3CrossProduct(V3(0, 1, 0), z)), y = Vector3CrossProduct(z, x);
    Matrix A = {x.x * 0.12f, y.x * 0.12f, z.x * L, wrist.x, x.y * 0.12f, y.y * 0.12f, z.y * L, wrist.y, x.z * 0.12f, y.z * 0.12f, z.z * L, wrist.z, 0, 0, 0, 1};
    Matrix M = MatrixMultiply(A, B);
    ADRAW("viewmodels/sleeve", M, WHITE, BX(T_CLOTH, 0, 0, 0.55f, 1.0f, 1.0f, 0.9f, ((Color){52, 56, 64, 255})), BX(T_CLOTH, 0, 0, 0.06f, 1.12f, 1.12f, 0.12f, ((Color){36, 38, 44, 255})));
}
static Matrix handM(Matrix B, HandP h) { return frameM(B, h.p, h.r.x, h.r.y, h.r.z); }
static void drawHand(Matrix B, HandP h, bool right) {
    if (!h.vis) return;
    Matrix H = handM(B, h);
    Vector3 wrist = Vector3Transform(V3(right ? 0.04f : -0.04f, -0.06f, 0.06f), MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixRotateZ(h.r.z), MatrixRotateX(h.r.x)), MatrixRotateY(h.r.y)), MatrixTranslate(h.p.x, h.p.y, h.p.z)));
    drawArm(B, wrist, V3(right ? 0.32f : -0.32f, -0.62f, 0.32f));
    drawHandAsset(H, right, h.grip);
}
// the manga, held open: two pages, and one turning when a flip is under way
static void drawManga(Matrix B, Vector3 c, float tilt, float flip, int page) {
    float W = 0.13f, Hh = 0.18f; Matrix Mb = frameM(B, c, tilt, 0, 0);
    ADRAW("viewmodels/manga_cover", Mb, WHITE, BX(T_PAPER, 0, 0, -0.005f, W * 2 + 0.012f, Hh + 0.01f, 0.006f, ((Color){180, 40, 40, 255})));
    Texture2D t = mangaRT.texture; int pL = page % MANGA_PAGES, pR = (page + 1) % MANGA_PAGES, pN = (page + 2) % MANGA_PAGES, pB = (page + 3) % MANGA_PAGES;
    #define PT(X, Y) Vector3Transform(V3(X, Y, 0), Mb)
    rlDrawRenderBatchActive(); setEmis(0);
    texQuad(t, PT(-W, Hh / 2), PT(0, Hh / 2), PT(0, -Hh / 2), PT(-W, -Hh / 2), (Rectangle){(float)pL * MANGA_PW, 0, MANGA_PW, MANGA_PH}, WHITE, true);
    texQuad(t, PT(0, Hh / 2), PT(W, Hh / 2), PT(W, -Hh / 2), PT(0, -Hh / 2), (Rectangle){(float)(flip > 0 ? pN : pR) * MANGA_PW, 0, MANGA_PW, MANGA_PH}, WHITE, true);
    if (flip > 0) {   // the turning leaf rotates about the spine, front then back
        float a = flip * PI, cx = cosf(a) * W, cz = sinf(a) * W * 0.6f;
        Vector3 t0 = Vector3Transform(V3(0, Hh / 2, 0.002f), Mb), t1 = Vector3Transform(V3(cx, Hh / 2, cz + 0.002f), Mb), b1 = Vector3Transform(V3(cx, -Hh / 2, cz + 0.002f), Mb), b0 = Vector3Transform(V3(0, -Hh / 2, 0.002f), Mb);
        texQuad(t, t0, t1, b1, b0, (Rectangle){(float)(flip < 0.5f ? pR : pB) * MANGA_PW, 0, MANGA_PW, MANGA_PH}, flip < 0.5f ? WHITE : (Color){220, 220, 215, 255}, true);
    }
    #undef PT
}

// ---- the viewmodel
static void drawViewmodel2(Vector3 eye, Vector3 aim) {
    Vector3 f = aim, r = Vector3Normalize(Vector3CrossProduct(f, V3(0, 1, 0))), u = Vector3CrossProduct(r, f);
    Matrix B = {r.x, u.x, -f.x, eye.x, r.y, u.y, -f.y, eye.y, r.z, u.z, -f.z, eye.z, 0, 0, 0, 1};
    float t = (float)GetTime();
    float bx = sinf(pl.bob) * 0.012f * (1 + pl.sprintK), by = fabsf(cosf(pl.bob)) * 0.012f * (1 + pl.sprintK * 1.5f);
    float br = sinf(t * 1.6f) * 0.004f;   // breathing
    Vector3 sway = V3(pl.swayX, pl.swayY - pl.landK * 0.09f + br, 0);
    rlDrawRenderBatchActive();
    glClear(0x00000100);   // depth only: the hands never sink into walls, but sort correctly among themselves
    setEmis(0);
    int ht = pl.hands.t; bool melee = ht >= 0 && idef[ht].cat == CAT_MELEE, gun = ht == IT_REVOLVER;
    float u2 = animDur > 0 ? clampf(animT / animDur, 0, 1) : 1; int A = animId != ANIM_NONE && animT < animDur ? animId : ANIM_NONE;
    HandP R = {V3(0.22f, -0.25f, -0.44f), V3(-0.95f, 0.12f, -0.3f), 0, true}, L = {V3(-0.22f, -0.32f, -0.42f), V3(-0.3f, 0, 0.2f), 1, false};
    float spinX = 0, spinY = 0;   // item spin inside the hand (inspect flips)
    float heavy = ht >= 0 ? clampf(idef[ht].weight / 5.0f, 0.1f, 1) : 0.05f;
    // ---- idle poses per held thing
    if (pl.build) { R = (HandP){V3(0.22f, -0.25f, -0.4f), V3(-0.3f, 0, 0), 0, true}; }
    else if (gun) { R = (HandP){V3(0.15f, -0.215f, -0.36f), V3(0.04f, 0.06f, 0), 0, true}; }   // one-handed: the left hand is free for the flashlight
    else if (!melee) { R = (HandP){V3(0.17f, -0.21f, -0.36f), V3(-0.25f, 0.15f, -0.12f), 2, true}; L = (HandP){V3(-0.17f, -0.22f, -0.37f), V3(-0.25f, -0.15f, 0.12f), 2, true}; }
    else if (heavy > 0.45f) L.vis = true;   // two-handed: the left hand grips below the right
    // two hand sets: the left holds the flashlight while it is on, the right holds fists or the weapon
    bool torch = pl.flash && !pl.build && !(melee && heavy > 0.45f);
    if (torch) L = (HandP){V3(-0.13f, -0.165f, -0.47f), V3(0.3f, 0.32f, 0.05f), 0, true};   // the phone, upright in the palm, screen toward you, light on the back
    // ---- melee swing (wind-up, strike, recovery) and the fist jab
    float dur = pl.swingDur > 0 ? pl.swingDur : 0.6f, s = pl.swingT > 0 ? 1 - pl.swingT / dur : 0;
    float wu = ez(s / 0.3f), st = ezOut((s - 0.3f) / 0.15f), rc = ez((s - 0.45f) / 0.55f);
    float shakeHit = pl.hitStopT > 0 ? fr(-0.03f, 0.03f) : 0;
    if (melee && !pl.build && pl.swingT > 0) {
        R.r.x = -0.95f + wu * (1.35f + heavy * 0.6f) - st * (2.4f + heavy * 0.4f) + rc * (1.6f + heavy * 0.4f) * (st >= 1 ? 1 : 0);
        R.r.z = -0.3f - wu * 0.65f + st * 1.6f - rc * 1.2f * (st >= 1 ? 1 : 0);
        R.p.x = 0.22f + wu * 0.08f - st * 0.32f + rc * 0.24f * (st >= 1 ? 1 : 0) + shakeHit; R.p.y = -0.25f + wu * 0.1f - st * 0.12f + rc * 0.12f * (st >= 1 ? 1 : 0);
    } else if (!melee && !gun && !pl.build && pl.swingT > 0) {
        float jab = bell(s, 0.15f, 0.7f); R.p.z -= jab * 0.26f; R.p.x -= jab * 0.09f; R.p.y += jab * 0.05f; R.r.x += jab * 0.25f; L.p.z += jab * 0.04f; R.p.x += shakeHit;
    }
    // ---- guard
    float gk = pl.blockK;
    if (gk > 0.001f) {
        if (melee) { R.p = Vector3Lerp(R.p, V3(0.12f, -0.2f, -0.42f), gk); R.r = Vector3Lerp(R.r, V3(-0.3f, 0.0f, 0.85f), gk); }   // blade raised diagonally across the body
        else if (!gun) { R.p = Vector3Lerp(R.p, V3(0.07f, -0.07f, -0.3f), gk); R.r.x -= gk * 0.4f;
            if (torch) L.p = Vector3Lerp(L.p, V3(-0.12f, -0.12f, -0.36f), gk); else { L.p = Vector3Lerp(L.p, V3(-0.07f, -0.08f, -0.31f), gk); L.r.x -= gk * 0.4f; } }
    }
    // ---- one-shot animations
    switch (A) {
    case ANIM_EQUIP: { float e = ezBack(u2); R.p.y -= (1 - e) * 0.38f; R.r.x += (1 - e) * 1.1f; R.r.z -= (1 - e) * 0.5f; L.p.y -= (1 - e) * 0.38f; break; }
    case ANIM_HOLSTER: { float e = ez(u2); R.p.y -= e * 0.4f; R.r.x += e * 0.9f; L.p.y -= e * 0.4f; break; }
    case ANIM_INSPECT:
        if (melee) {
            float a = ph(u2, 0, 0.16f), b = ph(u2, 0.16f, 0.42f), c = ph(u2, 0.45f, 0.62f), d = ph(u2, 0.62f, 0.8f), e = ph(u2, 0.8f, 1);
            Vector3 up = V3(0.08f, -0.17f, -0.42f);
            R.p = Vector3Lerp(R.p, up, a * (1 - e)); R.r.z = Lerp(R.r.z, -1.45f, a * (1 - e)); R.r.y += (0.7f * a - 1.4f * b) * (1 - e); R.r.x += 0.25f * a * (1 - e);
            spinX = c * 2 * PI; R.p.y += bell(u2, 0.45f, 0.62f) * 0.07f;   // toss and flip
            if (u2 > 0.6f && u2 < 0.82f) { L.vis = true; L.grip = 1; float sl = d; L.p = Vector3Add(R.p, V3(-0.12f + sl * 0.18f, 0.03f, -0.03f)); L.r = V3(0.1f, 0, -0.3f); }
        } else if (gun) {
            float a = ph(u2, 0, 0.15f), b = ph(u2, 0.15f, 0.33f), c = ph(u2, 0.36f, 0.7f), d = bell(u2, 0.7f, 0.86f), e = ph(u2, 0.86f, 1);
            R.p = Vector3Lerp(R.p, V3(0.06f, -0.15f, -0.36f), a * (1 - e)); R.r.z += (-0.9f * a + 1.6f * b) * (1 - e); R.r.y += 0.5f * a * (1 - e);
            spinX = -c * 4 * PI; R.r.x += d * 0.7f; L.vis = u2 < 0.12f || u2 > 0.9f;
        } else {
            float a = ph(u2, 0, 0.18f), crack = bell(u2, 0.2f, 0.55f), shake = bell(u2, 0.58f, 0.85f), e = ph(u2, 0.85f, 1);
            R.p = Vector3Lerp(R.p, V3(0.03f, -0.16f, -0.32f), a * (1 - e)); L.p = Vector3Lerp(L.p, V3(-0.04f, -0.16f, -0.33f), a * (1 - e));
            R.r.y -= crack * 0.6f; L.r.y += crack * 0.3f; R.p.x -= crack * 0.02f + (crack > 0.9f ? fr(-0.006f, 0.006f) : 0);
            R.p.y += sinf(t * 30) * 0.015f * shake; L.p.y += cosf(t * 27) * 0.015f * shake; R.grip = L.grip = shake > 0.1f ? 1 : 2;
        }
        break;
    case ANIM_EAT: case ANIM_PILL: case ANIM_DRINK: {
        float lift = A == ANIM_PILL ? bell(u2, 0, 1) : ph(u2, 0, 0.28f) * (1 - ph(u2, 0.8f, 1));
        L.vis = true; L.grip = 0; L.p = Vector3Lerp(V3(-0.2f, -0.45f, -0.38f), V3(-0.01f, -0.1f, -0.17f), lift);
        L.r = V3(Lerp(-0.3f, A == ANIM_DRINK ? -1.35f : 0.2f, lift), Lerp(0, 0.9f, lift), 0);
        if (A == ANIM_EAT) L.p.y += sinf(u2 * 22) * 0.012f * lift;
        R.p.y -= lift * 0.12f;
        break; }
    case ANIM_BANDAGE: {
        float in = ph(u2, 0, 0.18f) * (1 - ph(u2, 0.86f, 1)), ang = ph(u2, 0.15f, 0.85f) * 4 * PI;
        L.vis = true; L.grip = 1; L.p = Vector3Lerp(L.p, V3(-0.04f, -0.27f, -0.36f), in); L.r = V3(0.1f, 0.4f, 0);
        R.grip = 0; R.p = Vector3Lerp(R.p, Vector3Add(L.p, V3(0.07f * cosf(ang), 0.05f * sinf(ang), -0.02f)), in); R.r = Vector3Lerp(R.r, V3(0.3f * sinf(ang), 0, 0.4f * cosf(ang)), in);
        break; }
    case ANIM_TAKE: { float k = bell(u2, 0, 1); R.p = Vector3Lerp(R.p, V3(0.1f, -0.16f, -0.62f), k); R.grip = k > 0.3f ? 1 : R.grip; break; }
    case ANIM_DOOR: { float k = bell(u2, 0, 1); R.p = Vector3Lerp(R.p, V3(0.07f, -0.08f, -0.58f), k); R.r = Vector3Lerp(R.r, V3(-1.4f, 0, 0), k); R.grip = k > 0.25f ? 1 : R.grip; break; }
    case ANIM_RELOAD: if (gun) {
        float a = ph(u2, 0, 0.15f), e = ph(u2, 0.86f, 1), load = clampf((u2 - 0.35f) / 0.45f, 0, 1);
        R.p = Vector3Lerp(R.p, V3(0.04f, -0.2f, -0.36f), a * (1 - e)); R.r.z = Lerp(R.r.z, -1.0f, a * (1 - e)); R.r.x += 0.45f * a * (1 - e);
        L.vis = true; L.grip = 1; float bob = fabsf(sinf(load * 4 * PI)) * (u2 > 0.35f && u2 < 0.8f ? 1 : 0);
        L.p = V3(-0.05f, -0.24f - bob * 0.08f + (u2 < 0.35f ? bell(u2, 0.22f, 0.35f) * 0.05f : 0), -0.33f); L.r = V3(0.6f, 0.3f, 0.2f);
        R.r.z += bell(u2, 0.8f, 0.88f) * 0.6f;   // flick it shut
    } break;
    case ANIM_BUILD: { float k = bell(u2, 0, 1); R.p.y -= k * 0.06f; R.r.x -= k * 0.5f; break; }
    }
    // ---- searching, climbing, sprinting
    if (pl.searchC >= 0) { R.vis = L.vis = true; R.grip = L.grip = 1;
        R.p = V3(0.12f + sinf(t * 9) * 0.03f, -0.27f + fabsf(sinf(t * 7)) * 0.03f, -0.5f + cosf(t * 8) * 0.03f); L.p = V3(-0.12f + cosf(t * 8.5f) * 0.03f, -0.27f + fabsf(cosf(t * 6.5f)) * 0.03f, -0.5f + sinf(t * 7.5f) * 0.03f);
        R.r = L.r = V3(0.5f, 0, 0); }
    if (pl.climbT > 0 && pl.climbDur > 0) { float k = 1 - pl.climbT / pl.climbDur; R.vis = L.vis = true; R.grip = L.grip = 1;
        float lift = bell(k, 0, 1); R.p = V3(0.18f, -0.15f + lift * 0.12f - k * 0.18f, -0.48f); L.p = V3(-0.18f, -0.15f + lift * 0.12f - k * 0.18f, -0.48f); R.r = L.r = V3(0.2f, 0, 0); }
    if (pl.sprintK > 0.01f && pl.swingT <= 0 && A == ANIM_NONE && gk < 0.1f) {
        float k = pl.sprintK; R.p = Vector3Add(R.p, V3(0.04f * k, -0.07f * k + sinf(pl.bob * 2) * 0.02f * k, 0.05f * k)); R.r.z += 0.45f * k; R.r.x += 0.25f * k;
        L.p = Vector3Add(L.p, V3(-0.02f * k, -0.07f * k + cosf(pl.bob * 2) * 0.02f * k, 0.04f * k));
    }
    R.p = Vector3Add(R.p, Vector3Add(sway, V3(bx, by, 0))); L.p = Vector3Add(L.p, Vector3Add(sway, V3(bx * 0.8f, by, 0)));
    R.r.z += pl.swayX * 1.5f; L.r.z += pl.swayX * 1.5f;
    // ---- reading: both hands hold the open book
    if (pl.watchT > 0 && pl.watchKind == 2) {
        float in = ez((5 - pl.watchT) / 0.5f) * ez(pl.watchT / 0.5f);
        Vector3 bc = V3(0, Lerp(-0.45f, -0.1f, in), -0.33f);
        drawManga(B, bc, -0.42f, pl.readFlip, (int)((5 - pl.watchT) / 1.25f) * 2);
        HandP l2 = {Vector3Add(bc, V3(-0.15f, -0.02f, 0.02f)), V3(0.6f, 0.2f, -1.4f), 0, true}, r2 = {Vector3Add(bc, V3(0.15f, -0.02f, 0.02f)), V3(0.6f, -0.2f, 1.4f), 0, true};
        drawHand(B, l2, false); drawHand(B, r2, true);
        rlDrawRenderBatchActive(); return;
    }
    if (pl.sleeping) { rlDrawRenderBatchActive(); return; }
    // ---- draw hands and what they hold
    if (R.vis) {
        Matrix H = handM(B, R);
        if (pl.build) { ADRAW("viewmodels/build_plank", H, WHITE, BX(T_WOOD, 0, 0, -0.05f, 0.5f, 0.05f, 0.12f, CWH)); drawHandAsset(H, true, 0); }
        else if (gun) {
            // a .38 revolver: wooden grip in the fist, steel frame above it, cylinder, barrel with front sight, hammer, trigger guard
            Matrix P = MatrixMultiply(MatrixMultiply(MatrixTranslate(0, -0.085f, 0.05f), MatrixRotateX(spinX)), MatrixTranslate(0, 0.085f, -0.05f));   // spin about the cylinder axis line
            Matrix G = MatrixMultiply(P, MatrixMultiply(MatrixTranslate(0, -0.01f, pl.kick * 0.05f), MatrixMultiply(MatrixRotateX(pl.kick * 0.3f), H)));
            ADRAW("viewmodels/revolver3", G, WHITE,
                  BX(T_WOOD, 0, -0.005f, 0.012f, 0.03f, 0.11f, 0.045f, ((Color){110, 70, 42, 255})),          // grip
                  BX(T_METAL, 0, 0.065f, -0.005f, 0.026f, 0.04f, 0.075f, ((Color){62, 64, 70, 255})),          // frame
                  BX(T_METAL, 0, 0.09f, -0.135f, 0.018f, 0.02f, 0.17f, ((Color){50, 52, 58, 255})),            // barrel
                  BX(T_METAL, 0, 0.076f, -0.13f, 0.012f, 0.008f, 0.15f, ((Color){44, 46, 52, 255})),           // ejector rod housing
                  BX(T_METAL, 0, 0.106f, -0.212f, 0.004f, 0.012f, 0.012f, ((Color){40, 40, 44, 255})),         // front sight
                  BX(T_METAL, 0, 0.1f, 0.03f, 0.01f, 0.025f, 0.02f, ((Color){40, 40, 46, 255})),               // hammer
                  BX(T_METAL, 0, 0.03f, -0.035f, 0.008f, 0.03f, 0.045f, ((Color){50, 50, 56, 255})),           // trigger guard
                  BX(T_METAL, 0, 0.04f, -0.03f, 0.004f, 0.02f, 0.006f, ((Color){30, 30, 34, 255})));           // trigger
            float cylOut = A == ANIM_RELOAD ? ph(u2, 0.15f, 0.25f) * (1 - ph(u2, 0.8f, 0.86f)) : 0;
            ADRAW("viewmodels/revolver3_cyl", MatrixMultiply(MatrixTranslate(-cylOut * 0.05f, 0.083f, -0.05f), G), WHITE, BX(T_METAL, 0, 0, 0, 0.044f, 0.044f, 0.05f, ((Color){78, 80, 86, 255})),
                  BX(T_METAL, 0, 0, 0, 0.05f, 0.03f, 0.044f, ((Color){70, 72, 78, 255})));
            if (pl.muzzle > 0) { setEmis(1); ADRAW("viewmodels/muzzle_flash3", G, WHITE, BX(T_PLAIN, 0, 0.09f, -0.25f, 0.09f, 0.09f, 0.06f, ((Color){255, 220, 120, 255}))); setEmis(0); }
            drawHandAsset(H, true, 0);
        } else if (melee) {
            Matrix I = MatrixMultiply(MatrixMultiply(MatrixRotateX(spinX), MatrixRotateY(spinY)), H);
            drawHeld(I, ht); drawHandAsset(H, true, 0);
            if (L.vis && !torch && A != ANIM_INSPECT) { Matrix L2 = MatrixMultiply(MatrixTranslate(0, -0.17f, 0), H); drawHandAsset(L2, false, 0); L.vis = false; }
        } else drawHandAsset(H, true, R.grip);
        Vector3 wrist = Vector3Transform(V3(0.04f, -0.06f, 0.06f), MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixRotateZ(R.r.z), MatrixRotateX(R.r.x)), MatrixRotateY(R.r.y)), MatrixTranslate(R.p.x, R.p.y, R.p.z)));
        drawArm(B, wrist, V3(0.32f, -0.62f, 0.32f));
    }
    bool showTorch = torch && L.vis && L.grip == 0 && pl.searchC < 0 && pl.climbT <= 0 && A != ANIM_EAT && A != ANIM_DRINK && A != ANIM_PILL && A != ANIM_BANDAGE && A != ANIM_RELOAD;
    if (L.vis) {
        drawHand(B, L, false);
        if (showTorch) {   // a 2006 flip phone (keitai): keypad half in the hand, screen half open above it; the back LED is the light
            Matrix Hl = handM(B, L), Top = MatrixMultiply(MatrixMultiply(MatrixTranslate(0, 0.0f, 0), MatrixRotateX(-0.22f)), MatrixMultiply(MatrixTranslate(0, 0.075f, 0.0f), Hl));
            ADRAW("viewmodels/keitai_base", Hl, WHITE, BX(T_PLAIN, 0, 0.02f, 0.004f, 0.05f, 0.1f, 0.018f, ((Color){150, 30, 40, 255})), BX(T_PLAIN, 0, 0.02f, 0.0135f, 0.042f, 0.085f, 0.002f, ((Color){200, 200, 205, 255})),
                  BX(T_PLAIN, 0, 0.072f, 0.004f, 0.05f, 0.012f, 0.02f, ((Color){60, 60, 66, 255})));
            ADRAW("viewmodels/keitai_top", Top, WHITE, BX(T_PLAIN, 0, 0.05f, 0.004f, 0.05f, 0.1f, 0.016f, ((Color){150, 30, 40, 255})), BX(T_PLAIN, 0, 0.083f, -0.0045f, 0.022f, 0.012f, 0.002f, ((Color){30, 30, 34, 255})));
            setEmis(1);
            ADRAW("viewmodels/keitai_screen", Top, WHITE, BX(T_PLAIN, 0, 0.055f, 0.0125f, 0.038f, 0.05f, 0.002f, pl.batt > 0 ? ((Color){150, 210, 255, 255}) : ((Color){20, 24, 30, 255})));
            if (pl.batt > 0) ADRAW("viewmodels/keitai_led", Top, WHITE, BX(T_PLAIN, 0, 0.083f, -0.0058f, 0.01f, 0.01f, 0.002f, ((Color){255, 250, 230, 255})));
            setEmis(0);
        }
        if ((A == ANIM_EAT || A == ANIM_DRINK || A == ANIM_PILL) && animItem >= 0) {   // the food or bottle in the left hand
            Matrix Hl = handM(B, L); float bite = A == ANIM_EAT ? 1 - 0.5f * ph(u2, 0.35f, 0.8f) : 1;
            drawItemType(animItem, MatrixMultiply(MatrixMultiply(MatrixScale(bite, bite, bite), MatrixTranslate(0, -0.05f, -0.01f)), Hl));
        }
    }
    if (A == ANIM_BANDAGE && animItem >= 0) drawItemType(animItem, MatrixMultiply(MatrixTranslate(0, -0.03f, -0.02f), handM(B, R)));
    rlDrawRenderBatchActive();
}
// eyelids: closing when you fall asleep, opening when you wake
static void drawEyelids(float W, float H) {
    float k = pl.eyeK; if (k <= 0.002f) return;
    float h = H / 2 * k;
    for (int i = 0; i < 24; i++) { float x0 = W * i / 24, x1 = W * (i + 1) / 24, xm = (x0 + x1) / 2 - W / 2, curve = (1 - (xm * xm) / (W * W / 4)) * H * 0.06f * (1 - k);
        DrawRectangle((int)x0, 0, (int)(x1 - x0) + 1, (int)(h + curve), BLACK); DrawRectangle((int)x0, (int)(H - h - curve), (int)(x1 - x0) + 1, (int)(h + curve) + 1, BLACK); }
}

// ---- item icons: each item's own 3D model rendered once into a small texture, three-quarter view, lit flat
#define ICON_PX 96
static RenderTexture2D icons[IT_N]; static bool iconOk = false;
static void iconsInit(void) {
    float amb = 1.35f, fogD = 0, zero = 0, one = 1; int nl = 0; float snap[2] = {ICON_PX * 1.5f, ICON_PX * 1.5f};
    SetShaderValue(sh, locAmb, &amb, SHADER_UNIFORM_FLOAT); SetShaderValue(sh, locFogD, &fogD, SHADER_UNIFORM_FLOAT); SetShaderValue(sh, locFlashOn, &zero, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, locMuzzle, &zero, SHADER_UNIFORM_FLOAT); SetShaderValue(sh, locNL, &nl, SHADER_UNIFORM_INT); SetShaderValue(sh, locDesat, &zero, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sh, locRage, &zero, SHADER_UNIFORM_FLOAT); SetShaderValue(sh, locSnap, snap, SHADER_UNIFORM_VEC2);
    for (int t = 0; t < IT_N; t++) {
        icons[t] = LoadRenderTexture(ICON_PX, ICON_PX); SetTextureFilter(icons[t].texture, TEXTURE_FILTER_POINT);
        int cat = idef[t].cat; float size = (cat == CAT_MELEE || cat == CAT_GUN) ? (t == IT_KNIFE || t == IT_HAMMER || t == IT_REVOLVER ? 0.5f : 1.0f) : cat == CAT_BAG ? 0.8f : t == IT_PLANKS ? 1.1f : (t == IT_WATER || t == IT_DRINK || t == IT_DIRTYWATER || t == IT_BOTTLE) ? 0.42f : 0.3f;
        Vector3 ctr = V3(0, size * 0.18f, 0);
        Camera3D c = {Vector3Add(ctr, Vector3Scale(Vector3Normalize(V3(0.9f, 1.0f, 1.2f)), 4)), ctr, V3(0, 1, 0), size, CAMERA_ORTHOGRAPHIC};
        SetShaderValue(sh, locView, &c.position, SHADER_UNIFORM_VEC3);
        BeginTextureMode(icons[t]); ClearBackground(BLANK); BeginMode3D(c);
        setEmis(0);
        drawItemType(t, MatrixMultiply(MatrixRotateY((cat == CAT_MELEE || cat == CAT_GUN || t == IT_PLANKS) ? 0.75f : 0.3f), MatrixIdentity()));
        EndMode3D(); EndTextureMode();
    }
    (void)one; iconOk = true;
}
