/*
   rdesktop: A Remote Desktop Protocol client.
   RDP order processing
   Copyright (C) Matthew Chapman 1999-2008

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

#ifndef __ORDERSTYPES_H
#define __ORDERSTYPES_H

#include "types.h"

#define RDP_ORDER_STANDARD   0x01
#define RDP_ORDER_SECONDARY  0x02
#define RDP_ORDER_BOUNDS     0x04
#define RDP_ORDER_CHANGE     0x08
#define RDP_ORDER_DELTA      0x10
#define RDP_ORDER_LASTBOUNDS 0x20
#define RDP_ORDER_SMALL      0x40
#define RDP_ORDER_TINY       0x80

/* Primary Drawing Orders */
enum RDP_ORDER_TYPE
{
	RDP_ORDER_DSTBLT = 0,
	RDP_ORDER_PATBLT = 1,
	RDP_ORDER_SCRBLT = 2,
	RDP_ORDER_DRAWNINEGRID = 8,
	RDP_ORDER_LINETO = 9,
	RDP_ORDER_OPAQUERECT = 10,
	RDP_ORDER_SAVEBITMAP = 11,
	RDP_ORDER_MEMBLT = 13,
	RDP_ORDER_MEM3BLT = 14,
	RDP_ORDER_MULTIDSTBLT = 15,
	RDP_ORDER_MULTIPATBLT = 16,
	RDP_ORDER_MULTISCRBLT = 17,
	RDP_ORDER_MULTIOPAQUERECT = 18,
	RDP_ORDER_FAST_INDEX = 19,
	RDP_ORDER_POLYGON_SC = 20,
	RDP_ORDER_POLYGON_CB = 21,
	RDP_ORDER_POLYLINE = 22,
	RDP_ORDER_FAST_GLYPH = 24,
	RDP_ORDER_ELLIPSE_SC = 25,
	RDP_ORDER_ELLIPSE_CB = 26,
	RDP_ORDER_GLYPH_INDEX = 27
};

/* Secondary Drawing Orders */
enum RDP_SECONDARY_ORDER_TYPE
{
	RDP_ORDER_CACHE_BITMAP_UNCOMPRESSED = 0,
	RDP_ORDER_CACHE_COLOR_TABLE = 1,
	RDP_ORDER_CACHE_BITMAP_COMPRESSED = 2,
	RDP_ORDER_CACHE_GLYPH = 3,
	RDP_ORDER_CACHE_BITMAP_UNCOMPRESSED_REV2 = 4,
	RDP_ORDER_CACHE_BITMAP_COMPRESSED_REV2 = 5,
	RDP_ORDER_CACHE_BRUSH = 7,
	RDP_ORDER_CACHE_BITMAP_COMPRESSED_REV3 = 8
};

/* Alternate Secondary Drawing Orders */
enum RDP_ALTSEC_ORDER_TYPE
{
	RDP_ORDER_ALTSEC_SWITCH_SURFACE = 0,
	RDP_ORDER_ALTSEC_CREATE_OFFSCR_BITMAP = 1,
	RDP_ORDER_ALTSEC_STREAM_BITMAP_FIRST = 2,
	RDP_ORDER_ALTSEC_STREAM_BITMAP_NEXT = 3,
	RDP_ORDER_ALTSEC_CREATE_NINEGRID_BITMAP = 4,
	RDP_ORDER_ALTSEC_GDIP_FIRST = 5,
	RDP_ORDER_ALTSEC_GDIP_NEXT = 6,
	RDP_ORDER_ALTSEC_GDIP_END = 7,
	RDP_ORDER_ALTSEC_GDIP_CACHE_FIRST = 8,
	RDP_ORDER_ALTSEC_GDIP_CACHE_NEXT = 9,
	RDP_ORDER_ALTSEC_GDIP_CACHE_END = 10,
	RDP_ORDER_ALTSEC_WINDOW = 11,
	RDP_ORDER_ALTSEC_COMPDESK_FIRST = 12,
	RDP_ORDER_ALTSEC_FRAME_MARKER = 13
};

typedef struct _DSTBLT_ORDER
{
	sint16 x;
	sint16 y;
	sint16 cx;
	sint16 cy;
	uint8 opcode;

}
DSTBLT_ORDER;

