/* Close-up: one friend big on the cushion. A squishes it (3 squishes go back, like B), B goes back.
 * Reveal mode (after a container pops): the friend drops onto the cushion
 * with confetti and a star burst; A or B squishes; after 3 squishes or a
 * calm wait it hops up and away, back to the meadow. */
#include "collection.h"
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
#define T_CONF    264
#define T_STAR    276
#define P_SMALL   2
#define P_CONF    3
#define P_STAR    4
#define P_TEXT    15

#define N_CONF    24
#define N_STARS   6
#define DROP_H    150     /* friend starts this far above the cushion */
#define REVEAL_WAIT 300   /* 5 s without a squish, then it hops away */
#define SQUISHES  3

#define CX        120
#define FLOOR     121
#define BOTTOM    29      /* texture rows from centre to the friend's bottom */
#define HEART_OFF (-1000)

int sel_species, sel_flavor;

EWRAM_BSS static TextStrip st_name;
static int squish_t, bounce_t, blink_t, idle_t;
static struct { int t, x, y; } hearts[3];

static bool reveal;
static int drop_y, drop_v;            /* 8.8 px above the cushion while falling */
static int landed_t = -1, squishes, calm_t, leave_t = -1;
static struct { int x, y, vx, vy, tile, flip; } conf[N_CONF];   /* 8.8 fixed; tile < 0 = off */

static void enter_common(void);

static void enter(void) {
    reveal = false;
    leave_t = -1;             /* a reveal before this left it at the end of its hop away */
    landed_t = 0;
    squishes = calm_t = 0;
    enter_common();
}

static void enter_reveal(void) {
    reveal = true;
    drop_y = DROP_H << 8;
    drop_v = 0;
    landed_t = -1;
    leave_t = -1;
    squishes = calm_t = 0;
    for (int i = 0; i < N_CONF; i++) conf[i].tile = -1;
    enter_common();
}

