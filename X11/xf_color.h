/*
   Copyright (c) 2009-2010 Jay Sorg

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#ifndef __XF_COLOR_H
#define __XF_COLOR_H

#include <freerdp/freerdp.h>
#include "xf_types.h"

int
xf_color_convert(xfInfo * xfi, rdpSet * settings, int color);
uint8 *
xf_image_convert(xfInfo * xfi, rdpSet * settings, int width, int height,
	uint8 * in_data);
RD_HPALETTE
xf_create_colormap(xfInfo * xfi, rdpSet * settings, RD_PALETTE * colors);
int
xf_set_colormap(xfInfo * xfi, rdpSet * settings, RD_HPALETTE map);
int
xf_cursor_convert_mono(xfInfo * xfi, uint8 * src_glyph, uint8 * msk_glyph,
	uint8 * xormask, uint8 * andmask, int width, int height, int bpp);
int
xf_cursor_convert_alpha(xfInfo * xfi, uint8 * alpha_data,
	uint8 * xormask, uint8 * andmask, int width, int height, int bpp);

#endif
