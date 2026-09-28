/* Scene manager: one scene runs at a time; changes use a soft white fade. */
#ifndef SCENE_H
#define SCENE_H
#include "gba.h"

typedef struct {
    void (*enter)(void);     /* load VRAM; the screen is white while this runs */
    void (*update)(void);    /* once per frame, after input and sound */
} Scene;

void scene_run(const Scene *first);
void scene_go(const Scene *next);   /* ignored while a fade runs */
bool scene_fading(void);
void scene_reload(void);            /* fade out and enter the same scene again (walking to another area) */

/* blend setup the scene wants when no fade runs (shadows etc.) */
void scene_blend(u16 bldcnt, u16 bldalpha);

/* background scroll, applied at the start of the next frame */
extern s16 bg_scroll_x[4], bg_scroll_y[4];

extern u32 frame_count;

#endif
