/* Hardware check screen: proves display, sprites, affine squash, palette
 * swaps, every button, all four sound channels and the battery save work
 * on the target emulator before the real game is built on top. */
#include "gba.h"
#include "hw_assets.h"
#include "save.h"
#include "sound.h"
#include "system.h"
#include "text.h"

static const s16 sin64[64] = {
    0, 25, 50, 74, 98, 121, 142, 162, 181, 198, 213, 226, 237, 245, 251, 255,
    256, 255, 251, 245, 237, 226, 213, 198, 181, 162, 142, 121, 98, 74, 50, 25,
    0, -25, -50, -74, -98, -121, -142, -162, -181, -198, -213, -226, -237, -245, -251, -255,
    -256, -255, -251, -245, -237, -226, -213, -198, -181, -162, -142, -121, -98, -74, -50, -25};

#define BUNNY_X      188     /* centre of the bunny on the cushion */
#define BUNNY_FLOOR  110     /* screen y of the bunny's bottom edge */
#define BUNNY_BOTTOM 29      /* texture rows from sprite centre to bottom */

#define TILE_IDLE    0
#define TILE_SQUISH  64
#define TILE_SHADOW  128
#define TILE_ICONS   132
#define TILE_TWINKLE 180

#define PAL_BUNNY    0
#define PAL_SHADOW   1
#define PAL_ICON_OFF 2
#define PAL_ICON_ON  3
#define PAL_TWINKLE  4
#define PAL_TEXT     15

typedef struct { u16 key; s16 x; u16 tile; u8 wide; } Icon;
static const Icon icons[10] = {
    {KEY_LEFT, 9, 0, 0},    {KEY_UP, 27, 4, 0},     {KEY_DOWN, 45, 8, 0},  {KEY_RIGHT, 63, 12, 0},
    {KEY_L, 85, 24, 0},     {KEY_R, 103, 28, 0},    {KEY_B, 125, 20, 0},   {KEY_A, 143, 16, 0},
    {KEY_SELECT, 165, 40, 1}, {KEY_START, 199, 32, 1},
};
#define ICON_Y 141

static SaveData sd;
static TextStrip st_boots, st_squish, st_save, st_emu, st_flavor;
static int flavor;
static int squish_t = -1, hop_t = -1, bounce_t = -1, nudge_x, breathe;

static void update_twinkles(int cx, int cy);

static void load_flavor(int f) {
    flavor = f;
    for (int i = 0; i < 16; i++) PAL_OBJ[PAL_BUNNY * 16 + i] = hw_bunny_pals[f * 16 + i];
    strip_print(&st_flavor, hw_flavor_names[f], 1, GLYPH_Y, 1, 0);
}

static void show_counts(void) {
    strip_print_num(&st_boots, sd.boots, GLYPH_Y, 1);
    strip_print_num(&st_squish, sd.squishes, GLYPH_Y, 1);
}

static void store(void) {
    if (!save_store(&sd)) {
        strip_print(&st_save, "error", 0, GLYPH_Y, 3, 0);
        dbg("save: write verify failed");
    }
}

