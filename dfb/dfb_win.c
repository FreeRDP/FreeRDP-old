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
#include "dfb_gdi.h"
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
dfb_update_screen(dfbInfo * dfbi)
{
	if (dfbi->update_rect.w > 0 && dfbi->update_rect.h > 0)
	{
		dfbi->primary->Blit(dfbi->primary, dfbi->screen_surface,
	                    &(dfbi->update_rect), dfbi->update_rect.x, dfbi->update_rect.y);
		
		dfbi->update_rect.x = 0;
		dfbi->update_rect.y = 0;
		dfbi->update_rect.w = 0;
		dfbi->update_rect.h = 0;
	}
}

static void
dfb_invalidate_rect(dfbInfo * dfbi, int x1, int y1, int x2, int y2)
{
	if (dfbi->update_rect.w == 0 && dfbi->update_rect.h == 0)
	{
		dfbi->update_rect.x = x1;
		dfbi->update_rect.y = y1;
		dfbi->update_rect.w = x2 - x1;
		dfbi->update_rect.h = y2 - y1;
	}
	else
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
	dfb_update_screen(dfbi);
}

static void
l_ui_end_update(struct rdp_inst * inst)
{

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
	HBITMAP bitmap;
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	bitmap = CreateBitmap(width, height, dfbi->bpp, dfb_image_convert(dfbi, inst->settings, width, height, data));
	return (RD_HBITMAP) bitmap;
}

static void
l_ui_paint_bitmap(struct rdp_inst * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
	HBITMAP bitmap;
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	bitmap = (HBITMAP) inst->ui_create_bitmap(inst, width, height, data);
	inst->ui_memblt(inst, 12, x, y, cx, cy, (RD_HBITMAP) bitmap, 0, 0);
}

static void
l_ui_destroy_bitmap(struct rdp_inst * inst, RD_HBITMAP bmp)
{
	HBITMAP bitmap = (HBITMAP) bmp;
	
	if (bitmap != NULL)
	{
		if (bitmap->data != NULL)
			free(bitmap->data);
		free(bitmap);
	}
}

static void
l_ui_line(struct rdp_inst * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen)
{
	printf("ui_line\n");
}

static void
l_ui_rect(struct rdp_inst * inst, int x, int y, int cx, int cy, int colour)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	dfb_colour_convert(dfbi, colour, &(dfbi->pixel), inst->settings->server_depth, dfbi->bpp);
	dfbi->primary->SetColor(dfbi->primary, dfbi->pixel.red, dfbi->pixel.green, dfbi->pixel.blue, dfbi->pixel.alpha);
	dfbi->primary->FillRectangle(dfbi->primary, x, y, cx, cy);
	dfb_invalidate_rect(dfbi, x, y, x + cx, y + cy);
	dfb_update_screen(dfbi);
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
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	dfb_colour_convert(dfbi, bgcolour, &(dfbi->bgcolour), inst->settings->server_depth, dfbi->bpp);
	dfb_colour_convert(dfbi, fgcolour, &(dfbi->fgcolour), inst->settings->server_depth, dfbi->bpp);
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
	printf("ui_destblt opcode: 0x%X\n", opcode);
	
	if (opcode == 0)
	{
		/* BLACKNESS */
		inst->ui_rect(inst, x, y, cx, cy, 0);
	}
}

static void
l_ui_patblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	dfb_colour_convert(dfbi, bgcolour, &(dfbi->bgcolour), inst->settings->server_depth, dfbi->bpp);
	dfb_colour_convert(dfbi, fgcolour, &(dfbi->fgcolour), inst->settings->server_depth, dfbi->bpp);

	printf("ui_patblt opcode: 0x%X\n", opcode);

	switch (brush->style)
	{
		case BS_SOLID:
			printf("solid brush style\n");
			return;
		case BS_NULL:
			printf("null brush style\n");
			return;
		case BS_HATCHED:
			printf("hatch brush style\n");
			break;
		case BS_PATTERN:
			printf("pattern brush style\n");
			break;
		default:
			printf("brush %d\n", brush->style);
			break;
	}

	/* ROP3 can be one of the following: PATCOPY, PATINVERT, DSTINVERT, BLACKNESS or WHITENESS */
}

