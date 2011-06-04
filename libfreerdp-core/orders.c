/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP orders processing

   Copyright (C) Matthew Chapman 1999-2008

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

#include "frdp.h"
#include "rdp.h"
#include "pstcache.h"
#include "cache.h"
#include "bitmap.h"
#include <freerdp/rdpset.h>

#include "orders.h"

/* Read field indicating which parameters are present */
static void
rdp_in_present(STREAM s, uint32 * present, uint8 flags, int size)
{
	int i;
	uint8 bits;

	if (flags & RDP_ORDER_CTL_ZERO_FIELD_BYTE_BIT0)
	{
		size--;
	}

	if (flags & RDP_ORDER_CTL_ZERO_FIELD_BYTE_BIT1)
	{
		if (size < 2)
			size = 0;
		else
			size -= 2;
	}

	*present = 0;
	for (i = 0; i < size; i++)
	{
		in_uint8(s, bits);
		*present |= bits << (i * 8);
	}
}

/* Read a coordinate (16-bit, or 8-bit delta) */
static void
rdp_in_coord(STREAM s, sint16 * coord, RD_BOOL delta)
{
	sint8 change;

	if (delta)
	{
		in_uint8(s, change);
		*coord += change;
	}
	else
	{
		in_uint16_le(s, *coord);
	}
}

/* Parse a delta coordinate in polyline/polygon order form */
static int
parse_delta(uint8 * buffer, int *offset)
{
	int value = buffer[(*offset)++];
	int two_byte = value & 0x80;

	if (value & 0x40)	/* sign bit */
		value |= ~0x3f;
	else
		value &= 0x3f;

	if (two_byte)
		value = (value << 8) | buffer[(*offset)++];

	return value;
}

static int
parse_s2byte(uint8 * buffer, int *offset)
{
	int value = buffer[(*offset)++];
	int two_byte = value & 0x80;
	int sign = value & 0x40;

	value &= 0x3f;
	if (two_byte)
		value = (value << 8) | buffer[(*offset)++];
	if (sign)
		value = value * -1;
	return value;
}

static int
parse_u2byte(uint8 * buffer, int *offset)
{
	int value = buffer[(*offset)++];
	int two_byte = value & 0x80;

	value &= 0x7f;
	if (two_byte)
		value = (value << 8) | buffer[(*offset)++];
	return value;
}

/* Read a color entry */
static void
rdp_in_color(STREAM s, uint32 *color)
{
	uint32 i;
	in_uint8(s, i);
	*color = i;
	in_uint8(s, i);
	*color |= i << 8;
	in_uint8(s, i);
	*color |= i << 16;
}

/* Parse bounds information */
static RD_BOOL
rdp_parse_bounds(STREAM s, BOUNDS * bounds)
{
	uint8 present;

	in_uint8(s, present);

	if (present & 1)
		rdp_in_coord(s, &bounds->left, False);
	else if (present & 16)
		rdp_in_coord(s, &bounds->left, True);

	if (present & 2)
		rdp_in_coord(s, &bounds->top, False);
	else if (present & 32)
		rdp_in_coord(s, &bounds->top, True);

	if (present & 4)
		rdp_in_coord(s, &bounds->right, False);
	else if (present & 64)
		rdp_in_coord(s, &bounds->right, True);

	if (present & 8)
		rdp_in_coord(s, &bounds->bottom, False);
	else if (present & 128)
		rdp_in_coord(s, &bounds->bottom, True);

	return s_check(s);
}

/* Parse a pen */
static RD_BOOL
rdp_parse_pen(STREAM s, RD_PEN * pen, uint32 present)
{
	if (present & 1)
		in_uint8(s, pen->style);

	if (present & 2)
		in_uint8(s, pen->width);

	if (present & 4)
		rdp_in_color(s, &pen->color);

	return s_check(s);
}

static void
setup_brush(rdpOrders * orders, RD_BRUSH * out_brush, RD_BRUSH * in_brush)
{
	RD_BRUSHDATA *brush_data;
	uint8 cache_idx;
	uint8 color_code;

	memcpy(out_brush, in_brush, sizeof(RD_BRUSH));
	if (out_brush->style & 0x80)
	{
		color_code = out_brush->style & 0x0f;
		cache_idx = out_brush->pattern[0];
		brush_data = cache_get_brush_data(orders->rdp->cache, color_code, cache_idx);
		if ((brush_data == NULL) || (brush_data->data == NULL))
		{
			ui_error(orders->rdp->inst, "error getting brush data, style %x\n",
				 out_brush->style);
			out_brush->bd = NULL;
			memset(out_brush->pattern, 0, 8);
		}
		else
		{
			out_brush->bd = brush_data;
		}
		out_brush->style = 3;
	}
}

/* Parse a brush */
static RD_BOOL
rdp_parse_brush(STREAM s, RD_BRUSH * brush, uint32 present)
{
	if (present & 1)
		in_uint8(s, brush->xorigin);

	if (present & 2)
		in_uint8(s, brush->yorigin);

	if (present & 4)
		in_uint8(s, brush->style);

	if (present & 8)
		in_uint8(s, brush->pattern[0]);

	if (present & 16)
		in_uint8a(s, &brush->pattern[1], 7);

	return s_check(s);
}

/* Process a destination blt order */
static void
process_dstblt(rdpOrders * orders, STREAM s, DSTBLT_ORDER * os, uint32 present, RD_BOOL delta)
{
	if (present & 0x01)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x02)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x04)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x08)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x10)
		in_uint8(s, os->opcode);

	DEBUG_ORDERS("DESTBLT(op=0x%x,x=%d,y=%d,cx=%d,cy=%d)",
	      os->opcode, os->x, os->y, os->cx, os->cy);

	ui_destblt(orders->rdp->inst, os->opcode, os->x, os->y, os->cx, os->cy);
}

/* Process a pattern blt order */
static void
process_patblt(rdpOrders * orders, STREAM s, PATBLT_ORDER * os, uint32 present, RD_BOOL delta)
{
	RD_BRUSH brush;

	if (present & 0x0001)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x0002)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x0004)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x0008)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x0010)
		in_uint8(s, os->opcode);

	if (present & 0x0020)
		rdp_in_color(s, &os->bgcolor);

	if (present & 0x0040)
		rdp_in_color(s, &os->fgcolor);

	rdp_parse_brush(s, &os->brush, present >> 7);

	DEBUG_ORDERS("PATBLT(op=0x%x,x=%d,y=%d,cx=%d,cy=%d,bs=%d,bg=0x%x,fg=0x%x)", os->opcode, os->x,
	       os->y, os->cx, os->cy, os->brush.style, os->bgcolor, os->fgcolor);

	setup_brush(orders, &brush, &os->brush);

	ui_patblt(orders->rdp->inst, os->opcode, os->x, os->y, os->cx, os->cy,
		  &brush, os->bgcolor, os->fgcolor);
}

/* Process a multi pattern blt order */
static void
process_multipatblt(rdpOrders * orders, STREAM s, MULTIPATBLT_ORDER * os, uint32 present, RD_BOOL delta)
{
	RD_BRUSH brush;
	size_t size;
	int index, data, next;
	uint8 flags = 0;
	RECTANGLE *rects;

	if (present & 0x0001)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x0002)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x0004)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x0008)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x0010)
		in_uint8(s, os->opcode);

	if (present & 0x0020)
		rdp_in_color(s, &os->bgcolor);

	if (present & 0x0040)
		rdp_in_color(s, &os->fgcolor);

	rdp_parse_brush(s, &os->brush, present >> 7);

	if (present & 0x1000)
		in_uint8(s, os->nentries);

	if (present & 0x2000)
	{
		in_uint16_le(s, os->datasize);
		in_uint8a(s, os->data, os->datasize);
	}

	DEBUG_ORDERS("MULTIPATBLT(op=0x%x,x=%d,y=%d,cx=%d,cy=%d,n=%d)",
	      os->opcode, os->x, os->y, os->cx, os->cy, os->nentries);

	setup_brush(orders, &brush, &os->brush);

	size = (os->nentries + 1) * sizeof(RECTANGLE);
	if (size > orders->buffer_size)
	{
		orders->buffer = xrealloc(orders->buffer, size);
		orders->buffer_size = size;
	}

	rects = (RECTANGLE *) orders->buffer;
	memset(rects, 0, size);

	index = 0;
	data = (os->nentries + 1) >> 1;
	for (next = 1; (next <= os->nentries) && (next <= 45) && (data < os->datasize); next++)
	{
		if ((next - 1) % 2 == 0)
			flags = os->data[index++];

		if (~flags & 0x80)
			rects[next].l = parse_delta(os->data, &data);

		if (~flags & 0x40)
			rects[next].t = parse_delta(os->data, &data);

		if (~flags & 0x20)
			rects[next].w = parse_delta(os->data, &data);
		else
			rects[next].w = rects[next - 1].w;

		if (~flags & 0x10)
			rects[next].h = parse_delta(os->data, &data);
		else
			rects[next].h = rects[next - 1].h;

		rects[next].l = rects[next].l + rects[next - 1].l;
		rects[next].t = rects[next].t + rects[next - 1].t;

		DEBUG_ORDERS("rect (%d, %d, %d, %d)",
			rects[next].l, rects[next].t, rects[next].w, rects[next].h);

		flags <<= 4;

		ui_patblt(orders->rdp->inst, os->opcode, rects[next].l, rects[next].t,
			rects[next].w, rects[next].h, &brush, os->bgcolor, os->fgcolor);
	}
}

