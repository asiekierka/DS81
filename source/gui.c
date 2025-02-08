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
  
   $Id: gui.c,v 1.13 2008-12-12 00:19:53 ianc Exp $
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <nds.h>

#include <dirent.h>
#include <sys/stat.h>

#include "framebuffer.h"
#include "zx81.h"
#include "keyboard.h"
#include "config.h"


/* ---------------------------------------- PRIVATE INTERFACES - PATH HANDLING
*/
#define FSEL_FILENAME_LEN	MAXNAMLEN
#define FSEL_LINES		16
#define FSEL_MAX_FILES		1024

#define FSEL_LIST_Y		10
#define FSEL_LIST_H		FSEL_LINES*8

typedef struct
{
    char	name[FSEL_FILENAME_LEN+1];
    bool	is_dir;
} FSEL_File;

static FSEL_File fsel[FSEL_MAX_FILES];


static void CheckPath(char *path)
{
    size_t l;

    l = strlen(path);

    if (l == 1)
    {
    	path[0] = '/';
    }
    else
    {
    	if (path[l-1] != '/')
	{
	    path[l] = '/';
	    path[l+1] = 0;
	}
    }
}


static void AddPath(char *path, const char *dir)
{
    if (strcmp(dir,"..") == 0)
    {
    	size_t l;

	l = strlen(path);

	if (l > 1)
	{
	    path[--l] = 0;

	    while(l && path[l] != '/')
	    {
	    	path[l--] = 0;
	    }
	}
    }
    else
    {
	strcat(path,dir);
	strcat(path,"/");
    }
}




static int SortFiles(const void *a, const void *b)
{
    const FSEL_File *f1;
    const FSEL_File *f2;

    f1 = (const FSEL_File *)a;
    f2 = (const FSEL_File *)b;

    if (f1->is_dir == f2->is_dir)
    {
    	return strcasecmp(f1->name, f2->name);
    }
    else if (f1->is_dir)
    {
    	return -1;
    }
    else
    {
    	return 1;
    }
}


static int ValidFilename(const char *name, int is_dir, const char *filter)
{
    size_t l;
    size_t f;

    l = strlen(name);

    if (l > FSEL_FILENAME_LEN)
    	return 0;

    if (strcmp(name,".") == 0)
    	return 0;

    if (is_dir || !filter)
    	return 1;

    f = strlen(filter);

    if (l > f)
    {
    	if (strcasecmp(name+l-f,filter) == 0)
	{
	    return 1;
	}
    }

    return 0;
}


static int LoadDir(const char *path, const char *filter)
{
    DIR *dir;
    struct dirent *dt;
    int no = 0;

    if ((dir = opendir(path)))
    {
    	while(no < FSEL_MAX_FILES && (dt = readdir(dir)))
	{
	    if (ValidFilename(dt->d_name, (dt->d_type == DT_DIR), filter))
	    {
		strcpy(fsel[no].name,dt->d_name);
		fsel[no].is_dir = (dt->d_type == DT_DIR);
		no++;
	    }
	}

	closedir(dir);

	qsort(fsel,no,sizeof fsel[0],SortFiles);
    }

    return no;
}


/* ---------------------------------------- PUBLIC INTERFACES
*/
int GUI_Menu(const char *opts[])
{
    int x,y;
    int h;
    int w;
    int no;
    int sel;
    int f;
    int done;
    int defer;

    w=0;
    h=0;
    sel=0;
    done=FALSE;
    defer=FALSE;

    for(no=0;opts[no];no++)
    {
    	h+=16;

	if (strlen(opts[no])>w)
	{
	    w=strlen(opts[no]);
	}
    }

    w=w*8+16;

    x=SCREEN_WIDTH/2-w/2;
    y=2;

    while(!done)
    {
	uint32 key=0;

	FB_FillBox(x,y,w,h,COL_BLACK);
	FB_Box(x,y,w,h,COL_WHITE);
	FB_FillBox(x+1,y+sel*16+1,w-2,14,COL_GUISELECT);

    	for(f=0;f<no;f++)
	{
	    FB_Centre(opts[f],y+4+f*16,COL_WHITE,COL_TRANSPARENT);
	}

	do
	{
	    swiWaitForVBlank();
	} while(!defer && !(key=keysDownRepeat()));

	if (defer)
	{
	    do
	    {
	    	swiWaitForVBlank();
	    } while (keysHeld()&KEY_TOUCH);
	    done=TRUE;
	}
	else
	{
	    if (key & (KEY_A|KEY_B|KEY_X|KEY_Y))
	    {
		done=TRUE;
	    }
	    else if ((key & KEY_UP) && sel)
	    {
		sel--;
	    }
	    else if ((key & KEY_DOWN) && sel<no-1)
	    {
		sel++;
	    }
	    else if (key & KEY_TOUCH)
	    {
		touchPosition tp;
		touchRead(&tp);

		if (tp.px>=x && tp.px<(w+w) && tp.py>=y && tp.py<(y+h))
		{
		    defer=TRUE;
		    sel=(tp.py-y)/16;
		}
	    }
	}
    }

    return sel;
}


