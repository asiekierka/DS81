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

    Provides the emulation for the ZX81

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <nds.h>

#include "zx81.h"
#include "gui.h"

#include "stream.h"

#include "config.h"

#include "zx81_bin.h"

#include "ds81_debug.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* ---------------------------------------- STATICS
*/
#define ROMLEN		0x2000
#define ROM_SAVE	0x2fc
#define ROM_LOAD	0x347

#define ED_SAVE		0xf0
#define ED_LOAD		0xf1
#define ED_WAITKEY	0xf2
#define ED_ENDWAITKEY	0xf3
#define ED_PAUSE	0xf4

#define SLOW_TSTATES	16000
#define FAST_TSTATES	64000

#define E_LINE		16404
#define LASTK1		16421
#define LASTK2		16422
#define MARGIN		16424
#define FRAMES		16436
#define CDFLAG		16443

static Z80Val		FRAME_TSTATES=FAST_TSTATES;

/* The ZX81 screen and memory
*/
static void		(*DrawScreen)(Z80 *z80);

static int		waitkey=FALSE;
static int		started=FALSE;

static int		hires=FALSE;
static int		hires_dfile;
static int		last_I;

static unsigned		prev_lk1;
static unsigned		prev_lk2;

#define	SCR_W		256
#define	SCR_H		192
#define	TXT_W		32
#define	TXT_H		24

static Z80Byte		mem[0x10000];

static Z80Byte		scr_mirror[7000];

static Z80Word		RAMBOT=0;
static Z80Word		RAMTOP=0;

#define DFILE		0x400c

#define WORD(a)		(mem[a] | (Z80Word)mem[a+1]<<8)

/* Tape
*/
static int		enable_filesystem;
static int		allow_save;
static const Z80Byte	*tape_image;
static int		tape_len;

static char		last_dir[FILENAME_MAX] = "/";

/* GFX vars
*/
static uint16		*txt_screen;
static uint16		*bmp_screen;

/* The keyboard
*/
static Z80Byte		matrix[8];

static struct
{
    int row;
    int bit;
} key_matrix[]=
    {
    	{3,0x01}, {3,0x02}, {3,0x04}, {3,0x08}, {3,0x10}, 	/* 1 - 5 */
    	{4,0x10}, {4,0x08}, {4,0x04}, {4,0x02}, {4,0x01}, 	/* 6 - 0 */
    	{2,0x01}, {2,0x02}, {2,0x04}, {2,0x08}, {2,0x10}, 	/* Q - T */
    	{5,0x10}, {5,0x08}, {5,0x04}, {5,0x02}, {5,0x01}, 	/* Y - P */
    	{1,0x01}, {1,0x02}, {1,0x04}, {1,0x08}, {1,0x10}, 	/* A - G */
    	{6,0x10}, {6,0x08}, {6,0x04}, {6,0x02}, {6,0x01}, 	/* H - NL */
    	{0,0x01}, {0,0x02}, {0,0x04}, {0,0x08}, {0,0x10}, 	/* CAPS - V */
    	{7,0x10}, {7,0x08}, {7,0x04}, {7,0x02}, {7,0x01} 	/* B - SPACE */
    };


/* ---------------------------------------- PRIVATE FUNCTIONS
*/
#define PEEKW(addr)		(mem[addr] | (Z80Word)mem[addr+1]<<8)

#define POKEW(addr,val)         do					\
                                {					\
                                    Z80Word wa=addr;			\
                                    Z80Word wv=val;			\
                                    mem[wa]=wv;				\
                                    mem[wa+1]=wv>>8;			\
                                } while(0)

