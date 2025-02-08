/*
    ds81 - Nintendo DS ZX81 emulator

    Copyright (C) 2006  Ian Cowburn <ianc@noddybox.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    -------------------------------------------------------------------------

    Provides the routines for snapshotting.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <nds.h>

#include "snapshot.h"
#include "zx81.h"
#include "gui.h"

#include "config.h"

#include "ds81_debug.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


/* ---------------------------------------- STATICS
*/
static int		enabled;
static const char 	*magic = "V01_DS81";
static const char	*extension[2] = {".D81", ".K81"};


/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static void WriteMagic(FILE *fp, SnapshotType t)
{
    const char *p = magic;

    while(*p)
    {
    	fputc(*p++, fp);
    }

    fputc(t, fp);
}

static int CheckMagic(FILE *fp, SnapshotType t)
{
    const char *p = magic;

    while(*p)
    {
	if (fgetc(fp) != *p++)
	{
	    return FALSE;
	}
    }

    return (fgetc(fp) == t);
}


/* ---------------------------------------- EXPORTED INTERFACES
*/
void SNAP_Enable(int enable)
{
    enabled = enable;
}

void SNAP_Save(Z80 *cpu, SnapshotType type)
{
    char base[FILENAME_MAX] = "";
    char file[FILENAME_MAX];
    FILE *fp = NULL;

    if (!enabled)
    {
    	return;
    }

    if(!GUI_InputName("enter snapshot filename",
    			extension[type], base, 8) || !base[0])
    {
    	return;
    }

    strcat(base, extension[type]);

    strcpy(file, DEFAULT_SNAPDIR);
    strcat(file, base);

    fp = fopen(file, "wb");

    if (!fp)
    {
	fp = fopen(base, "wb");
    }

    if (fp)
    {
	WriteMagic(fp, type);

	SK_SaveSnapshot(fp);

	if (type == SNAP_TYPE_FULL)
	{
	    Z80SaveSnapshot(cpu, fp);
	    ZX81SaveSnapshot(fp);
	}

    	fclose(fp);
    }
    else
    {
	GUI_Alert(FALSE, "Failed to save snapshot");
    }
}

void SNAP_Load(Z80 *cpu, const char *optional_name, SnapshotType type)
{
    static char last_dir[FILENAME_MAX] = "/";
    char file[FILENAME_MAX];
    FILE *fp = NULL;

    if (!enabled)
    {
    	return;
    }

    if (optional_name)
    {
	strcpy(file, DEFAULT_SNAPDIR);
    	strcat(file, optional_name);
    	strcat(file, extension[type]);

	fp = fopen(file, "rb");

	if (!fp)
	{
	    strcpy(file, optional_name);
	    strcat(file, extension[type]);

	    fp = fopen(file, "rb");
	}
    }
    else
    {
    	if (GUI_FileSelect(last_dir, file, extension[type]))
	{
	    fp = fopen(file, "rb");
	}
    }

    if (fp)
    {
	if (!CheckMagic(fp, type))
	{
	    GUI_Alert(FALSE, "Not a valid snapshot");
	}
	else
	{
	    SK_LoadSnapshot(fp);

	    if (type == SNAP_TYPE_FULL)
	    {
		Z80LoadSnapshot(cpu, fp);
		ZX81LoadSnapshot(fp);
	    }
	}

    	fclose(fp);
    }
}
