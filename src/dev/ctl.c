#include <stdint.h>

#include "uxn.h"
#include "dev.h"

/* ==========================================================================
   B6X CONTROLLER
   ========================================================================== */

static uint8_t  ctl_btn = 0;
static uint16_t ctl_vec = 0;

uint8_t dev_ctl_dei(uint8_t *port) {
    return *port = ctl_btn;
}
void dev_ctl_deo(uint8_t *port) {
    port--;
    ctl_vec = PEEK2(0, port, 1);
}

void dev_ctl(uint8_t code) {
    ctl_btn = code;
    if (ctl_vec) uxn_eval(ctl_vec);
}