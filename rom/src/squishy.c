#include "squishy.h"

void sq_load_frames(int species, int size, int first, int nframes, int obj_tile) {
    const SpeciesInfo *s = &species_info[species];
    const u32 *src = size == 16 ? s->t16 : size == 32 ? s->t32 : s->t64;
    int tiles = (size / 8) * (size / 8);
    dma3_copy32(OBJ_TILES + obj_tile * 16, src + first * tiles * 8, (u32)(nframes * tiles * 32));
}

void sq_load_palette(int species, int flavor, int obj_bank) {
    dma3_copy16(PAL_OBJ + obj_bank * 16, sq_palette(species, flavor), 32);
}

void sq_full_name(char *buf, int species, int flavor) {
    int n = 0;
    for (const char *p = flavor_names[flavor]; *p; p++) buf[n++] = *p;
    buf[n++] = ' ';
    for (const char *p = species_info[species].name; *p; p++) buf[n++] = *p;
    buf[n] = 0;
}
