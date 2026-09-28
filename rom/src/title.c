/* Title: the logo letters drop in and bounce (any button skips), friends
 * hop on the island, then a small menu: Play, or Continue / New game when
 * a save has friends. New game asks first: "No" is the default and "Yes"
 * needs A held for 3 s while hearts fill, so a toddler cannot wipe the
 * collection by mashing buttons. */
#include "collection.h"
#include "game.h"
#include "game_assets.h"
#include "music_data.h"
#include "sound.h"
#include "squishy.h"
#include "system.h"
#include "text.h"

#define T_LOGO    0        /* letters, tile offsets from logo_table */
#define T_CAST    360      /* 4 friends x (idle, open) x 16 tiles */
#define T_PIP     488
#define T_SMALL   496
#define T_UI      500      /* hearts on/off */
#define T_ABTN    511
#define P_LOGO    0        /* 0..LOGO_PALS-1 */
#define P_CAST    5        /* 5..8 */
#define P_PIP     9
#define P_SMALL   10
#define P_UI      11
#define P_ABTN    12
#define P_TEXT    15

#define LETTER_GAP  8      /* frames between letters dropping in */
#define DROP_FROM   90     /* px above the letter's place */
#define HOLD_TIME   180    /* 3 s of holding A on "Yes" */
#define HOLD_HEARTS 6

enum { MODE_INTRO, MODE_MENU, MODE_CONFIRM };

static const struct { u8 species, flavor, x, y; } cast[4] = {
    {4, 0, 32, 100}, {0, 1, 68, 94}, {8, 2, 140, 94}, {13, 4, 176, 98}};

static int mode, t, sel, n_opts, hold_t, menu_wait;
static int ly[LOGO_LETTERS], lv[LOGO_LETTERS];     /* 8.8 offset above the letter's place, speed */
static bool landed[LOGO_LETTERS];
static int hop_who, hop_t;

EWRAM_BSS static TextStrip st_opt[2], st_line1, st_line2;

static void print_options(void) {
    static const char *const names2[2] = {"Continue", "New game"};
    static const char *const names_confirm[2] = {"No", "Yes"};
    for (int i = 0; i < 2; i++) {
        const char *txt = "";
        if (mode == MODE_MENU) txt = n_opts == 1 ? (i == 0 ? "Play" : "") : names2[i];
        if (mode == MODE_CONFIRM) txt = (i == 1 && sel == 1) ? "Yes, hold A" : names_confirm[i];
        bool on = i == sel;
        strip_print(&st_opt[i], txt, 1, 3, on ? 2 : 1, on ? 1 : 0);
    }
}

static void clear_confirm_text(void) {
    strip_print(&st_line1, "", 1, 3, 2, 1);
    strip_print(&st_line2, "", 1, 3, 2, 1);
}

static void enter(void) {
    dma3_copy32(CHARBLOCK(0), titlebg_tiles, sizeof titlebg_tiles);
    dma3_copy32(SCREENBLOCK(30), titlebg_map0, sizeof titlebg_map0);
    dma3_copy16(PAL_BG, titlebg_pal, sizeof titlebg_pal);
    dma3_copy16(PAL_BG + P_TEXT * 16, ui_text_pal, sizeof ui_text_pal);
    for (int i = 0; i < 16; i++) CHARBLOCK(2)[i] = 0;
    for (int i = 0; i < 32 * 32; i++) SCREENBLOCK(31)[i] = 0;
    REG_BGCNT(1) = BG_PRIO(2) | BG_CBB(0) | BG_SBB(30);
    REG_BGCNT(0) = BG_PRIO(0) | BG_CBB(2) | BG_SBB(31);
    n_opts = found_total() > 0 ? 2 : 1;
    /* left option centred at x 72 (or x 120 when it is the only one), right at x 168 */
    strip_init(&st_opt[1], 2, 25, 31, 15, 17, 12, 2, P_TEXT);
    strip_init(&st_opt[0], 2, 1, 31, n_opts == 1 ? 9 : 3, 17, 12, 2, P_TEXT);   /* after: "Play" overlaps it */
    strip_init(&st_line1, 2, 49, 31, 7, 3, 16, 2, P_TEXT);
    strip_init(&st_line2, 2, 81, 31, 3, 6, 24, 2, P_TEXT);

    dma3_copy32(OBJ_TILES + T_LOGO * 16, logo_tiles, sizeof logo_tiles);
    dma3_copy16(PAL_OBJ + P_LOGO * 16, logo_pal, sizeof logo_pal);
    for (int i = 0; i < 4; i++) {
        sq_load_frames(cast[i].species, 32, 0, 2, T_CAST + i * 32);
        sq_load_palette(cast[i].species, cast[i].flavor, P_CAST + i);
    }
    dma3_copy32(OBJ_TILES + T_PIP * 16, pip_tiles, 8 * 32);      /* first frame: facing down */
    dma3_copy16(PAL_OBJ + P_PIP * 16, pip_pal, sizeof pip_pal);
    dma3_copy32(OBJ_TILES + T_SMALL * 16, ui_small_tiles, sizeof ui_small_tiles);
    dma3_copy16(PAL_OBJ + P_SMALL * 16, ui_small_pal, sizeof ui_small_pal);
    dma3_copy32(OBJ_TILES + T_UI * 16, openui_tiles, 8 * 32);
    dma3_copy16(PAL_OBJ + P_UI * 16, openui_pal, sizeof openui_pal);
    dma3_copy32(OBJ_TILES + T_ABTN * 16, abubble_tiles, sizeof abubble_tiles);
    dma3_copy16(PAL_OBJ + P_ABTN * 16, abubble_pal, sizeof abubble_pal);

    for (int i = 0; i < LOGO_LETTERS; i++) {
        ly[i] = DROP_FROM << 8;
        lv[i] = 0;
        landed[i] = false;
    }
    mode = MODE_INTRO;
    t = 0;
    hop_t = -1;
    sel = 0;
    print_options();
    clear_confirm_text();
    dbg("scene title options=%d found=%d", n_opts, found_total());
    scene_blend(0, 0);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
}

