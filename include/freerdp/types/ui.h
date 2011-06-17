/*
   FreeRDP: A Remote Desktop Protocol client.

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

   these are the types needed for the ui interface
   self contained file, requires no other includes

*/

#ifndef __TYPES_UI_H
#define __TYPES_UI_H

#include <freerdp/types/base.h>

typedef int RD_BOOL;
typedef void *RD_HBITMAP;
typedef void *RD_HGLYPH;
typedef void *RD_HPALETTE;
typedef void *RD_HCURSOR;

#ifndef True
#define True  (1)
#define False (0)
#endif

typedef struct _RD_POINT
{
	sint16 x, y;
}
RD_POINT;

typedef struct _RD_PALETTEENTRY
{
	uint8 red;
	uint8 green;
	uint8 blue;
}
RD_PALETTEENTRY;

typedef struct _RD_PALETTE
{
	uint16 count;
	RD_PALETTEENTRY *entries;
}
RD_PALETTE;

typedef struct _RD_PEN
{
	uint8 style;
	uint8 width;
	uint32 color;
}
RD_PEN;

/* this is whats in the brush cache */
typedef struct _RD_BRUSHDATA
{
	uint32 color_code;
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

typedef struct _RD_RECT
{
	sint16 x;
	sint16 y;
	sint16 width;
	sint16 height;
}
RD_RECT;

typedef struct _RD_EVENT RD_EVENT;

typedef void (*RD_EVENT_CALLBACK) (RD_EVENT * event);

struct _RD_EVENT
{
	uint16 event_type;
	RD_EVENT_CALLBACK event_callback;
	void * user_data;
};

typedef struct _RD_VIDEO_FRAME_EVENT RD_VIDEO_FRAME_EVENT;
struct _RD_VIDEO_FRAME_EVENT
{
	RD_EVENT event;
	uint8 * frame_data;
	uint32 frame_size;
	uint32 frame_pixfmt;
	sint16 frame_width;
	sint16 frame_height;
	sint16 x;
	sint16 y;
	sint16 width;
	sint16 height;
	uint16 num_visible_rects;
	RD_RECT * visible_rects;
};

typedef struct _RD_REDRAW_EVENT RD_REDRAW_EVENT;
struct _RD_REDRAW_EVENT
{
	RD_EVENT event;
	sint16 x;
	sint16 y;
	sint16 width;
	sint16 height;
};

/* defined in include/freerdp/freerdp.h */
struct rdp_inst;
typedef struct rdp_inst rdpInst;

/* defined in include/freerdp/rdpset.h */
struct rdp_set;
typedef struct rdp_set rdpSet;

#endif
