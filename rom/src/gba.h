/* Squishy Isle: GBA hardware definitions. */
#ifndef GBA_H
#define GBA_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;

#define IWRAM_CODE __attribute__((section(".iwram"), long_call, target("arm")))
#define EWRAM_DATA __attribute__((section(".ewram")))
#define ALIGN4 __attribute__((aligned(4)))

#define SCREEN_W 240
#define SCREEN_H 160

/* ---- memory ---- */
#define MEM_IO    0x04000000
#define MEM_PAL   0x05000000
#define MEM_VRAM  0x06000000
#define MEM_OAM   0x07000000
#define MEM_SRAM  0x0E000000

#define PAL_BG    ((vu16 *)MEM_PAL)
#define PAL_OBJ   ((vu16 *)(MEM_PAL + 0x200))
#define CHARBLOCK(n)   ((vu16 *)(MEM_VRAM + (n) * 0x4000))
#define SCREENBLOCK(n) ((vu16 *)(MEM_VRAM + (n) * 0x800))
#define OBJ_TILES ((vu16 *)(MEM_VRAM + 0x10000))
#define SRAM      ((vu8 *)MEM_SRAM)

/* ---- display ---- */
#define REG_DISPCNT   (*(vu16 *)(MEM_IO + 0x000))
#define REG_DISPSTAT  (*(vu16 *)(MEM_IO + 0x004))
#define REG_VCOUNT    (*(vu16 *)(MEM_IO + 0x006))
#define REG_BGCNT(n)  (*(vu16 *)(MEM_IO + 0x008 + 2 * (n)))
#define REG_BGHOFS(n) (*(vu16 *)(MEM_IO + 0x010 + 4 * (n)))
#define REG_BGVOFS(n) (*(vu16 *)(MEM_IO + 0x012 + 4 * (n)))
#define REG_BLDCNT    (*(vu16 *)(MEM_IO + 0x050))
#define REG_BLDALPHA  (*(vu16 *)(MEM_IO + 0x052))
#define REG_BLDY      (*(vu16 *)(MEM_IO + 0x054))

#define DCNT_MODE0    0x0000
#define DCNT_MODE1    0x0001
#define DCNT_OBJ_1D   0x0040
#define DCNT_BLANK    0x0080
#define DCNT_BG0      0x0100
#define DCNT_BG1      0x0200
#define DCNT_BG2      0x0400
#define DCNT_BG3      0x0800
#define DCNT_OBJ      0x1000

#define DSTAT_VBL_IRQ 0x0008

#define BG_PRIO(n)    (n)
#define BG_CBB(n)     ((n) << 2)
#define BG_SBB(n)     ((n) << 8)
#define BG_4BPP       0
#define BG_REG_32x32  0

#define BLD_BG0 0x01
#define BLD_BG1 0x02
#define BLD_BG2 0x04
#define BLD_BG3 0x08
#define BLD_OBJ 0x10
#define BLD_BD  0x20
#define BLD_ALPHA 0x40
#define BLD_WHITE 0x80
#define BLD_BLACK 0xC0

/* ---- sprites ---- */
typedef struct {
    u16 attr0, attr1, attr2;
    s16 fill;
} ALIGN4 ObjAttr;

typedef struct {
    u16 f0[3]; s16 pa;
    u16 f1[3]; s16 pb;
    u16 f2[3]; s16 pc;
    u16 f3[3]; s16 pd;
} ALIGN4 ObjAffine;

#define OAM ((volatile ObjAttr *)MEM_OAM)

#define A0_Y(y)        ((y) & 0xFF)
#define A0_AFFINE      0x0100
#define A0_HIDE        0x0200
#define A0_DOUBLE      0x0200
#define A0_BLEND       0x0400
#define A0_SQUARE      0x0000
#define A0_WIDE        0x4000
#define A0_TALL        0x8000
#define A1_X(x)        ((x) & 0x1FF)
#define A1_AFF(n)      ((n) << 9)
#define A1_HFLIP       0x1000
#define A1_VFLIP       0x2000
#define A1_SIZE(n)     ((n) << 14)
#define A2_TILE(n)     ((n) & 0x3FF)
#define A2_PRIO(n)     ((n) << 10)
#define A2_PAL(n)      ((n) << 12)