/* Process a screen blt order */
static void
process_scrblt(rdpOrders * orders, STREAM s, SCRBLT_ORDER * os, uint32 present, RD_BOOL delta)
{
	if (present & 0x0001)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x0002)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x0004)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x0008)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x0010)
		in_uint8(s, os->opcode);

	if (present & 0x0020)
		rdp_in_coord(s, &os->srcx, delta);

	if (present & 0x0040)
		rdp_in_coord(s, &os->srcy, delta);

	DEBUG_ORDERS("SCRBLT(op=0x%x,x=%d,y=%d,cx=%d,cy=%d,srcx=%d,srcy=%d)",
	       os->opcode, os->x, os->y, os->cx, os->cy, os->srcx, os->srcy);

	ui_screenblt(orders->rdp->inst, os->opcode, os->x, os->y, os->cx, os->cy,
		     os->srcx, os->srcy);
}

/* Process a lineto order */
static void
process_lineto(rdpOrders * orders, STREAM s, LINETO_ORDER * os, uint32 present, RD_BOOL delta)
{
	if (present & 0x0001)
		in_uint16_le(s, os->mixmode);

	if (present & 0x0002)
		rdp_in_coord(s, &os->startx, delta);

	if (present & 0x0004)
		rdp_in_coord(s, &os->starty, delta);

	if (present & 0x0008)
		rdp_in_coord(s, &os->endx, delta);

	if (present & 0x0010)
		rdp_in_coord(s, &os->endy, delta);

	if (present & 0x0020)
		rdp_in_color(s, &os->bgcolor);

	if (present & 0x0040)
		in_uint8(s, os->opcode);

	rdp_parse_pen(s, &os->pen, present >> 7);

	DEBUG_ORDERS("LINETO(op=0x%x,sx=%d,sy=%d,dx=%d,dy=%d,fg=0x%x)",
	       os->opcode, os->startx, os->starty, os->endx, os->endy, os->pen.color);

	if (os->opcode < 0x01 || os->opcode > 0x10)
	{
		ui_error(orders->rdp->inst, "bad ROP2 0x%x\n", os->opcode);
		return;
	}

	ui_line(orders->rdp->inst, os->opcode, os->startx, os->starty, os->endx,
		os->endy, &os->pen);
}

/* Process an opaque rectangle order */
static void
process_opaquerect(rdpOrders * orders, STREAM s, OPAQUERECT_ORDER * os, uint32 present, RD_BOOL delta)
{
	uint32 i;

	if (present & 0x01)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x02)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x04)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x08)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x10)
	{
		in_uint8(s, i);
		os->color = (os->color & 0xffffff00) | i;
	}

	if (present & 0x20)
	{
		in_uint8(s, i);
		os->color = (os->color & 0xffff00ff) | (i << 8);
	}

	if (present & 0x40)
	{
		in_uint8(s, i);
		os->color = (os->color & 0xff00ffff) | (i << 16);
	}

	DEBUG_ORDERS("OPAQUERECT(x=%d,y=%d,cx=%d,cy=%d,fg=0x%x)", os->x, os->y, os->cx, os->cy, os->color);

	ui_rect(orders->rdp->inst, os->x, os->y, os->cx, os->cy, os->color);
}

/* Process a multi opaque rectangle order */
static void
process_multiopaquerect(rdpOrders * orders, STREAM s, MULTIOPAQUERECT_ORDER * os, uint32 present, RD_BOOL delta)
{
	uint32 i;
	size_t size;
	int index, data, next;
	uint8 flags = 0;
	RECTANGLE *rects;

	if (present & 0x001)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x002)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x004)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x008)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x010)
	{
		in_uint8(s, i);
		os->color = (os->color & 0xffffff00) | i;
	}

	if (present & 0x020)
	{
		in_uint8(s, i);
		os->color = (os->color & 0xffff00ff) | (i << 8);
	}

	if (present & 0x040)
	{
		in_uint8(s, i);
		os->color = (os->color & 0xff00ffff) | (i << 16);
	}

	if (present & 0x080)
		in_uint8(s, os->nentries);

	if (present & 0x100)
	{
		in_uint16_le(s, os->datasize);
		in_uint8a(s, os->data, os->datasize);
	}

	DEBUG_ORDERS("MULTIOPAQUERECT(x=%d,y=%d,cx=%d,cy=%d,fg=0x%x,ne=%d,n=%d)", os->x, os->y, os->cx, os->cy,
		os->color, os->nentries, os->datasize);

	size = (os->nentries + 1) * sizeof(RECTANGLE);
	if (size > orders->buffer_size)
	{
		orders->buffer = xrealloc(orders->buffer, size);
		orders->buffer_size = size;
	}

	rects = (RECTANGLE *) orders->buffer;
	memset(rects, 0, size);

	index = 0;
	data = (os->nentries + 1) >> 1;
	for (next = 1; (next <= os->nentries) && (next <= 45) && (data < os->datasize); next++)
	{
		if ((next - 1) % 2 == 0)
			flags = os->data[index++];

		if (~flags & 0x80)
			rects[next].l = parse_delta(os->data, &data);

		if (~flags & 0x40)
			rects[next].t = parse_delta(os->data, &data);

		if (~flags & 0x20)
			rects[next].w = parse_delta(os->data, &data);
		else
			rects[next].w = rects[next - 1].w;

		if (~flags & 0x10)
			rects[next].h = parse_delta(os->data, &data);
		else
			rects[next].h = rects[next - 1].h;

		rects[next].l = rects[next].l + rects[next - 1].l;
		rects[next].t = rects[next].t + rects[next - 1].t;

		DEBUG_ORDERS("rect (%d, %d, %d, %d)",
			rects[next].l, rects[next].t, rects[next].w, rects[next].h);

		flags <<= 4;

		ui_rect(orders->rdp->inst, rects[next].l, rects[next].t,
			rects[next].w, rects[next].h, os->color);
	}
}

/* Process a save bitmap order */
static void
process_savebitmap(rdpOrders * orders, STREAM s, SAVEBITMAP_ORDER * os, uint32 present, RD_BOOL delta)
{
	int width, height;
	void * inst;

	if (present & 0x01)
		in_uint32_le(s, os->offset);

	if (present & 0x02)
		rdp_in_coord(s, &os->left, delta);

	if (present & 0x04)
		rdp_in_coord(s, &os->top, delta);

	if (present & 0x08)
		rdp_in_coord(s, &os->right, delta);

	if (present & 0x10)
		rdp_in_coord(s, &os->bottom, delta);

	if (present & 0x20)
		in_uint8(s, os->action);

	DEBUG_ORDERS("SAVEBITMAP(l=%d,t=%d,r=%d,b=%d,off=%d,op=%d)",
	       os->left, os->top, os->right, os->bottom, os->offset, os->action);

	width = os->right - os->left + 1;
	height = os->bottom - os->top + 1;

	inst = orders->rdp->inst;
	if (os->action == 0)
		ui_desktop_save(inst, os->offset, os->left, os->top, width, height);
	else
		ui_desktop_restore(inst, os->offset, os->left, os->top, width, height);
}

