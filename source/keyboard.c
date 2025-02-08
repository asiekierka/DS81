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
  
   $Id: keyboard.c,v 1.15 2008-12-05 00:37:25 ianc Exp $
*/

#include <nds.h>

#include "keyboard.h"
#include "framebuffer.h"
#include "touchwrap.h"
#include "keyb_bin.h"
#include "stream.h"

/* ---------------------------------------- STATIC DATA
*/

#define PAL_OFFSET	110

static int is_dim = FALSE;
static int selection_on = COL_WHITE;
static int selection_off = COL_BLACK;

static struct
{
    int	state;
    int	new_state;
    int	handled;
    int is_sticky;
} key_state[NUM_SOFT_KEYS];

static SoftKey	pad_left_key	= SK_5;
static SoftKey	pad_right_key	= SK_8;
static SoftKey	pad_up_key	= SK_7;
static SoftKey	pad_down_key	= SK_6;
static SoftKey	pad_A_key	= SK_0;
static SoftKey	pad_B_key	= SK_NEWLINE;
static SoftKey	pad_X_key	= NUM_SOFT_KEYS;
static SoftKey	pad_Y_key	= NUM_SOFT_KEYS;
static SoftKey	pad_R_key	= NUM_SOFT_KEYS;
static SoftKey	pad_L_key	= NUM_SOFT_KEYS;
static SoftKey	pad_start_key	= NUM_SOFT_KEYS;
static SoftKey	pad_select_key	= NUM_SOFT_KEYS;

#define CLEAR_STATE(SHORTCUT)					\
	do							\
	{							\
	    if (SHORTCUT != NUM_SOFT_KEYS &&			\
		!key_state[SHORTCUT].handled)			\
	    {							\
		key_state[SHORTCUT].new_state = FALSE;		\
	    }							\
	} while(0)

#define CHECK_STATE(KEYS,BIT,CODE,SHORTCUT,USE_SHORTCUT)	\
	do							\
	{							\
	    key_state[CODE].new_state = (KEYS & BIT);		\
	    if (USE_SHORTCUT && SHORTCUT != NUM_SOFT_KEYS &&	\
		!key_state[SHORTCUT].handled && (KEYS & BIT))	\
	    {							\
		key_state[SHORTCUT].new_state = TRUE;		\
	    }							\
	} while(0)


static const char *keynames[]=
{
    "1", "2", "3", "4", "5",
    "6", "7", "8", "9", "0",
    "Q", "W", "E", "R", "T",
    "Y", "U", "I", "O", "P",
    "A", "S", "D", "F", "G",
    "H", "J", "K", "L", "NEWLINE",
    "SHIFT", "Z", "X", "C", "V",
    "B", "N", "M", "PERIOD", "SPACE",

    "ABOUT",
    "CONFIG",
    "JOYPAD UP",
    "JOYPAD DOWN",
    "JOYPAD LEFT",
    "JOYPAD RIGHT",
    "A BUTTON",
    "B BUTTON",
    "X BUTTON",
    "Y BUTTON",
    "RIGHT SHOULDER BUTTON",
    "LEFT SHOULDER BUTTON",
    "START BUTTON",
    "SELECT BUTTON"
};

/* ---------------------------------------- PRIVATE INTERFACES
*/
static SoftKey LocatePress(const touchPosition *p)
{
    int kx=0,ky=0;
    int py=0;
    SoftKey key = NUM_SOFT_KEYS;

    if (p->py > 36 && p->px > 2)
    {
	kx = (p->px - 3) / 25;
	ky = p->py - 37;

	py = ky % 30;
	ky /= 30;

	if (py<17 && kx >= 0 && kx<10 && ky>=0 && ky<=4)
	{
	    key = kx + ky * 10;
	}
    }

    if (key>SK_SPACE)
    {
	key = NUM_SOFT_KEYS;
    }

    return key;
}


