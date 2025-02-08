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
  
   $Id: touchwrap.h,v 1.1 2007-02-16 01:10:41 ianc Exp $
*/
#ifndef DS81_TOUCHWRAP_H
#define DS81_TOUCHWRAP_H

/* Don't know whether I have a problem with my DS or the library, but sometimes
   the touchscreen value is off (one co-ord generally completely out).

   To alleviate this, and as this is a simple touch screen keyboard, allow
   touchs to be averaged if the config says so.  And averaged touch just means
   that two touchs have to happen within 5 pixels on X and Y before being
   allowed.

   If not configured to average, this simply reads the touchscreen position
   and returns true.
*/
int		AllowTouch(touchPosition *tp);

#endif	/* DS81_TOUCHWRAP_H */
