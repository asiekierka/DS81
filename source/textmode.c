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
  
   $Id: textmode.c,v 1.2 2007-03-02 23:18:38 ianc Exp $
*/

#include <nds.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "textmode.h"

/* ---------------------------------------- STATIC DATA
*/
static int	mapw;
static int	maph; 
static uint16	*text;
static int	is_rot;
static void	(*draw_string)(const char *str, int x, int y);


/* ---------------------------------------- PRIVATE INTERFACES
*/
static inline void Plot_Text(int x, int y, int c)
{
    int xw;
    int yw;

    xw = x/32;
    yw = y/32;
    x %= 32;
    y %= 32;

    *(text + x + y*32 + (xw+yw) * 1024) = c;
}


static inline void Plot_RS(int x, int y, int c)
{
    uint16 ch;
    int odd;
    uint16 *off;

    odd = x&1;

    off = text + x/2 + y*mapw/2;

    ch = *off;

    if (odd)
    {
    	ch = (c<<8) | (ch&0xff);
    }
    else
    {
    	ch = c | (ch&0xff00);
    }

    *off = ch;
}


static void Text_Put(const char *str, int x, int y)
{
    while(*str && x<mapw)
    {
    	Plot_Text(x,y,*str - 32);
	x++;
	str++;
    }
}

static void RS_Put(const char *str, int x, int y)
{
    while(*str && x<mapw)
    {
    	Plot_RS(x,y,*str - 32);
	x++;
	str++;
    }
}

/* ---------------------------------------- PUBLIC INTERFACES
*/
void TM_Init(uint16 *vram, int map_width, int map_height, int map_is_rotation)
{
    text = vram;

    mapw = map_width;
    maph = map_height;

    is_rot = map_is_rotation;

    draw_string = map_is_rotation ? RS_Put : Text_Put;

    TM_Cls();
}


void TM_Cls(void)
{
    uint16 *scr;
    int f;

    scr = text;

    if (is_rot)
    {
	for(f=0;f<mapw*maph/2;f++)
	{
	    *scr++=0;
	}
    }
    else
    {
	for(f=0;f<mapw*maph;f++)
	{
	    *scr++=0;
	}
    }
}


void TM_Put(int x, int y, const char *str)
{
    draw_string(str,x,y);
}


void TM_printf(int x, int y, const char *format, ...)
{
    char buff[128];
    va_list va;

    va_start(va,format);
    vsnprintf(buff,sizeof buff,format,va);
    va_end(va);

    draw_string(buff,x,y);
}
