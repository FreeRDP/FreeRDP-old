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
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>
#include <freerdp/chanman.h>
#include "dfb_win.h"
#include "dfb_event.h"
#include "dfb_colour.h"
#include "dfb_keyboard.h"

static void
l_ui_error(struct rdp_inst * inst, char * text)
{
	printf("ui_error: %s", text);
}

static void
l_ui_warning(struct rdp_inst * inst, char * text)
{
	printf("ui_warning: %s\n", text);
}

static void
l_ui_unimpl(struct rdp_inst * inst, char * text)
{
	printf("ui_unimpl: %s\n", text);
}

static void
dfb_invalidate_rect(dfbInfo * dfbi, int x1, int y1, int x2, int y2)
{
	if (dfbi->update_pending)
	{
		int right = dfbi->update_rect.x + dfbi->update_rect.w;
		int bottom = dfbi->update_rect.y + dfbi->update_rect.h;
		
		if (x1 < dfbi->update_rect.x)
			dfbi->update_rect.x = x1;

		if (y1 < dfbi->update_rect.y)
			dfbi->update_rect.y = y1;

		if (x2 > right)
			right = x2;

		if (y2 > bottom)
			bottom = y2;

		dfbi->update_rect.w = right - dfbi->update_rect.x;
		dfbi->update_rect.h = bottom - dfbi->update_rect.y;
	}
	else
	{
		dfbi->update_rect.x = 0;
		dfbi->update_rect.y = 0;
		dfbi->update_rect.w = dfbi->width;
		dfbi->update_rect.h = dfbi->height;
		dfbi->update_pending = 1;
	}
}

static char *
dfb_get_screen_pointer(dfbInfo * dfbi, int x, int y)
{
	char * p;

	if (x >= 0 && x < dfbi->width && y >= 0 && y < dfbi->height)
	{
		p = dfbi->screen + (y * dfbi->width * dfbi->bytes_per_pixel) + (x * dfbi->bytes_per_pixel);
		return p;
	}
	else
	{
		return 0;
	}
}

static void
dfb_copy_mem(char * d, char * s, int n)
{
	while (n & (~7))
	{
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		n = n - 8;
	}
	while (n > 0)
	{
		*(d++) = *(s++);
		n--;
	}
}

static void
l_ui_begin_update(struct rdp_inst * inst)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	
	dfbi->primary->Blit(dfbi->primary, dfbi->screen_surface,
	                    &(dfbi->update_rect), dfbi->update_rect.x, dfbi->update_rect.y);
}

static void
l_ui_end_update(struct rdp_inst * inst)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);	
	dfbi->update_pending = 0;
}

static void
l_ui_desktop_save(struct rdp_inst * inst, int offset, int x, int y, int cx, int cy)
{
}

static void
l_ui_desktop_restore(struct rdp_inst * inst, int offset, int x, int y, int cx, int cy)
{
}

static RD_HGLYPH
l_ui_create_glyph(struct rdp_inst * inst, int width, int height, uint8 * data)
{
	return (RD_HGLYPH) NULL;
}

static void
l_ui_destroy_glyph(struct rdp_inst * inst, RD_HGLYPH glyph)
{
}

static RD_HBITMAP
l_ui_create_bitmap(struct rdp_inst * inst, int width, int height, uint8 * data)
{
	uint8 * cdata;
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	cdata = dfb_image_convert(dfbi, inst->settings, width, height, data);
	return (RD_HBITMAP) cdata;
}

static void
l_ui_paint_bitmap(struct rdp_inst * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
	dfbInfo * dfbi;
	uint8 * cdata;
	DFBRectangle rect;
	dfbi = GET_DFBI(inst);
	
	rect.x = x;
	rect.y = y;
	rect.w = cx;
	rect.h = cy;

	cdata = dfb_image_convert(dfbi, inst->settings, width, height, data);
	inst->ui_memblt(inst, 12, x, y, cx, cy, cdata, 0, 0);
	dfb_invalidate_rect(dfbi, 0, 0, dfbi->width, dfbi->height);
}

static void
l_ui_destroy_bitmap(struct rdp_inst * inst, RD_HBITMAP bmp)
{
	if (bmp)
		free(bmp);
}

static void
l_ui_line(struct rdp_inst * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen)
{
}

static void
l_ui_rect(struct rdp_inst * inst, int x, int y, int cx, int cy, int colour)
{	
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	dfb_colour_convert(dfbi, colour, &(dfbi->pixel), inst->settings->server_depth, dfbi->bpp);
	dfbi->primary->SetColor(dfbi->primary, dfbi->pixel.red, dfbi->pixel.green, dfbi->pixel.blue, dfbi->pixel.alpha);
	dfbi->primary->FillRectangle(dfbi->primary, x, y, cx, cy);
}

static void
l_ui_polygon(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point,
	int npoints, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
}

static void
l_ui_polyline(struct rdp_inst * inst, uint8 opcode, RD_POINT * points, int npoints, RD_PEN * pen)
{
}

static void
l_ui_ellipse(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, int x, int y,
	int cx, int cy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
}

