/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   GDI Window Routines

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

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

#ifndef __GDI_WINDOW_H
#define __GDI_WINDOW_H

#include <freerdp/freerdp.h>
#include "libfreerdpgdi.h"

unsigned int gdi_rop3_code(unsigned char code);
int gdi_clip_coords(GDI *gdi, int *x, int *y, int *w, int *h, int *srcx, int *srcy);
void gdi_invalidate_region(GDI *gdi, int x, int y, int w, int h);
void gdi_copy_mem(char * d, char * s, int n);
char* gdi_get_bitmap_pointer(HDC hdcBmp, int x, int y);
int gdi_is_mono_pixel_set(char* data, int x, int y, int width);
int gdi_init(rdpInst * inst);

#endif /* __GDI_WINDOW_H */
