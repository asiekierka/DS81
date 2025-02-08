/*

    z80 - Z80 Emulator

    Copyright (C) 2006  Ian Cowburn <ianc@noddybox.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    -------------------------------------------------------------------------

    $Id: z80_decode.c,v 1.1 2006-10-04 23:05:43 ianc Exp $

*/
#include <stdlib.h>
#include <limits.h>

#include "z80.h"
#include "z80_private.h"

static const char ident[]="$Id: z80_decode.c,v 1.1 2006-10-04 23:05:43 ianc Exp $";

/* ---------------------------------------- TABLES AND INIT
*/
static Z80Byte		PSZtable[512];
static Z80Byte		SZtable[512];
static Z80Byte		Ptable[512];
static Z80Byte		Stable[512];
static Z80Byte		Ztable[512];


int Z80_HI_WORD;
int Z80_LO_WORD;

#define HI Z80_HI_WORD
#define LO Z80_LO_WORD

/* ---------------------------------------- MISC FUNCTIONS
*/
void Z80_InitialiseInternals(void)
{
    Z80Word f;
    Z80Reg r;

    /* Check endianness
    */
    r.w=0x1234;

    if (r.b[0] == 0x12)
    {
    	HI=0;
	LO=1;
    }
    else if (r.b[1] == 0x12)
    {
    	HI=1;
	LO=0;
    }
    else
    {
    	exit(1);
    }

    /* Check variable sizes
    */
    if (CHAR_BIT!=8 || sizeof(Z80Word)!=2)
    {
    	exit(2);
    }

    /* Initialise flag tables
    */
    for(f=0;f<256;f++)
    {
	Z80Byte p,z,s;
	int b;

	p=0;

	for(b=0;b<8;b++)
	    if (f&(1<<b))
		p++;

	if (p&1)
	    p=0;
	else
	    p=P_Z80;

	if (f)
	    z=0;
	else
	    z=Z_Z80;

	if (f&0x80)
	    s=S_Z80;
	else
	    s=0;

	Ptable[f]=p;
	Stable[f]=s;
	Ztable[f]=z;
	SZtable[f]=z|s;
	PSZtable[f]=z|s|p;

	Ptable[f+256]=Ptable[f]|C_Z80;
	Stable[f+256]=Stable[f]|C_Z80;
	Ztable[f+256]=Ztable[f]|C_Z80;
	SZtable[f+256]=SZtable[f]|C_Z80;
	PSZtable[f+256]=PSZtable[f]|C_Z80;
    }
}

#ifndef ENABLE_ARRAY_MEMORY
static Z80Word FPEEKW(Z80 *cpu, Z80Word addr)
{
    return (PEEK(addr) | (Z80Word)PEEK(addr+1)<<8);
}


static void FPOKEW(Z80 *cpu, Z80Word addr, Z80Word val)
{
    PRIV->mwrite(cpu,addr,val);
    PRIV->mwrite(cpu,addr+1,val>>8);
}
#endif


/* ---------------------------------------- GENERAL MACROS
*/
#define SWAP(A,B) \
do { \
    unsigned swap_tmp; \
    swap_tmp=A; \
    A=B; \
    B=swap_tmp; \
} while(0)


/* ---------------------------------------- ARITHMETIC OPS
*/
#define ADD8(ONCE) \
do { \
    Z80Byte VAL=ONCE; \
    unsigned w; \
    w=cpu->AF.b[HI]+(unsigned)VAL; \
    cpu->AF.b[LO]=SZtable[w]; \
    if ((cpu->AF.b[HI]^w^VAL)&H_Z80) cpu->AF.b[LO]|=H_Z80; \
    if ((VAL^cpu->AF.b[HI]^0x80)&(VAL^w)&0x80) cpu->AF.b[LO]|=P_Z80; \
    SETHIDDEN(w); \
    cpu->AF.b[HI]=w; \
} while(0)


#define ADC8(ONCE) \
do { \
    Z80Byte VAL=ONCE; \
    unsigned w; \
    w=(cpu->AF.b[HI]+(unsigned)VAL+CARRY)&0x1ff; \
    cpu->AF.b[LO]=SZtable[w]; \
    if ((cpu->AF.b[HI]^w^VAL)&H_Z80) cpu->AF.b[LO]|=H_Z80; \
    if ((VAL^cpu->AF.b[HI]^0x80)&(VAL^w)&0x80) cpu->AF.b[LO]|=P_Z80; \
    SETHIDDEN(w); \
    cpu->AF.b[HI]=w; \
} while(0)


#define SUB8(ONCE) \
do { \
    Z80Byte VAL=ONCE; \
    unsigned w; \
    w=(cpu->AF.b[HI]-(unsigned)VAL)&0x1ff; \
    cpu->AF.b[LO]=SZtable[w]|N_Z80; \
    if ((cpu->AF.b[HI]^w^VAL)&H_Z80) cpu->AF.b[LO]|=H_Z80; \
    if ((VAL^cpu->AF.b[HI])&(cpu->AF.b[HI]^w)&0x80) cpu->AF.b[LO]|=P_Z80; \
    SETHIDDEN(w); \
    cpu->AF.b[HI]=w; \
} while(0)


#define CMP8(ONCE) \
do { \
    Z80Byte VAL=ONCE; \
    unsigned w; \
    w=(cpu->AF.b[HI]-(unsigned)VAL)&0x1ff; \
    cpu->AF.b[LO]=SZtable[w]|N_Z80; \
    if ((cpu->AF.b[HI]^w^VAL)&H_Z80) cpu->AF.b[LO]|=H_Z80; \
    if ((VAL^cpu->AF.b[HI])&(cpu->AF.b[HI]^w)&0x80) cpu->AF.b[LO]|=P_Z80; \
    SETHIDDEN(VAL); \
} while(0)


#define SBC8(ONCE) \
do { \
    Z80Byte VAL=ONCE; \
    unsigned w; \
    w=(cpu->AF.b[HI]-(unsigned)VAL-CARRY)&0x1ff; \
    cpu->AF.b[LO]=SZtable[w]|N_Z80; \
    if ((cpu->AF.b[HI]^w^VAL)&H_Z80) cpu->AF.b[LO]|=H_Z80; \
    if ((VAL^cpu->AF.b[HI])&(cpu->AF.b[HI]^w)&0x80) cpu->AF.b[LO]|=P_Z80; \
    SETHIDDEN(w); \
    cpu->AF.b[HI]=w; \
} while(0)


#define ADD16(REG,ONCE) \
do { \
    Z80Word VAL=ONCE; \
    Z80Val w; \
    w=(REG)+(Z80Val)VAL; \
    cpu->AF.b[LO]&=(S_Z80|Z_Z80|V_Z80); \
    if (w>0xffff) cpu->AF.b[LO]|=C_Z80; \
    if (((REG)^w^VAL)&0x1000) cpu->AF.b[LO]|=H_Z80; \
    SETHIDDEN(w>>8); \
    (REG)=w; \
} while(0) 


#define ADC16(REG, ONCE) \
do { \
    Z80Word VAL=ONCE; \
    Z80Val w; \
    w=(REG)+(Z80Val)VAL+CARRY; \
    cpu->AF.b[LO]=0; \
    if ((w&0xffff)==0) cpu->AF.b[LO]=Z_Z80; \
    if (w&0x8000) cpu->AF.b[LO]|=S_Z80; \
    if (w>0xffff) cpu->AF.b[LO]|=C_Z80; \
    if ((VAL^(REG)^0x8000)&((REG)^w)&0x8000) cpu->AF.b[LO]|=P_Z80; \
    if (((REG)^w^VAL)&0x1000) cpu->AF.b[LO]|=H_Z80; \
    SETHIDDEN(w>>8); \
    (REG)=w; \
} while(0) 


#define SBC16(REG, ONCE) \
do { \
    Z80Word VAL=ONCE; \
    Z80Val w; \
    w=(REG)-(Z80Val)VAL-CARRY; \
    cpu->AF.b[LO]=N_Z80; \
    if (w&0x8000) cpu->AF.b[LO]|=S_Z80; \
    if ((w&0xffff)==0) cpu->AF.b[LO]|=Z_Z80; \
    if (w>0xffff) cpu->AF.b[LO]|=C_Z80; \
    if ((VAL^(REG))&((REG)^w)&0x8000) cpu->AF.b[LO]|=P_Z80; \
    if (((REG)^w^VAL)&0x1000) cpu->AF.b[LO]|=H_Z80; \
    SETHIDDEN(w>>8); \
    (REG)=w; \
} while(0)


#define INC8(REG) \
do { \
    (REG)++; \
    cpu->AF.b[LO]=CARRY|SZtable[(REG)]; \
    if ((REG)==0x80) cpu->AF.b[LO]|=P_Z80; \
    if (((REG)&0x0f)==0) cpu->AF.b[LO]|=H_Z80; \
} while(0) 


#define DEC8(REG) \
do { \
    (REG)--; \
    cpu->AF.b[LO]=N_Z80|CARRY; \
    if ((REG)==0x7f) cpu->AF.b[LO]|=P_Z80; \
    if (((REG)&0x0f)==0x0f) cpu->AF.b[LO]|=H_Z80; \
    cpu->AF.b[LO]|=SZtable[(REG)]; \
} while(0)


