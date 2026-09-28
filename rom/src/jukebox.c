/* Sound test: every song of the game, for a parent (and for the tests).
 * Hold L + R + START at power-on. LEFT / RIGHT choose, A plays, B stops,
 * SELECT plays a jingle over the song, L boings (both check that the song
 * goes on after an effect). */
#include "game.h"
#include "game_assets.h"
#include "music_data.h"
#include "sound.h"
#include "system.h"
#include "text.h"

#define P_TEXT 15

EWRAM_BSS static TextStrip st_head, st_song, st_state, st_help1, st_help2;
static int sel;

static void show(void) {
    char buf[40];
    int k = 0;
    int n = sel + 1;
    if (n >= 10) buf[k++] = (char)('0' + n / 10);
    buf[k++] = (char)('0' + n % 10);
    buf[k++] = ' ';
    for (const char *p = songs[sel].title; *p && k < 38; p++) buf[k++] = *p;
    buf[k] = 0;
    strip_print(&st_song, buf, 1, 3, 1, 2);
    strip_print(&st_state, music_current() == sel ? "playing" : "stopped", 1, 3, 3, 0);
}

static void enter(void) {
    for (int i = 0; i < 16; i++) CHARBLOCK(2)[i] = 0;
    for (int i = 0; i < 32 * 32; i++) SCREENBLOCK(31)[i] = 0;
    dma3_copy16(PAL_BG + P_TEXT * 16, ui_text_pal, sizeof ui_text_pal);
    PAL_BG[0] = 0x7F9F;                               /* soft pink */
    REG_BGCNT(0) = BG_PRIO(0) | BG_CBB(2) | BG_SBB(31);
    strip_init(&st_head, 2, 1, 31, 7, 2, 16, 2, P_TEXT);
    strip_init(&st_song, 2, 33, 31, 3, 7, 24, 2, P_TEXT);
    strip_init(&st_state, 2, 81, 31, 9, 10, 12, 2, P_TEXT);
    strip_init(&st_help1, 2, 105, 31, 2, 14, 26, 2, P_TEXT);
    strip_init(&st_help2, 2, 157, 31, 2, 16, 26, 2, P_TEXT);
    strip_print(&st_head, "Sound test", 1, 3, 2, 1);
    strip_print(&st_help1, "LEFT RIGHT choose   A play   B stop", 1, 3, 1, 0);
    strip_print(&st_help2, "SELECT jingle   L boing", 1, 3, 1, 0);
    sel = 0;
    show();
    dbg("scene jukebox songs=%d", SONG_COUNT);
    scene_blend(0, 0);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
}

static void update(void) {
    u16 hit = key_hit();
    if (hit & KEY_RIGHT) { sel = (sel + 1) % SONG_COUNT; show(); }
    if (hit & KEY_LEFT) { sel = (sel + SONG_COUNT - 1) % SONG_COUNT; show(); }
    if (hit & KEY_A) {
        music_play(sel);
        dbg("music play %d", sel);
        show();
    }
    if (hit & KEY_B) {
        music_stop();
        dbg("music stop");
        show();
    }
    if (hit & KEY_SELECT) song_play(tune_hello, tune_hello_len);
    if (hit & KEY_L) sfx_boing();
}

const Scene scene_jukebox = {enter, update};
