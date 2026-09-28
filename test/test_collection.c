/* Host unit test for the roll and collection rules (rom/src/collection.c). */
#define HOST_TEST
#include "../rom/src/collection.c"

#include <stdio.h>
#include <stdlib.h>

static int fails;
#define CHECK(c, ...) do { printf("%s ", (c) ? "PASS" : "FAIL"); printf(__VA_ARGS__); printf("\n"); if (!(c)) fails++; } while (0)

bool collection_save(void) { return true; }

static void reset(u32 seed) { memset(&game_save, 0, sizeof game_save); game_save.rng = seed; }

int main(void) {
    /* first 5 opens are always new, in every area, for many seeds */
    int bad = 0;
    for (u32 seed = 1; seed <= 2000; seed++) {
        reset(seed);
        for (int area = 0; area < 4; area++) {
            for (int k = 0; k < 5; k++) {
                int id = collection_roll(area, seed * 7 + k);
                if (id < area * 20 || id >= area * 20 + 20 || friend_found(id)) bad++;
                collection_add(id);
            }
        }
    }
    CHECK(bad == 0, "first 5 opens in each area always new (%d bad of 40000)", bad);

    /* whole meadow: every friend reachable; opens needed to reach 6 and 20 */
    long sum6 = 0, sum20 = 0; int max20 = 0, never = 0;
    for (u32 seed = 1; seed <= 2000; seed++) {
        reset(seed);
        int n = 0, at6 = 0;
        while (found_in_area(0) < 20 && n < 500) {
            collection_add(collection_roll(0, n * 31u));
            n++;
            if (!at6 && found_in_area(0) >= 6) at6 = n;
        }
        if (n >= 500) never++;
        sum6 += at6; sum20 += n; if (n > max20) max20 = n;
    }
    CHECK(never == 0, "all 20 meadow friends reachable (stuck runs: %d)", never);
    CHECK(sum6 / 2000.0 <= 7.0, "6 friends (next area) after %.1f opens on average", sum6 / 2000.0);
    CHECK(sum20 / 2000.0 <= 35.0, "full meadow after %.1f opens on average (worst %d)", sum20 / 2000.0, max20);

    /* sparkle is rare among repeats: roll a full collection many times */
    reset(99);
    for (int id = 0; id < 20; id++) game_save.found[id] = 1;
    game_save.opens = 20;
    int sparkle = 0, n = 20000, seen[20] = {0};
    for (int i = 0; i < n; i++) { int id = collection_roll(0, i); seen[id]++; sparkle += friend_flavor(id) == 4; }
    int all = 1; for (int i = 0; i < 20; i++) all &= seen[i] > 0;
    CHECK(sparkle > n * 0.04 && sparkle < n * 0.08, "Sparkle rolls are rare (%.1f%%, expect ~5.9%%)", 100.0 * sparkle / n);
    CHECK(all, "every friend can come again as a repeat");

    /* hearts: new, then 3 hearts, then capped */
    reset(5);
    GotResult r[5];
    for (int i = 0; i < 5; i++) r[i] = collection_add(7);
    CHECK(r[0] == GOT_NEW && r[1] == GOT_HEART && r[3] == GOT_HEART && r[4] == GOT_REPEAT && friend_hearts(7) == 3,
          "new, 3 hearts, then no more hearts (hearts=%d)", friend_hearts(7));

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
