#include <stdint.h>
#include <stdio.h>

#include "uxn.h"
#include "dev.h"

/* ==========================================================================
   B6X DEBUG INTERFACE
   ========================================================================== */

static void stack_printer(char *name, uint8_t ptr, uint8_t *stk) {
	fprintf(stderr, "%s%c", name, ptr - 8 ? ' ' : '|');
	for(uint8_t i = ptr - 8; i != ptr; i++)
		fprintf(stderr, "%02x%c", stk[i], i == 0xff ? '|' : ' ');
	fprintf(stderr, "<%02x\n", ptr);
}

uint8_t dev_dbg_dei(uint8_t *port) {
    return 0;
}

void dev_dbg_deo(uint8_t *port) {
    switch (*port) {
        case 0: return;
        case 1: stack_printer("WST", uxn_ptr[0], &uxn_stk[0][0]);
                stack_printer("RST", uxn_ptr[1], &uxn_stk[1][0]); return;
        case 2: getchar(); return;

        default: return;
    }
}