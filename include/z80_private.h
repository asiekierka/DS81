/*

    z80 - Z80 emulation

    Copyright (C) 2006  Ian Cowburn (ianc@noddybox.co.uk)

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

    $Id: z80_private.h,v 1.2 2006-10-12 16:38:57 ianc Exp $

    Private macros for Z80

*/

#ifndef Z80_PRIVATE_H
#define Z80_PRIVATE_H "$Id: z80_private.h,v 1.2 2006-10-12 16:38:57 ianc Exp $"

#include "z80_config.h"

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#define MAX_PER_CALLBACK	10


/* ---------------------------------------- TYPES
*/

struct Z80Private
{
    Z80Val		cycle;

    int			halt;

    Z80Byte		shift;

    int			raise;
    Z80Byte		devbyte;
    int			nmi;

#ifndef ENABLE_ARRAY_MEMORY
    Z80ReadMemory	disread;

    Z80ReadMemory	mread;
    Z80WriteMemory	mwrite;
#endif

    Z80ReadPort		pread;
    Z80WritePort	pwrite;

    Z80Callback		callback[eZ80_NO_CALLBACK][MAX_PER_CALLBACK];

    int			last_cb;
};

#define PRIV		cpu->priv


/* ---------------------------------------- ARRAY MEMORY
*/

#ifdef ENABLE_ARRAY_MEMORY
extern Z80Byte	Z80_MEMORY[];
#endif


/* ---------------------------------------- MACROS
*/

/* NOTE: A lot of these macros assume you have a variable called 'cpu'
         which is a pointer to Z80
*/


/* Invoke a callback class
*/
#define CALLBACK(r,d)	do					\
			{					\
			int f;					\
								\
			for(f=0;f<MAX_PER_CALLBACK;f++)		\
			    if (PRIV->callback[r][f])		\
				PRIV->last_cb &=		\
				    PRIV->callback[r][f](cpu,d);\
			} while(0)

/* Flag register
*/
#define C_Z80			0x01
#define N_Z80			0x02
#define P_Z80			0x04
#define V_Z80			P_Z80
#define H_Z80			0x10
#define Z_Z80			0x40
#define S_Z80			0x80

#define B3_Z80			0x08
#define B5_Z80			0x20


#define SET(v,b)		(v)|=b
#define CLR(v,b)		(v)&=~(b)

#define SETFLAG(f)		SET(cpu->AF.b[LO],f)
#define CLRFLAG(f)		CLR(cpu->AF.b[LO],f)

#ifdef ENABLE_ARRAY_MEMORY

#define PEEK(addr)		Z80_MEMORY[addr]

static inline Z80Word PEEKW(Z80Word addr)
{
    return (PEEK(addr) | (Z80Word)PEEK(addr+1)<<8);
}

#define POKE(addr,val)		do					\
				{					\
				    Z80Word ba=addr;			\
				    if (ba>=RAMBOT && ba<=RAMTOP)	\
				    	Z80_MEMORY[ba]=val;		\
				} while(0)

#define POKEW(addr,val)		do					\
				{					\
				    Z80Word wa=addr;			\
				    Z80Word wv=val;			\
				    POKE(wa,wv);			\
				    POKE(wa+1,wv>>8);			\
				} while(0)


#define FETCH_BYTE		(Z80_MEMORY[cpu->PC++])
#define FETCH_WORD              (cpu->PC+=2,				\
				    Z80_MEMORY[cpu->PC-2]|		\
					((Z80Word)Z80_MEMORY[cpu->PC-1]<<8))

#else

#define PEEK(addr)		(PRIV->mread(cpu,addr))
#define PEEKW(addr)		FPEEKW(cpu,addr)

#define POKE(addr,val)		PRIV->mwrite(cpu,addr,val)
#define POKEW(addr,val)		FPOKEW(cpu,addr,val)

#define FETCH_BYTE		(PRIV->mread(cpu,cpu->PC++))
#define FETCH_WORD		(cpu->PC+=2,FPEEKW(cpu,cpu->PC-2))

#endif


