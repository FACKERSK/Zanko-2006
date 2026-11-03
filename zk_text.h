// zk_text.h — real Japanese in the world: a subset Noto Sans CJK font (Data/UI/jp.ttf), a sign atlas (shop signs,
// station board, tunnel plates, notices), procedurally drawn manga pages, and the public-broadcast screen text.
static Font jpFont; static bool jpOk = false;
static RenderTexture2D signAtlasH, signAtlasV, mangaRT;   // horizontal 512x64 cells stacked; vertical 64x512 cells side by side; manga pages
#define SA_HW 512
#define SA_HH 64
#define MANGA_PAGES 6
#define MANGA_PW 256
#define MANGA_PH 360
static const char *signText(int i) {
    if (i < JP_NSHOP) return jpShop[i];
    switch (i) { case SGN_KONBINI: return jpKonbini; case SGN_KOBAN: return jpKoban; case SGN_STATION: return jpStation; case SGN_SUBWAY: return jpSubway;
                 case SGN_TUN0: return jpTunnel[0]; case SGN_TUN1: return jpTunnel[1]; case SGN_TUN2: return jpTunnel[2]; case SGN_TUN3: return jpTunnel[3]; case SGN_SEAL: return jpSeal; }
    return "";
}
static const char *signSub(int i) {
    switch (i) { case SGN_STATION: return jpStationSub; case SGN_SUBWAY: return jpSubwaySub; case SGN_SEAL: return jpSealSub; }
    return NULL;
}
static void jpDrawFit(const char *s, Rectangle r, Color c, bool vertical) {   // largest size that fits the box, centred
    if (!jpOk) return;
    if (!vertical) {
        float sz = r.height * 0.78f; Vector2 m = MeasureTextEx(jpFont, s, sz, 1);
        if (m.x > r.width * 0.92f) { sz *= r.width * 0.92f / m.x; m = MeasureTextEx(jpFont, s, sz, 1); }
        DrawTextEx(jpFont, s, (Vector2){r.x + (r.width - m.x) / 2, r.y + (r.height - m.y) / 2}, sz, 1, c);
    } else {   // one character per line, top to bottom
        int n = 0, cps[64]; int len = 0; int *cp = LoadCodepoints(s, &len); for (int k = 0; k < len && n < 64; k++) if (cp[k] != ' ') cps[n++] = cp[k]; UnloadCodepoints(cp);
        if (n == 0) return;
        float sz = fminf(r.width * 0.8f, r.height * 0.9f / n);
        for (int k = 0; k < n; k++) { Vector2 m = MeasureTextEx(jpFont, CodepointToUTF8(cps[k], &len), sz, 0); char b[8]; int L; const char *u = CodepointToUTF8(cps[k], &L); memcpy(b, u, L); b[L] = 0;
            m = MeasureTextEx(jpFont, b, sz, 0);
            DrawTextEx(jpFont, b, (Vector2){r.x + (r.width - m.x) / 2, r.y + (r.height - n * sz) / 2 + k * sz}, sz, 0, c); }
    }
}
static void mangaPage(int pg, float ox, float oy) {   // one page of an ink horror manga: panels, figures, speed lines, screentone, bubbles
    unsigned int r = 977u * (pg + 3);
    #define RN() ((r = r * 1103515245u + 12345u) >> 8 & 0xFFFF) / 65535.0f
    DrawRectangle((int)ox, (int)oy, MANGA_PW, MANGA_PH, (Color){236, 232, 222, 255});
    float y = oy + 10; int rows = 3 + (pg % 2);
    for (int row = 0; row < rows; row++) {
        float h = (MANGA_PH - 20 - (rows - 1) * 6) / rows; int cols = 1 + (int)(RN() * 2.2f); float x = ox + 10;
        for (int c = 0; c < cols; c++) {
            float w = (MANGA_PW - 20 - (cols - 1) * 6) / cols;
            Rectangle pr = {x, y, w, h};
            int style = (int)(RN() * 5);
            if (style == 0) { for (int k = 0; k < 40; k++) { float a = RN() * 6.28f; DrawLineEx((Vector2){pr.x + w / 2, pr.y + h / 2}, (Vector2){pr.x + w / 2 + cosf(a) * w, pr.y + h / 2 + sinf(a) * h}, 1, (Color){20, 20, 20, 255}); }   // speed lines
                DrawRectangle((int)(pr.x + w / 2 - 6), (int)(pr.y + h / 2 - 6), 12, 12, (Color){236, 232, 222, 255}); }
            else if (style == 1) { for (int yy = 0; yy < h; yy += 4) for (int xx = (yy / 4) % 2 * 2; xx < w; xx += 4) DrawPixel((int)(pr.x + xx), (int)(pr.y + yy), (Color){60, 60, 60, 255});   // screentone + a silhouette
                DrawCircle((int)(pr.x + w * 0.5f), (int)(pr.y + h * 0.35f), h * 0.16f, (Color){15, 15, 15, 255}); DrawRectangle((int)(pr.x + w * 0.5f - h * 0.14f), (int)(pr.y + h * 0.45f), (int)(h * 0.28f), (int)(h * 0.6f), (Color){15, 15, 15, 255}); }
            else if (style == 2) { DrawRectangleRec(pr, (Color){18, 18, 20, 255});   // a dark corridor with a figure at the end
                for (int k = 0; k < 6; k++) DrawRectangleLines((int)(pr.x + k * w / 14), (int)(pr.y + k * h / 14), (int)(w - k * w / 7), (int)(h - k * h / 7), (Color){90, 90, 95, 255});
                DrawRectangle((int)(pr.x + w / 2 - 3), (int)(pr.y + h / 2 - 8), 6, 16, (Color){230, 230, 230, 255}); }
            else if (style == 3) { DrawCircle((int)(pr.x + w * 0.5f), (int)(pr.y + h * 0.55f), fminf(w, h) * 0.32f, (Color){240, 238, 230, 255});   // a face, eyes wide
                DrawCircleLines((int)(pr.x + w * 0.5f), (int)(pr.y + h * 0.55f), fminf(w, h) * 0.32f, BLACK);
                for (int e = -1; e <= 1; e += 2) { DrawCircle((int)(pr.x + w * 0.5f + e * fminf(w, h) * 0.12f), (int)(pr.y + h * 0.5f), 5, BLACK); DrawCircle((int)(pr.x + w * 0.5f + e * fminf(w, h) * 0.12f), (int)(pr.y + h * 0.5f), 1.5f, WHITE); }
                for (int k = 0; k < 12; k++) DrawLine((int)(pr.x + RN() * w), (int)pr.y, (int)(pr.x + RN() * w), (int)(pr.y + h * 0.35f), (Color){30, 30, 30, 255}); }
            else { for (int k = 0; k < 9; k++) DrawRectangle((int)(pr.x + k * w / 9), (int)(pr.y + h * (0.3f + 0.4f * RN())), (int)(w / 10), (int)(h), (Color){40, 40, 44, 255}); }   // city skyline
            DrawRectangleLinesEx(pr, 2, BLACK);
            if (RN() < 0.75f && jpOk) {   // speech bubble / sound effect
                const char *t = jpManga[(int)(RN() * JP_NMANGA) % JP_NMANGA];
                bool sfx = strlen(t) <= 12;
                Rectangle br = {pr.x + 6 + RN() * fmaxf(1, w - 96), pr.y + 6, fminf(w - 12, 90), fminf(h * 0.45f, 40)};
                if (!sfx) { DrawEllipse((int)(br.x + br.width / 2), (int)(br.y + br.height / 2), br.width / 2, br.height / 2, WHITE); DrawEllipseLines((int)(br.x + br.width / 2), (int)(br.y + br.height / 2), br.width / 2, br.height / 2, BLACK);
                    jpDrawFit(t, (Rectangle){br.x + 8, br.y + 6, br.width - 16, br.height - 12}, BLACK, false); }
                else jpDrawFit(t, br, (Color){10, 10, 10, 255}, false);
            }
            x += w + 6;
        }
        y += h + 6;
    }
    #undef RN
}
static void textInit(void) {
    if (FileExists(P("Data/UI/jp.ttf"))) {
        static char all[8192]; all[0] = 0;
        #define ADD(S) strncat(all, S, sizeof all - strlen(all) - 1)
        for (int i = 0; i < JP_NSHOP; i++) ADD(jpShop[i]);
        ADD(jpKonbini); ADD(jpKoban); ADD(jpStation); ADD(jpStationSub); for (int i = 0; i < 4; i++) ADD(jpTunnel[i]); ADD(jpSeal); ADD(jpSealSub);
        for (int i = 0; i < JP_NBCAST; i++) ADD(jpBcast[i]); ADD(jpTicker); ADD(jpNoSignal); for (int i = 0; i < JP_NMANGA; i++) ADD(jpManga[i]);
        ADD(jpVend); ADD(jpSubway); ADD(jpSubwaySub); ADD(jpMapTitle);
        ADD(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_abcdefghijklmnopqrstuvwxyz{|}~");
        #undef ADD
        int n = 0; int *cp = LoadCodepoints(all, &n);
        int uniq[2048]; int nu = 0; for (int i = 0; i < n && nu < 2048; i++) { bool seen = false; for (int j = 0; j < nu; j++) if (uniq[j] == cp[i]) { seen = true; break; } if (!seen) uniq[nu++] = cp[i]; }
        UnloadCodepoints(cp);
        jpFont = LoadFontEx(P("Data/UI/jp.ttf"), 48, uniq, nu); SetTextureFilter(jpFont.texture, TEXTURE_FILTER_BILINEAR);
        jpOk = jpFont.texture.id > 0;
        TraceLog(LOG_INFO, "JPFONT: %d glyphs, ok=%d", nu, jpOk);
    }
    // sign atlases: bright text on transparent; drawn over the sign's own glowing box
    signAtlasH = LoadRenderTexture(SA_HW, SA_HH * SGN_N); signAtlasV = LoadRenderTexture(SA_HH * SGN_N, SA_HW);
    BeginTextureMode(signAtlasH); ClearBackground(BLANK);
    for (int i = 0; i < SGN_N; i++) {
        Rectangle r = {8, i * SA_HH + 4, SA_HW - 16, SA_HH - 8};
        const char *sub = signSub(i);
        if (sub) { jpDrawFit(signText(i), (Rectangle){r.x, r.y, r.width, r.height * 0.68f}, WHITE, false); jpDrawFit(sub, (Rectangle){r.x, r.y + r.height * 0.66f, r.width, r.height * 0.34f}, WHITE, false); }
        else jpDrawFit(signText(i), r, WHITE, false);
    }
    EndTextureMode();
    BeginTextureMode(signAtlasV); ClearBackground(BLANK);
    for (int i = 0; i < SGN_N; i++) jpDrawFit(signText(i), (Rectangle){i * SA_HH + 4, 8, SA_HH - 8, SA_HW - 16}, WHITE, true);
    EndTextureMode();
    SetTextureFilter(signAtlasH.texture, TEXTURE_FILTER_BILINEAR); SetTextureFilter(signAtlasV.texture, TEXTURE_FILTER_BILINEAR);
    mangaRT = LoadRenderTexture(MANGA_PW * MANGA_PAGES, MANGA_PH);
    BeginTextureMode(mangaRT); ClearBackground(WHITE);
    for (int p = 0; p < MANGA_PAGES; p++) mangaPage(p, p * MANGA_PW, 0);
    EndTextureMode();
    GenTextureMipmaps(&mangaRT.texture); SetTextureFilter(mangaRT.texture, TEXTURE_FILTER_TRILINEAR);   // fine ink and screentone shimmer badly when point-sampled at arm's length
}
// a textured quad in world space: corners a (top-left) b (top-right) c (bottom-right) d (bottom-left); uv rectangle in texture pixels
static void texQuad(Texture2D tx, Vector3 a, Vector3 b, Vector3 c, Vector3 d, Rectangle src, Color tint, bool flipY) {
    float u0 = src.x / tx.width, u1 = (src.x + src.width) / tx.width, v0 = src.y / tx.height, v1 = (src.y + src.height) / tx.height;
    if (flipY) { v0 = 1 - v0; v1 = 1 - v1; }   // render textures are stored upside down
    rlDrawRenderBatchActive(); rlSetTexture(tx.id);
    rlBegin(RL_QUADS); rlColor4ub(tint.r, tint.g, tint.b, tint.a);
    Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(d, a))); rlNormal3f(n.x, n.y, n.z);
    rlTexCoord2f(u0, v0); rlVertex3f(a.x, a.y, a.z); rlTexCoord2f(u0, v1); rlVertex3f(d.x, d.y, d.z);
    rlTexCoord2f(u1, v1); rlVertex3f(c.x, c.y, c.z); rlTexCoord2f(u1, v0); rlVertex3f(b.x, b.y, b.z);
    rlTexCoord2f(u0, v0); rlVertex3f(a.x, a.y, a.z); rlTexCoord2f(u1, v0); rlVertex3f(b.x, b.y, b.z);   // back side too
    rlTexCoord2f(u1, v1); rlVertex3f(c.x, c.y, c.z); rlTexCoord2f(u0, v1); rlVertex3f(d.x, d.y, d.z);
    rlEnd(); rlSetTexture(0); rlDrawRenderBatchActive();
}
// the words on a sign box: on its street face (horizontal signs) or both faces of a blade sign (vertical)
static void drawSignText(Sign *s, Color c) {
    Vector3 mn = s->mn, mx = s->mx; float e = 0.012f;
    if (s->tex == T_SIGNH) {
        Rectangle src = {0, (float)s->text * SA_HH, SA_HW, SA_HH};
        Texture2D t = signAtlasH.texture;
        switch (s->face) {
        case 0: texQuad(t, V3(mx.x, mx.y, mn.z - e), V3(mn.x, mx.y, mn.z - e), V3(mn.x, mn.y, mn.z - e), V3(mx.x, mn.y, mn.z - e), src, c, true); break;
        case 2: texQuad(t, V3(mn.x, mx.y, mx.z + e), V3(mx.x, mx.y, mx.z + e), V3(mx.x, mn.y, mx.z + e), V3(mn.x, mn.y, mx.z + e), src, c, true); break;
        case 1: texQuad(t, V3(mx.x + e, mx.y, mx.z), V3(mx.x + e, mx.y, mn.z), V3(mx.x + e, mn.y, mn.z), V3(mx.x + e, mn.y, mx.z), src, c, true); break;
        default: texQuad(t, V3(mn.x - e, mx.y, mn.z), V3(mn.x - e, mx.y, mx.z), V3(mn.x - e, mn.y, mx.z), V3(mn.x - e, mn.y, mn.z), src, c, true); break;
        }
    } else {
        Rectangle src = {(float)s->text * SA_HH, 0, SA_HH, SA_HW};
        Texture2D t = signAtlasV.texture;
        bool thinX = (mx.x - mn.x) < (mx.z - mn.z);   // blade lies in the z-y plane
        if (thinX) { texQuad(t, V3(mn.x - e, mx.y, mn.z), V3(mn.x - e, mx.y, mx.z), V3(mn.x - e, mn.y, mx.z), V3(mn.x - e, mn.y, mn.z), src, c, true);
                     texQuad(t, V3(mx.x + e, mx.y, mx.z), V3(mx.x + e, mx.y, mn.z), V3(mx.x + e, mn.y, mn.z), V3(mx.x + e, mn.y, mx.z), src, c, true); }
        else { texQuad(t, V3(mx.x, mx.y, mn.z - e), V3(mn.x, mx.y, mn.z - e), V3(mn.x, mn.y, mn.z - e), V3(mx.x, mn.y, mn.z - e), src, c, true);
               texQuad(t, V3(mn.x, mx.y, mx.z + e), V3(mx.x, mx.y, mx.z + e), V3(mx.x, mn.y, mx.z + e), V3(mn.x, mn.y, mx.z + e), src, c, true); }
    }
}