static void load_graphics(void) {
    REG_DISPCNT = DCNT_BLANK;
    dma3_copy32(CHARBLOCK(0), hw_bg_tiles, sizeof hw_bg_tiles);
    dma3_copy32(SCREENBLOCK(30), hw_bg_map, sizeof hw_bg_map);
    dma3_copy16(PAL_BG, hw_bg_pal, sizeof hw_bg_pal);
    dma3_copy16(PAL_BG + PAL_TEXT * 16, hw_text_pal, sizeof hw_text_pal);
    for (int i = 0; i < 16; i++) CHARBLOCK(1)[i] = 0;            /* blank tile 0 */
    for (int i = 0; i < 32 * 32; i++) SCREENBLOCK(31)[i] = 0;

    dma3_copy32(OBJ_TILES, hw_bunny_tiles, sizeof hw_bunny_tiles);
    dma3_copy32(OBJ_TILES + TILE_SHADOW * 16, hw_shadow_tiles, sizeof hw_shadow_tiles);
    dma3_copy32(OBJ_TILES + TILE_ICONS * 16, hw_icon_tiles, sizeof hw_icon_tiles);
    dma3_copy32(OBJ_TILES + TILE_TWINKLE * 16, hw_twinkle_tiles, sizeof hw_twinkle_tiles);
    dma3_copy16(PAL_OBJ + PAL_TWINKLE * 16, hw_twinkle_pal, sizeof hw_twinkle_pal);
    dma3_copy16(PAL_OBJ + PAL_SHADOW * 16, hw_shadow_pal, sizeof hw_shadow_pal);
    dma3_copy16(PAL_OBJ + PAL_ICON_OFF * 16, hw_icon_pal_off, sizeof hw_icon_pal_off);
    dma3_copy16(PAL_OBJ + PAL_ICON_ON * 16, hw_icon_pal_on, sizeof hw_icon_pal_on);

    REG_BGCNT(1) = BG_PRIO(2) | BG_CBB(0) | BG_SBB(30);
    REG_BGCNT(0) = BG_PRIO(0) | BG_CBB(1) | BG_SBB(31);
    strip_init(&st_boots, 1, 1, 31, VALUE_COL, ROW_BOOTS, 8, 2, PAL_TEXT);
    strip_init(&st_squish, 1, 17, 31, VALUE_COL, ROW_SQUISH, 8, 2, PAL_TEXT);
    strip_init(&st_save, 1, 33, 31, VALUE_COL, ROW_SAVE, 8, 2, PAL_TEXT);
    strip_init(&st_emu, 1, 49, 31, VALUE_COL, ROW_EMU, 8, 2, PAL_TEXT);
    strip_init(&st_flavor, 1, 65, 31, FLAVOR_COL, FLAVOR_ROW, FLAVOR_TILES, 2, PAL_TEXT);
    /* semi-transparent shadow blends onto the background */
    REG_BLDCNT = BLD_BG1 << 8;
    REG_BLDALPHA = 6 | (10 << 8);
    oam_hide_all();
    oam_commit();
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
}

static void update_bunny(void) {
    int sx = 256, sy = 256, lift = 0, tile = TILE_IDLE;

    breathe = (breathe + 1) & 127;
    int b = sin64[breathe >> 1];
    sy += b / 48;
    sx -= b / 64;

    if (hop_t >= 0) {
        const int T = 26, Hh = 24;
        if (hop_t < T) {
            lift = 4 * Hh * hop_t * (T - hop_t) / (T * T);
            if (hop_t < 6) { sx = 230; sy = 284; }          /* stretch on take-off */
        } else {
            tile = TILE_SQUISH;                               /* land with a squish */
        }
        if (hop_t == T) sfx_tick();
        if (++hop_t > T + 5) hop_t = -1;
    }
    if (bounce_t >= 0) {
        lift += (6 * sin64[bounce_t * 4]) / 256;
        if (++bounce_t >= 8) bounce_t = -1;
    }
    if (squish_t >= 0) {
        if (squish_t < 7) {
            tile = TILE_SQUISH;
        } else {
            int k = squish_t - 7;                              /* springy recovery */
            int amp = 40 - k * 3;
            if (amp < 0) amp = 0;
            int w = sin64[(k * 9) & 63];
            sy = 256 + amp * w / 256;
            sx = 256 - amp * w / 512;
        }
        if (++squish_t > 22) squish_t = -1;
    }
    if (nudge_x > 0) nudge_x--;
    if (nudge_x < 0) nudge_x++;

    int cx = BUNNY_X + nudge_x / 2;
    int bottom = BUNNY_FLOOR - lift;
    int cy = bottom - (BUNNY_BOTTOM * sy) / 256;

    oam[0].attr0 = A0_Y(cy - 64) | A0_AFFINE | A0_DOUBLE | A0_SQUARE;
    oam[0].attr1 = A1_X(cx - 64) | A1_AFF(0) | A1_SIZE(3);
    oam[0].attr2 = A2_TILE(tile) | A2_PRIO(1) | A2_PAL(PAL_BUNNY);
    affine_scale(0, sx, sy);
    update_twinkles(cx, cy);

    /* shadow shrinks a little while the bunny is in the air */
    oam[1].attr0 = A0_Y(BUNNY_FLOOR - 5) | A0_WIDE | A0_BLEND;
    oam[1].attr1 = A1_X(BUNNY_X - 16 + nudge_x / 2) | A1_SIZE(1);
    oam[1].attr2 = A2_TILE(TILE_SHADOW) | A2_PRIO(2) | A2_PAL(PAL_SHADOW);
}