#define IS_C			(cpu->AF.b[LO]&C_Z80)
#define IS_N			(cpu->AF.b[LO]&N_Z80)
#define IS_P			(cpu->AF.b[LO]&P_Z80)
#define IS_H			(cpu->AF.b[LO]&H_Z80)
#define IS_Z			(cpu->AF.b[LO]&Z_Z80)
#define IS_S			(cpu->AF.b[LO]&S_Z80)

#define CARRY			IS_C

#define IS_IX_IY		(PRIV->shift==0xdd || PRIV->shift==0xfd)
#define OFFSET(off)		off=(IS_IX_IY ? (Z80Relative)FETCH_BYTE:0)

#define TSTATE(n)		PRIV->cycle+=n

#define ADD_R(v)		cpu->R=((cpu->R&0x80)|((cpu->R+(v))&0x7f))
#define INC_R			ADD_R(1)

#ifdef ENABLE_ARRAY_MEMORY

#define PUSH(REG)		do					\
				{					\
				    Z80Word pv=REG;			\
				    cpu->SP-=2;				\
				    POKE(cpu->SP,pv);			\
				    POKE(cpu->SP+1,pv>>8);		\
				} while(0)

#else

#define PUSH(REG)		do					\
				{					\
				    Z80Word pushv=REG;			\
				    cpu->SP-=2;				\
				    PRIV->mwrite(cpu,cpu->SP,pushv);	\
				    PRIV->mwrite(cpu,cpu->SP+1,pushv>>8);\
				} while(0)
#endif

#define POP(REG)		do					\
				{					\
				    REG=PEEK(cpu->SP) |			\
				    	(Z80Word)PEEK(cpu->SP+1)<<8;	\
				    cpu->SP+=2;				\
				} while(0)

#define SETHIDDEN(res)		cpu->AF.b[LO]=(cpu->AF.b[LO]&~(B3_Z80|B5_Z80))|\
					((res)&(B3_Z80|B5_Z80))

#define CALL			do				\
				{				\
				    PUSH(cpu->PC+2);		\
				    cpu->PC=PEEKW(cpu->PC);	\
				} while(0)

#define NOCALL			cpu->PC+=2
#define JP			cpu->PC=PEEKW(cpu->PC)
#define NOJP			cpu->PC+=2
#define JR			cpu->PC+=(Z80Relative)PEEK(cpu->PC)+1
#define NOJR			cpu->PC++

#define OUT(P,V)		do				\
				{				\
				    if (PRIV->pwrite)		\
				    	PRIV->pwrite(cpu,P,V);	\
				} while(0)

#define IN(P)			(PRIV->pread?PRIV->pread(cpu,P):0)



/* ---------------------------------------- LABELS
*/
extern Z80Label			*z80_labels;


/* ---------------------------------------- GLOBAL GENERAL OPCODES/ROUTINES
*/
void Z80_Decode(Z80 *cpu, Z80Byte opcode);
void Z80_InitialiseInternals(void);


/* ---------------------------------------- DISASSEMBLY
*/
#ifdef ENABLE_DISASSEM
typedef void		(*DIS_OP_CALLBACK)(Z80 *z80, Z80Byte op, Z80Word *pc);

extern DIS_OP_CALLBACK	dis_CB_opcode[];
extern DIS_OP_CALLBACK	dis_DD_opcode[];
extern DIS_OP_CALLBACK	dis_DD_CB_opcode[];
extern DIS_OP_CALLBACK	dis_ED_opcode[];
extern DIS_OP_CALLBACK	dis_FD_opcode[];
extern DIS_OP_CALLBACK	dis_FD_CB_opcode[];
extern DIS_OP_CALLBACK	dis_opcode_z80[];

const char	*Z80_Dis_Printf(const char *format, ...);

Z80Byte		Z80_Dis_FetchByte(Z80 *cpu, Z80Word *pc);
Z80Word		Z80_Dis_FetchWord(Z80 *cpu, Z80Word *pc);

void		Z80_Dis_Set(const char *op, const char *arg);
const char	*Z80_Dis_GetOp(void);
const char	*Z80_Dis_GetArg(void);
#endif	/* ENABLE_DISASSEM */

#endif	/* Z80_PRIVATE_H */

/* END OF FILE */
