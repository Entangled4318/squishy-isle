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

u32 game_rand(void) { return rng_next(); }

bool friend_found(int id) { return game_save.found[id] != 0; }

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

bool area_open(int area) { return area <= 0 || (area < AREA_COUNT && found_in_area(area - 1) >= GATE_NEED); }

/* Sparkle is the rare flavor: weight 1 against 4 for the others, so the
 * Sparkles of an area tend to come last. */
static int weight(int id) { return friend_flavor(id) == 4 ? 1 : 4; }

/* Picks one of the area's friends not found yet. */
static int pick(int area, u32 r) {
    int total = 0;
    for (int id = area * 20; id < area * 20 + 20; id++)
        if (!friend_found(id)) total += weight(id);
    if (total == 0) return -1;
    int t = (int)(r % (u32)total);
    for (int id = area * 20; id < area * 20 + 20; id++) {
        if (friend_found(id)) continue;
        t -= weight(id);
        if (t < 0) return id;
    }
    return -1;
}

int collection_roll(int area, u32 entropy) {
    game_save.rng ^= entropy * 0x9E3779B9u;
    return pick(area, rng_next());
}

bool collection_add(int id) {
    if (id < 0 || id >= NUM_FRIENDS || friend_found(id)) return false;
    game_save.found[id] = 1;
    game_save.opens++;
    game_save.mail = (u8)(id + 1);           /* a letter from the new friend: the mailbox flag goes up */
    game_save.mail_new = 1;
    follower_join(id);
    collection_save();
    return true;
}

void collection_new_game(u32 seed) {
    memset(game_save.found, 0, sizeof game_save.found);
    memset(game_save.followers, 0, sizeof game_save.followers);
    memset(game_save.lines, 0, sizeof game_save.lines);
    game_save.opens = 0;
    game_save.pip_x = game_save.pip_y = 0;
    game_save.area = 0;
    game_save.gates = 0;
    game_save.mail = game_save.mail_new = 0;
    game_save.rng = seed | 1;
    collection_save();
}

/* Each area has its own line of up to 3 followers (friend sprites use the
 * area's palettes, so only that area's friends can walk there). */
static u8 *line_of(int area) {
    if (area <= 0) return game_save.followers;
    return game_save.lines[(area > AREA_COUNT - 1 ? AREA_COUNT - 1 : area) - 1];
}
static u8 *line_for(int id) { return line_of(friend_area(id)); }

void follower_add(int id) {
    u8 *l = line_for(id);
    int at = MAX_FOLLOWERS - 1;          /* drops the oldest unless id is already in line */
    for (int i = 0; i < MAX_FOLLOWERS; i++)
        if (l[i] == id + 1) at = i;
    for (int i = at; i > 0; i--) l[i] = l[i - 1];
    l[0] = (u8)(id + 1);
}

void follower_join(int id) {
    u8 *l = line_for(id);
    for (int i = 0; i < MAX_FOLLOWERS; i++) {
        if (l[i] == id + 1) return;
        if (l[i] == 0) {
            l[i] = (u8)(id + 1);
            return;
        }
    }
}

int follower_get(int area, int i) { return line_of(area)[i] - 1; }

bool follower_has(int id) {
    const u8 *l = line_for(id);
    for (int i = 0; i < MAX_FOLLOWERS; i++)
        if (l[i] == id + 1) return true;
    return false;
}

void follower_remove(int id) {
    u8 *l = line_for(id);
    int n = 0;
    for (int i = 0; i < MAX_FOLLOWERS; i++)
        if (l[i] && l[i] != id + 1) l[n++] = l[i];
    while (n < MAX_FOLLOWERS) l[n++] = 0;
}

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
