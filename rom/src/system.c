#include "system.h"

#include <stdarg.h>

extern void irq_handler(void);

volatile u32 vbl_count;            /* counted by the IRQ handler */

void system_init(void) {
    REG_WAITCNT = 0x4317;          /* SRAM 8 cycles, ROM 3/1 with prefetch */
    REG_IME = 0;
    IRQ_VECTOR = irq_handler;
    REG_DISPSTAT = DSTAT_VBL_IRQ;
    REG_IE = IRQ_VBLANK;
    REG_IF = 0xFFFF;
    REG_IME = 1;
}

/* ---------------------------------------------------------------- input */
static u16 keys_now, keys_before;

void input_poll(void) {
    keys_before = keys_now;
    keys_now = ~REG_KEYINPUT & KEY_ANY;
}

u16 key_held(void) { return keys_now; }
u16 key_hit(void) { return keys_now & ~keys_before; }

/* -------------------------------------------------------------- sprites */
ObjAttr oam[128];
ObjAffine *const affine = (ObjAffine *)oam;

void oam_hide_all(void) {
    for (int i = 0; i < 128; i++) {
        oam[i].attr0 = A0_HIDE;
        oam[i].attr1 = 0;
        oam[i].attr2 = 0;
    }
    for (int i = 0; i < 32; i++) affine_scale(i, 256, 256);
}

void oam_commit(void) {
    dma3_copy32((void *)MEM_OAM, oam, sizeof oam);
}

void affine_scale(int n, int sx, int sy) {
    if (sx < 16) sx = 16;
    if (sy < 16) sy = 16;
    affine[n].pa = (s16)((256 * 256) / sx);
    affine[n].pb = 0;
    affine[n].pc = 0;
    affine[n].pd = (s16)((256 * 256) / sy);
}

void affine_rot_scale(int n, int angle, int sx, int sy) {
    extern const s16 sin64[64];
    if (sx < 16) sx = 16;
    if (sy < 16) sy = 16;
    /* sin64 has 64 steps; interpolate for 256 */
    int i = (angle >> 2) & 63, f = angle & 3;
    int s = sin64[i] + ((sin64[(i + 1) & 63] - sin64[i]) * f) / 4;
    int j = (i + 16) & 63;
    int c = sin64[j] + ((sin64[(j + 1) & 63] - sin64[j]) * f) / 4;
    affine[n].pa = (s16)(c * 256 / sx);
    affine[n].pb = (s16)(-s * 256 / sx);
    affine[n].pc = (s16)(s * 256 / sy);
    affine[n].pd = (s16)(c * 256 / sy);
}

/* ---------------------------------------------------------------- debug */
#define MGBA_DEBUG_ENABLE (*(vu16 *)0x04FFF780)
#define MGBA_DEBUG_FLAGS  (*(vu16 *)0x04FFF700)
#define MGBA_DEBUG_STRING ((char *)0x04FFF600)

bool is_mgba(void) {
    MGBA_DEBUG_ENABLE = 0xC0DE;
    return MGBA_DEBUG_ENABLE == 0x1DEA;
}

static int put_uint(char *out, int n, int max, u32 v, int base) {
    char tmp[12];
    int k = 0;
    do {
        int d = (int)(v % (u32)base);
        tmp[k++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= (u32)base;
    } while (v);
    while (k && n < max) out[n++] = tmp[--k];
    return n;
}

void dbg(const char *fmt, ...) {
    static int checked, enabled;
    if (!checked) {
        enabled = is_mgba();
        checked = 1;
    }
    if (!enabled) return;
    char buf[200];
    int n = 0, max = (int)sizeof buf - 1;
    va_list ap;
    va_start(ap, fmt);
    for (const char *p = fmt; *p && n < max; p++) {
        if (*p != '%') {
            buf[n++] = *p;
            continue;
        }
        p++;
        if (*p == 'd') {
            int v = va_arg(ap, int);
            if (v < 0 && n < max) {
                buf[n++] = '-';
                v = -v;
            }
            n = put_uint(buf, n, max, (u32)v, 10);
        } else if (*p == 'u') {
            n = put_uint(buf, n, max, va_arg(ap, u32), 10);
        } else if (*p == 'x') {
            n = put_uint(buf, n, max, va_arg(ap, u32), 16);
        } else if (*p == 's') {
            for (const char *s = va_arg(ap, const char *); *s && n < max; s++) buf[n++] = *s;
        } else if (*p) {
            buf[n++] = *p;
        } else {
            break;
        }
    }
    va_end(ap);
    buf[n] = 0;
    for (int i = 0; i <= n; i++) MGBA_DEBUG_STRING[i] = buf[i];
    MGBA_DEBUG_FLAGS = 0x100 | 3;
}
