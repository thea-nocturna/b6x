#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <MiniFB.h>

#include "uxn.h"
#include "dev.h"
#include "bios.h"

/* ==========================================================================
   B6X MINIFB BACKEND
   ========================================================================== */

#define WIDTH 320
#define HEIGHT 224

static uint32_t buffer[WIDTH * HEIGHT];
static char win_title[256];

void dev_meta_deo(uint8_t *port) {}

static void ctl_update(struct mfb_window *window, mfb_key key,
                           mfb_key_mod mod, bool isPressed) {
    uint8_t code = isPressed << 7;

    switch (key) {
        case KB_KEY_ESCAPE: mfb_close(window); return;
        /* Player 1 */
        case KB_KEY_ENTER: code |= 0x00; break;
        case KB_KEY_A:     code |= 0x01; break;
        case KB_KEY_S:     code |= 0x02; break;
        case KB_KEY_D:     code |= 0x03; break;
        case KB_KEY_UP:    code |= 0x04; break;
        case KB_KEY_DOWN:  code |= 0x05; break;
        case KB_KEY_LEFT:  code |= 0x06; break;
        case KB_KEY_RIGHT: code |= 0x07; break;

        /* Player 2 */
        case KB_KEY_KP_0:  code |= 0x08; break;
        case KB_KEY_Z:     code |= 0x09; break;
        case KB_KEY_X:     code |= 0x0A; break;
        case KB_KEY_C:     code |= 0x0B; break;
        case KB_KEY_KP_5:  code |= 0x0C; break;
        case KB_KEY_KP_2:  code |= 0x0D; break;
        case KB_KEY_KP_1:  code |= 0x0E; break;
        case KB_KEY_KP_3:  code |= 0x0F; break;
        default: return;
    }

    dev_ctl(code);
}

int main(int argc, char **argv) {
	snprintf(win_title, 256, "B6X %04x", VERSION);

    if(argc < 2) dev_rom_open("boot.rom"); else {
        dev_rom_open(argv[1]);
        char *fname = argv[1];
        for (char *f = argv[1]; *f; f++)
            if (*f == '/' ||  *f == '\\') fname = f + 1;

        snprintf(win_title, 256, "B6X %04x - %s", VERSION, fname);
    }

    dev_init();

    memcpy(uxn_ram, bios, bios_len);
    uxn_eval(0);

    struct mfb_window *window = mfb_open_ex(win_title, WIDTH+32, HEIGHT+32, 0);
    if (!(window)) goto terminate;

    mfb_set_keyboard_callback(window, ctl_update);
    mfb_set_viewport(window, 16, 16, 320, 224);
    mfb_set_target_fps(60);

    int state; do {
        dev_vdp(buffer);
        state = mfb_update_ex(window, buffer, WIDTH, HEIGHT);
        if (state < 0) { window = NULL; break; }
    } while(mfb_wait_sync(window));

terminate:
    dev_rom_close();
    return 0;
}


#undef WIDTH
#undef HEIGHT