/* ---- sound ---- */
#define REG_SND1SWEEP (*(vu16 *)(MEM_IO + 0x060))
#define REG_SND1CNT   (*(vu16 *)(MEM_IO + 0x062))
#define REG_SND1FREQ  (*(vu16 *)(MEM_IO + 0x064))
#define REG_SND2CNT   (*(vu16 *)(MEM_IO + 0x068))
#define REG_SND2FREQ  (*(vu16 *)(MEM_IO + 0x06C))
#define REG_SND3SEL   (*(vu16 *)(MEM_IO + 0x070))
#define REG_SND3CNT   (*(vu16 *)(MEM_IO + 0x072))
#define REG_SND3FREQ  (*(vu16 *)(MEM_IO + 0x074))
#define REG_SND4CNT   (*(vu16 *)(MEM_IO + 0x078))
#define REG_SND4FREQ  (*(vu16 *)(MEM_IO + 0x07C))
#define REG_SNDDMGCNT (*(vu16 *)(MEM_IO + 0x080))
#define REG_SNDDSCNT  (*(vu16 *)(MEM_IO + 0x082))
#define REG_SNDSTAT   (*(vu16 *)(MEM_IO + 0x084))
#define REG_SNDBIAS   (*(vu16 *)(MEM_IO + 0x088))
#define WAVE_RAM      ((vu32 *)(MEM_IO + 0x090))

/* ---- DMA ---- */
#define REG_DMA3SAD   (*(vu32 *)(MEM_IO + 0x0D4))
#define REG_DMA3DAD   (*(vu32 *)(MEM_IO + 0x0D8))
#define REG_DMA3CNT   (*(vu32 *)(MEM_IO + 0x0DC))
#define DMA_ENABLE    0x80000000
#define DMA_32        0x04000000
#define DMA_16        0x00000000

/* ---- input ---- */
#define REG_KEYINPUT  (*(vu16 *)(MEM_IO + 0x130))
#define KEY_A      0x0001
#define KEY_B      0x0002
#define KEY_SELECT 0x0004
#define KEY_START  0x0008
#define KEY_RIGHT  0x0010
#define KEY_LEFT   0x0020
#define KEY_UP     0x0040
#define KEY_DOWN   0x0080
#define KEY_R      0x0100
#define KEY_L      0x0200
#define KEY_ANY    0x03FF

/* ---- interrupts / system ---- */
#define REG_IE        (*(vu16 *)(MEM_IO + 0x200))
#define REG_IF        (*(vu16 *)(MEM_IO + 0x202))
#define REG_WAITCNT   (*(vu16 *)(MEM_IO + 0x204))
#define REG_IME       (*(vu16 *)(MEM_IO + 0x208))
#define IRQ_VBLANK    0x0001
#define IRQ_VECTOR    (*(void (**)(void))0x03007FFC)

static inline void vblank_wait(void) {
    __asm__ volatile("swi 0x05" ::: "r0", "r1", "r2", "r3", "memory");
}

static inline void dma3_copy32(volatile void *dst, const void *src, u32 bytes) {
    REG_DMA3SAD = (u32)src;
    REG_DMA3DAD = (u32)dst;
    REG_DMA3CNT = DMA_ENABLE | DMA_32 | (bytes >> 2);
}

static inline void dma3_copy16(volatile void *dst, const void *src, u32 bytes) {
    REG_DMA3SAD = (u32)src;
    REG_DMA3DAD = (u32)dst;
    REG_DMA3CNT = DMA_ENABLE | DMA_16 | (bytes >> 1);
}

#endif