/* Sparkle flavor: three twinkles pulse around the bunny (tiny, small, big) */
static void update_twinkles(int cx, int cy) {
    static const s8 off[3][2] = {{-26, -18}, {22, -8}, {-18, 14}};
    static int t;
    t++;
    for (int i = 0; i < 3; i++) {
        ObjAttr *o = &oam[12 + i];
        int phase = (t + i * 23) % 60;
        int frame = phase < 10 ? 0 : phase < 20 ? 1 : phase < 30 ? 2 : phase < 40 ? 1 : phase < 48 ? 0 : -1;
        if (flavor != FLAVOR_SPARKLE || frame < 0) {
            o->attr0 = A0_HIDE;
            continue;
        }
        o->attr0 = A0_Y(cy + off[i][1] - 4) | A0_SQUARE;
        o->attr1 = A1_X(cx + off[i][0] - 4) | A1_SIZE(0);
        o->attr2 = A2_TILE(TILE_TWINKLE + frame) | A2_PRIO(1) | A2_PAL(PAL_TWINKLE);
    }
}

static void update_icons(void) {
    u16 held = key_held();
    for (int i = 0; i < 10; i++) {
        const Icon *ic = &icons[i];
        oam[2 + i].attr0 = A0_Y(ICON_Y) | (ic->wide ? A0_WIDE : A0_SQUARE);
        oam[2 + i].attr1 = A1_X(ic->x) | A1_SIZE(ic->wide ? 2 : 1);
        oam[2 + i].attr2 = A2_TILE(TILE_ICONS + ic->tile) | A2_PRIO(1) |
                           A2_PAL((held & ic->key) ? PAL_ICON_ON : PAL_ICON_OFF);
    }
}

int hwcheck_main(void) {
    system_init();
    sound_init();
    load_graphics();

    SaveStatus st = save_load(&sd);
    sd.boots++;
    store();
    const char *label = st == SAVE_LOADED ? "OK" : st == SAVE_NEW ? "NEW" : "RESET";
    strip_print(&st_save, st == SAVE_LOADED ? "working" : st == SAVE_NEW ? "new file" : "was reset",
                0, GLYPH_Y, 1, 0);
    strip_print(&st_emu, is_mgba() ? "mGBA" : "other", 0, GLYPH_Y, 1, 0);
    show_counts();
    load_flavor(sd.flavor % 5);
    dbg("boot: save=%s boots=%u squishes=%u flavor=%d", label, sd.boots, sd.squishes, flavor);

    for (;;) {
        vblank_wait();
        oam_commit();
        input_poll();
        sound_tick();
        u16 hit = key_hit();

        if (hit & KEY_A) {
            squish_t = 0;
            sfx_squeak(flavor);
            sd.squishes++;
            show_counts();
            store();
            dbg("squish %u", sd.squishes);
        }
        if ((hit & KEY_B) && hop_t < 0) {
            hop_t = 0;
            sfx_boing();
        }
        if (hit & (KEY_L | KEY_R)) {
            load_flavor((flavor + ((hit & KEY_R) ? 1 : 4)) % 5);
            sd.flavor = (u16)flavor;
            store();
            sfx_chime(flavor);
            bounce_t = 0;
            dbg("flavor %s", hw_flavor_names[flavor]);
        }
        if (hit & KEY_START) {
            song_play(tune_hello, tune_hello_len);
            bounce_t = 0;
        }
        if (hit & KEY_SELECT) sfx_blip();
        if (hit & KEY_LEFT) { nudge_x = -12; sfx_tick(); }
        if (hit & KEY_RIGHT) { nudge_x = 12; sfx_tick(); }
        if (hit & KEY_UP) { bounce_t = 0; sfx_tick(); }
        if (hit & KEY_DOWN) { squish_t = 7; sfx_tick(); }

        update_bunny();
        update_icons();
    }
}
