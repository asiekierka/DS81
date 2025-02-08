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
  
   $Id: ds81_debug.h,v 1.2 2008-11-03 17:07:31 ianc Exp $
*/
#ifndef DS81_DEBUG_H
#define DS81_DEBUG_H

#include "gui.h"
#include "framebuffer.h"

#define DS81_DEBUG(fmt, args...)					\
    do									\
    {									\
	char tempdebug[512];						\
	sprintf(tempdebug, fmt, ## args);				\
	GUI_Alert(FALSE, tempdebug);					\
    } while(0)

#define DS81_DEBUG_STATUS(fmt, args...)					\
    do									\
    {									\
	FB_FillBox(0,184,256,8,COL_DARKGREY);				\
	FB_printf(0,184,COL_WHITE,COL_DARKGREY, fmt , ## args);		\
    } while(0)

#endif	/* DS81_DEBUG_H */
