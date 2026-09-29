/* PSG sound effects and a tiny per-frame note sequencer. */
#ifndef SOUND_H
#define SOUND_H
#include "gba.h"

typedef struct {
    u16 hz;        /* 0 = rest */
    u8 frames;     /* length in frames */
    u8 vol;        /* envelope start volume 0-15 */
} Note;

void sound_init(void);
void sound_tick(void);            /* call once per frame */

void sfx_squeak(int pitch);       /* squish: rising chirp, pitch 0..4 */
void sfx_boing(void);             /* hop: up then down slide */
void sfx_hop(void);               /* Pip's hop (B on the map): a soft quick rise on channel 1 */
void sfx_chime(int step);         /* soft bell, step selects the note */
void sfx_tick(void);              /* tiny click */
void sfx_blip(void);              /* low soft blip */
void sfx_pop(void);               /* container pops open: soft burst */
void song_play(const Note *notes, int count);   /* jingle on channel 3: pauses the music */

/* Looping 2-voice music: lead on the wave channel (3), bass on square
 * channel 2. Data comes from tools/music.py (music_data.h). Effects keep
 * channels 1 and 4; the boing borrows channel 2 from the bass, and a
 * jingle pauses the song, which then goes on where it stopped. */
typedef struct {
    const char *title;
    const u8 *lead, *bass;       /* pairs: note (0 = rest, MIDI 1..127), length in ticks */
    u16 lead_n, bass_n;          /* number of notes */
    const u32 *wave;             /* 32 x 4-bit wave for the lead */
    u8 tick;                     /* frames per tick */
    u8 loop;                     /* 1 = loops, 0 = plays once */
    u8 lead_level;               /* 0 = 100%, 1 = 75%, 2 = 50%, 3 = 25% */
    u8 lead_decay;               /* frames per step down to 25%, 0 = hold */
    u8 lead_gap;                 /* silent frames at the end of each note */
    u8 bass_vol, bass_duty, bass_step, bass_gap;   /* square envelope, duty 0..2, fade step */
} Song;

void music_play(int song);        /* no restart when that song already plays; a looping song goes on where it was left */
void music_play_from_start(int song);   /* always from bar 1 (sound test) */
void music_stop(void);
int music_current(void);          /* song index, -1 = none */
extern bool music_mute;           /* true: the song keeps time but makes no sound (tests) */

extern const Note tune_hello[];
extern const int tune_hello_len;
extern const Note tune_pop[];
extern const int tune_pop_len;

#endif
