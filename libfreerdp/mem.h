/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   memory functions
   Copyright (c) 2009-2010 Jay Sorg

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __MEM1_H
#define __MEM1_H

void *
xmalloc(int size);
void *
xrealloc(void * oldmem, int size);
void
xfree(void * mem);
char *
xstrdup(const char * s);

#endif
