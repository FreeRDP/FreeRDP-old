/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   GDI Color Conversion Routines

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

#ifndef __GDI_COLOR_H
#define __GDI_COLOR_H

#include "libfreerdpgdi.h"

unsigned int gdi_make_colorref(PIXEL *pixel);
void gdi_split_colorref(unsigned int colorref, PIXEL *pixel);
void gdi_color_convert(PIXEL *pixel, int color, int bpp, HPALETTE palette);
char* gdi_image_convert(char* srcData, int width, int height, int srcBpp, int dstBpp, HPALETTE palette);

#endif /* __GDI_COLOR_H */
