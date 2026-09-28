/* Squishy Isle entry point.
 * Holding L + R + SELECT at power-on opens the hardware check screen,
 * L + R + START the sound test. */
#include "collection.h"
#include "game.h"
#include "sound.h"
#include "system.h"

int hwcheck_main(void);

int main(void) {
    system_init();
    sound_init();
    input_poll();
    if ((key_held() & (KEY_L | KEY_R | KEY_SELECT)) == (KEY_L | KEY_R | KEY_SELECT)) return hwcheck_main();
    if ((key_held() & (KEY_L | KEY_R | KEY_START)) == (KEY_L | KEY_R | KEY_START)) {
        scene_run(&scene_jukebox);
        return 0;
    }
    collection_init();
    scene_run(&scene_title);
    return 0;
}
