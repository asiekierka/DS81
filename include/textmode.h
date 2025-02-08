/*
   ds81 - Nintendo DS ZX81 emulator.

   Copyright (C) 2007  Ian Cowburn <ianc@noddybox.co.uk>
   
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
  
   $Id: textmode.h,v 1.1 2007-03-01 00:36:54 ianc Exp $
*/
#ifndef DS81_TEXTMODE_H
#define DS81_TEXTMODE_H

/* Note that the co-ords are into the map -- the user is free to use this and
   move the map around, scale it, blend it, do want they want with it...

   The routines assume they can write into this map using the ASCII code 
   with 32 subtracted for each char.
*/
void TM_Init(uint16 *vram, int map_width, int map_height, int map_is_rotation);

void TM_Cls(void);
void TM_Put(int x, int y, const char *str);
void TM_printf(int x, int y, const char *format, ...);

#endif	/* DS81_TEXTMODE_H */
