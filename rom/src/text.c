#include "text.h"

#include "font_data.h"

void strip_init(TextStrip *s, int cbb, int first_tile, int sbb, int col, int row,
                int tw, int th, int pal) {
    s->tw = (u8)tw;
    s->th = (u8)th;
    s->first_tile = (u16)first_tile;
    s->cbb = (u8)cbb;
    s->sbb = (u8)sbb;
    s->col = (u8)col;
    s->row = (u8)row;
    s->pal = (u8)pal;
    vu16 *map = SCREENBLOCK(sbb);
    for (int y = 0; y < th; y++)
        for (int x = 0; x < tw; x++)
            map[(row + y) * 32 + col + x] = (u16)((first_tile + y * tw + x) | (pal << 12));
}

static const u8 *glyph(char c, int *w) {
    int i = (u8)c;
    if (i < FONT_FIRST || i > FONT_LAST) i = '?';
    i -= FONT_FIRST;
    *w = font_widths[i];
    return &font_rows[i * 9];
}

int text_width(const char *str) {
    int total = 0, w;
    for (const char *p = str; *p; p++) {
        glyph(*p, &w);
        total += w + 1;
    }
    return total ? total - 1 : 0;
}

static void plot(TextStrip *s, int x, int y, int c) {
    if (x < 0 || y < 0 || x >= s->tw * 8 || y >= s->th * 8) return;
    int tile = (y >> 3) * s->tw + (x >> 3);
    u32 *row = &s->pix[tile * 8 + (y & 7)];
    int sh = (x & 7) * 4;
    *row = (*row & ~(0xFu << sh)) | ((u32)c << sh);
}

static void upload(TextStrip *s) {
    dma3_copy32(CHARBLOCK(s->cbb) + s->first_tile * 16, s->pix, s->tw * s->th * 32);
}

void strip_print(TextStrip *s, const char *str, int align, int y, int color, int outline) {
    for (int i = 0; i < s->tw * s->th * 8; i++) s->pix[i] = 0;
    int x = align ? (s->tw * 8 - text_width(str)) / 2 : 0;
    for (int pass = outline ? 0 : 1; pass < 2; pass++) {
        int cx = x;
        for (const char *p = str; *p; p++) {
            int w;
            const u8 *rows = glyph(*p, &w);
            for (int r = 0; r < 9; r++) {
                for (int b = 0; b < w; b++) {
                    if (!(rows[r] & (1 << b))) continue;
                    if (pass == 0) {
                        for (int dy = -1; dy <= 1; dy++)
                            for (int dx = -1; dx <= 1; dx++)
                                plot(s, cx + b + dx, y + r + dy, outline);
                    } else {
                        plot(s, cx + b, y + r, color);
                    }
                }
            }
            cx += w + 1;
        }
    }
    upload(s);
}

void strip_print_num(TextStrip *s, u32 value, int y, int color) {
    char buf[12];
    int n = 0;
    char tmp[12];
    do {
        tmp[n++] = (char)('0' + value % 10);
        value /= 10;
    } while (value && n < 11);
    for (int i = 0; i < n; i++) buf[i] = tmp[n - 1 - i];
    buf[n] = 0;
    strip_print(s, buf, 0, y, color, 0);
}
