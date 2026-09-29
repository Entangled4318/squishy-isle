/* Open screen: the container from the map (gift box, acorn, seashell or
 * capsule, by area) sits big on a cushion. Each press of A or B makes it
 * jump higher with a rising note; the third press pops the lid off (lid,
 * acorn cap, top shell, capsule dome). If the child does not press, the box starts pressing
 * itself after a few seconds, so nobody is ever stuck. */
#include "collection.h"
#include "game.h"
#include "game_assets.h"
#include "music_data.h"
#include "sound.h"
#include "system.h"

#define T_BOX     0        /* body, open body, lid: 64 tiles each */
#define T_SHADOW  192
#define T_SMALL   196      /* twinkles, heart */
#define T_ABIG    200      /* 2 frames of 32x32 */
#define T_UI      232
#define P_BOX     0
#define P_SHADOW  1
#define P_SMALL   2
#define P_ABIG    3
#define P_UI      4

#define CX        120
#define FLOOR     121      /* cushion top where the box stands */
#define BOX_BASE  61       /* box bottom row inside its 64x64 art */
#define PRESSES   3
#define AUTO_WAIT 240      /* 4 s without a press, then it opens by itself */
#define AUTO_STEP 70       /* then one self-press every ~1.2 s */
#define COOLDOWN  14       /* frames between counted presses */
#define POP_TIME  70       /* frames after the pop before moving on */

int open_color, open_friend;

static int presses, idle_t, cool, jump_t, jump_h, land_t, pop_t, press_anim;
static int lid_x, lid_y, lid_vx, lid_vy, lid_spin;   /* 8.8 fixed while flying */

static void enter(void) {
    music_play(SONG_OPEN);           /* tip-toe loop in C pentatonic: the press chimes fit over it */
    dma3_copy32(CHARBLOCK(0), openbg_tiles, sizeof openbg_tiles);
    dma3_copy32(SCREENBLOCK(30), openbg_map0, sizeof openbg_map0);
    dma3_copy16(PAL_BG, openbg_pal, sizeof openbg_pal);
    REG_BGCNT(1) = BG_PRIO(2) | BG_CBB(0) | BG_SBB(30);

    int c = (game_save.area < AREA_COUNT ? game_save.area : 0) * 5 + open_color;   /* the area's container */
    dma3_copy32(OBJ_TILES + T_BOX * 16, cont64_tiles[c], 3 * 64 * 32);
    dma3_copy16(PAL_OBJ + P_BOX * 16, cont64_pal[c], 32);
    dma3_copy32(OBJ_TILES + T_SHADOW * 16, ui_shadow_tiles, sizeof ui_shadow_tiles);
    dma3_copy16(PAL_OBJ + P_SHADOW * 16, ui_shadow_pal, sizeof ui_shadow_pal);
    dma3_copy32(OBJ_TILES + T_SMALL * 16, ui_small_tiles, sizeof ui_small_tiles);
    dma3_copy16(PAL_OBJ + P_SMALL * 16, ui_small_pal, sizeof ui_small_pal);
    dma3_copy32(OBJ_TILES + T_ABIG * 16, abig_tiles, sizeof abig_tiles);
    dma3_copy16(PAL_OBJ + P_ABIG * 16, abig_pal, sizeof abig_pal);
    dma3_copy32(OBJ_TILES + T_UI * 16, openui_tiles, sizeof openui_tiles);
    dma3_copy16(PAL_OBJ + P_UI * 16, openui_pal, sizeof openui_pal);

    presses = idle_t = cool = land_t = press_anim = 0;
    jump_t = -1;
    pop_t = -1;
    dbg("scene open color %d friend %d area %d", open_color, open_friend, game_save.area);
    scene_blend(BLD_BG1 << 8, 6 | (10 << 8));
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
}

static void press(bool by_child) {
    presses++;
    cool = COOLDOWN;
    idle_t = 0;
    press_anim = 8;
    dbg("open press %d%s", presses, by_child ? "" : " (auto)");
    if (presses < PRESSES) {
        jump_t = 0;
        jump_h = presses == 1 ? 10 : 18;
        sfx_chime(presses == 1 ? 0 : 2);
        sfx_boing();
        return;
    }
    /* pop: the lid flies up and off, sparkles burst out */
    pop_t = 0;
    jump_t = -1;
    lid_x = CX << 8;
    lid_y = (FLOOR - BOX_BASE + 32) << 8;
    lid_vx = (frame_count & 1) ? 300 : -300;
    lid_vy = -6 * 256;
    lid_spin = 0;
    sfx_pop();
    song_play(tune_pop, tune_pop_len);
    if (open_friend >= 0) collection_add(open_friend);
    dbg("open pop friend %d found %d", open_friend, found_total());
    music_stop();                    /* the ta-da jingle already holds the wave channel */
}