static void to_menu(void) {
    mode = MODE_MENU;
    menu_wait = 10;
    sel = 0;
    clear_confirm_text();
    print_options();
}

static void finish_intro(void) {
    for (int i = 0; i < LOGO_LETTERS; i++) {
        ly[i] = lv[i] = 0;
        landed[i] = true;
    }
    song_play(tune_hello, tune_hello_len);
    music_play(SONG_TITLE);                        /* starts when the hello jingle ends */
    dbg("title intro done at %d", t);
    to_menu();
}

static void update_letters(void) {
    for (int i = 0; i < LOGO_LETTERS; i++) {
        if (mode == MODE_INTRO && t < i * LETTER_GAP) continue;
        if (landed[i] && ly[i] == 0 && lv[i] == 0) continue;
        lv[i] += 80;                               /* gravity, 8.8 */
        ly[i] -= lv[i];
        if (ly[i] <= 0) {                          /* bounce, smaller each time */
            ly[i] = 0;
            if (!landed[i]) {
                landed[i] = true;
                sfx_chime(i % 5);
            }
            lv[i] = lv[i] > 500 ? -lv[i] * 2 / 5 : 0;
        }
    }
}

static void confirm_enter(void) {
    mode = MODE_CONFIRM;
    sel = 0;                                       /* "No" is the default */
    hold_t = 0;
    strip_print(&st_line1, "Start over?", 1, 3, 2, 1);
    strip_print(&st_line2, "Your friends will go home.", 1, 3, 2, 1);
    print_options();
    dbg("title confirm");
}

static void start_game(bool fresh) {
    if (fresh) {
        collection_new_game(frame_count * 2654435761u ^ game_save.rng ^ ((u32)game_save.boots << 16));
        meadow_reset();
    }
    dbg("title start %s", fresh ? "new" : "continue");
    sfx_chime(4);
    music_stop();
    scene_go(&scene_meadow_view);
}

static void update_input(void) {
    u16 hit = key_hit(), held = key_held();
    if (mode == MODE_INTRO) {
        if (hit) finish_intro();
        return;
    }
    if (menu_wait > 0) {
        menu_wait--;
        return;
    }
    bool move = hit & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN);
    if (mode == MODE_MENU) {
        if (move && n_opts == 2) {
            sel ^= 1;
            sfx_tick();
            print_options();
        }
        if (hit & (KEY_A | KEY_START)) {
            if (sel == 0) start_game(n_opts == 1);
            else confirm_enter();
        } else if (hit & KEY_B) {                  /* B: the letters hop, just for fun */
            for (int i = 0; i < LOGO_LETTERS; i++) lv[i] = -(700 + (i & 1) * 200);
            sfx_boing();
        }
        return;
    }
    /* MODE_CONFIRM */
    if (move) {
        sel ^= 1;
        hold_t = 0;
        sfx_tick();
        print_options();
    }
    if ((hit & KEY_B) || (sel == 0 && (hit & (KEY_A | KEY_START)))) {
        sfx_blip();
        dbg("title confirm no");
        to_menu();
        return;
    }
    if (sel == 1) {
        if ((hit & KEY_A) || (hold_t > 0 && (held & KEY_A))) {
            if (hold_t++ == 0) dbg("title hold start");
            if (hold_t % (HOLD_TIME / HOLD_HEARTS) == 0) sfx_chime(hold_t * HOLD_HEARTS / HOLD_TIME - 1);
            if (hold_t >= HOLD_TIME) start_game(true);
        } else if (hold_t > 0) {
            dbg("title hold released at %d", hold_t);
            hold_t = 0;
        }
    }
}

static int n_oam;
static ObjAttr *spr(void) { return n_oam < 128 ? &oam[n_oam++] : &oam[127]; }

