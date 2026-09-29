/* Boot setup, input, sprite shadow buffer and debug output. */
#ifndef SYSTEM_H
#define SYSTEM_H
#include "gba.h"

void system_init(void);
extern volatile u32 vbl_count;      /* VBlanks since power-on (IRQ handler) */

/* input (call input_poll once per frame) */
void input_poll(void);
u16 key_held(void);
u16 key_hit(void);
void input_block(bool on);         /* true: key_hit() reports nothing (scene fades) */

/* sprites: edit oam[] and affine[], then oam_commit() right after vblank */
extern ObjAttr oam[128];
extern ObjAffine *const affine;
void oam_hide_all(void);
void oam_commit(void);
void affine_scale(int n, int sx, int sy);   /* 8.8 fixed, 256 = 1.0 */
void affine_rot_scale(int n, int angle, int sx, int sy);   /* angle: 256 steps per turn */

/* debug output to the mGBA log; is_mgba() tells which emulator runs us */
bool is_mgba(void);
void dbg(const char *fmt, ...);

#endif
