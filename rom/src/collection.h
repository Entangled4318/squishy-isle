/* The child's collection: which friends are found, hearts for repeats,
 * the roll for the next container, and the follower line.
 * Everything lives in the save and is stored right after each change. */
#ifndef COLLECTION_H
#define COLLECTION_H
#include "save.h"

#define FIRST_NEW_OPENS 5      /* the first 5 friends of each area are always new */
#define MAX_HEARTS      3

extern SaveData game_save;

void collection_init(void);              /* load the save at boot */
bool collection_save(void);

static inline int friend_id(int species, int flavor) { return species * 5 + flavor; }
static inline int friend_species(int id) { return id / 5; }
static inline int friend_flavor(int id) { return id % 5; }

bool friend_found(int id);
int friend_hearts(int id);               /* 0..3 */
int found_total(void);
int found_in_area(int area);

int collection_roll(int area, u32 entropy);   /* friend id for the next container */

typedef enum { GOT_NEW, GOT_HEART, GOT_REPEAT } GotResult;
GotResult collection_add(int id);        /* records the friend and saves */

void follower_add(int id);               /* puts id at the front of the line */
int follower_get(int i);                 /* friend id, or -1 */

#endif
