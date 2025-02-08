/*

    ds81 - Nintendo DS ZX81 emulator

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

    Provides the emulation for the ZX81

*/

#ifndef DS81_ZX81_H
#define DS81_ZX81_H

#include <stdio.h>

#include "z80.h"
#include "keyboard.h"


/* Initialise the ZX81
*/
void	ZX81Init(uint16 *text_vram, uint16 *bitmap_vram, Z80 *z80);

/* Handle keypresses
*/
void	ZX81HandleKey(SoftKey k, int is_pressed);

/* Enable fopen() loading of tape files
*/
void	ZX81EnableFileSystem(int enable);

/* Set a file to load from tape
*/
void	ZX81SetTape(const Z80Byte *image, int len);

/* Reset the 81
*/
void	ZX81Reset(Z80 *z80);

/* Tell the 81 that config may have changed.
*/
void	ZX81Reconfigure(void);

/* Displays a string on the ZX81's dislpay.  The screen is cleared and the
   string displayed with \n characters breaking the line.

   Not all characters can be respresented by the ZX81, and the screen will be
   lost on the next emulation update cycle.

   The character '%' toggles inverse video.

   ZX81SuspendDisplay() and ZX81ResumeDisplay() should be called so that the
   ZX81 can set up its internals.
*/
void	ZX81DisplayString(const char *p);
void	ZX81SuspendDisplay(void);
void	ZX81ResumeDisplay(void);

/* Interfaces for the Z80
*/
Z80Byte	ZX81ReadMem(Z80 *z80, Z80Word addr);
void	ZX81WriteMem(Z80 *z80, Z80Word addr, Z80Byte val);
Z80Byte	ZX81ReadPort(Z80 *z80, Z80Word port);
void	ZX81WritePort(Z80 *z80, Z80Word port, Z80Byte val);

#define	ZX81ReadDisassem ZX81ReadMem

/* Interfaces to allows the ZX81 to save/load itself as a snapshot to/from
   a stream.
*/
void	ZX81SaveSnapshot(FILE *fp);
void	ZX81LoadSnapshot(FILE *fp);

#endif


/* END OF FILE */
