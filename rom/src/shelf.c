/* Squishy Shelf: four pages (one per area), 4 species x 5 flavors each.
 * D-pad moves the gold frame, L/R turn pages, A opens the friend big.
 * Pick mode (A at the pen sign): the meadow page with the real collection;
 * A adds or removes a follower (up to 3, the oldest drops off), B goes back. */
#include "collection.h"
#include "game.h"
#include "game_assets.h"
#include "music_data.h"
#include "sound.h"
#include "squishy.h"
#include "system.h"
#include "text.h"

#define T_SQ      0      /* row r: idle at r*32, open at r*32+16 */
#define T_FRAME   128
#define T_SMALL   160
#define P_SIL     5      /* 5..8 silhouettes, one per row */
#define P_FRAME   9
#define P_SMALL   10
#define P_TEXT    15

static int page, cur_r, cur_c = 0;
bool shelf_pick;
static int pop_id = -1, pop_t;      /* the heart that just appeared, for a little pop */
EWRAM_BSS static TextStrip st_name;
static const u16 *const page_maps[4] = {shelf_map0, shelf_map1, shelf_map2, shelf_map3};

static bool collected(int r, int c) { return friend_found(friend_id(page * 4 + r, c)); }

static void show_name(void) {
    char buf[40];
    if (collected(cur_r, cur_c)) {
        sq_full_name(buf, page * 4 + cur_r, cur_c);
        strip_print(&st_name, buf, 1, 7, 1, 0);
    } else {
        strip_print(&st_name, "Find me!", 1, 7, 1, 0);
    }
}

static void load_page(void) {
    dma3_copy32(SCREENBLOCK(30), page_maps[page], sizeof shelf_map0);
    for (int r = 0; r < 4; r++) sq_load_frames(page * 4 + r, 32, 0, 2, T_SQ + r * 32);
    dma3_copy16(PAL_OBJ, sq_area_pals[page], 5 * 32);
    show_name();
}

static void enter(void) {
    music_play(SONG_SHELF);          /* music box; the close-up keeps it */
    dma3_copy32(CHARBLOCK(0), shelf_tiles, sizeof shelf_tiles);
    dma3_copy16(PAL_BG, shelf_pal, sizeof shelf_pal);
    dma3_copy16(PAL_BG + P_TEXT * 16, ui_text_pal, sizeof ui_text_pal);
    for (int i = 0; i < 16; i++) CHARBLOCK(2)[i] = 0;
    for (int i = 0; i < 32 * 32; i++) SCREENBLOCK(31)[i] = 0;
    REG_BGCNT(1) = BG_PRIO(2) | BG_CBB(0) | BG_SBB(30);
    REG_BGCNT(0) = BG_PRIO(0) | BG_CBB(2) | BG_SBB(31);
    strip_init(&st_name, 2, 1, 31, 16, 0, 13, 2, P_TEXT);

    dma3_copy32(OBJ_TILES + T_FRAME * 16, ui_frame_tiles, sizeof ui_frame_tiles);
    dma3_copy32(OBJ_TILES + T_SMALL * 16, ui_small_tiles, sizeof ui_small_tiles);
    dma3_copy16(PAL_OBJ + P_SIL * 16, sq_sil_pal, sizeof sq_sil_pal);
    dma3_copy16(PAL_OBJ + P_FRAME * 16, ui_frame_pal, sizeof ui_frame_pal);
    dma3_copy16(PAL_OBJ + P_SMALL * 16, ui_small_pal, sizeof ui_small_pal);
    if (shelf_pick) {                 /* start on the first follower, or the first friend found */
        page = 0;
        int id = follower_get(0);
        for (int i = 0; id < 0 && i < 20; i++)
            if (friend_found(i)) id = i;
        if (id >= 0) { cur_r = friend_species(id); cur_c = friend_flavor(id); }
        pop_id = -1;
    } else if (!collected(cur_r, cur_c)) {   /* start on a friend: A then opens it straight away */
        for (int i = 0; i < 20 && !collected(cur_r, cur_c); i++) {
            cur_r = i / 5;
            cur_c = i % 5;
        }
        if (!collected(cur_r, cur_c)) cur_r = cur_c = 0;
    }
    load_page();
    if (shelf_pick) strip_print(&st_name, "Who follows Pip?", 1, 7, 1, 0);   /* until the frame moves */
    dbg("scene shelf page=%d pick=%d", page, shelf_pick);
    scene_blend(0, 0);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
}