#define OP_ON_MEM(OP,addr) \
do { \
    Z80Byte memop=PEEK(addr); \
    OP(memop); \
    POKE(addr,memop); \
} while(0)


#define OP_ON_MEM_WITH_ARG(OP,addr,arg) \
do { \
    Z80Byte memop=PEEK(addr); \
    OP(memop,arg); \
    POKE(addr,memop); \
} while(0)


#define OP_ON_MEM_WITH_COPY(OP,addr,copy) \
do { \
    Z80Byte memop=PEEK(addr); \
    OP(memop); \
    copy=memop; \
    POKE(addr,memop); \
} while(0)


#define OP_ON_MEM_WITH_ARG_AND_COPY(OP,addr,arg,copy) \
do { \
    Z80Byte memop=PEEK(addr); \
    OP(memop,arg); \
    copy=memop; \
    POKE(addr,memop); \
} while(0)


/* ---------------------------------------- ROTATE AND SHIFT OPS
*/
#define RRCA \
do { \
    cpu->AF.b[LO]=(cpu->AF.b[LO]&(S_Z80|Z_Z80|P_Z80))|(cpu->AF.b[HI]&C_Z80); \
    cpu->AF.b[HI]=(cpu->AF.b[HI]>>1)|(cpu->AF.b[HI]<<7); \
    SETHIDDEN(cpu->AF.b[HI]); \
} while(0)


#define RRA \
do { \
    Z80Byte c; \
    c=CARRY; \
    cpu->AF.b[LO]=(cpu->AF.b[LO]&(S_Z80|Z_Z80|P_Z80))|(cpu->AF.b[HI]&C_Z80); \
    cpu->AF.b[HI]=(cpu->AF.b[HI]>>1)|(c<<7); \
    SETHIDDEN(cpu->AF.b[HI]); \
} while(0)


#define RRC(REG) \
do { \
    Z80Byte c; \
    c=(REG)&C_Z80; \
    (REG)=((REG)>>1)|((REG)<<7); \
    cpu->AF.b[LO]=PSZtable[(REG)]|c; \
    SETHIDDEN(REG); \
} while(0) 


#define RR(REG) \
do { \
    Z80Byte c; \
    c=(REG)&C_Z80; \
    (REG)=((REG)>>1)|(CARRY<<7); \
    cpu->AF.b[LO]=PSZtable[(REG)]|c; \
    SETHIDDEN(REG); \
} while(0)


#define RLCA \
do { \
    cpu->AF.b[LO]=(cpu->AF.b[LO]&(S_Z80|Z_Z80|P_Z80))|(cpu->AF.b[HI]>>7); \
    cpu->AF.b[HI]=(cpu->AF.b[HI]<<1)|(cpu->AF.b[HI]>>7); \
    SETHIDDEN(cpu->AF.b[HI]); \
} while(0)


#define RLA \
do { \
    Z80Byte c; \
    c=CARRY; \
    cpu->AF.b[LO]=(cpu->AF.b[LO]&(S_Z80|Z_Z80|P_Z80))|(cpu->AF.b[HI]>>7); \
    cpu->AF.b[HI]=(cpu->AF.b[HI]<<1)|c; \
    SETHIDDEN(cpu->AF.b[HI]); \
} while(0)


#define RLC(REG) \
do { \
    Z80Byte c; \
    c=(REG)>>7; \
    (REG)=((REG)<<1)|c; \
    cpu->AF.b[LO]=PSZtable[(REG)]|c; \
    SETHIDDEN(REG); \
} while(0)


#define RL(REG) \
do { \
    Z80Byte c; \
    c=(REG)>>7; \
    (REG)=((REG)<<1)|CARRY; \
    cpu->AF.b[LO]=PSZtable[(REG)]|c; \
    SETHIDDEN(REG); \
} while(0)


#define SRL(REG) \
do { \
    Z80Byte c; \
    c=(REG)&C_Z80; \
    (REG)>>=1; \
    cpu->AF.b[LO]=PSZtable[(REG)]|c; \
    SETHIDDEN(REG); \
} while(0)


#define SRA(REG) \
do { \
    Z80Byte c; \
    c=(REG)&C_Z80; \
    (REG)=((REG)>>1)|((REG)&0x80); \
    cpu->AF.b[LO]=PSZtable[(REG)]|c; \
    SETHIDDEN(REG); \
} while(0)


#define SLL(REG) \
do { \
    Z80Byte c; \
    c=(REG)>>7; \
    (REG)=((REG)<<1)|1; \
    cpu->AF.b[LO]=PSZtable[(REG)]|c; \
    SETHIDDEN(REG); \
} while(0)


#define SLA(REG) \
do { \
    Z80Byte c; \
    c=(REG)>>7; \
    (REG)=(REG)<<1; \
    cpu->AF.b[LO]=PSZtable[(REG)]|c; \
    SETHIDDEN(REG); \
} while(0)


/* ---------------------------------------- BOOLEAN OPS
*/
#define AND(VAL) \
do { \
    cpu->AF.b[HI]&=VAL; \
    cpu->AF.b[LO]=PSZtable[cpu->AF.b[HI]]|H_Z80; \
    SETHIDDEN(cpu->AF.b[HI]); \
} while(0)


#define OR(VAL) \
do { \
    cpu->AF.b[HI]|=VAL; \
    cpu->AF.b[LO]=PSZtable[cpu->AF.b[HI]]; \
    SETHIDDEN(cpu->AF.b[HI]); \
} while(0)


#define XOR(VAL) \
do { \
    cpu->AF.b[HI]^=VAL; \
    cpu->AF.b[LO]=PSZtable[cpu->AF.b[HI]]; \
    SETHIDDEN(cpu->AF.b[HI]); \
} while(0)


#define BIT(REG,B) \
do { \
    cpu->AF.b[LO]=CARRY|H_Z80; \
    if ((REG)&(1<<B)) \
    { \
	if (B==7 && (REG&S_Z80)) cpu->AF.b[LO]|=S_Z80; \
	if (B==5 && (REG&B5_Z80)) cpu->AF.b[LO]|=B5_Z80; \
	if (B==3 && (REG&B3_Z80)) cpu->AF.b[LO]|=B3_Z80; \
    } \
    else \
    { \
	cpu->AF.b[LO]|=Z_Z80; \
	cpu->AF.b[LO]|=P_Z80; \
    } \
} while(0)

#define BIT_SET(REG,B) (REG)|=(1<<B)
#define BIT_RES(REG,B) (REG)&=~(1<<B)


/* ---------------------------------------- JUMP OPERATIONS
*/
#define JR_COND(COND) \
do { \
    if (COND) \
    { \
	TSTATE(12); \
	JR; \
    } \
    else \
    { \
	TSTATE(7); \
	NOJR; \
    } \
} while(0)


#define JP_COND(COND) \
do { \
    TSTATE(10); \
    if (COND) \
    { \
	JP; \
    } \
    else \
    { \
	NOJP; \
    } \
} while(0)


#define CALL_COND(COND) \
do { \
    if (COND) \
    { \
	TSTATE(17); \
	CALL; \
    } \
    else \
    { \
	TSTATE(10); \
	NOCALL; \
    } \
} while(0)


#define RET_COND(COND) \
do { \
    if (COND) \
    { \
	TSTATE(11); \
	POP(cpu->PC); \
    } \
    else \
    { \
	TSTATE(5); \
    } \
} while(0)


#define RST(ADDR) \
    TSTATE(11); \
    PUSH(cpu->PC); \
    cpu->PC=ADDR

/* ---------------------------------------- BLOCK OPERATIONS
*/
#define LDI \
do { \
    Z80Byte b; \
 \
    b=PEEK(cpu->HL.w); \
    POKE(cpu->DE.w,b); \
    cpu->DE.w++; \
    cpu->HL.w++; \
    cpu->BC.w--; \
 \
    CLRFLAG(H_Z80); \
    CLRFLAG(N_Z80); \
 \
    if (cpu->BC.w) \
	SETFLAG(P_Z80); \
    else \
	CLRFLAG(P_Z80); \
 \
    SETHIDDEN(cpu->AF.b[HI]+b); \
} while(0)

#define LDD \
do { \
    Z80Byte b; \
 \
    b=PEEK(cpu->HL.w); \
    POKE(cpu->DE.w,b); \
    cpu->DE.w--; \
    cpu->HL.w--; \
    cpu->BC.w--; \
 \
    CLRFLAG(H_Z80); \
    CLRFLAG(N_Z80); \
 \
    if (cpu->BC.w) \
	SETFLAG(P_Z80); \
    else \
	CLRFLAG(P_Z80); \
 \
    SETHIDDEN(cpu->AF.b[HI]+b); \
} while(0)

#define CPI \
do { \
    Z80Byte c,b; \
 \
    c=CARRY; \
    b=PEEK(cpu->HL.w); \
 \
    CMP8(b); \
 \
    if (c) \
    	SETFLAG(C_Z80); \
    else \
    	CLRFLAG(C_Z80); \
 \
    cpu->HL.w++; \
    cpu->BC.w--; \
 \
    if (cpu->BC.w) \
	SETFLAG(P_Z80); \
    else \
	CLRFLAG(P_Z80); \
} while(0)

