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
  
   $Id: snapshot.h,v 1.2 2008-12-12 00:19:07 ianc Exp $
*/
#ifndef DS81_SNAPSHOT_H
#define DS81_SNAPSHOT_H

#include "z80.h"

typedef enum
{
    SNAP_TYPE_FULL,
    SNAP_TYPE_KEYBOARD
} SnapshotType;

void	SNAP_Enable(int enable);
void	SNAP_Save(Z80 *cpu, SnapshotType type);
void	SNAP_Load(Z80 *cpu, const char *optional_name, SnapshotType type);

#endif	/* DS81_SNAPSHOT_H */
