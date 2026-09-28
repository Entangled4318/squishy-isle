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

bool friend_found(int id);
int found_total(void);
int found_in_area(int area);

int collection_roll(int area, u32 entropy);   /* a friend not found yet, or -1 when the area is full */

bool collection_add(int id);             /* records the friend and saves; false if already found */

void follower_add(int id);               /* puts id at the front of the line */
int follower_get(int i);                 /* friend id, or -1 */

#endif
