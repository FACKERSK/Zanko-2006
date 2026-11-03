// panel3d.h — Silent Aero menus as real 3D glass slabs placed on a sphere around the camera (the character),
// each one turned to face it. Panel contents are drawn with the aero.h widgets into a texture; gel buttons,
// checkbox cubes, slider thumbs and caption capsules are meshes lit with the banded toon shader from silent.h.
// Requires aero.h and silent.h (saInit must have run).
#ifndef PANEL3D_H
#define PANEL3D_H

#define P3MAX 16   // 0-7 menus, 8-15 the in-game HUD
typedef struct {
    bool used; int frame;            // 0 bare texture, 1 window with a caption bar, 2 gadget slab (no caption)
    float yaw, pitch, wpx, hpx;      // placement on the sphere (radians) and size in 720p reference pixels
    RenderTexture2D rt; int rtw, rth;
    Vector3 c, rx, uy, n; Matrix M;
    AW3 w[AW3MAX]; int nw;
    float hover;                     // eases toward 1 while the mouse is over the slab
} P3;
static P3 p3[P3MAX];
static Camera3D p3Cam; static float p3R = 4.2f, p3Upp = 0.01f, p3Q = 1.0f;
static float p3SavedU;
static float p3Fov = 45;   // a wider lens shrinks a whole panel layout (the inventory spreads three panels across the screen)
static const float P3_FRONT = 0.17f;   // z of the texture plane in front of the slab face

