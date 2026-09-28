/* Variable-width text drawn into tile strips at run time.
 * A strip is a block of tiles on a BG layer; text is rendered into a RAM
 * copy, then uploaded. Transparent pixels let the art underneath show. */
#ifndef TEXT_H
#define TEXT_H
#include "gba.h"

#define STRIP_MAX_TILES 64

typedef struct {
    u32 pix[STRIP_MAX_TILES * 8];   /* 4bpp tile rows */
    u8 tw, th;                      /* size in tiles */
    u16 first_tile;                 /* tile index inside the BG charblock */
    u8 cbb, sbb, col, row, pal;
} TextStrip;

/* set up a strip and write its map entries once. Strips are 2 KB each:
 * declare them EWRAM_BSS. tw * th must not exceed STRIP_MAX_TILES. */
void strip_init(TextStrip *s, int cbb, int first_tile, int sbb, int col, int row,
                int tw, int th, int pal);
int text_width(const char *str);
/* draw text; align 0 = left, 1 = centre; y is the glyph top in the strip */
void strip_print(TextStrip *s, const char *str, int align, int y, int color, int outline);
void strip_print_num(TextStrip *s, u32 value, int y, int color);

#endif
