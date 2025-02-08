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
  
   $Id: config.c,v 1.4 2008-12-12 00:19:07 ianc Exp $
*/

#include <nds.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

/* ---------------------------------------- PRIVATE DATA
*/
const char *conf_filename = "DS81.CFG";

const char *conf_entry[DS81_NUM_CONFIG_ITEMS]=
{
    "sticky_shift",
    "average_touchscreen",
    "static_ram_at_0x2000",
    "allow_tape_save",
    "load_default_snapshot"
};


/* ---------------------------------------- GLOBAL DATA
*/
int	DS81_Config[DS81_NUM_CONFIG_ITEMS]=
{
    TRUE,
    FALSE,
    FALSE,
    FALSE,
    FALSE
};


/* ---------------------------------------- PUBLIC INTERFACES
*/
int LoadConfig(void)
{
    FILE *fp = NULL;

#ifndef DS81_DISABLE_FAT
    fp=fopen(conf_filename,"r");
#endif

    if (fp)
    {
	char line[80];

	while(fgets(line, sizeof line, fp))
	{
	    char *p;

	    if ((p = strchr(line, '=')))
	    {
		int f;

		for(f=0;f<DS81_NUM_CONFIG_ITEMS;f++)
		{
		    if (strncmp(line, conf_entry[f],
		    		strlen(conf_entry[f])) == 0)
		    {
		    	DS81_Config[f] = (*(p+1) == '1');
		    }
		}
		
	    }
	}

	fclose(fp);

	return TRUE;
    }

    return FALSE;
}

int SaveConfig(void)
{
    FILE *fp = NULL;

#ifndef DS81_DISABLE_FAT
    fp=fopen(conf_filename,"w");
#endif

    if (fp)
    {
	int f;

	for(f=0;f<DS81_NUM_CONFIG_ITEMS;f++)
	{
	    fprintf(fp,"%s=%d\n",conf_entry[f],DS81_Config[f]);
	}

    	fclose(fp);

	return TRUE;
    }

    return FALSE;
}

const char *ConfigDesc(DS81_ConfigItem item)
{
    switch(item)
    {
	case DS81_STICKY_SHIFT:
	    return "STICKY SHIFT";

	case DS81_AVERAGE_TOUCHSCREEN:
	    return "AVERAGE TOUCHSCREEN";

	case DS81_STATIC_RAM_AT_0x2000:
	    return "RAM AT 8192";

	case DS81_LOAD_DEFAULT_SNAPSHOT:
	    return "LOAD DEFAULT SNAPSHOT";

	case DS81_ALLOW_TAPE_SAVE:
	    return "ALLOW TAPE SAVING";

    	default:
	    return "UNKNOWN";
    }
}

