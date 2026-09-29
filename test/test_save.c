/* Host unit test for the two-slot battery save (rom/src/save.c), step 7.1.
 * SRAM is a plain array here. A power cut in the middle of a write is
 * modelled by writing only the first k bytes of the new slot, for every k:
 * the next boot must load either the save before or the new one, never
 * lose both. Also: the sequence number wrapping, a flipped bit, both slots
 * broken, and a blank chip. */
#include "../rom/src/gba.h"
#undef SRAM
static u8 fake_sram[0x8000];
#define SRAM ((vu8 *)fake_sram)
#include "../rom/src/save.c"

#include <stdio.h>
#include <stdlib.h>

static int fails;
#define CHECK(c, ...) do { printf("%s ", (c) ? "PASS" : "FAIL"); printf(__VA_ARGS__); printf("\n"); if (!(c)) fails++; } while (0)

/* a save with recognisable contents: found[] and opens carry the stamp */
static void stamp(SaveData *d, u32 n) {
    for (int i = 0; i < NUM_FRIENDS; i++) d->found[i] = (u8)((n + i) % 2);
    d->opens = n;
    d->rng = n * 2654435761u;
    d->area = (u8)(n % 4);
}

static bool same(const SaveData *a, const SaveData *b) {
    return a->opens == b->opens && !memcmp(a->found, b->found, sizeof a->found) && a->rng == b->rng &&
           a->area == b->area;
}

int main(void) {
    SaveData d, got;

    memset(fake_sram, 0xFF, sizeof fake_sram);
    CHECK(save_load(&got) == SAVE_NEW && got.magic && got.opens == 0, "a blank chip (0xFF) starts a new save");
    memset(fake_sram, 0, sizeof fake_sram);
    CHECK(save_load(&got) == SAVE_NEW, "a zeroed chip starts a new save");

    /* many saves in a row: each boot loads the newest */
    memset(fake_sram, 0xFF, sizeof fake_sram);
    save_load(&d);
    int wrong = 0;
    for (u32 n = 1; n <= 50; n++) {
        stamp(&d, n);
        save_store(&d);
        if (save_load(&got) != SAVE_LOADED || !same(&got, &d)) wrong++;
    }
    CHECK(wrong == 0, "50 saves in a row: every boot loads the newest (%d wrong)", wrong);

    /* power cut after k of the 144 bytes of the new slot, for every k, over several saves
     * (so both slots get torn): the boot finds the old save or the new one */
    int lost = 0, old_ok = 0, new_ok = 0;
    for (u32 n = 100; n < 104; n++) {
        for (u32 k = 0; k <= sizeof(SaveData); k++) {
            SaveData before, after;
            save_load(&before);                       /* the save on the chip now */
            u8 chip[2 * SLOT_SIZE];
            memcpy(chip, fake_sram, sizeof chip);
            after = before;
            stamp(&after, n * 1000 + k);
            save_store(&after);                       /* what a full write puts there */
            int slot = active_slot;
            u8 full[SLOT_SIZE];
            memcpy(full, fake_sram + slot * SLOT_SIZE, SLOT_SIZE);
            memcpy(fake_sram, chip, sizeof chip);     /* undo, then write only k bytes */
            memcpy(fake_sram + slot * SLOT_SIZE, full, k);
            SaveStatus st = save_load(&got);
            if (st != SAVE_LOADED) lost++;
            else if (same(&got, &after)) new_ok++;
            else if (same(&got, &before)) old_ok++;
            else lost++;
            if (k == sizeof(SaveData) && !same(&got, &after)) lost++;
        }
        save_load(&d);
        stamp(&d, n);
        save_store(&d);                                /* a clean save between rounds */
    }
    CHECK(lost == 0, "power cut at every byte of a write (4 x 145 cuts): old save %d, new save %d, lost %d",
          old_ok, new_ok, lost);

    /* the sequence number wraps from 65535 to 0 */
    memset(fake_sram, 0xFF, sizeof fake_sram);
    save_load(&d);
    d.seq = 0xFFF8;                                   /* both slots get written near the wrap first */
    wrong = 0;
    for (u32 n = 1; n <= 8; n++) {
        stamp(&d, 7000 + n);
        save_store(&d);
        if (save_load(&got) != SAVE_LOADED || !same(&got, &d)) wrong++;
    }
    CHECK(wrong == 0 && got.seq < 10, "the save counter wraps past 65535 and the newest still wins (seq now %u)", got.seq);

    /* a flipped bit in the newest slot: the other slot loads */
    save_load(&d);
    SaveData older = d;
    stamp(&d, 9001);
    save_store(&d);
    fake_sram[active_slot * SLOT_SIZE + 40] ^= 0x10;
    CHECK(save_load(&got) == SAVE_LOADED && same(&got, &older), "a flipped bit in the newest slot: the slot before loads");

    /* both slots broken: a fresh save, reported as a reset */
    fake_sram[0 * SLOT_SIZE + 20] ^= 1;
    fake_sram[1 * SLOT_SIZE + 20] ^= 1;
    SaveStatus st = save_load(&got);
    CHECK(st == SAVE_BROKEN && got.opens == 0, "both slots broken: starts fresh, reported as a reset (status %d)", st);

    /* a save of an older layout version is not loaded as this one */
    memset(fake_sram, 0xFF, sizeof fake_sram);
    save_load(&d);
    stamp(&d, 5);
    save_store(&d);
    SaveData *s = (SaveData *)(fake_sram + active_slot * SLOT_SIZE);
    s->version = 1;
    s->checksum = checksum(s);
    CHECK(save_load(&got) != SAVE_LOADED, "a version 1 save is not read as version 2");

    /* a save written before step 7 (rotate-add sum) still loads */
    memset(fake_sram, 0xFF, sizeof fake_sram);
    save_load(&d);
    stamp(&d, 77);
    save_store(&d);
    s = (SaveData *)(fake_sram + active_slot * SLOT_SIZE);
    s->checksum = checksum_old(s);
    CHECK(save_load(&got) == SAVE_LOADED && same(&got, &d), "a save with the old checksum still loads");
    CHECK(s->checksum != checksum(s), "new saves use the CRC-32, not the old sum");

    if (fails) { printf("%d save check(s) FAILED\n", fails); return 1; }
    printf("all save unit checks passed\n");
    return 0;
}