#define CPD \
do { \
    Z80Byte c,b; \
 \
    c=CARRY; \
    b=PEEK(cpu->HL.w); \
 \
    CMP8(b); \
 \
    if (c) \
    	SETFLAG(C_Z80); \
    else \
    	CLRFLAG(C_Z80); \
 \
    cpu->HL.w--; \
    cpu->BC.w--; \
 \
    if (cpu->BC.w) \
	SETFLAG(P_Z80); \
    else \
	CLRFLAG(P_Z80); \
} while(0)

#define INI \
do { \
    Z80Word w; \
    Z80Byte b; \
 \
    b=IN(cpu->BC.w); \
    POKE(cpu->HL.w,b); \
 \
    cpu->BC.b[HI]--; \
    cpu->HL.w++; \
 \
    cpu->AF.b[LO]=SZtable[cpu->BC.b[HI]]; \
    SETHIDDEN(cpu->BC.b[HI]); \
 \
    w=(((Z80Word)cpu->BC.b[LO])&0xff)+b; \
 \
    if (b&0x80) \
    	SETFLAG(N_Z80); \
 \
    if (w&0x100) \
    { \
    	SETFLAG(C_Z80); \
    	SETFLAG(H_Z80); \
    } \
    else \
    { \
    	CLRFLAG(C_Z80); \
    	CLRFLAG(H_Z80); \
    } \
} while(0)

#define IND \
do { \
    Z80Word w; \
    Z80Byte b; \
 \
    b=IN(cpu->BC.w); \
    POKE(cpu->HL.w,b); \
 \
    cpu->BC.b[HI]--; \
    cpu->HL.w--; \
 \
    cpu->AF.b[LO]=SZtable[cpu->BC.b[HI]]; \
    SETHIDDEN(cpu->BC.b[HI]); \
 \
    w=(((Z80Word)cpu->BC.b[LO])&0xff)+b; \
 \
    if (b&0x80) \
    	SETFLAG(N_Z80); \
 \
    if (w&0x100) \
    { \
    	SETFLAG(C_Z80); \
    	SETFLAG(H_Z80); \
    } \
    else \
    { \
    	CLRFLAG(C_Z80); \
    	CLRFLAG(H_Z80); \
    } \
} while(0) \

#define OUTI \
do { \
    OUT(cpu->BC.w,PEEK(cpu->HL.w)); \
 \
    cpu->HL.w++; \
    cpu->BC.b[HI]--; \
 \
    cpu->AF.b[LO]=SZtable[cpu->BC.b[HI]]; \
    SETHIDDEN(cpu->BC.b[HI]); \
} while(0)

#define OUTD \
do { \
    OUT(cpu->BC.w,PEEK(cpu->HL.w)); \
 \
    cpu->HL.w--; \
    cpu->BC.b[HI]--; \
 \
    cpu->AF.b[LO]=SZtable[cpu->BC.b[HI]]; \
    SETFLAG(N_Z80); \
    SETHIDDEN(cpu->BC.b[HI]); \
} while(0)


/* ---------------------------------------- BASE OPCODE SHORT-HAND BLOCKS
*/

#define LD_BLOCK(BASE,DEST,DEST2) \
    case BASE:		/* LD DEST,B */ \
	TSTATE(4); \
	DEST=cpu->BC.b[HI]; \
	break; \
 \
    case BASE+1:	/* LD DEST,C */ \
	TSTATE(4); \
	DEST=cpu->BC.b[LO]; \
	break; \
 \
    case BASE+2:	/* LD DEST,D */ \
	TSTATE(4); \
	DEST=cpu->DE.b[HI]; \
	break; \
 \
    case BASE+3:	/* LD DEST,E */ \
	TSTATE(4); \
	DEST=cpu->DE.b[LO]; \
	break; \
 \
    case BASE+4:	/* LD DEST,H */ \
	TSTATE(4); \
	DEST=*H; \
	break; \
 \
    case BASE+5:	/* LD DEST,L */ \
	TSTATE(4); \
	DEST=*L; \
	break; \
 \
    case BASE+6:	/* LD DEST,(HL) */ \
	TSTATE(7); \
	OFFSET(off); \
	DEST2=PEEK(*HL+off); \
	break; \
 \
    case BASE+7:	/* LD DEST,A */ \
	TSTATE(4); \
	DEST=cpu->AF.b[HI]; \
	break;

#define ALU_BLOCK(BASE,OP) \
    case BASE:		/* OP A,B */ \
	TSTATE(4); \
	OP(cpu->BC.b[HI]); \
	break; \
 \
    case BASE+1:	/* OP A,C */ \
	TSTATE(4); \
	OP(cpu->BC.b[LO]); \
	break; \
 \
    case BASE+2:	/* OP A,D */ \
	TSTATE(4); \
	OP(cpu->DE.b[HI]); \
	break; \
 \
    case BASE+3:	/* OP A,E */ \
	TSTATE(4); \
	OP(cpu->DE.b[LO]); \
	break; \
 \
    case BASE+4:	/* OP A,H */ \
	TSTATE(4); \
	OP(*H); \
	break; \
 \
    case BASE+5:	/* OP A,L */ \
	TSTATE(4); \
	OP(*L); \
	break; \
 \
    case BASE+6:	/* OP A,(HL) */ \
	TSTATE(7); \
	OFFSET(off); \
	OP_ON_MEM(OP,*HL+off); \
	break; \
 \
    case BASE+7:	/* OP A,A */ \
	TSTATE(4); \
	OP(cpu->AF.b[HI]); \
	break;


/* ---------------------------------------- CB OPCODE SHORT-HAND BLOCKS
*/

#define CB_ALU_BLOCK(BASE,OP) \
    case BASE:		/* OP B */ \
	TSTATE(8); \
	OP(cpu->BC.b[HI]); \
	break; \
 \
    case BASE+1:	/* OP C */ \
	TSTATE(8); \
	OP(cpu->BC.b[LO]); \
	break; \
 \
    case BASE+2:	/* OP D */ \
	TSTATE(8); \
	OP(cpu->DE.b[HI]); \
	break; \
 \
    case BASE+3:	/* OP E */ \
	TSTATE(8); \
	OP(cpu->DE.b[LO]); \
	break; \
 \
    case BASE+4:	/* OP H */ \
	TSTATE(8); \
	OP(cpu->HL.b[HI]); \
	break; \
 \
    case BASE+5:	/* OP L */ \
	TSTATE(8); \
	OP(cpu->HL.b[LO]); \
	break; \
 \
    case BASE+6:	/* OP (HL) */ \
	TSTATE(15); \
	OP_ON_MEM(OP,cpu->HL.w); \
	break; \
 \
    case BASE+7:	/* OP A */ \
	TSTATE(8); \
	OP(cpu->AF.b[HI]); \
	break;

#define CB_BITMANIP_BLOCK(BASE,OP,BIT_NO) \
    case BASE:		/* OP B */ \
	TSTATE(8); \
	OP(cpu->BC.b[HI],BIT_NO); \
	break; \
 \
    case BASE+1:	/* OP C */ \
	TSTATE(8); \
	OP(cpu->BC.b[LO],BIT_NO); \
	break; \
 \
    case BASE+2:	/* OP D */ \
	TSTATE(8); \
	OP(cpu->DE.b[HI],BIT_NO); \
	break; \
 \
    case BASE+3:	/* OP E */ \
	TSTATE(8); \
	OP(cpu->DE.b[LO],BIT_NO); \
	break; \
 \
    case BASE+4:	/* OP H */ \
	TSTATE(8); \
	OP(cpu->HL.b[HI],BIT_NO); \
	break; \
 \
    case BASE+5:	/* OP L */ \
	TSTATE(8); \
	OP(cpu->HL.b[LO],BIT_NO); \
	break; \
 \
    case BASE+6:	/* OP (HL) */ \
	TSTATE(12); \
	OP_ON_MEM_WITH_ARG(OP,cpu->HL.w,BIT_NO); \
	break; \
 \
    case BASE+7:	/* OP A */ \
	TSTATE(8); \
	OP(cpu->AF.b[HI],BIT_NO); \
	break;

/* ---------------------------------------- SHIFTED CB OPCODE SHORT-HAND BLOCKS
*/

#define SHIFTED_CB_ALU_BLOCK(BASE,OP) \
    case BASE:		/* OP B */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_COPY(OP,addr,cpu->BC.b[HI]); \
	break; \
 \
    case BASE+1:	/* OP C */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_COPY(OP,addr,cpu->BC.b[LO]); \
	break; \
 \
    case BASE+2:	/* OP D */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_COPY(OP,addr,cpu->DE.b[HI]); \
	break; \
 \
    case BASE+3:	/* OP E */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_COPY(OP,addr,cpu->DE.b[LO]); \
	break; \
 \
    case BASE+4:	/* OP H */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_COPY(OP,addr,cpu->HL.b[HI]); \
	break; \
 \
    case BASE+5:	/* OP L */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_COPY(OP,addr,cpu->HL.b[LO]); \
	break; \
 \
    case BASE+6:	/* OP (HL) */ \
	TSTATE(15); \
	OP_ON_MEM(OP,addr); \
	break; \
 \
    case BASE+7:	/* OP A */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_COPY(OP,addr,cpu->AF.b[HI]); \
	break;