static void
l_ui_start_draw_glyphs(struct rdp_inst * inst, int bgcolour, int fgcolour)
{
}

static void
l_ui_draw_glyph(struct rdp_inst * inst, int x, int y, int cx, int cy, RD_HGLYPH glyph)
{
}

static void
l_ui_end_draw_glyphs(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
}

static uint32
l_ui_get_toggle_keys_state(struct rdp_inst * inst)
{
	return 0;
}

static void
l_ui_bell(struct rdp_inst * inst)
{
}

static void
l_ui_destblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy)
{
}

static void
l_ui_patblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
}

static void
l_ui_screenblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy)
{
}

static void
l_ui_memblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy)
{
	int i;
	int rv = 0;
	char * dst;
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	for (i = 0; i < cy; i++)
	{
		dst = dfb_get_screen_pointer(dfbi, x, y + i);
		if (dst != 0)
		{
			if (!rv)
			{
				if (memcmp(dst, src, cx * dfbi->bytes_per_pixel) != 0)
					rv = 1;
			}
			dfb_copy_mem(dst, src, cx * dfbi->bytes_per_pixel);
			src += srcx * dfbi->bytes_per_pixel;
		}
	}
}

static void
l_ui_triblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
}

static int
l_ui_select(struct rdp_inst * inst, int rdp_socket)
{
	return 1;
}

static void
l_ui_set_clip(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
	dfbInfo * dfbi;	
	dfbi = GET_DFBI(inst);
	
	dfbi->region.x1 = x;
	dfbi->region.y1 = y;
	dfbi->region.x2 = cx;
	dfbi->region.y2 = cy;

	dfbi->primary->SetClip(dfbi->primary, &(dfbi->region));
}

static void
l_ui_reset_clip(struct rdp_inst * inst)
{
	dfbInfo * dfbi;	
	dfbi = GET_DFBI(inst);
	
	dfbi->region.x1 = 0;
	dfbi->region.y1 = 0;
	dfbi->region.x2 = dfbi->width;
	dfbi->region.y2 = dfbi->height;

	dfbi->primary->SetClip(dfbi->primary, &(dfbi->region));
}

static void
l_ui_resize_window(struct rdp_inst * inst)
{
}

static void
l_ui_set_cursor(struct rdp_inst * inst, RD_HCURSOR cursor)
{
}

static void
l_ui_destroy_cursor(struct rdp_inst * inst, RD_HCURSOR cursor)
{
}

static RD_HCURSOR
l_ui_create_cursor(struct rdp_inst * inst, uint32 x, uint32 y,
	int width, int height, uint8 * andmask, uint8 * xormask, int bpp)
{
	return (RD_HCURSOR) NULL;
}

static void
l_ui_set_null_cursor(struct rdp_inst * inst)
{
}

static void
l_ui_set_default_cursor(struct rdp_inst * inst)
{
}

static void
l_ui_move_pointer(struct rdp_inst * inst, int x, int y)
{
}

static RD_HCOLOURMAP
l_ui_create_colourmap(struct rdp_inst * inst, RD_COLOURMAP * colours)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	return dfb_create_colourmap(dfbi, inst->settings, colours);
}

static void
l_ui_set_colourmap(struct rdp_inst * inst, RD_HCOLOURMAP map)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	dfb_set_colourmap(dfbi, inst->settings, map);
}

static RD_HBITMAP
l_ui_create_surface(struct rdp_inst * inst, int width, int height, RD_HBITMAP old_surface)
{
	dfbInfo * dfbi;
	IDirectFBSurface* surface;
	DFBSurfaceDescription dsc;
	dfbi = GET_DFBI(inst);
	
	surface = (IDirectFBSurface*) malloc(sizeof(IDirectFBSurface));
	dsc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PREALLOCATED | DSDESC_PIXELFORMAT;
	dsc.caps = DSCAPS_SYSTEMONLY;
	dsc.width = width;
	dsc.height = height;

	surface = (IDirectFBSurface*) malloc(sizeof(IDirectFBSurface));
	dfbi->dfb->CreateSurface(dfbi->dfb, &dsc, &surface);
	
	return (RD_HBITMAP) surface;
}

static void
l_ui_set_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	if (surface != 0)
		dfbi->drw = (IDirectFBSurface*) surface;
	else
		dfbi->drw = dfbi->primary;
}

static void
l_ui_destroy_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	free(surface);
}

static void
l_ui_channel_data(struct rdp_inst * inst, int chan_id, char * data, int data_size, int flags, int total_size)
{
}

