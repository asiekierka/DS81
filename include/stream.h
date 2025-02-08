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
  
   $Id: stream.h,v 1.2 2008-12-05 00:37:25 ianc Exp $
*/
#ifndef DS81_STREAM_H
#define DS81_STREAM_H

#include <stdio.h>

void		PUT_Byte(FILE *fp, unsigned char c);
void		PUT_Long(FILE *fp, long l);
void		PUT_ULong(FILE *fp, unsigned long l);

unsigned char	GET_Byte(FILE *fp);
long		GET_Long(FILE *fp);
unsigned long	GET_ULong(FILE *fp);

#endif	/* DS81_STREAM_H */
