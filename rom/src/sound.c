/* PSG sound: channel 1 short effects, channel 2 music bass (the boing
 * borrows it), channel 3 (wave) music lead and jingles, channel 4 clicks. Everything is soft by default: a toddler plays
 * this, and the handheld's own volume goes up if needed. */
#include "sound.h"

#include "music_data.h"
#include "system.h"

#define SQ_RATE(hz)   (2048 - 131072 / (hz))
#define WAVE_RATE(hz) (2048 - 65536 / (hz))

/* envelope: start volume, step time, direction 0 = fade out */
#define ENV(vol, step)  (((vol) << 12) | ((step) << 8))
#define DUTY_12 (0 << 6)
#define DUTY_25 (1 << 6)
#define DUTY_50 (2 << 6)
#define RESTART 0x8000


static struct {
    const Note *notes;
    int count, index, timer;
} melody;

static int boing_t = -1;
static const u32 *wave_now;

/* Wave RAM can only be written to the bank that is not playing. The
 * channel stops for this, so change waves only between songs. */
static void load_wave(const u32 *w) {
    if (w == wave_now) return;
    wave_now = w;
    REG_SND3CNT = 0;
    REG_SND3SEL = 0x40;                        /* play bank 1, write bank 0 */
    for (int i = 0; i < 4; i++) WAVE_RAM[i] = w[i];
    REG_SND3SEL = 0x80;                        /* enable, play bank 0 */
}
static int squeak_t = -1, squeak_pitch;

