/*
   ds81 - Nintendo DS ZX81 emulator.

   Copyright (C) 2006  Ian Cowburn <ianc@noddybox.co.uk>
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
   $Id: monitor.c,v 1.6 2007-03-25 19:06:31 ianc Exp $
*/

#include <nds.h>
#include <stdlib.h>

#include "monitor.h"
#include "keyboard.h"
#include "textmode.h"
#include "framebuffer.h"
#include "zx81.h"
#include "config.h"

/* ---------------------------------------- PRIVATE DATA AND TYPES
*/
typedef enum
{
    DISPLAY_ADDR,
    DISPLAY_HL,
    DISPLAY_IX,
    DISPLAY_SP,
    DISPLAY_IY,
    DISPLAY_BC,
    DISPLAY_DE,
    DISPLAY_PC,
    DISPLAY_TYPE_COUNT
} MemDisplayType;

typedef enum
{
    MODE_CPU_STATE,
    MODE_HEX,
    MODE_ASSEM,
    MODE_TYPE_COUNT
} DisplayModeType;


/* ---------------------------------------- STATIC INTERFACES
*/
static void DisplayHelp()
{
    static const char *help[]=
    	{
	/*   12345678901234567890123456789012	*/
	    "MONITOR HELP",
	    "",
	    "Click on the config bar to exit.",
	    "",
	    "Press START to toggle between",
	    "single step mode and running.",
	    "",
	    "Press SELECT to toggle between",
	    "CPU info and memory display.",
	    "",
	    "In single step mode press A",
	    "to execute next instruction.",
	    "",
	    "Use L/R (+ Y for larger jumps)",
	    "to alter address in mem display.",
	    "",
	    "Press B to cycle between address",
	    "and register memory display.",
	    "",
	    "Note that all numbers are in hex",
	    "and the all keyboard keys are",
	    "sticky until the monitor exits.",
	    "",
	    "Press X to continue",
	    NULL
	};

    int f;

    TM_Cls();

    for(f=0; help[f]; f++)
    {
    	TM_Put(0,f,help[f]);
    }

    while(!(keysDownRepeat() & KEY_X))
    {
    	swiWaitForVBlank();
    }
}

static void DisplayRunningState(int running)
{
    if (running)
    {
	TM_Put(0,23,"RUNNING     [PRESS X FOR HELP]");
    }
    else
    {
	TM_Put(0,23,"SINGLE STEP [PRESS X FOR HELP]");
    }
}


static void DisplayCPU(Z80 *cpu)
{
    static const char *flag_char = "SZ5H3PNC";
    Z80Word tmp;
    int f;
    char flags[]="--------";

    tmp = cpu->PC;

    /* Display disassembly
    */
    for(f=0;f<17;f++)
    {
	/* These may seem a bit convuluted, but there's no point being at home
	   to Mr Undefined Behaviour
	*/
	TM_printf(0,f,"%c%4.4x:",f==0 ? '>':' ',tmp);
	TM_Put(7,f,Z80Disassemble(cpu,&tmp));
    }

    /* Display process state
    */
    tmp = cpu->AF.b[Z80_LO_WORD];

    for(f=0;f<8;f++)
    {
    	if (tmp & 1<<(7-f))
	{
	    flags[f] = flag_char[f];
	}
    }

    TM_printf(0,18,"A:%2.2x  F:%s    IM:%2.2x",
    			cpu->AF.b[Z80_HI_WORD],flags,cpu->IM);

    TM_printf(0,19,"BC:%4.4x   DE:%4.4x   HL:%4.4x",
    			cpu->BC.w,cpu->DE.w,cpu->HL.w);

    TM_printf(0,20,"IX:%4.4x   IY:%4.4x   SP:%4.4x",
    			cpu->IX.w,cpu->IY.w,cpu->SP);

    TM_printf(0,21,"PC:%4.4x   IF:%d/%d    IR:%2.2x%2.2x",
    			cpu->PC,cpu->IFF1,cpu->IFF2,cpu->I,cpu->R);
}


