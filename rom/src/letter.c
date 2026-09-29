/* Letter scene (from the meadow mailbox): a pink envelope drops in and
 * opens, the letter slides up out of it, and the newest friend waves from
 * its round frame: "Dear Pip, <a happy message> Love, <name>". A squishes
 * the friend (3 squishes go back, like the close-up), B or START go back.
 * Any press while the envelope opens skips to the letter. */
#include "collection.h"
#include "game.h"
#include "game_assets.h"
#include "sound.h"
#include "squishy.h"
#include "system.h"
#include "text.h"

#define T_SQ      0       /* 4 frames x 64 tiles */
#define T_ENV     256     /* closed, open: 64 tiles each */
#define T_SMALL   384     /* twinkles, heart */
#define P_SQ      0
#define P_ENV     1
#define P_SMALL   2
#define P_TEXT    15

#define DROP      24      /* frames: the envelope falls in and bounces */
#define FLAP      40      /* the flap opens */
#define RISE      76      /* the letter has slid up; the friend pops in */
#define SQUISHES  3
#define CARD_LOW  96      /* px below its place where the card starts to rise */
#define PX        60      /* the portrait's centre on the card */
#define PY        80

static const char *const lines1[12] = {
    "Thank you for", "I love my", "You give the", "Let's play", "I made a new", "Hugs and",
    "I had a yummy", "You are my", "I found a", "Today was a", "I did a big", "Come and visit",
};
static const char *const lines2[12] = {
    "finding me!", "new home!", "best squishes!", "again soon!", "friend today!", "squishes for you!",
    "snack today!", "best friend!", "shiny pebble!", "happy day!", "happy hop!", "me soon!",
};

EWRAM_BSS static TextStrip st_line;   /* one buffer for all 5 lines: each line's tiles stay in VRAM (boot clears less RAM) */
static int t, writer, squish_t, squishes, calm_t, bounce_t;
static struct { int t, x, y; } hearts[3];

static void enter(void) {
    writer = game_save.mail ? game_save.mail - 1 : 0;
    int sp = friend_species(writer), fl = friend_flavor(writer);
    bool was_new = game_save.mail_new;
    if (game_save.mail_new) {                  /* read: the flag goes down */
        game_save.mail_new = 0;
        collection_save();
    }

    dma3_copy32(CHARBLOCK(0), letter_tiles, sizeof letter_tiles);
    for (int i = 0; i < 32 * 32; i++) SCREENBLOCK(29)[i] = SCREENBLOCK(30)[i] = SCREENBLOCK(31)[i] = 0;
    dma3_copy32(SCREENBLOCK(29), letter_map0, sizeof letter_map0);   /* backdrop, BG2 */
    dma3_copy32(SCREENBLOCK(30), letter_map1, sizeof letter_map1);   /* the card, BG1 */
    dma3_copy16(PAL_BG, letter_pal, sizeof letter_pal);
    dma3_copy16(PAL_BG + P_TEXT * 16, ui_text_pal, sizeof ui_text_pal);
    for (int i = 0; i < 16; i++) CHARBLOCK(2)[i] = 0;
    REG_BGCNT(2) = BG_PRIO(3) | BG_CBB(0) | BG_SBB(29);
    REG_BGCNT(1) = BG_PRIO(2) | BG_CBB(0) | BG_SBB(30);
    REG_BGCNT(0) = BG_PRIO(1) | BG_CBB(2) | BG_SBB(31);

    /* text lines sit on the card's ruled lines (letter_parts in export_game.py) */
    static const u8 top[5] = {28, 50, 64, 90, 104};
    char name[40];
    sq_full_name(name, sp, fl);
    int m = (writer * 5) % 12;
    const char *text[5] = {"Dear Pip,", lines1[m], lines2[m], "Love,", name};
    for (int i = 0; i < 5; i++) {
        strip_init(&st_line, 2, 1 + i * 26, 31, 14, top[i] / 8, 13, 2, P_TEXT);
        strip_print(&st_line, text[i], 0, top[i] % 8, 1, 0);
    }

    sq_load_frames(sp, 64, 0, 4, T_SQ);
    sq_load_palette(sp, fl, P_SQ);
    dma3_copy32(OBJ_TILES + T_ENV * 16, envelope64_tiles, sizeof envelope64_tiles);
    dma3_copy16(PAL_OBJ + P_ENV * 16, envelope64_pal, sizeof envelope64_pal);
    dma3_copy32(OBJ_TILES + T_SMALL * 16, ui_small_tiles, sizeof ui_small_tiles);
    dma3_copy16(PAL_OBJ + P_SMALL * 16, ui_small_pal, sizeof ui_small_pal);

    t = 0;
    squish_t = bounce_t = -1;
    squishes = calm_t = 0;
    for (int i = 0; i < 3; i++) hearts[i].t = -1000;
    dbg("scene letter friend %d new %d msg %d", writer, was_new, m);
    scene_blend(0, 0);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;   /* the card shows when the flap opens */
}

static void spawn_hearts(void) {
    static const s8 dx[3] = {-24, 20, -4};
    for (int i = 0; i < 3; i++) {
        hearts[i].t = -i * 5;
        hearts[i].x = PX + dx[i];
        hearts[i].y = PY - 30 - i * 5;
    }
}

