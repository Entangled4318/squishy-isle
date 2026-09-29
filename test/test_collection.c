#include <stddef.h>
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
    CHECK(found_total() == 0 && follower_get(0, 0) == -1 && game_save.opens == 0, "new game clears friends, followers and opens");

    /* follower line: newest first, max 3, no duplicates */
    reset(5);
    follower_add(1); follower_add(2); follower_add(3); follower_add(4);
    CHECK(follower_get(0, 0) == 4 && follower_get(0, 1) == 3 && follower_get(0, 2) == 2, "line keeps the newest 3 (4,3,2)");
    follower_add(2);
    CHECK(follower_get(0, 0) == 2 && follower_get(0, 1) == 4 && follower_get(0, 2) == 3, "a friend already in line moves to the front (2,4,3)");
    reset(5);
    follower_add(0);
    CHECK(follower_get(0, 0) == 0 && follower_get(0, 1) == -1, "friend id 0 works, empty places read -1");

    /* new friends: the first 3 found stay in line, later ones go to the pen */
    reset(5);
    collection_add(7); collection_add(3); collection_add(9); collection_add(12);
    CHECK(follower_get(0, 0) == 7 && follower_get(0, 1) == 3 && follower_get(0, 2) == 9, "the first 3 friends found keep following (7,3,9)");
    follower_remove(3);
    collection_add(14);
    CHECK(follower_get(0, 0) == 7 && follower_get(0, 1) == 9 && follower_get(0, 2) == 14, "a new friend fills a free place at the end (7,9,14)");
    game_save.pip_x = 300; game_save.pip_y = 200;
    collection_new_game(99);
    CHECK(game_save.pip_x == 0 && game_save.pip_y == 0, "new game forgets Pip's saved position");
    CHECK(offsetof(SaveData, checksum) == 140, "save layout unchanged by the position field (checksum at %u)", (unsigned)offsetof(SaveData, checksum));

    /* each area has its own line; a woods friend never joins the meadow's */
    reset(5);
    collection_add(1); collection_add(21); collection_add(45); collection_add(2);
    CHECK(follower_get(0, 0) == 1 && follower_get(0, 1) == 2 && follower_get(1, 0) == 21 && follower_get(2, 0) == 45 &&
          follower_get(3, 0) == -1, "each area keeps its own follower line (1,2 | 21 | 45 | -)");
    follower_remove(21);
    CHECK(follower_get(1, 0) == -1 && follower_get(0, 0) == 1, "removing a woods follower leaves the meadow line");
    /* gates: area n + 1 opens at GATE_NEED friends in area n */
    reset(5);
    for (int id = 0; id < GATE_NEED - 1; id++) collection_add(id);
    bool shut = !area_open(1);
    collection_add(GATE_NEED - 1);
    CHECK(area_open(0) && shut && area_open(1) && !area_open(2), "the woods open at %d meadow friends, the shore stays shut", GATE_NEED);
    game_save.area = 2; game_save.gates = 3;
    collection_new_game(7);
    CHECK(game_save.area == 0 && game_save.gates == 0 && follower_get(1, 0) == -1, "new game goes back to the meadow and closes the gates");
    CHECK(sizeof(SaveData) <= 0x100, "SaveData fits a 256-byte slot (%u bytes)", (unsigned)sizeof(SaveData));

    /* mailbox: each new friend sends a letter; a friend found again sends none */
    reset(5);
    CHECK(game_save.mail == 0 && !game_save.mail_new, "a new game has no letter yet");
    collection_add(4);
    collection_add(27);
    CHECK(game_save.mail == 28 && game_save.mail_new, "the newest friend (27) wrote the letter, the flag is up");
    game_save.mail_new = 0;
    collection_add(27);
    CHECK(game_save.mail == 28 && !game_save.mail_new, "no letter for a friend found before");
    collection_new_game(3);
    CHECK(game_save.mail == 0 && !game_save.mail_new, "new game empties the mailbox");
    CHECK(offsetof(SaveData, mail) == 123 && offsetof(SaveData, checksum) == 140, "mail sits in the old padding (at %u)",
          (unsigned)offsetof(SaveData, mail));
    return fails ? 1 : 0;
}