typedef struct _PATBLT_ORDER
{
	sint16 x;
	sint16 y;
	sint16 cx;
	sint16 cy;
	uint8 opcode;
	uint32 bgcolor;
	uint32 fgcolor;
	RD_BRUSH brush;

}
PATBLT_ORDER;

typedef struct _SCRBLT_ORDER
{
	sint16 x;
	sint16 y;
	sint16 cx;
	sint16 cy;
	uint8 opcode;
	sint16 srcx;
	sint16 srcy;

}
SCRBLT_ORDER;

typedef struct _LINETO_ORDER
{
	uint16 mixmode;
	sint16 startx;
	sint16 starty;
	sint16 endx;
	sint16 endy;
	uint32 bgcolor;
	uint8 opcode;
	RD_PEN pen;

}
LINETO_ORDER;

typedef struct _OPAQUERECT_ORDER
{
	sint16 x;
	sint16 y;
	sint16 cx;
	sint16 cy;
	uint32 color;

}
OPAQUERECT_ORDER;

typedef struct _SAVEBITMAP_ORDER
{
	uint32 offset;
	sint16 left;
	sint16 top;
	sint16 right;
	sint16 bottom;
	uint8 action;

}
SAVEBITMAP_ORDER;

typedef struct _MEM3BLT_ORDER
{
	uint8 color_table;
	uint8 cache_id;
	sint16 x;
	sint16 y;
	sint16 cx;
	sint16 cy;
	uint8 opcode;
	sint16 srcx;
	sint16 srcy;
	uint32 bgcolor;
	uint32 fgcolor;
	RD_BRUSH brush;
	uint16 cache_idx;
	uint16 unknown;

}
MEM3BLT_ORDER;

typedef struct _MEMBLT_ORDER
{
	uint8 color_table;
	uint8 cache_id;
	sint16 x;
	sint16 y;
	sint16 cx;
	sint16 cy;
	uint8 opcode;
	sint16 srcx;
	sint16 srcy;
	uint16 cache_idx;

}
MEMBLT_ORDER;

#define MAX_DATA 256

typedef struct _POLYGON_SC_ORDER
{
	sint16 x;
	sint16 y;
	uint8 opcode;
	uint8 fillmode;
	uint32 fgcolor;
	uint8 npoints;
	uint8 datasize;
	uint8 data[MAX_DATA];

}
POLYGON_SC_ORDER;

typedef struct _POLYGON_CB_ORDER
{
	sint16 x;
	sint16 y;
	uint8 opcode;
	uint8 fillmode;
	uint32 bgcolor;
	uint32 fgcolor;
	RD_BRUSH brush;
	uint8 npoints;
	uint8 datasize;
	uint8 data[MAX_DATA];

}
POLYGON_CB_ORDER;

typedef struct _POLYLINE_ORDER
{
	sint16 x;
	sint16 y;
	uint8 opcode;
	uint32 fgcolor;
	uint8 lines;
	uint8 datasize;
	uint8 data[MAX_DATA];

}
POLYLINE_ORDER;

typedef struct _ELLIPSE_SC_ORDER
{
	sint16 left;
	sint16 top;
	sint16 right;
	sint16 bottom;
	uint8 opcode;
	uint8 fillmode;
	uint32 fgcolor;

}
ELLIPSE_SC_ORDER;

typedef struct _ELLIPSE_CB_ORDER
{
	sint16 left;
	sint16 top;
	sint16 right;
	sint16 bottom;
	uint8 opcode;
	uint8 fillmode;
	RD_BRUSH brush;
	uint32 bgcolor;
	uint32 fgcolor;

}
ELLIPSE_CB_ORDER;

#define MAX_TEXT 256

typedef struct _GLYPH_INDEX_ORDER
{
	uint8 font;
	uint8 flags;
	uint8 opcode;
	uint8 mixmode;
	uint32 bgcolor;
	uint32 fgcolor;
	sint16 clipleft;
	sint16 cliptop;
	sint16 clipright;
	sint16 clipbottom;
	sint16 boxleft;
	sint16 boxtop;
	sint16 boxright;
	sint16 boxbottom;
	RD_BRUSH brush;
	sint16 x;
	sint16 y;
	uint8 length;
	uint8 text[MAX_TEXT];

}
GLYPH_INDEX_ORDER;

