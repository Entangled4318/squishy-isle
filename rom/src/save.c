#include "save.h"

#include <stddef.h>
#include <string.h>

#define SAVE_MAGIC 0x53495153u          /* "SQIS" */
#define SLOT_SIZE  0x100

/* Emulators and flash carts look for this string to pick the save type. */
const char save_type_id[16] __attribute__((section(".rodata.keep"), aligned(4), used)) = "SRAM_V113";

_Static_assert(sizeof(SaveData) <= SLOT_SIZE, "SaveData must fit one slot");

static int active_slot = -1;

/* CRC-32 (the zlib one) over everything before the checksum field, 4 bits
 * a step from a 16-entry table: bit by bit it was slow enough to push the
 * title a frame later at boot (a test timing trap, see HANDOFF.md). */
static const u32 crc_nibble[16] = {
    0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
    0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C, 0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C};

static u32 checksum(const SaveData *d) {
    const u8 *p = (const u8 *)d;
    u32 c = 0xFFFFFFFFu;
    for (u32 i = 0; i < offsetof(SaveData, checksum); i++) {
        c ^= p[i];
        c = (c >> 4) ^ crc_nibble[c & 15];
        c = (c >> 4) ^ crc_nibble[c & 15];
    }
    return ~c;
}

/* The rotate-add sum saves used before step 7. Still read, so an older
 * save loads; the next write uses the CRC. (Two bit flips 32 bytes apart
 * can cancel out in it, which the CRC catches.) */
static u32 checksum_old(const SaveData *d) {
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
    return d->magic == SAVE_MAGIC && d->version == SAVE_VERSION &&
           (d->checksum == checksum(d) || d->checksum == checksum_old(d));
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