static void RomPatch(void)
{
    static const Z80Byte save[]=
    {
    	0xed, ED_SAVE,		/* (SAVE)		*/
	0xc3, 0x07, 0x02,	/* JP $0207		*/
	0xff			/* End of patch		*/
    };

    static const Z80Byte load[]=
    {
    	0xed, ED_LOAD,		/* (LOAD)		*/
	0xc3, 0x07, 0x02,	/* JP $0207		*/
	0xff			/* End of patch		*/
    };

    static const Z80Byte fast_hack[]=
    {
	0xed, ED_WAITKEY,	/* (START KEY WAIT)	*/
	0xcb,0x46,		/* L: bit 0,(hl)	*/
	0x28,0xfc,		/* jr z,L		*/
	0xed, ED_ENDWAITKEY,	/* (END KEY WAIT)	*/
    	0x00,			/* nop			*/
	0xff			/* End of patch		*/
    };

    static const Z80Byte kbd_hack[]=
    {
	0x2a,0x25,0x40,		/* ld hl,(LASTK)	*/
	0xc9,			/* ret			*/
	0xff			/* End of patch		*/
    };

    static const Z80Byte pause_hack[]=
    {
	0xed, ED_PAUSE,		/* (PAUSE)		*/
    	0x00,			/* nop			*/
	0xff			/* End of patch		*/
    };

    int f;

    for(f=0;save[f]!=0xff;f++)
    {
	mem[ROM_SAVE+f]=save[f];
    }

    for(f=0;load[f]!=0xff;f++)
    {
	mem[ROM_LOAD+f]=load[f];
    }

    for(f=0;fast_hack[f]!=0xff;f++)
    {
	mem[0x4ca+f]=fast_hack[f];
    }

    for(f=0;kbd_hack[f]!=0xff;f++)
    {
	mem[0x2bb+f]=kbd_hack[f];
    }

    for(f=0;pause_hack[f]!=0xff;f++)
    {
	mem[0xf3a+f]=pause_hack[f];
    }

    /* Trust me, we have a ZX81... Honestly.
    */
    mem[0x21c]=0x00;
    mem[0x21d]=0x00;

    /* Remove HALTs as we don't do interrupts
    */
    mem[0x0079]=0;
    mem[0x02ec]=0;
}

static Z80Byte FromASCII(char c)
{
    static const char *charset =
      /* 0123456789 */
    	" _________"
	"_\"#$:?()><"
	"=+-*/:,.01"
	"23456789ab"
	"cdefghijkl"
	"mnopqrstuv"
	"wxyz";

    int f;

    c = tolower(c);

    for(f = 0; charset[f]; f++)
    {
    	if (charset[f] == c)
	{
	    return f;
	}
    }

    return 0;
}


/* Open a tape file the passed address
*/
static FILE *OpenTapeFile(Z80Word addr, int *cancelled, const char *mode)
{
    static const char zx_chars[] = "\"#$:?()><=+-*/;,."
				   "0123456789"
				   "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    FILE *fp;
    char full_fn[FILENAME_MAX] = DEFAULT_SNAPDIR;
    char fn[FILENAME_MAX];
    int f;
    int done;

    fp = NULL;
    f = 0;
    done = FALSE;
    *cancelled = FALSE;

    while(f<(FILENAME_MAX-3) && !done)
    {
    	int ch;

	ch = mem[addr++];

	if (ch&0x80)
	{
	    done = TRUE;
	    ch &= 0x7f;
	}

	if (ch>=11 && ch<=63)
	{
	    fn[f++] = zx_chars[ch-11];
	}
    }

    if (fn[0] == '*')
    {
    	if (GUI_FileSelect(last_dir,fn,".P"))
	{
	    fp = fopen(fn, mode);
	}
	else
	{
	    *cancelled = TRUE;
	}

	SK_DisplayKeyboard();
    }
    else
    {
	fn[f++] = '.';
	fn[f++] = 'P';
	fn[f] = 0;

	strcat(full_fn,fn);

	if (!(fp = fopen(full_fn, mode)))
	{
	    fp = fopen(fn, mode);
	}
    }

    return fp;
}


static void LoadInternalTape(Z80 *z80)
{
    memcpy(mem+0x4009,tape_image,tape_len);
}


static void LoadExternalTape(FILE *tape, Z80 *z80)
{
    int c;
    Z80Byte *a;

    a=mem+0x4009;

    while((c=getc(tape))!=EOF)
    {
    	*a++=c;
    }
}


static void SaveExternalTape(FILE *tape, Z80 *z80)
{
    int f;
    int end;

    f = 0x4009;
    end = WORD(E_LINE);

    while(f <= end)
    {
    	fputc(mem[f++], tape);
    }
}