/* Process a memblt order */
static void
process_memblt(rdpOrders * orders, STREAM s, MEMBLT_ORDER * os, uint32 present, RD_BOOL delta)
{
	RD_HBITMAP bitmap;

	if (present & 0x0001)
	{
		in_uint8(s, os->cache_id);
		in_uint8(s, os->color_table);
	}

	if (present & 0x0002)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x0004)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x0008)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x0010)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x0020)
		in_uint8(s, os->opcode);

	if (present & 0x0040)
		rdp_in_coord(s, &os->srcx, delta);

	if (present & 0x0080)
		rdp_in_coord(s, &os->srcy, delta);

	if (present & 0x0100)
		in_uint16_le(s, os->cache_idx);

	DEBUG_ORDERS("MEMBLT(op=0x%x,x=%d,y=%d,cx=%d,cy=%d,id=%d,idx=%d)",
	       os->opcode, os->x, os->y, os->cx, os->cy, os->cache_id, os->cache_idx);

	bitmap = cache_get_bitmap(orders->rdp->cache, os->cache_id, os->cache_idx);
	if (bitmap == NULL)
		return;

	ui_memblt(orders->rdp->inst, os->opcode, os->x, os->y, os->cx, os->cy,
		  bitmap, os->srcx, os->srcy);
}

/* Process a mem3blt order */
static void
process_mem3blt(rdpOrders * orders, STREAM s, MEM3BLT_ORDER * os, uint32 present, RD_BOOL delta)
{
	RD_HBITMAP bitmap;
	RD_BRUSH brush;

	if (present & 0x000001)
	{
		in_uint8(s, os->cache_id);
		in_uint8(s, os->color_table);
	}

	if (present & 0x000002)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x000004)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x000008)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x000010)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x000020)
		in_uint8(s, os->opcode);

	if (present & 0x000040)
		rdp_in_coord(s, &os->srcx, delta);

	if (present & 0x000080)
		rdp_in_coord(s, &os->srcy, delta);

	if (present & 0x000100)
		rdp_in_color(s, &os->bgcolor);

	if (present & 0x000200)
		rdp_in_color(s, &os->fgcolor);

	rdp_parse_brush(s, &os->brush, present >> 10);

	if (present & 0x008000)
		in_uint16_le(s, os->cache_idx);

	if (present & 0x010000)
		in_uint16_le(s, os->unknown);

	DEBUG_ORDERS("MEM3BLT(op=0x%x,x=%d,y=%d,cx=%d,cy=%d,id=%d,idx=%d,bs=%d,bg=0x%x,fg=0x%x)",
	       os->opcode, os->x, os->y, os->cx, os->cy, os->cache_id, os->cache_idx,
	       os->brush.style, os->bgcolor, os->fgcolor);

	bitmap = cache_get_bitmap(orders->rdp->cache, os->cache_id, os->cache_idx);
	if (bitmap == NULL)
		return;

	setup_brush(orders, &brush, &os->brush);

	ui_triblt(orders->rdp->inst, os->opcode, os->x, os->y, os->cx, os->cy,
		  bitmap, os->srcx, os->srcy, &brush, os->bgcolor, os->fgcolor);
}

/* Process a polygon order */
static void
process_polygon_sc(rdpOrders * orders, STREAM s, POLYGON_SC_ORDER* os, uint32 present, RD_BOOL delta)
{
	size_t size;
	int index, data, next;
	uint8 flags = 0;
	RD_POINT *points;

	if (present & 0x01)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x02)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x04)
		in_uint8(s, os->opcode);

	if (present & 0x08)
		in_uint8(s, os->fillmode);

	if (present & 0x10)
		rdp_in_color(s, &os->fgcolor);

	if (present & 0x20)
		in_uint8(s, os->npoints);

	if (present & 0x40)
	{
		in_uint8(s, os->datasize);
		in_uint8a(s, os->data, os->datasize);
	}

	DEBUG_ORDERS("POLYGON_SC(x=%d,y=%d,op=0x%x,fm=%d,fg=0x%x,n=%d,sz=%d)",
	       os->x, os->y, os->opcode, os->fillmode, os->fgcolor, os->npoints, os->datasize);

	if (os->opcode < 0x01 || os->opcode > 0x10)
	{
		ui_error(orders->rdp->inst, "bad ROP2 0x%x\n", os->opcode);
		return;
	}

	size = (os->npoints + 1) * sizeof(RD_POINT);

	if (size > orders->buffer_size)
	{
		orders->buffer = xrealloc(orders->buffer, size);
		orders->buffer_size = size;
	}

	points = (RD_POINT *) orders->buffer;
	memset(points, 0, size);

	points[0].x = os->x;
	points[0].y = os->y;

	index = 0;
	data = ((os->npoints - 1) / 4) + 1;
	for (next = 1; (next <= os->npoints) && (next < 256) && (data < os->datasize); next++)
	{
		if ((next - 1) % 4 == 0)
			flags = os->data[index++];

		if (~flags & 0x80)
			points[next].x = parse_delta(os->data, &data);

		if (~flags & 0x40)
			points[next].y = parse_delta(os->data, &data);

		flags <<= 2;
	}

	if (next - 1 == os->npoints)
		ui_polygon(orders->rdp->inst, os->opcode, os->fillmode, points,
			   os->npoints + 1, NULL, 0, os->fgcolor);
	else
		ui_error(orders->rdp->inst, "polygon_sc parse error\n");
}

/* Process a polygon2 order */
static void
process_polygon_cb(rdpOrders * orders, STREAM s, POLYGON_CB_ORDER * os, uint32 present, RD_BOOL delta)
{
	size_t size;
	int index, data, next;
	uint8 flags = 0;
	RD_POINT *points;
	RD_BRUSH brush;

	if (present & 0x0001)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x0002)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x0004)
		in_uint8(s, os->opcode);

	if (present & 0x0008)
		in_uint8(s, os->fillmode);

	if (present & 0x0010)
		rdp_in_color(s, &os->bgcolor);

	if (present & 0x0020)
		rdp_in_color(s, &os->fgcolor);

	rdp_parse_brush(s, &os->brush, present >> 6);

	if (present & 0x0800)
		in_uint8(s, os->npoints);

	if (present & 0x1000)
	{
		in_uint8(s, os->datasize);
		in_uint8a(s, os->data, os->datasize);
	}

	DEBUG_ORDERS("POLYGON_CB(x=%d,y=%d,op=0x%x,fm=%d,bs=%d,bg=0x%x,fg=0x%x,n=%d,sz=%d)",
	       os->x, os->y, os->opcode, os->fillmode, os->brush.style, os->bgcolor, os->fgcolor,
	       os->npoints, os->datasize);

	if (os->opcode < 0x01 || os->opcode > 0x10)
	{
		ui_error(orders->rdp->inst, "bad ROP2 0x%x\n", os->opcode);
		return;
	}

	setup_brush(orders, &brush, &os->brush);

	size = (os->npoints + 1) * sizeof(RD_POINT);

	if (size > orders->buffer_size)
	{
		orders->buffer = xrealloc(orders->buffer, size);
		orders->buffer_size = size;
	}

	points = (RD_POINT *) orders->buffer;
	memset(points, 0, size);

	points[0].x = os->x;
	points[0].y = os->y;

	index = 0;
	data = ((os->npoints - 1) / 4) + 1;
	for (next = 1; (next <= os->npoints) && (next < 256) && (data < os->datasize); next++)
	{
		if ((next - 1) % 4 == 0)
			flags = os->data[index++];

		if (~flags & 0x80)
			points[next].x = parse_delta(os->data, &data);

		if (~flags & 0x40)
			points[next].y = parse_delta(os->data, &data);

		flags <<= 2;
	}

	if (next - 1 == os->npoints)
		ui_polygon(orders->rdp->inst, os->opcode, os->fillmode, points,
			   os->npoints + 1, &brush, os->bgcolor, os->fgcolor);
	else
		ui_error(orders->rdp->inst, "polygon_cb parse error\n");
}