static void draw(void) {
    int n = 0;
    /* the card (BG0 text and BG1 paper) comes up out of the envelope. It
     * starts at most 96 px down: the 256 px map wraps, and rows 160..255
     * are the only blank ones above the card */
    int rise = t < RISE ? CARD_LOW - CARD_LOW * (t - FLAP) / (RISE - FLAP) : 0;
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D | (t >= FLAP ? DCNT_BG0 | DCNT_BG1 : 0);
    for (int i = 0; i <= 2; i++) {
        bg_scroll_x[i] = 0;
        bg_scroll_y[i] = (s16)(i < 2 ? -rise : 0);   /* the backdrop (BG2) stays */
    }
    /* envelope: drops in with a bounce, opens, then sinks away as the letter comes out */
    if (t < RISE + 20) {
        int ey;
        if (t < DROP) ey = -64 + (48 + 64) * t / DROP;
        else if (t < FLAP) ey = 48 - (((t - DROP) < 8) ? (t - DROP) * (8 - (t - DROP)) / 3 : 0);
        else ey = 48 + (t - FLAP) * 4;
        int wob = (t >= DROP && t < FLAP) ? (((t / 4) & 1) ? 1 : -1) : 0;
        oam[n].attr0 = A0_Y(ey) | A0_SQUARE;
        oam[n].attr1 = A1_X(88 + wob) | A1_SIZE(3);
        oam[n].attr2 = A2_TILE(T_ENV + (t >= FLAP - 6 ? 64 : 0)) | A2_PRIO(0) | A2_PAL(P_ENV);
        n++;
    }
    if (t >= RISE) {                            /* the friend in its round frame */
        int sx = 256, sy = 256, lift = 0, frame = SQ64_IDLE;
        int b = isin((int)frame_count);
        sy += b / 48;
        if ((frame_count % 170) < 6) frame = SQ64_BLINK;
        if (bounce_t >= 0) {
            lift = (10 * isin(bounce_t * 2)) / 256;
            frame = SQ64_OPEN;
            if (++bounce_t > 16) bounce_t = -1;
        }
        if (squish_t >= 0) {
            if (squish_t < 7) {
                frame = SQ64_SQUISH;
            } else {
                int k = squish_t - 7, amp = 40 - k * 3, w = isin(k * 9);
                if (amp < 0) amp = 0;
                sy = 256 + amp * w / 256;
                sx = 256 - amp * w / 512;
                frame = SQ64_OPEN;
            }
            if (++squish_t > 24) squish_t = -1;
        }
        int bottom = PY + 30 - lift, cy = bottom - (29 * sy) / 256;
        affine_scale(0, sx, sy);
        oam[n].attr0 = A0_Y(cy - 64) | A0_AFFINE | A0_DOUBLE | A0_SQUARE;
        oam[n].attr1 = A1_X(PX - 64) | A1_AFF(0) | A1_SIZE(3);
        oam[n].attr2 = A2_TILE(T_SQ + frame * 64) | A2_PRIO(1) | A2_PAL(P_SQ);
        n++;
        for (int i = 0; i < 3; i++) {
            if (hearts[i].t == -1000) continue;
            int ht = hearts[i].t++;
            if (ht < 0) continue;
            if (ht > 44) {
                hearts[i].t = -1000;
                continue;
            }
            oam[n].attr0 = A0_Y(hearts[i].y - ht) | A0_SQUARE;
            oam[n].attr1 = A1_X(hearts[i].x + isin(ht * 3 + i * 20) * 4 / 256) | A1_SIZE(0);
            oam[n].attr2 = A2_TILE(T_SMALL + UI_HEART) | A2_PRIO(0) | A2_PAL(P_SMALL);
            n++;
        }
    }
    for (int i = n; i < 128; i++) oam[i].attr0 = A0_HIDE;
}

static void update(void) {
    u16 hit = scene_fading() ? 0 : key_hit();
    if (t < RISE) {
        if (hit & (KEY_A | KEY_B | KEY_START)) t = RISE - 1;          /* skip to the letter */
        t++;
        if (t == DROP) sfx_boing();
        if (t == FLAP - 6) sfx_chime(3);
        if (t == RISE) {
            bounce_t = 0;
            sfx_squeak(friend_flavor(writer));
            song_play(tune_hello, tune_hello_len);
            spawn_hearts();
            dbg("letter open");
        }
        draw();
        return;
    }
    t++;
    calm_t++;
    if ((hit & KEY_A) && squishes < SQUISHES) {
        squish_t = 0;
        squishes++;
        calm_t = 0;
        sfx_squeak(friend_flavor(writer));
        spawn_hearts();
        dbg("letter squish %d", squishes);
    }
    bool done = squishes >= SQUISHES && squish_t < 0 && calm_t > 30;
    if (done || (hit & (KEY_B | KEY_START))) {
        sfx_blip();
        dbg("letter close");
        scene_go(&scene_meadow_view);
    }
    draw();
}

const Scene scene_letter = {enter, update};