static void ClearBitmap(void)
{
    uint16 *s;
    uint16 p;
    int f;

    s = bmp_screen;
    p = 0x8000|RGB15(31,31,31);

    for(f=0;f<SCREEN_WIDTH*SCREEN_HEIGHT;f++)
    {
    	*s++=p;
    }
}


static void ClearText(void)
{
    uint16 *s;
    int f;

    s = txt_screen;

    for(f=0;f<TXT_W*TXT_H;f++)
    {
    	*s++=0;
    }
}


static void DrawScreen_HIRES_Dirty(Z80 *z80)
{
    uint16 *bmp;
    Z80Byte *scr;
    Z80Byte *mirror;
    int x,y;
    int table;
    int c;
    int v;
    int b;

    scr = mem + hires_dfile;
    mirror = scr_mirror;
    bmp = bmp_screen;
    table = z80->I << 8;

    /* scr is increment in both loops so that it can skip the end-of-line
       character
    */
    for(y=0; y<192; y++)
    {
    	for(x=0; x<32; x++)
	{
	    c = *scr;

	    if (c != *mirror)
	    {
	    	*mirror++ = c;

		v = mem[table + (c&0x3f)*8];

		if (c & 0x80)
		{
		    v ^= 0xff;
		}

		for(b=0;b<8;b++)
		{
		    if (v & 0x80)
		    {
			*bmp++ = 0x8000;
		    }
		    else
		    {
			*bmp++ = 0xffff;
		    }

		    v=v<<1;
		}
	    }
	    else
	    {
	    	mirror++;
		bmp+=8;
	    }

	    scr++;
	}

	scr++;
    }
}


static void DrawScreen_HIRES_Full(Z80 *z80)
{
    uint16 *bmp;
    Z80Byte *scr;
    Z80Byte *mirror;
    int x,y;
    int table;

    scr = mem + hires_dfile;
    mirror = scr_mirror;
    bmp = bmp_screen;
    table = z80->I << 8;

    /* scr is increment in both loops so that it can skip the end-of-line
       character
    */
    for(y=0; y<192; y++)
    {
    	for(x=0; x<32; x++)
	{
	    int c;
	    int v;
	    int b;

	    c = *mirror++ = *scr;

	    v = mem[table + (c&0x3f)*8];

	    if (c & 0x80)
	    {
	    	v ^= 0xff;
	    }

	    for(b=0;b<8;b++)
	    {
	    	if (v & 0x80)
		{
		    *bmp++ = 0x8000;
		}
		else
		{
		    *bmp++ = 0xffff;
		}

		v=v<<1;
	    }

	    scr++;
	}

	scr++;
    }

    DrawScreen = DrawScreen_HIRES_Dirty;
}


static void DrawScreen_TEXT(Z80 *z80)
{
    Z80Byte *scr=mem+WORD(DFILE);
    int x,y;

    x=0;
    y=0;

    while(y<TXT_H)
    {
	scr++;
	x=0;

	while((*scr!=118)&&(x<TXT_W))
	{
	    Z80Byte ch = *scr++;

	    if (ch&0x80)
	    {
	    	txt_screen[x+y*32]=(ch&0x3f)|0x40;
	    }
	    else
	    {
	    	txt_screen[x+y*32]=(ch&0x3f);
	    }

	    x++;
	}

	while (x<TXT_W)
	{
	    txt_screen[x+y*32]=0;
	    x++;
	}

	y++;
    }
}


static void DrawSnow(Z80 *z80)
{
    uint16 *s;
    int f;

    s = txt_screen;

    for(f=0;f<TXT_W*TXT_H;f++)
    {
    	*s++=8;
    }
}