/* Process a polyline order */
static void
process_polyline(rdpOrders * orders, STREAM s, POLYLINE_ORDER * os, uint32 present, RD_BOOL delta)
{
	size_t size;
	int index, next, data;
	uint8 flags = 0;
	RD_PEN pen;
	RD_POINT *points;

	if (present & 0x01)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x02)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x04)
		in_uint8(s, os->opcode);

	if (present & 0x10)
		rdp_in_color(s, &os->fgcolor);

	if (present & 0x20)
		in_uint8(s, os->lines);

	if (present & 0x40)
	{
		in_uint8(s, os->datasize);
		in_uint8a(s, os->data, os->datasize);
	}

	DEBUG_ORDERS("POLYLINE(x=%d,y=%d,op=0x%x,fg=0x%x,n=%d,sz=%d)",
	       os->x, os->y, os->opcode, os->fgcolor, os->lines, os->datasize);

	if (os->opcode < 0x01 || os->opcode > 0x10)
	{
		ui_error(orders->rdp->inst, "bad ROP2 0x%x\n", os->opcode);
		return;
	}

	size = (os->lines + 1) * sizeof(RD_POINT);

	if (size > orders->buffer_size)
	{
		orders->buffer = xrealloc(orders->buffer, size);
		orders->buffer_size = size;
	}

	points = (RD_POINT *) orders->buffer;
	memset(points, 0, size);

	points[0].x = os->x;
	points[0].y = os->y;
	pen.style = pen.width = 0;
	pen.color = os->fgcolor;

	index = 0;
	data = ((os->lines - 1) / 4) + 1;
	for (next = 1; (next <= os->lines) && (data < os->datasize); next++)
	{
		if ((next - 1) % 4 == 0)
			flags = os->data[index++];

		if (~flags & 0x80)
			points[next].x = parse_delta(os->data, &data);

		if (~flags & 0x40)
			points[next].y = parse_delta(os->data, &data);

		flags <<= 2;
	}

	if (next - 1 == os->lines)
		ui_polyline(orders->rdp->inst, os->opcode, points, os->lines + 1, &pen);
	else
		ui_error(orders->rdp->inst, "polyline parse error\n");
}

/* Process an EllipseSC order */
static void
process_ellipse_sc(rdpOrders * orders, STREAM s, ELLIPSE_SC_ORDER * os, uint32 present, RD_BOOL delta)
{
	if (present & 0x01)
		rdp_in_coord(s, &os->left, delta);

	if (present & 0x02)
		rdp_in_coord(s, &os->top, delta);

	if (present & 0x04)
		rdp_in_coord(s, &os->right, delta);

	if (present & 0x08)
		rdp_in_coord(s, &os->bottom, delta);

	if (present & 0x10)
		in_uint8(s, os->opcode);

	if (present & 0x20)
		in_uint8(s, os->fillmode);

	if (present & 0x40)
		rdp_in_color(s, &os->fgcolor);

	DEBUG_ORDERS("ELLIPSE_SC(l=%d,t=%d,r=%d,b=%d,op=0x%x,fm=%d,fg=0x%x)", os->left, os->top,
	       os->right, os->bottom, os->opcode, os->fillmode, os->fgcolor);

	ui_ellipse(orders->rdp->inst, os->opcode, os->fillmode, os->left, os->top,
		   os->right - os->left, os->bottom - os->top, NULL, 0, os->fgcolor);
}

/* Process an EllipseCB order */
static void
process_ellipse_cb(rdpOrders * orders, STREAM s, ELLIPSE_CB_ORDER * os, uint32 present, RD_BOOL delta)
{
	RD_BRUSH brush;

	if (present & 0x0001)
		rdp_in_coord(s, &os->left, delta);

	if (present & 0x0002)
		rdp_in_coord(s, &os->top, delta);

	if (present & 0x0004)
		rdp_in_coord(s, &os->right, delta);

	if (present & 0x0008)
		rdp_in_coord(s, &os->bottom, delta);

	if (present & 0x0010)
		in_uint8(s, os->opcode);

	if (present & 0x0020)
		in_uint8(s, os->fillmode);

	if (present & 0x0040)
		rdp_in_color(s, &os->bgcolor);

	if (present & 0x0080)
		rdp_in_color(s, &os->fgcolor);

	rdp_parse_brush(s, &os->brush, present >> 8);

	DEBUG_ORDERS("ELLIPSE_CB(l=%d,t=%d,r=%d,b=%d,op=0x%x,fm=%d,bs=%d,bg=0x%x,fg=0x%x)",
	       os->left, os->top, os->right, os->bottom, os->opcode, os->fillmode, os->brush.style,
	       os->bgcolor, os->fgcolor);

	setup_brush(orders, &brush, &os->brush);

	ui_ellipse(orders->rdp->inst, os->opcode, os->fillmode, os->left, os->top,
		   os->right - os->left, os->bottom - os->top, &brush, os->bgcolor, os->fgcolor);
}

static void
do_glyph(rdpOrders * orders, uint8 * ttext, int * index, int * x, int * y, uint8 flags, uint8 font)
{
	int xyoffset, lindex = *index, lx = *x, ly = *y, gx, gy;
	FONTGLYPH * glyph;

	glyph = cache_get_font(orders->rdp->cache, font, ttext[lindex]);
	if (!(flags & TEXT2_IMPLICIT_X))
	{
		xyoffset = ttext[++lindex];
		if ((xyoffset & 0x80))
		{
			if (flags & TEXT2_VERTICAL)
				ly += ttext[lindex + 1] | (ttext[lindex + 2] << 8);
			else
				lx += ttext[lindex + 1] | (ttext[lindex + 2] << 8);
			lindex += 2;
		}
		else
		{
			if (flags & TEXT2_VERTICAL)
				ly += xyoffset;
			else
				lx += xyoffset;
		}
	}
	if (glyph != NULL)
	{
		gx = lx + glyph->offset;
		gy = ly + glyph->baseline;
		ui_draw_glyph(orders->rdp->inst, gx, gy, glyph->width, glyph->height,
			      glyph->pixmap);
		if (flags & TEXT2_IMPLICIT_X)
			lx += glyph->width;
	}
	*index = lindex;
	*x = lx;
	*y = ly;
}

static void
draw_text(rdpOrders * orders, uint8 font, uint8 flags, uint8 opcode, int mixmode,
	  int x, int y, int clipx, int clipy, int clipcx, int clipcy,
	  int boxx, int boxy, int boxcx, int boxcy, RD_BRUSH * brush,
	  uint32 bgcolor, uint32 fgcolor, uint8 * text, uint8 length)
{
	/* TODO: use brush appropriately */

	DATABLOB * entry;
	int i, j;
	uint8 * btext;

	/* Sometimes, the boxcx value is something really large, like
	   32691. This makes XCopyArea fail with Xvnc. The code below
	   is a quick fix. */
	if (boxx + boxcx > orders->rdp->settings->width)
		boxcx = orders->rdp->settings->width - boxx;

	if (boxcx > 1)
	{
		ui_rect(orders->rdp->inst, boxx, boxy, boxcx, boxcy, bgcolor);
	}
	else if (mixmode == MIX_OPAQUE)
	{
		ui_rect(orders->rdp->inst, clipx, clipy, clipcx, clipcy, bgcolor);
	}
	ui_start_draw_glyphs(orders->rdp->inst, bgcolor, fgcolor);
	/* Paint text, character by character */
	for (i = 0; i < length;)
	{
		switch (text[i])
		{
			case 0xff:
				/* At least two bytes needs to follow */
				if (i + 3 > length)
				{
					ui_warning(orders->rdp->inst, "Skipping short 0xff command:");
					for (j = 0; j < length; j++)
						printf("%02x ", text[j]);
					printf("\n");
					i = length = 0;
					break;
				}
				cache_put_text(orders->rdp->cache, text[i + 1], text, text[i + 2]);
				i += 3;
				length -= i;
				/* this will move pointer from start to first character after FF command */
				text = &(text[i]);
				i = 0;
				break;

			case 0xfe:
				/* At least one byte needs to follow */
				if (i + 2 > length)
				{
					ui_warning(orders->rdp->inst, "Skipping short 0xfe command:");
					for (j = 0; j < length; j++)
						printf("%02x ", text[j]);
					printf("\n");
					i = length = 0;
					break;
				}
				entry = cache_get_text(orders->rdp->cache, text[i + 1]);
				if (entry->data != NULL)
				{
					btext = (uint8 *) (entry->data);
					if ((btext[1] == 0) && (!(flags & TEXT2_IMPLICIT_X)) &&
					    (i + 2 < length))
					{
						if (flags & TEXT2_VERTICAL)
							y += text[i + 2];
						else
							x += text[i + 2];
					}
					for (j = 0; j < entry->length; j++)
						do_glyph(orders, btext, &j, &x, &y, flags, font);
				}
				if (i + 2 < length)
					i += 3;
				else
					i += 2;
				length -= i;
				/* this will move pointer from start to first character after FE command */
				text = &(text[i]);
				i = 0;
				break;

			default:
				do_glyph(orders, text, &i, &x, &y, flags, font);
				i++;
				break;
		}
	}
	if (boxcx > 1)
	{
		ui_end_draw_glyphs(orders->rdp->inst, boxx, boxy, boxcx, boxcy);
	}
	else
	{
		ui_end_draw_glyphs(orders->rdp->inst, clipx, clipy, clipcx, clipcy);
	}
}

