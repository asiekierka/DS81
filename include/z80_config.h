/*

    z80 - Z80 emulation

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

    $Id: z80_config.h,v 1.2 2007-03-01 00:36:54 ianc Exp $

*/

#ifndef Z80_CONFIG_H
#define Z80_CONFIG_H "$Id: z80_config.h,v 1.2 2007-03-01 00:36:54 ianc Exp $"


/* This file defines various compile-time configuration options
   for the Z80 emulation
*/


/* Define this to enable the disassembly interface
*/
#define ENABLE_DISASSEM


/* Define this to enable the array-based memory model.  In this mode
   an externally visible Z80Byte array called Z80_MEMORY must be
   defined.  The macros RAMBOT and RAMTOP define the writable area of
   memory and must be changed accordingly.

   In this mode the signature of Z80Init changes so that the memory functions
   are not passed.  ALL processor instances share the same memory.
#define ENABLE_ARRAY_MEMORY
*/

#ifdef ENABLE_ARRAY_MEMORY
#define RAMBOT 0x0000
#define RAMTOP 0xffff
#endif


#endif

/* END OF FILE */
