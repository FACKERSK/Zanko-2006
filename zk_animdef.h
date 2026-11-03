// zk_animdef.h — first-person animation state (the drawing lives in zk_anim.h)
enum { ANIM_NONE, ANIM_EQUIP, ANIM_HOLSTER, ANIM_INSPECT, ANIM_EAT, ANIM_DRINK, ANIM_PILL, ANIM_BANDAGE, ANIM_TAKE, ANIM_DOOR, ANIM_RELOAD, ANIM_BUILD, ANIM_FLOURISH, ANIM_N };
static const float animDurs[ANIM_N] = {0, 0.45f, 0.32f, 2.8f, 1.3f, 1.5f, 0.9f, 1.8f, 0.45f, 0.5f, 1.8f, 0.35f, 1.0f};
static int animId = ANIM_NONE, animItem = -1; static float animT = 0, animDur = 0;
static void animPlay(int a) {
    if (a <= ANIM_NONE || a >= ANIM_N) return;
    animId = a; animT = 0; animDur = animDurs[a];
    if (a == ANIM_EQUIP && pl.hands.t == IT_KATANA) animDur = 0.75f;   // iaido draw
    if (a == ANIM_INSPECT) animDur = pl.hands.t < 0 ? 2.0f : pl.hands.t == IT_REVOLVER ? 2.6f : 2.8f;
}
static void animUse(int a, int item) { animPlay(a); animItem = item; }
static bool animBusy(void) { return animId != ANIM_NONE && animId != ANIM_TAKE && animId != ANIM_DOOR && animT < animDur; }
