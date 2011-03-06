/*
   FreeRDP: A Remote Desktop Protocol client.

   Copyright (c) 2009-2011 Jay Sorg

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

#ifndef __WF_COLOR_H
#define __WF_COLOR_H

#include "wf_event.h"

int
wf_color_convert(wfInfo * wfi, int in_color, int in_bpp);
uint8 *
wf_image_convert(wfInfo * wfi, int width, int height, int bpp,
	int reverse, uint8 * in_data, uint8 * out_data);
RD_HPALETTE
wf_create_colormap(wfInfo * wfi, RD_PALETTE * colors);
int
wf_set_colormap(wfInfo * wfi, RD_HPALETTE map);
uint8 *
wf_glyph_convert(wfInfo * wfi, int width, int height, uint8 * data);
uint8 *
wf_cursor_mask_convert(wfInfo * wfi, int width, int height, uint8 * data);

#endif
