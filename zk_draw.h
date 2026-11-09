// zk_draw.h — item models, held weapons, base pieces, furniture, junk piles, new creatures, the hills
#define CSK {150, 30, 26, 255}
static void glowEyes(Matrix H, Vector3 a, Vector3 b, float s, Color c) {   // two hard emissive pin-points
    setEmis(1);
    ADRAW("creatures/common/eye_l", MatrixMultiply(MatrixTranslate(a.x, a.y, a.z), H), c, BX(T_PLAIN, 0, 0, 0, 0.035f, 0.025f, 0.02f, CWH));
    ADRAW("creatures/common/eye_r", MatrixMultiply(MatrixTranslate(b.x, b.y, b.z), H), c, BX(T_PLAIN, 0, 0, 0, 0.035f, 0.025f, 0.02f, CWH));
    setEmis(0); (void)s;
}
// ---- creatures 5..9
static void drawCreature2(Cr *c, Matrix base, Color tn, float *w, float a, float wind) {
    bool alive = c->state != CS_DEAD;
    if (c->type == 5) {   // stalker: a salaryman stretched to 2.4 m, head snapped sideways, no face
        for (int s = -1; s <= 1; s += 2) { Matrix L = frameM(base, V3(0.1f * s, 1.25f, 0), sinf(a * 0.6f) * 0.3f * s + w[s > 0 ? 0 : 1] * 0.2f, 0, 0);
            ADRAW("creatures/stalker/leg", L, tn, BX(T_CLOTH, 0, -0.62f, 0, 0.13f, 1.25f, 0.13f, ((Color){30, 30, 34, 255})), BX(T_PLAIN, 0, -1.24f, 0.06f, 0.12f, 0.06f, 0.26f, ((Color){15, 15, 15, 255}))); }
        Matrix Tm = frameM(base, V3(0, 1.25f, 0), 0.12f + w[2] * 0.2f + wind * 0.5f, w[3] * 0.2f, w[4] * 0.15f);
        ADRAW("creatures/stalker/torso", Tm, tn, BX(T_CLOTH, 0, 0.42f, 0, 0.42f, 0.85f, 0.2f, ((Color){32, 32, 36, 255})), BX(T_PLAIN, 0, 0.55f, 0.101f, 0.12f, 0.6f, 0.01f, ((Color){225, 222, 215, 255})),
              BX(T_PLAIN, 0, 0.52f, 0.108f, 0.05f, 0.42f, 0.01f, ((Color){130, 20, 20, 255})));
        for (int s = -1; s <= 1; s += 2) { Matrix Am = frameM(Tm, V3(0.24f * s, 0.8f, 0), -0.1f + w[4] * 0.3f - wind * 1.9f, 0, 0.05f * s);
            ADRAW("creatures/stalker/arm", Am, tn, BX(T_CLOTH, 0, -0.6f, 0, 0.1f, 1.2f, 0.1f, ((Color){32, 32, 36, 255})), BX(T_FLESH, 0, -1.32f, 0.02f, 0.1f, 0.26f, 0.05f, ((Color){200, 190, 182, 255})),
                  BX(T_FLESH, 0.03f, -1.55f, 0.02f, 0.02f, 0.22f, 0.02f, ((Color){200, 190, 182, 255})), BX(T_FLESH, -0.03f, -1.55f, 0.02f, 0.02f, 0.22f, 0.02f, ((Color){200, 190, 182, 255}))); }
        Matrix Hm = MatrixMultiply(MatrixRotateY(crLook(c)), frameM(Tm, V3(0, 0.86f, 0), w[0] * 0.3f, w[1] * 0.4f, 1.45f + w[5] * 0.2f));
        ADRAW("creatures/stalker/head", Hm, tn, BX(T_FLESH, 0, 0.16f, 0.02f, 0.22f, 0.32f, 0.24f, ((Color){215, 205, 198, 255})), BX(T_PLAIN, 0, 0.31f, -0.01f, 0.23f, 0.06f, 0.25f, ((Color){15, 15, 15, 255})));
    } else if (c->type == 6) {   // screamer: hunched, hair over the face, jaw that unhinges
        Matrix Rm = frameM(base, V3(0, 0.9f, 0), 0.55f + w[2] * 0.3f + wind * 0.4f, w[3] * 0.2f, w[4] * 0.2f);
        for (int s = -1; s <= 1; s += 2) { Matrix L = frameM(base, V3(0.11f * s, 0.9f, 0), sinf(a) * 0.4f * s, 0, 0.08f * s);
            ADRAW("creatures/screamer/leg", L, tn, BX(T_FLESH, 0, -0.45f, 0, 0.1f, 0.9f, 0.1f, CFL)); }
        ADRAW("creatures/screamer/body", Rm, tn, BX(T_CLOTH, 0, 0.35f, 0, 0.4f, 0.75f, 0.28f, ((Color){150, 120, 105, 255})), BX(T_CLOTH, 0, -0.15f, 0, 0.5f, 0.35f, 0.34f, ((Color){110, 45, 38, 255})));
        for (int s = -1; s <= 1; s += 2) { Matrix Am = frameM(Rm, V3(0.22f * s, 0.65f, 0.05f), -0.9f + w[4] * 0.5f - wind, 0, 0.2f * s + w[5] * 0.3f);
            ADRAW("creatures/screamer/arm", Am, tn, BX(T_FLESH, 0, -0.45f, 0, 0.06f, 0.9f, 0.06f, CFL), BX(T_FLESH, 0, -0.95f, 0.05f, 0.1f, 0.12f, 0.16f, ((Color){90, 40, 35, 255}))); }
        float open = c->scream > 10.5f ? 1 : fabsf(w[0]) * 0.5f;
        Matrix Hm = MatrixMultiply(MatrixRotateY(crLook(c)), frameM(Rm, V3(0, 0.75f, 0.05f), -0.3f + w[1] * 0.8f, w[2] * 0.6f, w[3] * 0.6f));
        ADRAW("creatures/screamer/head", Hm, tn, BX(T_FLESH, 0, 0.14f, 0, 0.22f, 0.26f, 0.22f, CFL), BX(T_CLOTH, 0, 0.1f, 0.02f, 0.27f, 0.6f, 0.28f, ((Color){12, 10, 10, 255})));
        if (alive) glowEyes(Hm, V3(-0.05f, 0.16f, 0.142f), V3(0.06f, 0.15f, 0.142f), c->state == CS_CHASE ? 1 : 0.5f, (Color){255, 40, 30, 255});
        Matrix J = frameM(Hm, V3(0, 0.02f, 0.1f), 0.2f + open * 0.9f, 0, 0);
        ADRAW("creatures/screamer/jaw", J, tn, BX(T_FLESH, 0, -0.16f, 0.02f, 0.18f, 0.32f, 0.08f, ((Color){110, 20, 18, 255})));
    } else if (c->type == 7) {   // mannequin: glossy white shop dummy, segmented joints, cyan price tag
        Color pl2 = alive ? (Color){235, 238, 235, 255} : (Color){180, 180, 175, 255};
        for (int s = -1; s <= 1; s += 2) { Matrix L = frameM(base, V3(0.1f * s, 0.95f, 0), w[s > 0 ? 0 : 1] * 0.5f, 0, 0);
            ADRAW("creatures/mannequin/leg", L, pl2, BX(T_PLAIN, 0, -0.24f, 0, 0.12f, 0.46f, 0.12f, CWH), BX(T_PLAIN, 0, -0.5f, 0, 0.08f, 0.06f, 0.08f, ((Color){150, 155, 155, 255})), BX(T_PLAIN, 0, -0.74f, 0, 0.11f, 0.44f, 0.11f, CWH)); }
        Matrix Tm = frameM(base, V3(0, 0.95f, 0), w[2] * 0.6f, w[3] * 0.8f, w[4] * 0.4f);
        ADRAW("creatures/mannequin/torso", Tm, pl2, BX(T_PLAIN, 0, 0.12f, 0, 0.3f, 0.24f, 0.2f, CWH), BX(T_PLAIN, 0, 0.27f, 0, 0.1f, 0.05f, 0.1f, ((Color){150, 155, 155, 255})), BX(T_PLAIN, 0, 0.48f, 0, 0.4f, 0.38f, 0.22f, CWH),
              BX(T_PLAIN, 0.12f, 0.5f, 0.115f, 0.08f, 0.1f, 0.01f, ((Color){90, 220, 210, 255})));
        for (int s = -1; s <= 1; s += 2) { Matrix Am = frameM(Tm, V3(0.23f * s, 0.62f, 0), w[5] * 1.2f - wind * 1.5f, 0, 0.1f * s + w[0] * 0.8f);
            ADRAW("creatures/mannequin/arm", Am, pl2, BX(T_PLAIN, 0, -0.3f, 0, 0.08f, 0.6f, 0.08f, CWH), BX(T_PLAIN, 0, -0.66f, 0.02f, 0.07f, 0.12f, 0.04f, CWH)); }
        Matrix Hm = MatrixMultiply(MatrixRotateY(crLook(c)), frameM(Tm, V3(0, 0.67f, 0), w[1] * 1.5f, w[2] * 2.5f, w[3] * 1.5f));
        ADRAW("creatures/mannequin/head", Hm, pl2, BX(T_PLAIN, 0, 0.04f, 0, 0.06f, 0.08f, 0.06f, ((Color){150, 155, 155, 255})), BX(T_PLAIN, 0, 0.2f, 0.01f, 0.2f, 0.27f, 0.22f, CWH));
    } else if (c->type == 8) {   // hanger: emaciated, too many joints, waits upside down on ceilings
        Matrix B2 = c->hanging ? MatrixMultiply(MatrixMultiply(MatrixRotateZ(PI), MatrixRotateY(c->yaw)), MatrixTranslate(c->pos.x, c->pos.y, c->pos.z)) : base;
        Matrix Rm = frameM(B2, V3(0, 0.38f, 0), w[2] * 0.2f, 0, w[3] * 0.25f);
        ADRAW("creatures/hanger/body", Rm, tn, BX(T_FLESH, 0, 0, 0, 0.3f, 0.18f, 1.1f, ((Color){150, 135, 128, 255})), BX(T_FLESH, 0, 0.1f, -0.2f, 0.16f, 0.06f, 0.6f, ((Color){100, 85, 80, 255})));
        Matrix Hm = MatrixMultiply(MatrixRotateY(crLook(c)), frameM(Rm, V3(0, 0.02f, 0.6f), w[0] * 2.0f, w[1] * 2.0f, w[5] * 2.0f));
        ADRAW("creatures/hanger/head", Hm, tn, BX(T_FLESH, 0, 0.0f, 0.12f, 0.2f, 0.18f, 0.26f, ((Color){175, 160, 152, 255})), BX(T_PLAIN, 0, -0.05f, 0.25f, 0.14f, 0.05f, 0.02f, ((Color){20, 4, 4, 255})));
        glowEyes(Hm, V3(-0.05f, 0.03f, 0.25f), V3(0.05f, 0.03f, 0.25f), 1, (Color){255, 230, 200, 255});
        for (int k = 0; k < 6; k++) {
            float s = (k & 1) ? 1.0f : -1.0f, z = (k / 2 - 1) * 0.45f;
            Matrix L1 = frameM(Rm, V3(0.15f * s, 0, z), sinf(a * 1.3f + k * 1.7f) * 0.5f, 0, 1.0f * s);
            ADRAW("creatures/hanger/limb_a", L1, tn, BX(T_FLESH, 0, -0.3f, 0, 0.05f, 0.6f, 0.05f, ((Color){130, 115, 108, 255})));
            Matrix L2 = frameM(L1, V3(0, -0.6f, 0), 0, 0, -1.6f * s + w[k % 6] * 0.4f);
            ADRAW("creatures/hanger/limb_b", L2, tn, BX(T_FLESH, 0, -0.3f, 0, 0.04f, 0.6f, 0.04f, ((Color){110, 95, 90, 255})));
        }
    } else {   // 9 station attendant: huge, in uniform, face hidden behind a paper notice
        for (int s = -1; s <= 1; s += 2) { Matrix L = frameM(base, V3(0.18f * s, 1.0f, 0), sinf(a * 0.5f) * 0.25f * s, 0, 0);
            ADRAW("creatures/attendant/leg", L, tn, BX(T_CLOTH, 0, -0.5f, 0, 0.22f, 1.0f, 0.24f, ((Color){30, 40, 70, 255})), BX(T_PLAIN, 0, -1.0f, 0.06f, 0.24f, 0.08f, 0.36f, ((Color){15, 15, 15, 255}))); }
        Matrix Tm = frameM(base, V3(0, 1.0f, 0), 0.1f + w[2] * 0.1f + wind * 0.3f, w[3] * 0.1f, 0);
        ADRAW("creatures/attendant/torso", Tm, tn, BX(T_CLOTH, 0, 0.5f, 0, 0.8f, 1.0f, 0.46f, ((Color){32, 44, 78, 255})), BX(T_PLAIN, 0, 0.75f, 0.235f, 0.18f, 0.12f, 0.01f, ((Color){210, 180, 60, 255})));
        for (int s = -1; s <= 1; s += 2) { Matrix Am = frameM(Tm, V3(0.46f * s, 0.92f, 0), -0.2f - wind * 1.6f + w[4] * 0.1f, 0, 0.12f * s);
            ADRAW("creatures/attendant/arm", Am, tn, BX(T_CLOTH, 0, -0.45f, 0, 0.18f, 0.9f, 0.2f, ((Color){32, 44, 78, 255})), BX(T_PLAIN, 0, -0.98f, 0.02f, 0.2f, 0.2f, 0.16f, ((Color){235, 235, 230, 255}))); }
        Matrix Hm = MatrixMultiply(MatrixRotateY(crLook(c)), frameM(Tm, V3(0, 1.0f, 0), w[0] * 0.15f, w[1] * 0.3f, w[5] * 0.15f));
        ADRAW("creatures/attendant/head", Hm, tn, BX(T_FLESH, 0, 0.2f, 0, 0.36f, 0.4f, 0.36f, CFL), BX(T_PAPER, 0, 0.18f, 0.19f, 0.34f, 0.44f, 0.01f, CWH),
              BX(T_CLOTH, 0, 0.44f, 0, 0.42f, 0.1f, 0.42f, ((Color){30, 40, 70, 255})), BX(T_PLAIN, 0, 0.42f, 0.24f, 0.4f, 0.04f, 0.12f, ((Color){20, 20, 25, 255})));
    }
}

