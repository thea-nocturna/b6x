#include <stdint.h>

#include "dev.h"
#include "uxn.h"

/* ==========================================================================
   B6X STACK
   ========================================================================== */

uint8_t dev_rst_dei(uint8_t *port) { return uxn_ptr[1]; }
void    dev_rst_deo(uint8_t *port) { uxn_ptr[1] = *port; }

uint8_t dev_wst_dei(uint8_t *port) { return uxn_ptr[0]; }
void    dev_wst_deo(uint8_t *port) { uxn_ptr[0] = *port; }
