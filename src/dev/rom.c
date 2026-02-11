#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#include "dev.h"
#include "uxn.h"

/* ==========================================================================
   B6X READ ONLY MEMORY
   ========================================================================== */

static FILE  *rom      = NULL;
static size_t rom_size = 0;

void dev_rom_deo(uint8_t *port) {
    if (!rom) return;
    port--;

    static uint8_t step = 0;
    static size_t src = 0;
    static size_t dst = 0;
    static size_t num = 0;

    switch (step++) {
        case 0: src = PEEK2(0, port, 1); return;
        case 1: dst = PEEK2(0, port, 1); return;
        case 2: {
            num = PEEK2(0, port, 1);
            src = (src << 8) % rom_size; /* Convert pages to bytes */

            while (num) {
                size_t avail = 65536 - dst;
                if (avail > rom_size - src) avail = rom_size - src;
                if (avail > num) avail = num;

                fseek(rom, src, SEEK_SET);
                fread(uxn_ram + dst, avail, 1, rom);

                dst = (dst + avail) & 65535;
                src = (src + avail) % rom_size;
                num -= avail;
            }

            step = 0;
            return;
        }
    }
}

void dev_rom_open(const char *fname) {
    dev_rom_close();
    if (!(rom = fopen(fname, "rb"))) return;

    fseek(rom, 0, SEEK_END);
    rom_size = ftell(rom) + 1;
    fseek(rom, 0, SEEK_SET);
}

void dev_rom_close(void) {
    if (!rom) return;

    fclose(rom);
    rom = NULL;
}


