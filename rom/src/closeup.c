/* Close-up: one friend big on the cushion. A squishes it, B goes back. */
#include "game.h"
#include "game_assets.h"
#include "sound.h"
#include "squishy.h"
#include "system.h"
#include "text.h"

#define T_SQ      0       /* 4 frames x 64 tiles */
#define T_SHADOW  256
#define T_SMALL   260
#define P_SQ      0
#define P_SHADOW  1
#define P_SMALL   2
#define P_TEXT    15

#define CX        120
#define FLOOR     121
#define BOTTOM    29      /* texture rows from centre to the friend's bottom */
#define HEART_OFF (-1000)

int sel_species, sel_flavor;

EWRAM_BSS static TextStrip st_name;
static int squish_t, bounce_t, blink_t, idle_t;
static struct { int t, x, y; } hearts[3];

static void enter(void) {
    dma3_copy32(CHARBLOCK(0), closeup_tiles, sizeof closeup_tiles);
    dma3_copy32(SCREENBLOCK(30), closeup_map0, sizeof closeup_map0);
    dma3_copy16(PAL_BG, closeup_pal, sizeof closeup_pal);
    dma3_copy16(PAL_BG + P_TEXT * 16, ui_text_pal, sizeof ui_text_pal);
    for (int i = 0; i < 16; i++) CHARBLOCK(2)[i] = 0;
    for (int i = 0; i < 32 * 32; i++) SCREENBLOCK(31)[i] = 0;
    REG_BGCNT(1) = BG_PRIO(2) | BG_CBB(0) | BG_SBB(30);
    REG_BGCNT(0) = BG_PRIO(0) | BG_CBB(2) | BG_SBB(31);
    strip_init(&st_name, 2, 1, 31, 7, 17, 16, 3, P_TEXT);
    char buf[40];
    sq_full_name(buf, sel_species, sel_flavor);
    strip_print(&st_name, buf, 1, 9, 1, 0);
    dbg("scene closeup %s", buf);

    sq_load_frames(sel_species, 64, 0, 4, T_SQ);
    sq_load_palette(sel_species, sel_flavor, P_SQ);
    dma3_copy32(OBJ_TILES + T_SHADOW * 16, ui_shadow_tiles, sizeof ui_shadow_tiles);
    dma3_copy16(PAL_OBJ + P_SHADOW * 16, ui_shadow_pal, sizeof ui_shadow_pal);
    dma3_copy32(OBJ_TILES + T_SMALL * 16, ui_small_tiles, sizeof ui_small_tiles);
    dma3_copy16(PAL_OBJ + P_SMALL * 16, ui_small_pal, sizeof ui_small_pal);

    squish_t = -1;
    bounce_t = 0;           /* arrive with a little bounce */
    blink_t = 90;
    idle_t = 0;
    for (int i = 0; i < 3; i++) hearts[i].t = HEART_OFF;
    scene_blend(BLD_BG1 << 8, 6 | (10 << 8));
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
}

static void spawn_hearts(void) {
    static const s8 dx[3] = {-30, 26, -6};
    for (int i = 0; i < 3; i++) {
        hearts[i].t = -i * 5;          /* staggered start */
        hearts[i].x = CX + dx[i];
        hearts[i].y = 72 - i * 6;
    }
}

static void draw(void) {
    int sx = 256, sy = 256, lift = 0, frame = SQ64_IDLE;
    int b = isin((int)frame_count);
    sy += b / 40;
    sx -= b / 56;

    if (--blink_t < 0) blink_t = 150 + (int)(frame_count % 60);
    if (blink_t < 6) frame = SQ64_BLINK;

    if (bounce_t >= 0) {
        lift = (14 * isin(bounce_t * 2)) / 256;
        frame = SQ64_OPEN;
        if (++bounce_t > 16) bounce_t = -1;
    }
    if (squish_t >= 0) {
        if (squish_t < 7) {
            frame = SQ64_SQUISH;
        } else {
            int k = squish_t - 7;
            int amp = 44 - k * 3;
            if (amp < 0) amp = 0;
            int w = isin(k * 9);
            sy = 256 + amp * w / 256;
            sx = 256 - amp * w / 512;
            frame = SQ64_OPEN;
        }
        if (++squish_t > 24) squish_t = -1;
    }
    /* every few seconds of calm, a small happy hop */
    if (squish_t < 0 && bounce_t < 0 && ++idle_t > 240) {
        idle_t = 0;
        bounce_t = 0;
    }

    int bottom = FLOOR - lift;
    int cy = bottom - (BOTTOM * sy) / 256;
    oam[0].attr0 = A0_Y(cy - 64) | A0_AFFINE | A0_DOUBLE | A0_SQUARE;
    oam[0].attr1 = A1_X(CX - 64) | A1_AFF(0) | A1_SIZE(3);
    oam[0].attr2 = A2_TILE(T_SQ + frame * 64) | A2_PRIO(1) | A2_PAL(P_SQ);
    affine_scale(0, sx, sy);

    for (int i = 0; i < 2; i++) {
        oam[1 + i].attr0 = A0_Y(FLOOR - 4) | A0_WIDE | A0_BLEND;
        oam[1 + i].attr1 = A1_X(CX - 32 + i * 32) | A1_SIZE(1) | (i ? A1_HFLIP : 0);
        oam[1 + i].attr2 = A2_TILE(T_SHADOW) | A2_PRIO(2) | A2_PAL(P_SHADOW);
    }

    for (int i = 0; i < 3; i++) {
        ObjAttr *o = &oam[3 + i];
        o->attr0 = A0_HIDE;
        if (hearts[i].t == HEART_OFF) continue;
        int t = hearts[i].t++;
        if (t < 0) continue;
        if (t > 44) {
            hearts[i].t = HEART_OFF;
            continue;
        }
        int x = hearts[i].x + (isin(t * 3 + i * 20) * 4) / 256;
        int y = hearts[i].y - t;
        o->attr0 = A0_Y(y) | A0_SQUARE;
        o->attr1 = A1_X(x) | A1_SIZE(0);
        o->attr2 = A2_TILE(T_SMALL + UI_HEART) | A2_PRIO(0) | A2_PAL(P_SMALL);
    }

    static const s8 tw[3][2] = {{-34, -30}, {30, -18}, {-22, 6}};
    for (int i = 0; i < 3; i++) {
        ObjAttr *o = &oam[6 + i];
        int phase = ((int)frame_count + i * 23) % 60;
        int f = phase < 10 ? 0 : phase < 20 ? 1 : phase < 30 ? 2 : phase < 40 ? 1 : phase < 48 ? 0 : -1;
        if (sel_flavor != FLAVOR_SPARKLE || f < 0) {
            o->attr0 = A0_HIDE;
            continue;
        }
        o->attr0 = A0_Y(cy + tw[i][1] - 4) | A0_SQUARE;
        o->attr1 = A1_X(CX + tw[i][0] - 4) | A1_SIZE(0);
        o->attr2 = A2_TILE(T_SMALL + UI_TWINKLE0 + f) | A2_PRIO(0) | A2_PAL(P_SMALL);
    }
}

static void update(void) {
    u16 hit = key_hit();
    if (hit & KEY_A) {
        squish_t = 0;
        idle_t = 0;
        sfx_squeak(sel_flavor);
        spawn_hearts();
        dbg("closeup squish");
    }
    if (hit & (KEY_B | KEY_START)) {
        sfx_blip();
        scene_go(&scene_shelf);
    }
    draw();
}

const Scene scene_closeup = {enter, update};
