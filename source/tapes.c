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
  
   $Id: tapes.c,v 1.12 2008-11-03 17:07:31 ianc Exp $
*/

#include <nds.h>

#include "tapes.h"
#include "framebuffer.h"
#include "keyboard.h"
#include "zx81.h"

#include "maze_bin.h"
#include "maze_inlay_bin.h"
#include "cpatrol_bin.h"
#include "cpatrol_inlay_bin.h"
#include "sabotage_bin.h"
#include "sabotage_inlay_bin.h"
#include "mazogs_bin.h"
#include "mazogs_inlay_bin.h"

#include "ds81_debug.h"


/* ---------------------------------------- STATIC DATA
*/
typedef struct
{
    const u8	*tape;
    const u8	*tape_end;
    sImage	img;
    const void	*source_pcx;
    SoftKey	*keys;
    const char	*text;
} Tape;

#define NO_TAPES	4

static SoftKey	maze_keys[]=
		    {
		    	SK_PAD_UP,	SK_7,
		    	SK_PAD_LEFT,	SK_5,
		    	SK_PAD_RIGHT,	SK_8,
			SK_PAD_START,	SK_C,
			SK_PAD_SELECT,	SK_A,
			NUM_SOFT_KEYS
		    };

static SoftKey	cpatrol_keys[]=
		    {
		    	SK_PAD_UP,	SK_F,
		    	SK_PAD_RIGHT,	SK_J,
		    	SK_PAD_LEFT,	SK_N,
			SK_PAD_DOWN,	SK_V,
		    	SK_PAD_R,	SK_N,
		    	SK_PAD_L,	SK_J,
			SK_PAD_A,	SK_0,
			SK_PAD_B,	SK_NEWLINE,
			NUM_SOFT_KEYS
		    };

static SoftKey	sabotage_keys[]=
		    {
		    	SK_PAD_UP,	SK_W,
		    	SK_PAD_LEFT,	SK_H,
		    	SK_PAD_RIGHT,	SK_J,
			SK_PAD_DOWN,	SK_S,
			SK_PAD_A,	SK_E,
			SK_PAD_R,	SK_1,
			SK_PAD_L,	SK_2,
			SK_PAD_START,	SK_0,
			NUM_SOFT_KEYS
		    };

static SoftKey	mazogs_keys[]=
		    {
		    	SK_PAD_UP,	SK_W,
		    	SK_PAD_LEFT,	SK_H,
		    	SK_PAD_RIGHT,	SK_J,
			SK_PAD_DOWN,	SK_S,
			SK_PAD_A,	SK_NEWLINE,
			SK_PAD_R,	SK_R,
			SK_PAD_L,	SK_L,
			SK_PAD_START,	SK_Y,
			SK_PAD_SELECT,	SK_V,
			NUM_SOFT_KEYS
		    };

