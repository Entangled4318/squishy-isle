/* Squishy Isle entry point.
 * Holding L + R + SELECT at power-on opens the hardware check screen,
 * L + R + START the sound test. In both, holding L + R for 1 s goes on to
 * the game's title (step 9.7; before, only a power cycle left them). */
#include "collection.h"
#include "game.h"
#include "sound.h"
#include "system.h"

int hwcheck_main(void);

int main(void) {
    system_init();
    sound_init();
    input_poll();
    if ((key_held() & (KEY_L | KEY_R | KEY_SELECT)) == (KEY_L | KEY_R | KEY_SELECT)) hwcheck_main();   /* returns on L + R */
    if ((key_held() & (KEY_L | KEY_R | KEY_START)) == (KEY_L | KEY_R | KEY_START)) {
        scene_run(&scene_jukebox);
        return 0;
    }
    collection_init();
    scene_run(&scene_title);
    return 0;
}