static Vector3 p3Dir(float yaw, float pitch) { return (Vector3){sinf(yaw) * cosf(pitch), sinf(pitch), -cosf(yaw) * cosf(pitch)}; }
// screen point (720p reference px from the centre, y down) -> yaw / pitch in degrees on the sphere
static Vector2 p3At(float xr, float yr) { float k = 2 * tanf(p3Fov * DEG2RAD / 2) / 720.0f; return (Vector2){atanf(xr * k) * RAD2DEG, atan2f(-yr * k, sqrtf(1 + xr * k * xr * k)) * RAD2DEG}; }
// call once per frame before any panel
static bool p3Hud; static Vector2 p3HudLook; static float p3HudScale = 0.8f;   // HUD slabs are built smaller than menu slabs: they sit further off in view   // HUD mode: the slabs swing with the head (sway / landing) instead of the mouse
static void p3Begin(float t) {
    int SW = GetScreenWidth(), SH = GetScreenHeight();
    Vector2 m = aRealMouse();
    float mx = (m.x / SW - 0.5f), my = (m.y / SH - 0.5f);
    static float px = 0, py = 0; px += (mx - px) * 0.08f; py += (my - py) * 0.08f;   // eased parallax
    if (p3Hud) { p3Cam = (Camera3D){0}; p3Cam.up = (Vector3){0, 1, 0}; p3Cam.fovy = p3Fov; p3Cam.projection = CAMERA_PERSPECTIVE;
        p3Cam.target = p3Dir(p3HudLook.x, p3HudLook.y);
        p3Upp = p3HudScale * 2 * p3R * tanf(45 * DEG2RAD / 2) / 720.0f; p3Q = SH / 720.0f;
        for (int i = 0; i < P3MAX; i++) p3[i].used = false;
        return; }
    p3Cam = (Camera3D){0}; p3Cam.position = (Vector3){0, 0, 0}; p3Cam.up = (Vector3){0, 1, 0}; p3Cam.fovy = p3Fov; p3Cam.projection = CAMERA_PERSPECTIVE;
    float yaw = px * 0.10f + sinf(t * 0.21f) * 0.006f, pitch = -py * 0.07f + sinf(t * 0.17f) * 0.004f;
    p3Cam.target = p3Dir(yaw, pitch);
    p3Upp = 2 * p3R * tanf(45 * DEG2RAD / 2) / 720.0f;   // one 720p-reference pixel at the sphere radius
    p3Q = SH / 720.0f;
    for (int i = 0; i < P3MAX; i++) p3[i].used = false;
}
// Begin drawing panel <id>. Inside, draw widgets in a rectangle {0, 0, wpx*aU, hpx*aU}. Always pair with p3End().
static float p3FitMargin = 0;   // > 0: p3Panel nudges each slab onto the screen before the mouse test
static void p3Orient(P3 *p) {   // place the slab on the sphere from its yaw / pitch, facing the eye
    Vector3 d = p3Dir(p->yaw, p->pitch);
    p->c = Vector3Scale(d, p3R); p->n = Vector3Negate(d);
    p->rx = Vector3Normalize(Vector3CrossProduct((Vector3){0, 1, 0}, p->n)); p->uy = Vector3CrossProduct(p->n, p->rx);
    p->M = (Matrix){p->rx.x, p->uy.x, p->n.x, p->c.x,  p->rx.y, p->uy.y, p->n.y, p->c.y,  p->rx.z, p->uy.z, p->n.z, p->c.z,  0, 0, 0, 1};
}
// keep a slab (including its slab rim and drip marks) fully on screen with a margin in screen px: nudges yaw / pitch by measuring the projected corners
static void p3FitP(P3 *p, float mpx) {
    int SW = GetScreenWidth(), SH = GetScreenHeight();
    float k = 2 * tanf(p3Fov * DEG2RAD / 2) / SH;
    for (int it = 0; it < 24; it++) {
        float W = p->wpx * p3Upp / 2 + 0.06f, H = p->hpx * p3Upp / 2 + 0.12f, x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
        for (int c = 0; c < 8; c++) { Vector3 v = Vector3Transform((Vector3){(c & 1) ? W : -W, (c & 2) ? H : -H, (c & 4) ? 0.2f : -0.15f}, p->M);
            Vector2 sp = GetWorldToScreenEx(v, p3Cam, SW, SH); x0 = fminf(x0, sp.x); x1 = fmaxf(x1, sp.x); y0 = fminf(y0, sp.y); y1 = fmaxf(y1, sp.y); }
        float dx = x0 < mpx ? mpx - x0 : x1 > SW - mpx ? (SW - mpx) - x1 : 0, dy = y0 < mpx ? mpx - y0 : y1 > SH - mpx ? (SH - mpx) - y1 : 0;
        if (fabsf(dx) < 1.0f && fabsf(dy) < 1.0f) break;
        p->yaw += dx * k * 0.4f; p->pitch -= dy * k * 0.4f; p3Orient(p);   // small steps: near the edges one radian moves the slab further on screen
    }
}
static void p3Fit(int id, float mpx) { p3FitP(&p3[id], mpx); }
static Rectangle p3Panel(int id, float yawDeg, float pitchDeg, float wpx, float hpx, int frame) {
    P3 *p = &p3[id];
    p->used = true; p->frame = frame; p->yaw = yawDeg * DEG2RAD; p->pitch = pitchDeg * DEG2RAD; p->wpx = wpx; p->hpx = hpx;
    int tw = (int)(wpx * p3Q + 0.5f), th = (int)(hpx * p3Q + 0.5f);
    if (tw != p->rtw || th != p->rth) { if (p->rtw) UnloadRenderTexture(p->rt); p->rt = LoadRenderTexture(tw, th); SetTextureFilter(p->rt.texture, TEXTURE_FILTER_BILINEAR); p->rtw = tw; p->rth = th; }
    Vector3 d = p3Dir(p->yaw, p->pitch);
    p->c = Vector3Scale(d, p3R); p->n = Vector3Negate(d);
    p->rx = Vector3Normalize(Vector3CrossProduct((Vector3){0, 1, 0}, p->n)); p->uy = Vector3CrossProduct(p->n, p->rx);
    p->M = (Matrix){p->rx.x, p->uy.x, p->n.x, p->c.x,  p->rx.y, p->uy.y, p->n.y, p->c.y,  p->rx.z, p->uy.z, p->n.z, p->c.z,  0, 0, 0, 1};
    if (p3FitMargin > 0) p3FitP(p, p3FitMargin);
    // mouse ray -> texture plane -> panel pixels
    Ray ray = GetScreenToWorldRay(aRealMouse(), p3Cam);
    Vector3 pc = Vector3Add(p->c, Vector3Scale(p->n, P3_FRONT));
    float den = Vector3DotProduct(ray.direction, p->n);
    Vector2 local = {-99999, -99999};
    if (fabsf(den) > 1e-5f) {
        float tt = Vector3DotProduct(Vector3Subtract(pc, ray.position), p->n) / den;
        if (tt > 0) { Vector3 hit = Vector3Add(ray.position, Vector3Scale(ray.direction, tt)), dv = Vector3Subtract(hit, pc);
            float lx = Vector3DotProduct(dv, p->rx) / p3Upp + wpx / 2, ly = hpx / 2 - Vector3DotProduct(dv, p->uy) / p3Upp;
            if (lx >= 0 && ly >= 0 && lx <= wpx && ly <= hpx) local = (Vector2){lx * p3Q, ly * p3Q}; }
    }
    p->hover += ((local.x > -9999 ? 1.0f : 0.0f) - p->hover) * 0.12f;
    p3SavedU = aU; aU = p3Q;
    aMO = true; aMOp = local; aIn3D = true; aW3n = 0;
    BeginTextureMode(p->rt);
    ClearBackground((Color){0, 0, 0, 0});
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    return (Rectangle){0, 0, (float)tw, (float)th};
}
static void p3End(int id) {
    P3 *p = &p3[id];
    rlDrawRenderBatchActive();
    EndBlendMode(); EndTextureMode();
    memcpy(p->w, aW3, sizeof(AW3) * aW3n); p->nw = aW3n;
    aMO = false; aIn3D = false; aU = p3SavedU;
}
// ---- mesh parts in panel-local units (x right, y up, z toward the camera)
static void p3Part(P3 *p, Model *m, Vector3 pos, Vector3 rot, Vector3 sc, Color c) { saDraw(m, MatrixMultiply(saTRS(pos, rot, sc), p->M), c, saWhite); }
static Vector3 p3Px(P3 *p, float x, float y, float z) { return (Vector3){(x / p3Q - p->wpx / 2) * p3Upp, (p->hpx / 2 - y / p3Q) * p3Upp, z}; }   // texture px -> local
static void p3Gel(P3 *p, Rectangle r, int state, Color base) {
    float u = p3Upp / p3Q; float w = r.width * u, h = r.height * u, dz = 0.05f;
    Color c = base;
    if (state == 1) c = aAdd(base, 34);
    if (state == 2) c = aMul(base, 0.6f);
    float push = state == 2 ? -0.03f : 0;
    Vector3 ctr = p3Px(p, r.x + r.width / 2, r.y + r.height / 2, 0.1f + push);
    saMat(state == 3 ? 0.1f : 0.22f, state == 1 ? 1.4f : 0.35f, 0);
    if (state == 1) saV3(saLRimCol, (Vector3){0.44f, 0.84f, 0.82f});
    float rr = h / 2, body = fmaxf(w - h, 0.001f);
    p3Part(p, &saSphere, (Vector3){ctr.x - body / 2, ctr.y, ctr.z}, (Vector3){0, 0, 0}, (Vector3){rr, rr, dz}, c);
    p3Part(p, &saSphere, (Vector3){ctr.x + body / 2, ctr.y, ctr.z}, (Vector3){0, 0, 0}, (Vector3){rr, rr, dz}, c);
    p3Part(p, &saCyl, (Vector3){ctr.x - body / 2, ctr.y, ctr.z}, (Vector3){0, 0, -PI / 2}, (Vector3){rr, body, dz}, c);
    saV3(saLRimCol, (Vector3){0.55f, 0.85f, 0.82f});
    // split gloss: a flat highlight strip across the upper half
    saMat(0, 0, 1);
    p3Part(p, &saCube, (Vector3){ctr.x, ctr.y + h * 0.2f, ctr.z + dz * 0.98f}, (Vector3){0, 0, 0}, (Vector3){body + rr * 0.6f, h * 0.3f, 0.002f}, (Color){255, 255, 255, (unsigned char)(state == 2 ? 20 : state == 3 ? 18 : 46)});
    if (state != 3) p3Part(p, &saSphereLo, (Vector3){ctr.x - body / 2 - rr * 0.35f, ctr.y + rr * 0.45f, ctr.z + dz * 0.9f}, (Vector3){0, 0, 0}, (Vector3){rr * 0.14f, rr * 0.1f, 0.004f}, (Color){255, 255, 255, 235});   // hard specular dot
    if (state != 3) for (int k = 0; k < 3; k++) {   // soot specks
        unsigned int hh = (unsigned int)(r.x * 7 + r.y * 13 + k * 101);
        p3Part(p, &saSphereLo, (Vector3){ctr.x - body / 2 + (hh % 97) / 97.0f * body, ctr.y - h * 0.15f + (hh % 13) / 13.0f * h * 0.3f, ctr.z + dz * 0.95f}, (Vector3){0, 0, 0}, (Vector3){0.006f, 0.006f, 0.003f}, (Color){40, 30, 22, 255});
    }
}
static void p3Slab(P3 *p, int stage, float closeHover) {
    float W = p->wpx * p3Upp, H = p->hpx * p3Upp, u = p3Upp;
    Color frame = stage == 3 ? (Color){110, 52, 34, 255} : stage == 2 ? (Color){96, 98, 96, 255} : stage == 1 ? (Color){104, 110, 110, 255} : (Color){92, 112, 116, 255};
    saMat(0.3f, 0.2f, 0);
    p3Part(p, &saCube, (Vector3){6 * u, -9 * u, -0.14f}, (Vector3){0, 0, 0}, (Vector3){W + 0.04f, H + 0.04f, 0.02f}, (Color){10, 12, 12, 255});
    p3Part(p, &saCube, (Vector3){0, 0, -0.05f}, (Vector3){0, 0, 0}, (Vector3){W + 0.06f, H + 0.06f, 0.1f}, (Color){18, 24, 26, 255});
    saMat(0.6f, 1.1f + p->hover * 0.6f, 0);
    p3Part(p, &saCube, (Vector3){0, 0, 0}, (Vector3){0, 0, 0}, (Vector3){W, H, 0.14f}, frame);
    saMat(0, 0, 1);
    p3Part(p, &saCube, (Vector3){0, H / 4, 0.0705f}, (Vector3){0, 0, 0}, (Vector3){W - 0.03f, H / 2 - 0.03f, 0.002f}, (Color){255, 255, 255, 26});
    p3Part(p, &saCube, (Vector3){0, H / 2 - 0.012f, 0.072f}, (Vector3){0, 0, 0}, (Vector3){W - 0.02f, 0.02f, 0.01f}, (Color){200, 214, 210, 255});
    p3Part(p, &saCube, (Vector3){-W / 2 + 0.012f, 0, 0.072f}, (Vector3){0, 0, 0}, (Vector3){0.02f, H - 0.02f, 0.01f}, (Color){150, 166, 164, 255});
    p3Part(p, &saCube, (Vector3){0, -H / 2 + 0.012f, 0.072f}, (Vector3){0, 0, 0}, (Vector3){W - 0.02f, 0.02f, 0.01f}, (Color){20, 26, 28, 255});
    // rust stain and drips under the frame
    p3Part(p, &saCube, (Vector3){-W / 4, -H / 2 + 0.035f, 0.073f}, (Vector3){0, 0, 0}, (Vector3){W * 0.4f, 0.04f, 0.004f}, (Color){92, 58, 32, 170});
    int nd = (int)(W / 0.45f);
    for (int k = 0; k < nd; k++) p3Part(p, &saCube, (Vector3){-W / 2 + 0.2f + k * 0.45f + (k % 3) * 0.07f, -H / 2 - 0.03f - (k % 2) * 0.04f, 0.06f}, (Vector3){0, 0, 0}, (Vector3){0.016f, 0.07f + (k % 2) * 0.08f, 0.005f}, (Color){80, 48, 26, 200});
    if (p->frame != 1) return;
    // raised caption bar (texture px 6..W-6, 5..36)
    float capH = 31 * u, capY = H / 2 - 20.5f * u;
    saMat(0.2f, 0.6f, 0);
    p3Part(p, &saCube, (Vector3){0, capY, 0.09f}, (Vector3){0, 0, 0}, (Vector3){W - 12 * u, capH, 0.04f}, stage == 3 ? (Color){130, 70, 50, 255} : (Color){120, 140, 142, 255});
    saMat(0, 0, 1);
    p3Part(p, &saCube, (Vector3){0, capY + capH / 4, 0.111f}, (Vector3){0, 0, 0}, (Vector3){W - 14 * u, capH / 2, 0.002f}, (Color){255, 255, 255, 34});
    // caption buttons are drawn into the texture (Windows Vista style), a thin dark plate gives them depth
    saMat(0.2f, 0.4f, 0);
    p3Part(p, &saCube, (Vector3){W / 2 - 6 * u - 8 * u - 48 * u + 2 * u, H / 2 - 11 * u, 0.115f}, (Vector3){0, 0, 0}, (Vector3){100 * u, 21 * u, 0.04f}, (Color){14, 18, 20, 255});
    if (stage == 3) {   // an eye opens in the caption bar and follows the cursor
        Vector2 m = aRealMouse(); float dx = Clamp((m.x / GetScreenWidth() - 0.5f) * 2, -1, 1), dy = Clamp((m.y / GetScreenHeight() - 0.5f) * 2, -1, 1);
        saMat(2.0f, 0.6f, 0);
        p3Part(p, &saSphere, (Vector3){W * 0.05f, capY, 0.12f}, (Vector3){0, 0, 0}, (Vector3){26 * u, 9 * u, 0.04f}, (Color){225, 205, 165, 255});
        p3Part(p, &saSphere, (Vector3){W * 0.05f + dx * 12 * u, capY - dy * 3 * u, 0.15f}, (Vector3){0, 0, 0}, (Vector3){7 * u, 7 * u, 0.02f}, (Color){70, 14, 10, 255});
        saMat(1.6f, 0.4f, 0);
        for (int k = 0; k < 7; k++) p3Part(p, &saSphere, (Vector3){-W / 2 + 0.3f + k * (W - 0.6f) / 6, -H / 2 - 0.08f - (k % 3) * 0.06f, 0.06f}, (Vector3){0, 0, 0}, (Vector3){0.022f, 0.06f + (k % 3) * 0.03f, 0.022f}, (Color){90, 20, 14, 255});
        saMat(0.4f, 0.2f, 0);
        for (int k = -7; k <= 7; k++) {
            p3Part(p, &saCube, (Vector3){k * W / 14, 0, 0.2f}, (Vector3){0, 0, 0.785f}, (Vector3){0.008f, H * 1.3f, 0.006f}, (Color){60, 26, 16, 80});
            p3Part(p, &saCube, (Vector3){k * W / 14, 0, 0.2f}, (Vector3){0, 0, -0.785f}, (Vector3){0.008f, H * 1.3f, 0.006f}, (Color){60, 26, 16, 80});
        }
    }
}
static void p3Widgets(P3 *p) {
    for (int i = 0; i < p->nw; i++) {
        AW3 *w = &p->w[i];
        if (w->kind == 0) p3Gel(p, w->r, w->state, w->base);
        else if (w->kind == 1) {   // glass cube checkbox
            float u = p3Upp / p3Q; Vector3 c = p3Px(p, w->r.x + w->r.width / 2, w->r.y + w->r.height / 2, 0.11f);
            saMat(1.4f, w->state ? 1.6f : 0.8f, 0);
            p3Part(p, &saCube, c, (Vector3){0.12f, 0.18f, 0}, (Vector3){w->r.width * u, w->r.height * u, 0.06f}, w->state ? (Color){224, 218, 200, 255} : w->base);
        } else if (w->kind == 2) {   // slider thumb: chrome bubble
            float u = p3Upp / p3Q; Vector3 c = p3Px(p, w->r.x + w->r.width / 2, w->r.y + w->r.height / 2, 0.12f);
            saMat(3.0f, w->state ? 1.6f : 1.0f, 0);
            p3Part(p, &saSphere, c, (Vector3){0, 0, 0}, (Vector3){w->r.width * u * 0.5f, w->r.height * u * 0.5f, 0.04f}, w->state ? (Color){210, 216, 214, 255} : w->base);
        }
    }
}
static void p3Quad(P3 *p) {
    float W = p->wpx * p3Upp, H = p->hpx * p3Upp;
    Vector3 c[4] = {{-W / 2, H / 2, P3_FRONT}, {-W / 2, -H / 2, P3_FRONT}, {W / 2, -H / 2, P3_FRONT}, {W / 2, H / 2, P3_FRONT}};
    float uv[4][2] = {{0, 1}, {0, 0}, {1, 0}, {1, 1}};
    rlDrawRenderBatchActive();
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    rlSetTexture(p->rt.texture.id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255); rlNormal3f(p->n.x, p->n.y, p->n.z);
    for (int k = 0; k < 4; k++) { Vector3 v = Vector3Transform(c[k], p->M); rlTexCoord2f(uv[k][0], uv[k][1]); rlVertex3f(v.x, v.y, v.z); }
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    EndBlendMode();
}
// draw every panel used this frame (on top of whatever background is already on screen)
static void p3Render(int stage) {
    BeginMode3D(p3Cam);
    BeginShaderMode(saSh);
    float snap[2] = {(float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2}; SetShaderValue(saSh, saLSnap, snap, SHADER_UNIFORM_VEC2);
    saV3(saLView, p3Cam.position); saV3(saLLight, Vector3Normalize((Vector3){-0.45f, -0.55f, -0.7f}));
    bool hell = stage == 3;
    saV3(saLLCol, hell ? (Vector3){0.95f, 0.55f, 0.4f} : (Vector3){0.95f, 0.84f, 0.66f});
    saV3(saLAmb, hell ? (Vector3){0.34f, 0.22f, 0.2f} : (Vector3){0.36f, 0.38f, 0.4f});
    saV3(saLRimCol, (Vector3){0.55f, 0.85f, 0.82f});
    saF(saLNoFog, 1); saF(saLGFog, 0); saF(saLBub, 0); saF(saLSky, 0); saF(saLDirt, 0);
    EndShaderMode();
    for (int i = 0; i < P3MAX; i++) if (p3[i].used) {
        P3 *p = &p3[i];
        float ch = 0; for (int k = 0; k < p->nw; k++) if (p->w[k].kind == 3 && p->w[k].state) ch = 1;
        if (p->frame) p3Slab(p, stage, ch);
        p3Widgets(p);
    }
    for (int i = 0; i < P3MAX; i++) if (p3[i].used) p3Quad(&p3[i]);
    saF(saLNoFog, 0);
    EndMode3D();
}
#endif