/* Process a glyph index order */
static void
process_glyph_index(rdpOrders * orders, STREAM s, GLYPH_INDEX_ORDER * os, uint32 present, RD_BOOL delta)
{
	RD_BRUSH brush;

	if (present & 0x000001)
		in_uint8(s, os->font);

	if (present & 0x000002)
		in_uint8(s, os->flags);

	if (present & 0x000004)
		in_uint8(s, os->opcode);

	if (present & 0x000008)
		in_uint8(s, os->mixmode);

	if (present & 0x000010)
		rdp_in_color(s, &os->fgcolor);

	if (present & 0x000020)
		rdp_in_color(s, &os->bgcolor);

	if (present & 0x000040)
		in_uint16_le(s, os->clipleft);

	if (present & 0x000080)
		in_uint16_le(s, os->cliptop);

	if (present & 0x000100)
		in_uint16_le(s, os->clipright);

	if (present & 0x000200)
		in_uint16_le(s, os->clipbottom);

	if (present & 0x000400)
		in_uint16_le(s, os->boxleft);

	if (present & 0x000800)
		in_uint16_le(s, os->boxtop);

	if (present & 0x001000)
		in_uint16_le(s, os->boxright);

	if (present & 0x002000)
		in_uint16_le(s, os->boxbottom);

	rdp_parse_brush(s, &os->brush, present >> 14);

	if (present & 0x080000)
		in_uint16_le(s, os->x);

	if (present & 0x100000)
		in_uint16_le(s, os->y);

	if (present & 0x200000)
	{
		in_uint8(s, os->length);
		in_uint8a(s, os->text, os->length);
	}

	DEBUG_ORDERS("GLYPH_INDEX(x=%d,y=%d,cl=%d,ct=%d,cr=%d,cb=%d,bl=%d,bt=%d,br=%d,bb=%d,bs=%d,bg=0x%x,fg=0x%x,font=%d,fl=0x%x,op=0x%x,mix=%d,n=%d)",
			os->x, os->y, os->clipleft, os->cliptop, os->clipright, os->clipbottom, os->boxleft, os->boxtop, os->boxright, os->boxbottom,
			os->brush.style, os->bgcolor, os->fgcolor, os->font, os->flags, os->opcode, os->mixmode, os->length);

	setup_brush(orders, &brush, &os->brush);

	draw_text(orders, os->font, os->flags, os->opcode, os->mixmode, os->x, os->y,
		  os->clipleft, os->cliptop, os->clipright - os->clipleft,
		  os->clipbottom - os->cliptop, os->boxleft, os->boxtop,
		  os->boxright - os->boxleft, os->boxbottom - os->boxtop,
		  &brush, os->bgcolor, os->fgcolor, os->text, os->length);
}

static void
process_fast_index(rdpOrders * orders, STREAM s, FAST_INDEX_ORDER * os, uint32 present, RD_BOOL delta)
{
	int x;
	int y;
	int clipx1;
	int clipy1;
	int clipx2;
	int clipy2;
	int boxx1;
	int boxy1;
	int boxx2;
	int boxy2;

	/* cacheId */
	if (present & 0x000001)
		in_uint8(s, os->font);
	/* fDrawing */
	if (present & 0x000002)
	{
		in_uint8(s, os->opcode);
		in_uint8(s, os->flags);
	}
	/* BackColor */
	if (present & 0x000004)
		rdp_in_color(s, &os->fgcolor);
	/* ForeColor */
	if (present & 0x000008)
		rdp_in_color(s, &os->bgcolor);
	/* BkLeft */
	if (present & 0x000010)
		rdp_in_coord(s, &os->clipleft, delta);
	/* BkTop */
	if (present & 0x000020)
		rdp_in_coord(s, &os->cliptop, delta);
	/* BkRight */
	if (present & 0x000040)
		rdp_in_coord(s, &os->clipright, delta);
	/* BkBottom */
	if (present & 0x000080)
		rdp_in_coord(s, &os->clipbottom, delta);
	/* OpLeft */
	if (present & 0x000100)
		rdp_in_coord(s, &os->boxleft, delta);
	/* OpTop */
	if (present & 0x000200)
		rdp_in_coord(s, &os->boxtop, delta);
	/* OpRight */
	if (present & 0x000400)
		rdp_in_coord(s, &os->boxright, delta);
	/* OpBottom */
	if (present & 0x000800)
		rdp_in_coord(s, &os->boxbottom, delta);
	/* x */
	if (present & 0x001000)
		rdp_in_coord(s, &os->x, delta);
	/* y */
	if (present & 0x002000)
		rdp_in_coord(s, &os->y, delta);
	/* VariableBytes */
	if (present & 0x004000)
	{
		in_uint8(s, os->length);
		in_uint8a(s, os->text, os->length);
	}

	DEBUG_ORDERS("FAST_INDEX(x=%d,y=%d,cl=%d,ct=%d,cr=%d,cb=%d,bl=%d,bt=%d,br=%d,bb=%d,bg=0x%x,fg=0x%x,font=%d,fl=0x%x,op=0x%x,n=%d)",
			os->x, os->y, os->clipleft, os->cliptop, os->clipright, os->clipbottom, os->boxleft, os->boxtop, os->boxright, os->boxbottom,
			os->bgcolor, os->fgcolor, os->font, os->flags, os->opcode, os->length);

	x = os->x == -32768 ? os->clipleft : os->x;
	y = os->y == -32768 ? os->cliptop : os->y;
	clipx1 = os->clipleft;
	clipy1 = os->cliptop;
	clipx2 = os->clipright;
	clipy2 = os->clipbottom;
	boxx1 = os->boxleft;
	boxy1 = os->boxtop;
	boxx2 = os->boxright;
	boxy2 = os->boxbottom;
	if (os->boxleft == 0)
	{
		boxx1 = os->clipleft;
	}
	if (os->boxright == 0)
	{
		boxx2 = os->clipright;
	}
	if (os->boxbottom == -32768)
	{
		if (os->boxtop & 0x01)
		{
			boxy2 = os->clipbottom;
		}
		if (os->boxtop & 0x02)
		{
			boxx2 = os->clipright;
		}
		if (os->boxtop & 0x04)
		{
			boxy1 = os->cliptop;
		}
		if (os->boxtop & 0x08)
		{
			boxx1 = os->clipleft;
		}
	}
	if (!((boxx2 > boxx1) && (boxy2 > boxy1)))
	{
		boxx1 = 0;
		boxy1 = 0;
		boxx2 = 0;
		boxy2 = 0;
	}
	draw_text(orders, os->font, os->flags, os->opcode,
		  MIX_TRANSPARENT, x, y,
		  clipx1, clipy1, clipx2 - clipx1, clipy2 - clipy1,
		  boxx1, boxy1, boxx2 - boxx1, boxy2 - boxy1,
		  &os->brush, os->bgcolor, os->fgcolor, os->text, os->length);
}

