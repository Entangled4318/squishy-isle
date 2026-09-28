/* Squishy data helpers: tiles per size/frame and flavor palettes. */
#ifndef SQUISHY_H
#define SQUISHY_H
#include "gba.h"
#include "game_assets.h"

#define NUM_SPECIES 16
#define NUM_FLAVORS 5
#define FLAVOR_SPARKLE 4

static inline const u16 *sq_palette(int species, int flavor) {
    return sq_area_pals[species_info[species].area] + flavor * 16;
}

/* copy nframes frames (starting at first) of size 16/32/64 to OBJ tile slot */
void sq_load_frames(int species, int size, int first, int nframes, int obj_tile);
void sq_load_palette(int species, int flavor, int obj_bank);
/* "Matcha Kitty" into buf (at least 32 bytes) */
void sq_full_name(char *buf, int species, int flavor);

#endif