#define SHIFTED_CB_BITMANIP_BLOCK(BASE,OP,BIT_NO) \
    case BASE:		/* OP B */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_ARG_AND_COPY(OP,addr,BIT_NO,cpu->BC.b[HI]); \
	break; \
 \
    case BASE+1:	/* OP C */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_ARG_AND_COPY(OP,addr,BIT_NO,cpu->BC.b[LO]); \
	break; \
 \
    case BASE+2:	/* OP D */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_ARG_AND_COPY(OP,addr,BIT_NO,cpu->DE.b[HI]); \
	break; \
 \
    case BASE+3:	/* OP E */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_ARG_AND_COPY(OP,addr,BIT_NO,cpu->DE.b[LO]); \
	break; \
 \
    case BASE+4:	/* OP H */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_ARG_AND_COPY(OP,addr,BIT_NO,cpu->HL.b[HI]); \
	break; \
 \
    case BASE+5:	/* OP L */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_ARG_AND_COPY(OP,addr,BIT_NO,cpu->HL.b[LO]); \
	break; \
 \
    case BASE+6:	/* OP (HL) */ \
	TSTATE(12); \
	OP_ON_MEM_WITH_ARG(OP,addr,BIT_NO); \
	break; \
 \
    case BASE+7:	/* OP A */ \
	TSTATE(8); \
	OP_ON_MEM_WITH_ARG_AND_COPY(OP,addr,BIT_NO,cpu->AF.b[HI]); \
	break;

/* ---------------------------------------- DAA
*/

/* This alogrithm is based on info from
   http://www.worldofspectrum.org/faq/reference/z80reference.htm
*/
static void DAA (Z80 *cpu)
{
    Z80Byte add=0;
    Z80Byte carry=0;
    Z80Byte nf=cpu->AF.b[LO]&N_Z80;
    Z80Byte acc=cpu->AF.b[HI];

    if (acc>0x99 || IS_C)
    {
	add|=0x60;
	carry=C_Z80;
    }

    if ((acc&0xf)>0x9 || IS_H)
    {
    	add|=0x06;
    }

    if (nf)
    {
    	cpu->AF.b[HI]-=add;
    }
    else
    {
    	cpu->AF.b[HI]+=add;
    }

    cpu->AF.b[LO]=PSZtable[cpu->AF.b[HI]]
		    | carry
		    | nf
		    | ((acc^cpu->AF.b[HI])&H_Z80)
		    | (cpu->AF.b[HI]&(B3_Z80|B5_Z80));
}