void sound_init(void) {
    REG_SNDSTAT = 0x80;                        /* master on */
    REG_SNDDMGCNT = 0xFF66;                    /* all channels L+R, volume 6/7 */
    REG_SNDDSCNT = 0x0002;                     /* PSG at 100% */
    REG_SNDBIAS = 0x0200;
    REG_SND1SWEEP = 0x0008;                    /* sweep off */
    load_wave(music_wave_sine);
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

void sfx_pop(void) {
    REG_SND4CNT = ENV(11, 2);
    REG_SND4FREQ = RESTART | (4 << 4) | 2;     /* soft low noise puff */
    sfx_chime(4);
}

void sfx_boing(void) {
    boing_t = 0;
    REG_SND2CNT = DUTY_50 | ENV(11, 2);
    REG_SND2FREQ = RESTART | SQ_RATE(262);
}

static void music_silence(void);

void song_play(const Note *notes, int count) {
    music_silence();
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

/* ---- music ---- */
/* wave volume by level: 0 = 100%, 1 = 75%, 2 = 50%, 3 = 25%, 4 = off.
 * PSG channels only output positive values, so switching a voice hard on
 * or off makes a click. The lead fades in and out one level per frame
 * and changes pitch without a restart while it sounds. */
static const u16 wave_vol[5] = {0x2000, 0x8000, 0x4000, 0x6000, 0};

typedef struct {
    const u8 *ev;
    int n, index;
    int left;        /* frames left in the current note */
    int len;         /* frames of the current note */
    int age;         /* frames since the note started */
    int note;        /* current MIDI note, 0 = rest */
    int level;       /* lead: volume level now (4 = off) */
} VoiceState;

static struct {
    const Song *song;
    int id;
    VoiceState lead, bass;     /* a voice cut by a jingle or the boing waits for its next note */
} mus = {0, -1, {0}, {0}};

bool music_mute;

static void voice_start(VoiceState *v, const u8 *ev, int n) {
    v->ev = ev;
    v->n = n;
    v->index = -1;
    v->left = 0;
    v->level = 4;
}

void music_play(int id) {
    if (id == mus.id && mus.song) return;
    music_stop();
    if (id < 0 || id >= SONG_COUNT) return;
    mus.song = &songs[id];
    mus.id = id;
    load_wave(mus.song->wave);
    voice_start(&mus.lead, mus.song->lead, mus.song->lead_n);
    voice_start(&mus.bass, mus.song->bass, mus.song->bass_n);
    dbg("song start %d", id);
}

void music_stop(void) {
    if (mus.song) {
        dbg("song stop %d", mus.id);
        if (!melody.notes) REG_SND3CNT = 0;
        if (boing_t < 0) {
            REG_SND2CNT = 0;                                  /* volume 0: silent */
            REG_SND2FREQ = RESTART;
        }
    }
    mus.song = 0;
    mus.id = -1;
}

int music_current(void) { return mus.song ? mus.id : -1; }

static void music_silence(void) {
    if (!mus.song) return;
    REG_SND2CNT = 0;
    REG_SND2FREQ = RESTART;
}

/* next note of a voice; false when a one-shot song has ended */
static bool voice_next(VoiceState *v) {
    if (++v->index >= v->n) {
        if (!mus.song->loop) return false;
        v->index = 0;
        if (v == &mus.lead) dbg("music loop %d", mus.id);
    }
    v->note = v->ev[v->index * 2];
    v->len = v->left = v->ev[v->index * 2 + 1] * mus.song->tick;
    v->age = 0;
    return true;
}

static void lead_frame(void) {
    const Song *s = mus.song;
    VoiceState *v = &mus.lead;
    if (melody.notes || music_mute) {           /* the channel is busy or muted */
        v->level = 4;
        return;
    }
    int lv = 4;
    if (v->note) {
        lv = s->lead_level;
        if (s->lead_decay) lv += v->age / s->lead_decay;
        int attack = s->lead_level + 2 - v->age;               /* starts 2 levels quieter */
        if (attack > lv) lv = attack;
        if (v->left <= s->lead_gap) {                          /* fades out over the gap */
            int release = s->lead_level + s->lead_gap - v->left + 1;
            if (release > lv) lv = release;
        }
        if (lv > 3) lv = 3;
        if (v->age == 0) {
            u16 rate = music_wave_rate[v->note];
            if (v->level == 4) {
                REG_SND3CNT = wave_vol[lv];                    /* from silence: restart is safe */
                REG_SND3FREQ = RESTART | rate;
            } else {
                REG_SND3FREQ = rate;                           /* still sounding: glide on */
            }
        }
    }
    if (lv != v->level) {
        if (lv > v->level + 1) lv = v->level + 1;           /* down by at most 1 level a frame */
        REG_SND3CNT = wave_vol[lv];
        v->level = lv;
    }
}

static void bass_frame(void) {
    const Song *s = mus.song;
    VoiceState *v = &mus.bass;
    if (boing_t >= 0 || melody.notes || music_mute) return;
    if (v->age == 0) {
        if (v->note) {
            REG_SND2CNT = (u16)((s->bass_duty << 6) | ENV(s->bass_vol, s->bass_step));
            REG_SND2FREQ = RESTART | music_sq_rate[v->note];
        } else {
            REG_SND2CNT = 0;
            REG_SND2FREQ = RESTART;
        }
    } else if (v->note && s->bass_gap && v->left == s->bass_gap) {
        REG_SND2CNT = 0;
        REG_SND2FREQ = RESTART;
    }
}

static void music_tick(void) {
    if (!mus.song) return;
    if (melody.notes) return;                   /* a jingle plays: the song waits */
    if (mus.lead.left == 0 && !voice_next(&mus.lead)) { music_stop(); return; }
    if (mus.bass.left == 0 && !voice_next(&mus.bass)) { music_stop(); return; }
    lead_frame();
    bass_frame();
    mus.lead.age++;
    mus.lead.left--;
    mus.bass.age++;
    mus.bass.left--;
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
    music_tick();
    melody_tick();
    boing_tick();
    squeak_tick();
}

/* C5 E5 G5 C6 . G5 C6: a bright "hello" */
const Note tune_hello[] = {
    {523, 8, 12}, {659, 8, 12}, {784, 8, 12}, {1047, 14, 12}, {0, 4, 0}, {784, 8, 10}, {1047, 20, 12},
};
const int tune_hello_len = sizeof tune_hello / sizeof tune_hello[0];

/* after the pop: a little rising "ta-da" */
const Note tune_pop[] = {
    {784, 5, 11}, {988, 5, 11}, {1175, 5, 11}, {1568, 16, 12},
};
const int tune_pop_len = sizeof tune_pop / sizeof tune_pop[0];