static Tape	tapes[NO_TAPES]=
		{
		    {
		    	maze_bin,
		    	maze_bin_end,
			{0},
			maze_inlay_bin,
			maze_keys,
			"%3d monster maze%\n"
			"(c) 1983 Malcolm E. Evans\n\n"
			"Escape the maze and its T-Rex\n\n"
			"use joypad for turning and to\n"
			"move forward.\n"
			"%start% to start.\n"
			"%select% to appeal.\n\n"
			"%note% when the screen goes grey\n"
			"for 30-60 seconds this is not a\n"
			"problem - the game is creating\n"
			"the maze."
		    },
		    {
		    	mazogs_bin,
		    	mazogs_bin_end,
			{0},
			mazogs_inlay_bin,
			mazogs_keys,
			"%Mazogs%\n"
			"(c) 1981 Don Priestley\n\n"
			"Find the treasure and\n"
			"return to the start.\n"
			"Avoid the %Mazogs% that roam\n"
			"the maze.\n\n"
			"Use joypad to move.\n"
			"%select% to view map.\n"
			"%start% to quit.\n"
			"%L% or %R% shoulder to select\n"
			"direction at start."
		    },
		    {
		    	cpatrol_bin,
		    	cpatrol_bin_end,
			{0},
			cpatrol_inlay_bin,
			cpatrol_keys,
			"%city patrol%\n"
			"(c) 1982 Don Priestley\n\n"
			"Defend the city from the aliens.\n\n"
			"yes - that parallax city was\n"
			"done with a text mode and the\n"
			"equivalent of a 0.8mhz z80\n\n"
			"the joypad controls the cursor.\n"
			"hold %L% or %R% shoulder buttons\n"
			"to move fast when moving in the\n"
			"same direction.\n\n"
			"%A% fires when moving.\n"
			"%B% fires when still.\n\n"
			"sorry about that, but the keys\n"
			"are a bit odd in this game."
		    },
		    {
		    	sabotage_bin,
		    	sabotage_bin_end,
			{0},
			sabotage_inlay_bin,
			sabotage_keys,
			"%sabotage%\n"
			"(c) 1982 Don Priestley\n\n"
			"Destroy the boxes before the\n"
			"guard finds you.\n\n"
			"or find the saboteur as the\n"
			"guard.\n\n"
			"while this game may not feature\n"
			"the dazzling graphics of other\n"
			"ZX81 games it more than makes\n"
			"up with a simply joyous\n"
			"gameplay mechanic.\n\n"
			"The joypad controls the player.\n"
			"%A% plants a bomb.  %L% shoulder\n"
			"to play as the guard, %R% as\n"
			"the saboteur."
		    }
		};


static int	current=0;

/* ---------------------------------------- PRIVATE INTERFACES
*/
static void InitTapes(void)
{
    static int init=FALSE;
    int f;

    if (init)
    {
    	return;
    }

    init=TRUE;

    for(f=0;f<NO_TAPES;f++)
    {
    	loadPCX(tapes[f].source_pcx,&tapes[f].img);
    }
}

static void DisplayTape(Tape *t)
{
    FB_Clear();
    FB_Blit(&t->img,255-t->img.width,0,1);

    FB_Print("LEFT/RIGHT",0,0,COL_WHITE,COL_TRANSPARENT);
    FB_Print("to choose",0,10,COL_WHITE,COL_TRANSPARENT);
    FB_Print("A to select",0,30,COL_WHITE,COL_TRANSPARENT);
    FB_Print("B to cancel",0,40,COL_WHITE,COL_TRANSPARENT);
    FB_Print("REMEMBER TO",0,60,COL_WHITE,COL_TRANSPARENT);
    FB_Print("LOAD \"\"",0,70,COL_WHITE,COL_TRANSPARENT);
    FB_Print("ON THE ZX81!",0,80,COL_WHITE,COL_TRANSPARENT);

    ZX81DisplayString(t->text);
}

/* ---------------------------------------- PUBLIC INTERFACES
*/
void SelectTape(void)
{
    int done=FALSE;

    InitTapes();

    ZX81SuspendDisplay();

    while(!done)
    {
	uint32 key=0;

    	DisplayTape(tapes+current);

        do
        {
            swiWaitForVBlank();
        } while(!(key=keysDownRepeat()));

	if (key & KEY_LEFT)
	{
	    if (--current<0)
	    {
	    	current=NO_TAPES-1;
	    }
	}
	else if (key & KEY_RIGHT)
	{
	    current=(current+1)%NO_TAPES;
	}
	else if (key & KEY_A)
	{
	    int f;

	    done=TRUE;
	    ZX81SetTape(tapes[current].tape,
	    		tapes[current].tape_end - tapes[current].tape);

	    for(f=0;tapes[current].keys[f]!=NUM_SOFT_KEYS;f+=2)
	    {
	    	SK_DefinePad(tapes[current].keys[f],
			     tapes[current].keys[f+1]);
	    }
	}
	else if (key & KEY_B)
	{
	    done=TRUE;
	}
    }

    ZX81ResumeDisplay();
}

