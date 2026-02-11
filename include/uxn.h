#ifndef UXN_H
#define UXN_H

#include <stdint.h>

extern uint8_t uxn_ram[65536], uxn_dev[256], uxn_ptr[2], uxn_stk[2][256];
extern void    (*uxn_deo_handlers[256])(uint8_t *port);
extern uint8_t (*uxn_dei_handlers[256])(uint8_t *port);

uint32_t uxn_eval(uint16_t pc);

#endif /* UXN_H */