// ---- items (in piles, on display) — one model per item type
static void drawItemType(int type, Matrix F) {
    switch (type) {
    case IT_FOOD: ADRAW("items/onigiri", F, WHITE, BX(T_PLAIN, 0, 0.06f, 0, 0.16f, 0.12f, 0.08f, ((Color){235, 235, 225, 255})), BX(T_PLAIN, 0, 0.03f, 0, 0.17f, 0.06f, 0.09f, ((Color){20, 40, 25, 255}))); break;
    case IT_DRINK: ADRAW("items/green_tea", F, WHITE, BX(T_PLAIN, 0, 0.12f, 0, 0.08f, 0.24f, 0.08f, ((Color){90, 170, 90, 255})), BX(T_PLAIN, 0, 0.26f, 0, 0.04f, 0.04f, 0.04f, ((Color){230, 230, 230, 255}))); break;
    case IT_WATER: ADRAW("items/water_bottle", F, WHITE, BX(T_GLASS, 0, 0.15f, 0, 0.09f, 0.3f, 0.09f, ((Color){170, 210, 240, 255})), BX(T_PLAIN, 0, 0.31f, 0, 0.04f, 0.03f, 0.04f, ((Color){40, 90, 200, 255}))); break;
    case IT_CANNED: ADRAW("items/canned_mackerel", F, WHITE, BX(T_METAL, 0, 0.04f, 0, 0.14f, 0.08f, 0.1f, ((Color){200, 200, 205, 255})), BX(T_PLAIN, 0, 0.04f, 0, 0.145f, 0.05f, 0.105f, ((Color){60, 110, 170, 255}))); break;
    case IT_BANDAGE: ADRAW("items/bandage", F, WHITE, BX(T_PLAIN, 0, 0.05f, 0, 0.16f, 0.1f, 0.12f, ((Color){240, 240, 240, 255})), BX(T_PLAIN, 0, 0.05f, 0, 0.05f, 0.11f, 0.13f, ((Color){200, 30, 30, 255}))); break;
    case IT_RAG: ADRAW("items/rags", F, WHITE, BX(T_CLOTH, 0, 0.03f, 0, 0.2f, 0.06f, 0.16f, ((Color){180, 170, 150, 255})), BX(T_CLOTH, 0.04f, 0.07f, 0.02f, 0.12f, 0.04f, 0.1f, ((Color){150, 140, 120, 255}))); break;
    case IT_PAINKILLER: ADRAW("items/painkillers", F, WHITE, BX(T_PLAIN, 0, 0.05f, 0, 0.06f, 0.1f, 0.06f, ((Color){230, 140, 40, 255})), BX(T_PLAIN, 0, 0.11f, 0, 0.065f, 0.02f, 0.065f, CWH)); break;
    case IT_BATTERY: ADRAW("items/battery", F, WHITE, BX(T_PLAIN, 0, 0.07f, 0, 0.05f, 0.14f, 0.05f, ((Color){230, 200, 50, 255}))); break;
    case IT_AMMO: ADRAW("items/revolver_rounds", F, WHITE, BX(T_PLAIN, 0, 0.05f, 0, 0.14f, 0.1f, 0.1f, ((Color){160, 130, 60, 255}))); break;
    case IT_PLANKS: ADRAW("items/wood_planks", F, WHITE, BX(T_WOOD, 0, 0.04f, -0.05f, 0.9f, 0.06f, 0.16f, CWH), BX(T_WOOD, 0, 0.11f, 0, 0.9f, 0.06f, 0.16f, CWH), BX(T_WOOD, 0, 0.18f, 0.05f, 0.9f, 0.06f, 0.16f, CWH)); break;
    case IT_NAILS: ADRAW("items/nails", F, WHITE, BX(T_PLAIN, 0, 0.04f, 0, 0.12f, 0.08f, 0.08f, ((Color){180, 120, 60, 255}))); break;
    case IT_SCRAP: ADRAW("items/scrap_metal", F, WHITE, BX(T_RUST, 0, 0.03f, 0, 0.4f, 0.04f, 0.3f, CWH), BX(T_METAL, 0.05f, 0.07f, 0.02f, 0.3f, 0.03f, 0.1f, ((Color){150, 150, 150, 255}))); break;
    case IT_OFUDA: ADRAW("items/ofuda", F, WHITE, BX(T_PAPER, 0, 0.01f, 0, 0.08f, 0.012f, 0.24f, CWH), BX(T_PAPER, 0.05f, 0.02f, 0.02f, 0.08f, 0.012f, 0.24f, CWH)); break;
    case IT_SALT: ADRAW("items/salt", F, WHITE, BX(T_PLAIN, 0, 0.08f, 0, 0.12f, 0.16f, 0.08f, ((Color){240, 240, 245, 255})), BX(T_PLAIN, 0, 0.09f, 0, 0.125f, 0.06f, 0.085f, ((Color){40, 90, 190, 255}))); break;
    case IT_ROPE: ADRAW("items/shimenawa", F, WHITE, BX(T_CLOTH, 0, 0.04f, 0, 0.42f, 0.08f, 0.42f, ((Color){225, 205, 145, 255})), BX(T_PAPER, 0.1f, 0.09f, 0, 0.08f, 0.02f, 0.2f, CWH)); break;
    case IT_MANGA: ADRAW("items/manga", F, WHITE, BX(T_PLAIN, 0, 0.02f, 0, 0.13f, 0.03f, 0.18f, ((Color){235, 235, 230, 255})), BX(T_PLAIN, 0, 0.037f, 0, 0.135f, 0.006f, 0.185f, ((Color){210, 60, 70, 255}))); break;
    case IT_VHS: ADRAW("items/anime_vhs", F, WHITE, BX(T_PLAIN, 0, 0.013f, 0, 0.19f, 0.026f, 0.105f, ((Color){25, 25, 28, 255})), BX(T_PLAIN, 0, 0.027f, 0.01f, 0.12f, 0.004f, 0.05f, ((Color){120, 200, 240, 255}))); break;
    case IT_RAMEN: ADRAW("items/instant_ramen", F, WHITE, BX(T_PLAIN, 0, 0.055f, 0, 0.12f, 0.11f, 0.12f, ((Color){240, 240, 235, 255})), BX(T_PLAIN, 0, 0.113f, 0, 0.125f, 0.008f, 0.125f, ((Color){200, 40, 40, 255}))); break;
    case IT_AXE: ADRAW("items/fire_axe", F, WHITE, BX(T_WOOD, 0, 0.04f, 0, 0.06f, 0.06f, 0.8f, CWH), BX(T_PLAIN, 0, 0.04f, 0.38f, 0.24f, 0.05f, 0.14f, ((Color){200, 30, 30, 255}))); break;
    case IT_KATANA: ADRAW("items/katana", F, WHITE, BX(T_PLAIN, 0, 0.03f, 0.1f, 0.04f, 0.03f, 0.9f, ((Color){210, 215, 225, 255})), BX(T_PLAIN, 0, 0.04f, -0.42f, 0.05f, 0.05f, 0.26f, ((Color){30, 20, 20, 255}))); break;
    case IT_REVOLVER: ADRAW("items/revolver", F, WHITE, BX(T_PLAIN, 0, 0.05f, 0.05f, 0.05f, 0.06f, 0.24f, ((Color){60, 60, 65, 255})), BX(T_WOOD, 0, 0.03f, -0.1f, 0.05f, 0.12f, 0.07f, CWH)); break;
    case IT_PIPE: ADRAW("items/steel_pipe", F, WHITE, BX(T_METAL, 0, 0.03f, 0, 0.05f, 0.05f, 0.75f, ((Color){140, 145, 150, 255}))); break;
    case IT_BAT: ADRAW("items/baseball_bat", F, WHITE, BX(T_WOOD, 0, 0.04f, 0.1f, 0.08f, 0.08f, 0.5f, ((Color){210, 180, 130, 255})), BX(T_WOOD, 0, 0.03f, -0.25f, 0.05f, 0.05f, 0.3f, ((Color){190, 160, 110, 255}))); break;
    case IT_NAILBAT: ADRAW("items/nail_bat", F, WHITE, BX(T_WOOD, 0, 0.04f, 0.1f, 0.08f, 0.08f, 0.5f, ((Color){170, 135, 100, 255})), BX(T_WOOD, 0, 0.03f, -0.25f, 0.05f, 0.05f, 0.3f, ((Color){150, 120, 90, 255})), BX(T_METAL, 0, 0.04f, 0.15f, 0.16f, 0.02f, 0.02f, ((Color){170, 170, 170, 255}))); break;
    case IT_CROWBAR: ADRAW("items/crowbar", F, WHITE, BX(T_METAL, 0, 0.025f, 0, 0.04f, 0.04f, 0.7f, ((Color){160, 30, 30, 255})), BX(T_METAL, 0, 0.025f, 0.38f, 0.04f, 0.04f, 0.1f, ((Color){160, 30, 30, 255}))); break;
    case IT_KNIFE: ADRAW("items/kitchen_knife", F, WHITE, BX(T_PLAIN, 0, 0.02f, 0.08f, 0.04f, 0.01f, 0.2f, ((Color){215, 220, 225, 255})), BX(T_PLAIN, 0, 0.02f, -0.08f, 0.03f, 0.03f, 0.12f, ((Color){30, 25, 25, 255}))); break;
    case IT_SLEDGE: ADRAW("items/sledgehammer", F, WHITE, BX(T_WOOD, 0, 0.04f, 0, 0.06f, 0.06f, 0.9f, CWH), BX(T_METAL, 0, 0.07f, 0.42f, 0.24f, 0.14f, 0.14f, ((Color){90, 90, 95, 255}))); break;
    case IT_HAMMER: ADRAW("items/claw_hammer", F, WHITE, BX(T_WOOD, 0, 0.03f, 0, 0.04f, 0.04f, 0.32f, CWH), BX(T_METAL, 0, 0.04f, 0.15f, 0.14f, 0.05f, 0.05f, ((Color){100, 100, 105, 255}))); break;
    case IT_SCHOOLBAG: ADRAW("items/randoseru", F, WHITE, BX(T_CLOTH, 0, 0.18f, 0, 0.3f, 0.36f, 0.2f, ((Color){150, 25, 30, 255}))); break;
    case IT_DUFFEL: ADRAW("items/duffel_bag", F, WHITE, BX(T_CLOTH, 0, 0.12f, 0, 0.6f, 0.24f, 0.26f, ((Color){40, 50, 70, 255}))); break;
    case IT_HIKEPACK: ADRAW("items/hiking_pack", F, WHITE, BX(T_CLOTH, 0, 0.3f, 0, 0.38f, 0.6f, 0.26f, ((Color){60, 90, 60, 255})), BX(T_CLOTH, 0, 0.62f, 0, 0.3f, 0.12f, 0.22f, ((Color){180, 120, 40, 255}))); break;
    case IT_BOTTLE: ADRAW("items/empty_bottle", F, WHITE, BX(T_GLASS, 0, 0.12f, 0, 0.08f, 0.24f, 0.08f, ((Color){200, 225, 235, 255})), BX(T_PLAIN, 0, 0.26f, 0, 0.04f, 0.04f, 0.04f, ((Color){60, 120, 200, 255}))); break;
    case IT_DIRTYWATER: ADRAW("items/unboiled_water", F, WHITE, BX(T_GLASS, 0, 0.12f, 0, 0.08f, 0.24f, 0.08f, ((Color){140, 150, 110, 255})), BX(T_PLAIN, 0, 0.26f, 0, 0.04f, 0.04f, 0.04f, ((Color){60, 120, 200, 255}))); break;
    case IT_SPLINT: ADRAW("items/splint", F, WHITE, BX(T_WOOD, -0.05f, 0.02f, 0, 0.04f, 0.03f, 0.42f, ((Color){190, 160, 120, 255})), BX(T_WOOD, 0.05f, 0.02f, 0, 0.04f, 0.03f, 0.42f, ((Color){190, 160, 120, 255})),
                          BX(T_CLOTH, 0, 0.03f, -0.1f, 0.16f, 0.04f, 0.05f, ((Color){230, 225, 215, 255})), BX(T_CLOTH, 0, 0.03f, 0.1f, 0.16f, 0.04f, 0.05f, ((Color){230, 225, 215, 255}))); break;
    case IT_CHARCOAL: ADRAW("items/charcoal", F, WHITE, BX(T_PLAIN, 0, 0.03f, 0, 0.12f, 0.06f, 0.08f, ((Color){30, 30, 32, 255})), BX(T_PLAIN, 0.04f, 0.07f, 0.01f, 0.06f, 0.04f, 0.05f, ((Color){45, 44, 46, 255}))); break;
    case IT_DISINFECT: ADRAW("items/disinfectant", F, WHITE, BX(T_PLAIN, 0, 0.09f, 0, 0.09f, 0.18f, 0.06f, ((Color){230, 235, 240, 255})), BX(T_PLAIN, 0, 0.1f, 0.031f, 0.07f, 0.07f, 0.005f, ((Color){200, 40, 40, 255})),
                             BX(T_PLAIN, 0, 0.2f, 0, 0.03f, 0.04f, 0.03f, ((Color){60, 120, 200, 255}))); break;
    case IT_SEEDS: ADRAW("items/seeds", F, WHITE, BX(T_PAPER, 0, 0.01f, 0, 0.1f, 0.02f, 0.14f, ((Color){200, 180, 120, 255})), BX(T_PLAIN, 0, 0.021f, 0.02f, 0.06f, 0.002f, 0.06f, ((Color){90, 160, 70, 255}))); break;
    case IT_DAIKON: ADRAW("items/daikon", F, WHITE, BX(T_PLAIN, 0, 0.05f, 0, 0.07f, 0.07f, 0.3f, ((Color){240, 236, 225, 255})), BX(T_PLAIN, 0, 0.06f, -0.2f, 0.09f, 0.09f, 0.12f, ((Color){90, 160, 70, 255}))); break;
    default: break;
    }
}
static void drawItem(Item *it, float t) {   // kept for asset prewarm
    Vector3 p = it->pos; float ry = t * 1.2f + p.z;
    drawItemType(it->type, MatrixMultiply(MatrixRotateY(ry), MatrixTranslate(p.x, p.y, p.z)));
}