static void DisplayMem(Z80 *cpu, MemDisplayType disp, Z80Word addr, int as_hex)
{
    static const char *label[]=
    {
	"Address",
	"HL",
	"IX",
	"SP",
	"IY",
	"BC",
	"DE",
	"PC"
    };

    int x,y;

    switch(disp)
    {
	case DISPLAY_HL:
	    addr = cpu->HL.w;
	    break;

	case DISPLAY_IX:
	    addr = cpu->IX.w;
	    break;

	case DISPLAY_SP:
	    addr = cpu->SP;
	    break;

	case DISPLAY_IY:
	    addr = cpu->IY.w;
	    break;

	case DISPLAY_BC:
	    addr = cpu->BC.w;
	    break;

	case DISPLAY_DE:
	    addr = cpu->DE.w;
	    break;

	case DISPLAY_PC:
	    addr = cpu->PC;
	    break;

	default:
	    break;
    }

    TM_printf(0,0,"%s: %4.4x",label[disp],addr);

    if (as_hex)
    {
	for(y=0;y<20;y++)
	{
	    TM_printf(0,y+2,"%4.4x:",addr);

	    for(x=0;x<8;x++)
	    {
		TM_printf(6+x*3,y+2,"%2.2x",ZX81ReadDisassem(cpu,addr++));
	    }
	}
    }
    else
    {
	for(y=0;y<20;y++)
	{
	    TM_printf(0,y+2,"%4.4x:",addr);
	    TM_Put(7,y+2,Z80Disassemble(cpu,&addr));
	}
    }
}


/* ---------------------------------------- PUBLIC INTERFACES
*/
void MachineCodeMonitor(Z80 *cpu)
{
    static Z80Word display_address = 0x4000;
    static MemDisplayType mem_display = DISPLAY_ADDR;
    int done = FALSE;
    int running = FALSE;
    DisplayModeType display_mode = MODE_CPU_STATE;
    int key;
    SoftKey soft_key;

    SK_DisplayKeyboard();
    SK_SetDisplayBrightness(TRUE);

    for(soft_key = SK_1; soft_key <= SK_SPACE; soft_key++)
    {
    	SK_SetSticky(soft_key,TRUE);
    }

    while(!done)
    {
	TM_Cls();
	DisplayRunningState(running);

	switch(display_mode)
	{
	    case MODE_CPU_STATE:
		DisplayCPU(cpu);
		break;
	    case MODE_HEX:
		DisplayMem(cpu,mem_display,display_address,TRUE);
		break;
	    case MODE_ASSEM:
		DisplayMem(cpu,mem_display,display_address,FALSE);
		break;
	    default:
		TM_Put(0,0,"Oops!");
	    	break;
	}

	do
	{
	    SoftKeyEvent ev;

	    swiWaitForVBlank();

	    while(SK_GetBareEvent(&ev))
	    {
	    	ZX81HandleKey(ev.key,ev.pressed);

		if (ev.key == SK_CONFIG && ev.pressed)
		{
		    done = TRUE;
		}
	    }

	    key = (keysDownRepeat() & ~KEY_TOUCH);

	} while (!done && !running && !key);

	if (key & KEY_START)
	{
	    running = !running;
	}

	if (key & KEY_X)
	{
	    DisplayHelp();
	}

	if (key & KEY_SELECT)
	{
	    display_mode = (display_mode+1) % MODE_TYPE_COUNT;
	}

	if (key & KEY_L)
	{
	    if (keysHeld() & KEY_Y)
	    {
		display_address -= 512;
	    }
	    else
	    {
		display_address -= 64;
	    }
	}

	if (key & KEY_R)
	{
	    if (keysHeld() & KEY_Y)
	    {
		display_address += 512;
	    }
	    else
	    {
		display_address += 64;
	    }
	}

	if (key & KEY_B)
	{
	    mem_display = (mem_display+1) % DISPLAY_TYPE_COUNT;
	}

	if (running || (key & KEY_A))
	{
	    Z80SingleStep(cpu);
	}
    }

    SK_SetDisplayBrightness(FALSE);
    TM_Cls();

    for(soft_key = SK_1; soft_key <= SK_SPACE; soft_key++)
    {
    	SK_SetSticky(soft_key,FALSE);
	ZX81HandleKey(soft_key,FALSE);
    }

    SK_SetSticky(SK_SHIFT,DS81_Config[DS81_STICKY_SHIFT]);

}