typedef struct _FAST_INDEX_ORDER
{
	uint8 font;
	uint8 flags;
	uint8 opcode;
	uint32 bgcolor;
	uint32 fgcolor;
	sint16 clipleft;
	sint16 cliptop;
	sint16 clipright;
	sint16 clipbottom;
	sint16 boxleft;
	sint16 boxtop;
	sint16 boxright;
	sint16 boxbottom;
	RD_BRUSH brush;
	sint16 x;
	sint16 y;
	uint8 length;
	uint8 text[MAX_TEXT];
}
FAST_INDEX_ORDER;

typedef struct _FAST_GLYPH_ORDER
{
	uint8 font;
	uint8 flags;
	uint8 opcode;
	uint32 bgcolor;
	uint32 fgcolor;
	sint16 clipleft;
	sint16 cliptop;
	sint16 clipright;
	sint16 clipbottom;
	sint16 boxleft;
	sint16 boxtop;
	sint16 boxright;
	sint16 boxbottom;
	sint16 x;
	sint16 y;
	uint8 length;
	uint8 data[256];
}
FAST_GLYPH_ORDER;

typedef struct _RDP_ORDER_STATE
{
	uint8 order_type;
	BOUNDS bounds;

	DSTBLT_ORDER dstblt;
	PATBLT_ORDER patblt;
	SCRBLT_ORDER scrblt;
	LINETO_ORDER lineto;
	OPAQUERECT_ORDER opaquerect;
	SAVEBITMAP_ORDER savebitmap;
	MEMBLT_ORDER memblt;
	MEM3BLT_ORDER mem3blt;
	POLYGON_SC_ORDER polygon_sc;
	POLYGON_CB_ORDER polygon_cb;
	POLYLINE_ORDER polyline;
	ELLIPSE_SC_ORDER ellipse_sc;
	ELLIPSE_CB_ORDER ellipse_cb;
	GLYPH_INDEX_ORDER glyph_index;
	FAST_INDEX_ORDER fast_index;
	FAST_GLYPH_ORDER fast_glyph;
}
RDP_ORDER_STATE;

typedef struct _RDP_CACHE_BITMAP_UNCOMPRESSED_ORDER
{
	uint8 cache_id;
	uint8 pad1;
	uint8 width;
	uint8 height;
	uint8 bpp;
	uint16 bufsize;
	uint16 cache_idx;
	uint8 *data;

}
RDP_CACHE_BITMAP_UNCOMPRESSED_ORDER;

typedef struct _RDP_CACHE_BITMAP_COMPRESSED_ORDER
{
	uint8 cache_id;
	uint8 pad1;
	uint8 width;
	uint8 height;
	uint8 bpp;
	uint16 bufsize;
	uint16 cache_idx;
	uint16 pad2;
	uint16 size;
	uint16 row_size;
	uint16 final_size;
	uint8 *data;

}
RDP_CACHE_BITMAP_COMPRESSED_ORDER;

/* RDP_CACHE_BITMAP_COMPRESSED_REV2_ORDER */
#define ID_MASK			0x0007
#define MODE_MASK		0x0038
#define SQUARE			0x0080
#define PERSIST			0x0100
#define FLAG_51_UNKNOWN		0x0800

#define MODE_SHIFT		3

#define LONG_FORMAT		0x80
#define BUFSIZE_MASK		0x3FFF	/* or 0x1FFF? */

#define MAX_GLYPH 32

typedef struct _RDP_GLYPH
{
	uint16 character;
	uint16 unknown;
	uint16 baseline;
	uint16 width;
	uint16 height;
	uint8 data[MAX_GLYPH];

}
RDP_GLYPH;

#define MAX_GLYPHS 256

typedef struct _RDP_CACHE_GLYPH_ORDER
{
	uint8 font;
	uint8 nglyphs;
	RDP_GLYPH glyphs[MAX_GLYPHS];

}
RDP_CACHE_GLYPH_ORDER;

typedef struct _RDP_CACHE_COLOR_TABLE_ORDER
{
	uint8 cache_id;
	RD_PALETTE map;
}
RDP_CACHE_COLOR_TABLE_ORDER;

#endif // __ORDERSTYPES_H
