/* Squishy Isle entry point.
 * Holding L + R + SELECT at power-on opens the hardware check screen. */
#include "game.h"
#include "sound.h"
#include "system.h"

int hwcheck_main(void);

int main(void) {
    system_init();
    sound_init();
    input_poll();
    if ((key_held() & (KEY_L | KEY_R | KEY_SELECT)) == (KEY_L | KEY_R | KEY_SELECT)) return hwcheck_main();
    scene_run(&scene_meadow_view);
    return 0;
}
