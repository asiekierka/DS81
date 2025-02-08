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
  
   $Id: gui.h,v 1.7 2008-12-05 00:37:25 ianc Exp $
*/
#ifndef DS81_GUI_H
#define DS81_GUI_H

int	GUI_Menu(const char *opts[]);
void	GUI_Alert(int fatal, const char *text);
void	GUI_Config(void);
int	GUI_FileSelect(char pwd[], char selected_file[], const char *filter);
int	GUI_InputName(const char *prompt, const char *ext,
		      char name[], int maxlen);

#endif	/* DS81_GUI_H */