/* ---------------------------------------- HANDLERS FOR ED OPCODES
*/
static void DecodeED(Z80 *cpu, Z80Byte opcode)
{
    switch(opcode)
    {
	case 0x40:	/* IN B,(C) */
	    TSTATE(12);

	    if (PRIV->pread)
	    {
		cpu->BC.b[HI]=PRIV->pread(cpu,cpu->BC.w);
	    }
	    else
	    {
	    	cpu->BC.b[HI]=0;
	    }

	    cpu->AF.b[LO]=CARRY|PSZtable[cpu->BC.b[HI]];
	    SETHIDDEN(cpu->BC.b[HI]);
	    break;

	case 0x41:	/* OUT (C),B */
	    TSTATE(12);
	    if (PRIV->pwrite) PRIV->pwrite(cpu,cpu->BC.w,cpu->BC.b[HI]);
	    break;

	case 0x42:	/* SBC HL,BC */
	    TSTATE(15);
	    SBC16(cpu->HL.w,cpu->BC.w);
	    break;

	case 0x43:	/* LD (nnnn),BC */
	    TSTATE(20);
	    POKEW(FETCH_WORD,cpu->BC.w);
	    break;

	case 0x44:	/* NEG */
	    {
	    Z80Byte b;

	    TSTATE(8);

	    b=cpu->AF.b[HI];
	    cpu->AF.b[HI]=0;
	    SUB8(b);
	    break;
	    }

	case 0x45:	/* RETN */
	    TSTATE(14);
	    cpu->IFF1=cpu->IFF2;
	    POP(cpu->PC);
	    break;

	case 0x46:	/* IM 0 */
	    TSTATE(8);
	    cpu->IM=0;
	    break;

	case 0x47:	/* LD I,A */
	    TSTATE(9);
	    cpu->I=cpu->AF.b[HI];
	    break;

	case 0x48:	/* IN C,(C) */
	    TSTATE(12);

	    if (PRIV->pread)
	    {
		cpu->BC.b[LO]=PRIV->pread(cpu,cpu->BC.w);
	    }
	    else
	    {
	    	cpu->BC.b[LO]=0;
	    }

	    cpu->AF.b[LO]=CARRY|PSZtable[cpu->BC.b[LO]];
	    SETHIDDEN(cpu->BC.b[LO]);
	    break;

	case 0x49:	/* OUT (C),C */
	    TSTATE(12);
	    if (PRIV->pwrite) PRIV->pwrite(cpu,cpu->BC.w,cpu->BC.b[LO]);
	    break;

	case 0x4a:	/* ADC HL,BC */
	    TSTATE(15);
	    ADC16(cpu->HL.w,cpu->BC.w);
	    break;

	case 0x4b:	/* LD BC,(nnnn) */
	    TSTATE(20);
	    cpu->BC.w=PEEKW(FETCH_WORD);
	    break;

	case 0x4c:	/* NEG */
	    {
	    Z80Byte b;

	    TSTATE(8);

	    b=cpu->AF.b[HI];
	    cpu->AF.b[HI]=0;
	    SUB8(b);
	    break;
	    }

	case 0x4d:	/* RETI */
	    TSTATE(14);
	    CALLBACK(eZ80_RETI,0);
	    cpu->IFF1=cpu->IFF2;
	    POP(cpu->PC);
	    break;

	case 0x4e:	/* IM 0/1 */
	    TSTATE(8);
	    cpu->IM=0;
	    break;

	case 0x4f:	/* LD R,A */
	    TSTATE(9);
	    cpu->R=cpu->AF.b[HI];
	    break;

	case 0x50:	/* IN D,(C) */
	    TSTATE(12);

	    if (PRIV->pread)
	    {
		cpu->DE.b[HI]=PRIV->pread(cpu,cpu->BC.w);
	    }
	    else
	    {
	    	cpu->DE.b[HI]=0;
	    }

	    cpu->AF.b[LO]=CARRY|PSZtable[cpu->DE.b[HI]];
	    SETHIDDEN(cpu->BC.b[HI]);
	    break;

	case 0x51:	/* OUT (C),D */
	    TSTATE(12);
	    if (PRIV->pwrite) PRIV->pwrite(cpu,cpu->BC.w,cpu->DE.b[HI]);
	    break;

	case 0x52:	/* SBC HL,DE */
	    TSTATE(15);
	    SBC16(cpu->HL.w,cpu->DE.w);
	    break;

	case 0x53:	/* LD (nnnn),DE */
	    TSTATE(20);
	    POKEW(FETCH_WORD,cpu->DE.w);
	    break;

	case 0x54:	/* NEG */
	    {
	    Z80Byte b;

	    TSTATE(8);

	    b=cpu->AF.b[HI];
	    cpu->AF.b[HI]=0;
	    SUB8(b);
	    break;
	    }

	case 0x55:	/* RETN */
	    TSTATE(14);
	    cpu->IFF1=cpu->IFF2;
	    POP(cpu->PC);
	    break;

	case 0x56:	/* IM 1 */
	    TSTATE(8);
	    cpu->IM=1;
	    break;

	case 0x57:	/* LD A,I */
	    TSTATE(9);
	    cpu->AF.b[HI]=cpu->I;
	    break;

	case 0x58:	/* IN E,(C) */
	    TSTATE(12);

	    if (PRIV->pread)
	    {
		cpu->DE.b[LO]=PRIV->pread(cpu,cpu->BC.w);
	    }
	    else
	    {
	    	cpu->BC.b[LO]=0;
	    }

	    cpu->AF.b[LO]=CARRY|PSZtable[cpu->DE.b[LO]];
	    SETHIDDEN(cpu->DE.b[LO]);
	    break;

	case 0x59:	/* OUT (C),E */
	    TSTATE(12);
	    if (PRIV->pwrite) PRIV->pwrite(cpu,cpu->BC.w,cpu->DE.b[LO]);
	    break;

	case 0x5a:	/* ADC HL,DE */
	    TSTATE(15);
	    ADC16(cpu->HL.w,cpu->DE.w);
	    break;

	case 0x5b:	/* LD DE,(nnnn) */
	    TSTATE(20);
	    cpu->DE.w=PEEKW(FETCH_WORD);
	    break;

	case 0x5c:	/* NEG */
	    {
	    Z80Byte b;

	    TSTATE(8);

	    b=cpu->AF.b[HI];
	    cpu->AF.b[HI]=0;
	    SUB8(b);
	    break;
	    }

	case 0x5d:	/* RETN */
	    TSTATE(14);
	    cpu->IFF1=cpu->IFF2;
	    POP(cpu->PC);
	    break;

	case 0x5e:	/* IM 2 */
	    TSTATE(8);
	    cpu->IM=2;
	    break;

	case 0x5f:	/* LD A,R */
	    TSTATE(9);
	    cpu->AF.b[HI]=cpu->R;
	    break;

	case 0x60:	/* IN H,(C) */
	    TSTATE(12);

	    if (PRIV->pread)
	    {
		cpu->HL.b[HI]=PRIV->pread(cpu,cpu->BC.w);
	    }
	    else
	    {
	    	cpu->HL.b[HI]=0;
	    }

	    cpu->AF.b[LO]=CARRY|PSZtable[cpu->HL.b[HI]];
	    SETHIDDEN(cpu->HL.b[HI]);
	    break;

	case 0x61:	/* OUT (C),H */
	    TSTATE(12);
	    if (PRIV->pwrite) PRIV->pwrite(cpu,cpu->BC.w,cpu->HL.b[HI]);
	    break;

	case 0x62:	/* SBC HL,HL */
	    TSTATE(15);
	    SBC16(cpu->HL.w,cpu->HL.w);
	    break;

	case 0x63:	/* LD (nnnn),HL */
	    TSTATE(20);
	    POKEW(FETCH_WORD,cpu->HL.w);
	    break;

	case 0x64:	/* NEG */
	    {
	    Z80Byte b;

	    TSTATE(8);

	    b=cpu->AF.b[HI];
	    cpu->AF.b[HI]=0;
	    SUB8(b);
	    break;
	    }

	case 0x65:	/* RETN */
	    TSTATE(14);
	    cpu->IFF1=cpu->IFF2;
	    POP(cpu->PC);
	    break;

	case 0x66:	/* IM 0 */
	    TSTATE(8);
	    cpu->IM=0;
	    break;

	case 0x67:	/* RRD */
	    {
	    Z80Byte b;

	    TSTATE(18);

	    b=PEEK(cpu->HL.w);

	    POKE(cpu->HL.w,(b>>4)|(cpu->AF.b[HI]<<4));
	    cpu->AF.b[HI]=(cpu->AF.b[HI]&0xf0)|(b&0x0f);

	    cpu->AF.b[LO]=CARRY|PSZtable[cpu->AF.b[HI]];
	    SETHIDDEN(cpu->AF.b[HI]);
	    break;
	    }

	case 0x68:	/* IN L,(C) */
	    TSTATE(12);

	    if (PRIV->pread)
	    {
		cpu->HL.b[LO]=PRIV->pread(cpu,cpu->BC.w);
	    }
	    else
	    {
	    	cpu->HL.b[LO]=0;
	    }

	    cpu->AF.b[LO]=CARRY|PSZtable[cpu->HL.b[LO]];
	    SETHIDDEN(cpu->HL.b[LO]);
	    break;

	case 0x69:	/* OUT (C),L */
	    TSTATE(12);
	    if (PRIV->pwrite) PRIV->pwrite(cpu,cpu->BC.w,cpu->HL.b[LO]);
	    break;

	case 0x6a:	/* ADC HL,HL */
	    TSTATE(15);
	    ADC16(cpu->HL.w,cpu->HL.w);
	    break;

	case 0x6b:	/* LD HL,(nnnn) */
	    TSTATE(20);
	    cpu->HL.w=PEEKW(FETCH_WORD);
	    break;

	case 0x6c:	/* NEG */
	    {
	    Z80Byte b;

	    TSTATE(8);

	    b=cpu->AF.b[HI];
	    cpu->AF.b[HI]=0;
	    SUB8(b);
	    break;
	    }

	case 0x6d:	/* RETN */
	    TSTATE(14);
	    cpu->IFF1=cpu->IFF2;
	    POP(cpu->PC);
	    break;

	case 0x6e:	/* IM 0/1 */
	    TSTATE(8);
	    cpu->IM=0;
	    break;

	case 0x6f:	/* RLD */
	    {
	    Z80Byte b;

	    TSTATE(18);

	    b=PEEK(cpu->HL.w);

	    POKE(cpu->HL.w,(b<<4)|(cpu->AF.b[HI]&0x0f));
	    cpu->AF.b[HI]=(cpu->AF.b[HI]&0xf0)|(b>>4);

	    cpu->AF.b[LO]=CARRY|PSZtable[cpu->AF.b[HI]];
	    SETHIDDEN(cpu->AF.b[HI]);
	    break;
	    }

	case 0x70:	/* IN (C) */
	    {
	    Z80Byte b;

	    TSTATE(12);

	    if (PRIV->pread)
	    {
		b=PRIV->pread(cpu,cpu->BC.w);
	    }
	    else
	    {
	    	b=0;
	    }

	    cpu->AF.b[LO]=CARRY|PSZtable[b];
	    SETHIDDEN(b);
	    break;
	    }

	case 0x71:	/* OUT (C) */
	    TSTATE(12);
	    if (PRIV->pwrite) PRIV->pwrite(cpu,cpu->BC.w,0);
	    break;

	case 0x72:	/* SBC HL,SP */
	    TSTATE(15);
	    SBC16(cpu->HL.w,cpu->SP);
	    break;

	case 0x73:	/* LD (nnnn),SP */
	    TSTATE(20);
	    POKEW(FETCH_WORD,cpu->SP);
	    break;

	case 0x74:	/* NEG */
	    {
	    Z80Byte b;

	    TSTATE(8);

	    b=cpu->AF.b[HI];
	    cpu->AF.b[HI]=0;
	    SUB8(b);
	    break;
	    }

	case 0x75:	/* RETN */
	    TSTATE(14);
	    cpu->IFF1=cpu->IFF2;
	    POP(cpu->PC);
	    break;

	case 0x76:	/* IM 1 */
	    TSTATE(8);
	    cpu->IM=1;
	    break;

	case 0x77:	/* NOP */
	    TSTATE(8);
	    CALLBACK(eZ80_EDHook,opcode);
	    break;

	case 0x78:	/* IN A,(C) */
	    TSTATE(12);

	    if (PRIV->pread)
	    {
		cpu->AF.b[HI]=PRIV->pread(cpu,cpu->BC.w);
	    }
	    else
	    {
	    	cpu->AF.b[HI]=0;
	    }

	    cpu->AF.b[LO]=CARRY|PSZtable[cpu->AF.b[HI]];
	    SETHIDDEN(cpu->AF.b[HI]);
	    break;

	case 0x79:	/* OUT (C),A */
	    TSTATE(12);
	    if (PRIV->pwrite) PRIV->pwrite(cpu,cpu->BC.w,cpu->AF.b[HI]);
	    break;

	case 0x7a:	/* ADC HL,SP */
	    TSTATE(15);
	    ADC16(cpu->HL.w,cpu->SP);
	    break;

	case 0x7b:	/* LD SP,(nnnn) */
	    TSTATE(20);
	    cpu->SP=PEEKW(FETCH_WORD);
	    break;

	case 0x7c:	/* NEG */
	    {
	    Z80Byte b;

	    TSTATE(8);

	    b=cpu->AF.b[HI];
	    cpu->AF.b[HI]=0;
	    SUB8(b);
	    break;
	    }

	case 0x7d:	/* RETN */
	    TSTATE(14);
	    cpu->IFF1=cpu->IFF2;
	    POP(cpu->PC);
	    break;

	case 0x7e:	/* IM 2 */
	    TSTATE(8);
	    cpu->IM=2;
	    break;

	case 0x7f:	/* NOP */
	    TSTATE(8);
	    CALLBACK(eZ80_EDHook,opcode);
	    break;

	case 0xa0:	/* LDI */
	    TSTATE(16);
	    LDI;
	    break;

	case 0xa1:	/* CPI */
	    TSTATE(16);
	    CPI;
	    break;

	case 0xa2:	/* INI */
	    TSTATE(16);
	    INI;
	    break;

	case 0xa3:	/* OUTI */
	    TSTATE(16);
	    OUTI;
	    break;

	case 0xa8:	/* LDD */
	    TSTATE(16);
	    LDD;
	    break;

	case 0xa9:	/* CPD */
	    TSTATE(16);
	    CPD;
	    break;

	case 0xaa:	/* IND */
	    TSTATE(16);
	    IND;
	    break;

	case 0xab:	/* OUTD */
	    TSTATE(16);
	    OUTD;
	    break;

	case 0xb0:	/* LDIR */
	    TSTATE(16);
	    LDI;
	    if (cpu->BC.w)
	    {
	    	TSTATE(5);
		cpu->PC-=2;
	    }
	    break;

	case 0xb1:	/* CPIR */
	    TSTATE(16);
	    CPI;
	    if (cpu->BC.w && !IS_Z)
	    {
	    	TSTATE(5);
		cpu->PC-=2;
	    }
	    break;

	case 0xb2:	/* INIR */
	    TSTATE(16);
	    INI;
	    if (cpu->BC.w)
	    {
	    	TSTATE(5);
		cpu->PC-=2;
	    }
	    break;

	case 0xb3:	/* OTIR */
	    TSTATE(16);
	    OUTI;
	    if (cpu->BC.w)
	    {
	    	TSTATE(5);
		cpu->PC-=2;
	    }
	    break;

	case 0xb8:	/* LDDR */
	    TSTATE(16);
	    LDD;
	    if (cpu->BC.w)
	    {
	    	TSTATE(5);
		cpu->PC-=2;
	    }
	    break;

	case 0xb9:	/* CPDR */
	    TSTATE(16);
	    CPD;
	    if (cpu->BC.w && !IS_Z)
	    {
	    	TSTATE(5);
		cpu->PC-=2;
	    }
	    break;

	case 0xba:	/* INDR */
	    TSTATE(16);
	    IND;
	    if (cpu->BC.w)
	    {
	    	TSTATE(5);
		cpu->PC-=2;
	    }
	    break;

	case 0xbb:	/* OTDR */
	    TSTATE(16);
	    OUTD;
	    if (cpu->BC.w)
	    {
	    	TSTATE(5);
		cpu->PC-=2;
	    }
	    break;

	/* All the rest are NOP/invalid
	*/
    	default:
	    TSTATE(8);
	    CALLBACK(eZ80_EDHook,opcode);
	    break;
    }
}


