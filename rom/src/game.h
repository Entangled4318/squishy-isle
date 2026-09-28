/* Shared game state and the list of scenes. */
#ifndef GAME_H
#define GAME_H
#include "scene.h"

extern int sel_species, sel_flavor;       /* friend shown in the close-up */

extern const Scene scene_shelf;
extern const Scene scene_closeup;
extern const Scene scene_meadow_view;

/* shared 64-entry sine table, 256 = 1.0 */
extern const s16 sin64[64];
static inline int isin(int i) { return sin64[i & 63]; }

#endif
