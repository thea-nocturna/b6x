#ifndef DEV_H
#define DEV_H

#include <stdint.h>

#define PEEK2(addr, mem, mask) \
    (mem[(addr) & (mask)] << 8 | mem[((addr)+1) & (mask)])

#define POKE2(addr, mem, mask, value) \
    { uint16_t v = value;             \
      mem[(addr) & (mask)] = v >> 8;  \
      mem[((addr)+1) & (mask)] = v; } \

void dev_init(void);

void    dev_vdp_deo(uint8_t *port);
uint8_t dev_vdp_dei(uint8_t *port);
void    dev_vdp(uint32_t *buffer);

uint8_t dev_ctl_dei(uint8_t *port);
void    dev_ctl_deo(uint8_t *port);
void    dev_ctl(uint8_t code);

void    dev_rom_deo(uint8_t *port);
void    dev_rom_open(const char *fname);
void    dev_rom_close(void);

uint8_t dev_rst_dei(uint8_t *port);
void    dev_rst_deo(uint8_t *port);

uint8_t dev_wst_dei(uint8_t *port);
void    dev_wst_deo(uint8_t *port);

uint8_t dev_snd_dei(uint8_t *port);
void    dev_snd_deo(uint8_t *port);

uint8_t dev_dbg_dei(uint8_t *port);
void    dev_dbg_deo(uint8_t *port);

void    dev_meta_deo(uint8_t *port);

#endif /* DEV_H */