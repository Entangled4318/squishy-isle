/* Battery save (32 KB SRAM) with two alternating slots.
 * Each slot has a sequence number and checksum, so a power cut in the
 * middle of a write never loses the previous good save. */
#ifndef SAVE_H
#define SAVE_H
#include "gba.h"

#define SAVE_VERSION 1

typedef struct {
    u32 magic;
    u16 version;
    u16 seq;
    u16 boots;
    u16 flavor;
    u32 squishes;
    u32 reserved[8];
    u32 checksum;
} SaveData;

typedef enum { SAVE_NEW, SAVE_LOADED, SAVE_BROKEN } SaveStatus;

SaveStatus save_load(SaveData *out);   /* fills defaults when nothing valid */
bool save_store(SaveData *d);          /* writes the other slot, verifies it */

#endif
