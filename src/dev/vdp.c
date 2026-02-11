#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dev.h"
#include "uxn.h"

/* ==========================================================================
   B6X VIDEO DISPLAY PROCESSOR
   ========================================================================== */

static uint16_t regs[16],    cram[64];
static uint8_t  vram[65536], cgram[1024];

#define W 320 /* - Screen width  */
#define H 224 /* - Screen height */

/* === MODE Register fields === */
#define F_TXTBUF  0b0000000000000001 /* - Show text buffer                */
#define F_SPRITES 0b0000000000000010 /* - Show sprites                    */
#define F_PLANE_A 0b0000000000000100 /* - Show tile layer A               */
#define F_PLANE_B 0b0000000000001000 /* - Show tile layer B               */
#define F_BG_COL  0b0000000000110000 /* - Palette row of background color */
#define F_HBLANK  0b0000000001000000 /* - Enable H-blank vector           */
#define F_VBLANK  0b0000000010000000 /* - Enable V-blank vector           */

#define F_REGS_W  0b0001000000000000 /* - Register write indicator        */
#define F_VRAM_W  0b0010000000000000 /* - VRAM write indicator            */
#define F_CRAM_W  0b0100000000000000 /* - CRAM write indicator            */
#define F_CGRAM_W 0b1000000000000000 /* - CGRAM write indicator           */

/* === Register aliases === */
#define COMMAND   regs[0x0]  /* - Command of DEO/DEI operation            */
#define MODE      regs[0x1]  /* - VDP mode and indicators                 */

#define USER_A    regs[0x2]  /* - User register A                         */
#define USER_B    regs[0x3]  /* - User register B                         */
#define USER_C    regs[0x4]  /* - User register C                         */

#define HBLANK    regs[0x5]  /* - H-blank vector location in RAM          */
#define HBLANK_Y  regs[0x6]  /* - H-blank vector trigger line             */
#define VBLANK    regs[0x7]  /* - V-blank vector location in RAM          */

#define TXTBUF    regs[0x8]  /* - Text buffer location in VRAM            */
#define SPRITES   regs[0x9]  /* - Sprite attribute table location in VRAM */

#define PLANE_A   regs[0xA]  /* - Tile layer A location in VRAM           */
#define PLANE_A_X regs[0xB]  /* - Layer A horizontal scroll               */
#define PLANE_A_Y regs[0xC]  /* - Layer A vertical scroll                 */

#define PLANE_B   regs[0xD]  /* - Tile layer B location in VRAM           */
#define PLANE_B_X regs[0xE]  /* - Layer B horizontal scroll               */
#define PLANE_B_Y regs[0xF]  /* - Layer B vertical scroll                 */

static void circ_fill(void *d, size_t i, size_t s,
                                        uint8_t c, size_t n) {
    if (n >= s) { memset(d, c, s); return; }

    i %= s;

    while (n) {
        size_t a = (a = s - i) > n ? n : a;
        memset(d+i, c, a);
        i = (i + a) % s;
        n -= a;
    }
}

static void circ_copy(void *d, size_t di, size_t ds,
                      void *s, size_t si, size_t ss, size_t n) {
    di %= ds; si %= ss;

    while (n) {
        size_t a = ds - di;
        if (a > ss - si) a = ss - si;
        if (a > n) a = n;
        memcpy(d+di, s+si, a);
        di = (di + a) % ds;
        si = (si + a) % ss;
        n -= a;
    }
}

void dev_vdp_deo(uint8_t *port) {
    port--;

    if (!COMMAND) { COMMAND = PEEK2(0, port, 1); return; }

    uint8_t parameter = COMMAND >> 8;

    /* printf("VDP %04x %04x\n", PEEK2(0, port, 1), COMMAND); */

    switch (COMMAND & 31) {
        /* === Register access === */
        case 0x00: regs[parameter & 15] = 0; break;
        case 0x01:
        case 0x02: regs[parameter & 15] = port[COMMAND & 1]; break;
        case 0x03: regs[parameter & 15] = PEEK2(0, port, 1); break;

        /* === Palette access === */
        case 0x04: cram[parameter & 63] = 0; break;
        case 0x05:
        case 0x06: cram[parameter & 63] = port[COMMAND & 1]; break;
        case 0x07: cram[parameter & 63] = PEEK2(0, port, 1); break;

        /* === VRAM access 1 === */
        case 0x08: vram[PEEK2(0, port, 1)] = 0; break;
        case 0x09:
        case 0x0A: vram[regs[parameter & 15]] = port[COMMAND & 1]; break;
        case 0x0B: circ_copy(vram, regs[parameter & 15], 65536,
                          uxn_ram, regs[parameter >> 4], 65536,
                                            PEEK2(0, port, 1)); break;

        /* === VRAM access 2 === */
        case 0x0C: circ_fill(vram, regs[parameter & 15], 65536,
                                0, PEEK2(0, port, 1)); break;
        case 0x0D:
        case 0x0E: circ_fill(vram, regs[parameter & 15], 65536,
                port[COMMAND & 1], regs[parameter >> 4]); break;
        case 0x0F: circ_copy(vram, regs[parameter & 15], 65536,
                       port, 0, 2, regs[parameter >> 4]); break;

        /* === CGRAM Font setting === */
        case 0x10: circ_copy(cgram, 0, 1024,
                uxn_ram, PEEK2(0, port, 1), 65536, 1024); break;

        default: break;
    }

    MODE |= (uint16_t[]) { F_REGS_W, F_CRAM_W, F_VRAM_W, F_VRAM_W,
                           F_CGRAM_W, 0, 0, 0 }[COMMAND >> 4 & 8];
    COMMAND = 0;
}


