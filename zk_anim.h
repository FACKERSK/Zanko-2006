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

// hand models. The thumb always joins the palm through the web / thenar pad (the ball of the thumb), so nothing floats.
// grip 3 = the revolver grip without the thumb: the thumb is drawn separately so it can reach the hammer.
static void drawHandAsset(Matrix H, bool right, int grip) {
    if (grip == 0 || grip == 3) {   // wrapped around a vertical handle through the origin
        if (right) ADRAW("viewmodels/hand_grip2_R", H, skinC, BX(T_SKIN, 0.045f, 0, 0.012f, 0.03f, 0.095f, 0.085f, CWH),
                         BX(T_SKIN, 0.0f, 0.034f, -0.034f, 0.085f, 0.02f, 0.028f, CWH), BX(T_SKIN, 0.0f, 0.012f, -0.036f, 0.086f, 0.02f, 0.028f, CWH),
                         BX(T_SKIN, 0.0f, -0.010f, -0.035f, 0.084f, 0.02f, 0.027f, CWH), BX(T_SKIN, 0.0f, -0.031f, -0.032f, 0.078f, 0.018f, 0.025f, CWH),
                         BX(T_SKIN, 0.012f, 0.047f, 0.03f, 0.062f, 0.03f, 0.042f, CWH), BX(T_SKIN, 0.045f, -0.07f, 0.03f, 0.07f, 0.05f, 0.08f, CWH));
        else ADRAW("viewmodels/hand_grip2_L", H, skinC, BX(T_SKIN, -0.045f, 0, 0.012f, 0.03f, 0.095f, 0.085f, CWH),
                   BX(T_SKIN, 0.0f, 0.034f, -0.034f, 0.085f, 0.02f, 0.028f, CWH), BX(T_SKIN, 0.0f, 0.012f, -0.036f, 0.086f, 0.02f, 0.028f, CWH),
                   BX(T_SKIN, 0.0f, -0.010f, -0.035f, 0.084f, 0.02f, 0.027f, CWH), BX(T_SKIN, 0.0f, -0.031f, -0.032f, 0.078f, 0.018f, 0.025f, CWH),
                   BX(T_SKIN, -0.012f, 0.047f, 0.03f, 0.062f, 0.03f, 0.042f, CWH), BX(T_SKIN, -0.045f, -0.07f, 0.03f, 0.07f, 0.05f, 0.08f, CWH));
        if (grip == 0) {   // the thumb, from the pad over the top of the handle and down its far side
            if (right) ADRAW("viewmodels/thumb_grip2_R", H, skinC, BX(T_SKIN, -0.012f, 0.05f, 0.012f, 0.03f, 0.026f, 0.04f, CWH), BX(T_SKIN, -0.02f, 0.05f, -0.014f, 0.024f, 0.022f, 0.032f, CWH));
            else ADRAW("viewmodels/thumb_grip2_L", H, skinC, BX(T_SKIN, 0.012f, 0.05f, 0.012f, 0.03f, 0.026f, 0.04f, CWH), BX(T_SKIN, 0.02f, 0.05f, -0.014f, 0.024f, 0.022f, 0.032f, CWH));
        }
    } else if (grip == 1) {   // open hand, palm down, fingers forward
        if (right) ADRAW("viewmodels/hand_open2_R", H, skinC, BX(T_SKIN, 0, 0, 0, 0.085f, 0.026f, 0.09f, CWH),
                         BX(T_SKIN, -0.03f, 0, -0.072f, 0.017f, 0.018f, 0.058f, CWH), BX(T_SKIN, -0.01f, 0, -0.078f, 0.017f, 0.018f, 0.068f, CWH),
                         BX(T_SKIN, 0.01f, 0, -0.076f, 0.017f, 0.018f, 0.064f, CWH), BX(T_SKIN, 0.03f, 0, -0.068f, 0.016f, 0.017f, 0.05f, CWH),
                         BX(T_SKIN, -0.038f, -0.004f, 0.012f, 0.03f, 0.024f, 0.05f, CWH), BX(T_SKIN, -0.054f, -0.006f, -0.022f, 0.022f, 0.02f, 0.05f, CWH));
        else ADRAW("viewmodels/hand_open2_L", H, skinC, BX(T_SKIN, 0, 0, 0, 0.085f, 0.026f, 0.09f, CWH),
                   BX(T_SKIN, 0.03f, 0, -0.072f, 0.017f, 0.018f, 0.058f, CWH), BX(T_SKIN, 0.01f, 0, -0.078f, 0.017f, 0.018f, 0.068f, CWH),
                   BX(T_SKIN, -0.01f, 0, -0.076f, 0.017f, 0.018f, 0.064f, CWH), BX(T_SKIN, -0.03f, 0, -0.068f, 0.016f, 0.017f, 0.05f, CWH),
                   BX(T_SKIN, 0.038f, -0.004f, 0.012f, 0.03f, 0.024f, 0.05f, CWH), BX(T_SKIN, 0.054f, -0.006f, -0.022f, 0.022f, 0.02f, 0.05f, CWH));
    } else {   // fist, knuckles forward, the thumb folded across the front of the fingers
        if (right) ADRAW("viewmodels/fist2_R", H, skinC, BX(T_SKIN, 0, 0, 0, 0.088f, 0.08f, 0.09f, CWH), BX(T_SKIN, 0, 0.012f, -0.05f, 0.09f, 0.05f, 0.022f, ((Color){226, 190, 168, 255})),
                         BX(T_SKIN, -0.04f, -0.018f, -0.012f, 0.024f, 0.04f, 0.06f, CWH), BX(T_SKIN, -0.02f, -0.03f, -0.05f, 0.04f, 0.02f, 0.022f, CWH));
        else ADRAW("viewmodels/fist2_L", H, skinC, BX(T_SKIN, 0, 0, 0, 0.088f, 0.08f, 0.09f, CWH), BX(T_SKIN, 0, 0.012f, -0.05f, 0.09f, 0.05f, 0.022f, ((Color){226, 190, 168, 255})),
                   BX(T_SKIN, 0.04f, -0.018f, -0.012f, 0.024f, 0.04f, 0.06f, CWH), BX(T_SKIN, 0.02f, -0.03f, -0.05f, 0.04f, 0.02f, 0.022f, CWH));
    }
}
// a box stretched from a to b (both in M's space), w thick: thumb bones
static void drawSeg(Matrix M, Vector3 a, Vector3 b, float w) {
    Vector3 d = Vector3Subtract(b, a); float L = Vector3Length(d); if (L < 0.001f) return;
    Vector3 z = Vector3Scale(d, 1 / L), x = Vector3CrossProduct(V3(0, 1, 0), z); if (Vector3Length(x) < 0.01f) x = V3(1, 0, 0); x = Vector3Normalize(x); Vector3 y = Vector3CrossProduct(z, x);
    Matrix S = {x.x * w, y.x * w, z.x * L, a.x, x.y * w, y.y * w, z.y * L, a.y, x.z * w, y.z * w, z.z * L, a.z, 0, 0, 0, 1};
    ADRAW("viewmodels/seg_skin", MatrixMultiply(S, M), skinC, BX(T_SKIN, 0, 0, 0.5f, 1.0f, 1.0f, 1.25f, CWH));
}
// a sleeve from the wrist back to a shoulder anchor; both points in camera-local space
static void drawArm(Matrix B, Vector3 wrist, Vector3 shoulder) {
    Vector3 d = Vector3Subtract(shoulder, wrist); float L = Vector3Length(d); if (L < 0.01f) return;
    Vector3 z = Vector3Scale(d, 1 / L), x = Vector3Normalize(Vector3CrossProduct(V3(0, 1, 0), z)), y = Vector3CrossProduct(z, x);
    Matrix A = {x.x * 0.1f, y.x * 0.1f, z.x * L, wrist.x, x.y * 0.1f, y.y * 0.1f, z.y * L, wrist.y, x.z * 0.1f, y.z * 0.1f, z.z * L, wrist.z, 0, 0, 0, 1};
    Matrix M = MatrixMultiply(A, B);
    ADRAW("viewmodels/sleeve2", M, WHITE, BX(T_CLOTH, 0, 0, 0.55f, 1.0f, 1.0f, 0.9f, ((Color){124, 130, 146, 255})), BX(T_CLOTH, 0, 0, 0.06f, 1.12f, 1.12f, 0.12f, ((Color){92, 97, 110, 255})));
}
static Matrix handM(Matrix B, HandP h) { return frameM(B, h.p, h.r.x, h.r.y, h.r.z); }
static void drawHand(Matrix B, HandP h, bool right) {
    if (!h.vis) return;
    Matrix H = handM(B, h);
    Vector3 wrist = Vector3Transform(V3(right ? 0.04f : -0.04f, -0.06f, 0.06f), MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixRotateZ(h.r.z), MatrixRotateX(h.r.x)), MatrixRotateY(h.r.y)), MatrixTranslate(h.p.x, h.p.y, h.p.z)));
    drawArm(B, wrist, V3(right ? 0.36f : -0.34f, -0.8f, 0.2f));
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
    float bx = sinf(pl.bob) * 0.012f * (1 + pl.sprintK) * gs.bob, by = fabsf(cosf(pl.bob)) * 0.012f * (1 + pl.sprintK * 1.5f) * gs.bob;
    float br = sinf(t * 1.6f) * 0.004f;   // breathing
    Vector3 sway = V3(pl.swayX * gs.sway, (pl.swayY - pl.landK * 0.09f) * gs.sway + br, 0);
    rlDrawRenderBatchActive();
    glClear(0x00000100);   // depth only: the hands never sink into walls, but sort correctly among themselves
    setEmis(0);
    int ht = pl.hands.t; bool melee = ht >= 0 && idef[ht].cat == CAT_MELEE, gun = ht == IT_REVOLVER;
    float u2 = animDur > 0 ? clampf(animT / animDur, 0, 1) : 1; int A = animId != ANIM_NONE && animT < animDur ? animId : ANIM_NONE;
    HandP R = {V3(0.22f, -0.25f, -0.44f), V3(-0.95f, 0.12f, -0.3f), 0, true}, L = {V3(-0.22f, -0.32f, -0.42f), V3(-0.3f, 0, 0.2f), 1, false};
    float spinX = 0, spinY = 0, itemUp = 0, spinC = 0;   // item spin inside the hand (inspect flips), and how far it is tossed up out of it
    float heavy = ht >= 0 ? clampf(idef[ht].weight / 5.0f, 0.1f, 1) : 0.05f;
    // ---- revolver mechanics: hammer, cylinder index, crane, ejector, speedloader, all driven from the game state
    static int cyCur = 0, cySpent = -1, cyLastMag = -1, cyFallMask = 0; static bool cyEjected = false, cyFallPending = false, ldDropped = false;
    static float cyFallT = 9, ldFallT = 9; static Vector3 cyFallP[6], cyFallV[6], ldFallV, ldHand; static Matrix ldFallM;
    float vdt = fminf(GetFrameTime(), 0.05f); revShotT += vdt; cyFallT += vdt; ldFallT += vdt;
    float hk = 1, thumbK = 0, dec = 0, crane = 0, ej = 0, cyExtra = 0, twirl = 0, ldA = 0, ldIn = 0, ldTw = 0, ldOut = 0; int liveAdd = 0, ldN = 0;
    if (gun) {
        if (cySpent < 0) cySpent = 6 - pl.mag;
        if (cyLastMag >= 0 && pl.mag < cyLastMag) { int d = cyLastMag - pl.mag; cyCur = (cyCur + d) % 6; cySpent = cySpent + d; }
        if (cySpent > 6 - pl.mag) cySpent = 6 - pl.mag;
        cyLastMag = pl.mag;
        if (A == ANIM_EQUIP) { hk = ph(u2, 0.5f, 0.68f); thumbK = ph(u2, 0.38f, 0.5f) * (1 - ph(u2, 0.72f, 0.86f)); }
        else if (revShotT < 0.5f) { hk = revShotT < 0.09f ? 0 : ph(revShotT, 0.11f, 0.24f); thumbK = ph(revShotT, 0.04f, 0.11f) * (1 - ph(revShotT, 0.27f, 0.4f)); }
        if (A == ANIM_RELOAD || A == ANIM_INSPECT) {   // decock before the crane opens, cock again at the end
            dec = ph(u2, 0.03f, 0.08f) * (1 - ph(u2, 0.9f, 0.95f));
            thumbK = fmaxf(bell(u2, 0.0f, 0.1f), bell(u2, 0.87f, 0.98f));
        }
        if (A == ANIM_RELOAD) {
            crane = ph(u2, 0.1f, 0.17f) * (1 - ezOut((u2 - 0.845f) / 0.035f));
            ej = bell(u2, 0.23f, 0.31f);
            if (u2 > 0.27f && !cyEjected) {   // the empties fly out
                cyEjected = true; cyFallMask = 0;
                for (int i = 1; i <= cySpent; i++) cyFallMask |= 1 << ((cyCur - i + 6) % 6);
                cySpent = 0; cyFallPending = cyFallMask != 0;
            }
            ldN = 6 - pl.mag; { int have = invCount(IT_AMMO); if (have < ldN) ldN = have; }
            ldA = ph(u2, 0.35f, 0.5f); ldIn = ph(u2, 0.5f, 0.58f); ldTw = ph(u2, 0.58f, 0.64f); ldOut = ph(u2, 0.64f, 0.68f);
            if (u2 > 0.58f) liveAdd = ldN;
            if (u2 < 0.6f) ldDropped = false;
            cyExtra = ezOut((u2 - 0.72f) / 0.13f) * 2 * PI;   // flicked spin before closing
        } else cyEjected = false;
        if (A == ANIM_INSPECT) {
            crane = ph(u2, 0.15f, 0.22f) * (1 - ezOut((u2 - 0.6f) / 0.035f));
            cyExtra = ezOut((u2 - 0.25f) / 0.32f) * 4 * PI;
        }
    }
    // ---- idle poses per held thing
    if (pl.build) { R = (HandP){V3(0.22f, -0.25f, -0.4f), V3(-0.3f, 0, 0), 0, true}; }
    else if (gun) { R = (HandP){V3(0.15f, -0.215f, -0.36f), V3(0.04f, 0.06f, 0), 0, true}; }   // one-handed: the left hand is free for the flashlight
    else if (ht >= 0 && isConsumable(ht)) { R = (HandP){V3(0.17f, -0.23f, -0.42f), V3(-0.15f, 0.3f, -0.15f), 0, true}; L.vis = false; }   // food, a bottle or a bandage held low in the right hand
    else if (!melee) { R = (HandP){V3(0.16f, -0.175f, -0.38f), V3(-0.12f, 0.22f, -0.6f), 2, true}; L = (HandP){V3(-0.16f, -0.185f, -0.4f), V3(-0.12f, -0.22f, 0.6f), 2, true}; }   // a loose guard: knuckles forward, wrists turned in so the thumbs ride on top
    else if (heavy > 0.45f) L.vis = true;   // two-handed: the left hand grips below the right
    // two hand sets: the left holds the flashlight while it is on, the right holds fists or the weapon
    // the phone comes up from the pocket and flips open when the light goes on; folds shut and drops away when it goes off
    static float torchK = 0; bool torchWant = pl.flash && !pl.build && !(melee && heavy > 0.45f);
    torchK = clampf(torchK + (torchWant ? 1.0f : -1.0f) * vdt * 3.4f, 0, 1);
    bool torch = torchK > 0.001f; float phoneOpen = ph(torchK, 0.5f, 0.95f);
    if (torch) { float e = ezOut(torchK);   // the phone, upright in the palm, screen toward you, light on the back
        L = (HandP){Vector3Lerp(V3(-0.24f, -0.62f, -0.3f), V3(-0.13f, -0.165f, -0.47f), e), Vector3Lerp(V3(1.2f, 0.8f, 0.5f), V3(0.3f, 0.32f, 0.05f), e), 0, true};
        L.r.z += bell(torchK, 0.45f, 0.95f) * 0.25f; }   // a flick of the wrist to snap it open
    // ---- melee swing (wind-up, strike, recovery) and the fist jab
    float dur = pl.swingDur > 0 ? pl.swingDur : 0.6f, s = pl.swingT > 0 ? 1 - pl.swingT / dur : 0;
    float wu = ez(s / 0.3f), st = ezOut((s - 0.3f) / 0.15f), rc = ez((s - 0.45f) / 0.55f);
    float shakeHit = pl.hitStopT > 0 ? fr(-0.03f, 0.03f) : 0;
    // weapon styles: 0 blade (alternating diagonal slashes), 1 knife (stab), 2 blunt (horizontal sweep), 3 heavy (two-handed overhead chop)
    int style = ht >= 0 && idef[ht].style > 0 ? idef[ht].style - 1 : ht == IT_KATANA ? 0 : ht == IT_KNIFE ? 1 : (ht == IT_AXE || ht == IT_SLEDGE) ? 3 : 2;
    {   // sounds keyed to points in the animations: each fires once as the animation passes it
        static int sA = -1, sHt = -2; static float sU = 0, sShot = 9, sFall = 9, sLd = 9;
        if (A != sA || ht != sHt) sU = -1;
        #define AT(x) (sU < (x) && u2 >= (x))
        if (A == ANIM_EQUIP && gun) { if (AT(0.5f)) play(sCock, 0.55f); }
        if (A == ANIM_RELOAD && gun) {
            if (AT(0.04f)) play(sCock, 0.25f);   // hammer let down
            if (AT(0.11f)) play(sCylOpen, 0.6f);
            if (AT(0.25f)) play(sEject, 0.65f);
            if (AT(0.33f)) play(sCloth, 0.35f);   // the loader from the belt
            if (AT(0.53f)) play(sLoader, 0.6f);
            if (AT(0.6f)) play(sClick, 0.45f);    // knob twisted, rounds released
            if (AT(0.73f)) play(sSpin, 0.5f);
            if (AT(0.85f)) play(sCylClose, 0.75f);
            if (AT(0.91f)) play(sCock, 0.5f);
        }
        if (A == ANIM_INSPECT) {
            if (gun) { if (AT(0.05f)) play(sCock, 0.25f); if (AT(0.16f)) play(sCylOpen, 0.55f); if (AT(0.25f)) play(sSpin, 0.55f); if (AT(0.61f)) play(sCylClose, 0.7f); if (AT(0.91f)) play(sCock, 0.45f); }
            else if (melee && style == 0) { if (AT(0.05f)) play(sCloth, 0.3f); if (AT(0.2f)) play(sBlade, 0.35f); if (AT(0.48f)) play(sBlade, 0.25f); if (AT(0.64f)) play(sBlade, 0.2f); }
            else if (melee && style == 1) { if (AT(0.18f)) play(sSwing, 0.25f); if (AT(0.3f)) play(sSwing, 0.2f); if (AT(0.42f)) play(sSwing, 0.2f); if (AT(0.52f)) play(sClick, 0.4f); if (AT(0.6f)) play(sSwing, 0.25f); }
            else if (melee && style == 2) { if (AT(0.12f)) play(sCloth, 0.4f); if (AT(0.28f)) play(sStep, 0.3f); if (AT(0.4f)) play(sStep, 0.3f); if (AT(0.6f)) play(sSwing, 0.35f); if (AT(0.79f)) play(sWood, 0.3f); }
            else if (melee) { if (AT(0.08f)) play(sCloth, 0.4f); if (AT(0.56f)) play(sBlade, 0.15f); }
            else { if (AT(0.3f)) play(sBone, 0.18f); if (AT(0.42f)) play(sBone, 0.14f); if (AT(0.6f)) play(sCloth, 0.3f); }   // knuckles cracking, hands shaken out
        }
        if (A == ANIM_EQUIP && melee && style == 1 && AT(0.3f)) play(sSwing, 0.2f);
        if (A == ANIM_TURNGUN) { if (AT(0.01f)) play(sHeart, 1.0f); if (AT(0.2f)) play(sBreath, 0.7f); if (AT(0.45f)) play(sHeart, 1.0f); if (AT(0.62f)) play(sCloth, 0.4f); if (AT(0.8f)) play(sCock, 0.8f); if (AT(0.85f)) play(sHeart, 1.0f); }
        #undef AT
        if (gun && sShot < 0.12f && revShotT >= 0.12f && revShotT < 0.5f && A == ANIM_NONE) play(sCock, 0.45f);   // the thumb rolls the hammer back after a shot
        if (sFall < 0.42f && cyFallT >= 0.42f && cyFallMask) play(sShell, 0.5f);   // the empties land
        if (sLd < 0.5f && ldFallT >= 0.5f) play(sClang, 0.2f);   // the empty speedloader lands
        sA = A; sHt = ht; sU = u2; sShot = revShotT; sFall = cyFallT; sLd = ldFallT;
    }
    static float prevSw = 0; static int swingN = 0, lastKills = -1;
    if (pl.swingT > prevSw + 0.02f) swingN++;   // a new swing started: combos alternate direction
    prevSw = pl.swingT;
    if (lastKills >= 0 && pl.kills > lastKills && ht == IT_KATANA && A == ANIM_NONE) animPlay(ANIM_FLOURISH);   // chiburi: flick the blood off after a kill
    lastKills = pl.kills;
    float sd = (swingN & 1) ? -1.0f : 1.0f;
    if (melee && !pl.build && pl.swingT > 0) {
        float post = st >= 1 ? 1 : 0, sw = sinf(s * PI);
        if (style == 0) {   // katana: wind up high on one side, cut down across to the other, settle
            R.p.x = 0.22f + sd * (wu * 0.06f - st * 0.36f + rc * 0.3f * post) + shakeHit; R.p.y = -0.25f + wu * 0.16f - st * 0.14f + rc * 0.1f * post;
            R.p.z = -0.44f - st * 0.08f + rc * 0.08f * post;
            R.r.x = -0.95f + wu * 1.5f - st * 2.3f + rc * 1.7f * post; R.r.z = -0.3f + sd * (-wu * 0.9f + st * 1.9f - rc * 1.0f * post); R.r.y = sd * (wu * 0.3f - st * 0.5f + rc * 0.2f * post);
        } else if (style == 1) {   // knife: draw back, stab, twist out
            float stab = bell(s, 0.25f, 0.6f); R.p = Vector3Add(R.p, V3(-stab * 0.07f + shakeHit, stab * 0.03f - wu * 0.03f, -stab * 0.3f + wu * 0.05f));
            R.r.x += -0.6f * stab; R.r.z += bell(s, 0.45f, 0.8f) * 0.6f;
        } else if (style == 2) {   // bat, pipe, crowbar: cock behind the right shoulder, sweep flat across
            R.p.x = 0.24f + wu * 0.12f - st * 0.5f + rc * 0.38f * post + shakeHit; R.p.y = -0.24f + wu * 0.1f - st * 0.04f; R.p.z = -0.42f + wu * 0.06f - st * 0.1f + rc * 0.1f * post;
            R.r.x = -0.95f + wu * 0.4f + st * 0.25f; R.r.y = wu * 0.9f - st * 2.0f + rc * 1.1f * post; R.r.z = -0.3f - wu * 0.4f + st * 1.3f - rc * 0.9f * post;
        } else {   // axe, sledge: both hands, lifted over the head, brought straight down
            L.vis = true;
            R.p.x = 0.14f - st * 0.06f + rc * 0.06f * post + shakeHit; R.p.y = -0.25f + wu * 0.3f - st * 0.36f + rc * 0.06f * post; R.p.z = -0.4f + wu * 0.1f - st * 0.12f + rc * 0.02f * post;
            R.r.x = -0.95f + wu * 1.9f - st * 2.6f + rc * 0.7f * post; R.r.z = -0.15f + 0.1f * sw;
        }
        (void)sw;
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
    case ANIM_EQUIP:
        if (ht == IT_KATANA) {   // iaido: the hand starts at the left hip and draws up and out in one fast arc
            float d = ezOut((u2 - 0.15f) / 0.45f), hold = ph(u2, 0, 0.15f), set = ph(u2, 0.65f, 1);
            Vector3 hip = V3(-0.12f, -0.42f, -0.3f), out = V3(0.3f, -0.12f, -0.52f);
            Vector3 p = Vector3Lerp(hip, out, d); R.p = Vector3Lerp(Vector3Lerp(V3(-0.1f, -0.55f, -0.28f), hip, hold), R.p, set); if (u2 > 0.15f && u2 < 0.65f) R.p = p; else if (u2 >= 0.65f) R.p = Vector3Lerp(out, R.p, set);
            float rx = Lerp(0.9f, -1.3f, d), rz = Lerp(1.4f, -0.6f, d); R.r.x = u2 < 0.65f ? rx : Lerp(-1.3f, R.r.x, set); R.r.z = u2 < 0.65f ? rz : Lerp(-0.6f, R.r.z, set);
        } else if (gun) {   // up out of the holster at the right hip, punched forward to aim, then the thumb rolls the hammer back
            float d = ezBack(clampf(u2 / 0.45f, 0, 1)), cock = bell(u2, 0.45f, 0.8f);
            R.p = Vector3Lerp(V3(0.26f, -0.56f, -0.1f), R.p, d); R.r = Vector3Lerp(V3(-1.5f, 0.35f, 0.5f), R.r, d);
            R.r.z += cock * 0.35f; R.r.x -= cock * 0.12f; R.p.x -= cock * 0.02f;   // tilt in to the thumb
            R.p.y -= bell(u2, 0.62f, 0.76f) * 0.006f;
        } else if (melee && style == 2) {   // light weapons: tossed up and caught
            float e = ezBack(u2); R.p.y -= (1 - e) * 0.32f; spinX = -(1 - ez(u2 * 1.2f)) * 2 * PI; R.p.y += bell(u2, 0.2f, 0.7f) * 0.05f;
        } else { float e = ezBack(u2); R.p.y -= (1 - e) * 0.38f; R.r.x += (1 - e) * 1.1f; R.r.z -= (1 - e) * 0.5f; L.p.y -= (1 - e) * 0.38f; }
        break;
    case ANIM_FLOURISH: if (ht == IT_KATANA) {   // chiburi: a sharp downward flick to the side, then a slow return
        float f1 = ezOut(u2 / 0.25f), back = ph(u2, 0.45f, 1);
        R.p = Vector3Add(R.p, Vector3Scale(V3(0.08f, -0.08f, 0.03f), f1 * (1 - back))); R.r.z += (1.2f * f1) * (1 - back); R.r.x += (-0.6f * f1) * (1 - back);
        R.p.x += bell(u2, 0.2f, 0.3f) * 0.015f;
    } break;
    case ANIM_TURNGUN: if (gun) {   // insanity: the arm brings the revolver up, the wrist turns it round, the muzzle comes to the face. It shakes the whole way.
        float a = ph(u2, 0, 0.3f), b = ph(u2, 0.3f, 0.75f), c = ph(u2, 0.75f, 0.97f), tr = 0.003f + 0.006f * u2;
        R.p = Vector3Lerp(R.p, V3(0.05f, -0.17f, -0.36f), ez(a));
        R.p = Vector3Lerp(R.p, V3(0.02f, -0.26f, -0.46f), ez(b)); R.r.y += PI * ez(b); R.r.x = Lerp(R.r.x, 0.4f, b); R.r.z += 0.25f * bell(u2, 0.3f, 0.75f);
        R.p = Vector3Lerp(R.p, V3(0.0f, -0.26f, -0.36f), ez(c)); R.r.x += 0.13f * c;   // arm out, muzzle angled up to the mouth just below the eyes
        R.p.x += sinf(t * 37) * tr + sinf(t * 23) * tr * 0.6f; R.p.y += cosf(t * 41) * tr; R.r.z += sinf(t * 29) * 0.03f;
        R.p.x -= bell(u2, 0.12f, 0.24f) * 0.03f + bell(u2, 0.5f, 0.62f) * 0.02f;   // it fights you, twice
        L.vis = false;
    } break;
    case ANIM_HOLSTER: { float e = ez(u2); R.p.y -= e * 0.4f; R.r.x += e * 0.9f; L.p.y -= e * 0.4f; break; }
    case ANIM_INSPECT:
        if (melee && style == 0) {   // katana: raised flat across the view, turned to show both faces, rolled to sight down the edge, two fingers along the flat
            float a = ph(u2, 0, 0.16f), b = ph(u2, 0.16f, 0.42f), c = bell(u2, 0.44f, 0.62f), d = ph(u2, 0.62f, 0.8f), e = ph(u2, 0.82f, 1);
            Vector3 up = V3(0.08f, -0.17f, -0.42f);
            R.p = Vector3Lerp(R.p, up, a * (1 - e)); R.r.z = Lerp(R.r.z, -1.45f, a * (1 - e)); R.r.y += (0.7f * a - 1.4f * b) * (1 - e); R.r.x += 0.25f * a * (1 - e);
            R.r.z += c * 0.9f; R.r.x += c * 0.35f; R.p.y += c * 0.02f;   // edge toward the eye
            if (u2 > 0.6f && u2 < 0.82f) { L.vis = true; L.grip = 1; float sl = d; L.p = Vector3Add(R.p, V3(-0.12f + sl * 0.18f, 0.03f, -0.03f)); L.r = V3(0.1f, 0, -0.3f); }
        } else if (melee && style == 1) {   // knife: raised, the wrist turns it to show both faces of the blade, then one short flip about its middle and a catch
            float a = ph(u2, 0, 0.14f), e = ph(u2, 0.86f, 1), fl = ph(u2, 0.66f, 0.82f);
            R.p = Vector3Lerp(R.p, V3(0.09f, -0.19f, -0.4f), a * (1 - e)); R.r.x += 0.15f * a * (1 - e); R.r.z += -0.35f * a * (1 - e);
            R.r.y += (ez(ph(u2, 0.14f, 0.32f)) * 1.1f - ez(ph(u2, 0.36f, 0.56f)) * 2.0f + ez(ph(u2, 0.58f, 0.66f)) * 0.9f) * (1 - e);   // one face, then the other
            spinX = -ez(fl) * 2 * PI; spinC = 0.07f; itemUp = bell(u2, 0.66f, 0.82f) * 0.09f; R.grip = (u2 > 0.665f && u2 < 0.815f) ? 1 : 0;
            R.p.y += bell(u2, 0.64f, 0.7f) * 0.02f - bell(u2, 0.81f, 0.87f) * 0.018f;   // the little lift that throws it, the dip of the catch
        } else if (melee && style == 2) {   // bat / pipe / crowbar: rested on the shoulder, two taps, then a toss-spin and catch
            float a = ph(u2, 0, 0.2f), off = ph(u2, 0.5f, 0.6f), e = ph(u2, 0.84f, 1);
            float sh = a * (1 - off);
            R.p = Vector3Lerp(R.p, V3(0.2f, -0.12f, -0.24f), sh); R.r = Vector3Lerp(R.r, V3(0.75f, 0.3f, -0.35f), sh);
            R.p.y -= (bell(u2, 0.24f, 0.33f) + bell(u2, 0.36f, 0.45f)) * 0.025f * sh;
            R.p = Vector3Lerp(R.p, V3(0.08f, -0.17f, -0.42f), off * (1 - e));
            spinX = ph(u2, 0.6f, 0.8f) * 2 * PI; itemUp = bell(u2, 0.6f, 0.8f) * 0.14f; R.grip = (u2 > 0.62f && u2 < 0.78f) ? 1 : 0;
        } else if (melee) {   // axe / sledge: both hands lift the head up to look at it, turn it, thumb along the edge
            float a = ph(u2, 0, 0.18f), turn = bell(u2, 0.2f, 0.5f), th = ph(u2, 0.52f, 0.6f) * (1 - ph(u2, 0.76f, 0.82f)), e = ph(u2, 0.84f, 1);
            R.p = Vector3Lerp(R.p, V3(0.08f, -0.24f, -0.46f), a * (1 - e)); R.r.x += 0.3f * a * (1 - e); R.r.z += -0.5f * a * (1 - e);
            R.r.y += turn * 0.9f - bell(u2, 0.5f, 0.8f) * 0.5f;
            if (th > 0) { L.vis = true; L.grip = 1; Matrix Hh = frameM(MatrixIdentity(), R.p, R.r.x, R.r.y, R.r.z); L.p = Vector3Add(Vector3Transform(V3(0, 0.42f, -0.02f), Hh), V3(-0.06f, -0.02f, 0.05f)); L.r = V3(0.2f, -0.3f, 1.2f);
                L.p.y += sinf(u2 * 40) * 0.004f * th; }
        } else if (gun) {   // turn it to look down the side, open the crane, spin the cylinder, look at the rims, flick it shut, twirl
            float a = ph(u2, 0, 0.12f), look = ph(u2, 0.38f, 0.48f) * (1 - ph(u2, 0.56f, 0.62f)), snap = bell(u2, 0.6f, 0.68f), e = ph(u2, 0.88f, 1);
            Vector3 p0 = R.p, r0 = R.r;
            R.p = Vector3Lerp(p0, V3(0.06f, -0.17f, -0.34f), a); R.r = Vector3Lerp(r0, V3(0.12f, -0.85f, 0.25f), a);
            R.r = Vector3Lerp(R.r, V3(0.55f, 0.45f, 0.35f), look); R.p.y += look * 0.02f;
            R.r.z -= snap * 0.45f; R.p.x += snap * 0.015f;
            float rs = ph(u2, 0.68f, 0.76f) * (1 - ph(u2, 0.84f, 0.9f));   // then turned over to the right side
            R.r = Vector3Lerp(R.r, V3(0.1f, 0.75f, -0.5f), rs); R.p = Vector3Lerp(R.p, V3(0.1f, -0.17f, -0.36f), rs);
            R.p = Vector3Lerp(R.p, p0, e); R.r = Vector3Lerp(R.r, r0, e);
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
    case ANIM_RELOAD: if (gun) {   // in to the chest canted left; muzzle up to punch the ejector; muzzle down for the speedloader; spin; flick shut
        float a = ph(u2, 0, 0.1f), up = ph(u2, 0.17f, 0.23f) * (1 - ph(u2, 0.31f, 0.37f)), dn = ph(u2, 0.33f, 0.4f) * (1 - ph(u2, 0.7f, 0.78f)), e = ph(u2, 0.88f, 1);
        Vector3 p0 = R.p, r0 = R.r;
        R.p = Vector3Lerp(p0, V3(0.06f, -0.215f, -0.44f), a); R.r = Vector3Lerp(r0, V3(0.15f, -0.55f, 0.3f), a);
        R.r.x += up * 0.6f - dn * 0.8f; R.p.y -= up * 0.01f + dn * 0.01f; R.p.z -= up * 0.03f;
        R.p.y -= ej * 0.012f;   // the palm strike jolts it
        R.r.z -= bell(u2, 0.84f, 0.9f) * 0.55f; R.p.x += bell(u2, 0.84f, 0.9f) * 0.02f;   // wrist flick closes the crane
        R.p = Vector3Lerp(R.p, p0, e); R.r = Vector3Lerp(R.r, r0, e);
        L.vis = true; L.grip = 1;
    } break;
    case ANIM_BUILD: { float k = bell(u2, 0, 1); R.p.y -= k * 0.06f; R.r.x -= k * 0.5f; break; }
    }
    if (gun) { float kk = pl.kick * pl.kick; R.p.z += pl.kick * 0.07f; R.p.y += kk * 0.03f; R.r.x += pl.kick * 0.5f; R.r.z += kk * 0.08f; }
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
    if (gs.centered && (gun || melee) && A != ANIM_TURNGUN) { R.p.x -= 0.1f; R.p.y += 0.02f; }   // weapon in the middle of the view
    R.r.z += pl.swayX * 1.5f; L.r.z += pl.swayX * 1.5f;
    R.r.z -= vmRoll * 0.09f; L.r.z -= vmRoll * 0.09f; R.p.x -= vmRoll * 0.008f; L.p.x -= vmRoll * 0.008f;   // lean into a strafe
    R.p.z += vmPushZ * 0.05f; L.p.z += vmPushZ * 0.05f;   // inertia when you start or stop
    R.r.y += pl.swayX * 0.6f; R.r.x -= pl.swayY * 0.8f;   // and the weapon twists a little as it lags
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
            // a .38 revolver: wooden grip, frame with a window for the cylinder, top strap, barrel with a vent rib and a full
            // underlug, front sight; a six-chamber fluted cylinder on a crane that swings out to the left; ejector rod and star;
            // hammer that the thumb rolls back; trigger
            Matrix P = MatrixMultiply(twirl != 0 ? MatrixMultiply(MatrixMultiply(MatrixTranslate(0, -0.03f, 0.04f), MatrixRotateX(-twirl)), MatrixTranslate(0, 0.03f, -0.04f)) : MatrixIdentity(),
                                      MatrixScale(1.35f, 1.35f, 1.35f));   // gun space -> hand space (first-person guns read better a little oversized)
            Matrix G = MatrixMultiply(P, H);
            Color st = {120, 126, 140, 255}, st2 = {92, 97, 110, 255}, dk = {16, 16, 18, 255}, wd = {138, 84, 48, 255}, wd2 = {108, 64, 36, 255};
            ADRAW("viewmodels/revolver4", G, WHITE,
                  BX(T_WOOD, 0, -0.005f, 0.012f, 0.03f, 0.11f, 0.045f, wd), BX(T_WOOD, 0.0155f, -0.004f, 0.01f, 0.002f, 0.07f, 0.03f, wd2), BX(T_WOOD, -0.0155f, -0.004f, 0.01f, 0.002f, 0.07f, 0.03f, wd2),
                  BX(T_METAL, 0, 0.062f, 0.012f, 0.022f, 0.036f, 0.034f, st),                // frame behind the trigger, down into the grip (the web of the hand sits under it)
                  BX(T_METAL, 0, 0.046f, -0.04f, 0.024f, 0.02f, 0.08f, st),                  // frame under the cylinder
                  BX(T_METAL, 0, 0.075f, -0.015f, 0.026f, 0.06f, 0.018f, st),                // recoil shield
                  BX(T_METAL, 0, 0.088f, 0.004f, 0.02f, 0.03f, 0.022f, st),                  // frame hump behind it
                  BX(T_METAL, 0, 0.104f, -0.05f, 0.018f, 0.008f, 0.06f, st),                 // top strap
                  BX(T_PLAIN, 0, 0.1085f, -0.03f, 0.004f, 0.0015f, 0.04f, dk),               // rear sight groove
                  BX(T_METAL, 0, 0.088f, -0.083f, 0.026f, 0.04f, 0.012f, st),                // front of the window
                  BX(T_METAL, 0, 0.092f, -0.165f, 0.017f, 0.017f, 0.16f, st2),               // barrel
                  BX(T_METAL, 0, 0.1025f, -0.165f, 0.008f, 0.005f, 0.16f, st),               // vent rib
                  BX(T_METAL, 0, 0.075f, -0.15f, 0.015f, 0.016f, 0.13f, st2),                // underlug / ejector shroud
                  BX(T_METAL, 0, 0.108f, -0.236f, 0.004f, 0.01f, 0.014f, st2), BX(T_PLAIN, 0, 0.1115f, -0.236f, 0.0045f, 0.004f, 0.006f, ((Color){230, 90, 40, 255})),   // front sight, orange insert
                  BX(T_PLAIN, 0, 0.092f, -0.2455f, 0.009f, 0.009f, 0.002f, dk),              // bore
                  BX(T_METAL, 0, 0.022f, -0.04f, 0.008f, 0.005f, 0.05f, st), BX(T_METAL, 0, 0.032f, -0.064f, 0.008f, 0.024f, 0.005f, st),   // trigger guard
                  BX(T_METAL, -0.0145f, 0.077f, -0.012f, 0.004f, 0.008f, 0.012f, st2));      // cylinder latch
            float trig = revShotT < 0.14f ? 1 - ph(revShotT, 0.07f, 0.14f) : 0;
            ADRAW("viewmodels/revolver4_trigger", MatrixMultiply(MatrixTranslate(0, 0, trig * 0.007f), G), WHITE, BX(T_METAL, 0, 0.034f, -0.038f, 0.004f, 0.022f, 0.006f, dk));
            float ha = 0.62f * hk * (1 - dec);   // hammer: 0 down on the frame, 0.62 rad cocked back
            Matrix Hm = MatrixMultiply(MatrixMultiply(MatrixTranslate(0, -0.088f, -0.012f), MatrixRotateX(ha)), MatrixMultiply(MatrixTranslate(0, 0.088f, 0.012f), G));
            ADRAW("viewmodels/revolver4_hammer", Hm, WHITE, BX(T_METAL, 0, 0.098f, 0.016f, 0.008f, 0.024f, 0.014f, st2), BX(T_METAL, 0, 0.111f, 0.027f, 0.01f, 0.006f, 0.018f, st));
            // crane, ejector rod, cylinder
            Matrix S = MatrixMultiply(MatrixRotateZ(crane * 1.5f), MatrixMultiply(MatrixTranslate(-0.008f, 0.052f, -0.05f), G));
            float ejz = ej * 0.02f;
            ADRAW("viewmodels/revolver4_crane", S, WHITE, BX(T_METAL, 0.004f, 0.013f, -0.028f, 0.01f, 0.03f, 0.006f, st));
            ADRAW("viewmodels/revolver4_rod", MatrixMultiply(MatrixTranslate(0, 0, ejz), S), WHITE, BX(T_METAL, 0.008f, 0.026f, -0.05f, 0.005f, 0.005f, 0.05f, st2), BX(T_METAL, 0.008f, 0.026f, -0.076f, 0.008f, 0.008f, 0.006f, st));
            float ang = -((float)cyCur - (1 - hk)) * PI / 3 + cyExtra;
            Matrix C = MatrixMultiply(MatrixRotateZ(ang), MatrixMultiply(MatrixTranslate(0.008f, 0.026f, 0), S));
            for (int k = 0; k < 3; k++) ADRAW("viewmodels/revolver4_cyl", MatrixMultiply(MatrixRotateZ(k * PI / 3), C), WHITE, BX(T_METAL, 0, 0, 0, 0.044f, 0.026f, 0.05f, ((Color){138, 144, 158, 255})));
            for (int k = 0; k < 6; k++) ADRAW("viewmodels/revolver4_flute", MatrixMultiply(MatrixRotateZ(PI / 6 + k * PI / 3), C), WHITE, BX(T_PLAIN, 0, 0.0205f, -0.004f, 0.0065f, 0.007f, 0.032f, ((Color){44, 46, 54, 255})));
            Matrix CE = MatrixMultiply(MatrixTranslate(0, 0, ejz), C);
            ADRAW("viewmodels/revolver4_star", CE, WHITE, BX(T_METAL, 0, 0, 0.0255f, 0.013f, 0.013f, 0.0025f, ((Color){96, 98, 104, 255})));
            int live = pl.mag + liveAdd; if (live > 6) live = 6;
            Matrix Chm[6];
            for (int i = 0; i < 6; i++) {
                float ai = i * PI / 3; Chm[i] = MatrixMultiply(MatrixTranslate(-sinf(ai) * 0.0135f, cosf(ai) * 0.0135f, 0), C);
                int rel = (i - cyCur + 6) % 6, stt = rel < live ? 1 : (6 - rel) <= cySpent ? 2 : 0;
                ADRAW("viewmodels/revolver4_hole", Chm[i], WHITE, BX(T_PLAIN, 0, 0, 0.0252f, 0.0092f, 0.0092f, 0.0012f, dk), BX(T_PLAIN, 0, 0, -0.0252f, 0.0092f, 0.0092f, 0.0012f, dk));
                Matrix Rm = MatrixMultiply(MatrixTranslate(0, 0, ejz), Chm[i]);
                if (stt == 1) ADRAW("viewmodels/round_live", Rm, WHITE, BX(T_METAL, 0, 0, 0.0035f, 0.0088f, 0.0088f, 0.043f, ((Color){190, 150, 64, 255})), BX(T_METAL, 0, 0, 0.0262f, 0.011f, 0.011f, 0.0025f, ((Color){200, 160, 70, 255})),
                                     BX(T_METAL, 0, 0, 0.0278f, 0.0038f, 0.0038f, 0.001f, ((Color){205, 205, 195, 255})), BX(T_METAL, 0, 0, -0.021f, 0.0072f, 0.0072f, 0.007f, ((Color){150, 110, 80, 255})));
                else if (stt == 2) ADRAW("viewmodels/round_spent", Rm, WHITE, BX(T_METAL, 0, 0, 0.0035f, 0.0088f, 0.0088f, 0.043f, ((Color){150, 112, 52, 255})), BX(T_METAL, 0, 0, 0.0262f, 0.011f, 0.011f, 0.0025f, ((Color){160, 120, 56, 255})),
                                         BX(T_PLAIN, 0, 0, 0.0278f, 0.0038f, 0.0038f, 0.001f, ((Color){70, 60, 50, 255})));
            }
            if (cyFallPending) {   // the empties leave the chambers with a little kick and fall in the world from here
                cyFallPending = false; cyFallT = 0;
                for (int i = 0; i < 6; i++) if (cyFallMask >> i & 1) {
                    cyFallP[i] = Vector3Transform(V3(0, 0, 0.03f), MatrixMultiply(MatrixTranslate(0, 0, ejz), Chm[i]));
                    Vector3 out = Vector3Subtract(Vector3Transform(V3(0, 0, 0.06f), Chm[i]), Vector3Transform(V3(0, 0, 0), Chm[i]));
                    cyFallV[i] = Vector3Add(Vector3Scale(out, 14), V3(fr(-0.25f, 0.25f), fr(0.1f, 0.5f), fr(-0.25f, 0.25f)));
                }
            }
            // the speedloader: brought up from the belt, rounds lined up with the empty chambers, pushed in, knob twisted, pulled off and dropped
            if (A == ANIM_RELOAD && ldN > 0 && u2 > 0.33f && !ldDropped) {
                float gap = (1 - ldIn) * 0.045f + ldOut * 0.03f;
                Matrix Lf = MatrixMultiply(MatrixTranslate(0, 0, gap), C);
                Vector3 belt = Vector3Transform(V3(-0.24f, -0.6f, -0.26f), B), at = {Lf.m12, Lf.m13, Lf.m14};
                Vector3 pos = Vector3Lerp(belt, at, ezOut(ldA));
                Matrix Lw = Lf; Lw.m12 = pos.x; Lw.m13 = pos.y; Lw.m14 = pos.z;
                Matrix Body = MatrixMultiply(MatrixRotateZ(ldTw * 0.7f), Lw);
                ADRAW("viewmodels/speedloader", Body, WHITE, BX(T_METAL, 0, 0, 0.034f, 0.036f, 0.026f, 0.009f, ((Color){30, 30, 34, 255})), BX(T_METAL, 0, 0, 0.034f, 0.026f, 0.036f, 0.009f, ((Color){30, 30, 34, 255})),
                      BX(T_METAL, 0, 0, 0.046f, 0.012f, 0.012f, 0.016f, ((Color){50, 50, 56, 255})), BX(T_METAL, 0, 0, 0.055f, 0.016f, 0.016f, 0.004f, ((Color){60, 60, 66, 255})));
                if (u2 <= 0.58f) for (int i = 0; i < 6; i++) { int rel = (i - cyCur + 6) % 6; if (rel < pl.mag || rel >= pl.mag + ldN) continue;
                    Matrix Rm = MatrixMultiply(Chm[i], MatrixTranslate(pos.x - at.x, pos.y - at.y, pos.z - at.z)); Rm = MatrixMultiply(MatrixTranslate(0, 0, gap), Rm);
                    ADRAW("viewmodels/round_live", Rm, WHITE, BX(T_METAL, 0, 0, 0.0035f, 0.0088f, 0.0088f, 0.043f, ((Color){190, 150, 64, 255})), BX(T_METAL, 0, 0, 0.0262f, 0.011f, 0.011f, 0.0025f, ((Color){200, 160, 70, 255})),
                          BX(T_METAL, 0, 0, 0.0278f, 0.0038f, 0.0038f, 0.001f, ((Color){205, 205, 195, 255})), BX(T_METAL, 0, 0, -0.021f, 0.0072f, 0.0072f, 0.007f, ((Color){150, 110, 80, 255}))); }
                if (u2 >= 0.68f) { ldDropped = true; ldFallT = 0; ldFallM = Body; ldFallV = V3(fr(-0.3f, 0.3f), -0.4f, fr(-0.3f, 0.3f)); }
                Vector3 knob = Vector3Transform(V3(0, 0, 0.05f), Body);
                ldHand = Vector3Add(Vector3Transform(knob, MatrixInvert(B)), V3(-0.075f, -0.01f, 0.012f));
            }
            // the right thumb: its base sits in the web of the hand; it lies along the frame, or reaches up to roll the hammer back
            {   Matrix PtoH = P;   // G = P * H, so P takes gun space into hand space
                Vector3 spur = Vector3Transform(Vector3Transform(V3(0, 0.113f, 0.03f), MatrixMultiply(MatrixMultiply(MatrixTranslate(0, -0.088f, -0.012f), MatrixRotateX(ha)), MatrixTranslate(0, 0.088f, 0.012f))), PtoH);
                Vector3 base = V3(-0.016f, 0.058f, 0.02f);
                Vector3 kn = Vector3Lerp(V3(-0.03f, 0.07f, 0.002f), V3(-0.034f, 0.104f, 0.044f), thumbK);
                Vector3 tip = Vector3Lerp(V3(-0.031f, 0.076f, -0.026f), Vector3Add(spur, V3(-0.006f, 0.004f, 0.012f)), thumbK);
                drawSeg(H, base, kn, 0.022f); drawSeg(H, kn, tip, 0.018f);
            }
            if (pl.muzzle > 0) { setEmis(1); ADRAW("viewmodels/muzzle_flash4", G, WHITE, BX(T_PLAIN, 0, 0.092f, -0.275f, 0.1f, 0.03f, 0.06f, ((Color){255, 220, 120, 255})), BX(T_PLAIN, 0, 0.092f, -0.275f, 0.03f, 0.1f, 0.06f, ((Color){255, 220, 120, 255})),
                                       BX(T_PLAIN, 0, 0.092f, -0.29f, 0.05f, 0.05f, 0.09f, ((Color){255, 245, 200, 255}))); setEmis(0); }
            drawHandAsset(H, true, 3);
            // the left hand during reload and inspect: follows the parts it works on
            if (A == ANIM_RELOAD || A == ANIM_INSPECT) {
                Matrix Bi = MatrixInvert(B);
                Vector3 side = Vector3Transform(Vector3Transform(V3(-0.02f, 0.026f, 0), S), Bi);
                Vector3 rest = torch ? V3(-0.13f, -0.165f, -0.47f) : V3(-0.3f, -0.75f, -0.2f);
                Vector3 lp = rest, ldp = ldHand;
                L.grip = 1; L.r = V3(0.1f, -0.3f, 1.3f); L.vis = true;   // open, palm turned toward the gun
                if (A == ANIM_RELOAD) {
                    lp = Vector3Lerp(lp, Vector3Add(side, V3(-0.04f, -0.01f, 0.02f)), ph(u2, 0.04f, 0.1f));   // fingers push the cylinder out
                    lp = Vector3Lerp(lp, Vector3Add(side, V3(-0.045f, -0.005f, 0.01f)), ph(u2, 0.12f, 0.2f));   // holds the open cylinder; the thumb drives the ejector rod
                    lp = Vector3Add(lp, Vector3Scale(Vector3Subtract(Vector3Transform(Vector3Transform(V3(0, 0, 0.03f), S), Bi), Vector3Transform(Vector3Transform(V3(0, 0, 0), S), Bi)), ej * 0.6f));
                    lp = Vector3Lerp(lp, V3(-0.26f, -0.62f, -0.24f), ph(u2, 0.3f, 0.36f));   // down to the belt
                    if (ldN > 0) { float hold = ph(u2, 0.35f, 0.4f) * (1 - ph(u2, 0.66f, 0.7f)); lp = Vector3Lerp(lp, ldp, hold); if (hold > 0.3f) { L.grip = 2; L.r = V3(0.1f, -0.3f, 0.4f + ldTw * 0.7f); } }   // holding the loader
                    lp = Vector3Lerp(lp, Vector3Add(side, V3(-0.045f, 0.0f, 0.02f)), ph(u2, 0.68f, 0.73f));   // to the cylinder
                    lp.y += bell(u2, 0.72f, 0.78f) * 0.05f; lp.x -= bell(u2, 0.72f, 0.78f) * 0.02f;   // swipe: spin it
                    lp = Vector3Lerp(lp, rest, ph(u2, 0.8f, 0.94f));
                    L.vis = u2 < 0.95f || torch;
                } else {
                    lp = Vector3Lerp(lp, Vector3Add(side, V3(-0.04f, -0.01f, 0.02f)), ph(u2, 0.08f, 0.15f));
                    lp = Vector3Lerp(lp, Vector3Add(side, V3(-0.045f, -0.02f, 0.02f)), ph(u2, 0.2f, 0.24f));
                    lp.y += bell(u2, 0.24f, 0.3f) * 0.06f;   // swipe up: the cylinder spins
                    lp = Vector3Lerp(lp, rest, ph(u2, 0.3f, 0.42f));
                    L.vis = (u2 > 0.03f && u2 < 0.42f) || torch;
                }
                L.p = lp; if (torch && ((A == ANIM_RELOAD && u2 > 0.94f) || (A == ANIM_INSPECT && u2 > 0.42f))) { L.r = V3(0.3f, 0.32f, 0.05f); L.grip = 0; }
            }
        } else if (melee) {
            Matrix I = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixTranslate(0, -spinC, 0), MatrixRotateX(spinX)), MatrixRotateY(spinY)), MatrixTranslate(0, itemUp + spinC, 0)), H);   // spun about spinC up the item
            drawHeld(I, ht); drawHandAsset(H, true, R.grip == 1 ? 1 : 0);
            if (L.vis && !torch && (A != ANIM_INSPECT || (style == 3 && L.grip == 0))) { Matrix L2 = MatrixMultiply(MatrixTranslate(0, -0.17f, 0), H); drawHandAsset(L2, false, 0); L.vis = false; }
        } else if (ht >= 0 && isConsumable(ht)) {   // held for use; while it is being eaten or applied the other hand has it
            if (A != ANIM_EAT && A != ANIM_DRINK && A != ANIM_PILL && A != ANIM_BANDAGE) drawItemType(ht, MatrixMultiply(MatrixMultiply(MatrixScale(0.6f, 0.6f, 0.6f), MatrixTranslate(0, -0.035f, -0.01f)), H));
            drawHandAsset(H, true, (A == ANIM_EAT || A == ANIM_DRINK || A == ANIM_PILL || A == ANIM_BANDAGE) ? 2 : 0);
        } else drawHandAsset(H, true, R.grip);
        Vector3 wrist = Vector3Transform(V3(0.04f, -0.06f, 0.06f), MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixRotateZ(R.r.z), MatrixRotateX(R.r.x)), MatrixRotateY(R.r.y)), MatrixTranslate(R.p.x, R.p.y, R.p.z)));
        drawArm(B, wrist, V3(0.36f, -0.8f, 0.2f));
    }
    bool showTorch = torch && L.vis && L.grip == 0 && pl.searchC < 0 && pl.climbT <= 0 && A != ANIM_EAT && A != ANIM_DRINK && A != ANIM_PILL && A != ANIM_BANDAGE && A != ANIM_RELOAD;
    if (L.vis) {
        drawHand(B, L, false);
        if (showTorch) {   // a 2006 flip phone (keitai): keypad half in the hand, screen half open above it; the back LED is the light
            Matrix Hl = handM(B, L), Top = MatrixMultiply(MatrixMultiply(MatrixTranslate(0, 0.0f, 0), MatrixRotateX(-0.22f + (1 - phoneOpen) * 2.85f)), MatrixMultiply(MatrixTranslate(0, 0.075f, 0.0f), Hl));
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
    if (cyFallT < 1.2f) for (int i = 0; i < 6; i++) if (cyFallMask >> i & 1) {
        cyFallV[i].y -= 9.8f * vdt; cyFallP[i] = Vector3Add(cyFallP[i], Vector3Scale(cyFallV[i], vdt));
        Matrix M = MatrixMultiply(MatrixRotateXYZ(V3(cyFallT * 17 + i, cyFallT * 11, i * 0.7f)), MatrixTranslate(cyFallP[i].x, cyFallP[i].y, cyFallP[i].z));
        ADRAW("viewmodels/round_spent", M, WHITE, BX(T_METAL, 0, 0, 0.0035f, 0.0088f, 0.0088f, 0.043f, ((Color){150, 112, 52, 255})), BX(T_METAL, 0, 0, 0.0262f, 0.011f, 0.011f, 0.0025f, ((Color){160, 120, 56, 255})),
              BX(T_PLAIN, 0, 0, 0.0278f, 0.0038f, 0.0038f, 0.001f, ((Color){70, 60, 50, 255})));
    }
    if (ldFallT < 1.2f) { float T2 = ldFallT; Matrix M = MatrixMultiply(MatrixMultiply(MatrixRotateX(T2 * 3), ldFallM), MatrixTranslate(ldFallV.x * T2, ldFallV.y * T2 - 4.9f * T2 * T2, ldFallV.z * T2));
        ADRAW("viewmodels/speedloader", M, WHITE, BX(T_METAL, 0, 0, 0.034f, 0.036f, 0.026f, 0.009f, ((Color){30, 30, 34, 255})), BX(T_METAL, 0, 0, 0.034f, 0.026f, 0.036f, 0.009f, ((Color){30, 30, 34, 255})),
              BX(T_METAL, 0, 0, 0.046f, 0.012f, 0.012f, 0.016f, ((Color){50, 50, 56, 255})), BX(T_METAL, 0, 0, 0.055f, 0.016f, 0.016f, 0.004f, ((Color){60, 60, 66, 255}))); }
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
