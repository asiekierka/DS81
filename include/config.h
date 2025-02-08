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
  
   $Id: config.h,v 1.4 2008-12-12 00:19:07 ianc Exp $
*/
#ifndef DS81_CONFIG_H
#define DS81_CONFIG_H

/* Default snapshot dir
*/
#define DEFAULT_SNAPDIR	"/ZX81SNAP/"

typedef enum
{
    DS81_STICKY_SHIFT,
    DS81_AVERAGE_TOUCHSCREEN,
    DS81_STATIC_RAM_AT_0x2000,
    DS81_ALLOW_TAPE_SAVE,
    DS81_LOAD_DEFAULT_SNAPSHOT,
    DS81_NUM_CONFIG_ITEMS
} DS81_ConfigItem;

/* Returns TRUE if config loaded from FAT device
*/
int		LoadConfig(void);

/* Returns TRUE if config saved to FAT device
*/
int		SaveConfig(void);

/* Gets a description for a config item.
*/
const char	*ConfigDesc(DS81_ConfigItem item);

/* Table of configs.  Done like this for simple performance reasons.
*/
extern int	DS81_Config[/*DS81_ConfigItem item*/];

#endif	/* DS81_CONFIG_H */