// ---- held weapon (first person)
static void drawHeld(Matrix H, int t) {
    switch (t) {
    case IT_PIPE: ADRAW("viewmodels/steel_pipe", H, WHITE, BX(T_METAL, 0, 0.26f, 0, 0.04f, 0.62f, 0.04f, ((Color){140, 145, 150, 255}))); break;
    case IT_AXE: ADRAW("viewmodels/fire_axe", H, WHITE, BX(T_WOOD, 0, 0.24f, 0, 0.045f, 0.58f, 0.045f, CWH), BX(T_PLAIN, 0, 0.48f, -0.07f, 0.03f, 0.13f, 0.17f, ((Color){200, 30, 30, 255}))); break;
    case IT_KATANA: ADRAW("viewmodels/katana", H, WHITE, BX(T_PLAIN, 0, 0.05f, 0, 0.05f, 0.25f, 0.05f, ((Color){30, 20, 20, 255})), BX(T_PLAIN, 0, 0.2f, 0, 0.1f, 0.02f, 0.06f, ((Color){160, 140, 60, 255})), BX(T_PLAIN, 0, 0.62f, 0, 0.02f, 0.8f, 0.045f, ((Color){215, 220, 230, 255}))); break;
    case IT_BAT: ADRAW("viewmodels/baseball_bat", H, WHITE, BX(T_WOOD, 0, 0.12f, 0, 0.045f, 0.3f, 0.045f, ((Color){190, 160, 110, 255})), BX(T_WOOD, 0, 0.45f, 0, 0.075f, 0.4f, 0.075f, ((Color){210, 180, 130, 255}))); break;
    case IT_NAILBAT: ADRAW("viewmodels/nail_bat", H, WHITE, BX(T_WOOD, 0, 0.12f, 0, 0.045f, 0.3f, 0.045f, ((Color){150, 120, 90, 255})), BX(T_WOOD, 0, 0.45f, 0, 0.075f, 0.4f, 0.075f, ((Color){170, 135, 100, 255})),
                          BX(T_METAL, 0, 0.5f, 0, 0.14f, 0.015f, 0.015f, ((Color){170, 170, 170, 255})), BX(T_METAL, 0, 0.4f, 0, 0.015f, 0.015f, 0.14f, ((Color){170, 170, 170, 255})), BX(T_METAL, 0, 0.58f, 0, 0.015f, 0.015f, 0.14f, ((Color){170, 170, 170, 255}))); break;
    case IT_CROWBAR: ADRAW("viewmodels/crowbar", H, WHITE, BX(T_METAL, 0, 0.3f, 0, 0.035f, 0.65f, 0.035f, ((Color){160, 30, 30, 255})), BX(T_METAL, 0, 0.62f, -0.05f, 0.035f, 0.04f, 0.1f, ((Color){160, 30, 30, 255}))); break;
    case IT_KNIFE: ADRAW("viewmodels/kitchen_knife", H, WHITE, BX(T_PLAIN, 0, 0.05f, 0, 0.035f, 0.12f, 0.035f, ((Color){30, 25, 25, 255})), BX(T_PLAIN, 0, 0.2f, 0, 0.01f, 0.2f, 0.04f, ((Color){215, 220, 225, 255}))); break;
    case IT_SLEDGE: ADRAW("viewmodels/sledgehammer", H, WHITE, BX(T_WOOD, 0, 0.32f, 0, 0.05f, 0.75f, 0.05f, CWH), BX(T_METAL, 0, 0.7f, 0, 0.14f, 0.14f, 0.26f, ((Color){90, 90, 95, 255}))); break;
    case IT_HAMMER: ADRAW("viewmodels/claw_hammer", H, WHITE, BX(T_WOOD, 0, 0.14f, 0, 0.035f, 0.3f, 0.035f, CWH), BX(T_METAL, 0, 0.3f, -0.02f, 0.04f, 0.05f, 0.14f, ((Color){100, 100, 105, 255}))); break;
    default: break;
    }
}
static void drawViewmodel(Vector3 eye, Vector3 aim) {
    Vector3 f = aim, r = Vector3Normalize(Vector3CrossProduct(f, V3(0, 1, 0))), u = Vector3CrossProduct(r, f);
    Matrix B = {r.x, u.x, -f.x, eye.x, r.y, u.y, -f.y, eye.y, r.z, u.z, -f.z, eye.z, 0, 0, 0, 1};
    float bx = sinf(pl.bob) * 0.012f, by = fabsf(cosf(pl.bob)) * 0.012f;
    rlDrawRenderBatchActive(); rlDisableDepthTest();
    setEmis(0);
    Color skin = {210, 180, 160, 255};
    int ht = pl.hands.t;
    if (pl.build) {
        Matrix H = frameM(B, V3(0.22f + bx, -0.25f + by, -0.4f), -0.3f, 0, 0);
        ADRAW("viewmodels/build_plank", H, WHITE, BX(T_WOOD, 0, 0, -0.05f, 0.5f, 0.05f, 0.12f, CWH), BX(T_FLESH, 0.05f, -0.03f, 0, 0.08f, 0.08f, 0.1f, ((Color){210, 180, 160, 255})));
    } else if (ht == IT_REVOLVER) {
        float rl = pl.reloadT > 0 ? sinf((1 - pl.reloadT / REV_RELOAD) * PI) : 0;
        Matrix G = MatrixMultiply(MatrixScale(0.7f, 0.7f, 0.7f), frameM(B, V3(0.17f + bx, -0.16f + by - rl * 0.15f, -0.42f + pl.kick * 0.05f), pl.kick * 0.3f + rl * 0.6f, 0, rl * 0.5f));
        ADRAW("viewmodels/revolver", G, WHITE, BX(T_WOOD, 0, -0.07f, 0.04f, 0.045f, 0.12f, 0.06f, CWH), BX(T_PLAIN, 0, 0, -0.08f, 0.05f, 0.055f, 0.24f, ((Color){55, 55, 60, 255})),
              BX(T_PLAIN, 0, -0.005f, -0.02f, 0.07f, 0.07f, 0.07f, ((Color){70, 70, 75, 255})), BX(T_FLESH, 0, -0.12f, 0.05f, 0.08f, 0.08f, 0.1f, ((Color){210, 180, 160, 255})));
        if (pl.muzzle > 0) { setEmis(1); ADRAW("viewmodels/muzzle_flash", G, WHITE, BX(T_PLAIN, 0, 0, -0.24f, 0.09f, 0.09f, 0.06f, ((Color){255, 220, 120, 255}))); setEmis(0); }
    } else {
        // swing: wind-up (raise back), strike (fast arc across), recovery; heavier weapons travel slower and further
        float dur = pl.swingDur > 0 ? pl.swingDur : 0.6f;
        float s = pl.swingT > 0 ? 1 - pl.swingT / dur : 0;
        float wu = clampf(s / 0.3f, 0, 1), st = clampf((s - 0.3f) / 0.15f, 0, 1), rc = clampf((s - 0.45f) / 0.55f, 0, 1);
        float heavy = ht >= 0 ? clampf(idef[ht].weight / 5.0f, 0.1f, 1) : 0.05f;
        float rx = -0.5f + wu * (0.9f + heavy * 0.6f) - st * (2.4f + heavy * 0.4f) + rc * (1.6f + heavy * 0.4f) * (st >= 1 ? 1 : 0);
        float rz = -0.55f - wu * 0.4f + st * 1.6f - rc * 1.2f * (st >= 1 ? 1 : 0);
        float px = 0.34f + bx - st * 0.32f + rc * 0.32f * (st >= 1 ? 1 : 0), py = -0.32f + by + wu * 0.14f - st * 0.12f;
        float shakeHit = pl.hitStopT > 0 ? fr(-0.035f, 0.035f) : 0;
        if (ht < 0) {   // fists: two hands, jab with the right
            Matrix L = frameM(B, V3(-0.24f + bx, -0.28f + by, -0.42f), -0.2f, 0, 0.2f);
            ADRAW("viewmodels/fist_left", L, skin, BX(T_FLESH, 0, 0, 0, 0.1f, 0.1f, 0.12f, CWH), BX(T_FLESH, 0, -0.12f, 0.06f, 0.08f, 0.2f, 0.08f, CWH));
            Matrix R = frameM(B, V3(0.24f + bx - st * 0.12f + shakeHit, -0.28f + by + st * 0.06f, -0.42f - st * 0.28f + rc * 0.28f * (st >= 1 ? 1 : 0)), -0.2f, 0, -0.2f);
            ADRAW("viewmodels/fist_right", R, skin, BX(T_FLESH, 0, 0, 0, 0.1f, 0.1f, 0.12f, CWH), BX(T_FLESH, 0, -0.12f, 0.06f, 0.08f, 0.2f, 0.08f, CWH));
        } else {
            Matrix H = frameM(B, V3(px + shakeHit, py, -0.5f), rx, 0, rz);
            ADRAW("viewmodels/hand", H, WHITE, BX(T_FLESH, 0, 0, 0, 0.09f, 0.09f, 0.11f, ((Color){210, 180, 160, 255})));
            drawHeld(H, ht);
        }
    }
    rlDrawRenderBatchActive(); rlEnableDepthTest();
}