static int GetEvent(SoftKeyEvent *ev, int map)
{
    static SoftKey last = NUM_SOFT_KEYS;
    static int poll_index = -1;

    /* Read the keys if this is a new loop
    */
    if (poll_index == -1)
    {
	int f;
	uint32 keys;

	keys = keysHeld();

	/* Clear the non-sticky keys
	*/
	for(f=SK_1; f<=SK_CONFIG; f++)
	{
	    key_state[f].handled = FALSE;

	    if (key_state[f].is_sticky)
	    {
		key_state[f].new_state = key_state[f].state;
	    }
	    else
	    {
		key_state[f].new_state = FALSE;
	    }
	}

	/* Check the soft keyboard
	*/
	if (keys & KEY_TOUCH)
	{
	    touchPosition tp;

	    if (AllowTouch(&tp))
	    {
		if (tp.py<21 || tp.py>165)
		{
		    key_state[SK_CONFIG].new_state = TRUE;
		}
		else
		{
		    SoftKey press;

		    press = LocatePress(&tp);

		    if (press != NUM_SOFT_KEYS)
		    {
			key_state[press].handled = TRUE;

			if (key_state[press].is_sticky)
			{
			    if (last != press)
			    {
				key_state[press].new_state =
					    !key_state[press].state;
			    }
			}
			else
			{
			    key_state[press].new_state = TRUE;
			}

			last = press;
		    }
		}
	    }
	}
	else
	{
	    last = NUM_SOFT_KEYS;
	}

	/* Check non soft-keyboard controls
	*/
	CHECK_STATE(keys, KEY_A,	SK_PAD_A,	pad_A_key,	map);
	CHECK_STATE(keys, KEY_B,	SK_PAD_B,	pad_B_key,	map);
	CHECK_STATE(keys, KEY_X,	SK_PAD_X,	pad_X_key,	map);
	CHECK_STATE(keys, KEY_Y,	SK_PAD_Y,	pad_Y_key,	map);
	CHECK_STATE(keys, KEY_R,	SK_PAD_R,	pad_R_key,	map);
	CHECK_STATE(keys, KEY_L,	SK_PAD_L,	pad_L_key,	map);
	CHECK_STATE(keys, KEY_START,	SK_PAD_START,	pad_start_key,	map);
	CHECK_STATE(keys, KEY_SELECT,	SK_PAD_SELECT,	pad_select_key,	map);
	CHECK_STATE(keys, KEY_UP,	SK_PAD_UP,	pad_up_key,	map);
	CHECK_STATE(keys, KEY_DOWN,	SK_PAD_DOWN,	pad_down_key,	map);
	CHECK_STATE(keys, KEY_LEFT,	SK_PAD_LEFT,	pad_left_key,	map);
	CHECK_STATE(keys, KEY_RIGHT,	SK_PAD_RIGHT,	pad_right_key,	map);

	/* Reset key event poll index
	*/
	poll_index = 0;

	/* Update any on-screen indicators
	*/
	for(f=SK_1; f<SK_CONFIG; f++)
	{
	    if (key_state[f].state != key_state[f].new_state)
	    {
		int x,y;

		x = 3 + (f % 10) * 25;
		y = 37 + (f / 10) * 30;
	    	
		FB_Box(x, y, 25, 18, key_state[f].new_state ?
						selection_on : selection_off);
	    }
	}
    }

    while(poll_index < NUM_SOFT_KEYS &&
    		key_state[poll_index].state == key_state[poll_index].new_state)
    {
    	poll_index++;
    }

    if (poll_index < NUM_SOFT_KEYS)
    {
	key_state[poll_index].state = key_state[poll_index].new_state;

	ev->key = poll_index;
	ev->pressed = key_state[poll_index].state;

	return TRUE;
    }
    else
    {
    	poll_index = -1;
	return FALSE;
    }
}


/* ---------------------------------------- PUBLIC INTERFACES
*/
void SK_DisplayKeyboard(void)
{
    static sImage img;
    static int loaded;
    int f;

    if (!loaded)
    {
	loadPCX(keyb_bin,&img);
	loaded = true;
    }

    FB_Blit(&img,0,0,PAL_OFFSET);

    /* Update any on-screen indicators
    */
    for(f=SK_1; f<SK_CONFIG; f++)
    {
	if (key_state[f].state)
	{
	    int x,y;

	    x = 3 + (f % 10) * 25;
	    y = 37 + (f / 10) * 30;
	    
	    FB_Box(x, y, 25, 18, key_state[f].new_state ?
					    selection_on : selection_off);
	}
    }
}