void GUI_Alert(int fatal, const char *text)
{
    char line[80];
    int h;
    const char *p;
    char *d;

    h=40;
    p=text;

    while(*p)
    {
    	if (*p++=='\n')
	{
	    h+=8;
	}
    }

    FB_FillBox(0,0,SCREEN_WIDTH,h,COL_BLACK);
    FB_Box(1,1,SCREEN_WIDTH-2,h-2,COL_WHITE);

    p=text;
    h=4;
    d=line;

    while(*p)
    {
    	if (*p=='\n')
	{
	    *d++=0;
	    p++;
	    FB_Centre(line,h,COL_WHITE,COL_TRANSPARENT);
	    h+=8;
	    d=line;
	}
	else
	{
	    *d++=*p++;
	}
    }

    if (d>line)
    {
	*d=0;
	FB_Centre(line,h,COL_WHITE,COL_TRANSPARENT);
	h+=8;
    }

    if (!fatal)
    {
	FB_Centre("PRESS ANY BUTTON OR SCREEN",h+16,COL_YELLOW,COL_TRANSPARENT);

	while(!keysDown())
	{
	    swiWaitForVBlank();
	}

	while(keysHeld())
	{
	    swiWaitForVBlank();
	}
    }
    else
    {
	FB_Centre("PLEASE RESET YOUR CONSOLE",h+16,COL_YELLOW,COL_TRANSPARENT);

	while(1)
	{
	    swiWaitForVBlank();
	}
    }
}


void GUI_Config(void)
{
    int sel;
    DS81_ConfigItem f;
    int done;
    int save;

    sel = 0;
    done = FALSE;
    save = FALSE;

    FB_Clear();

    FB_Centre("Up/Down to select",140,COL_YELLOW,COL_TRANSPARENT);
    FB_Centre("A to toggle",150,COL_YELLOW,COL_TRANSPARENT);
    FB_Centre("Or use touchscreen",160,COL_YELLOW,COL_TRANSPARENT);
    FB_Centre("START to finish",170,COL_YELLOW,COL_TRANSPARENT);

#ifndef DS81_DISABLE_FAT
    FB_Centre("SELECT to finish and save",180,COL_YELLOW,COL_TRANSPARENT);
#endif

    for(f=0;f<DS81_NUM_CONFIG_ITEMS;f++)
    {
	FB_Print(ConfigDesc(f),14,20+f*14,COL_WHITE,COL_TRANSPARENT);
    }

    while(!done)
    {
	uint32 key=0;

	for(f=0;f<DS81_NUM_CONFIG_ITEMS;f++)
	{
	    FB_FillBox(2,20+f*14-1,10,10,
			DS81_Config[f] ? COL_WHITE : COL_BLACK);

	    FB_Box(2,20+f*14-1,10,10,COL_GREY);
	}

	FB_Box(0,20+sel*14-3,SCREEN_WIDTH-1,14,COL_GUISELECT);

	do
	{
	    swiWaitForVBlank();
	} while(!(key=keysDownRepeat()));

	FB_Box(0,20+sel*14-3,SCREEN_WIDTH-1,14,COL_BLACK);

	if (key & KEY_START)
	{
	    done=TRUE;
	}
#ifndef DS81_DISABLE_FAT
	else if (key & KEY_SELECT)
	{
	    done=TRUE;
	    save=TRUE;
	}
#endif
	else if (key & KEY_A)
	{
	    DS81_Config[sel] = !DS81_Config[sel];
	}
	else if ((key & KEY_UP) && sel)
	{
	    sel--;
	}
	else if ((key & KEY_DOWN) && sel<DS81_NUM_CONFIG_ITEMS-1)
	{
	    sel++;
	}
	else if (key & KEY_TOUCH)
	{
	    touchPosition tp;
	    touchRead(&tp);
	    int nsel;

	    nsel = (tp.py-18)/14;

	    if (nsel>=0 && nsel<DS81_NUM_CONFIG_ITEMS)
	    {
	    	sel = nsel;
		DS81_Config[sel] = !DS81_Config[sel];
	    }
	}
    }

    if (save)
    {
	SaveConfig();
    }
}


