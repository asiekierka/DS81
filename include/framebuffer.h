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
  
   $Id: framebuffer.h,v 1.6 2007-03-12 00:59:51 ianc Exp $
*/
#ifndef DS81_FRAMEBUFFER_H
#define DS81_FRAMEBUFFER_H

/* Predefined colours.
*/
typedef enum 
{
    COL_TRANSPARENT	= -1,
    COL_BLACK		= 0,
    COL_WHITE		= 240,
    COL_RED		= 241,
    COL_GREEN		= 242,
    COL_BLUE		= 243,
    COL_GUISELECT	= 244,
    COL_GREY		= 245,
    COL_LIGHTGREY	= 246,
    COL_DARKGREY	= 247,
    COL_YELLOW		= 248
} FB_Colour;


/* Initialise 'framebuffer' code.  vram is where the 8-bit framebuffer is.
   palette is the palette to use/set.
*/
void	FB_Init(uint16 *vram, uint16 *palette);

/* Gives access to the parameters of the frame buffer.
*/
uint16	*FB_VRAM(void);
uint16	*FB_PALETTE(void);

/* Load the internal framebuffer font as a set of ASCII tiles (starting with
   space) at tiles.  The tiles will use colour COL_WHITE.
*/
void	FB_LoadASCIITiles(uint16 *tiles);

/* Print the text into the framebuffer. 
*/
void	FB_Print(const char *text, int x, int y,
		 FB_Colour colour, FB_Colour paper);
void	FB_Centre(const char *text, int y, 
		  FB_Colour colour, FB_Colour paper);
void	FB_printf(int x, int y, FB_Colour colour, FB_Colour paper,
		  const char *format, ...);

/* Lines and boxes.
*/
void	FB_HLine(int x1, int x2, int y, FB_Colour colour);
void	FB_VLine(int x, int y1, int y2, FB_Colour colour);
void	FB_Box(int x, int y, int w, int h, FB_Colour colour);
void	FB_FillBox(int x, int y, int w, int h, FB_Colour colour);

/* Clear to background
*/
void	FB_Clear(void);

/* Draw the image.  The image must be an 8-bit image, but with only the first
   16 palette entries used.  Just to complicate matters!

   The image is assumed to be an even number of pixels wide.  Also the passed
   X co-ord will be forced even.

   offset is used to give an offset into the palette to place colours from the
   image.  Palette entries 1 - 128 will always be safe to use (these routines
   will never use them).
*/
void	FB_Blit(sImage *img, int x, int y, int offset);

#endif	/* DS81_FRAMEBUFFER_H */
