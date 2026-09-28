/* Host unit test for the roll and collection rules (rom/src/collection.c). */
#define HOST_TEST
#include "../rom/src/collection.c"

#include <stdio.h>
#include <stdlib.h>

static int fails;
#define CHECK(c, ...) do { printf("%s ", (c) ? "PASS" : "FAIL"); printf(__VA_ARGS__); printf("\n"); if (!(c)) fails++; } while (0)

bool collection_save(void) { return true; }

static void reset(u32 seed) { memset(&game_save, 0, sizeof game_save); collection_new_game(seed); }

int main(void) {
    /* play every area to the end for many games: never a repeat, never outside the area */
    int repeats = 0, outside = 0, short_area = 0, extra = 0;
    for (u32 seed = 1; seed <= 2000; seed++) {
        reset(seed);
        for (int area = 0; area < 4; area++) {
            for (int k = 0; k < 20; k++) {
                int id = collection_roll(area, seed * 7 + k);
                if (id < area * 20 || id >= area * 20 + 20) { outside++; continue; }
                if (!collection_add(id)) repeats++;
            }
            if (found_in_area(area) != 20) short_area++;
            if (collection_roll(area, 5) != -1) extra++;
        }
    }
    CHECK(repeats == 0 && outside == 0, "20 opens per area give 20 different friends of that area (%d repeats, %d outside)", repeats, outside);
    CHECK(short_area == 0, "every area fills completely in exactly 20 opens");
    CHECK(extra == 0, "a full area rolls -1 (no more containers)");

    /* order differs between new games */
    int first[20] = {0}, same_order = 0;
    int prev[20];
    for (u32 seed = 1; seed <= 200; seed++) {
        reset(seed * 2654435761u);
        int order[20];
        for (int k = 0; k < 20; k++) { order[k] = collection_roll(0, k); collection_add(order[k]); }
        first[order[0] % 20]++;
        if (seed > 1 && memcmp(order, prev, sizeof order) == 0) same_order++;
        memcpy(prev, order, sizeof prev);
    }
    int kinds = 0; for (int i = 0; i < 20; i++) kinds += first[i] > 0;
    CHECK(same_order == 0 && kinds >= 12, "each new game has its own order (%d different first friends in 200 games)", kinds);

    /* Sparkles lean late: average position of the 4 Sparkles vs the rest */
    double sp = 0, other = 0;
    for (u32 seed = 1; seed <= 2000; seed++) {
        reset(seed * 97);
        for (int k = 0; k < 20; k++) {
            int id = collection_roll(0, k);
            collection_add(id);
            if (friend_flavor(id) == 4) sp += k; else other += k;
        }
    }
    sp /= 2000.0 * 4; other /= 2000.0 * 16;
    CHECK(sp > other + 3, "Sparkles tend to come later (average open %.1f vs %.1f)", sp + 1, other + 1);

    /* a friend can never be added twice */
    reset(5);
    bool a = collection_add(7), b = collection_add(7);
    CHECK(a && !b && found_total() == 1 && game_save.opens == 1, "adding a found friend again is refused");

    /* new game forgets friends and followers */
    collection_add(8);
    collection_new_game(1234);
    CHECK(found_total() == 0 && follower_get(0) == -1 && game_save.opens == 0, "new game clears friends, followers and opens");

    /* follower line: newest first, max 3, no duplicates */
    reset(5);
    follower_add(1); follower_add(2); follower_add(3); follower_add(4);
    CHECK(follower_get(0) == 4 && follower_get(1) == 3 && follower_get(2) == 2, "line keeps the newest 3 (4,3,2)");
    follower_add(2);
    CHECK(follower_get(0) == 2 && follower_get(1) == 4 && follower_get(2) == 3, "a friend already in line moves to the front (2,4,3)");
    reset(5);
    follower_add(0);
    CHECK(follower_get(0) == 0 && follower_get(1) == -1, "friend id 0 works, empty places read -1");

    CHECK(sizeof(SaveData) <= 0x100, "SaveData fits a 256-byte slot (%u bytes)", (unsigned)sizeof(SaveData));
    return fails ? 1 : 0;
}