/* Process a fast glyph order */
static void
process_fast_glyph(rdpOrders * orders, STREAM s, FAST_GLYPH_ORDER * os, uint32 present, RD_BOOL delta)
{
	int x;
	int y;
	int clipx1;
	int clipy1;
	int clipx2;
	int clipy2;
	int clipcx;
	int clipcy;
	int boxx1;
	int boxy1;
	int boxx2;
	int boxy2;
	int boxcx;
	int boxcy;
	int character;
	int offset;
	int baseline;
	int width;
	int height;
	RD_HGLYPH gl;
	FONTGLYPH * ft;
	int gx;
	int gy;
	int index;

	if (present & 0x000001)
		in_uint8(s, os->font);
	if (present & 0x000002)
	{
		in_uint8(s, os->opcode);
		in_uint8(s, os->flags);
	}
	if (present & 0x000004)
		rdp_in_color(s, &os->fgcolor);
	if (present & 0x000008)
		rdp_in_color(s, &os->bgcolor);
	if (present & 0x000010)
		rdp_in_coord(s, &os->clipleft, delta);
	if (present & 0x000020)
		rdp_in_coord(s, &os->cliptop, delta);
	if (present & 0x000040)
		rdp_in_coord(s, &os->clipright, delta);
	if (present & 0x000080)
		rdp_in_coord(s, &os->clipbottom, delta);
	if (present & 0x000100)
		rdp_in_coord(s, &os->boxleft, delta);
	if (present & 0x000200)
		rdp_in_coord(s, &os->boxtop, delta);
	if (present & 0x000400)
		rdp_in_coord(s, &os->boxright, delta);
	if (present & 0x000800)
		rdp_in_coord(s, &os->boxbottom, delta);
	if (present & 0x001000)
		rdp_in_coord(s, &os->x, delta);
	if (present & 0x002000)
		rdp_in_coord(s, &os->y, delta);
	if (present & 0x004000)
	{
		in_uint8(s, os->length);
		in_uint8a(s, os->data, os->length);
	}

	DEBUG_ORDERS("FAST_GLYPH(x=%d,y=%d,cl=%d,ct=%d,cr=%d,cb=%d,bl=%d,bt=%d,br=%d,bb=%d,bg=0x%x,fg=0x%x,font=%d,fl=0x%x,op=0x%x,n=%d)",
			os->x, os->y, os->clipleft, os->cliptop, os->clipright, os->clipbottom, os->boxleft, os->boxtop, os->boxright, os->boxbottom,
			os->bgcolor, os->fgcolor, os->font, os->flags, os->opcode, os->length);

	x = os->x == -32768 ? os->clipleft : os->x;
	y = os->y == -32768 ? os->cliptop : os->y;
	clipx1 = os->clipleft;
	clipy1 = os->cliptop;
	clipx2 = os->clipright;
	clipy2 = os->clipbottom;
	boxx1 = os->boxleft;
	boxy1 = os->boxtop;
	boxx2 = os->boxright;
	boxy2 = os->boxbottom;
	if (os->boxleft == 0)
	{
		boxx1 = os->clipleft;
	}
	if (os->boxright == 0)
	{
		boxx2 = os->clipright;
	}
	if (os->boxbottom == -32768)
	{
		if (os->boxtop & 0x01)
		{
			boxy2 = os->clipbottom;
		}
		if (os->boxtop & 0x02)
		{
			boxx2 = os->clipright;
		}
		if (os->boxtop & 0x04)
		{
			boxy1 = os->cliptop;
		}
		if (os->boxtop & 0x08)
		{
			boxx1 = os->clipleft;
		}
	}
	if (!((boxx2 > boxx1) && (boxy2 > boxy1)))
	{
		boxx1 = 0;
		boxy1 = 0;
		boxx2 = 0;
		boxy2 = 0;
	}
	clipcx = clipx2 - clipx1;
	clipcy = clipy2 - clipy1;
	boxcx = boxx2 - boxx1;
	boxcy = boxy2 - boxy1;
	if (os->length < 2)
	{
		character = os->data[0];
	}
	else
	{
		character = os->data[0];
		index = 1;
		offset = parse_s2byte(os->data, &index);
		baseline = parse_s2byte(os->data, &index);
		width = parse_u2byte(os->data, &index);
		height = parse_u2byte(os->data, &index);
		gl = ui_create_glyph(orders->rdp->inst, width, height, os->data + index);
		cache_put_font(orders->rdp->cache, os->font, character, offset, baseline, width, height, gl);
	}
	ft = cache_get_font(orders->rdp->cache, os->font, character);
	if (ft != NULL)
	{
		gx = x + ft->offset;
		gy = y + ft->baseline;
		if (boxcx > 1)
		{
			ui_rect(orders->rdp->inst, boxx1, boxy1, boxcx, boxcy, os->bgcolor);
		}
		ui_start_draw_glyphs(orders->rdp->inst, os->bgcolor, os->fgcolor);
		ui_draw_glyph(orders->rdp->inst, gx, gy, ft->width, ft->height, ft->pixmap);
		if (boxcx > 1)
		{
			ui_end_draw_glyphs(orders->rdp->inst, boxx1, boxy1, boxcx, boxcy);
		}
		else
		{
			ui_end_draw_glyphs(orders->rdp->inst, clipx1, clipy1, clipcx, clipcy);
		}
	}
}

/* Process a raw bitmap cache order */
static void
process_cache_bitmap_uncompressed(rdpOrders * orders, STREAM s)
{
	size_t size;
	RD_HBITMAP bitmap;
	uint16 cache_idx, bufsize;
	uint8 cache_id, width, height, bpp, Bpp;
	uint8 *data, *inverted;
	int y;

	in_uint8(s, cache_id);
	in_uint8s(s, 1);	/* pad */
	in_uint8(s, width);
	in_uint8(s, height);
	in_uint8(s, bpp);
	Bpp = (bpp + 7) / 8;
	in_uint16_le(s, bufsize);
	in_uint16_le(s, cache_idx);
	in_uint8p(s, data, bufsize);

	DEBUG_ORDERS("RAWBMPCACHE(cx=%d,cy=%d,id=%d,idx=%d)", width, height, cache_id, cache_idx);

	size = width * height * Bpp;

	if (size > orders->buffer_size)
	{
		orders->buffer = xrealloc(orders->buffer, size);
		orders->buffer_size = size;
	}
	inverted = (uint8 *) orders->buffer;

	for (y = 0; y < height; y++)
	{
		memcpy(&inverted[(height - y - 1) * (width * Bpp)], &data[y * (width * Bpp)],
		       width * Bpp);
	}

	bitmap = ui_create_bitmap(orders->rdp->inst, width, height, inverted);
	cache_put_bitmap(orders->rdp->cache, cache_id, cache_idx, bitmap);
}

/* Process a bitmap cache order */
static void
process_cache_bitmap_compressed(rdpOrders * orders, STREAM s, uint16 flags)
{
	size_t buffer_size;
	RD_HBITMAP bitmap;
	uint16 cache_idx, size;
	uint8 cache_id, width, height, bpp, Bpp;
	uint8 *data, *bmpdata;
	uint16 bufsize, pad2, row_size, final_size;
	uint8 pad1;

	pad2 = row_size = final_size = 0xffff;	/* Shut the compiler up */

	in_uint8(s, cache_id);
	in_uint8(s, pad1);	/* pad */
	in_uint8(s, width);
	in_uint8(s, height);
	in_uint8(s, bpp);
	Bpp = (bpp + 7) / 8;
	in_uint16_le(s, bufsize);	/* bufsize */
	in_uint16_le(s, cache_idx);

	if (flags & 0x0400)
	{
		size = bufsize;
	}
	else
	{

		/* Begin compressedBitmapData */
		in_uint16_le(s, pad2);	/* pad */
		in_uint16_le(s, size);
		/*      in_uint8s(s, 4);  *//* row_size, final_size */
		in_uint16_le(s, row_size);
		in_uint16_le(s, final_size);

	}
	in_uint8p(s, data, size);

	DEBUG_ORDERS("BMPCACHE(cx=%d,cy=%d,id=%d,idx=%d,bpp=%d,size=%d,pad1=%d,bufsize=%d,pad2=%d,rs=%d,fs=%d)",
			width, height, cache_id, cache_idx, bpp, size, pad1, bufsize, pad2, row_size, final_size);

	buffer_size = width * height * Bpp;

	if (buffer_size > orders->buffer_size)
	{
		orders->buffer = xrealloc(orders->buffer, buffer_size);
		orders->buffer_size = buffer_size;
	}

	bmpdata = (uint8 *) orders->buffer;

	if (bitmap_decompress(orders->rdp->inst, bmpdata, width, height, data, size, Bpp))
	{
		bitmap = ui_create_bitmap(orders->rdp->inst, width, height, bmpdata);
		cache_put_bitmap(orders->rdp->cache, cache_id, cache_idx, bitmap);
	}
	else
	{
		DEBUG_ORDERS("Failed to decompress bitmap data");
	}
}