static void draw(void) {
    n_oam = 0;
    ObjAttr *o;
    bool confirm = mode == MODE_CONFIRM;

    if (mode == MODE_MENU || confirm) {            /* heart cursor and A button by the chosen option */
        int cx = (confirm ? (sel ? 168 : 72) : (n_opts == 1 ? 120 : sel ? 168 : 72));
        const char *txt = confirm ? (sel ? "Yes, hold A" : "No") : n_opts == 1 ? "Play" : sel ? "New game" : "Continue";
        int half = text_width(txt) / 2;
        int bob = isin((int)frame_count * 2) * 2 / 256;
        if (n_opts == 2 || confirm) {
            o = spr();
            o->attr0 = A0_Y(138) | A0_SQUARE;
            o->attr1 = A1_X(cx - half - 20 + bob) | A1_SIZE(1);
            o->attr2 = A2_TILE(T_UI) | A2_PRIO(0) | A2_PAL(P_UI);
        }
        {
            o = spr();
            o->attr0 = A0_Y(137 - (bob > 0 ? 1 : 0)) | A0_TALL;
            o->attr1 = A1_X(cx + half + 5) | A1_SIZE(2);
            o->attr2 = A2_TILE(T_ABTN) | A2_PRIO(0) | A2_PAL(P_ABTN);
        }
    }
    if (confirm && sel == 1) {                     /* hearts fill while A is held */
        int filled = hold_t * HOLD_HEARTS / HOLD_TIME;
        for (int i = 0; i < HOLD_HEARTS; i++) {
            o = spr();
            o->attr0 = A0_Y(70) | A0_SQUARE;
            o->attr1 = A1_X(120 - HOLD_HEARTS * 9 + i * 18 + 1) | A1_SIZE(1);
            o->attr2 = A2_TILE(T_UI + (i < filled ? 0 : 4)) | A2_PRIO(0) | A2_PAL(P_UI);
        }
    }

    for (int i = 0; i < LOGO_LETTERS && !confirm; i++) {
        const u16 *e = &logo_table[i * 8];
        if (mode == MODE_INTRO && t < i * LETTER_GAP) continue;
        int bob = mode == MODE_MENU && ly[i] == 0 ? isin((int)frame_count + i * 6) * 3 / (2 * 256) : 0;
        int y = e[2] - (ly[i] >> 8) + bob;
        o = spr();
        o->attr0 = A0_Y(y) | (u16)(e[3] << 14);
        o->attr1 = A1_X(e[1]) | A1_SIZE(e[4]);
        o->attr2 = A2_TILE(T_LOGO + e[0]) | A2_PRIO(1) | A2_PAL(P_LOGO + e[5]);
    }
    /* friends hop one at a time */
    if (hop_t < 0 && (frame_count % 90) == 0) {
        hop_who = (int)((frame_count / 90 * 3) % 4);   /* not game_rand: the title must not use up saved randomness */
        hop_t = 0;
    }
    for (int i = 0; i < 4; i++) {
        int lift = 0, frame = 0;
        if (i == hop_who && hop_t >= 0) {
            lift = isin(hop_t * 2) * 8 / 256;
            frame = 1;
        }
        o = spr();
        o->attr0 = A0_Y(cast[i].y - lift) | A0_SQUARE;
        o->attr1 = A1_X(cast[i].x) | A1_SIZE(2);
        o->attr2 = A2_TILE(T_CAST + i * 32 + frame * 16) | A2_PRIO(1) | A2_PAL(P_CAST + i);
    }
    if (hop_t >= 0 && ++hop_t > 16) hop_t = -1;
    o = spr();
    o->attr0 = A0_Y(99) | A0_TALL;
    o->attr1 = A1_X(112) | A1_SIZE(2);
    o->attr2 = A2_TILE(T_PIP) | A2_PRIO(1) | A2_PAL(P_PIP);
    static const u8 tw[6][2] = {{202, 92}, {64, 88}, {196, 18}, {34, 30}, {180, 66}, {56, 60}};
    for (int i = 0; i < 6; i++) {
        int ph = ((int)frame_count + i * 17) % 50;
        int f = ph < 8 ? 0 : ph < 16 ? 1 : ph < 24 ? 2 : ph < 32 ? 1 : ph < 40 ? 0 : -1;
        if (f < 0) continue;
        o = spr();
        o->attr0 = A0_Y(tw[i][1]) | A0_SQUARE;
        o->attr1 = A1_X(tw[i][0]) | A1_SIZE(0);
        o->attr2 = A2_TILE(T_SMALL + f) | A2_PRIO(1) | A2_PAL(P_SMALL);
    }
    for (int i = n_oam; i < 128; i++) oam[i].attr0 = A0_HIDE;
}

static void update(void) {
    t++;
    update_input();
    update_letters();
    if (mode == MODE_INTRO && t > LOGO_LETTERS * LETTER_GAP + 40) finish_intro();
    draw();
}

const Scene scene_title = {enter, update};
