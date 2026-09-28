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
void sfx_chime(int step);         /* soft bell, step selects the note */
void sfx_tick(void);              /* tiny click */
void sfx_blip(void);              /* low soft blip */
void sfx_pop(void);               /* container pops open: soft burst */
void song_play(const Note *notes, int count);   /* melody on channel 2 */

extern const Note tune_hello[];
extern const int tune_hello_len;
extern const Note tune_pop[];
extern const int tune_pop_len;

#endif