uint8_t dev_vdp_dei(uint8_t *port) {
    uint8_t  parameter = COMMAND >> 8;
    uint16_t data = 0;

    switch (COMMAND & 15) {
        case 0x00: data = regs[parameter & 15]; break;
        case 0x01: data = cram[parameter & 63]; break;
        case 0x02: data = PEEK2(regs[parameter & 15], vram, 0xFFFF); break;
    }

    POKE2(0, port, 1, data);
    COMMAND = 0;

    return *port;
}

#define PLANE_GET_PX(plane, plane_x, plane_y, plane_pixels)        \
    do {                                                           \
        uint16_t scrolled_x = x + (plane_x),                       \
                 scrolled_y = y + (plane_y);                       \
                                                                   \
        uint8_t column = scrolled_x >> 3 & 63,                     \
                row    = scrolled_y >> 3 & 31;                     \
                                                                   \
        uint16_t addr  = (plane) + (column << 1) + (row << 7),     \
                 entry = PEEK2(addr, vram, 0xFFFF);                \
                                                                   \
        uint16_t pattern_id = entry & 2047;                        \
                                                                   \
        if (!pattern_id) break;                                    \
                                                                   \
        local_x = scrolled_x & 7, local_y = scrolled_y & 7;        \
                                                                   \
        if (entry & 2048) local_x = 7 - local_x;                   \
        if (entry & 4096) local_y = 7 - local_y;                   \
                                                                   \
        addr = (pattern_id << 5) + (local_y << 2) + (local_x >> 1);\
                                                                   \
        uint8_t pixel = vram[addr] >> ((~local_x & 1) << 2) & 15;  \
                                                                   \
        if (!pixel) break;                                         \
                                                                   \
        pixel |= entry >> 9 & F_BG_COL;                            \
                                                                   \
        if (entry & 32768) plane_pixels |= pixel << 8;             \
        else               plane_pixels  = pixel;                  \
                                                                   \
    } while (0);