static void FindHiresDFILE(void)
{
    /* Somewhat based on the code from xz81, an X-based ZX81 emulator,
       (C) 1994 Ian Collier.  Search the ZX81's RAM until we find what looks
       like a hi-res display file.

       Bizarrely the original code used 'f' for a loop counter too...  Another
       poor soul forever damaged by the ZX81's keyword entry system...
    */
    int f;

    for(f=0x8000-(33*192); f>0x4000 ; f--)
    {
	int v;

	v = mem[f+32];

	if (v&0x40)
	{
	    int ok = TRUE;
	    int n;

	    for(n=0;n<192 && ok;n++)
	    {
	    	if (mem[f+33*n]&0x40)
		{
		    ok = FALSE;
		}

		if (mem[f+32+33*n] != v)
		{
		    ok = FALSE;
		}
	    }

	    if (ok)
	    {
	    	hires_dfile = f;
		return;
	    }
	}
    }

    /* All else fails, put the hires dfile at 0x4000 -- at least it should be
       obvious that the hires won't work for whatever is being run.
    */
    hires_dfile = 0x4000;
}


/* Perform ZX81 housekeeping functions like updating FRAMES and updating LASTK
*/
static void ZX81HouseKeeping(Z80 *z80)
{
    unsigned row;
    unsigned lastk1;
    unsigned lastk2;

    /* British ZX81
    */
    mem[MARGIN]=55;

    /* Update FRAMES
    */
    if (FRAME_TSTATES==SLOW_TSTATES)
    {
    	Z80Word frame=PEEKW(FRAMES)&0x7fff;

	if (frame)
	{
	    frame--;
	}

	POKEW(FRAMES,frame|0x8000);
    }

    if (!started)
    {
    	prev_lk1=0;
    	prev_lk2=0;
	return;
    }

    /* Update LASTK
    */
    lastk1=0;
    lastk2=0;

    for(row=0;row<8;row++)
    {
    	unsigned b;

	b=(~matrix[row]&0x1f)<<1;

	if (row==0)
	{
	    unsigned shift;

	    shift=b&2;
	    b&=~2;
	    b|=(shift>>1);
	}

	if (b)
	{
	    if (b>1)
	    {
		lastk1|=(1<<row);
	    }

	    lastk2|=b;
	}
    }

    if (lastk1 && (lastk1!=prev_lk1 || lastk2!=prev_lk2))
    {
    	mem[CDFLAG]|=1;
    }
    else
    {
    	mem[CDFLAG]&=~1;
    }

    mem[LASTK1]=lastk1^0xff;
    mem[LASTK2]=lastk2^0xff;

    prev_lk1=lastk1;
    prev_lk2=lastk2;
}


static int CheckTimers(Z80 *z80, Z80Val val)
{
    if (val>=FRAME_TSTATES)
    {
    	/* Check for hi-res modes
	*/
	if (z80->I && z80->I != last_I)
	{
	    last_I = z80->I;

	    if (z80->I == 0x1e)
	    {
	    	hires = FALSE;
		DrawScreen = DrawScreen_TEXT;
		ClearBitmap();
	    }
	    else
	    {
	    	hires = TRUE;
		DrawScreen = DrawScreen_HIRES_Full;
		ClearBitmap();
		ClearText();
		FindHiresDFILE();
	    }
	}

	Z80ResetCycles(z80,val-FRAME_TSTATES);

	/* Kludge warning - We assume that a hires display will not be in
	   FAST mode! 
	*/
	if (started && ((mem[CDFLAG] & 0x80) || waitkey || hires))
	{
	    DrawScreen(z80);
	    FRAME_TSTATES=SLOW_TSTATES;
	}
	else
	{
	    DrawSnow(z80);
	    FRAME_TSTATES=FAST_TSTATES;
	}

	/* Update FRAMES (if in SLOW) and scan the keyboard.  This only happens
	   once we've got to a decent point in the boot cycle (detected with
	   a valid stack pointer).
	*/
	if (z80->SP<0x8000)
	{
	    ZX81HouseKeeping(z80);
	}

	swiWaitForVBlank();

	return FALSE;
    }
    else
    {
	return TRUE;
    }
}