static void enter_common(void) {
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
    dbg("scene %s %s", reveal ? "reveal" : "closeup", buf);

    sq_load_frames(sel_species, 64, 0, 4, T_SQ);
    sq_load_palette(sel_species, sel_flavor, P_SQ);
    dma3_copy32(OBJ_TILES + T_SHADOW * 16, ui_shadow_tiles, sizeof ui_shadow_tiles);
    dma3_copy16(PAL_OBJ + P_SHADOW * 16, ui_shadow_pal, sizeof ui_shadow_pal);
    dma3_copy32(OBJ_TILES + T_SMALL * 16, ui_small_tiles, sizeof ui_small_tiles);
    dma3_copy16(PAL_OBJ + P_SMALL * 16, ui_small_pal, sizeof ui_small_pal);
    dma3_copy32(OBJ_TILES + T_CONF * 16, confetti_tiles, sizeof confetti_tiles);
    dma3_copy16(PAL_OBJ + P_CONF * 16, confetti_pal, sizeof confetti_pal);
    dma3_copy32(OBJ_TILES + T_STAR * 16, star16_tiles, sizeof star16_tiles);
    dma3_copy16(PAL_OBJ + P_STAR * 16, star16_pal, sizeof star16_pal);

    squish_t = -1;
    bounce_t = reveal ? -1 : 0;   /* arrive with a little bounce */
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

/* confetti rains gently from the top, behind the friend */
static void spawn_confetti(void) {
    for (int i = 0; i < N_CONF; i++) {
        u32 r = game_rand();
        conf[i].x = (4 + (int)(r % 224)) << 8;
        conf[i].y = -(8 + (int)((r >> 8) % 90)) << 8;
        conf[i].vx = (int)((r >> 16) % 64) - 32;
        conf[i].vy = 160 + (int)((r >> 20) % 160);
        conf[i].tile = i % CONFETTI_N;
        conf[i].flip = (int)((r >> 24) & 3);
    }
}

static void draw_confetti(int first) {
    for (int i = 0; i < N_CONF; i++) {
        ObjAttr *o = &oam[first + i];
        o->attr0 = A0_HIDE;
        if (conf[i].tile < 0) continue;
        conf[i].x += conf[i].vx;
        conf[i].y += conf[i].vy;
        int x = (conf[i].x >> 8) + isin((int)frame_count * 2 + i * 7) * 3 / 256;
        int y = conf[i].y >> 8;
        if (y > SCREEN_H) {
            conf[i].tile = -1;
            continue;
        }
        if (y < -8) continue;
        o->attr0 = A0_Y(y) | A0_SQUARE;
        o->attr1 = A1_X(x) | A1_SIZE(0) | ((conf[i].flip & 1) ? A1_HFLIP : 0) | ((conf[i].flip & 2) ? A1_VFLIP : 0);
        o->attr2 = A2_TILE(T_CONF + conf[i].tile) | A2_PRIO(1) | A2_PAL(P_CONF);
    }
}

/* big stars fly out in a ring when the friend lands */
static void draw_stars(int first, int cy) {
    for (int i = 0; i < N_STARS; i++) {
        ObjAttr *o = &oam[first + i];
        o->attr0 = A0_HIDE;
        if (landed_t < 0 || landed_t > 36 + i * 2) continue;
        int r = 20 + landed_t * 2;
        int a = i * 64 / N_STARS + 5;
        int x = CX + isin(a + 16) * r / 256, y = cy + isin(a) * r * 2 / (3 * 256);
        o->attr0 = A0_Y(y - 8) | A0_SQUARE;
        o->attr1 = A1_X(x - 8) | A1_SIZE(1);
        o->attr2 = A2_TILE(T_STAR) | A2_PRIO(1) | A2_PAL(P_STAR);
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

    if (reveal && landed_t < 0) {         /* falling in, stretched */
        lift = drop_y >> 8;
        frame = SQ64_OPEN;
        sx = 230;
        sy = 290;
    }
    if (leave_t >= 0) {                   /* hop up and away */
        lift = leave_t * 7;
        frame = SQ64_OPEN;
        sx = 236;
        sy = 280;
    }
    int bottom = FLOOR - lift;
    int cy = bottom - (BOTTOM * sy) / 256;
    oam[0].attr0 = A0_Y(cy - 64) | A0_AFFINE | A0_DOUBLE | A0_SQUARE;
    oam[0].attr1 = A1_X(CX - 64) | A1_AFF(0) | A1_SIZE(3);
    oam[0].attr2 = A2_TILE(T_SQ + frame * 64) | A2_PRIO(1) | A2_PAL(P_SQ);
    affine_scale(0, sx, sy);

    for (int i = 0; i < 2; i++) {
        oam[1 + i].attr0 = A0_Y(FLOOR - 4) | A0_WIDE | A0_BLEND | (lift > 60 ? A0_HIDE : 0);
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

    if (reveal) {
        draw_confetti(10);
        draw_stars(10 + N_CONF, cy);
    }
}

static void squish(void) {
    squish_t = 0;
    idle_t = 0;
    sfx_squeak(sel_flavor);
    spawn_hearts();
}

static void update_reveal(void) {
    u16 hit = key_hit();
    if (landed_t < 0) {
        drop_v += 60;                     /* gravity */
        drop_y -= drop_v;
        if (drop_y <= 0) {
            drop_y = 0;
            landed_t = 0;
            bounce_t = 0;                 /* happy bounce on landing */
            sfx_squeak(sel_flavor);
            song_play(tune_hello, tune_hello_len);
            spawn_confetti();
            dbg("reveal landed");
        }
    } else if (leave_t >= 0) {
        if (++leave_t == 30) scene_go(&scene_meadow_view);
    } else {
        landed_t++;
        calm_t++;
        if ((hit & (KEY_A | KEY_B)) && landed_t > 20 && squishes < SQUISHES) {
            squish();
            squishes++;
            calm_t = 0;
            dbg("reveal squish %d", squishes);
        }
        bool done = squishes >= SQUISHES && squish_t < 0 && calm_t > 40;
        if (done || calm_t > REVEAL_WAIT) {
            leave_t = 0;
            sfx_boing();
            dbg("reveal leave (%s)", done ? "squished" : "waited");
        }
    }
    draw();
}

static void update(void) {
    if (reveal) {
        update_reveal();
        return;
    }
    u16 hit = key_hit();
    calm_t++;
    if ((hit & KEY_A) && squishes < SQUISHES) {
        squish();
        squishes++;
        calm_t = 0;
        dbg("closeup squish %d", squishes);
    }
    bool done = squishes >= SQUISHES && squish_t < 0 && calm_t > 30;   /* 3 squishes: back to the shelf, like B */
    if (done || (hit & (KEY_B | KEY_START))) {
        sfx_blip();
        scene_go(&scene_shelf);
    }
    draw();
}

const Scene scene_closeup = {enter, update};
const Scene scene_reveal = {enter_reveal, update};
