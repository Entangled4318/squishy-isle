/* House room (step 6.9): the door of each area's house opens a cozy room
 * whose four wall shelves hold that area's 20 gifts, one shelf per
 * species, one gift per friend (each friend sends its gift with its
 * letter). Gifts not received yet are soft silhouettes. The D-pad moves a
 * bouncing arrow over the gifts, A makes the chosen gift hop with its
 * friend's squeak, B or START go back out of the door. */
#include "collection.h"
#include "game.h"
#include "game_assets.h"
#include "sound.h"
#include "squishy.h"
#include "system.h"
#include "text.h"

#define T_GIFT    0        /* the area's 4 gifts, 16x16 */
#define T_ARROW   16
#define T_PIP     28       /* up0: 16x32 */
#define T_SMALL   36       /* twinkles, heart */
#define P_FLAVOR  0        /* 0..4: the area's flavor palettes */
#define P_SIL     5        /* 5..8: silhouettes, one tint per shelf */
#define P_ARROW   9
#define P_PIP     10
#define P_SMALL   11
#define P_TEXT    15

#define GIFT_HOP  18       /* frames of a gift's hop */

EWRAM_BSS static TextStrip st_pill;
static int area, sel_x, sel_y;      /* cursor: 2 rows (top, bottom shelves) x 10 (left shelf, then right) */
static int hop_t = -1, hearts_t = -1;

/* the gift under the cursor: species index in the area (shelf) and flavor */
static int shelf_of(int x, int y) { return y * 2 + (x >= 5); }
static int flavor_at(int x) { return x % 5; }
static int gift_id(int x, int y) { return friend_id(area * 4 + shelf_of(x, y), flavor_at(x)); }

static void gift_pos(int x, int y, int *sx, int *sy) {   /* bottom centre of a gift on its shelf */
    int col = x % 5, right = x >= 5;
    *sx = (right ? ROOM_RIGHT_X : ROOM_LEFT_X) + col * ROOM_SLOT + 9;
    *sy = y ? ROOM_BOTTOM_Y : ROOM_TOP_Y;
}

static void show_name(void) {
    char buf[48] = "From ";
    int id = gift_id(sel_x, sel_y);
    if (friend_found(id)) sq_full_name(buf + 5, friend_species(id), friend_flavor(id));
    else {
        const char *s = "Find me!";
        for (int i = 0; (buf[i] = s[i]); i++) {}
    }
    strip_print(&st_pill, buf, 1, 6, 1, 0);
}

static void enter(void) {
    area = game_save.area < AREA_COUNT ? game_save.area : 0;
    const RoomArt *r = &room_art[area];
    dma3_copy32(CHARBLOCK(0), r->tiles, r->tiles_bytes);
    for (int i = 0; i < 32 * 32; i++) SCREENBLOCK(30)[i] = SCREENBLOCK(31)[i] = 0;
    dma3_copy32(SCREENBLOCK(30), r->map, 32 * 20 * 2);
    dma3_copy16(PAL_BG, r->pal, r->pal_bytes);
    dma3_copy16(PAL_BG + P_TEXT * 16, ui_text_pal, sizeof ui_text_pal);
    for (int i = 0; i < 16; i++) CHARBLOCK(2)[i] = 0;
    REG_BGCNT(1) = BG_PRIO(2) | BG_CBB(0) | BG_SBB(30);
    REG_BGCNT(0) = BG_PRIO(1) | BG_CBB(2) | BG_SBB(31);
    for (int i = 0; i < 2; i++) bg_scroll_x[i] = bg_scroll_y[i] = 0;
    strip_init(&st_pill, 2, 1, 31, 7, 0, 16, 2, P_TEXT);         /* inside the pill at the top (128 px: the longest name is 121) */

    dma3_copy32(OBJ_TILES + T_GIFT * 16, gift_tiles + area * 4 * 32, 4 * 4 * 32);   /* 4 species x 32 words */
    dma3_copy16(PAL_OBJ + P_FLAVOR * 16, sq_area_pals[area], 5 * 32);
    dma3_copy16(PAL_OBJ + P_SIL * 16, sq_sil_pal, 4 * 32);
    dma3_copy32(OBJ_TILES + T_ARROW * 16, arrow_tiles, sizeof arrow_tiles);
    dma3_copy16(PAL_OBJ + P_ARROW * 16, arrow_pal, sizeof arrow_pal);
    dma3_copy32(OBJ_TILES + T_PIP * 16, pip_tiles + 3 * 8 * 8, 8 * 32);   /* Pip from behind, looking at the shelves */
    dma3_copy16(PAL_OBJ + P_PIP * 16, pip_pal, sizeof pip_pal);
    dma3_copy32(OBJ_TILES + T_SMALL * 16, ui_small_tiles, sizeof ui_small_tiles);
    dma3_copy16(PAL_OBJ + P_SMALL * 16, ui_small_pal, sizeof ui_small_pal);

    /* start on the newest friend's gift when it lives here, else the first one found */
    sel_x = sel_y = 0;
    int want = game_save.mail ? game_save.mail - 1 : -1, n = 0, first = -1;
    for (int y = 0; y < 2; y++)
        for (int x = 0; x < 10; x++) {
            int id = gift_id(x, y);
            if (!friend_found(id)) continue;
            n++;
            if (first < 0) first = y * 10 + x;
            if (id == want) first = -100 - (y * 10 + x);
        }
    if (first <= -100) first = -100 - first;
    if (first >= 0) {
        sel_x = first % 10;
        sel_y = first / 10;
    }
    hop_t = hearts_t = -1;
    show_name();
    dbg("scene house area %d gifts %d sel %d", area, n, gift_id(sel_x, sel_y));
    scene_blend(0, 0);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
}