static int EDCallback(Z80 *z80, Z80Val data)
{
    Z80Word pause;

    switch((Z80Byte)data)
    {
    	case ED_SAVE:
	    if (allow_save && z80->DE.w<0x8000)
	    {
		FILE *fp;
		int cancel;

		if ((fp=OpenTapeFile(z80->HL.w, &cancel, "wb")))
		{
		    SaveExternalTape(fp,z80);
		    fclose(fp);
		}
	    }
	    break;

    	case ED_LOAD:
	    /* Try and load the external file if a name given.  Otherwise, we
	       try the internal one.  Some of this is slightly dodgy -- it was
	       never intended for the emulator to be doing any GUI related
	       nonsense (like the alerts) but simply emulating.
	    */
	    if (enable_filesystem && z80->DE.w<0x8000)
	    {
		FILE *fp;
		int cancel;

		if ((fp=OpenTapeFile(z80->DE.w, &cancel, "rb")))
		{
		    LoadExternalTape(fp,z80);
		    fclose(fp);
		}
		else
		{
		    if (!cancel)
		    {
			GUI_Alert(FALSE,"Couldn't open tape");
			SK_DisplayKeyboard();
		    }
		}
	    }
	    else
	    {
		if (tape_image)
		{
		    LoadInternalTape(z80);
		}
		else
		{
		    GUI_Alert(FALSE,"No tape image selected");
		    SK_DisplayKeyboard();
		}
	    }

	    mem[CDFLAG]=0xc0;
	    break;

	case ED_WAITKEY:
	    waitkey=TRUE;
	    started=TRUE;
	    break;

	case ED_ENDWAITKEY:
	    waitkey=FALSE;
	    break;

	case ED_PAUSE:
	    waitkey=TRUE;

	    pause=z80->BC.w;

	    while(pause-- && !(mem[CDFLAG]&1))
	    {
		SoftKeyEvent ev;

		while (SK_GetEvent(&ev))
		{
		    ZX81HandleKey(ev.key,ev.pressed);
		}

	    	CheckTimers(z80,FRAME_TSTATES);
	    }

	    waitkey=FALSE;
	    break;

	default:
	    break;
    }

    return TRUE;
}


/* ---------------------------------------- EXPORTED INTERFACES
*/
void ZX81Init(uint16 *text_vram, uint16* bitmap_vram, Z80 *z80)
{
    Z80Word f;

    txt_screen = text_vram;
    bmp_screen = bitmap_vram;

    hires = FALSE;
    hires_dfile = 0;
    last_I = 0x1e;
    DrawScreen = DrawScreen_TEXT;

    ClearBitmap();

    /* Load the ROM
    */
    memcpy(mem,zx81_bin,ROMLEN);

    /* Patch the ROM
    */
    RomPatch();
    Z80LodgeCallback(z80,eZ80_EDHook,EDCallback);
    Z80LodgeCallback(z80,eZ80_Instruction,CheckTimers);

    /* Mirror the ROM
    */
    memcpy(mem+ROMLEN,mem,ROMLEN);

    /* Memory size (16K)
    */
    RAMBOT=0x4000;
    RAMTOP=RAMBOT+0x4000;

    for(f = RAMBOT; f <= RAMTOP; f++)
    {
    	mem[f] = 0;
    }

    for(f = 0; f < 8; f++)
    {
    	matrix[f] = 0x1f;
    }

    /* Fill the upper 32K with RET opcodes -- hopefully by simply returning
       RET for ULA reads we save a lot of ROM patching shenanigans.
       
       Note that this check used to be in ZX81ReadMem, but obviously this 
       should cut down on a *lot* of pointless expression evaluation!
    */
    for(f = 0x8000; f <= RAMTOP; f++)
    {
    	mem[f] = 0xc9;
    }

}


void ZX81HandleKey(SoftKey key, int is_pressed)
{
    if (key<SK_CONFIG)
    {
	if (is_pressed)
	{
	    matrix[key_matrix[key].row]&=~key_matrix[key].bit;
	}
	else
	{
	    matrix[key_matrix[key].row]|=key_matrix[key].bit;
	}
    }
    else
    {
    	/* TODO: Joysticks?  Were there any common ones for the 81? */
    }
}


Z80Byte ZX81ReadMem(Z80 *z80, Z80Word addr)
{
    return mem[addr];
}


void ZX81WriteMem(Z80 *z80, Z80Word addr, Z80Byte val)
{
    if (addr>=RAMBOT && addr<=RAMTOP)
    {
	mem[addr]=val;
    }
}


