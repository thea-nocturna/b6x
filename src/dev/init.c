#include <stdint.h>

#include "dev.h"
#include "uxn.h"

/* ==========================================================================
   B6X DEVICE I/O HANDLING
   ========================================================================== */

void dev_init(void) {
    uxn_dei_handlers[0x04] = dev_wst_dei;
    uxn_deo_handlers[0x04] = dev_wst_deo;

	uxn_dei_handlers[0x05] = dev_rst_dei;
	uxn_deo_handlers[0x05] = dev_rst_deo;

    uxn_deo_handlers[0x07] = dev_meta_deo;

    uxn_deo_handlers[0x09] = dev_rom_deo;

    uxn_dei_handlers[0x0A] = dev_ctl_dei;
    uxn_deo_handlers[0x0B] = dev_ctl_deo;

    uxn_dei_handlers[0x0C] = dev_vdp_dei;
    uxn_deo_handlers[0x0D] = dev_vdp_deo;

    uxn_dei_handlers[0x0E] = dev_dbg_dei;
    uxn_deo_handlers[0x0E] = dev_dbg_deo;
}