/* Process a bitmap cache v2 order */
static void
process_cache_bitmap_rev2(rdpOrders * orders, STREAM s, uint16 flags, RD_BOOL compressed)
{
	int y;
	size_t size;
	RD_HBITMAP bitmap;
	uint8 cache_id, cache_idx_low, width, height, Bpp;
	uint16 cache_idx, bufsize;
	uint8 *data, *bmpdata, *bitmap_id;

	bitmap_id = NULL;	/* prevent compiler warning */
	cache_id = flags & ID_MASK;
	Bpp = ((flags & MODE_MASK) >> MODE_SHIFT) - 2;

	if (flags & PERSIST)
	{
		in_uint8p(s, bitmap_id, 8);
	}

	if (flags & SQUARE)
	{
		in_uint8(s, width);
		height = width;
	}
	else
	{
		in_uint8(s, width);
		in_uint8(s, height);
	}

	in_uint16_be(s, bufsize);
	bufsize &= BUFSIZE_MASK;
	in_uint8(s, cache_idx);

	if (cache_idx & LONG_FORMAT)
	{
		in_uint8(s, cache_idx_low);
		cache_idx = ((cache_idx ^ LONG_FORMAT) << 8) + cache_idx_low;
	}

	in_uint8p(s, data, bufsize);

	DEBUG_ORDERS("BMPCACHE2(compr=%d,flags=%x,cx=%d,cy=%d,id=%d,idx=%d,Bpp=%d,bs=%d)",
	       compressed, flags, width, height, cache_id, cache_idx, Bpp, bufsize);

	size = width * height * Bpp;

	if (size > orders->buffer_size)
	{
		orders->buffer = xrealloc(orders->buffer, size);
		orders->buffer_size = size;
	}

	bmpdata = (uint8 *) orders->buffer;

	if (compressed)
	{
		if (!bitmap_decompress(orders->rdp->inst, bmpdata, width, height, data, bufsize, Bpp))
		{
			DEBUG_ORDERS("Failed to decompress bitmap data");
			xfree(bmpdata);
			return;
		}
	}
	else
	{
		for (y = 0; y < height; y++)
			memcpy(&bmpdata[(height - y - 1) * (width * Bpp)],
			       &data[y * (width * Bpp)], width * Bpp);
	}

	bitmap = ui_create_bitmap(orders->rdp->inst, width, height, bmpdata);

	if (bitmap)
	{
		cache_put_bitmap(orders->rdp->cache, cache_id, cache_idx, bitmap);
		if (flags & PERSIST)
			pstcache_save_bitmap(orders->rdp->pcache, cache_id, cache_idx, bitmap_id,
					     width, height, width * height * Bpp, bmpdata);
	}
	else
	{
		DEBUG_ORDERS("process_cache_bitmap_rev2: ui_create_bitmap failed");
	}
}

/* Process a colormap cache order */
static void
process_cache_color_table(rdpOrders * orders, STREAM s)
{
	int i;
	size_t size;
	uint8 cacheIndex;
	RD_PALETTE palette;
	RD_HPALETTE hpalette;
	RD_PALETTEENTRY *entry;

	in_uint8(s, cacheIndex); /* cacheIndex */
	in_uint16_le(s, palette.count); /* numberColors */

	size = sizeof(RD_PALETTEENTRY) * palette.count;

	if (size > orders->buffer_size)
	{
		orders->buffer = xrealloc(orders->buffer, size);
		orders->buffer_size = size;
	}

	palette.entries = (RD_PALETTEENTRY *) orders->buffer;

	for (i = 0; i < palette.count; i++)
	{
		entry = &palette.entries[i];
		in_uint8(s, entry->blue);
		in_uint8(s, entry->green);
		in_uint8(s, entry->red);
		in_uint8s(s, 1);	/* pad */
	}

	DEBUG_ORDERS("COLORTABLECACHE(id=%d,n=%d)", cacheIndex, palette.count);

	if (cacheIndex)
	{
		hpalette = ui_create_palette(orders->rdp->inst, &palette);
		ui_set_palette(orders->rdp->inst, hpalette);
	}
}

/* Process a font cache order */
static void
process_cache_glyph(rdpOrders * orders, STREAM s)
{
	RD_HGLYPH bitmap;
	uint8 font, nglyphs;
	uint16 character, offset, baseline, width, height;
	int i, datasize;
	uint8 *data;

	in_uint8(s, font);
	in_uint8(s, nglyphs);

	DEBUG_ORDERS("GLYPHCACHE(font=%d,n=%d)", font, nglyphs);

	for (i = 0; i < nglyphs; i++)
	{
		in_uint16_le(s, character);
		in_uint16_le(s, offset);
		in_uint16_le(s, baseline);
		in_uint16_le(s, width);
		in_uint16_le(s, height);

		datasize = (height * ((width + 7) / 8) + 3) & ~3;
		in_uint8p(s, data, datasize);

		bitmap = ui_create_glyph(orders->rdp->inst, width, height, data);
		cache_put_font(orders->rdp->cache, font, character, offset, baseline, width,
			       height, bitmap);
	}
}

static void
process_compressed_8x8_brush_data(uint8 * in, uint8 * out, int Bpp)
{
	int x, y, pal_index, in_index, shift, do2, i;
	uint8 *pal;

	in_index = 0;
	pal = in + 16;
	/* read it bottom up */
	for (y = 7; y >= 0; y--)
	{
		/* 2 bytes per row */
		x = 0;
		for (do2 = 0; do2 < 2; do2++)
		{
			/* 4 pixels per byte */
			shift = 6;
			while (shift >= 0)
			{
				pal_index = (in[in_index] >> shift) & 3;
				/* size of palette entries depends on Bpp */
				for (i = 0; i < Bpp; i++)
				{
					out[(y * 8 + x) * Bpp + i] = pal[pal_index * Bpp + i];
				}
				x++;
				shift -= 2;
			}
			in_index++;
		}
	}
}

/* Process a brush cache order */
static void
process_cache_brush(rdpOrders * orders, STREAM s, uint16 flags)
{
	RD_BRUSHDATA brush_data;
	uint8 cache_idx, color_code, width, height, size, type;
	uint8 *comp_brush;
	int index;
	int Bpp;

	in_uint8(s, cache_idx);
	in_uint8(s, color_code);
	in_uint8(s, width);
	in_uint8(s, height);
	in_uint8(s, type);	/* type, 0x8x = cached */
	in_uint8(s, size);

	DEBUG_ORDERS("BRUSHCACHE(idx=%d,dp=%d,wd=%d,ht=%d,sz=%d)", cache_idx, color_code,
	       width, height, size);

	if ((width == 8) && (height == 8))
	{
		if (color_code == 1)
		{
			brush_data.color_code = 1;
			brush_data.data_size = 8;
			brush_data.data = (uint8 *) xmalloc(8);
			if (size == 8)
			{
				/* read it bottom up */
				for (index = 7; index >= 0; index--)
				{
					in_uint8(s, brush_data.data[index]);
				}
			}
			else
			{
				ui_warning(orders->rdp->inst, "incompatible brush, "
					   "color_code %d size %d\n", color_code, size);
			}
			cache_put_brush_data(orders->rdp->cache, 1, cache_idx, &brush_data);
		}
		else if ((color_code >= 3) && (color_code <= 6))
		{
			Bpp = color_code - 2;
			brush_data.color_code = color_code;
			brush_data.data_size = 8 * 8 * Bpp;
			brush_data.data = (uint8 *) xmalloc(8 * 8 * Bpp);
			if (size == 16 + 4 * Bpp)
			{
				in_uint8p(s, comp_brush, 16 + 4 * Bpp);
				process_compressed_8x8_brush_data(comp_brush, brush_data.data, Bpp);
			}
			else
			{
				in_uint8a(s, brush_data.data, 8 * 8 * Bpp);
			}
			cache_put_brush_data(orders->rdp->cache, color_code, cache_idx, &brush_data);
		}
		else
		{
			ui_warning(orders->rdp->inst, "incompatible brush, color_code %d "
				   "size %d\n", color_code, size);
		}
	}
	else
	{
		ui_warning(orders->rdp->inst, "incompatible brush, width height %d %d\n",
			   width, height);
	}
}

/* Process a switch surface alternate secondary drawing order */
static void
process_switch_surface(rdpOrders * orders, STREAM s)
{
	sint16 idx;

	in_uint16_le(s, idx);
	ui_set_surface(orders->rdp->inst, idx >= 0 ?
		cache_get_bitmap(orders->rdp->cache, 255, idx) : NULL);
}

/* Process a create off-screen bitmap alternate secondary drawing order */
static void
process_create_offscr_bitmap(rdpOrders * orders, STREAM s)
{
	RD_HBITMAP bitmap;
	uint16 idx, width, height, free_num, free_idx;
	int i;

	in_uint16_le(s, idx);
	in_uint16_le(s, width);
	in_uint16_le(s, height);
	if (idx & 0x8000)
	{
		in_uint16_le(s, free_num);
		for (i = 0; i < free_num; i++)
		{
			in_uint16_le(s, free_idx);
			bitmap = cache_get_bitmap(orders->rdp->cache, 255, free_idx);
			ui_destroy_surface(orders->rdp->inst, bitmap);
			cache_put_bitmap(orders->rdp->cache, 255, free_idx, NULL);
		}
	}
	idx &= ~0x8000;
	bitmap = cache_get_bitmap(orders->rdp->cache, 255, idx);
	bitmap = ui_create_surface(orders->rdp->inst, width, height, bitmap);
	cache_put_bitmap(orders->rdp->cache, 255, idx, bitmap);
}