/* ---------------------------------------- HANDLERS FOR CB OPCODES
*/
static void DecodeCB(Z80 *cpu, Z80Byte opcode)
{
    switch(opcode)
    {
    	CB_ALU_BLOCK(0x00,RLC)
    	CB_ALU_BLOCK(0x08,RRC)
    	CB_ALU_BLOCK(0x10,RL)
    	CB_ALU_BLOCK(0x18,RR)
    	CB_ALU_BLOCK(0x20,SLA)
    	CB_ALU_BLOCK(0x28,SRA)
    	CB_ALU_BLOCK(0x30,SLL)
    	CB_ALU_BLOCK(0x38,SRL)

    	CB_BITMANIP_BLOCK(0x40,BIT,0)
    	CB_BITMANIP_BLOCK(0x48,BIT,1)
    	CB_BITMANIP_BLOCK(0x50,BIT,2)
    	CB_BITMANIP_BLOCK(0x58,BIT,3)
    	CB_BITMANIP_BLOCK(0x60,BIT,4)
    	CB_BITMANIP_BLOCK(0x68,BIT,5)
    	CB_BITMANIP_BLOCK(0x70,BIT,6)
    	CB_BITMANIP_BLOCK(0x78,BIT,7)

    	CB_BITMANIP_BLOCK(0x80,BIT_RES,0)
    	CB_BITMANIP_BLOCK(0x88,BIT_RES,1)
    	CB_BITMANIP_BLOCK(0x90,BIT_RES,2)
    	CB_BITMANIP_BLOCK(0x98,BIT_RES,3)
    	CB_BITMANIP_BLOCK(0xa0,BIT_RES,4)
    	CB_BITMANIP_BLOCK(0xa8,BIT_RES,5)
    	CB_BITMANIP_BLOCK(0xb0,BIT_RES,6)
    	CB_BITMANIP_BLOCK(0xb8,BIT_RES,7)

    	CB_BITMANIP_BLOCK(0xc0,BIT_SET,0)
    	CB_BITMANIP_BLOCK(0xc8,BIT_SET,1)
    	CB_BITMANIP_BLOCK(0xd0,BIT_SET,2)
    	CB_BITMANIP_BLOCK(0xd8,BIT_SET,3)
    	CB_BITMANIP_BLOCK(0xe0,BIT_SET,4)
    	CB_BITMANIP_BLOCK(0xe8,BIT_SET,5)
    	CB_BITMANIP_BLOCK(0xf0,BIT_SET,6)
    	CB_BITMANIP_BLOCK(0xf8,BIT_SET,7)
    }
}


static void ShiftedDecodeCB(Z80 *cpu, Z80Byte opcode, Z80Relative offset)
{
    Z80Word addr;

    /* See if we've come here from a IX/IY shift.
    */
    switch (PRIV->shift)
    {
    	case 0xdd:
	    addr=cpu->IX.w+offset;
	    break;
    	case 0xfd:
	    addr=cpu->IY.w+offset;
	    break;
	default:
	    addr=cpu->HL.w;	/* Play safe... */
	    break;
    }

    switch(opcode)
    {
    	SHIFTED_CB_ALU_BLOCK(0x00,RLC)
    	SHIFTED_CB_ALU_BLOCK(0x08,RRC)
    	SHIFTED_CB_ALU_BLOCK(0x10,RL)
    	SHIFTED_CB_ALU_BLOCK(0x18,RR)
    	SHIFTED_CB_ALU_BLOCK(0x20,SLA)
    	SHIFTED_CB_ALU_BLOCK(0x28,SRA)
    	SHIFTED_CB_ALU_BLOCK(0x30,SLL)
    	SHIFTED_CB_ALU_BLOCK(0x38,SRL)

    	SHIFTED_CB_BITMANIP_BLOCK(0x40,BIT,0)
    	SHIFTED_CB_BITMANIP_BLOCK(0x48,BIT,1)
    	SHIFTED_CB_BITMANIP_BLOCK(0x50,BIT,2)
    	SHIFTED_CB_BITMANIP_BLOCK(0x58,BIT,3)
    	SHIFTED_CB_BITMANIP_BLOCK(0x60,BIT,4)
    	SHIFTED_CB_BITMANIP_BLOCK(0x68,BIT,5)
    	SHIFTED_CB_BITMANIP_BLOCK(0x70,BIT,6)
    	SHIFTED_CB_BITMANIP_BLOCK(0x78,BIT,7)

    	SHIFTED_CB_BITMANIP_BLOCK(0x80,BIT_RES,0)
    	SHIFTED_CB_BITMANIP_BLOCK(0x88,BIT_RES,1)
    	SHIFTED_CB_BITMANIP_BLOCK(0x90,BIT_RES,2)
    	SHIFTED_CB_BITMANIP_BLOCK(0x98,BIT_RES,3)
    	SHIFTED_CB_BITMANIP_BLOCK(0xa0,BIT_RES,4)
    	SHIFTED_CB_BITMANIP_BLOCK(0xa8,BIT_RES,5)
    	SHIFTED_CB_BITMANIP_BLOCK(0xb0,BIT_RES,6)
    	SHIFTED_CB_BITMANIP_BLOCK(0xb8,BIT_RES,7)

    	SHIFTED_CB_BITMANIP_BLOCK(0xc0,BIT_SET,0)
    	SHIFTED_CB_BITMANIP_BLOCK(0xc8,BIT_SET,1)
    	SHIFTED_CB_BITMANIP_BLOCK(0xd0,BIT_SET,2)
    	SHIFTED_CB_BITMANIP_BLOCK(0xd8,BIT_SET,3)
    	SHIFTED_CB_BITMANIP_BLOCK(0xe0,BIT_SET,4)
    	SHIFTED_CB_BITMANIP_BLOCK(0xe8,BIT_SET,5)
    	SHIFTED_CB_BITMANIP_BLOCK(0xf0,BIT_SET,6)
    	SHIFTED_CB_BITMANIP_BLOCK(0xf8,BIT_SET,7)
    }
}