int GUI_FileSelect(char pwd[], char selected_file[], const char *filter)
{
    int no;
    int sel;
    int top;
    int bar_size;
    double bar_step;
    int done;
    int ret;
    FB_Colour paper;
    int off;
    int f;
    int drag;
    int drag_start;

    CheckPath(pwd);

    FB_Clear();

    FB_printf(0,0,COL_BLACK,COL_LIGHTGREY,"%-32.32s",pwd);

    FB_Centre("Use pad and A to select",140,COL_YELLOW,COL_TRANSPARENT);
    FB_Centre("L and R to page up/down",150,COL_YELLOW,COL_TRANSPARENT);
    FB_Centre("Or use touchscreen",160,COL_YELLOW,COL_TRANSPARENT);
    FB_Centre("B to cancel",170,COL_YELLOW,COL_TRANSPARENT);

    no = LoadDir(pwd,filter);

    sel = 0;
    top = 0;
    done = FALSE;
    ret = FALSE;
    drag = FALSE;
    drag_start = 0;

    if (no<=FSEL_LINES)
    {
	bar_step = 0;
	bar_size = FSEL_LIST_H;
    }
    else
    {
	bar_step = FSEL_LIST_H/(double)no;
	bar_size = bar_step*FSEL_LINES;
    }

    while(!done)
    {
	uint32 key=0;

	for (f=0;f<FSEL_LINES;f++)
	{
	    off = f + top;

	    if (off<no)
	    {
		if (off == sel)
		{
		    paper = COL_GUISELECT;
		}
		else
		{
		    paper = COL_BLACK;
		}

		FB_printf(8,FSEL_LIST_Y+f*8,COL_WHITE,paper,
				"%-*s  %s",
				    FSEL_FILENAME_LEN,
				    fsel[off].name,
				    fsel[off].is_dir ? "DIR" : "   ");
	    }
	    else
	    {
		FB_printf(8,FSEL_LIST_Y+f*8,COL_WHITE,COL_BLACK,
			    "%-*s  %s",
			    	FSEL_FILENAME_LEN,
				off==0 ? "No Files!" : "",
				"   ");
	    }
	}

	FB_FillBox(240,FSEL_LIST_Y,16,FSEL_LIST_H,COL_DARKGREY);
	FB_FillBox(240,FSEL_LIST_Y+top*bar_step,16,bar_size,COL_WHITE);

	if (drag)
	{
	    touchPosition tp = {0};
	    int diff = 0;

	    while (((key=keysHeld()) & KEY_TOUCH) && diff == 0)
	    {
		touchRead(&tp);
		diff = tp.py - drag_start;
		swiWaitForVBlank();
	    }

	    if (key & KEY_TOUCH)
	    {
		int new_top;

		new_top = top + diff / bar_step;

		if (new_top > (no - FSEL_LINES))
		{
		    new_top = no - FSEL_LINES;
		}

		if (new_top < 0)
		{
		    new_top = 0;
		}

		if (new_top != top)
		{
		    top = new_top;
		    sel = top;
		    drag_start = tp.py;
		}
	    }
	    else
	    {
	    	drag = FALSE;
	    }
	}

	if (!drag)
	{
	    int activate = FALSE;

	    do
	    {
		swiWaitForVBlank();
	    } while(!(key=keysDownRepeat()));

	    if (key & KEY_TOUCH)
	    {
		touchPosition tp;
		touchRead(&tp);

	    	if (tp.py >= FSEL_LIST_Y && tp.py <= (FSEL_LIST_Y+FSEL_LIST_H))
		{
		    if (tp.px > 239)
		    {
			drag = TRUE;
			drag_start = tp.py;
		    }
		    else
		    {
			int new_sel;

		    	new_sel = top + (tp.py - FSEL_LIST_Y)/8;

			if (new_sel < no)
			{
			    if (new_sel == sel)
			    {
			    	activate = TRUE;
			    }
			    else
			    {
			    	sel = new_sel;
			    }
			}
		    }
		}
	    }
	    else if (key & KEY_UP)
	    {
		if (sel)
		{
		    sel--;

		    if (sel<top)
		    {
			top--;
		    }
		}
	    }
	    else if (key & KEY_DOWN)
	    {
		if (sel < (no-1))
		{
		    sel++;

		    if (sel >= (top+FSEL_LINES))
		    {
			top++;
		    }
		}
	    }
	    else if (key & KEY_L)
	    {
		if (sel)
		{
		    sel-=FSEL_LINES;

		    if (sel < 0)
		    {
			sel = 0;
		    }

		    top = sel;
		}
	    }
	    else if (key & KEY_R)
	    {
		if (sel < (no-1))
		{
		    sel+=FSEL_LINES;

		    if (sel > (no-1))
		    {
			sel = no-1;
		    }

		    top = sel - FSEL_LINES + 1;

		    if (top < 0)
		    {
			top = 0;
		    }
		}
	    }
	    else if (key & KEY_A)
	    {
	    	activate = TRUE;
	    }
	    else if (key & KEY_B)
	    {
		done = TRUE;
	    }

	    if (activate)
	    {
		if (fsel[sel].is_dir)
		{
		    AddPath(pwd,fsel[sel].name);

		    FB_printf(0,0,COL_BLACK,COL_LIGHTGREY,"%-32.32s",pwd);

		    no = LoadDir(pwd,filter);

		    sel = 0;
		    top = 0;

		    if (no<=FSEL_LINES)
		    {
			bar_step = 0;
			bar_size = FSEL_LIST_H;
		    }
		    else
		    {
			bar_step = FSEL_LIST_H/(double)no;
			bar_size = bar_step*FSEL_LINES;
		    }
		}
		else
		{
		    done = TRUE;
		    ret = TRUE;

		    strcpy(selected_file,pwd);
		    strcat(selected_file,fsel[sel].name);
		}
	    }
	}
    }

    while (keysHeld());

    return ret;
}


