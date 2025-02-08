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
  
   $Id: touchwrap.c,v 1.1 2007-02-16 01:10:41 ianc Exp $
*/

#include <nds.h>
#include <stdlib.h>

#include "touchwrap.h"
#include "config.h"

/* ---------------------------------------- PUBLIC INTERFACES
*/
int AllowTouch(touchPosition *tp)
{
    static touchPosition last;
    int16 dx;
    int16 dy;
    int res;

    touchRead(tp);

    if (DS81_Config[DS81_AVERAGE_TOUCHSCREEN])
    {
	dx = last.px - tp->px;
	dy = last.py - tp->py;

	res = (abs(dx) < 5 && abs(dy) < 5);
    }
    else
    {
    	res = TRUE;
    }

    last = *tp;

    return res;
}