void SK_SetDisplayBrightness(int dim)
{
    static uint16 saved_pal[16];
    int f;
    uint16 *pal;

    pal = FB_PALETTE();

    if (dim != is_dim)
    {
    	is_dim = dim;

	if (is_dim)
	{
	    selection_on = COL_DARKGREY;

	    for(f=0;f<16;f++)
	    {
		int r,g,b;

	    	saved_pal[f] = pal[PAL_OFFSET + f];

		r = saved_pal[f] & 0x1f;
		g = (saved_pal[f]>>5) & 0x1f;
		b = (saved_pal[f]>>10) & 0x1f;

		r/=3;
		g/=3;
		b/=3;

		pal[PAL_OFFSET + f] = RGB15(r,g,b);
	    }
	}
	else
	{
	    selection_on = COL_WHITE;

	    for(f=0;f<16;f++)
	    {
	    	pal[PAL_OFFSET + f] = saved_pal[f];
	    }
	}
    }
}


int SK_GetEvent(SoftKeyEvent *ev)
{
    return GetEvent(ev,TRUE);
}


int SK_GetBareEvent(SoftKeyEvent *ev)
{
    return GetEvent(ev,FALSE);
}


void SK_SetSticky(SoftKey key, int is_sticky)
{
    key_state[key].is_sticky = is_sticky;

    if (!is_sticky)
    {
	key_state[key].new_state = FALSE;
    }
}


void SK_DefinePad(SoftKey pad, SoftKey key)
{
    switch(pad)
    {
	case SK_PAD_LEFT:
	    pad_left_key = key;
	    break;
	case SK_PAD_RIGHT:
	    pad_right_key = key;
	    break;
	case SK_PAD_UP:
	    pad_up_key = key;
	    break;
	case SK_PAD_DOWN:
	    pad_down_key = key;
	    break;
	case SK_PAD_A:
	    pad_A_key = key;
	    break;
	case SK_PAD_B:
	    pad_B_key = key;
	    break;
	case SK_PAD_X:
	    pad_X_key = key;
	    break;
	case SK_PAD_Y:
	    pad_Y_key = key;
	    break;
	case SK_PAD_R:
	    pad_R_key = key;
	    break;
	case SK_PAD_L:
	    pad_L_key = key;
	    break;
	case SK_PAD_START:
	    pad_start_key = key;
	    break;
	case SK_PAD_SELECT:
	    pad_select_key = key;
	    break;
	default:
	    break;
    }
}


const char *SK_KeyName(SoftKey k)
{
    return keynames[k];
}


void SK_SaveSnapshot(FILE *fp)
{
    int f;

    PUT_Long(fp, pad_left_key);
    PUT_Long(fp, pad_right_key);
    PUT_Long(fp, pad_up_key);
    PUT_Long(fp, pad_down_key);
    PUT_Long(fp, pad_A_key);
    PUT_Long(fp, pad_B_key);
    PUT_Long(fp, pad_X_key);
    PUT_Long(fp, pad_Y_key);
    PUT_Long(fp, pad_R_key);
    PUT_Long(fp, pad_L_key);
    PUT_Long(fp, pad_start_key);
    PUT_Long(fp, pad_select_key);

    for(f = 0; f < NUM_SOFT_KEYS; f++)
    {
    	PUT_Long(fp, key_state[f].state);
    	PUT_Long(fp, key_state[f].new_state);
    	PUT_Long(fp, key_state[f].handled);
    	PUT_Long(fp, key_state[f].is_sticky);
    }
}


void SK_LoadSnapshot(FILE *fp)
{
    int f;

    pad_left_key = GET_Long(fp);
    pad_right_key = GET_Long(fp);
    pad_up_key = GET_Long(fp);
    pad_down_key = GET_Long(fp);
    pad_A_key = GET_Long(fp);
    pad_B_key = GET_Long(fp);
    pad_X_key = GET_Long(fp);
    pad_Y_key = GET_Long(fp);
    pad_R_key = GET_Long(fp);
    pad_L_key = GET_Long(fp);
    pad_start_key = GET_Long(fp);
    pad_select_key = GET_Long(fp);

    for(f = 0; f < NUM_SOFT_KEYS; f++)
    {
    	key_state[f].state = GET_Long(fp);
    	key_state[f].new_state = GET_Long(fp);
    	key_state[f].handled = GET_Long(fp);
    	key_state[f].is_sticky = GET_Long(fp);
    }
}


