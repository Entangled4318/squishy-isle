#include "scene.h"

#include "sound.h"
#include "system.h"

#define FADE_FRAMES 10

static const Scene *current, *pending;
static int fade_level;        /* 0 = clear, 16 = white */
static int fade_dir;          /* +1 fading out, -1 fading in, 0 idle */
static u16 want_bldcnt, want_bldalpha;
static u32 sound_vbl;         /* last VBlank the sound was ticked for */

s16 bg_scroll_x[4], bg_scroll_y[4];
u32 frame_count;

void scene_blend(u16 bldcnt, u16 bldalpha) {
    want_bldcnt = bldcnt;
    want_bldalpha = bldalpha;
}

bool scene_fading(void) { return fade_dir != 0; }

void scene_go(const Scene *next) {
    if (fade_dir || next == current) return;
    pending = next;
    fade_dir = 1;
}

void scene_reload(void) {
    if (fade_dir) return;
    pending = current;
    fade_dir = 1;
}

static void apply_blend(void) {
    if (fade_level > 0) {
        REG_BLDCNT = 0x3F | BLD_WHITE;
        REG_BLDY = (u16)fade_level;
    } else {
        REG_BLDCNT = want_bldcnt;
        REG_BLDALPHA = want_bldalpha;
        REG_BLDY = 0;
    }
}

void scene_run(const Scene *first) {
    current = first;
    fade_level = 16;
    apply_blend();
    oam_hide_all();          /* OAM powers up as 128 visible sprites at (0,0) */
    oam_commit();
    current->enter();
    fade_dir = -1;
    sound_vbl = vbl_count;
    for (;;) {
        vblank_wait();
        oam_commit();
        for (int i = 0; i < 4; i++) {
            REG_BGHOFS(i) = (u16)bg_scroll_x[i];
            REG_BGVOFS(i) = (u16)bg_scroll_y[i];
        }
        apply_blend();
        frame_count++;
        input_poll();
        /* one sound tick per VBlank that passed: a slow frame (or a scene
         * loading while the screen is white) does not slow the music */
        int ticks = (int)(vbl_count - sound_vbl);
        sound_vbl = vbl_count;
        if (ticks > 8) ticks = 8;
        while (ticks-- > 0) sound_tick();

        if (fade_dir > 0) {
            fade_level += 16 / (FADE_FRAMES / 2);
            if (fade_level >= 16) {
                fade_level = 16;
                apply_blend();
                oam_hide_all();
                oam_commit();
                for (int i = 0; i < 4; i++) bg_scroll_x[i] = bg_scroll_y[i] = 0;
                current = pending;
                current->enter();
                fade_dir = -1;
            }
            continue;
        }
        if (fade_dir < 0) {
            fade_level -= 16 / (FADE_FRAMES / 2);
            if (fade_level <= 0) {
                fade_level = 0;
                fade_dir = 0;
            }
        }
        current->update();
    }
}