static int n_oam;

static ObjAttr *spr(void) { return n_oam < 128 ? &oam[n_oam++] : &oam[127]; }

static void draw_box(void) {
    int lift = 0, sx = 256, sy = 256, tilt;
    if (jump_t >= 0) {
        lift = jump_h * isin(jump_t * 32 / 22) / 256;       /* half sine over 22 frames */
        sx = 256 - lift;                                  /* stretch up while flying */
        sy = 256 + lift * 2;
    } else if (land_t > 0) {
        sx = 256 + land_t * 6;                            /* squash on landing */
        sy = 256 - land_t * 6;
    }
    /* wobble grows with each press and with waiting */
    int amp = pop_t >= 0 ? 0 : 5 + presses * 5 + (idle_t > 120 ? 3 : 0);
    tilt = amp * isin((int)frame_count * (3 + presses)) / 256;   /* 1/256 turn */
    if (pop_t >= 0 && pop_t < 12) {                        /* body settles after the pop */
        sx = 256 + (12 - pop_t) * 5;
        sy = 256 - (12 - pop_t) * 5;
    }
    affine_rot_scale(0, tilt, sx, sy);

    /* keep the bottom on the cushion: sprite centre sits above the floor */
    int base = FLOOR - lift;
    int cy = base - ((BOX_BASE - 32) * sy) / 256;
    int shift = (tilt * (BOX_BASE - 32) * 6) / 256;       /* rotate about the bottom, roughly */
    int cx = CX + shift;
    bool opened = pop_t >= 0;

    ObjAttr *o;
    if (!opened) {                                         /* lid rides along, in front (an acorn cap overlaps the nut) */
        o = spr();
        o->attr0 = A0_Y(cy - 64) | A0_AFFINE | A0_DOUBLE | A0_SQUARE;
        o->attr1 = A1_X(cx - 64) | A1_AFF(0) | A1_SIZE(3);
        o->attr2 = A2_TILE(T_BOX + 128) | A2_PRIO(1) | A2_PAL(P_BOX);
    }
    o = spr();                                             /* body (or open body) */
    o->attr0 = A0_Y(cy - 64) | A0_AFFINE | A0_DOUBLE | A0_SQUARE;
    o->attr1 = A1_X(cx - 64) | A1_AFF(0) | A1_SIZE(3);
    o->attr2 = A2_TILE(T_BOX + (opened ? 64 : 0)) | A2_PRIO(1) | A2_PAL(P_BOX);

    /* shadow shrinks as the box rises */
    int sw = 64 - lift;
    for (int i = 0; i < 2; i++) {
        o = spr();
        o->attr0 = A0_Y(FLOOR - 4) | A0_WIDE | A0_BLEND;
        o->attr1 = A1_X(CX - 32 + i * (sw / 2) + (64 - sw) / 2 * (1 - i)) | A1_SIZE(1) | (i ? A1_HFLIP : 0);
        o->attr2 = A2_TILE(T_SHADOW) | A2_PRIO(2) | A2_PAL(P_SHADOW);
    }

    /* motion arcs beside the box while it wobbles hard */
    if (!opened && (tilt > 6 || tilt < -6)) {
        int side = tilt > 0 ? 1 : -1;
        for (int k = 0; k < 2; k++) {
            o = spr();
            o->attr0 = A0_Y(cy - 20 + k * 12) | A0_SQUARE;
            o->attr1 = A1_X(CX + side * (40 + k * 4) - 4) | A1_SIZE(0) | (side > 0 ? A1_HFLIP : 0);
            o->attr2 = A2_TILE(T_UI + OPENUI_ARC) | A2_PRIO(1) | A2_PAL(P_UI);
        }
    }
}

