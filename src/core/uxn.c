#include <stdint.h>

#include "uxn.h"

/* ==========================================================================
   UXN VM CORE
   ========================================================================== */

uint8_t uxn_ram[0x10000], uxn_dev[0x100], uxn_ptr[2], uxn_stk[2][0x100];

void    (*uxn_deo_handlers[256])(uint8_t *port);
uint8_t (*uxn_dei_handlers[256])(uint8_t *port);

static uint8_t dei(uint8_t port) {
    if (uxn_dei_handlers[port]) return uxn_dei_handlers[port](uxn_dev+port);
    return uxn_dev[port];
}

static void deo(uint8_t port, uint8_t value) {
    uxn_dev[port] = value;
    if (uxn_deo_handlers[port]) uxn_deo_handlers[port](uxn_dev+port);
}

#define OPC(opc, A, B) {\
	case 0x00|opc:case 0x40|opc:{const int32_t d=0;A B} break;\
	case 0x20|opc:case 0x60|opc:{const int32_t d=1;A B} break;\
	case 0x80|opc:case 0xc0|opc:{const int32_t d=0,k=*p;A *p=k;B} break;\
	case 0xa0|opc:case 0xe0|opc:{const int32_t d=1,k=*p;A *p=k;B} break;}
#define DEC s[--(*p)]
#define INC s[(*p)++]
#define FLIP s = uxn_stk[!r], p = &uxn_ptr[!r];
#define RELA pc + (int8_t)a
#define JUMP(x) c = uxn_ram[pc] << 8, c |= uxn_ram[pc + 1], pc += x + 2;
#define DROP(o,m) o = DEC; if(m) o |= DEC << 8;
#define TAKE(o) if(d) o[1] = DEC; o[0] = DEC;
#define PUSH(i,m) { if(m) c = (i), INC = c >> 8, INC = c; else INC = i; }
#define GIVE(i) INC = i[0]; if(d) INC = i[1];
#define DEVO(o,r) deo(o, r[0]); if(d) deo(o + 1, r[1]);
#define DEVI(i,r) r[0] = dei(i); if(d) r[1] = dei(i + 1);
#define POKE(o,r,m) uxn_ram[o] = r[0]; if(d) uxn_ram[(o + 1) & m] = r[1];
#define PEEK(i,r,m) r[0] = uxn_ram[i]; if(d) r[1] = uxn_ram[(i + 1) & m];

uint32_t uxn_eval(uint16_t pc) {
	uint16_t a, b, c, x[2], y[2], z[2];
	for(;;) {
	uint8_t op = uxn_ram[pc++], r = (op >> 6) & 1,
            *s = uxn_stk[r],   *p = &uxn_ptr[r];
	switch(op) {
	/* BRK */ case 0x00:return 1;
	/* JCI */ case 0x20:if(DEC) JUMP(c) else pc += 2; break;
	/* JMI */ case 0x40:JUMP(c) break;
	/* JSI */ case 0x60:JUMP(0) INC = pc >> 8; INC = pc; pc += c; break;
	/* LI2 */ case 0xa0:INC = uxn_ram[pc++]; /* Fall-through */
	/* LIT */ case 0x80:INC = uxn_ram[pc++]; break;
	/* L2r */ case 0xe0:INC = uxn_ram[pc++]; /* Fall-through */
	/* LIr */ case 0xc0:INC = uxn_ram[pc++]; break;
	/* INC */ OPC(0x01,DROP(a,d),PUSH(a + 1,d))
	/* POP */ OPC(0x02,*p -= 1 + d;,{})
	/* NIP */ OPC(0x03,TAKE(x) *p -= 1 + d;,GIVE(x))
	/* SWP */ OPC(0x04,TAKE(x) TAKE(y),GIVE(x) GIVE(y))
	/* ROT */ OPC(0x05,TAKE(x) TAKE(y) TAKE(z),GIVE(y) GIVE(x) GIVE(z))
	/* DUP */ OPC(0x06,TAKE(x),GIVE(x) GIVE(x))
	/* OVR */ OPC(0x07,TAKE(x) TAKE(y),GIVE(y) GIVE(x) GIVE(y))
	/* EQU */ OPC(0x08,DROP(a,d) DROP(b,d),PUSH(b == a,0))
	/* NEQ */ OPC(0x09,DROP(a,d) DROP(b,d),PUSH(b != a,0))
	/* GTH */ OPC(0x0a,DROP(a,d) DROP(b,d),PUSH(b > a,0))
	/* LTH */ OPC(0x0b,DROP(a,d) DROP(b,d),PUSH(b < a,0))
	/* JMP */ OPC(0x0c,DROP(a,d),pc = d ? a : RELA;)
	/* JCN */ OPC(0x0d,DROP(a,d) DROP(b,0),if(b) pc = d ? a : RELA;)
	/* JSR */ OPC(0x0e,DROP(a,d),FLIP PUSH(pc,1) pc = d ? a : RELA;)
	/* STH */ OPC(0x0f,TAKE(x),FLIP PUSH(x[0],0) if(d) PUSH(x[1],0))
	/* LDZ */ OPC(0x10,DROP(a,0),PEEK(a, x, 0xff) GIVE(x))
	/* STZ */ OPC(0x11,DROP(a,0) TAKE(y),POKE(a, y, 0xff))
	/* LDR */ OPC(0x12,DROP(a,0),PEEK(RELA, x, 0xffff) GIVE(x))
	/* STR */ OPC(0x13,DROP(a,0) TAKE(y),POKE(RELA, y, 0xffff))
	/* LDA */ OPC(0x14,DROP(a,1),PEEK(a, x, 0xffff) GIVE(x))
	/* STA */ OPC(0x15,DROP(a,1) TAKE(y),POKE(a, y, 0xffff))
	/* DEI */ OPC(0x16,DROP(a,0),DEVI(a, x) GIVE(x))
	/* DEO */ OPC(0x17,DROP(a,0) TAKE(y),DEVO(a, y))
	/* ADD */ OPC(0x18,DROP(a,d) DROP(b,d),PUSH(b + a,d))
	/* SUB */ OPC(0x19,DROP(a,d) DROP(b,d),PUSH(b - a,d))
	/* MUL */ OPC(0x1a,DROP(a,d) DROP(b,d),PUSH(b * a,d))
	/* DIV */ OPC(0x1b,DROP(a,d) DROP(b,d),PUSH(a ? b / a : 0,d))
	/* AND */ OPC(0x1c,DROP(a,d) DROP(b,d),PUSH(b & a,d))
	/* ORA */ OPC(0x1d,DROP(a,d) DROP(b,d),PUSH(b | a,d))
	/* EOR */ OPC(0x1e,DROP(a,d) DROP(b,d),PUSH(b ^ a,d))
	/* SFT */ OPC(0x1f,DROP(a,0) DROP(b,d),PUSH(b >> (a & 0xf) << (a >> 4),d))
	}} return 0;
}

#undef OPC
#undef DEC
#undef INC
#undef FLIP
#undef RELA
#undef JUMP
#undef DROP
#undef TAKE
#undef PUSH
#undef GIVE
#undef DEVO
#undef DEVI
#undef POKE
#undef PEEK