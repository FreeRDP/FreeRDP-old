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
l_ui_begin_update(struct rdp_inst * inst)
{
}

static void
l_ui_end_update(struct rdp_inst * inst)
{

}

static void
l_ui_desktop_save(struct rdp_inst * inst, int offset, int x, int y, int cx, int cy)
{
	printf("ui_desktop_save:\n");
}

static void
l_ui_desktop_restore(struct rdp_inst * inst, int offset, int x, int y, int cx, int cy)
{
	printf("ui_desktop_restore:\n");
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
	dfbInfo * dfbi;
	IDirectFBSurface* surface;
	DFBSurfaceDescription dsc;
	dfbi = GET_DFBI(inst);
	
	surface = (IDirectFBSurface*) malloc(sizeof(IDirectFBSurface));
	dsc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT;
	dsc.caps = DSCAPS_VIDEOONLY;
	dsc.width = width;
	dsc.height = height;

	surface = (IDirectFBSurface*) malloc(sizeof(IDirectFBSurface));
	dfbi->dfb->CreateSurface(dfbi->dfb, &dsc, &surface);
	
	return (RD_HBITMAP) surface;
}

static void
l_ui_paint_bitmap(struct rdp_inst * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
	dfbInfo * dfbi;
	DFBRectangle rect;
	IDirectFBSurface* surface;
	dfbi = GET_DFBI(inst);

	rect.x = x;
	rect.y = y;
	rect.w = cx;
	rect.h = cy;
	
	surface = (IDirectFBSurface*) l_ui_create_bitmap(inst, width, height, data);
	surface->Blit(surface, dfbi->primary, &rect, 0, 0);
	dfbi->primary->Blit(dfbi->primary, surface, 0, x, y);
	surface->Release(surface);
}

static void
l_ui_destroy_bitmap(struct rdp_inst * inst, RD_HBITMAP bmp)
{
	free(bmp);
}

static void
l_ui_line(struct rdp_inst * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen)
{

}

static void
l_ui_rect(struct rdp_inst * inst, int x, int y, int cx, int cy, int colour)
{

}

static void
l_ui_polygon(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point,
	int npoints, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_polygon\n");
}

static void
l_ui_polyline(struct rdp_inst * inst, uint8 opcode, RD_POINT * points, int npoints, RD_PEN * pen)
{

}

static void
l_ui_ellipse(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, int x, int y,
	int cx, int cy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_ellipse:\n");
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
	printf("ui_bell\n");
}

static void
l_ui_destblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy)
{
	printf("ui_destblt\n");
}

static void
l_ui_patblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_patblt\n");
}

static void
l_ui_screenblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	int srcx, int srcy)
{
	printf("ui_screenblt\n");
}

static void
l_ui_memblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy)
{
	printf("ui_memblt\n");
}

static void
l_ui_triblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_triblt\n");
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
	dfbi->region.x2 = (x + cx) - 1;
	dfbi->region.y2 = (y + cy) - 1;

	dfbi->primary->SetClip(dfbi->primary, &(dfbi->region));
}

static void
l_ui_reset_clip(struct rdp_inst * inst)
{

}

static void
l_ui_resize_window(struct rdp_inst * inst)
{
	printf("ui_resize_window\n");
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

static RD_HCOLOURMAP
l_ui_create_colourmap(struct rdp_inst * inst, RD_COLOURMAP * colours)
{
	return (RD_HCOLOURMAP) NULL;
}

static void
l_ui_move_pointer(struct rdp_inst * inst, int x, int y)
{
	printf("ui_move_pointer\n");
}

static void
l_ui_set_colourmap(struct rdp_inst * inst, RD_HCOLOURMAP map)
{
	printf("ui_set_colourmap\n");
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

}

static void
l_ui_destroy_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{

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
	DFBResult err;
	dfbInfo * dfbi;
	
	dfb_assign_callbacks(inst);
	dfbi = (dfbInfo *) malloc(sizeof(dfbInfo));
	SET_DFBI(inst, dfbi);
	memset(dfbi, 0, sizeof(dfbInfo));

	err = DirectFBCreate(&(dfbi->dfb));
	
	dfbi->dsc.flags = DSDESC_CAPS;
	dfbi->dsc.caps  = DSCAPS_PRIMARY | DSCAPS_DOUBLE | DSCAPS_FLIPPING;
	err = dfbi->dfb->CreateSurface(dfbi->dfb, &(dfbi->dsc), &(dfbi->primary));
	err = dfbi->primary->GetSize(dfbi->primary, &(dfbi->width), &(dfbi->height));

	dfbi->dfb->CreateInputEventBuffer(dfbi->dfb, DICAPS_AXES | DICAPS_BUTTONS | DICAPS_KEYS, 0, &(dfbi->event));
	dfb_kb_inst_init(inst);

	return 0;
}

int
dfb_post_connect(rdpInst * inst)
{
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
dfb_get_fds(rdpInst * inst, void ** read_fds, int * read_count,
	void ** write_fds, int * write_count)
{

	return 0;
}

int
dfb_check_fds(rdpInst * inst)
{
	return 0;
}

