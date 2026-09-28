#include "collection.h"

#include <string.h>

SaveData game_save;

/* pure logic below save_*; test/test_collection.c builds this file on the host */

static u32 rng_next(void) {
    u32 x = game_save.rng ? game_save.rng : 0x2545F491u;   /* xorshift32 */
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    game_save.rng = x;
    return x;
}

bool friend_found(int id) { return game_save.found[id] != 0; }

int friend_hearts(int id) { return game_save.found[id] ? game_save.found[id] - 1 : 0; }

int found_in_area(int area) {
    int n = 0;
    for (int id = area * 20; id < area * 20 + 20; id++) n += friend_found(id);
    return n;
}

int found_total(void) {
    int n = 0;
    for (int a = 0; a < 4; a++) n += found_in_area(a);
    return n;
}

/* Sparkle is the rare flavor: weight 1 against 4 for the others. */
static int weight(int id) { return friend_flavor(id) == 4 ? 1 : 4; }

/* Picks from the area's friends; only_new limits it to friends not found yet. */
static int pick(int area, bool only_new, u32 r) {
    int total = 0;
    for (int id = area * 20; id < area * 20 + 20; id++)
        if (!only_new || !friend_found(id)) total += weight(id);
    if (total == 0) return -1;
    int t = (int)(r % (u32)total);
    for (int id = area * 20; id < area * 20 + 20; id++) {
        if (only_new && friend_found(id)) continue;
        t -= weight(id);
        if (t < 0) return id;
    }
    return -1;
}

int collection_roll(int area, u32 entropy) {
    game_save.rng ^= entropy * 0x9E3779B9u;
    u32 r = rng_next();
    u32 r2 = rng_next();
    int missing = 20 - found_in_area(area);
    /* the first friends of each area are always new; after that a new
     * friend 3 times in 4 while any are left */
    bool want_new = missing > 0 && (20 - missing < FIRST_NEW_OPENS || (r2 & 3) != 0);
    int id = pick(area, want_new, r);
    return id >= 0 ? id : pick(area, false, r);
}

GotResult collection_add(int id) {
    GotResult res;
    u8 *f = &game_save.found[id];
    if (*f == 0) {
        *f = 1;
        res = GOT_NEW;
    } else if (*f < 1 + MAX_HEARTS) {
        (*f)++;
        res = GOT_HEART;
    } else {
        res = GOT_REPEAT;
    }
    game_save.opens++;
    follower_add(id);
    collection_save();
    return res;
}

void follower_add(int id) {
    u8 *l = game_save.followers;
    int at = MAX_FOLLOWERS - 1;          /* drops the oldest unless id is already in line */
    for (int i = 0; i < MAX_FOLLOWERS; i++)
        if (l[i] == id + 1) at = i;
    for (int i = at; i > 0; i--) l[i] = l[i - 1];
    l[0] = (u8)(id + 1);
}

int follower_get(int i) { return game_save.followers[i] - 1; }

#ifndef HOST_TEST
#include "system.h"

void collection_init(void) {
    SaveStatus st = save_load(&game_save);
    game_save.boots++;
    dbg("save: %s v%d boots=%u found=%d opens=%u", st == SAVE_LOADED ? "loaded" : st == SAVE_NEW ? "new" : "reset",
        SAVE_VERSION, game_save.boots, found_total(), (unsigned)game_save.opens);
    collection_save();
}

bool collection_save(void) {
    bool ok = save_store(&game_save);
    if (!ok) dbg("save: write FAILED");
    return ok;
}
#endif