static void
l_ui_screenblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy)
{
	printf("ui_screenblt opcode: 0x%X\n", opcode);
}

static void
l_ui_memblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy)
{
	int i;
	char * dst;
	char * srcp;
	HBITMAP bitmap;
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	bitmap = (HBITMAP) src;
	srcp = (char *) (((unsigned int *) bitmap->data) + srcy * bitmap->width + srcx);
	
	if (opcode == 0xC) /* ??? */
	{
		for (i = 0; i < cy; i++)
		{			
			dst = dfb_get_screen_pointer(dfbi, x, y + i);
			if (dst != 0)
			{
				dfb_copy_mem(dst, srcp, cx * dfbi->bytes_per_pixel);
				srcp += bitmap->width * dfbi->bytes_per_pixel;
			}
		}

		dfb_invalidate_rect(dfbi, x, y, x + cx, y + cy);
		dfb_update_screen(dfbi);
	}
	else if (opcode == 0xCC) /* SRCCOPY */
	{
		for (i = 0; i < cy; i++)
		{			
			dst = dfb_get_screen_pointer(dfbi, x, y + i);
			if (dst != 0)
			{
				dfb_copy_mem(dst, srcp, cx * dfbi->bytes_per_pixel);
				srcp += bitmap->width * dfbi->bytes_per_pixel;
			}
		}

		dfb_invalidate_rect(dfbi, x, y, x + cx, y + cy);
		dfb_update_screen(dfbi);
	}
	else if (opcode == 0x66) /* ??? */
	{
		for (i = 0; i < cy; i++)
		{			
			dst = dfb_get_screen_pointer(dfbi, x, y + i);
			if (dst != 0)
			{
				dfb_copy_mem(dst, srcp, cx * dfbi->bytes_per_pixel);
				srcp += bitmap->width * dfbi->bytes_per_pixel;
			}
		}

		dfb_invalidate_rect(dfbi, x, y, x + cx, y + cy);
		dfb_update_screen(dfbi);
	}
	else if (opcode == 0x88) /* ??? */
	{
		for (i = 0; i < cy; i++)
		{			
			dst = dfb_get_screen_pointer(dfbi, x, y + i);
			if (dst != 0)
			{
				dfb_copy_mem(dst, srcp, cx * dfbi->bytes_per_pixel);
				srcp += bitmap->width * dfbi->bytes_per_pixel;
			}
		}

		dfb_invalidate_rect(dfbi, x, y, x + cx, y + cy);
		dfb_update_screen(dfbi);
	}
	else
	{
		printf("ui_memblt opcode: 0x%X x: %d y: %d cx: %d cy: %d srcx: %d srcy: %d\n",
		       opcode, x, y, cx, cy, srcx, srcy);
	}
}

static void
l_ui_triblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_screenblt opcode: 0x%X\n", opcode);
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
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	dfbi->cursor_x = x;
	dfbi->cursor_y = y;
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
	return (RD_HBITMAP) old_surface;
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
	dfbi->bpp = 32;
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
	dfbi->dfb->CreateInputEventBuffer(dfbi->dfb, DICAPS_ALL, 0, &(dfbi->event));

	dfbi->dfb->GetDisplayLayer(dfbi->dfb, 0, &(dfbi->layer));
	dfbi->layer->EnableCursor(dfbi->layer, 1);
	
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
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	
	if (dfbi->event->HasEvent(dfbi->event) == DFB_OK)
	{
		if (dfbi->event->GetEvent(dfbi->event, &(dfbi->events[0])) == 0)
		{
			dfb_process_event(inst, &(dfbi->events[0]));
		}
	}

	return 0;
}