static void draw_lid_and_burst(void) {
    if (pop_t < 0) return;
    ObjAttr *o;
    if (lid_y < (SCREEN_H + 40) << 8 && lid_y > -(64 << 8)) {
        affine_rot_scale(1, lid_spin >> 8, 256, 256);
        o = spr();
        o->attr0 = A0_Y((lid_y >> 8) - 64) | A0_AFFINE | A0_DOUBLE | A0_SQUARE;
        o->attr1 = A1_X((lid_x >> 8) - 64) | A1_AFF(1) | A1_SIZE(3);
        o->attr2 = A2_TILE(T_BOX + 128) | A2_PRIO(0) | A2_PAL(P_BOX);
    }
    /* sparkles fly out of the box in a ring, then twinkle away */
    int r = pop_t < 30 ? pop_t * 2 + 10 : 70;
    for (int k = 0; k < 10; k++) {
        if (pop_t > 40 + k * 2) continue;
        int a = k * 64 / 10 + 3;
        int x = CX + isin(a + 16) * r / 256, y = FLOOR - 30 + isin(a) * r * 3 / (4 * 256);
        int f = pop_t < 14 ? 2 : pop_t < 30 ? 1 : 0;
        o = spr();
        o->attr0 = A0_Y(y - 4) | A0_SQUARE;
        o->attr1 = A1_X(x - 4) | A1_SIZE(0);
        o->attr2 = A2_TILE(T_SMALL + f) | A2_PRIO(0) | A2_PAL(P_SMALL);
    }
    /* soft puffs on the cushion */
    if (pop_t < 30) {
        for (int k = 0; k < 2; k++) {
            o = spr();
            o->attr0 = A0_Y(FLOOR - 6 - pop_t / 6) | A0_WIDE;
            o->attr1 = A1_X(CX - 8 + (k ? 1 : -1) * (30 + pop_t / 2)) | A1_SIZE(0) | (k ? A1_HFLIP : 0);
            o->attr2 = A2_TILE(T_UI + OPENUI_PUFF) | A2_PRIO(1) | A2_PAL(P_UI);
        }
    }
}

static void draw_hud(void) {
    if (pop_t >= 0) return;
    /* big A button, pressed for a moment after each press; hops gently while waiting */
    int bob = press_anim ? 0 : (isin((int)frame_count * 2) > 180 ? -2 : 0);
    ObjAttr *o = spr();
    o->attr0 = A0_Y(128 + bob) | A0_SQUARE;
    o->attr1 = A1_X(CX - 16 - 22) | A1_SIZE(2);
    o->attr2 = A2_TILE(T_ABIG + (press_anim ? 16 : 0)) | A2_PRIO(0) | A2_PAL(P_ABIG);
    /* three hearts fill up, one per press */
    for (int i = 0; i < PRESSES; i++) {
        o = spr();
        o->attr0 = A0_Y(137) | A0_SQUARE;
        o->attr1 = A1_X(CX - 4 + i * 16) | A1_SIZE(1);
        o->attr2 = A2_TILE(T_UI + (i < presses ? OPENUI_HEART_ON : OPENUI_HEART_OFF)) | A2_PRIO(0) | A2_PAL(P_UI);
    }
}

static void update(void) {
    u16 hit = key_hit();
    if (cool > 0) cool--;
    if (press_anim > 0) press_anim--;
    if (pop_t < 0) {
        idle_t++;
        if ((hit & (KEY_A | KEY_B)) && cool == 0) {
            press(true);
        } else if (idle_t >= AUTO_WAIT && cool == 0 && (idle_t - AUTO_WAIT) % AUTO_STEP == 0) {
            press(false);
            idle_t = AUTO_WAIT + 1;     /* keep pressing by itself at the same pace */
        }
    }
    if (jump_t >= 0 && ++jump_t > 22) {
        jump_t = -1;
        land_t = 6;
    } else if (land_t > 0) {
        land_t--;
    }
    if (pop_t >= 0) {
        pop_t++;
        lid_x += lid_vx;
        lid_y += lid_vy;
        lid_vy += 40;
        lid_spin += lid_vx > 0 ? 900 : -900;
        if (pop_t >= POP_TIME) {          /* >=: goes on asking until the fade starts */
            if (open_friend >= 0) {
                sel_species = friend_species(open_friend);
                sel_flavor = friend_flavor(open_friend);
                scene_go(&scene_reveal);
            } else {
                scene_go(&scene_meadow_view);
            }
        }
    }
    n_oam = 0;
    draw_hud();
    draw_lid_and_burst();
    draw_box();
    for (int i = n_oam; i < 128; i++) oam[i].attr0 = A0_HIDE;
}

const Scene scene_open = {enter, update};
