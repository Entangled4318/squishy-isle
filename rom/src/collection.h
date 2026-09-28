/* The child's collection: which friends are found, the roll for the next
 * container (never a friend already found), and the follower line.
 * Everything lives in the save and is stored right after each change. */
#ifndef COLLECTION_H
#define COLLECTION_H
#include "save.h"


extern SaveData game_save;

void collection_init(void);              /* load the save at boot */
void collection_new_game(u32 seed);      /* forget all friends and reshuffle */
bool collection_save(void);

static inline int friend_id(int species, int flavor) { return species * 5 + flavor; }
static inline int friend_species(int id) { return id / 5; }
static inline int friend_flavor(int id) { return id % 5; }
static inline int friend_area(int id) { return id / 20; }

#define AREA_COUNT  4
#define GATE_NEED   10        /* friends found in an area that open the way to the next (owner) */

u32 game_rand(void);                     /* shared random numbers (saved state) */

bool friend_found(int id);
int found_total(void);
int found_in_area(int area);
bool area_open(int area);                /* the meadow always; later areas once the one before has GATE_NEED friends */

int collection_roll(int area, u32 entropy);   /* a friend not found yet, or -1 when the area is full */

bool collection_add(int id);             /* records the friend and saves; false if already found */

void follower_add(int id);               /* puts id at the front of its area's line */
void follower_join(int id);              /* a new friend: joins the end only while the line has room */
int follower_get(int area, int i);       /* friend id in that area's line, or -1 */
bool follower_has(int id);
void follower_remove(int id);            /* closes the gap; later ones move up */

#endif
