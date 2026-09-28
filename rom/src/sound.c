/* PSG sound: channel 1 short effects, channel 2 boing, channel 3 (wave)
 * melody, channel 4 clicks. Everything is soft by default: a toddler plays
 * this, and the handheld's own volume goes up if needed. */
#include "sound.h"

#define SQ_RATE(hz)   (2048 - 131072 / (hz))
#define WAVE_RATE(hz) (2048 - 65536 / (hz))

/* envelope: start volume, step time, direction 0 = fade out */
#define ENV(vol, step)  (((vol) << 12) | ((step) << 8))
#define DUTY_12 (0 << 6)
#define DUTY_25 (1 << 6)
#define DUTY_50 (2 << 6)
#define RESTART 0x8000

/* soft sine-like wave for the melody channel (32 x 4-bit samples) */
static const u32 wave_soft[4] = {0xEFDEAC89, 0xA9DCEEFF, 0x10215386, 0x56231100};

static struct {
    const Note *notes;
    int count, index, timer;
} melody;

static int boing_t = -1;
static int squeak_t = -1, squeak_pitch;

void sound_init(void) {
    REG_SNDSTAT = 0x80;                        /* master on */
    REG_SNDDMGCNT = 0xFF66;                    /* all channels L+R, volume 6/7 */
    REG_SNDDSCNT = 0x0002;                     /* PSG at 100% */
    REG_SNDBIAS = 0x0200;
    REG_SND1SWEEP = 0x0008;                    /* sweep off */
    REG_SND3SEL = 0x40;                        /* play bank 1, write bank 0 */
    for (int i = 0; i < 4; i++) WAVE_RAM[i] = wave_soft[i];
    REG_SND3SEL = 0x80;                        /* enable, play bank 0 */
    REG_SND3CNT = 0;                           /* silent until a note */
}

/* "squee": quick rise, small fall. Each flavor squeaks a little higher. */
static const u16 squeak_curve[12] = {620, 760, 900, 1040, 1150, 1220, 1250, 1230, 1180, 1100, 1010, 930};

void sfx_squeak(int pitch) {
    squeak_t = 0;
    squeak_pitch = 256 + (pitch % 5) * 20;     /* 8.8 multiplier */
    REG_SND1SWEEP = 0x0008;                    /* hardware sweep off */
    REG_SND1CNT = DUTY_25 | ENV(11, 2);
    REG_SND1FREQ = RESTART | SQ_RATE((squeak_curve[0] * squeak_pitch) >> 8);
}

static void squeak_tick(void) {
    if (squeak_t < 0) return;
    if (++squeak_t >= 12) {
        squeak_t = -1;
        return;
    }
    REG_SND1FREQ = SQ_RATE((squeak_curve[squeak_t] * squeak_pitch) >> 8);   /* no restart */
}

void sfx_chime(int step) {
    static const u16 bell[5] = {1047, 1175, 1319, 1568, 1760};
    squeak_t = -1;
    REG_SND1SWEEP = 0x0008;
    REG_SND1CNT = DUTY_12 | ENV(10, 2);
    REG_SND1FREQ = RESTART | SQ_RATE(bell[step % 5]);
}

void sfx_blip(void) {
    squeak_t = -1;
    REG_SND1SWEEP = 0x0008;
    REG_SND1CNT = DUTY_50 | ENV(7, 1);
    REG_SND1FREQ = RESTART | SQ_RATE(392);
}

void sfx_tick(void) {
    REG_SND4CNT = ENV(4, 1) | 0x0000;
    REG_SND4FREQ = RESTART | (2 << 4) | 1;     /* bright short noise */
}

void sfx_boing(void) {
    boing_t = 0;
    REG_SND2CNT = DUTY_50 | ENV(11, 2);
    REG_SND2FREQ = RESTART | SQ_RATE(262);
}

void song_play(const Note *notes, int count) {
    melody.notes = notes;
    melody.count = count;
    melody.index = 0;
    melody.timer = 0;
}

static void melody_tick(void) {
    if (!melody.notes) return;
    const Note *n = &melody.notes[melody.index];
    if (melody.timer == 0) {
        if (n->hz) {
            REG_SND3CNT = (n->vol >= 8) ? 0x2000 : 0x4000;      /* 100% or 50% */
            REG_SND3FREQ = RESTART | WAVE_RATE(n->hz);
        } else {
            REG_SND3CNT = 0;
        }
    } else if (melody.timer == n->frames - 2 && n->hz) {
        REG_SND3CNT = 0x6000;                                  /* 25%: soft release */
    }
    if (++melody.timer >= n->frames) {
        melody.timer = 0;
        if (++melody.index >= melody.count) {
            melody.notes = 0;
            REG_SND3CNT = 0;
        }
    }
}

static void boing_tick(void) {
    /* 262 Hz up to ~520 Hz, then down to ~200 Hz over 16 frames */
    static const u16 curve[16] = {262, 330, 415, 494, 523, 494, 440, 392,
                                  349, 311, 277, 262, 247, 233, 220, 208};
    if (boing_t < 0) return;
    if (boing_t < 16) REG_SND2FREQ = SQ_RATE(curve[boing_t]);   /* no restart */
    if (++boing_t >= 16) boing_t = -1;
}

void sound_tick(void) {
    melody_tick();
    boing_tick();
    squeak_tick();
}

/* C5 E5 G5 C6 . G5 C6: a bright "hello" */
const Note tune_hello[] = {
    {523, 8, 12}, {659, 8, 12}, {784, 8, 12}, {1047, 14, 12}, {0, 4, 0}, {784, 8, 10}, {1047, 20, 12},
};
const int tune_hello_len = sizeof tune_hello / sizeof tune_hello[0];
