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

    Provides the routines for streaming.

*/
#include "stream.h"

/* The long functions are a tad convuluted, but I'm in a dash.
*/

void PUT_Byte(FILE *fp, unsigned char c)
{
    fputc(c, fp);
}

void PUT_Long(FILE *fp, long l)
{
    union {long l; unsigned char c[4];} u;

    u.l = l;

    fputc(u.c[0], fp);
    fputc(u.c[1], fp);
    fputc(u.c[2], fp);
    fputc(u.c[3], fp);
}

void PUT_ULong(FILE *fp, unsigned long l)
{
    union {unsigned long l; unsigned char c[4];} u;

    u.l = l;

    fputc(u.c[0], fp);
    fputc(u.c[1], fp);
    fputc(u.c[2], fp);
    fputc(u.c[3], fp);
}

unsigned char GET_Byte(FILE *fp)
{
    return (unsigned char)fgetc(fp);
}

long GET_Long(FILE *fp)
{
    union {long l; unsigned char c[4];} u;

    u.c[0] = (unsigned char)fgetc(fp);
    u.c[1] = (unsigned char)fgetc(fp);
    u.c[2] = (unsigned char)fgetc(fp);
    u.c[3] = (unsigned char)fgetc(fp);

    return u.l;
}

unsigned long GET_ULong(FILE *fp)
{
    union {unsigned long l; unsigned char c[4];} u;

    u.c[0] = (unsigned char)fgetc(fp);
    u.c[1] = (unsigned char)fgetc(fp);
    u.c[2] = (unsigned char)fgetc(fp);
    u.c[3] = (unsigned char)fgetc(fp);

    return u.l;
}
