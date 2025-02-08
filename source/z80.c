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

    $Id: z80.c,v 1.3 2008-12-05 00:37:25 ianc Exp $

    Z80

*/
#include <stdlib.h>
#include <string.h>

#include "z80.h"
#include "z80_private.h"

#include "stream.h"

static const char ident[]="$Id: z80.c,v 1.3 2008-12-05 00:37:25 ianc Exp $";
static const char ident_z80_header[]=Z80_H;
static const char ident_z80_private_header[]=Z80_PRIVATE_H;

Z80Label        *z80_labels=NULL;

/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static void InitTables()
{
    static int init=FALSE;

    if (init)
    	return;

    init=TRUE;

    Z80_InitialiseInternals();
}

static void Z80_CheckInterrupt(Z80 *cpu)
{
    /* Check interrupts
    */
    if (PRIV->raise)
    {
    	if (PRIV->nmi)
	{
	    if (PRIV->halt)
	    {
		PRIV->halt=FALSE;
		CALLBACK(eZ80_Halt,0);
		cpu->PC++;
	    }

	    TSTATE(2);
	    cpu->IFF1=0;
	    PRIV->nmi=FALSE;
	    PUSH(cpu->PC);
	    cpu->PC=0x66;
	}
    	else if (cpu->IFF1)
	{
	    if (PRIV->halt)
	    {
		PRIV->halt=FALSE;
		CALLBACK(eZ80_Halt,0);
		cpu->PC++;
	    }

	    TSTATE(2);

	    cpu->IFF1=0;
	    cpu->IFF2=0;

	    switch(cpu->IM)
	    {
		default:
		case 0:
		    INC_R;
		    Z80_Decode(cpu,PRIV->devbyte);
		    return;
		    break;

		case 1:
		    PUSH(cpu->PC);
		    cpu->PC=0x38;
		    break;

		case 2:
		    PUSH(cpu->PC);
		    cpu->PC=(Z80Word)cpu->I*256+PRIV->devbyte;
		    break;
	    }
	}

	PRIV->raise=FALSE;
    }
}


/* ---------------------------------------- INTERFACES
*/

#ifdef ENABLE_ARRAY_MEMORY
Z80     *Z80Init(Z80ReadPort read_port,
                 Z80WritePort write_port)
#else
Z80     *Z80Init(Z80ReadMemory read_memory,
                 Z80WriteMemory write_memory,
                 Z80ReadPort read_port,
                 Z80WritePort write_port,
                 Z80ReadMemory read_for_disassem)
#endif
{
    Z80 *cpu;
    int f;
    int r;

    InitTables();

#ifndef ENABLE_ARRAY_MEMORY
    if (!read_memory || !write_memory)
    	return NULL;
#endif

    cpu=malloc(sizeof *cpu);

    if (cpu)
    {
    	cpu->priv=malloc(sizeof *cpu->priv);

	if (cpu->priv)
	{
#ifndef ENABLE_ARRAY_MEMORY
	    PRIV->mread=read_memory;
	    PRIV->mwrite=write_memory;
	    PRIV->disread=read_for_disassem;
#endif
	    PRIV->pread=read_port;
	    PRIV->pwrite=write_port;

	    for(f=0;f<eZ80_NO_CALLBACK;f++)
		for(r=0;r<MAX_PER_CALLBACK;r++)
		    PRIV->callback[f][r]=NULL;

	    Z80Reset(cpu);
	}
	else
	{
	    free(cpu);
	    cpu=NULL;
	}
    }

    return cpu;
}


void Z80Reset(Z80 *cpu)
{
    PRIV->cycle=0;
    cpu->PC=0;

    cpu->AF.w=0xffff;
    cpu->BC.w=0xffff;
    cpu->DE.w=0xffff;
    cpu->HL.w=0xffff;
    cpu->AF_=0xffff;
    cpu->BC_=0xffff;
    cpu->DE_=0xffff;
    cpu->HL_=0xffff;

    cpu->IX.w=0xffff;
    cpu->IY.w=0xffff;

    cpu->SP=0xffff;
    cpu->IFF1=0;
    cpu->IFF2=0;
    cpu->IM=0;
    cpu->I=0;
    cpu->R=0;
    PRIV->halt=0;

    PRIV->raise=FALSE;
    PRIV->nmi=FALSE;
}


Z80Val Z80Cycles(Z80 *cpu)
{
    return PRIV->cycle;
}


void Z80ResetCycles(Z80 *cpu, Z80Val cycles)
{
    PRIV->cycle=cycles;
}


int Z80LodgeCallback(Z80 *cpu, Z80CallbackReason reason, Z80Callback callback)
{
    int f;

    for(f=0;f<MAX_PER_CALLBACK;f++)
    {
    	if (!PRIV->callback[reason][f])
	{
	    PRIV->callback[reason][f]=callback;
	    return TRUE;
	}
    }

    return FALSE;
}


void Z80RemoveCallback(Z80 *cpu, Z80CallbackReason reason, Z80Callback callback)
{
    int f;

    for(f=0;f<MAX_PER_CALLBACK;f++)
    {
    	if (PRIV->callback[reason][f]==callback)
	{
	    PRIV->callback[reason][f]=NULL;
	}
    }
}