int GUI_InputName(const char *prompt, const char *ext, char name[], int maxlen)
{
    struct
    {
    	SoftKey	key;
	int	ascii;
    } keymap[] =
    	{
	    {SK_1, '1'},
	    {SK_2, '2'},
	    {SK_3, '3'},
	    {SK_4, '4'},
	    {SK_5, '5'},
	    {SK_6, '6'},
	    {SK_7, '7'},
	    {SK_8, '8'},
	    {SK_9, '9'},
	    {SK_0, '0'},
	    {SK_A, 'A'},
	    {SK_B, 'B'},
	    {SK_C, 'C'},
	    {SK_D, 'D'},
	    {SK_E, 'E'},
	    {SK_F, 'F'},
	    {SK_G, 'G'},
	    {SK_H, 'H'},
	    {SK_I, 'I'},
	    {SK_J, 'J'},
	    {SK_K, 'K'},
	    {SK_L, 'L'},
	    {SK_M, 'M'},
	    {SK_N, 'N'},
	    {SK_O, 'O'},
	    {SK_P, 'P'},
	    {SK_Q, 'Q'},
	    {SK_R, 'R'},
	    {SK_S, 'S'},
	    {SK_T, 'T'},
	    {SK_U, 'U'},
	    {SK_V, 'V'},
	    {SK_W, 'W'},
	    {SK_X, 'X'},
	    {SK_Y, 'Y'},
	    {SK_Z, 'Z'},
	    {0, 0}
	};

    SoftKeyEvent ev;
    char text[1024];
    int done = FALSE;
    int accept = FALSE;
    int update = TRUE;

    SK_DisplayKeyboard();
    ZX81SuspendDisplay();

    name[0] = 0;

    while(!done)
    {
	if (update)
	{
	    sprintf(text, "%s:\n"
			  "\"%s%%l%%%s\""
			  "\n\n\npress enter to accept.\n"
			  "press period to backspace.\n"
			  "press space/break to cancel.\n",
			  prompt, name, ext);

	    ZX81DisplayString(text);

	    update = FALSE;
	}

	while(SK_GetBareEvent(&ev))
	{
	    if (!ev.pressed)
	    {
		size_t l;
		int f;
		int ascii;

		l = strlen(name);

		switch(ev.key)
		{
		    case SK_PERIOD:
		    	if (l)
			{
			    name[--l] = 0;
			    update = TRUE;
			}
		    	break;

		    case SK_SPACE:
		    	done = TRUE;
			accept = FALSE;
			break;

		    case SK_NEWLINE:
		    	done = TRUE;
			accept = TRUE;
			break;

		    default:
			if (l < maxlen)
			{
			    f = 0;
			    ascii = 0;

			    while(!ascii && keymap[f].ascii)
			    {
				if (ev.key == keymap[f].key)
				{
				    ascii = keymap[f].ascii;
				}

				f++;
			    }

			    if (ascii)
			    {
				name[l++] = ascii;
				name[l] = 0;
				update = TRUE;
			    }
		    }
		    break;
		}
	    }
	}

	swiWaitForVBlank();
    }

    ZX81ResumeDisplay();

    return accept;
}

