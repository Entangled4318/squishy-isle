/* Shared game state and the list of scenes. */
#ifndef GAME_H
#define GAME_H
#include "scene.h"

#define GAME_VERSION "v1.1"               /* shown small on the title; BRICK.md and the release notes match it */

extern int sel_species, sel_flavor;       /* friend shown in the close-up */

extern const Scene scene_shelf;
extern const Scene scene_closeup;
extern const Scene scene_meadow_view;
extern const Scene scene_title;
extern const Scene scene_open;
extern const Scene scene_reveal;
extern const Scene scene_jukebox;
extern const Scene scene_letter;          /* the newest friend's letter, from the meadow mailbox */
extern const Scene scene_house;           /* the room inside each area's house: that area's gifts */

extern int open_color, open_friend;       /* the box being opened and its friend (-1 = none) */
extern bool shelf_pick;                   /* shelf opened from the pen sign: pick followers */

void meadow_reset(void);                  /* new game: Pip home, fresh boxes */

/* shared 64-entry sine table, 256 = 1.0 */
extern const s16 sin64[64];
static inline int isin(int i) { return sin64[i & 63]; }

#endif