// ---- furniture (searchable), piles, junk
static void drawCont(Cont *c, float t) {
    Matrix M = TRS(c->pos, c->yaw, V3(1, 1, 1));
    Color dim = c->searched ? (Color){175, 170, 165, 255} : WHITE;
    switch (c->kind) {
    case CK_PILE: {
        int shown = 0;
        for (int i = 0; i < CSLOTS && shown < 5; i++) if (c->s[i].t >= 0) {
            float ox = (shown % 3 - 1) * 0.16f, oz = (shown / 3) * 0.16f - 0.08f;
            setEmis(0.0f);
            drawItemType(c->s[i].t, MatrixMultiply(MatrixRotateY(shown * 1.3f + c->pos.x), MatrixTranslate(c->pos.x + ox, c->pos.y + 0.01f, c->pos.z + oz)));
            shown++;
        }
        break; }
    case CK_CABINET: ADRAW("furniture/kitchen_cabinet", M, dim, BX(T_WOOD, 0, 0.45f, 0, 1.0f, 0.9f, 0.5f, ((Color){200, 190, 170, 255})), BX(T_PLAIN, 0, 0.92f, 0, 1.04f, 0.04f, 0.54f, ((Color){180, 185, 190, 255})),
                           BX(T_WOOD, -0.245f, 0.5f, 0.27f, 0.44f, 0.7f, 0.04f, ((Color){214, 202, 180, 255})), BX(T_WOOD, 0.245f, 0.5f, 0.27f, 0.44f, 0.7f, 0.04f, ((Color){214, 202, 180, 255})),
                           BX(T_METAL, -0.06f, 0.62f, 0.3f, 0.03f, 0.14f, 0.03f, ((Color){90, 92, 96, 255})), BX(T_METAL, 0.06f, 0.62f, 0.3f, 0.03f, 0.14f, 0.03f, ((Color){90, 92, 96, 255}))); break;   // doors stand 2 cm proud of the body, with handles
    case CK_FRIDGE: ADRAW("furniture/refrigerator", M, dim, BX(T_PLAIN, 0, 0.85f, 0, 0.7f, 1.7f, 0.65f, ((Color){230, 235, 235, 255})), BX(T_PLAIN, 0, 1.2f, 0.326f, 0.66f, 0.01f, 0.01f, ((Color){120, 130, 130, 255})),
                          BX(T_PLAIN, 0.28f, 0.9f, 0.33f, 0.03f, 0.4f, 0.02f, ((Color){150, 160, 160, 255}))); break;
    case CK_WARDROBE: ADRAW("furniture/wardrobe", M, dim, BX(T_WOOD, 0, 0.95f, 0, 1.2f, 1.9f, 0.55f, ((Color){150, 110, 80, 255})), BX(T_PLAIN, 0, 0.95f, 0.276f, 0.01f, 1.8f, 0.01f, ((Color){60, 40, 30, 255}))); break;
    case CK_DESK: ADRAW("furniture/desk", M, dim, BX(T_WOOD, 0, 0.72f, 0, 1.2f, 0.05f, 0.6f, ((Color){170, 150, 120, 255})), BX(T_WOOD, -0.4f, 0.35f, 0, 0.36f, 0.7f, 0.56f, ((Color){150, 130, 100, 255})),
                        BX(T_METAL, 0.55f, 0.35f, 0, 0.04f, 0.7f, 0.04f, ((Color){90, 90, 90, 255})), BX(T_PLAIN, 0.2f, 0.8f, -0.1f, 0.3f, 0.1f, 0.22f, ((Color){60, 60, 65, 255}))); break;
    case CK_SHELF: ADRAW("furniture/store_shelf", M, dim, BX(T_METAL, 0, 0.8f, 0, 1.6f, 1.6f, 0.08f, ((Color){200, 200, 205, 255})), BX(T_METAL, 0, 0.3f, 0.12f, 1.6f, 0.04f, 0.38f, ((Color){200, 200, 205, 255})),
                         BX(T_METAL, 0, 0.8f, 0.12f, 1.6f, 0.04f, 0.38f, ((Color){200, 200, 205, 255})), BX(T_METAL, 0, 1.3f, 0.12f, 1.6f, 0.04f, 0.38f, ((Color){200, 200, 205, 255})),
                         BX(T_PLAIN, -0.4f, 0.4f, 0.12f, 0.5f, 0.16f, 0.3f, ((Color){220, 120, 60, 255})), BX(T_PLAIN, 0.3f, 0.9f, 0.12f, 0.6f, 0.14f, 0.3f, ((Color){70, 160, 200, 255})), BX(T_PLAIN, -0.2f, 1.4f, 0.12f, 0.7f, 0.16f, 0.3f, ((Color){230, 220, 120, 255}))); break;
    case CK_LOCKER: ADRAW("furniture/locker", M, dim, BX(T_METAL, 0, 0.9f, 0, 0.5f, 1.8f, 0.5f, ((Color){110, 130, 140, 255})), BX(T_PLAIN, 0, 1.55f, 0.251f, 0.3f, 0.04f, 0.01f, ((Color){30, 30, 30, 255})),
                          BX(T_PLAIN, 0, 1.45f, 0.251f, 0.3f, 0.04f, 0.01f, ((Color){30, 30, 30, 255})), BX(T_PLAIN, 0.17f, 0.95f, 0.26f, 0.03f, 0.12f, 0.02f, ((Color){60, 60, 60, 255}))); break;
    case CK_BOX: ADRAW("furniture/cardboard_boxes", M, dim, BX(T_PLAIN, 0, 0.18f, 0, 0.7f, 0.36f, 0.5f, ((Color){170, 130, 80, 255})), BX(T_PLAIN, 0.05f, 0.44f, 0.02f, 0.45f, 0.16f, 0.36f, ((Color){185, 145, 90, 255}))); break;
    case CK_TOOLBOX: ADRAW("furniture/toolbox", M, dim, BX(T_METAL, 0, 0.12f, 0, 0.5f, 0.24f, 0.3f, ((Color){180, 30, 30, 255})), BX(T_METAL, 0, 0.27f, 0, 0.3f, 0.04f, 0.04f, ((Color){40, 40, 40, 255}))); break;
    case CK_TRUNK: break;   // part of the car model
    case CK_OFFERING: ADRAW("furniture/offering_box", M, dim, BX(T_WOOD, 0, 0.27f, 0, 0.6f, 0.55f, 0.4f, ((Color){120, 80, 50, 255})), BX(T_PLAIN, 0, 0.56f, 0, 0.62f, 0.03f, 0.42f, ((Color){60, 40, 30, 255}))); break;
    case CK_COUNTER: ADRAW("furniture/counter", M, dim, BX(T_PLAIN, 0, 0.5f, 0, 1.4f, 1.0f, 0.6f, ((Color){215, 220, 225, 255})), BX(T_PLAIN, 0, 1.01f, 0, 1.44f, 0.04f, 0.64f, ((Color){90, 150, 170, 255})),
                           BX(T_PLAIN, 0.4f, 1.15f, -0.1f, 0.3f, 0.24f, 0.24f, ((Color){50, 50, 55, 255}))); break;
    case CK_CHEST: ADRAW("furniture/storage_chest", M, dim, BX(T_WOOD, 0, 0.28f, 0, 1.0f, 0.56f, 0.6f, ((Color){180, 150, 110, 255})), BX(T_METAL, 0, 0.3f, 0.301f, 0.1f, 0.12f, 0.01f, ((Color){70, 70, 70, 255}))); break;
    case CK_GUNLOCKER: ADRAW("furniture/police_locker", M, dim, BX(T_METAL, 0, 0.9f, 0, 0.6f, 1.8f, 0.5f, ((Color){60, 70, 90, 255})), BX(T_PLAIN, 0, 1.4f, 0.251f, 0.4f, 0.1f, 0.01f, ((Color){230, 230, 230, 255}))); break;
    case CK_FIRECAB: setEmis(0); ADRAW("furniture/fire_cabinet", M, dim, BX(T_METAL, 0, 0.4f, 0, 0.6f, 0.8f, 0.25f, ((Color){200, 30, 30, 255})), BX(T_GLASS, 0, 0.45f, 0.126f, 0.5f, 0.6f, 0.01f, ((Color){180, 200, 210, 255}))); break;
    default: break;
    }
    (void)t;
}
static void drawDebris(Debris *d) {   // chairs, a table and a mattress jammed together
    Vector3 c = V3((d->mn.x + d->mx.x) / 2, d->mn.y, (d->mn.z + d->mx.z) / 2);
    float yaw = (d->seed % 7) * 0.4f, sx = d->mx.x - d->mn.x, sz = d->mx.z - d->mn.z;
    Matrix M = MatrixMultiply(MatrixMultiply(MatrixScale(sx / 1.2f, (d->mx.y - d->mn.y) / 1.1f, sz / 1.2f), MatrixRotateY(yaw * 0.15f)), MatrixTranslate(c.x, c.y, c.z));
    ADRAW("props/junk_pile", M, WHITE,
          BX(T_WOOD, 0, 0.36f, 0, 1.1f, 0.06f, 0.7f, ((Color){150, 120, 90, 255})), BX(T_WOOD, -0.45f, 0.18f, -0.25f, 0.06f, 0.36f, 0.06f, ((Color){120, 95, 70, 255})), BX(T_WOOD, 0.45f, 0.18f, 0.25f, 0.06f, 0.36f, 0.06f, ((Color){120, 95, 70, 255})),
          BX(T_CLOTH, 0.1f, 0.62f, 0.1f, 0.9f, 0.14f, 0.6f, ((Color){150, 140, 120, 255})), BX(T_METAL, -0.3f, 0.82f, -0.2f, 0.45f, 0.06f, 0.45f, ((Color){90, 95, 100, 255})),
          BX(T_METAL, -0.3f, 0.95f, 0.02f, 0.45f, 0.4f, 0.05f, ((Color){90, 95, 100, 255})), BX(T_WOOD, 0.35f, 0.9f, 0.2f, 0.4f, 0.04f, 0.4f, ((Color){170, 140, 100, 255})),
          BX(T_WOOD, 0.35f, 1.0f, -0.0f, 0.04f, 0.3f, 0.4f, ((Color){170, 140, 100, 255})), BX(T_PLAIN, 0.4f, 0.12f, -0.3f, 0.35f, 0.24f, 0.25f, ((Color){60, 60, 60, 255})));
}
static void drawBuildPiece(Build *b) {
    float dmg = b->maxhp > 0 ? b->hp / b->maxhp : 1;
    Color c = rgb(200 * (0.5f + 0.5f * dmg) + 30, 180 * (0.5f + 0.5f * dmg), 150 * (0.5f + 0.5f * dmg));
    bool ax = (b->mx.x - b->mn.x) < (b->mx.z - b->mn.z);
    Vector3 bc = V3((b->mn.x + b->mx.x) / 2, b->mn.y, (b->mn.z + b->mx.z) / 2);
    switch (b->kind) {
    case BK_WALL: ADRAW("base/plank_wall", TRS(bc, ax ? PI / 2 : 0, V3(1, 1, 1)), c, BX(T_WOOD, 0, 1.2f, 0, 2.0f, 2.4f, 0.2f, CWH), BX(T_WOOD, -0.8f, 1.2f, 0.13f, 0.12f, 2.4f, 0.06f, ((Color){170, 150, 120, 255})),
                        BX(T_WOOD, 0.8f, 1.2f, 0.13f, 0.12f, 2.4f, 0.06f, ((Color){170, 150, 120, 255})), BX(T_WOOD, 0, 1.6f, 0.13f, 1.6f, 0.12f, 0.06f, ((Color){170, 150, 120, 255}))); break;
    case BK_DOOR: {
        ADRAW("base/door_frame", TRS(bc, ax ? PI / 2 : 0, V3(1, 1, 1)), c, BX(T_WOOD, -0.85f, 1.2f, 0, 0.3f, 2.4f, 0.2f, CWH), BX(T_WOOD, 0.85f, 1.2f, 0, 0.3f, 2.4f, 0.2f, CWH), BX(T_WOOD, 0, 2.25f, 0, 1.4f, 0.3f, 0.2f, CWH));
        float sw = b->open ? PI / 2 * 0.92f : 0;
        Matrix D = MatrixMultiply(MatrixMultiply(MatrixTranslate(0.7f, 0, 0), MatrixRotateY(sw)), MatrixMultiply(MatrixTranslate(-0.7f, 0, 0), TRS(bc, ax ? PI / 2 : 0, V3(1, 1, 1))));
        ADRAW("base/door_leaf", D, c, BX(T_WOOD, 0, 1.05f, 0, 1.38f, 2.1f, 0.08f, ((Color){190, 160, 120, 255})), BX(T_METAL, 0.5f, 1.05f, 0.06f, 0.06f, 0.1f, 0.06f, ((Color){60, 60, 60, 255})));
        break; }
    case BK_CRATE: ADRAW("props/crate", TRS(bc, 0, Vector3Subtract(b->mx, b->mn)), c, BX(T_WOOD, 0, 0.5f, 0, 1, 1, 1, ((Color){200, 170, 130, 255}))); break;
    case BK_FLOOR: ADRAW("base/floor_platform", TRS(V3(bc.x, b->mn.y, bc.z), 0, V3(1, 1, 1)), c, BX(T_WOOD, 0, 0.075f, 0, 2.0f, 0.15f, 2.0f, CWH), BX(T_WOOD, -0.9f, -0.5f, -0.9f, 0.12f, 1.0f, 0.12f, ((Color){150, 120, 90, 255})),
                         BX(T_WOOD, 0.9f, -0.5f, 0.9f, 0.12f, 1.0f, 0.12f, ((Color){150, 120, 90, 255}))); break;
    case BK_CHEST: break;   // drawn as its container
    case BK_BED: ADRAW("base/bed", TRS(bc, ax ? 0 : PI / 2, V3(1, 1, 1)), c, BX(T_WOOD, 0, 0.15f, 0, 1.0f, 0.3f, 2.0f, CWH), BX(T_CLOTH, 0, 0.34f, 0.1f, 0.9f, 0.1f, 1.7f, ((Color){90, 110, 150, 255})),
                       BX(T_CLOTH, 0, 0.38f, -0.78f, 0.6f, 0.1f, 0.3f, ((Color){230, 225, 215, 255}))); break;
    case BK_SHRINE: setEmis(0); ADRAW("base/kamidana", TRS(bc, ax ? PI / 2 : 0, V3(1, 1, 1)), WHITE, BX(T_WOOD, 0, 0.5f, 0, 0.12f, 1.0f, 0.12f, ((Color){150, 110, 80, 255})), BX(T_WOOD, 0, 1.02f, 0, 0.9f, 0.06f, 0.5f, ((Color){200, 170, 120, 255})),
                          BX(T_WOOD, 0, 1.2f, 0, 0.5f, 0.32f, 0.36f, ((Color){215, 190, 140, 255})), BX(T_WOOD, 0, 1.4f, 0, 0.7f, 0.08f, 0.5f, ((Color){90, 70, 50, 255})),
                          BX(T_CLOTH, 0, 1.1f, 0.26f, 0.88f, 0.05f, 0.05f, ((Color){225, 205, 145, 255})), BX(T_PAPER, -0.2f, 0.98f, 0.27f, 0.08f, 0.16f, 0.02f, CWH), BX(T_PAPER, 0.2f, 0.98f, 0.27f, 0.08f, 0.16f, 0.02f, CWH)); break;
    case BK_WARD: ADRAW("base/spirit_ward", TRS(bc, ax ? PI / 2 : 0, V3(1, 1, 1)), WHITE, BX(T_WOOD, -0.9f, 0.8f, 0, 0.12f, 1.6f, 0.12f, CWH), BX(T_WOOD, 0.9f, 0.8f, 0, 0.12f, 1.6f, 0.12f, CWH),
                        BX(T_CLOTH, 0, 1.44f, 0, 1.88f, 0.12f, 0.12f, ((Color){225, 205, 145, 255})), BX(T_PAPER, -0.45f, 1.21f, 0, 0.12f, 0.33f, 0.04f, CWH), BX(T_PAPER, 0, 1.21f, 0, 0.12f, 0.33f, 0.04f, CWH), BX(T_PAPER, 0.45f, 1.21f, 0, 0.12f, 0.33f, 0.04f, CWH)); break;
    case BK_GARDEN: {
        float g = b->open ? clampf(b->hp / b->maxhp, 0, 1) : 0;
        ADRAW("base/garden_bed", TRS(bc, ax ? PI / 2 : 0, V3(1, 1, 1)), c, BX(T_WOOD, 0, 0.12f, 0, 1.8f, 0.24f, 1.0f, ((Color){130, 100, 70, 255})), BX(T_CONCRETE, 0, 0.22f, 0, 1.66f, 0.06f, 0.86f, ((Color){70, 50, 35, 255})));
        if (g > 0.02f) for (int k = 0; k < 6; k++) { float ox = -0.6f + (k % 3) * 0.6f, oz = k < 3 ? -0.2f : 0.2f;
            Vector3 p = ax ? V3(bc.x + oz, b->mn.y + 0.25f, bc.z + ox) : V3(bc.x + ox, b->mn.y + 0.25f, bc.z + oz);
            ADRAW("base/daikon_leaves", TRS(p, k * 1.1f, V3(0.4f + g * 0.6f, 0.3f + g * 0.9f, 0.4f + g * 0.6f)), WHITE, BX(T_PLAIN, 0, 0.18f, 0, 0.3f, 0.36f, 0.08f, ((Color){90, 160, 70, 255})), BX(T_PLAIN, 0, 0.18f, 0, 0.08f, 0.36f, 0.3f, ((Color){70, 140, 60, 255})));
            if (g >= 1) ADRAW("base/daikon_root", TRS(p, 0, V3(1, 1, 1)), WHITE, BX(T_PLAIN, 0, 0.02f, 0, 0.1f, 0.12f, 0.1f, ((Color){235, 232, 220, 255}))); }
        break; }
    case BK_SPIKES: ADRAW("base/spike_trap", TRS(bc, 0, V3(1, 1, 1)), c, BX(T_WOOD, 0, 0.03f, 0, 1.1f, 0.06f, 1.1f, ((Color){120, 95, 70, 255})),
                          BX(T_METAL, -0.35f, 0.15f, -0.35f, 0.04f, 0.24f, 0.04f, ((Color){150, 150, 150, 255})), BX(T_METAL, 0.35f, 0.15f, -0.35f, 0.04f, 0.24f, 0.04f, ((Color){150, 150, 150, 255})),
                          BX(T_METAL, 0, 0.15f, 0, 0.04f, 0.24f, 0.04f, ((Color){150, 150, 150, 255})), BX(T_METAL, -0.35f, 0.15f, 0.35f, 0.04f, 0.24f, 0.04f, ((Color){150, 150, 150, 255})),
                          BX(T_METAL, 0.35f, 0.15f, 0.35f, 0.04f, 0.24f, 0.04f, ((Color){150, 150, 150, 255})), BX(T_METAL, 0, 0.15f, 0.35f, 0.04f, 0.24f, 0.04f, ((Color){150, 150, 150, 255})),
                          BX(T_METAL, 0, 0.15f, -0.35f, 0.04f, 0.24f, 0.04f, ((Color){150, 150, 150, 255})), BX(T_METAL, -0.35f, 0.15f, 0, 0.04f, 0.24f, 0.04f, ((Color){150, 150, 150, 255})),
                          BX(T_METAL, 0.35f, 0.15f, 0, 0.04f, 0.24f, 0.04f, ((Color){150, 150, 150, 255}))); break;
    case BK_ALARM: ADRAW("base/can_alarm", TRS(bc, 0, V3(1, 1, 1)), WHITE, BX(T_WOOD, 0, 0.25f, 0, 0.05f, 0.5f, 0.05f, ((Color){120, 95, 70, 255})), BX(T_PLAIN, 0, 0.46f, 0, 0.9f, 0.01f, 0.01f, ((Color){200, 200, 190, 255})),
                         BX(T_METAL, -0.3f, 0.38f, 0, 0.07f, 0.11f, 0.07f, ((Color){180, 60, 50, 255})), BX(T_METAL, 0.3f, 0.38f, 0, 0.07f, 0.11f, 0.07f, ((Color){200, 190, 170, 255})),
                         BX(T_METAL, 0.1f, 0.36f, 0, 0.07f, 0.11f, 0.07f, ((Color){70, 120, 170, 255}))); break;
    default: break;
    }
}
static void drawWater(Vector3 eye, float t) {
    for (int i = 0; i < nWs; i++) { WSrc *w = &wsrc[i]; if (Vector3DistanceSqr(w->pos, eye) > 60 * 60) continue;
        if (w->kind == 0) ADRAW("props/rain_barrel", TRS(w->pos, i * 0.7f, V3(1, 1, 1)), WHITE, BX(T_PLAIN, 0, 0.45f, 0, 0.6f, 0.9f, 0.6f, ((Color){50, 100, 160, 255})), BX(T_PLAIN, 0, 0.2f, 0, 0.64f, 0.06f, 0.64f, ((Color){40, 80, 130, 255})),
                                BX(T_PLAIN, 0, 0.7f, 0, 0.64f, 0.06f, 0.64f, ((Color){40, 80, 130, 255})), BX(T_PLAIN, 0, 0.905f, 0, 0.5f, 0.01f, 0.5f, ((Color){30, 50, 60, 255})));
        else if (w->kind == 1) { ADRAW("props/fountain", TRS(w->pos, 0, V3(1, 1, 1)), WHITE, BX(T_CONCRETE, 0, 0.25f, 0, 1.5f, 0.5f, 1.5f, ((Color){190, 195, 200, 255})), BX(T_CONCRETE, 0, 0.75f, 0, 0.25f, 0.6f, 0.25f, ((Color){200, 205, 210, 255})),
                                 BX(T_PLAIN, 0, 0.49f, 0, 1.3f, 0.02f, 1.3f, ((Color){90, 150, 170, 255})));
            setEmis(1); DrawCube(V3(w->pos.x, w->pos.y + 1.1f + sinf(t * 6) * 0.03f, w->pos.z), 0.08f, 0.2f, 0.08f, (Color){170, 220, 240, 255}); setEmis(0); }
        else ADRAW("props/spring", TRS(w->pos, i, V3(1, 1, 1)), WHITE, BX(T_CONCRETE, 0, 0.1f, 0, 1.4f, 0.2f, 1.1f, ((Color){150, 150, 140, 255})), BX(T_PLAIN, 0, 0.18f, 0, 1.1f, 0.06f, 0.8f, ((Color){120, 210, 240, 255})));
    }
}