static void draw(void) {
    int n = 0, sx, sy;
    gift_pos(sel_x, sel_y, &sx, &sy);
    int bob = isin((int)frame_count * 2) * 2 / 256;             /* the arrow bobs over the chosen gift */
    oam[n].attr0 = A0_Y(sy - 16 - 15 + bob) | A0_SQUARE;
    oam[n].attr1 = A1_X(sx - 8) | A1_SIZE(1) | A1_VFLIP;
    oam[n].attr2 = A2_TILE(T_ARROW + 4) | A2_PRIO(0) | A2_PAL(P_ARROW);
    n++;
    if (hearts_t >= 0) {                                         /* hearts rise from a squeezed gift */
        for (int k = 0; k < 2; k++) {
            int t = hearts_t - k * 8;
            if (t < 0 || t > 36) continue;
            oam[n].attr0 = A0_Y(sy - 22 - t / 2) | A0_SQUARE;
            oam[n].attr1 = A1_X(sx - 4 + (k ? 6 : -6) + isin(t * 3 + k * 20) * 2 / 256) | A1_SIZE(0);
            oam[n].attr2 = A2_TILE(T_SMALL + UI_HEART) | A2_PRIO(0) | A2_PAL(P_SMALL);
            n++;
        }
        if (++hearts_t > 44) hearts_t = -1;
    }
    for (int y = 0; y < 2; y++)
        for (int x = 0; x < 10; x++) {
            int gx, gy, id = gift_id(x, y), lift = 0;
            gift_pos(x, y, &gx, &gy);
            bool have = friend_found(id);
            if (x == sel_x && y == sel_y && hop_t >= 0) lift = have ? isin(hop_t * 32 / GIFT_HOP) * 8 / 256 : 0;
            int shake = (!have && x == sel_x && y == sel_y && hop_t >= 0) ? ((hop_t / 3) & 1 ? 1 : -1) : 0;
            oam[n].attr0 = A0_Y(gy - 16 - lift) | A0_SQUARE;
            oam[n].attr1 = A1_X(gx - 8 + shake) | A1_SIZE(1);
            oam[n].attr2 = A2_TILE(T_GIFT + shelf_of(x, y) * 4) | A2_PRIO(1) |
                           A2_PAL(have ? P_FLAVOR + friend_flavor(id) : P_SIL + shelf_of(x, y));
            n++;
        }
    if (hop_t >= 0 && ++hop_t > GIFT_HOP) hop_t = -1;
    /* Pip on the rug, looking at the shelves, a small idle bob */
    int pb = ((frame_count / 40) & 1);
    oam[n].attr0 = A0_Y(141 - PIP_FEET_ROW - pb) | A0_TALL;
    oam[n].attr1 = A1_X(112) | A1_SIZE(2);
    oam[n].attr2 = A2_TILE(T_PIP) | A2_PRIO(1) | A2_PAL(P_PIP);
    n++;
    for (int i = n; i < 128; i++) oam[i].attr0 = A0_HIDE;
}

static void update(void) {
    u16 hit = scene_fading() ? 0 : key_hit();
    int ox = sel_x, oy = sel_y;
    if (hit & KEY_LEFT) sel_x = (sel_x + 9) % 10;
    if (hit & KEY_RIGHT) sel_x = (sel_x + 1) % 10;
    if (hit & (KEY_UP | KEY_DOWN)) sel_y ^= 1;
    if (sel_x != ox || sel_y != oy) {
        sfx_tick();
        hop_t = -1;
        show_name();
    }
    if (hit & KEY_A) {
        int id = gift_id(sel_x, sel_y);
        hop_t = 0;
        if (friend_found(id)) {
            sfx_squeak(friend_flavor(id));
            hearts_t = 0;
        } else {
            sfx_blip();                  /* still hiding: the silhouette wiggles */
        }
        dbg("house pick %d found %d", id, friend_found(id));
    }
    if (hit & (KEY_B | KEY_START)) {
        sfx_blip();
        dbg("house leave");
        scene_go(&scene_meadow_view);
    }
    draw();
}

const Scene scene_house = {enter, update};
