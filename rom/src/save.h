/* Battery save (32 KB SRAM) with two alternating slots.
 * Each slot has a sequence number and checksum, so a power cut in the
 * middle of a write never loses the previous good save. */
#ifndef SAVE_H
#define SAVE_H
#include "gba.h"

#define SAVE_VERSION 2

#define NUM_FRIENDS   80      /* 16 species x 5 flavors, id = species * 5 + flavor */
#define MAX_FOLLOWERS 3

typedef struct {
    u32 magic;
    u16 version;
    u16 seq;
    u16 boots;
    u16 flavor;
    u32 squishes;
    /* collection: 0 = not found, 1 = found (each friend comes only once) */
    u8 found[NUM_FRIENDS];
    u32 opens;                /* containers opened, all areas */
    u32 rng;                  /* roll state, kept so every boot rolls differently */
    u8 followers[MAX_FOLLOWERS];  /* friend id + 1, 0 = empty; [0] walks nearest Pip */
    u8 pad;
    u16 pip_x, pip_y;         /* position (feet) in `area` for Continue; 0,0 = the area's start */
    u8 area;                  /* area Pip is in: 0 meadow, 1 woods, 2 shore, 3 cloud hill */
    u8 gates;                 /* bit n set: the way into area n + 1 is built (its scene has played) */
    u8 lines[3][MAX_FOLLOWERS];   /* follower lines of areas 1..3 (the meadow's is `followers`) */
    u8 mail;                  /* newest friend id + 1: the letter in the mailbox (0 = none yet) */
    u8 mail_new;              /* 1: not read yet, the mailbox flag is up */
    u8 parades;               /* bit n: area n's full-page parade has played */
    u8 pad2[2];
    u32 reserved[3];
    u32 checksum;
} SaveData;

_Static_assert(sizeof(SaveData) == 144, "save layout: older saves must still load");
_Static_assert(__builtin_offsetof(SaveData, area) == 112, "test/harness.c reads the area here (SAVE_AREA)");

typedef enum { SAVE_NEW, SAVE_LOADED, SAVE_BROKEN } SaveStatus;

SaveStatus save_load(SaveData *out);   /* fills defaults when nothing valid */
bool save_store(SaveData *d);          /* writes the other slot, verifies it */

#endif