void Z80Interrupt(Z80 *cpu, Z80Byte devbyte)
{
    PRIV->raise=TRUE;
    PRIV->devbyte=devbyte;
    PRIV->nmi=FALSE;
}


void Z80NMI(Z80 *cpu)
{
    PRIV->raise=TRUE;
    PRIV->nmi=TRUE;
}


int Z80SingleStep(Z80 *cpu)
{
    Z80Byte opcode;

    PRIV->last_cb=TRUE;
    PRIV->shift=0;

    Z80_CheckInterrupt(cpu);

    CALLBACK(eZ80_Instruction,PRIV->cycle);

    INC_R;

    opcode=FETCH_BYTE;

    Z80_Decode(cpu,opcode);

    return PRIV->last_cb;
}


void Z80Exec(Z80 *cpu)
{
    while (Z80SingleStep(cpu));
}


void Z80SetLabels(Z80Label labels[])
{
    z80_labels=labels;
}


const char *Z80Disassemble(Z80 *cpu, Z80Word *pc)
{
#ifdef ENABLE_DISASSEM
    Z80Byte Z80_Dis_FetchByte(Z80 *cpu, Z80Word *pc);
    static char s[80];
    Z80Word opc,npc;
    Z80Byte op;
    int f;

    opc=*pc;
    op=Z80_Dis_FetchByte(cpu,pc);
    dis_opcode_z80[op](cpu,op,pc);
    npc=*pc;

    strcpy(s,Z80_Dis_Printf("%-5s",Z80_Dis_GetOp()));
    strcat(s,Z80_Dis_Printf("%-40s ;",Z80_Dis_GetArg()));

    for(f=0;f<5 && opc!=npc;f++)
    {
#ifdef ENABLE_ARRAY_MEMORY
	strcat(s,Z80_Dis_Printf(" %.2x",(int)Z80_MEMORY[opc++]));
#else
	strcat(s,Z80_Dis_Printf(" %.2x",(int)PRIV->disread(cpu,opc++)));
#endif
    }

    if (opc!=npc)
	for(f=1;f<3;f++)
	    s[strlen(s)-f]='.';

    return s;
#else
    (*pc)+=4;
    return "NO DISASSEMBLER";
#endif
}


void Z80SaveSnapshot(Z80 *cpu, FILE *fp)
{
    PUT_ULong(fp, cpu->PC);
    PUT_ULong(fp, cpu->AF.w);
    PUT_ULong(fp, cpu->BC.w);
    PUT_ULong(fp, cpu->DE.w);
    PUT_ULong(fp, cpu->HL.w);
    PUT_ULong(fp, cpu->AF_);
    PUT_ULong(fp, cpu->BC_);
    PUT_ULong(fp, cpu->DE_);
    PUT_ULong(fp, cpu->HL_);
    PUT_ULong(fp, cpu->IX.w);
    PUT_ULong(fp, cpu->IY.w);
    PUT_ULong(fp, cpu->SP);
    PUT_Byte(fp, cpu->IFF1);
    PUT_Byte(fp, cpu->IFF2);
    PUT_Byte(fp, cpu->IM);
    PUT_Byte(fp, cpu->I);
    PUT_Byte(fp, cpu->R);
    PUT_Byte(fp, cpu->R);

    PUT_ULong(fp, cpu->priv->cycle);
    PUT_Long(fp, cpu->priv->halt);
    PUT_Byte(fp, cpu->priv->shift);
    PUT_Long(fp, cpu->priv->raise);
    PUT_Byte(fp, cpu->priv->devbyte);
    PUT_Long(fp, cpu->priv->nmi);
    PUT_Long(fp, cpu->priv->last_cb);
}

void Z80LoadSnapshot(Z80 *cpu, FILE *fp)
{
    cpu->PC = GET_ULong(fp);
    cpu->AF.w = GET_ULong(fp);
    cpu->BC.w = GET_ULong(fp);
    cpu->DE.w = GET_ULong(fp);
    cpu->HL.w = GET_ULong(fp);
    cpu->AF_ = GET_ULong(fp);
    cpu->BC_ = GET_ULong(fp);
    cpu->DE_ = GET_ULong(fp);
    cpu->HL_ = GET_ULong(fp);
    cpu->IX.w = GET_ULong(fp);
    cpu->IY.w = GET_ULong(fp);
    cpu->SP = GET_ULong(fp);
    cpu->IFF1 = GET_Byte(fp);
    cpu->IFF2 = GET_Byte(fp);
    cpu->IM = GET_Byte(fp);
    cpu->I = GET_Byte(fp);
    cpu->R = GET_Byte(fp);
    cpu->R = GET_Byte(fp);

    cpu->priv->cycle = GET_ULong(fp);
    cpu->priv->halt = GET_Long(fp);
    cpu->priv->shift = GET_Byte(fp);
    cpu->priv->raise = GET_Long(fp);
    cpu->priv->devbyte = GET_Byte(fp);
    cpu->priv->nmi = GET_Long(fp);
    cpu->priv->last_cb = GET_Long(fp);
}

/* END OF FILE */
