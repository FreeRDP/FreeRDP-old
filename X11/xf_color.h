/*
   FreeRDP: A Remote Desktop Protocol client.
   Color conversions

   Copyright (C) Jay Sorg 2009-2011

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
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