void dev_vdp(uint32_t *buffer) {
    uint32_t i, x, y = 0;
    uint8_t color = 0;

    uint16_t sprite_cache[128] = { 0 };
    uint32_t cram_cache[64]    = { 0 };

    if (MODE & F_VBLANK) uxn_eval(VBLANK);

    if (!(MODE & 0xF000)) return;

    MODE |= F_CRAM_W;

    for (i = 0; i < (W * H); i++) {
        y = i / W, x = i - y * W;

        if (x) goto not_hblank;

        uint16_t link = SPRITES, idx = 0;
        sprite_cache[0] = 0;

        while ((MODE & F_SPRITES) && idx != 80*4) {
            uint16_t base1 = PEEK2(link,     vram, 0xFFFF),
                     base2 = PEEK2(link + 2, vram, 0xFFFF),
                     x_pos = PEEK2(link + 4, vram, 0xFFFF) & 511,
                     y_pos = PEEK2(link + 6, vram, 0xFFFF) & 255;

            if (!(base1 & 2047)) goto skip;

            uint8_t hsize = ((base2 >> 8  & 3) + 1) << 3,
                    vsize = ((base2 >> 10 & 3) + 1) << 3;

            if (x_pos >= 320 && x_pos < (512 - hsize)) goto skip;

            uint8_t v_dist = y - y_pos;
            if (v_dist >= vsize) goto skip;

            sprite_cache[idx++ & 127] = base1;
            sprite_cache[idx++ & 127] = base2;
            sprite_cache[idx++ & 127] = x_pos;
            sprite_cache[idx++ & 127] = y_pos;

            if ((idx >> 2) < 32) sprite_cache[idx & 127] = 0;

            skip:
            if (link == ((base2 & 127) << 3) + SPRITES) break;
            link = (base2 & 127) << 3;
            link += SPRITES;
        }

        if ((MODE & F_HBLANK) && y == HBLANK_Y) uxn_eval(HBLANK);

        for (uint8_t idx = 0; (MODE & F_CRAM_W) && idx < 64; idx++) {
            uint16_t c = cram[idx];
            cram_cache[idx] = (c & 0xF00) >> 4 |
                              (c & 0x0F0) << 8 |
                              (c & 0x00F) << 20;
        }

        MODE &= 0x0FFF;

not_hblank:

        uint8_t txtbuf_char = 0, local_x = x & 7, local_y = y & 7;

        if (MODE & F_TXTBUF)
            txtbuf_char = vram[TXTBUF + (x >> 3) + (W >> 3) * (y >> 3)];

        if (txtbuf_char) {
            buffer[i] = cram_cache[MODE & F_BG_COL];
            if (txtbuf_char & 128) buffer[i] = ~buffer[i];
            txtbuf_char &= 127;
            if (cgram[(txtbuf_char << 3) + local_y] >> (7 - local_x) & 1)
                buffer[i] = ~buffer[i];

            continue;
        }

        uint16_t plane_a_pixels = 0, plane_b_pixels = 0, sprites_pixels = 0;

        idx = 0;
        while (idx < 32*4 && (MODE & F_SPRITES)) {
            uint16_t base1 = sprite_cache[idx++], base2 = sprite_cache[idx++],
                     x_pos = sprite_cache[idx++], y_pos = sprite_cache[idx++];

            if (!(base1 & 2047)) break;

            uint16_t local_x = (x - x_pos) & 511, local_y = (y - y_pos) & 255;

            uint8_t hsize = ((base2 >> 8  & 3) + 1) << 3,
                    vsize = ((base2 >> 10 & 3) + 1) << 3;

            if ((local_x >= hsize || local_y >= vsize) ||
                (sprites_pixels & 15 && !(base1 & 32768))) continue;

            if (base1 >> 11 & 1) local_x = hsize - 1 - local_x;
            if (base1 >> 12 & 1) local_y = vsize - 1 - local_y;

            uint16_t addr = (((base1 & 2047) + (local_x >> 3) *
                    (vsize >> 3) + (local_y >> 3)) << 5) +
                    ((local_y & 7) << 2) + ((local_x & 7) >> 1);

            uint8_t pixel = vram[addr] >> ((~local_x & 1) << 2) & 15;

            if (!pixel) continue;

            pixel |= base1 >> 9 & F_BG_COL;

            if (base1 & 32768) { sprites_pixels |= pixel << 8; break; }
            if (!(sprites_pixels & 0xFF)) sprites_pixels |= pixel;
        }
        if (sprites_pixels & 0x0F00) { color = sprites_pixels >> 8; goto put; }

        if (MODE & F_PLANE_A)
            PLANE_GET_PX(PLANE_A, PLANE_A_X, PLANE_A_Y, plane_a_pixels);
        if (plane_a_pixels & 0x0F00) { color = plane_a_pixels >> 8; goto put; }

        if (MODE & F_PLANE_B)
            PLANE_GET_PX(PLANE_B, PLANE_B_X, PLANE_B_Y, plane_b_pixels);
        if (plane_b_pixels & 0x0F00) { color = plane_b_pixels >> 8; goto put; }

        if (sprites_pixels & 0xF) { color = sprites_pixels; goto put; }
        if (plane_a_pixels & 0xF) { color = plane_a_pixels; goto put; }
        if (plane_b_pixels & 0xF) { color = plane_b_pixels; goto put; }

        color = MODE & F_BG_COL;

put:    buffer[i] = cram_cache[color & 63];
    }
}

#undef W
#undef H
#undef F_TXTBUF
#undef F_SPRITES
#undef F_PLANE_A
#undef F_PLANE_B
#undef F_BG_COL
#undef F_HBLANK
#undef F_VBLANK
#undef F_REGS_W
#undef F_VRAM_W
#undef F_CRAM_W
#undef COMMAND
#undef MODE
#undef USER_A
#undef USER_B
#undef USER_C
#undef HBLANK
#undef HBLANK_Y
#undef VBLANK
#undef TXTBUF
#undef SPRITES
#undef PLANE_A
#undef PLANE_A_X
#undef PLANE_A_Y
#undef PLANE_B
#undef PLANE_B_X
#undef PLANE_B_Y
#undef PLANE_GET_PX
