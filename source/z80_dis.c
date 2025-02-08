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

    $Id: z80_dis.c,v 1.1 2006-10-04 23:05:43 ianc Exp $

*/
static const char ident[]="$Id: z80_dis.c,v 1.1 2006-10-04 23:05:43 ianc Exp $";

#include "z80_config.h"

#ifdef ENABLE_DISASSEM

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "z80.h"
#include "z80_private.h"

static Z80Relative	cb_off;

/* ---------------------------------------- SHARED ROUTINES
*/
static const char *z80_dis_reg8[]={"b","c","d","e","h","l","(hl)","a"};
static const char *z80_dis_reg16[]={"bc","de","hl","sp"};
static const char *z80_dis_condition[]={"nz","z","nc","c","po","pe","p","m"};

static const char *dis_op;
static const char *dis_arg;

const char *Z80_Dis_Printf(const char *format, ...)
{
    static int p=0;
    static char s[16][80];
    va_list arg;

    va_start(arg,format);
    p=(p+1)%16;
    vsprintf(s[p],format,arg);
    va_end(arg);

    return s[p];
}


Z80Byte Z80_Dis_FetchByte(Z80 *cpu, Z80Word *pc)
{
#ifdef ENABLE_ARRAY_MEMORY
    return Z80_MEMORY[(*pc)++];
#else
    return cpu->priv->disread(cpu,(*pc)++);
#endif
}


Z80Word Z80_Dis_FetchWord(Z80 *cpu, Z80Word *pc)
{
    Z80Byte l,h;

    l=Z80_Dis_FetchByte(cpu,pc);
    h=Z80_Dis_FetchByte(cpu,pc);

    return ((Z80Word)h<<8)|l;
}


void Z80_Dis_Set(const char *op, const char *arg)
{
    dis_op=op;
    dis_arg=arg;
}


const char *Z80_Dis_GetOp(void)
{
    return dis_op ? dis_op : "";
}


const char *Z80_Dis_GetArg(void)
{
    return dis_arg ? dis_arg : "";
}


static const char *GetLabel(Z80Word w)
{
    if (z80_labels)
    {
    	int f;

	for(f=0;z80_labels[f].label;f++)
	{
	    if (z80_labels[f].address==w)
	    {
	    	return z80_labels[f].label;
	    }
	}
    }

    return NULL;
}



/* ---------------------------------------- CB xx BYTE OPCODES
*/
static void DIS_RLC_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("rlc",reg);
}

static void DIS_RRC_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("rrc",reg);
}

static void DIS_RL_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("rl",reg);
}

static void DIS_RR_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("rr",reg);
}

static void DIS_SLA_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("sla",reg);
}

static void DIS_SRA_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("sra",reg);
}

static void DIS_SLL_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("sll",reg);
}

static void DIS_SRL_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("srl",reg);
}

static void DIS_BIT_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;
    int bit;

    reg=z80_dis_reg8[op%8];
    bit=(op-0x40)/8;
    Z80_Dis_Set("bit",Z80_Dis_Printf("%d,%s",bit,reg));
}

static void DIS_RES_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;
    int bit;

    reg=z80_dis_reg8[op%8];
    bit=(op-0x80)/8;
    Z80_Dis_Set("res",Z80_Dis_Printf("%d,%s",bit,reg));
}

static void DIS_SET_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;
    int bit;

    reg=z80_dis_reg8[op%8];
    bit=(op-0xc0)/8;
    Z80_Dis_Set("set",Z80_Dis_Printf("%d,%s",bit,reg));
}

/* ---------------------------------------- DD OPCODES
*/

static const char *IX_RelStr(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    static char s[80];
    Z80Relative r;

    r=(Z80Relative)Z80_Dis_FetchByte(z80,pc);

    if (r<0)
	sprintf(s,"(ix-$%.2x)",-r);
    else
	sprintf(s,"(ix+$%.2x)",r);

    return(s);
}


static const char *IX_RelStrCB(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    static char s[80];
    Z80Relative r;

    r=(Z80Relative)cb_off;

    if (r<0)
	sprintf(s,"(ix-$%.2x)",-r);
    else
	sprintf(s,"(ix+$%.2x)",r);

    return(s);
}


static const char *XR8(Z80 *z80, int reg, Z80Word *pc)
{
    switch(reg)
    	{
	case 0:
	    return("b");
	    break;
	case 1:
	    return("c");
	    break;
	case 2:
	    return("d");
	    break;
	case 3:
	    return("e");
	    break;
	case 4:
	    return("ixh");
	    break;
	case 5:
	    return("ixl");
	    break;
	case 6:
	    return(IX_RelStr(z80,0,pc));
	    break;
	case 7:
	    return("a");
	    break;
	default:
	    return(Z80_Dis_Printf("BUG %d",reg));
	    break;
	}
}

static void DIS_DD_NOP(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    dis_opcode_z80[op](z80,op,pc);
}

static void DIS_ADD_IX_BC(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("add","ix,bc");
}
 
static void DIS_ADD_IX_DE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("add","ix,de");
}

static void DIS_LD_IX_WORD(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld",Z80_Dis_Printf("ix,$%.4x",Z80_Dis_FetchWord(z80,pc)));
}

static void DIS_LD_ADDR_IX(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);

    if ((p=GetLabel(w)))
	Z80_Dis_Set("ld",Z80_Dis_Printf("(%s),ix",p));
    else
	Z80_Dis_Set("ld",Z80_Dis_Printf("($%.4x),ix",w));
}

static void DIS_INC_IX(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("inc","ix");
}

static void DIS_INC_IXH(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("inc","ixh");
}

static void DIS_DEC_IXH(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("dec","ixh");
}

static void DIS_LD_IXH_BYTE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld",Z80_Dis_Printf("ixh,$%.2x",Z80_Dis_FetchByte(z80,pc)));
}

static void DIS_ADD_IX_IX(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("add","ix,ix");
}

static void DIS_LD_IX_ADDR(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);

    if ((p=GetLabel(w)))
	Z80_Dis_Set("ld",Z80_Dis_Printf("ix,(%s)",p));
    else
	Z80_Dis_Set("ld",Z80_Dis_Printf("ix,($%.4x)",w));
}

static void DIS_DEC_IX(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("dec","ix");
}

static void DIS_INC_IXL(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("inc","ixl");
}

static void DIS_DEC_IXL(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("dec","ixl");
}

static void DIS_LD_IXL_BYTE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld",Z80_Dis_Printf("ixl,$%.2x",Z80_Dis_FetchByte(z80,pc)));
}
 
static void DIS_INC_IIX(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("inc",Z80_Dis_Printf("%s",IX_RelStr(z80,op,pc)));
}

static void DIS_DEC_IIX(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("dec",Z80_Dis_Printf("%s",IX_RelStr(z80,op,pc)));
}

static void DIS_LD_IIX_BYTE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *rel;
    int b;

    rel=IX_RelStr(z80,op,pc);
    b=Z80_Dis_FetchByte(z80,pc);
    Z80_Dis_Set("ld",Z80_Dis_Printf("%s,$%.2x",rel,b));
}


static void DIS_ADD_IX_SP(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("add","ix,sp");
}
 
static void DIS_XLD_R8_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int src_r,dest_r;
    const char *src,*dest;

    dest_r=(op-0x40)/8;
    src_r=op%8;

    /* IX can't be used as source and destination when reading z80ory
    */
    if (dest_r==6)
    	{
	dest=XR8(z80,dest_r,pc);
	src=z80_dis_reg8[src_r];
	}
    else if (((dest_r==4)||(dest_r==5))&&(src_r==6))
    	{
	dest=z80_dis_reg8[dest_r];
	src=XR8(z80,src_r,pc);
	}
    else
	{
	dest=XR8(z80,dest_r,pc);
	src=XR8(z80,src_r,pc);
	}

    Z80_Dis_Set("ld",Z80_Dis_Printf("%s,%s",dest,src));
}

static void DIS_XADD_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("add",Z80_Dis_Printf("a,%s",XR8(z80,(op%8),pc)));
}

static void DIS_XADC_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("adc",Z80_Dis_Printf("a,%s",XR8(z80,(op%8),pc)));
}

static void DIS_XSUB_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("sub",Z80_Dis_Printf("a,%s",XR8(z80,(op%8),pc)));
}

static void DIS_XSBC_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("sbc",Z80_Dis_Printf("a,%s",XR8(z80,(op%8),pc)));
}

static void DIS_XAND_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("and",Z80_Dis_Printf("%s",XR8(z80,(op%8),pc)));
}

static void DIS_XXOR_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("xor",Z80_Dis_Printf("%s",XR8(z80,(op%8),pc)));
}

static void DIS_X_OR_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("or",Z80_Dis_Printf("%s",XR8(z80,(op%8),pc)));
}

static void DIS_XCP_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("cp",Z80_Dis_Printf("%s",XR8(z80,(op%8),pc)));
}

static void DIS_POP_IX(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("pop","ix");
}

static void DIS_EX_ISP_IX(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ex","(sp),ix");
}

static void DIS_PUSH_IX(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("push","ix");
}

static void DIS_JP_IX(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("jp","(ix)");
}
 
static void DIS_LD_SP_IX(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld","sp,ix");
}

static void DIS_DD_CB_DECODE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int nop;

    cb_off=(Z80Relative)Z80_Dis_FetchByte(z80,pc);
    nop=Z80_Dis_FetchByte(z80,pc);
    dis_DD_CB_opcode[nop](z80,nop,pc);
}