static void draw(void) {
    int bob = (isin((int)frame_count * 2) * 2) / 256;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 5; c++) {
            int i = r * 5 + c;
            bool sel = r == cur_r && c == cur_c;
            bool have = collected(r, c);
            int x = SHELF_X0 + c * SHELF_CW + (SHELF_CW - 32) / 2;
            int y = SHELF_Y0 + r * SHELF_CH + SHELF_CH - 4 - 31;
            if (sel && have) y -= 2 + (bob > 0 ? bob : 0);
            oam[i].attr0 = A0_Y(y) | A0_SQUARE;
            oam[i].attr1 = A1_X(x) | A1_SIZE(2);
            oam[i].attr2 = A2_TILE(T_SQ + r * 32 + (sel && have ? 16 : 0)) | A2_PRIO(1) |
                           A2_PAL(have ? c : P_SIL + r);
            /* sparkle friends twinkle */
            ObjAttr *tw = &oam[21 + r];
            int phase = ((int)frame_count + r * 17) % 72;
            int f = phase < 8 ? 0 : phase < 16 ? 1 : phase < 24 ? 2 : phase < 32 ? 1 : phase < 40 ? 0 : -1;
            if (c == 4) {
                if (have && f >= 0) {
                    tw->attr0 = A0_Y(y + 4) | A0_SQUARE;
                    tw->attr1 = A1_X(x + 24) | A1_SIZE(0);
                    tw->attr2 = A2_TILE(T_SMALL + f) | A2_PRIO(0) | A2_PAL(P_SMALL);
                } else {
                    tw->attr0 = A0_HIDE;
                }
            }
        }
    }
    for (int i = 0; i < MAX_FOLLOWERS; i++) {      /* a heart on each friend that follows Pip */
        ObjAttr *h = &oam[25 + i];
        int id = shelf_pick ? follower_get(i) : -1;
        if (id < 0) { h->attr0 = A0_HIDE; continue; }
        int hx = SHELF_X0 + friend_flavor(id) * SHELF_CW + 3;
        int hy = SHELF_Y0 + friend_species(id) * SHELF_CH + 3;
        if (id == pop_id && pop_t > 0) hy -= (pop_t * (12 - pop_t)) / 6;   /* small hop when chosen */
        h->attr0 = A0_Y(hy) | A0_SQUARE;
        h->attr1 = A1_X(hx) | A1_SIZE(0);
        h->attr2 = A2_TILE(T_SMALL + 3) | A2_PRIO(0) | A2_PAL(P_SMALL);
    }
    if (pop_t > 0) pop_t--;
    int fx = SHELF_X0 + cur_c * SHELF_CW - 1;
    int fy = SHELF_Y0 + cur_r * SHELF_CH - 1;
    oam[20].attr0 = A0_Y(fy) | A0_WIDE;
    oam[20].attr1 = A1_X(fx) | A1_SIZE(3);
    oam[20].attr2 = A2_TILE(T_FRAME) | A2_PRIO(1) | A2_PAL(P_FRAME);
}

static void update(void) {
    u16 hit = key_hit();
    int r = cur_r, c = cur_c;
    if (hit & KEY_LEFT && c > 0) c--;
    if (hit & KEY_RIGHT && c < 4) c++;
    if (hit & KEY_UP && r > 0) r--;
    if (hit & KEY_DOWN && r < 3) r++;
    if (r != cur_r || c != cur_c) {
        cur_r = r;
        cur_c = c;
        sfx_tick();
        show_name();
    }
    if (shelf_pick) {
        if (hit & KEY_A) {
            int id = friend_id(page * 4 + cur_r, cur_c);
            if (!friend_found(id)) {
                sfx_blip();
            } else if (follower_has(id)) {
                follower_remove(id);
                collection_save();
                sfx_chime(1);
                dbg("pick remove %d", id);
            } else {
                follower_add(id);
                collection_save();
                sfx_squeak(cur_c);
                pop_id = id;
                pop_t = 12;
                dbg("pick add %d", id);
            }
        }
        if (hit & (KEY_START | KEY_B)) {
            shelf_pick = false;
            sfx_chime(2);
            dbg("pick done %d %d %d", follower_get(0), follower_get(1), follower_get(2));
            scene_go(&scene_meadow_view);
        }
        draw();
        return;
    }
    if (hit & (KEY_L | KEY_R)) {
        page = (page + ((hit & KEY_R) ? 1 : 3)) & 3;
        sfx_chime(page);
        load_page();
        dbg("shelf page=%d", page);
    }
    if (hit & KEY_A) {
        if (collected(cur_r, cur_c)) {
            sel_species = page * 4 + cur_r;
            sel_flavor = cur_c;
            sfx_squeak(cur_c);
            scene_go(&scene_closeup);
        } else {
            sfx_blip();
        }
    }
    if (hit & (KEY_START | KEY_B)) {
        sfx_chime(2);
        scene_go(&scene_meadow_view);
    }
    draw();
}

const Scene scene_shelf = {enter, update};
