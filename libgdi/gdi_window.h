/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Window Routines

   Copyright 2010 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __GDI_WINDOW_H
#define __GDI_WINDOW_H

#include <freerdp/freerdp.h>
#include "gdi.h"

struct _gdi_bitmap
{
	HDC hdc;
	HBITMAP bitmap;
	HBITMAP org_bitmap;
};
typedef struct _gdi_bitmap gdi_bitmap;

struct _GDI
{
	int width;
	int height;
	int dstBpp;
	int srcBpp;
	int cursor_x;
	int cursor_y;
	int bytesPerPixel;

	HDC hdc;
	HCLRCONV clrconv;
	gdi_bitmap *primary;
	gdi_bitmap *drawing;
	uint8* primary_buffer;
	COLORREF textColor;
};
typedef struct _GDI GDI;

unsigned int gdi_rop3_code(unsigned char code);
void gdi_copy_mem(uint8 *d, uint8 *s, int n);
void gdi_copy_memb(uint8 *d, uint8 *s, int n);
uint8* gdi_get_bitmap_pointer(HDC hdcBmp, int x, int y);
uint8* gdi_get_brush_pointer(HDC hdcBrush, int x, int y);
int gdi_is_mono_pixel_set(uint8* data, int x, int y, int width);
int gdi_init(rdpInst * inst);

#endif /* __GDI_WINDOW_H */
