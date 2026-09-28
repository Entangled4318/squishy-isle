#include "save.h"

#include <stddef.h>
#include <string.h>

#define SAVE_MAGIC 0x53495153u          /* "SQIS" */
#define SLOT_SIZE  0x100

/* Emulators and flash carts look for this string to pick the save type. */
const char save_type_id[16] __attribute__((section(".rodata.keep"), aligned(4), used)) = "SRAM_V113";

static int active_slot = -1;

static u32 checksum(const SaveData *d) {
    const u8 *p = (const u8 *)d;
    u32 sum = 0x1234ABCDu;
    for (u32 i = 0; i < offsetof(SaveData, checksum); i++)
        sum = (sum << 5) + (sum >> 27) + p[i];
    return sum;
}

static void sram_read(int slot, SaveData *d) {
    u8 *p = (u8 *)d;
    vu8 *s = SRAM + slot * SLOT_SIZE;
    for (u32 i = 0; i < sizeof *d; i++) p[i] = s[i];
}

static void sram_write(int slot, const SaveData *d) {
    const u8 *p = (const u8 *)d;
    vu8 *s = SRAM + slot * SLOT_SIZE;
    for (u32 i = 0; i < sizeof *d; i++) s[i] = p[i];
}

static bool valid(const SaveData *d) {
    return d->magic == SAVE_MAGIC && d->version == SAVE_VERSION && d->checksum == checksum(d);
}

SaveStatus save_load(SaveData *out) {
    SaveData a, b;
    (void)*(volatile const char *)save_type_id;
    sram_read(0, &a);
    sram_read(1, &b);
    bool va = valid(&a), vb = valid(&b);
    if (va && (!vb || (s16)(a.seq - b.seq) > 0)) {
        *out = a;
        active_slot = 0;
        return SAVE_LOADED;
    }
    if (vb) {
        *out = b;
        active_slot = 1;
        return SAVE_LOADED;
    }
    bool blank = (a.magic == 0xFFFFFFFFu || a.magic == 0) && (b.magic == 0xFFFFFFFFu || b.magic == 0);
    memset(out, 0, sizeof *out);
    out->magic = SAVE_MAGIC;
    out->version = SAVE_VERSION;
    active_slot = -1;
    return blank ? SAVE_NEW : SAVE_BROKEN;
}

bool save_store(SaveData *d) {
    int slot = active_slot == 0 ? 1 : 0;
    d->magic = SAVE_MAGIC;
    d->version = SAVE_VERSION;
    d->seq++;
    d->checksum = checksum(d);
    sram_write(slot, d);
    SaveData check;
    sram_read(slot, &check);
    if (memcmp(&check, d, sizeof check) != 0) return false;
    active_slot = slot;
    return true;
}