/* ---------------------------------------- NORMAL OPCODE DECODER
*/
void Z80_Decode(Z80 *cpu, Z80Byte opcode)
{
    Z80Word *HL;
    Z80Byte *H;
    Z80Byte *L;
    Z80Relative off;

    /* See if we've come here from a IX/IY shift
    */
    switch (PRIV->shift)
    {
    	case 0xdd:
	    HL=&(cpu->IX.w);
	    L=cpu->IX.b+LO;
	    H=cpu->IX.b+HI;
	    break;
    	case 0xfd:
	    HL=&(cpu->IY.w);
	    L=cpu->IY.b+LO;
	    H=cpu->IY.b+HI;
	    break;
	default:
	    HL=&(cpu->HL.w);
	    L=cpu->HL.b+LO;
	    H=cpu->HL.b+HI;
	    break;
    }

    switch(opcode)
    {
	case 0x00:	/* NOP */
	    TSTATE(4);
	    break;

	case 0x01:	/* LD BC,nnnn */
	    TSTATE(10);
	    cpu->BC.w=FETCH_WORD;
	    break;

	case 0x02:	/* LD (BC),A */
	    TSTATE(7);
	    POKE(cpu->BC.w,cpu->AF.b[HI]);
	    break;

	case 0x03:	/* INC BC */
	    TSTATE(6);
	    cpu->BC.w++;
	    break;

	case 0x04:	/* INC B */
	    TSTATE(4);
	    INC8(cpu->BC.b[HI]);
	    break;

	case 0x05:	/* DEC B */
	    TSTATE(4);
	    DEC8(cpu->BC.b[HI]);
	    break;

	case 0x06:	/* LD B,n */
	    TSTATE(7);
	    cpu->BC.b[HI]=FETCH_BYTE;
	    break;

	case 0x07:	/* RLCA */
	    TSTATE(4);
	    RLCA;
	    break;

	case 0x08:	/* EX AF,AF' */
	    TSTATE(4);
	    SWAP(cpu->AF.w,cpu->AF_);
	    break;

	case 0x09:	/* ADD HL,BC */
	    TSTATE(11);
	    ADD16(*HL,cpu->BC.w);
	    break;

	case 0x0a:	/* LD A,(BC) */
	    TSTATE(7);
	    cpu->AF.b[HI]=PEEK(cpu->BC.w);
	    break;

	case 0x0b:	/* DEC BC */
	    TSTATE(6);
	    cpu->BC.w--;
	    break;

	case 0x0c:	/* INC C */
	    TSTATE(4);
	    INC8(cpu->BC.b[LO]);
	    break;

	case 0x0d:	/* DEC C */
	    TSTATE(4);
	    DEC8(cpu->BC.b[LO]);
	    break;

	case 0x0e:	/* LD C,n */
	    TSTATE(7);
	    cpu->BC.b[LO]=FETCH_BYTE;
	    break;

	case 0x0f:	/* RRCA */
	    TSTATE(4);
	    RRCA;
	    break;

	case 0x10:	/* DJNZ */
	    if (--(cpu->BC.b[HI]))
	    {
		TSTATE(13);
		JR;
	    }
	    else
	    {
		TSTATE(8);
		NOJR;
	    }
	    break;

	case 0x11:	/* LD DE,nnnn */
	    TSTATE(10);
	    cpu->DE.w=FETCH_WORD;
	    break;

	case 0x12:	/* LD (DE),A */
	    TSTATE(7);
	    POKE(cpu->DE.w,cpu->AF.b[HI]);
	    break;

	case 0x13:	/* INC DE */
	    TSTATE(6);
	    cpu->DE.w++;
	    break;

	case 0x14:	/* INC D */
	    TSTATE(4);
	    INC8(cpu->DE.b[HI]);
	    break;

	case 0x15:	/* DEC D */
	    TSTATE(4);
	    DEC8(cpu->DE.b[HI]);
	    break;

	case 0x16:	/* LD D,n */
	    TSTATE(7);
	    cpu->DE.b[HI]=FETCH_BYTE;
	    break;

	case 0x17:	/* RLA */
	    TSTATE(4);
	    RLA;
	    break;

	case 0x18:	/* JR d */
	    TSTATE(12);
	    JR;
	    break;

	case 0x19:	/* ADD HL,DE */
	    TSTATE(11);
	    ADD16(*HL,cpu->DE.w);
	    break;

	case 0x1a:	/* LD A,(DE) */
	    TSTATE(7);
	    cpu->AF.b[HI]=PEEK(cpu->DE.w);
	    break;

	case 0x1b:	/* DEC DE */
	    TSTATE(6);
	    cpu->DE.w--;
	    break;

	case 0x1c:	/* INC E */
	    TSTATE(4);
	    INC8(cpu->DE.b[LO]);
	    break;

	case 0x1d:	/* DEC E */
	    TSTATE(4);
	    DEC8(cpu->DE.b[LO]);
	    break;

	case 0x1e:	/* LD E,n */
	    TSTATE(7);
	    cpu->DE.b[LO]=FETCH_BYTE;
	    break;

	case 0x1f:	/* RRA */
	    TSTATE(4);
	    RRA;
	    break;

	case 0x20:	/* JR NZ,e */
	    JR_COND(!IS_Z);
	    break;

	case 0x21:	/* LD HL,nnnn */
	    TSTATE(10);
	    *HL=FETCH_WORD;
	    break;

	case 0x22:	/* LD (nnnn),HL */
	    TSTATE(16);
	    POKEW(FETCH_WORD,*HL);
	    break;

	case 0x23:	/* INC HL */
	    TSTATE(6);
	    (*HL)++;
	    break;

	case 0x24:	/* INC H */
	    TSTATE(4);
	    INC8(*H);
	    break;

	case 0x25:	/* DEC H */
	    TSTATE(4);
	    DEC8(*H);
	    break;

	case 0x26:	/* LD H,n */
	    TSTATE(7);
	    *H=FETCH_BYTE;
	    break;

	case 0x27:	/* DAA */
	    TSTATE(4);
	    DAA(cpu);
	    break;

	case 0x28:	/* JR Z,d */
	    JR_COND(IS_Z);
	    break;

	case 0x29:	/* ADD HL,HL */
	    TSTATE(11);
	    ADD16(*HL,*HL);
	    break;

	case 0x2a:	/* LD HL,(nnnn) */
	    TSTATE(7);
	    *HL=PEEKW(FETCH_WORD);
	    break;

	case 0x2b:	/* DEC HL */
	    TSTATE(6);
	    (*HL)--;
	    break;

	case 0x2c:	/* INC L */
	    TSTATE(4);
	    INC8(*L);
	    break;

	case 0x2d:	/* DEC L */
	    TSTATE(4);
	    DEC8(*L);
	    break;

	case 0x2e:	/* LD L,n */
	    TSTATE(7);
	    *L=FETCH_BYTE;
	    break;

	case 0x2f:	/* CPL */
	    TSTATE(4);
	    cpu->AF.b[HI]^=0xff;
	    SETFLAG(H_Z80);
	    SETFLAG(N_Z80);
	    SETHIDDEN(cpu->AF.b[HI]);
	    break;

	case 0x30:	/* JR NC,d */
	    JR_COND(!IS_C);
	    break;

	case 0x31:	/* LD SP,nnnn */
	    TSTATE(10);
	    cpu->SP=FETCH_WORD;
	    break;

	case 0x32:	/* LD (nnnn),A */
	    TSTATE(13);
	    POKE(FETCH_WORD,cpu->AF.b[HI]);
	    break;

	case 0x33:	/* INC SP */
	    TSTATE(6);
	    cpu->SP++;
	    break;

	case 0x34:	/* INC (HL) */
	    TSTATE(11);
	    OFFSET(off);
	    OP_ON_MEM(INC8,*HL+off);
	    break;

	case 0x35:	/* DEC (HL) */
	    TSTATE(11);
	    OFFSET(off);
	    OP_ON_MEM(DEC8,*HL+off);
	    break;

	case 0x36:	/* LD (HL),n */
	    TSTATE(10);
	    OFFSET(off);
	    POKE(*HL+off,FETCH_BYTE);
	    break;

	case 0x37:	/* SCF */
	    TSTATE(4);
	    cpu->AF.b[LO]=(cpu->AF.b[LO]&(S_Z80|Z_Z80|P_Z80))
			  | C_Z80
			  | (cpu->AF.b[HI]&(B3_Z80|B5_Z80));
	    break;

	case 0x38:	/* JR C,d */
	    JR_COND(IS_C);
	    break;

	case 0x39:	/* ADD HL,SP */
	    TSTATE(11);
	    ADD16(*HL,cpu->SP);
	    break;

	case 0x3a:	/* LD A,(nnnn) */
	    TSTATE(13);
	    cpu->AF.b[HI]=PEEK(FETCH_WORD);
	    break;

	case 0x3b:	/* DEC SP */
	    TSTATE(6);
	    cpu->SP--;
	    break;

	case 0x3c:	/* INC A */
	    TSTATE(4);
	    INC8(cpu->AF.b[HI]);
	    break;

	case 0x3d:	/* DEC A */
	    TSTATE(4);
	    DEC8(cpu->AF.b[HI]);
	    break;

	case 0x3e:	/* LD A,n */
	    TSTATE(7);
	    cpu->AF.b[HI]=FETCH_BYTE;
	    break;

	case 0x3f:	/* CCF */
	    TSTATE(4);

	    if (CARRY)
	    	SETFLAG(H_Z80);
	    else
	    	CLRFLAG(H_Z80);

	    cpu->AF.b[LO]^=C_Z80;
	    SETHIDDEN(cpu->AF.b[HI]);
	    break;

	LD_BLOCK(0x40,cpu->BC.b[HI],cpu->BC.b[HI])
	LD_BLOCK(0x48,cpu->BC.b[LO],cpu->BC.b[LO])
	LD_BLOCK(0x50,cpu->DE.b[HI],cpu->DE.b[HI])
	LD_BLOCK(0x58,cpu->DE.b[LO],cpu->DE.b[LO])
	LD_BLOCK(0x60,*H,cpu->HL.b[HI])
	LD_BLOCK(0x68,*L,cpu->HL.b[LO])

	case 0x70:	/* LD (HL),B */
	    TSTATE(7);
	    OFFSET(off);
	    POKE(*HL+off,cpu->BC.b[HI]);
	    break;

	case 0x71:	/* LD (HL),C */
	    TSTATE(7);
	    OFFSET(off);
	    POKE(*HL+off,cpu->BC.b[LO]);
	    break;

	case 0x72:	/* LD (HL),D */
	    TSTATE(7);
	    OFFSET(off);
	    POKE(*HL+off,cpu->DE.b[HI]);
	    break;

	case 0x73:	/* LD (HL),E */
	    TSTATE(7);
	    OFFSET(off);
	    POKE(*HL+off,cpu->DE.b[LO]);
	    break;

	case 0x74:	/* LD (HL),H */
	    TSTATE(7);
	    OFFSET(off);
	    POKE(*HL+off,cpu->HL.b[HI]);
	    break;

	case 0x75:	/* LD (HL),L */
	    TSTATE(7);
	    OFFSET(off);
	    POKE(*HL+off,cpu->HL.b[LO]);
	    break;

	case 0x76:	/* HALT */
	    TSTATE(4);
	    cpu->PC--;

	    if (!PRIV->halt)
		CALLBACK(eZ80_Halt,1);

	    PRIV->halt=TRUE;
	    break;

	case 0x77:	/* LD (HL),A */
	    TSTATE(7);
	    OFFSET(off);
	    POKE(*HL+off,cpu->AF.b[HI]);
	    break;

	LD_BLOCK(0x78,cpu->AF.b[HI],cpu->AF.b[HI])

	ALU_BLOCK(0x80,ADD8)
	ALU_BLOCK(0x88,ADC8)
	ALU_BLOCK(0x90,SUB8)
	ALU_BLOCK(0x98,SBC8)
	ALU_BLOCK(0xa0,AND)
	ALU_BLOCK(0xa8,XOR)
	ALU_BLOCK(0xb0,OR)
	ALU_BLOCK(0xb8,CMP8)

	case 0xc0:	/* RET NZ */
	    RET_COND(!IS_Z);
	    break;

	case 0xc1:	/* POP BC */
	    TSTATE(10);
	    POP(cpu->BC.w);
	    break;

	case 0xc2:	/* JP NZ,nnnn */
	    JP_COND(!IS_Z);
	    break;

	case 0xc3:	/* JP nnnn */
	    JP_COND(1);
	    break;

	case 0xc4:	/* CALL NZ,nnnn */
	    CALL_COND(!IS_Z);
	    break;

	case 0xc5:	/* PUSH BC */
	    TSTATE(10);
	    PUSH(cpu->BC.w);
	    break;

	case 0xc6:	/* ADD A,n */
	    TSTATE(7);
	    ADD8(FETCH_BYTE);
	    break;

	case 0xc7:	/* RST 0 */
	    RST(0);
	    break;

	case 0xc8:	/* RET Z */
	    RET_COND(IS_Z);
	    break;

	case 0xc9:	/* RET */
	    TSTATE(10);
	    POP(cpu->PC);
	    break;

	case 0xca:	/* JP Z,nnnn */
	    JP_COND(IS_Z);
	    break;

	case 0xcb:	/* CB PREFIX */
	    INC_R;

	    /* Check for previous IX/IY shift.
	    */
	    if (PRIV->shift!=0)
	    {
		Z80Relative cb_offset;

		TSTATE(4);	/* Wild stab in the dark! */
		cb_offset=FETCH_BYTE;
		ShiftedDecodeCB(cpu,FETCH_BYTE,cb_offset);
	    }
	    else
	    {
		DecodeCB(cpu,FETCH_BYTE);
	    }
	    break;

	case 0xcc:	/* CALL Z,nnnn */
	    CALL_COND(IS_Z);
	    break;

	case 0xcd:	/* CALL nnnn */
	    CALL_COND(1);
	    break;

	case 0xce:	/* ADC A,n */
	    ADC8(FETCH_BYTE);
	    break;

	case 0xcf:	/* RST 8 */
	    RST(8);
	    break;

	case 0xd0:	/* RET NC */
	    RET_COND(!IS_C);
	    break;

	case 0xd1:	/* POP DE */
	    TSTATE(10);
	    POP(cpu->DE.w);
	    break;

	case 0xd2:	/* JP NC,nnnn */
	    JP_COND(!IS_C);
	    break;

	case 0xd3:	/* OUT (n),A */
	    TSTATE(11);
	    if (PRIV->pwrite)
	    {
		Z80Word port;

		port=FETCH_BYTE;
		port|=(Z80Word)cpu->AF.b[HI]<<8;
	    	PRIV->pwrite(cpu,port,cpu->AF.b[HI]);
	    }
	    else
	    	cpu->PC++;
	    break;

	case 0xd4:	/* CALL NC,nnnn */
	    CALL_COND(!IS_C);
	    break;

	case 0xd5:	/* PUSH DE */
	    TSTATE(11);
	    PUSH(cpu->DE.w);
	    break;

	case 0xd6:	/* SUB A,n */
	    TSTATE(7);
	    SUB8(FETCH_BYTE);
	    break;

	case 0xd7:	/* RST 10 */
	    RST(0x10);
	    break;

	case 0xd8:	/* RET C */
	    RET_COND(IS_C);
	    break;

	case 0xd9:	/* EXX */
	    TSTATE(4);
	    SWAP(cpu->BC.w,cpu->BC_);
	    SWAP(cpu->DE.w,cpu->DE_);
	    SWAP(cpu->HL.w,cpu->HL_);
	    break;

	case 0xda:	/* JP C,nnnn */
	    JP_COND(IS_C);
	    break;

	case 0xdb:	/* IN A,(n) */
	    TSTATE(11);
	    if (PRIV->pread)
	    {
		Z80Word port;

		port=FETCH_BYTE;
		port|=(Z80Word)cpu->AF.b[HI]<<8;
		cpu->AF.b[HI]=PRIV->pread(cpu,port);
	    }
	    else
	    	cpu->PC++;
	    break;

	case 0xdc:	/* CALL C,nnnn */
	    CALL_COND(IS_C);
	    break;

	case 0xdd:	/* DD PREFIX */
	    TSTATE(4);
	    INC_R;

	    PRIV->shift=opcode;
	    Z80_Decode(cpu,FETCH_BYTE);
	    break;

	case 0xde:	/* SBC A,n */
	    TSTATE(7);
	    SBC8(FETCH_BYTE);
	    break;

	case 0xdf:	/* RST 18 */
	    RST(0x18);
	    break;

	case 0xe0:	/* RET PO */
	    RET_COND(!IS_P);
	    break;

	case 0xe1:	/* POP HL */
	    TSTATE(10);
	    POP(*HL);
	    break;

	case 0xe2:	/* JP PO,nnnn */
	    JP_COND(!IS_P);
	    break;

	case 0xe3:	/* EX (SP),HL */
	    {
	    Z80Word tmp;
	    TSTATE(19);
	    POP(tmp);
	    PUSH(*HL);
	    *HL=tmp;
	    }
	    break;

	case 0xe4:	/* CALL PO,nnnn */
	    CALL_COND(!IS_P);
	    break;

	case 0xe5:	/* PUSH HL */
	    TSTATE(10);
	    PUSH(*HL);
	    break;

	case 0xe6:	/* AND A,n */
	    TSTATE(7);
	    AND(FETCH_BYTE);
	    break;

	case 0xe7:	/* RST 20 */
	    RST(0x20);
	    break;

	case 0xe8:	/* RET PE */
	    RET_COND(IS_P);
	    break;

	case 0xe9:	/* JP (HL) */
	    TSTATE(4);
	    cpu->PC=*HL;
	    break;

	case 0xea:	/* JP PE,nnnn */
	    JP_COND(IS_P);
	    break;

	case 0xeb:	/* EX DE,HL */
	    TSTATE(4);
	    SWAP(cpu->DE.w,*HL);
	    break;

	case 0xec:	/* CALL PE,nnnn */
	    CALL_COND(IS_P);
	    break;

	case 0xed:	/* ED PREFIX */
	    INC_R;
	    DecodeED(cpu,FETCH_BYTE);
	    break;

	case 0xee:	/* XOR A,n */
	    TSTATE(7);
	    XOR(FETCH_BYTE);
	    break;

	case 0xef:	/* RST 28 */
	    RST(0x28);
	    break;

	case 0xf0:	/* RET P */
	    RET_COND(!IS_S);
	    break;

	case 0xf1:	/* POP AF */
	    TSTATE(10);
	    POP(cpu->AF.w);
	    break;

	case 0xf2:	/* JP P,nnnn */
	    JP_COND(!IS_S);
	    break;

	case 0xf3:	/* DI */
	    TSTATE(4);
	    cpu->IFF1=0;
	    cpu->IFF2=0;
	    break;

	case 0xf4:	/* CALL P,nnnn */
	    CALL_COND(!IS_S);
	    break;

	case 0xf5:	/* PUSH AF */
	    TSTATE(10);
	    PUSH(cpu->AF.w);
	    break;

	case 0xf6:	/* OR A,n */
	    TSTATE(7);
	    OR(FETCH_BYTE);
	    break;

	case 0xf7:	/* RST 30 */
	    RST(0x30);
	    break;

	case 0xf8:	/* RET M */
	    RET_COND(IS_S);
	    break;

	case 0xf9:	/* LD SP,HL */
	    TSTATE(6);
	    cpu->SP=*HL;
	    break;

	case 0xfa:	/* JP M,nnnn */
	    JP_COND(IS_S);
	    break;

	case 0xfb:	/* EI */
	    TSTATE(4);
	    cpu->IFF1=1;
	    cpu->IFF2=1;
	    break;

	case 0xfc:	/* CALL M,nnnn */
	    CALL_COND(IS_S);
	    break;

	case 0xfd:	/* FD PREFIX */
	    TSTATE(4);
	    INC_R;

	    PRIV->shift=opcode;
	    Z80_Decode(cpu,FETCH_BYTE);
	    break;

	case 0xfe:	/* CP A,n */
	    TSTATE(7);
	    CMP8(FETCH_BYTE);
	    break;

	case 0xff:	/* RST 38 */
	    RST(0x38);
	    break;

    }
}


/* END OF FILE */
