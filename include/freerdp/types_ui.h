/*
   Copyright (c) 2009 Jay Sorg

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

   these are the types needed for the ui interface
   self contained file, requires no other includes

*/

#ifndef __TYPES_UI_H
#define __TYPES_UI_H

typedef unsigned char uint8;
typedef signed char sint8;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned int uint32;
typedef signed int sint32;

typedef int RD_BOOL;
typedef void *RD_HBITMAP;
typedef void *RD_HGLYPH;
typedef void *RD_HCOLOURMAP;
typedef void *RD_HCURSOR;

typedef struct _RD_POINT
{
	sint16 x, y;
}
RD_POINT;

typedef struct _RD_COLOURENTRY
{
	uint8 red;
	uint8 green;
	uint8 blue;
}
RD_COLOURENTRY;

typedef struct _RD_COLOURMAP
{
	uint16 ncolours;
	RD_COLOURENTRY *colours;
}
RD_COLOURMAP;

typedef struct _RD_PEN
{
	uint8 style;
	uint8 width;
	uint32 colour;
}
RD_PEN;

/* this is whats in the brush cache */
typedef struct _RD_BRUSHDATA
{
	uint32 colour_code;
	uint32 data_size;
	uint8 *data;
}
RD_BRUSHDATA;

typedef struct _RD_BRUSH
{
	uint8 xorigin;
	uint8 yorigin;
	uint8 style;
	uint8 pattern[8];
	RD_BRUSHDATA *bd;
}
RD_BRUSH;

typedef struct _RD_PLUGIN_DATA
{
	uint16 size;
	void * data[4];
}
RD_PLUGIN_DATA;

#endif