static void DIS_DD_DD_DECODE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int nop;

    nop=Z80_Dis_FetchByte(z80,pc);
    dis_DD_opcode[nop](z80,nop,pc);
}

static void DIS_DD_ED_DECODE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int nop;

    nop=Z80_Dis_FetchByte(z80,pc);
    dis_ED_opcode[nop](z80,nop,pc);
}

static void DIS_DD_FD_DECODE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int nop;

    nop=Z80_Dis_FetchByte(z80,pc);
    dis_FD_opcode[nop](z80,nop,pc);
}


/* ---------------------------------------- DD CB OPCODES
*/

static void DIS_RLC_IX (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("rlc",Z80_Dis_Printf("%s",IX_RelStrCB(z80,op,pc)));
    else
	{
        Z80_Dis_Set("rlc",Z80_Dis_Printf("%s[%s]",IX_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
	}
}

static void DIS_RRC_IX (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("rrc",Z80_Dis_Printf("%s",IX_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("rrc",Z80_Dis_Printf("%s[%s]",IX_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_RL_IX (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("rl",Z80_Dis_Printf("%s",IX_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("rl",Z80_Dis_Printf("%s[%s]",IX_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_RR_IX (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("rr",Z80_Dis_Printf("%s",IX_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("rr",Z80_Dis_Printf("%s[%s]",IX_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_SLA_IX (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("sla",Z80_Dis_Printf("%s",IX_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("sla",Z80_Dis_Printf("%s[%s]",IX_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_SRA_IX (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("sra",Z80_Dis_Printf("%s",IX_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("sra",Z80_Dis_Printf("%s[%s]",IX_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_SRL_IX (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("srl",Z80_Dis_Printf("%s",IX_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("srl",Z80_Dis_Printf("%s[%s]",IX_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_SLL_IX (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("sll",Z80_Dis_Printf("%s",IX_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("sll",Z80_Dis_Printf("%s[%s]",IX_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_BIT_IX (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;
    int bit;

    reg=(op%8);
    bit=(op-0x40)/8;

    if (reg==6)
        Z80_Dis_Set("bit",Z80_Dis_Printf("%d,%s",bit,IX_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("bit",Z80_Dis_Printf("%d,%s[%s]",bit,IX_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_RES_IX (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;
    int bit;

    reg=(op%8);
    bit=(op-0x80)/8;

    if (reg==6)
        Z80_Dis_Set("res",Z80_Dis_Printf("%d,%s",bit,IX_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("res",Z80_Dis_Printf("%d,%s[%s]",bit,IX_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_SET_IX (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;
    int bit;

    reg=(op%8);
    bit=(op-0xc0)/8;

    if (reg==6)
        Z80_Dis_Set("set",Z80_Dis_Printf("%d,%s",bit,IX_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("set",Z80_Dis_Printf("%d,%s[%s]",bit,IX_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}


/* ---------------------------------------- ED OPCODES
*/

static const char *ER8(int reg)
{
    switch(reg)
    	{
	case 0:
	    return("b");
	    break;
	case 1:
	    return("c");
	    break;
	case 2:
	    return("d");
	    break;
	case 3:
	    return("e");
	    break;
	case 4:
	    return("h");
	    break;
	case 5:
	    return("l");
	    break;
	case 6:
	    return("0");
	    break;
	case 7:
	    return("a");
	    break;
	}

    return "?";
}

/* Assumes illegal ED ops are being used for break points
*/
static void DIS_ED_NOP(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("brk",Z80_Dis_Printf("$%.2x",op));
}

static void DIS_IN_R8_C(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("in",Z80_Dis_Printf("%s,(c)",ER8((op-0x40)/8)));
}

static void DIS_OUT_C_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("out",Z80_Dis_Printf("(c),%s",ER8((op-0x40)/8)));
}

static void DIS_SBC_HL_R16(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("sbc",Z80_Dis_Printf("hl,%s",z80_dis_reg16[(op-0x40)/16]));
}

static void DIS_ED_LD_ADDR_R16(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);

    if ((p=GetLabel(w)))
	Z80_Dis_Set("ld",Z80_Dis_Printf("(%s),%s",p,z80_dis_reg16[(op-0x40)/16]));
    else
	Z80_Dis_Set("ld",Z80_Dis_Printf("($%.4x),%s",w,z80_dis_reg16[(op-0x40)/16]));
}

static void DIS_NEG(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("neg",NULL);
}

static void DIS_RETN(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("retn",NULL);
}

static void DIS_IM_0(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("im","0");
}

static void DIS_LD_I_A(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld","i,a");
}

static void DIS_ADC_HL_R16(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("adc",Z80_Dis_Printf("hl,%s",z80_dis_reg16[(op-0x40)/16]));
}

static void DIS_ED_LD_R16_ADDR(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);

    if ((p=GetLabel(w)))
	Z80_Dis_Set("ld",Z80_Dis_Printf("%s,(%s)",z80_dis_reg16[(op-0x40)/16],p));
    else
	Z80_Dis_Set("ld",Z80_Dis_Printf("%s,($%.4x)",z80_dis_reg16[(op-0x40)/16],w));
}

static void DIS_RETI(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("reti",NULL);
}

static void DIS_LD_R_A(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld","r,a");
}

static void DIS_IM_1(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("im","1");
}

static void DIS_LD_A_I(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld","a,i");
}

static void DIS_IM_2(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("im","2");
}

static void DIS_LD_A_R(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld","a,r");
}

static void DIS_RRD(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("rrd",NULL);
}

static void DIS_RLD(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("rld",NULL);
}

static void DIS_LDI(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ldi",NULL);
}

static void DIS_CPI(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("cpi",NULL);
}

static void DIS_INI(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ini",NULL);
}

static void DIS_OUTI(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("outi",NULL);
}

static void DIS_LDD(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ldd",NULL);
}

static void DIS_CPD(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("cpd",NULL);
}

static void DIS_IND(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ind",NULL);
}

static void DIS_OUTD(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("outd",NULL);
}

static void DIS_LDIR(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ldir",NULL);
}

static void DIS_CPIR(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("cpir",NULL);
}

static void DIS_INIR(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("inir",NULL);
}

static void DIS_OTIR(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("otir",NULL);
}

static void DIS_LDDR(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("lddr",NULL);
}

static void DIS_CPDR(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("cpdr",NULL);
}

static void DIS_INDR(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("indr",NULL);
}

static void DIS_OTDR(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("otdr",NULL);
}


/* ---------------------------------------- FD OPCODES
*/

static const char *IY_RelStr(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    static char s[80];
    Z80Relative r;

    r=(Z80Relative)Z80_Dis_FetchByte(z80,pc);

    if (r<0)
	sprintf(s,"(iy-$%.2x)",-r);
    else
	sprintf(s,"(iy+$%.2x)",r);

    return(s);
}


static const char *IY_RelStrCB(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    static char s[80];
    Z80Relative r;

    r=(Z80Relative)cb_off;

    if (r<0)
	sprintf(s,"(iy-$%.2x)",-r);
    else
	sprintf(s,"(iy+$%.2x)",r);

    return(s);
}


static const char *YR8(Z80 *z80, int reg, Z80Word *pc)
{
    switch(reg)
    	{
	case 0:
	    return("b");
	    break;
	case 1:
	    return("c");
	    break;
	case 2:
	    return("d");
	    break;
	case 3:
	    return("e");
	    break;
	case 4:
	    return("iyh");
	    break;
	case 5:
	    return("iyl");
	    break;
	case 6:
	    return(IY_RelStr(z80,0,pc));
	    break;
	case 7:
	    return("a");
	    break;
	default:
	    return(Z80_Dis_Printf("BUG %d",reg));
	    break;
	}
}

static void DIS_FD_NOP(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    dis_opcode_z80[op](z80,op,pc);
}

static void DIS_ADD_IY_BC(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("add","iy,bc");
}
 
static void DIS_ADD_IY_DE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("add","iy,de");
}

static void DIS_LD_IY_WORD(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld",Z80_Dis_Printf("iy,$%.4x",Z80_Dis_FetchWord(z80,pc)));
}

static void DIS_LD_ADDR_IY(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);

    if ((p=GetLabel(w)))
	Z80_Dis_Set("ld",Z80_Dis_Printf("(%s),iy",p));
    else
	Z80_Dis_Set("ld",Z80_Dis_Printf("($%.4x),iy",w));
}

static void DIS_INC_IY(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("inc","iy");
}

static void DIS_INC_IYH(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("inc","iyh");
}

static void DIS_DEC_IYH(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("dec","iyh");
}

static void DIS_LD_IYH_BYTE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld",Z80_Dis_Printf("iyh,$%.2x",Z80_Dis_FetchByte(z80,pc)));
}

static void DIS_ADD_IY_IY(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("add","iy,iy");
}

static void DIS_LD_IY_ADDR(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);

    if ((p=GetLabel(w)))
	Z80_Dis_Set("ld",Z80_Dis_Printf("iy,(%s)",p));
    else
	Z80_Dis_Set("ld",Z80_Dis_Printf("iy,($%.4x)",w));
}

static void DIS_DEC_IY(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("dec","iy");
}

static void DIS_INC_IYL(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("inc","iyl");
}

static void DIS_DEC_IYL(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("dec","iyl");
}

static void DIS_LD_IYL_BYTE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld",Z80_Dis_Printf("iyl,$%.2x",Z80_Dis_FetchByte(z80,pc)));
}
 
static void DIS_INC_IIY(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("inc",Z80_Dis_Printf("%s",IY_RelStr(z80,op,pc)));
}

static void DIS_DEC_IIY(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("dec",Z80_Dis_Printf("%s",IY_RelStr(z80,op,pc)));
}

static void DIS_LD_IIY_BYTE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *rel;
    int b;

    rel=IY_RelStr(z80,op,pc);
    b=Z80_Dis_FetchByte(z80,pc);
    Z80_Dis_Set("ld",Z80_Dis_Printf("%s,$%.2x",rel,b));
}


static void DIS_ADD_IY_SP(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("add","iy,sp");
}
 
static void DIS_YLD_R8_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int src_r,dest_r;
    const char *src,*dest;

    dest_r=(op-0x40)/8;
    src_r=op%8;

    /* IY can't be used as source and destination when reading z80ory
    */
    if (dest_r==6)
	{
	dest=YR8(z80,dest_r,pc);
	src=z80_dis_reg8[src_r];
	}
    else if (((dest_r==4)||(dest_r==5))&&(src_r==6))
	{
	dest=z80_dis_reg8[dest_r];
	src=YR8(z80,src_r,pc);
	}
    else
	{
	dest=YR8(z80,dest_r,pc);
	src=YR8(z80,src_r,pc);
	}

    Z80_Dis_Set("ld",Z80_Dis_Printf("%s,%s",dest,src));
}

static void DIS_YADD_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("add",Z80_Dis_Printf("a,%s",YR8(z80,(op%8),pc)));
}

static void DIS_YADC_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("adc",Z80_Dis_Printf("a,%s",YR8(z80,(op%8),pc)));
}

static void DIS_YSUB_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("sub",Z80_Dis_Printf("a,%s",YR8(z80,(op%8),pc)));
}

static void DIS_YSBC_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("sbc",Z80_Dis_Printf("a,%s",YR8(z80,(op%8),pc)));
}

static void DIS_YAND_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("and",Z80_Dis_Printf("%s",YR8(z80,(op%8),pc)));
}

static void DIS_YYOR_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("xor",Z80_Dis_Printf("%s",YR8(z80,(op%8),pc)));
}

static void DIS_Y_OR_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("or",Z80_Dis_Printf("%s",YR8(z80,(op%8),pc)));
}

static void DIS_YCP_R8(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("cp",Z80_Dis_Printf("%s",YR8(z80,(op%8),pc)));
}

static void DIS_POP_IY(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("pop","iy");
}

static void DIS_EY_ISP_IY(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ex","(sp),iy");
}

static void DIS_PUSH_IY(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("push","iy");
}

static void DIS_JP_IY(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("jp","(iy)");
}
 
static void DIS_LD_SP_IY(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld","sp,iy");
}

static void DIS_FD_CB_DECODE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int nop;

    cb_off=(Z80Relative)Z80_Dis_FetchByte(z80,pc);
    nop=Z80_Dis_FetchByte(z80,pc);
    dis_FD_CB_opcode[nop](z80,nop,pc);
}

static void DIS_FD_DD_DECODE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int nop;

    nop=Z80_Dis_FetchByte(z80,pc);
    dis_DD_opcode[nop](z80,nop,pc);
}

static void DIS_FD_ED_DECODE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int nop;

    nop=Z80_Dis_FetchByte(z80,pc);
    dis_ED_opcode[nop](z80,nop,pc);
}

static void DIS_FD_FD_DECODE(Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int nop;

    nop=Z80_Dis_FetchByte(z80,pc);
    dis_FD_opcode[nop](z80,nop,pc);
}


/* ---------------------------------------- FD CB OPCODES
*/

static void DIS_RLC_IY (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("rlc",Z80_Dis_Printf("%s",IY_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("rlc",Z80_Dis_Printf("%s[%s]",IY_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_RRC_IY (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("rrc",Z80_Dis_Printf("%s",IY_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("rrc",Z80_Dis_Printf("%s[%s]",IY_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_RL_IY (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("rl",Z80_Dis_Printf("%s",IY_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("rl",Z80_Dis_Printf("%s[%s]",IY_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_RR_IY (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("rr",Z80_Dis_Printf("%s",IY_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("rr",Z80_Dis_Printf("%s[%s]",IY_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_SLA_IY (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("sla",Z80_Dis_Printf("%s",IY_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("sla",Z80_Dis_Printf("%s[%s]",IY_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_SRA_IY (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("sra",Z80_Dis_Printf("%s",IY_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("sra",Z80_Dis_Printf("%s[%s]",IY_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_SRL_IY (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("srl",Z80_Dis_Printf("%s",IY_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("srl",Z80_Dis_Printf("%s[%s]",IY_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_SLL_IY (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;

    reg=(op%8);

    if (reg==6)
        Z80_Dis_Set("sll",Z80_Dis_Printf("%s",IY_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("sll",Z80_Dis_Printf("%s[%s]",IY_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_BIT_IY (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;
    int bit;

    reg=(op%8);
    bit=(op-0x40)/8;

    if (reg==6)
        Z80_Dis_Set("bit",Z80_Dis_Printf("%d,%s",bit,IY_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("bit",Z80_Dis_Printf("%d,%s[%s]",bit,IY_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_RES_IY (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;
    int bit;

    reg=(op%8);
    bit=(op-0x80)/8;

    if (reg==6)
        Z80_Dis_Set("res",Z80_Dis_Printf("%d,%s",bit,IY_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("res",Z80_Dis_Printf("%d,%s[%s]",bit,IY_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}

static void DIS_SET_IY (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int reg;
    int bit;

    reg=(op%8);
    bit=(op-0xc0)/8;

    if (reg==6)
        Z80_Dis_Set("set",Z80_Dis_Printf("%d,%s",bit,IY_RelStrCB(z80,op,pc)));
    else
        Z80_Dis_Set("set",Z80_Dis_Printf("%d,%s[%s]",bit,IY_RelStrCB(z80,op,pc),z80_dis_reg8[reg]));
}


/* ---------------------------------------- SINGLE BYTE OPCODES
*/
static void DIS_NOP (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("nop",NULL);
}

static void DIS_LD_R16_WORD (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg16[(op&0x30)/0x10];
    Z80_Dis_Set("ld",Z80_Dis_Printf("%s,$%.4x",reg,Z80_Dis_FetchWord(z80,pc)));
}

static void DIS_LD_R16_A (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg16[(op&0x30)/0x10];
    Z80_Dis_Set("ld",Z80_Dis_Printf("(%s),a",reg));
}

static void DIS_INC_R16 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg16[(op&0x30)/0x10];
    Z80_Dis_Set("inc",reg);
}

static void DIS_INC_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[(op&0x38)/0x8];
    Z80_Dis_Set("inc",reg);
}

static void DIS_DEC_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[(op&0x38)/0x8];
    Z80_Dis_Set("dec",reg);
}

static void DIS_LD_R8_BYTE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[(op&0x38)/0x8];
    Z80_Dis_Set("ld",Z80_Dis_Printf("%s,$%.2x",reg,Z80_Dis_FetchByte(z80,pc)));
}

static void DIS_RLCA (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("rlca",NULL);
}

static void DIS_EX_AF_AF (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ex","af,af'");
}

static void DIS_ADD_HL_R16 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg16[(op&0x30)/0x10];
    Z80_Dis_Set("add",Z80_Dis_Printf("hl,%s",reg));
}

static void DIS_LD_A_R16 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg16[(op&0x30)/0x10];
    Z80_Dis_Set("ld",Z80_Dis_Printf("a,(%s)",reg));
}

static void DIS_DEC_R16 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg16[(op&0x30)/0x10];
    Z80_Dis_Set("dec",reg);
}

static void DIS_RRCA (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("rrca",NULL);
}

static void DIS_DJNZ (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word new;

#ifdef ENABLE_ARRAY_MEMORY
    new=*pc+(Z80Relative)Z80_MEMORY[*pc]+1;
#else
    new=*pc+(Z80Relative)z80->priv->disread(z80,*pc)+1;
#endif
    (*pc)++;
    Z80_Dis_Set("djnz",Z80_Dis_Printf("$%.4x",new));
}

static void DIS_RLA (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("rla",NULL);
}

static void DIS_JR (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word new;
    const char *p;

#ifdef ENABLE_ARRAY_MEMORY
    new=*pc+(Z80Relative)Z80_MEMORY[*pc]+1;
#else
    new=*pc+(Z80Relative)z80->priv->disread(z80,*pc)+1;
#endif
    (*pc)++;

    if ((p=GetLabel(new)))
	Z80_Dis_Set("jr",Z80_Dis_Printf("%s",p));
    else
	Z80_Dis_Set("jr",Z80_Dis_Printf("$%.4x",new));
}

static void DIS_RRA (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("rra",NULL);
}

static void DIS_JR_CO (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *con;
    Z80Word new;
    const char *p;

    con=z80_dis_condition[(op-0x20)/8];
#ifdef ENABLE_ARRAY_MEMORY
    new=*pc+(Z80Relative)Z80_MEMORY[*pc]+1;
#else
    new=*pc+(Z80Relative)z80->priv->disread(z80,*pc)+1;
#endif
    (*pc)++;

    if ((p=GetLabel(new)))
	Z80_Dis_Set("jr",Z80_Dis_Printf("%s,%s",con,p));
    else
	Z80_Dis_Set("jr",Z80_Dis_Printf("%s,$%.4x",con,new));
}

static void DIS_LD_ADDR_HL (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);

    if ((p=GetLabel(w)))
	Z80_Dis_Set("ld",Z80_Dis_Printf("(%s),hl",p));
    else
	Z80_Dis_Set("ld",Z80_Dis_Printf("($%.4x),hl",w));
}

static void DIS_DAA (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("daa",NULL);
}

static void DIS_LD_HL_ADDR (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);

    if ((p=GetLabel(w)))
	Z80_Dis_Set("ld",Z80_Dis_Printf("hl,(%s)",p));
    else
	Z80_Dis_Set("ld",Z80_Dis_Printf("hl,($%.4x)",w));
}

static void DIS_CPL (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("cpl",NULL);
}

static void DIS_LD_ADDR_A (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);

    if ((p=GetLabel(w)))
	Z80_Dis_Set("ld",Z80_Dis_Printf("(%s),a",p));
    else
	Z80_Dis_Set("ld",Z80_Dis_Printf("($%.4x),a",w));
}

static void DIS_SCF (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("scf",NULL);
}

static void DIS_LD_A_ADDR (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);

    if ((p=GetLabel(w)))
	Z80_Dis_Set("ld",Z80_Dis_Printf("a,(%s)",p));
    else
	Z80_Dis_Set("ld",Z80_Dis_Printf("a,($%.4x)",w));
}

static void DIS_CCF (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ccf",NULL);
}

static void DIS_LD_R8_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *src,*dest;

    dest=z80_dis_reg8[(op-0x40)/8];
    src=z80_dis_reg8[op%8];
    Z80_Dis_Set("ld",Z80_Dis_Printf("%s,%s",dest,src));
}

static void DIS_HALT (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("halt",NULL);
}

static void DIS_ADD_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("add",Z80_Dis_Printf("a,%s",reg));
}

static void DIS_ADC_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("adc",Z80_Dis_Printf("a,%s",reg));
}

static void DIS_SUB_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("sub",Z80_Dis_Printf("a,%s",reg));
}

static void DIS_SBC_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("sbc",Z80_Dis_Printf("a,%s",reg));
}

static void DIS_AND_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("and",Z80_Dis_Printf("%s",reg));
}

static void DIS_XOR_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("xor",Z80_Dis_Printf("%s",reg));
}

static void DIS_OR_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("or",Z80_Dis_Printf("%s",reg));
}

static void DIS_CP_R8 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg8[op%8];
    Z80_Dis_Set("cp",Z80_Dis_Printf("%s",reg));
}


static void DIS_RET_CO (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *con;

    con=z80_dis_condition[(op-0xc0)/8];
    Z80_Dis_Set("ret",con);
}

static void DIS_POP_R16 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg16[(op-0xc0)/16];

    if (!strcmp(reg,"sp"))
    	reg="af";

    Z80_Dis_Set("pop",reg);
}

static void DIS_JP (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);

    if ((p=GetLabel(w)))
	Z80_Dis_Set("jp",Z80_Dis_Printf("%s",p));
    else
	Z80_Dis_Set("jp",Z80_Dis_Printf("$%.4x",w));
}

static void DIS_PUSH_R16 (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *reg;

    reg=z80_dis_reg16[(op-0xc0)/16];

    if (!strcmp(reg,"sp"))
    	reg="af";

    Z80_Dis_Set("push",reg);
}

static void DIS_ADD_A_BYTE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("add",Z80_Dis_Printf("a,$%.2x",Z80_Dis_FetchByte(z80,pc)));
}

static void DIS_RST (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int add;

    add=(op&0x3f)-7;
    Z80_Dis_Set("rst",Z80_Dis_Printf("%.2xh",add));
}

static void DIS_RET (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ret",NULL);
}

static void DIS_JP_CO (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *con;
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);
    con=z80_dis_condition[(op-0xc0)/8];

    if ((p=GetLabel(w)))
	Z80_Dis_Set("jp",Z80_Dis_Printf("%s,%s",con,p));
    else
	Z80_Dis_Set("jp",Z80_Dis_Printf("%s,$%.4x",con,w));
}

static void DIS_CB_DECODE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int nop;

    nop=Z80_Dis_FetchByte(z80,pc);
    dis_CB_opcode[nop](z80,nop,pc);
}

static void DIS_CALL_CO (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    const char *con;
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);
    con=z80_dis_condition[(op-0xc0)/8];

    if ((p=GetLabel(w)))
	Z80_Dis_Set("call",Z80_Dis_Printf("%s,%s",con,p));
    else
	Z80_Dis_Set("call",Z80_Dis_Printf("%s,$%.4x",con,w));
}

static void DIS_CALL (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80Word w;
    const char *p;

    w=Z80_Dis_FetchWord(z80,pc);

    if ((p=GetLabel(w)))
	Z80_Dis_Set("call",Z80_Dis_Printf("%s",p));
    else
	Z80_Dis_Set("call",Z80_Dis_Printf("$%.4x",w));
}

static void DIS_ADC_A_BYTE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("adc",Z80_Dis_Printf("a,$%.2x",Z80_Dis_FetchByte(z80,pc)));
}

static void DIS_OUT_BYTE_A (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("out",Z80_Dis_Printf("($%.2x),a",Z80_Dis_FetchByte(z80,pc)));
}

static void DIS_SUB_A_BYTE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("sub",Z80_Dis_Printf("a,$%.2x",Z80_Dis_FetchByte(z80,pc)));
}

static void DIS_EXX (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("exx",NULL);
}

static void DIS_IN_A_BYTE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("in",Z80_Dis_Printf("a,($%.2x)",Z80_Dis_FetchByte(z80,pc)));
}

static void DIS_DD_DECODE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int nop;

    nop=Z80_Dis_FetchByte(z80,pc);
    dis_DD_opcode[nop](z80,nop,pc);
}

static void DIS_SBC_A_BYTE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("sbc",Z80_Dis_Printf("a,$%.2x",Z80_Dis_FetchByte(z80,pc)));
}


static void DIS_EX_ISP_HL (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ex","(sp),hl");
}

static void DIS_AND_A_BYTE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("and",Z80_Dis_Printf("$%.2x",Z80_Dis_FetchByte(z80,pc)));
}

static void DIS_JP_HL (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("jp","(hl)");
}

static void DIS_EX_DE_HL (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ex","de,hl");
}

static void DIS_ED_DECODE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int nop;

    nop=Z80_Dis_FetchByte(z80,pc);
    dis_ED_opcode[nop](z80,nop,pc);
}

static void DIS_XOR_A_BYTE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("xor",Z80_Dis_Printf("$%.2x",Z80_Dis_FetchByte(z80,pc)));
}

static void DIS_DI (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("di",NULL);
}

static void DIS_OR_A_BYTE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("or",Z80_Dis_Printf("$%.2x",Z80_Dis_FetchByte(z80,pc)));
}

static void DIS_LD_SP_HL (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ld","sp,hl");
}

static void DIS_EI (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("ei",NULL);
}

static void DIS_FD_DECODE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    int nop;

    nop=Z80_Dis_FetchByte(z80,pc);
    dis_FD_opcode[nop](z80,nop,pc);
}

static void DIS_CP_A_BYTE (Z80 *z80, Z80Byte op, Z80Word *pc)
{
    Z80_Dis_Set("cp",Z80_Dis_Printf("$%.2x",Z80_Dis_FetchByte(z80,pc)));
}


/* ---------------------------------------- TABLES
*/

/* CB opcodes
*/
DIS_OP_CALLBACK	dis_CB_opcode[0x100]=
		    {
/* 0x00 - 0x03 */    DIS_RLC_R8,  DIS_RLC_R8,    DIS_RLC_R8,    DIS_RLC_R8,
/* 0x04 - 0x07 */    DIS_RLC_R8,  DIS_RLC_R8,    DIS_RLC_R8,    DIS_RLC_R8,
/* 0x08 - 0x0b */    DIS_RRC_R8,  DIS_RRC_R8,    DIS_RRC_R8,    DIS_RRC_R8,
/* 0x0c - 0x0f */    DIS_RRC_R8,  DIS_RRC_R8,    DIS_RRC_R8,    DIS_RRC_R8,

/* 0x10 - 0x13 */    DIS_RL_R8,  DIS_RL_R8,    DIS_RL_R8,    DIS_RL_R8,  
/* 0x14 - 0x17 */    DIS_RL_R8,  DIS_RL_R8,    DIS_RL_R8,    DIS_RL_R8,
/* 0x18 - 0x1b */    DIS_RR_R8,  DIS_RR_R8,    DIS_RR_R8,    DIS_RR_R8,  
/* 0x1c - 0x1f */    DIS_RR_R8,  DIS_RR_R8,    DIS_RR_R8,    DIS_RR_R8,

/* 0x20 - 0x23 */    DIS_SLA_R8,  DIS_SLA_R8,    DIS_SLA_R8,    DIS_SLA_R8,
/* 0x24 - 0x27 */    DIS_SLA_R8,  DIS_SLA_R8,    DIS_SLA_R8,    DIS_SLA_R8,
/* 0x28 - 0x2b */    DIS_SRA_R8,  DIS_SRA_R8,    DIS_SRA_R8,    DIS_SRA_R8,
/* 0x2c - 0x2f */    DIS_SRA_R8,  DIS_SRA_R8,    DIS_SRA_R8,    DIS_SRA_R8,

/* 0x30 - 0x33 */    DIS_SLL_R8,  DIS_SLL_R8,    DIS_SLL_R8,    DIS_SLL_R8,
/* 0x34 - 0x37 */    DIS_SLL_R8,  DIS_SLL_R8,    DIS_SLL_R8,    DIS_SLL_R8,
/* 0x38 - 0x3b */    DIS_SRL_R8,  DIS_SRL_R8,    DIS_SRL_R8,    DIS_SRL_R8,
/* 0x3c - 0x3f */    DIS_SRL_R8,  DIS_SRL_R8,    DIS_SRL_R8,    DIS_SRL_R8,

/* 0x40 - 0x43 */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,
/* 0x44 - 0x47 */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,
/* 0x48 - 0x4b */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,
/* 0x4c - 0x4f */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,

/* 0x50 - 0x53 */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,
/* 0x54 - 0x57 */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,
/* 0x58 - 0x5b */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,
/* 0x5c - 0x5f */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,

/* 0x60 - 0x63 */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,
/* 0x64 - 0x67 */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,
/* 0x68 - 0x6b */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,
/* 0x6c - 0x6f */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,

/* 0x70 - 0x73 */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,
/* 0x74 - 0x77 */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,
/* 0x78 - 0x7b */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,
/* 0x7c - 0x7f */    DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,  DIS_BIT_R8,

/* 0x80 - 0x83 */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,
/* 0x84 - 0x87 */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,
/* 0x88 - 0x8b */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,
/* 0x8c - 0x8f */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,

/* 0x90 - 0x93 */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,
/* 0x94 - 0x97 */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,
/* 0x98 - 0x9b */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,
/* 0x9c - 0x9f */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,

/* 0xa0 - 0xa3 */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,
/* 0xa4 - 0xa7 */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,
/* 0xa8 - 0xab */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,
/* 0xac - 0xaf */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,

/* 0xb0 - 0xb3 */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,
/* 0xb4 - 0xb7 */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,
/* 0xb8 - 0xbb */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,
/* 0xbc - 0xbf */    DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,  DIS_RES_R8,

/* 0xc0 - 0xc3 */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
/* 0xc4 - 0xc7 */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
/* 0xc8 - 0xcb */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
/* 0xcc - 0xcf */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,

/* 0xd0 - 0xd3 */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
/* 0xd4 - 0xd7 */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
/* 0xd8 - 0xdb */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
/* 0xdc - 0xdf */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,

/* 0xe0 - 0xe3 */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
/* 0xe4 - 0xe7 */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
/* 0xe8 - 0xeb */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
/* 0xec - 0xef */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,

/* 0xf0 - 0xf3 */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
/* 0xf4 - 0xf7 */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
/* 0xf8 - 0xfb */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
/* 0xfc - 0xff */    DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,  DIS_SET_R8,
        };

/* DIS_DD opcodes
*/
DIS_OP_CALLBACK	dis_DD_opcode[0x100]=
		    {
/* 0x00 - 0x03 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x04 - 0x07 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x08 - 0x0b */    DIS_DD_NOP,	DIS_ADD_IX_BC,	DIS_DD_NOP,		DIS_DD_NOP,	
/* 0x0c - 0x0f */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,		

/* 0x10 - 0x13 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x14 - 0x17 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x18 - 0x1b */    DIS_DD_NOP,	DIS_ADD_IX_DE,	DIS_DD_NOP,		DIS_DD_NOP,	
/* 0x1c - 0x1f */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,		

/* 0x20 - 0x23 */    DIS_DD_NOP,	DIS_LD_IX_WORD,	DIS_LD_ADDR_IX,	DIS_INC_IX,
/* 0x24 - 0x27 */    DIS_INC_IXH,	DIS_DEC_IXH,	DIS_LD_IXH_BYTE,	DIS_DD_NOP,
/* 0x28 - 0x2b */    DIS_DD_NOP,	DIS_ADD_IX_IX,	DIS_LD_IX_ADDR,	DIS_DEC_IX,
/* 0x2c - 0x2f */    DIS_INC_IXL,	DIS_DEC_IXL,	DIS_LD_IXL_BYTE,	DIS_DD_NOP,

/* 0x30 - 0x33 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x34 - 0x37 */    DIS_INC_IIX,	DIS_DEC_IIX,	DIS_LD_IIX_BYTE,	DIS_DD_NOP,
/* 0x38 - 0x3b */    DIS_DD_NOP,	DIS_ADD_IX_SP,	DIS_DD_NOP,		DIS_DD_NOP,
/* 0x3c - 0x3f */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,

/* 0x40 - 0x43 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x44 - 0x47 */    DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_DD_NOP,
/* 0x48 - 0x4b */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x4c - 0x4f */    DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_DD_NOP,

/* 0x50 - 0x53 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x54 - 0x57 */    DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_DD_NOP,
/* 0x58 - 0x5b */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x5c - 0x5f */    DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_DD_NOP,

/* 0x60 - 0x63 */    DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,
/* 0x64 - 0x67 */    DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,
/* 0x68 - 0x6b */    DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,
/* 0x6c - 0x6f */    DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,

/* 0x70 - 0x73 */    DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,
/* 0x74 - 0x77 */    DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_DD_NOP,		DIS_XLD_R8_R8,
/* 0x78 - 0x7b */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x7c - 0x7f */    DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_XLD_R8_R8,	DIS_DD_NOP,

/* 0x80 - 0x83 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x84 - 0x87 */    DIS_XADD_R8,	DIS_XADD_R8,	DIS_XADD_R8,	DIS_DD_NOP,
/* 0x88 - 0x8b */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x8c - 0x8f */    DIS_XADC_R8,	DIS_XADC_R8,	DIS_XADC_R8,	DIS_DD_NOP,

/* 0x90 - 0x93 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x94 - 0x97 */    DIS_XSUB_R8,	DIS_XSUB_R8,	DIS_XSUB_R8,	DIS_DD_NOP,
/* 0x98 - 0x9b */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0x9c - 0x9f */    DIS_XSBC_R8,	DIS_XSBC_R8,	DIS_XSBC_R8,	DIS_DD_NOP,

/* 0xa0 - 0xa3 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0xa4 - 0xa7 */    DIS_XAND_R8,	DIS_XAND_R8,	DIS_XAND_R8,	DIS_DD_NOP,
/* 0xa8 - 0xab */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0xac - 0xaf */    DIS_XXOR_R8,	DIS_XXOR_R8,	DIS_XXOR_R8,	DIS_DD_NOP,

/* 0xb0 - 0xb3 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0xb4 - 0xb7 */    DIS_X_OR_R8,	DIS_X_OR_R8,	DIS_X_OR_R8,	DIS_DD_NOP,
/* 0xb8 - 0xbb */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0xbc - 0xbf */    DIS_XCP_R8,	DIS_XCP_R8,		DIS_XCP_R8,		DIS_DD_NOP,

/* 0xc0 - 0xc3 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0xc4 - 0xc7 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0xc8 - 0xcb */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_CB_DECODE,
/* 0xcc - 0xcf */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,

/* 0xd0 - 0xd3 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0xd4 - 0xd7 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0xd8 - 0xdb */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0xdc - 0xdf */    DIS_DD_NOP,	DIS_DD_DD_DECODE,	DIS_DD_NOP,		DIS_DD_NOP,

/* 0xe0 - 0xe3 */    DIS_DD_NOP,	DIS_POP_IX,		DIS_DD_NOP,		DIS_EX_ISP_IX,
/* 0xe4 - 0xe7 */    DIS_DD_NOP,	DIS_PUSH_IX,	DIS_DD_NOP,		DIS_DD_NOP,
/* 0xe8 - 0xeb */    DIS_DD_NOP,	DIS_JP_IX,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0xec - 0xef */    DIS_DD_NOP,	DIS_DD_ED_DECODE,	DIS_DD_NOP,		DIS_DD_NOP,

/* 0xf0 - 0xf3 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0xf4 - 0xf7 */    DIS_DD_NOP,	DIS_DD_NOP,		DIS_DD_NOP,		DIS_DD_NOP,
/* 0xf8 - 0xfb */    DIS_DD_NOP,	DIS_LD_SP_IX,	DIS_DD_NOP,		DIS_DD_NOP,
/* 0xfc - 0xff */    DIS_DD_NOP,	DIS_DD_FD_DECODE,	DIS_DD_NOP,		DIS_DD_NOP,
		    };


/* DIS_DD DIS_CB opcodes
*/
DIS_OP_CALLBACK	dis_DD_CB_opcode[0x100]=
		    {
/* 0x00 - 0x03 */    DIS_RLC_IX,	DIS_RLC_IX,		DIS_RLC_IX,		DIS_RLC_IX,
/* 0x04 - 0x07 */    DIS_RLC_IX,	DIS_RLC_IX,		DIS_RLC_IX,		DIS_RLC_IX,
/* 0x08 - 0x0b */    DIS_RRC_IX,	DIS_RRC_IX,		DIS_RRC_IX,		DIS_RRC_IX,
/* 0x0c - 0x0f */    DIS_RRC_IX,	DIS_RRC_IX,		DIS_RRC_IX,		DIS_RRC_IX,

/* 0x10 - 0x13 */    DIS_RL_IX,	DIS_RL_IX,		DIS_RL_IX,		DIS_RL_IX,	
/* 0x14 - 0x17 */    DIS_RL_IX,	DIS_RL_IX,		DIS_RL_IX,		DIS_RL_IX,
/* 0x18 - 0x1b */    DIS_RR_IX,	DIS_RR_IX,		DIS_RR_IX,		DIS_RR_IX,	
/* 0x1c - 0x1f */    DIS_RR_IX,	DIS_RR_IX,		DIS_RR_IX,		DIS_RR_IX,

/* 0x20 - 0x23 */    DIS_SLA_IX,	DIS_SLA_IX,		DIS_SLA_IX,		DIS_SLA_IX,
/* 0x24 - 0x27 */    DIS_SLA_IX,	DIS_SLA_IX,		DIS_SLA_IX,		DIS_SLA_IX,
/* 0x28 - 0x2b */    DIS_SRA_IX,	DIS_SRA_IX,		DIS_SRA_IX,		DIS_SRA_IX,
/* 0x2c - 0x2f */    DIS_SRA_IX,	DIS_SRA_IX,		DIS_SRA_IX,		DIS_SRA_IX,

/* 0x30 - 0x33 */    DIS_SLL_IX,	DIS_SLL_IX,		DIS_SLL_IX,		DIS_SLL_IX,
/* 0x34 - 0x37 */    DIS_SLL_IX,	DIS_SLL_IX,		DIS_SLL_IX,		DIS_SLL_IX,
/* 0x38 - 0x3b */    DIS_SRL_IX,	DIS_SRL_IX,		DIS_SRL_IX,		DIS_SRL_IX,
/* 0x3c - 0x3f */    DIS_SRL_IX,	DIS_SRL_IX,		DIS_SRL_IX,		DIS_SRL_IX,

/* 0x40 - 0x43 */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,
/* 0x44 - 0x47 */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,
/* 0x48 - 0x4b */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,
/* 0x4c - 0x4f */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,

/* 0x50 - 0x53 */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,
/* 0x54 - 0x57 */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,
/* 0x58 - 0x5b */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,
/* 0x5c - 0x5f */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,

/* 0x60 - 0x63 */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,
/* 0x64 - 0x67 */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,
/* 0x68 - 0x6b */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,
/* 0x6c - 0x6f */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,

/* 0x70 - 0x73 */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,
/* 0x74 - 0x77 */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,
/* 0x78 - 0x7b */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,
/* 0x7c - 0x7f */    DIS_BIT_IX,DIS_BIT_IX,	DIS_BIT_IX,	DIS_BIT_IX,

/* 0x80 - 0x83 */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,
/* 0x84 - 0x87 */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,
/* 0x88 - 0x8b */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,
/* 0x8c - 0x8f */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,

/* 0x90 - 0x93 */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,
/* 0x94 - 0x97 */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,
/* 0x98 - 0x9b */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,
/* 0x9c - 0x9f */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,

/* 0xa0 - 0xa3 */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,
/* 0xa4 - 0xa7 */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,
/* 0xa8 - 0xab */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,
/* 0xac - 0xaf */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,

/* 0xb0 - 0xb3 */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,
/* 0xb4 - 0xb7 */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,
/* 0xb8 - 0xbb */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,
/* 0xbc - 0xbf */    DIS_RES_IX,DIS_RES_IX,	DIS_RES_IX,	DIS_RES_IX,

/* 0xc0 - 0xc3 */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
/* 0xc4 - 0xc7 */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
/* 0xc8 - 0xcb */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
/* 0xcc - 0xcf */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,

/* 0xd0 - 0xd3 */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
/* 0xd4 - 0xd7 */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
/* 0xd8 - 0xdb */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
/* 0xdc - 0xdf */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,

/* 0xe0 - 0xe3 */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
/* 0xe4 - 0xe7 */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
/* 0xe8 - 0xeb */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
/* 0xec - 0xef */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,

/* 0xf0 - 0xf3 */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
/* 0xf4 - 0xf7 */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
/* 0xf8 - 0xfb */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
/* 0xfc - 0xff */    DIS_SET_IX,DIS_SET_IX,	DIS_SET_IX,	DIS_SET_IX,
		    };

/* DIS_ED opcodes
*/
DIS_OP_CALLBACK	dis_ED_opcode[0x100]=
		    {
/* 0x00 - 0x03 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0x04 - 0x07 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0x08 - 0x0b */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,	
/* 0x0c - 0x0f */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,		

/* 0x10 - 0x13 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0x14 - 0x17 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0x18 - 0x1b */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,	
/* 0x1c - 0x1f */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,		

/* 0x20 - 0x23 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0x24 - 0x27 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0x28 - 0x2b */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,	
/* 0x2c - 0x2f */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,		

/* 0x30 - 0x33 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0x34 - 0x37 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0x38 - 0x3b */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,	
/* 0x3c - 0x3f */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,		

/* 0x40 - 0x43 */    DIS_IN_R8_C,	DIS_OUT_C_R8,	DIS_SBC_HL_R16,	DIS_ED_LD_ADDR_R16,
/* 0x44 - 0x47 */    DIS_NEG,	DIS_RETN,		DIS_IM_0,		DIS_LD_I_A,
/* 0x48 - 0x4b */    DIS_IN_R8_C,	DIS_OUT_C_R8,	DIS_ADC_HL_R16,	DIS_ED_LD_R16_ADDR,
/* 0x4c - 0x4f */    DIS_NEG,	DIS_RETI,		DIS_IM_0,		DIS_LD_R_A,

/* 0x50 - 0x53 */    DIS_IN_R8_C,	DIS_OUT_C_R8,	DIS_SBC_HL_R16,	DIS_ED_LD_ADDR_R16,
/* 0x54 - 0x57 */    DIS_NEG,	DIS_RETN,		DIS_IM_1,		DIS_LD_A_I,
/* 0x58 - 0x5b */    DIS_IN_R8_C,	DIS_OUT_C_R8,	DIS_ADC_HL_R16,	DIS_ED_LD_R16_ADDR,
/* 0x5c - 0x5f */    DIS_NEG,	DIS_RETI,		DIS_IM_2,		DIS_LD_A_R,

/* 0x60 - 0x63 */    DIS_IN_R8_C,	DIS_OUT_C_R8,	DIS_SBC_HL_R16,	DIS_ED_LD_ADDR_R16,
/* 0x64 - 0x67 */    DIS_NEG,	DIS_RETN,		DIS_IM_0,		DIS_RRD,
/* 0x68 - 0x6b */    DIS_IN_R8_C,	DIS_OUT_C_R8,	DIS_ADC_HL_R16,	DIS_ED_LD_R16_ADDR,
/* 0x6c - 0x6f */    DIS_NEG,	DIS_RETI,		DIS_IM_0,		DIS_RLD,

/* 0x70 - 0x73 */    DIS_IN_R8_C,	DIS_OUT_C_R8,	DIS_SBC_HL_R16,	DIS_ED_LD_ADDR_R16,
/* 0x74 - 0x77 */    DIS_NEG,	DIS_RETN,		DIS_IM_1,		DIS_ED_NOP,
/* 0x78 - 0x7b */    DIS_IN_R8_C,	DIS_OUT_C_R8,	DIS_ADC_HL_R16,	DIS_ED_LD_R16_ADDR,
/* 0x7c - 0x7f */    DIS_NEG,	DIS_RETI,		DIS_IM_2,		DIS_ED_NOP,

/* 0x80 - 0x83 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0x84 - 0x87 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0x88 - 0x8b */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,	
/* 0x8c - 0x8f */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,		

/* 0x90 - 0x93 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0x94 - 0x97 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0x98 - 0x9b */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,	
/* 0x9c - 0x9f */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,		

/* 0xa0 - 0xa3 */    DIS_LDI,	DIS_CPI,		DIS_INI,		DIS_OUTI,
/* 0xa4 - 0xa7 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0xa8 - 0xab */    DIS_LDD,	DIS_CPD,		DIS_IND,		DIS_OUTD,	
/* 0xac - 0xaf */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,		

/* 0xb0 - 0xb3 */    DIS_LDIR,	DIS_CPIR,		DIS_INIR,		DIS_OTIR,
/* 0xb4 - 0xb7 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0xb8 - 0xbb */    DIS_LDDR,	DIS_CPDR,		DIS_INDR,		DIS_OTDR,	
/* 0xbc - 0xbf */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,		

/* 0xc0 - 0xc3 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0xc4 - 0xc7 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0xc8 - 0xcb */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,	
/* 0xcc - 0xcf */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,		

/* 0xd0 - 0xd3 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0xd4 - 0xd7 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0xd8 - 0xdb */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,	
/* 0xdc - 0xdf */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,		

/* 0xe0 - 0xe3 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0xe4 - 0xe7 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0xe8 - 0xeb */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,	
/* 0xec - 0xef */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,		

/* 0xf0 - 0xf3 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0xf4 - 0xf7 */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,
/* 0xf8 - 0xfb */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,	
/* 0xfc - 0xff */    DIS_ED_NOP,	DIS_ED_NOP,		DIS_ED_NOP,		DIS_ED_NOP,		
		    };

/* DIS_FD opcodes
*/
DIS_OP_CALLBACK	dis_FD_opcode[0x100]=
		    {
/* 0x00 - 0x03 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x04 - 0x07 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x08 - 0x0b */    DIS_FD_NOP,	DIS_ADD_IY_BC,	DIS_FD_NOP,		DIS_FD_NOP,	
/* 0x0c - 0x0f */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,		

/* 0x10 - 0x13 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x14 - 0x17 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x18 - 0x1b */    DIS_FD_NOP,	DIS_ADD_IY_DE,	DIS_FD_NOP,		DIS_FD_NOP,	
/* 0x1c - 0x1f */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,		

/* 0x20 - 0x23 */    DIS_FD_NOP,	DIS_LD_IY_WORD,	DIS_LD_ADDR_IY,	DIS_INC_IY,
/* 0x24 - 0x27 */    DIS_INC_IYH,	DIS_DEC_IYH,	DIS_LD_IYH_BYTE,	DIS_FD_NOP,
/* 0x28 - 0x2b */    DIS_FD_NOP,	DIS_ADD_IY_IY,	DIS_LD_IY_ADDR,	DIS_DEC_IY,
/* 0x2c - 0x2f */    DIS_INC_IYL,	DIS_DEC_IYL,	DIS_LD_IYL_BYTE,	DIS_FD_NOP,

/* 0x30 - 0x33 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x34 - 0x37 */    DIS_INC_IIY,	DIS_DEC_IIY,	DIS_LD_IIY_BYTE,	DIS_FD_NOP,
/* 0x38 - 0x3b */    DIS_FD_NOP,	DIS_ADD_IY_SP,	DIS_FD_NOP,		DIS_FD_NOP,
/* 0x3c - 0x3f */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,

/* 0x40 - 0x43 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x44 - 0x47 */    DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_FD_NOP,
/* 0x48 - 0x4b */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x4c - 0x4f */    DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_FD_NOP,

/* 0x50 - 0x53 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x54 - 0x57 */    DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_FD_NOP,
/* 0x58 - 0x5b */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x5c - 0x5f */    DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_FD_NOP,

/* 0x60 - 0x63 */    DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,
/* 0x64 - 0x67 */    DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,
/* 0x68 - 0x6b */    DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,
/* 0x6c - 0x6f */    DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,

/* 0x70 - 0x73 */    DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,
/* 0x74 - 0x77 */    DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_FD_NOP,		DIS_YLD_R8_R8,
/* 0x78 - 0x7b */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x7c - 0x7f */    DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_YLD_R8_R8,	DIS_FD_NOP,

/* 0x80 - 0x83 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x84 - 0x87 */    DIS_YADD_R8,	DIS_YADD_R8,	DIS_YADD_R8,	DIS_FD_NOP,
/* 0x88 - 0x8b */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x8c - 0x8f */    DIS_YADC_R8,	DIS_YADC_R8,	DIS_YADC_R8,	DIS_FD_NOP,

/* 0x90 - 0x93 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x94 - 0x97 */    DIS_YSUB_R8,	DIS_YSUB_R8,	DIS_YSUB_R8,	DIS_FD_NOP,
/* 0x98 - 0x9b */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0x9c - 0x9f */    DIS_YSBC_R8,	DIS_YSBC_R8,	DIS_YSBC_R8,	DIS_FD_NOP,

/* 0xa0 - 0xa3 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0xa4 - 0xa7 */    DIS_YAND_R8,	DIS_YAND_R8,	DIS_YAND_R8,	DIS_FD_NOP,
/* 0xa8 - 0xab */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0xac - 0xaf */    DIS_YYOR_R8,	DIS_YYOR_R8,	DIS_YYOR_R8,	DIS_FD_NOP,

/* 0xb0 - 0xb3 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0xb4 - 0xb7 */    DIS_Y_OR_R8,	DIS_Y_OR_R8,	DIS_Y_OR_R8,	DIS_FD_NOP,
/* 0xb8 - 0xbb */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0xbc - 0xbf */    DIS_YCP_R8,	DIS_YCP_R8,		DIS_YCP_R8,		DIS_FD_NOP,

/* 0xc0 - 0xc3 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0xc4 - 0xc7 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0xc8 - 0xcb */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_CB_DECODE,
/* 0xcc - 0xcf */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,

/* 0xd0 - 0xd3 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0xd4 - 0xd7 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0xd8 - 0xdb */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0xdc - 0xdf */    DIS_FD_NOP,	DIS_FD_DD_DECODE,	DIS_FD_NOP,		DIS_FD_NOP,

/* 0xe0 - 0xe3 */    DIS_FD_NOP,	DIS_POP_IY,		DIS_FD_NOP,		DIS_EY_ISP_IY,
/* 0xe4 - 0xe7 */    DIS_FD_NOP,	DIS_PUSH_IY,	DIS_FD_NOP,		DIS_FD_NOP,
/* 0xe8 - 0xeb */    DIS_FD_NOP,	DIS_JP_IY,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0xec - 0xef */    DIS_FD_NOP,	DIS_FD_ED_DECODE,	DIS_FD_NOP,		DIS_FD_NOP,

/* 0xf0 - 0xf3 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0xf4 - 0xf7 */    DIS_FD_NOP,	DIS_FD_NOP,		DIS_FD_NOP,		DIS_FD_NOP,
/* 0xf8 - 0xfb */    DIS_FD_NOP,	DIS_LD_SP_IY,	DIS_FD_NOP,		DIS_FD_NOP,
/* 0xfc - 0xff */    DIS_FD_NOP,	DIS_FD_FD_DECODE,	DIS_FD_NOP,		DIS_FD_NOP,
		    };


/* DIS_FD DIS_CB opcodes
*/
DIS_OP_CALLBACK	dis_FD_CB_opcode[0x100]=
		    {
/* 0x00 - 0x03 */    DIS_RLC_IY,	DIS_RLC_IY,		DIS_RLC_IY,		DIS_RLC_IY,
/* 0x04 - 0x07 */    DIS_RLC_IY,	DIS_RLC_IY,		DIS_RLC_IY,		DIS_RLC_IY,
/* 0x08 - 0x0b */    DIS_RRC_IY,	DIS_RRC_IY,		DIS_RRC_IY,		DIS_RRC_IY,
/* 0x0c - 0x0f */    DIS_RRC_IY,	DIS_RRC_IY,		DIS_RRC_IY,		DIS_RRC_IY,

/* 0x10 - 0x13 */    DIS_RL_IY,	DIS_RL_IY,		DIS_RL_IY,		DIS_RL_IY,	
/* 0x14 - 0x17 */    DIS_RL_IY,	DIS_RL_IY,		DIS_RL_IY,		DIS_RL_IY,
/* 0x18 - 0x1b */    DIS_RR_IY,	DIS_RR_IY,		DIS_RR_IY,		DIS_RR_IY,	
/* 0x1c - 0x1f */    DIS_RR_IY,	DIS_RR_IY,		DIS_RR_IY,		DIS_RR_IY,

/* 0x20 - 0x23 */    DIS_SLA_IY,	DIS_SLA_IY,		DIS_SLA_IY,		DIS_SLA_IY,
/* 0x24 - 0x27 */    DIS_SLA_IY,	DIS_SLA_IY,		DIS_SLA_IY,		DIS_SLA_IY,
/* 0x28 - 0x2b */    DIS_SRA_IY,	DIS_SRA_IY,		DIS_SRA_IY,		DIS_SRA_IY,
/* 0x2c - 0x2f */    DIS_SRA_IY,	DIS_SRA_IY,		DIS_SRA_IY,		DIS_SRA_IY,

/* 0x30 - 0x33 */    DIS_SLL_IY,	DIS_SLL_IY,		DIS_SLL_IY,		DIS_SLL_IY,
/* 0x34 - 0x37 */    DIS_SLL_IY,	DIS_SLL_IY,		DIS_SLL_IY,		DIS_SLL_IY,
/* 0x38 - 0x3b */    DIS_SRL_IY,	DIS_SRL_IY,		DIS_SRL_IY,		DIS_SRL_IY,
/* 0x3c - 0x3f */    DIS_SRL_IY,	DIS_SRL_IY,		DIS_SRL_IY,		DIS_SRL_IY,

/* 0x40 - 0x43 */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,
/* 0x44 - 0x47 */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,
/* 0x48 - 0x4b */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,
/* 0x4c - 0x4f */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,

/* 0x50 - 0x53 */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,
/* 0x54 - 0x57 */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,
/* 0x58 - 0x5b */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,
/* 0x5c - 0x5f */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,

/* 0x60 - 0x63 */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,
/* 0x64 - 0x67 */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,
/* 0x68 - 0x6b */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,
/* 0x6c - 0x6f */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,

/* 0x70 - 0x73 */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,
/* 0x74 - 0x77 */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,
/* 0x78 - 0x7b */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,
/* 0x7c - 0x7f */    DIS_BIT_IY,DIS_BIT_IY,	DIS_BIT_IY,	DIS_BIT_IY,

/* 0x80 - 0x83 */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,
/* 0x84 - 0x87 */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,
/* 0x88 - 0x8b */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,
/* 0x8c - 0x8f */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,

/* 0x90 - 0x93 */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,
/* 0x94 - 0x97 */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,
/* 0x98 - 0x9b */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,
/* 0x9c - 0x9f */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,

/* 0xa0 - 0xa3 */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,
/* 0xa4 - 0xa7 */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,
/* 0xa8 - 0xab */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,
/* 0xac - 0xaf */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,

/* 0xb0 - 0xb3 */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,
/* 0xb4 - 0xb7 */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,
/* 0xb8 - 0xbb */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,
/* 0xbc - 0xbf */    DIS_RES_IY,DIS_RES_IY,	DIS_RES_IY,	DIS_RES_IY,

/* 0xc0 - 0xc3 */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
/* 0xc4 - 0xc7 */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
/* 0xc8 - 0xcb */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
/* 0xcc - 0xcf */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,

/* 0xd0 - 0xd3 */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
/* 0xd4 - 0xd7 */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
/* 0xd8 - 0xdb */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
/* 0xdc - 0xdf */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,

/* 0xe0 - 0xe3 */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
/* 0xe4 - 0xe7 */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
/* 0xe8 - 0xeb */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
/* 0xec - 0xef */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,

/* 0xf0 - 0xf3 */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
/* 0xf4 - 0xf7 */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
/* 0xf8 - 0xfb */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
/* 0xfc - 0xff */    DIS_SET_IY,DIS_SET_IY,	DIS_SET_IY,	DIS_SET_IY,
		    };

/* DIS_First/single byte opcodes
*/
DIS_OP_CALLBACK	dis_opcode_z80[0x100]=
		    {
/* 0x00 - 0x03 */    DIS_NOP,	DIS_LD_R16_WORD,	DIS_LD_R16_A,	DIS_INC_R16,
/* 0x04 - 0x07 */    DIS_INC_R8,	DIS_DEC_R8,		DIS_LD_R8_BYTE,	DIS_RLCA,
/* 0x08 - 0x0b */    DIS_EX_AF_AF,	DIS_ADD_HL_R16,	DIS_LD_A_R16,	DIS_DEC_R16,	
/* 0x0c - 0x0f */    DIS_INC_R8,	DIS_DEC_R8,		DIS_LD_R8_BYTE,	DIS_RRCA,		

/* 0x10 - 0x13 */    DIS_DJNZ,	DIS_LD_R16_WORD,	DIS_LD_R16_A,	DIS_INC_R16,
/* 0x14 - 0x17 */    DIS_INC_R8,	DIS_DEC_R8,		DIS_LD_R8_BYTE,	DIS_RLA,
/* 0x18 - 0x1b */    DIS_JR,	DIS_ADD_HL_R16,	DIS_LD_A_R16,	DIS_DEC_R16,	
/* 0x1c - 0x1f */    DIS_INC_R8,	DIS_DEC_R8,		DIS_LD_R8_BYTE,	DIS_RRA,		

/* 0x20 - 0x23 */    DIS_JR_CO,	DIS_LD_R16_WORD,	DIS_LD_ADDR_HL,	DIS_INC_R16,
/* 0x24 - 0x27 */    DIS_INC_R8,	DIS_DEC_R8,		DIS_LD_R8_BYTE,	DIS_DAA,
/* 0x28 - 0x2b */    DIS_JR_CO,	DIS_ADD_HL_R16,	DIS_LD_HL_ADDR,	DIS_DEC_R16,	
/* 0x2c - 0x2f */    DIS_INC_R8,	DIS_DEC_R8,		DIS_LD_R8_BYTE,	DIS_CPL,		

/* 0x30 - 0x33 */    DIS_JR_CO,	DIS_LD_R16_WORD,	DIS_LD_ADDR_A,	DIS_INC_R16,
/* 0x34 - 0x37 */    DIS_INC_R8,	DIS_DEC_R8,		DIS_LD_R8_BYTE,	DIS_SCF,
/* 0x38 - 0x3b */    DIS_JR_CO,	DIS_ADD_HL_R16,	DIS_LD_A_ADDR,	DIS_DEC_R16,	
/* 0x3c - 0x3f */    DIS_INC_R8,	DIS_DEC_R8,		DIS_LD_R8_BYTE,	DIS_CCF,		

/* 0x40 - 0x43 */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,
/* 0x44 - 0x47 */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,
/* 0x48 - 0x4b */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,
/* 0x4c - 0x4f */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,

/* 0x50 - 0x53 */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,
/* 0x54 - 0x57 */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,
/* 0x58 - 0x5b */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,
/* 0x5c - 0x5f */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,

/* 0x60 - 0x63 */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,
/* 0x64 - 0x67 */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,
/* 0x68 - 0x6b */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,
/* 0x6c - 0x6f */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,

/* 0x70 - 0x73 */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,
/* 0x74 - 0x77 */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_HALT,		DIS_LD_R8_R8,
/* 0x78 - 0x7b */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,
/* 0x7c - 0x7f */    DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,	DIS_LD_R8_R8,

/* 0x80 - 0x83 */    DIS_ADD_R8,	DIS_ADD_R8,		DIS_ADD_R8,		DIS_ADD_R8,
/* 0x84 - 0x87 */    DIS_ADD_R8,	DIS_ADD_R8,		DIS_ADD_R8,		DIS_ADD_R8,
/* 0x88 - 0x8b */    DIS_ADC_R8,	DIS_ADC_R8,		DIS_ADC_R8,		DIS_ADC_R8,
/* 0x8c - 0x8f */    DIS_ADC_R8,	DIS_ADC_R8,		DIS_ADC_R8,		DIS_ADC_R8,

/* 0x90 - 0x93 */    DIS_SUB_R8,	DIS_SUB_R8,		DIS_SUB_R8,		DIS_SUB_R8,
/* 0x94 - 0x97 */    DIS_SUB_R8,	DIS_SUB_R8,		DIS_SUB_R8,		DIS_SUB_R8,
/* 0x98 - 0x9b */    DIS_SBC_R8,	DIS_SBC_R8,		DIS_SBC_R8,		DIS_SBC_R8,
/* 0x9c - 0x9f */    DIS_SBC_R8,	DIS_SBC_R8,		DIS_SBC_R8,		DIS_SBC_R8,

/* 0xa0 - 0xa3 */    DIS_AND_R8,	DIS_AND_R8,		DIS_AND_R8,		DIS_AND_R8,
/* 0xa4 - 0xa7 */    DIS_AND_R8,	DIS_AND_R8,		DIS_AND_R8,		DIS_AND_R8,
/* 0xa8 - 0xab */    DIS_XOR_R8,	DIS_XOR_R8,		DIS_XOR_R8,		DIS_XOR_R8,
/* 0xac - 0xaf */    DIS_XOR_R8,	DIS_XOR_R8,		DIS_XOR_R8,		DIS_XOR_R8,

/* 0xb0 - 0xb3 */    DIS_OR_R8,	DIS_OR_R8,		DIS_OR_R8,		DIS_OR_R8,
/* 0xb4 - 0xb7 */    DIS_OR_R8,	DIS_OR_R8,		DIS_OR_R8,		DIS_OR_R8,
/* 0xb8 - 0xbb */    DIS_CP_R8,	DIS_CP_R8,		DIS_CP_R8,		DIS_CP_R8,
/* 0xbc - 0xbf */    DIS_CP_R8,	DIS_CP_R8,		DIS_CP_R8,		DIS_CP_R8,

/* 0xc0 - 0xc3 */    DIS_RET_CO,	DIS_POP_R16,	DIS_JP_CO,		DIS_JP,
/* 0xc4 - 0xc7 */    DIS_CALL_CO,	DIS_PUSH_R16,	DIS_ADD_A_BYTE,	DIS_RST,
/* 0xc8 - 0xcb */    DIS_RET_CO,	DIS_RET,		DIS_JP_CO,		DIS_CB_DECODE,
/* 0xcc - 0xcf */    DIS_CALL_CO,	DIS_CALL,		DIS_ADC_A_BYTE,	DIS_RST,

/* 0xd0 - 0xd3 */    DIS_RET_CO,	DIS_POP_R16,	DIS_JP_CO,		DIS_OUT_BYTE_A,
/* 0xd4 - 0xd7 */    DIS_CALL_CO,	DIS_PUSH_R16,	DIS_SUB_A_BYTE,	DIS_RST,
/* 0xd8 - 0xdb */    DIS_RET_CO,	DIS_EXX,		DIS_JP_CO,		DIS_IN_A_BYTE,
/* 0xdc - 0xdf */    DIS_CALL_CO,	DIS_DD_DECODE,	DIS_SBC_A_BYTE,	DIS_RST,

/* 0xe0 - 0xe3 */    DIS_RET_CO,	DIS_POP_R16,	DIS_JP_CO,		DIS_EX_ISP_HL,
/* 0xe4 - 0xe7 */    DIS_CALL_CO,	DIS_PUSH_R16,	DIS_AND_A_BYTE,	DIS_RST,
/* 0xe8 - 0xeb */    DIS_RET_CO,	DIS_JP_HL,		DIS_JP_CO,		DIS_EX_DE_HL,
/* 0xec - 0xef */    DIS_CALL_CO,	DIS_ED_DECODE,	DIS_XOR_A_BYTE,	DIS_RST,

/* 0xf0 - 0xf3 */    DIS_RET_CO,	DIS_POP_R16,	DIS_JP_CO,		DIS_DI,
/* 0xf4 - 0xf7 */    DIS_CALL_CO,	DIS_PUSH_R16,	DIS_OR_A_BYTE,	DIS_RST,
/* 0xf8 - 0xfb */    DIS_RET_CO,	DIS_LD_SP_HL,	DIS_JP_CO,		DIS_EI,
/* 0xfc - 0xff */    DIS_CALL_CO,	DIS_FD_DECODE,	DIS_CP_A_BYTE,	DIS_RST,
		    };


#endif

/* END OF FILE */
