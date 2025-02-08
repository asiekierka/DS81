/*
   ds81 - Nintendo ZX81 emulator.

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
  
   $Id: keyboard.h,v 1.6 2008-11-03 17:07:31 ianc Exp $
*/
#ifndef DS81_KEYBOARD_H
#define DS81_KEYBOARD_H

#include <stdio.h>

/* Note that the first 40 values purposefully are the keyboard matrix keys.
   Note also that they are in display order, not matrix order.
*/
typedef enum
{
    SK_1,
    SK_2,
    SK_3,
    SK_4,
    SK_5,

    SK_6,
    SK_7,
    SK_8,
    SK_9,
    SK_0,

    SK_Q,
    SK_W,
    SK_E,
    SK_R,
    SK_T,

    SK_Y,
    SK_U,
    SK_I,
    SK_O,
    SK_P,

    SK_A,
    SK_S,
    SK_D,
    SK_F,
    SK_G,

    SK_H,
    SK_J,
    SK_K,
    SK_L,
    SK_NEWLINE,

    SK_SHIFT,
    SK_Z,
    SK_X,
    SK_C,
    SK_V,

    SK_B,
    SK_N,
    SK_M,
    SK_PERIOD,
    SK_SPACE,

    SK_ABOUT,
    SK_CONFIG,
    SK_PAD_UP,
    SK_PAD_DOWN,
    SK_PAD_LEFT,
    SK_PAD_RIGHT,
    SK_PAD_A,
    SK_PAD_B,
    SK_PAD_X,
    SK_PAD_Y,
    SK_PAD_R,
    SK_PAD_L,
    SK_PAD_START,
    SK_PAD_SELECT,

    NUM_SOFT_KEYS
} SoftKey;

typedef struct
{
    SoftKey	key;
    int		pressed;
} SoftKeyEvent;


/* Display the soft keyboard.
*/
void	SK_DisplayKeyboard(void);

/* If dim is TRUE, then the keyboard is displayed with reduced brightness along
   with the selection box.  This routine simply adjusts the palette, and
   assumes that the keyboard is already on display.
*/
void	SK_SetDisplayBrightness(int dim);

/* Returns TRUE while there are still key events for this cycle
*/
int	SK_GetEvent(SoftKeyEvent *ev);

/* Returns TRUE while there are still key events for this cycle.  Unlike 
   SK_GetEvent this does not do joypad mappings.
*/
int	SK_GetBareEvent(SoftKeyEvent *ev);

/* Sets a key to be 'sticky'.
*/
void	SK_SetSticky(SoftKey key, int is_sticky);

/* Map the joypad to keys.  Note that when mapped that both the key and the
   joypad code will be generated.
*/
void	SK_DefinePad(SoftKey pad, SoftKey key);

/* Returns a name for key symbols.
*/
const char *SK_KeyName(SoftKey pad);

/* Allows the keyboard to save/restore its state from a stream
*/
void	SK_SaveSnapshot(FILE *fp);
void	SK_LoadSnapshot(FILE *fp);

#endif	/* DS81_KEYBOARD_H */