/* Process a non-standard order */
static int
process_alternate_secondary_order(rdpOrders * orders, STREAM s, uint8 order_flags)
{
	if (!(order_flags & 0x2))
	{
		perror("order parsing failed\n");
		return 1;
	}
	order_flags >>= 2;
	switch (order_flags)
	{
		case RDP_ORDER_ALTSEC_SWITCH_SURFACE:
			process_switch_surface(orders, s);
			break;
		case RDP_ORDER_ALTSEC_CREATE_OFFSCR_BITMAP:
			process_create_offscr_bitmap(orders, s);
			break;
		default:
			ui_unimpl(orders->rdp->inst, "alternate secondary order %d:\n", order_flags);
			return 1;
	}
	return 0;
}

/* Process a secondary order */
static void
process_secondary_order(rdpOrders * orders, STREAM s)
{
	/* The length isn't calculated correctly by the server.
	 * For very compact orders the length becomes negative
	 * so a signed integer must be used. */
	
	uint16 length;
	uint16 flags;
	uint8 type;
	uint8 *next_order;

	in_uint16_le(s, length);
	in_uint16_le(s, flags);	/* used by RDP_ORDER_CACHE_BITMAP_COMPRESSED_REV2 */
	in_uint8(s, type);

	next_order = s->p + ((sint16) length) + 7;

	switch (type)
	{
		case RDP_ORDER_CACHE_BITMAP_UNCOMPRESSED:
			process_cache_bitmap_uncompressed(orders, s);
			break;

		case RDP_ORDER_CACHE_COLOR_TABLE:
			process_cache_color_table(orders, s);
			break;

		case RDP_ORDER_CACHE_BITMAP_COMPRESSED:
			process_cache_bitmap_compressed(orders, s, flags);
			break;

		case RDP_ORDER_CACHE_GLYPH:
			process_cache_glyph(orders, s);
			break;

		case RDP_ORDER_CACHE_BITMAP_UNCOMPRESSED_REV2:
			process_cache_bitmap_rev2(orders, s, flags, False);
			break;

		case RDP_ORDER_CACHE_BITMAP_COMPRESSED_REV2:
			process_cache_bitmap_rev2(orders, s, flags, True);
			break;

		case RDP_ORDER_CACHE_BRUSH:
			process_cache_brush(orders, s, flags);
			break;

		default:
			ui_unimpl(orders->rdp->inst, "secondary order %d\n", type);
	}

	s->p = next_order;
}

/* Process an order PDU */
void
process_orders(rdpOrders * orders, STREAM s, uint16 num_orders)
{
	RDP_ORDER_STATE * os = (RDP_ORDER_STATE *) (orders->order_state);
	uint32 present;
	uint8 order_flags;
	int size, processed = 0;
	RD_BOOL delta;

	while (processed < num_orders)
	{
		in_uint8(s, order_flags);

		if (!(order_flags & RDP_ORDER_CTL_STANDARD))
		{
			process_alternate_secondary_order(orders, s, order_flags);
		}
		else if (order_flags & RDP_ORDER_CTL_SECONDARY)
		{
			process_secondary_order(orders, s);
		}
		else
		{
			if (order_flags & RDP_ORDER_CTL_TYPE_CHANGE)
			{
				in_uint8(s, os->order_type);
			}

			switch (os->order_type)
			{
				case RDP_ORDER_MEM3BLT:
				case RDP_ORDER_GLYPH_INDEX:
					size = 3;
					break;

				case RDP_ORDER_PATBLT:
				case RDP_ORDER_MULTIPATBLT:
				case RDP_ORDER_MEMBLT:
				case RDP_ORDER_LINETO:
				case RDP_ORDER_POLYGON_CB:
				case RDP_ORDER_FAST_INDEX:
				case RDP_ORDER_ELLIPSE_CB:
				case RDP_ORDER_FAST_GLYPH:
				case RDP_ORDER_MULTIOPAQUERECT:
					size = 2;
					break;

				default:
					size = 1;
			}

			rdp_in_present(s, &present, order_flags, size);

			if (order_flags & RDP_ORDER_CTL_BOUNDS)
			{
				if (!(order_flags & RDP_ORDER_CTL_ZERO_BOUNDS_DELTA))
					rdp_parse_bounds(s, &os->bounds);

				ui_set_clip(orders->rdp->inst, os->bounds.left,
					    os->bounds.top,
					    os->bounds.right -
					    os->bounds.left + 1,
					    os->bounds.bottom - os->bounds.top + 1);
			}

			delta = order_flags & RDP_ORDER_CTL_DELTA_COORDINATES;

			switch (os->order_type)
			{
				case RDP_ORDER_DSTBLT:
					process_dstblt(orders, s, &os->dstblt, present, delta);
					break;

				case RDP_ORDER_PATBLT:
					process_patblt(orders, s, &os->patblt, present, delta);
					break;

				case RDP_ORDER_MULTIPATBLT:
					process_multipatblt(orders, s, &os->multipatblt, present, delta);
					break;

				case RDP_ORDER_SCRBLT:
					process_scrblt(orders, s, &os->scrblt, present, delta);
					break;

				case RDP_ORDER_LINETO:
					process_lineto(orders, s, &os->lineto, present, delta);
					break;

				case RDP_ORDER_OPAQUERECT:
					process_opaquerect(orders, s, &os->opaquerect, present, delta);
					break;

				case RDP_ORDER_MULTIOPAQUERECT:
					process_multiopaquerect(orders, s, &os->multiopaquerect, present, delta);
					break;

				case RDP_ORDER_SAVEBITMAP:
					process_savebitmap(orders, s, &os->savebitmap, present, delta);
					break;

				case RDP_ORDER_MEMBLT:
					process_memblt(orders, s, &os->memblt, present, delta);
					break;

				case RDP_ORDER_MEM3BLT:
					process_mem3blt(orders, s, &os->mem3blt, present, delta);
					break;

				case RDP_ORDER_POLYGON_SC:
					process_polygon_sc(orders, s, &os->polygon_sc, present, delta);
					break;

				case RDP_ORDER_POLYGON_CB:
					process_polygon_cb(orders, s, &os->polygon_cb, present, delta);
					break;

				case RDP_ORDER_POLYLINE:
					process_polyline(orders, s, &os->polyline, present, delta);
					break;

				case RDP_ORDER_ELLIPSE_SC:
					process_ellipse_sc(orders, s, &os->ellipse_sc, present, delta);
					break;

				case RDP_ORDER_ELLIPSE_CB:
					process_ellipse_cb(orders, s, &os->ellipse_cb, present, delta);
					break;

				case RDP_ORDER_GLYPH_INDEX:
					process_glyph_index(orders, s, &os->glyph_index, present, delta);
					break;

				case RDP_ORDER_FAST_INDEX:
					process_fast_index(orders, s, &os->fast_index, present, delta);
					break;

				case RDP_ORDER_FAST_GLYPH:
					process_fast_glyph(orders, s, &os->fast_glyph, present, delta);
					break;

				default:
					ui_unimpl(orders->rdp->inst, "order %d\n", os->order_type);
					return;
			}

			if (order_flags & RDP_ORDER_CTL_BOUNDS)
				ui_reset_clip(orders->rdp->inst);
		}

		processed++;
	}
}

/* Reset order state */
void
reset_order_state(rdpOrders * orders)
{
	RDP_ORDER_STATE * os = (RDP_ORDER_STATE *) (orders->order_state);

	memset(os, 0, sizeof(RDP_ORDER_STATE));
	os->order_type = RDP_ORDER_PATBLT;
	ui_set_surface(orders->rdp->inst, NULL);
}

rdpOrders *
orders_new(struct rdp_rdp * rdp)
{
	rdpOrders * self;

	self = (rdpOrders *) xmalloc(sizeof(rdpOrders));
	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpOrders));
		self->rdp = rdp;
		/* orders_state is void * */
		self->order_state = xmalloc(sizeof(RDP_ORDER_STATE));
		memset(self->order_state, 0, sizeof(RDP_ORDER_STATE));
		/* reusable buffer */
		self->buffer_size = 4096;
		self->buffer = xmalloc(self->buffer_size);
		memset(self->buffer, 0, self->buffer_size);
	}
	return self;
}

void
orders_free(rdpOrders * orders)
{
	if (orders != NULL)
	{
		xfree(orders->order_state);
		xfree(orders->buffer);
		xfree(orders);
	}
}