Z80Byte ZX81ReadPort(Z80 *z80, Z80Word port)
{
    Z80Byte b=0;

    switch(port&0xff)
    {
    	case 0xfe:	/* ULA */
	    /* Key matrix
	    */
	    switch(port&0xff00)
	    {
	    	case 0xfe00:
		    b=matrix[0];
		    break;
	    	case 0xfd00:
		    b=matrix[1];
		    break;
	    	case 0xfb00:
		    b=matrix[2];
		    break;
	    	case 0xf700:
		    b=matrix[3];
		    break;
	    	case 0xef00:
		    b=matrix[4];
		    break;
	    	case 0xdf00:
		    b=matrix[5];
		    break;
	    	case 0xbf00:
		    b=matrix[6];
		    break;
	    	case 0x7f00:
		    b=matrix[7];
		    break;
	    }

	    /* Some code expects some of the top bits set...  Of course, whether
	       or not this may be worse as other code doesn't expect the bits,
	       we shall find out!
	    */
	    b |= 0x60;

	    break;

	default:
	    b = 0xff;	/* Idle bus */
	    break;
    }

    return b;
}


void ZX81WritePort(Z80 *z80, Z80Word port, Z80Byte val)
{
    switch(port&0xff)
    {
    	case 0xfd:
	    break;

	case 0xfe:
	    break;
    }
}


void ZX81Reset(Z80 *z80)
{
    int f;

    for(f=0;f<8;f++)
    	matrix[f]=0x1f;

    Z80Reset(z80);
    Z80ResetCycles(z80,0);

    started=FALSE;

    hires = FALSE;
    hires_dfile = 0;
    last_I = 0x1e;
    DrawScreen = DrawScreen_TEXT;

    ClearBitmap();
}


void ZX81EnableFileSystem(int enable)
{
    enable_filesystem=enable;
}


void ZX81SetTape(const Z80Byte *image, int len)
{
    tape_image=image;
    tape_len=len;
}


void ZX81SuspendDisplay(void)
{
    ClearBitmap();
    ClearText();
}


void ZX81ResumeDisplay(void)
{
    ClearText();

    /* Reset last_I to force hi/lo res detection
    */
    last_I = 0;
}


void ZX81DisplayString(const char *p)
{
    uint16 *s;
    uint16 inv=0;
    int f;

    ClearText();

    s = txt_screen;
    f = 0;

    while(*p)
    {
	switch(*p)
	{
	    case '\n':
		s+=32;
		f=0;
		break;

	    case '%':
	    	inv^=0x40;
		break;

	    default:
		s[f++]=FromASCII(*p)|inv;
		break;
	}

	p++;
    }
}


void ZX81Reconfigure(void)
{
    if (DS81_Config[DS81_STATIC_RAM_AT_0x2000])
    {
    	RAMBOT = 0x2000;
    }
    else
    {
    	RAMBOT = 0x4000;
	memcpy(mem+ROMLEN,mem,ROMLEN);
    }

    allow_save = enable_filesystem && DS81_Config[DS81_ALLOW_TAPE_SAVE];
}


void ZX81SaveSnapshot(FILE *fp)
{
    int f;

    for(f=0; f<sizeof mem; f++)
    {
	PUT_Byte(fp, mem[f]);
    }

    for(f=0; f<sizeof matrix; f++)
    {
	PUT_Byte(fp, matrix[f]);
    }

    PUT_Long(fp, waitkey);
    PUT_Long(fp, started);

    PUT_ULong(fp, RAMBOT);
    PUT_ULong(fp, RAMTOP);

    PUT_ULong(fp, prev_lk1);
    PUT_ULong(fp, prev_lk2);
}


void ZX81LoadSnapshot(FILE *fp)
{
    int f;

    for(f=0; f<sizeof mem; f++)
    {
	mem[f] = GET_Byte(fp);
    }

    for(f=0; f<sizeof matrix; f++)
    {
	matrix[f] = GET_Byte(fp);
    }

    waitkey = GET_Long(fp);
    started = GET_Long(fp);

    RAMBOT = GET_ULong(fp);
    RAMTOP = GET_ULong(fp);

    prev_lk1 = GET_ULong(fp);
    prev_lk2 = GET_ULong(fp);

    /* Reset last_I to force hi/lo res detection
    */
    last_I = 0;
}


/* END OF FILE */