static int
dfb_assign_callbacks(rdpInst * inst)
{
	inst->ui_error = l_ui_error;
	inst->ui_warning = l_ui_warning;
	inst->ui_unimpl = l_ui_unimpl;
	inst->ui_begin_update = l_ui_begin_update;
	inst->ui_end_update = l_ui_end_update;
	inst->ui_desktop_save = l_ui_desktop_save;
	inst->ui_desktop_restore = l_ui_desktop_restore;
	inst->ui_create_bitmap = l_ui_create_bitmap;
	inst->ui_paint_bitmap = l_ui_paint_bitmap;
	inst->ui_destroy_bitmap = l_ui_destroy_bitmap;
	inst->ui_line = l_ui_line;
	inst->ui_rect = l_ui_rect;
	inst->ui_polygon = l_ui_polygon;
	inst->ui_polyline = l_ui_polyline;
	inst->ui_ellipse = l_ui_ellipse;
	inst->ui_start_draw_glyphs = l_ui_start_draw_glyphs;
	inst->ui_draw_glyph = l_ui_draw_glyph;
	inst->ui_end_draw_glyphs = l_ui_end_draw_glyphs;
	inst->ui_get_toggle_keys_state = l_ui_get_toggle_keys_state;
	inst->ui_bell = l_ui_bell;
	inst->ui_destblt = l_ui_destblt;
	inst->ui_patblt = l_ui_patblt;
	inst->ui_screenblt = l_ui_screenblt;
	inst->ui_memblt = l_ui_memblt;
	inst->ui_triblt = l_ui_triblt;
	inst->ui_create_glyph = l_ui_create_glyph;
	inst->ui_destroy_glyph = l_ui_destroy_glyph;
	inst->ui_select = l_ui_select;
	inst->ui_set_clip = l_ui_set_clip;
	inst->ui_reset_clip = l_ui_reset_clip;
	inst->ui_resize_window = l_ui_resize_window;
	inst->ui_set_cursor = l_ui_set_cursor;
	inst->ui_destroy_cursor = l_ui_destroy_cursor;
	inst->ui_create_cursor = l_ui_create_cursor;
	inst->ui_set_null_cursor = l_ui_set_null_cursor;
	inst->ui_set_default_cursor = l_ui_set_default_cursor;
	inst->ui_create_colourmap = l_ui_create_colourmap;
	inst->ui_move_pointer = l_ui_move_pointer;
	inst->ui_set_colourmap = l_ui_set_colourmap;
	inst->ui_create_surface = l_ui_create_surface;
	inst->ui_set_surface = l_ui_set_surface;
	inst->ui_destroy_surface = l_ui_destroy_surface;
	inst->ui_channel_data = l_ui_channel_data;
	return 0;
}

void
dfb_init(int *argc, char *(*argv[]))
{
	DFBResult err;
	err = DirectFBInit(argc, argv);
}

int
dfb_pre_connect(rdpInst * inst)
{
	dfbInfo * dfbi;
	dfb_assign_callbacks(inst);
	dfbi = (dfbInfo *) malloc(sizeof(dfbInfo));
	SET_DFBI(inst, dfbi);
	memset(dfbi, 0, sizeof(dfbInfo));
	dfbi->bytes_per_pixel = 4;
	dfbi->update_pending = 0;
	dfbi->bpp = 16;
	return 0;
}

int
dfb_post_connect(rdpInst * inst)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	dfbi->err = DirectFBCreate(&(dfbi->dfb));

	dfbi->dsc.flags = DSDESC_CAPS;
	dfbi->dsc.caps = DSCAPS_PRIMARY;
	dfbi->err = dfbi->dfb->CreateSurface(dfbi->dfb, &(dfbi->dsc), &(dfbi->primary));
	dfbi->err = dfbi->primary->GetSize(dfbi->primary, &(dfbi->width), &(dfbi->height));
	dfbi->dfb->SetVideoMode(dfbi->dfb, dfbi->width, dfbi->height, dfbi->bpp);
	dfbi->dfb->CreateInputEventBuffer(dfbi->dfb, DICAPS_AXES | DICAPS_BUTTONS | DICAPS_KEYS, 0, &(dfbi->event));
	
	dfbi->width = inst->settings->width;
	dfbi->height = inst->settings->height;
	dfbi->screen = malloc(dfbi->width * dfbi->height * dfbi->bytes_per_pixel);
	dfbi->dsc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PREALLOCATED | DSDESC_PIXELFORMAT;
	dfbi->dsc.caps = DSCAPS_SYSTEMONLY;
	dfbi->dsc.width = dfbi->width;
	dfbi->dsc.height = dfbi->height;
	dfbi->dsc.pixelformat = DSPF_AiRGB;
	dfbi->dsc.preallocated[0].data = dfbi->screen;
	dfbi->dsc.preallocated[0].pitch = dfbi->width * dfbi->bytes_per_pixel;
	dfbi->dfb->CreateSurface(dfbi->dfb, &(dfbi->dsc), &(dfbi->screen_surface));
	
	return 0;
}

void
dfb_uninit(void * dfb_info)
{
	dfbInfo * dfbi;
	dfbi = (dfbInfo *) dfb_info;
	dfbi->primary->Release(dfbi->primary);
	dfbi->dfb->Release(dfbi->dfb);
}

int
dfb_get_fds(rdpInst * inst, void ** read_fds, int * read_count, void ** write_fds, int * write_count)
{
	return 0;
}

int
dfb_check_fds(rdpInst * inst)
{
	return 0;
}

