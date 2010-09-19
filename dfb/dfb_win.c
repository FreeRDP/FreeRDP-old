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
#include "libfreerdpgdi.h"
#include "dfbfreerdp.h"
#include "dfb_win.h"
#include "dfb_keyboard.h"

static void
l_ui_error(struct rdp_inst * inst, char * text)
{
	//printf("ui_error: %s", text);
}

static void
l_ui_warning(struct rdp_inst * inst, char * text)
{
	//printf("ui_warning: %s\n", text);
}

static void
l_ui_unimpl(struct rdp_inst * inst, char * text)
{
	//printf("ui_unimpl: %s\n", text);
}

static void
l_ui_begin_update(struct rdp_inst * inst)
{
	dfbInfo *dfbi = GET_DFBI(inst);

	dfbi->update_rect.x = dfbi->hwnd->invalid->left;
	dfbi->update_rect.y = dfbi->hwnd->invalid->top;
	dfbi->update_rect.w = dfbi->hwnd->invalid->right - dfbi->hwnd->invalid->left;
	dfbi->update_rect.h = dfbi->hwnd->invalid->bottom - dfbi->hwnd->invalid->top;

	if (dfbi->update_rect.w > 0 && dfbi->update_rect.h > 0)
	{
		printf("ui_begin_update: x:%d y:%d w:%d h:%d\n", dfbi->hwnd->invalid->left, dfbi->hwnd->invalid->top,
		       dfbi->hwnd->invalid->right - dfbi->hwnd->invalid->left,
		       dfbi->hwnd->invalid->bottom - dfbi->hwnd->invalid->top);
	
		dfbi->primary->Blit(dfbi->primary, dfbi->screen_surface, &(dfbi->update_rect), dfbi->update_rect.x, dfbi->update_rect.y);

		dfbi->update_rect.x = 0;
		dfbi->update_rect.y = 0;
		dfbi->update_rect.w = 0;
		dfbi->update_rect.h = 0;

		dfbi->hwnd->invalid->left = 0;
		dfbi->hwnd->invalid->right = 0;
		dfbi->hwnd->invalid->top = 0;
		dfbi->hwnd->invalid->bottom = 0;
		dfbi->hwnd->dirty = 0;
	}
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
	HBITMAP glyph;
	char* glyphData;
	dfbInfo *dfbi = GET_DFBI(inst);

	glyphData = (char*) gdi_image_convert((char*) data, width, height, 1, 1, dfbi->palette);
	glyph = CreateBitmap(width, height, 1, glyphData);

	return (RD_HGLYPH) glyph;
}

static void
l_ui_destroy_glyph(struct rdp_inst * inst, RD_HGLYPH glyph)
{
	DeleteObject((HGDIOBJ) glyph);
}

static RD_HBITMAP
l_ui_create_bitmap(struct rdp_inst * inst, int width, int height, uint8 * data)
{
	char* bmpData;
	HBITMAP bitmap;
	dfbInfo *dfbi = GET_DFBI(inst);
	
	bmpData = (char*) gdi_image_convert((char*) data, width, height, dfbi->srcBpp, dfbi->dstBpp, dfbi->palette);
	bitmap = CreateBitmap(width, height, dfbi->dstBpp, bmpData);
	
	return (RD_HBITMAP) bitmap;
}

static void
l_ui_paint_bitmap(struct rdp_inst * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
	HBITMAP bitmap;
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	
	bitmap = (HBITMAP) inst->ui_create_bitmap(inst, width, height, data);
	SelectObject(dfbi->hdcBmp, (HGDIOBJ) bitmap);
	BitBlt(dfbi->hdc, x, y, cx, cy, dfbi->hdcBmp, 0, 0, SRCCOPY);

	dfbi->rect->left = x;
	dfbi->rect->top = y;
	dfbi->rect->right = x + cx;
	dfbi->rect->bottom = y + cy;
	InvalidateRect(dfbi->hwnd, dfbi->rect);
	//inst->ui_begin_update(inst);
}

static void
l_ui_destroy_bitmap(struct rdp_inst * inst, RD_HBITMAP bmp)
{
	DeleteObject((HGDIOBJ) bmp);
}

static void
l_ui_line(struct rdp_inst * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen)
{
}

static void
l_ui_rect(struct rdp_inst * inst, int x, int y, int cx, int cy, int colour)
{
	RECT rect;
	HBRUSH hBrush;
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	printf("ui_rect: x: %d y: %d cx: %d cy: %d\n", x, y, cx, cy);

	rect.left = x;
	rect.top = y;
	rect.right = x + cx;
	rect.bottom = y + cy;

	gdi_colour_convert(&(dfbi->pixel), colour, dfbi->srcBpp, dfbi->palette);
	hBrush = CreateSolidBrush((COLORREF) gdi_make_colorref(&(dfbi->pixel)));
	FillRect(dfbi->hdc, &rect, hBrush);

	dfbi->rect->left = x;
	dfbi->rect->top = y;
	dfbi->rect->right = x + cx;
	dfbi->rect->bottom = y + cy;
	InvalidateRect(dfbi->hwnd, dfbi->rect);
	//inst->ui_begin_update(inst);
}

static void
l_ui_polygon(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point, int npoints, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
}

static void
l_ui_polyline(struct rdp_inst * inst, uint8 opcode, RD_POINT * points, int npoints, RD_PEN * pen)
{
}

static void
l_ui_ellipse(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
}

static void
l_ui_start_draw_glyphs(struct rdp_inst * inst, int bgcolour, int fgcolour)
{
	dfbInfo *dfbi = GET_DFBI(inst);
	
	gdi_colour_convert(&(dfbi->fgcolour), fgcolour, dfbi->dstBpp, dfbi->palette);
	dfbi->textColor = SetTextColor(dfbi->hdc, gdi_make_colorref(&(dfbi->fgcolour)));
}

static void
l_ui_draw_glyph(struct rdp_inst * inst, int x, int y, int cx, int cy, RD_HGLYPH glyph)
{
	HBITMAP glyphBitmap = (HBITMAP) glyph;
	dfbInfo *dfbi = GET_DFBI(inst);
	
	SelectObject(dfbi->hdcBmp, (HGDIOBJ) glyphBitmap);
	BitBlt(dfbi->hdc, x, y, cx, cy, dfbi->hdcBmp, 0, 0, 0x00E20746); /* DSPDxax */

	dfbi->rect->left = x;
	dfbi->rect->top = y;
	dfbi->rect->right = x + cx;
	dfbi->rect->bottom = y + cy;
	InvalidateRect(dfbi->hwnd, dfbi->rect);
	//inst->ui_begin_update(inst);
}

static void
l_ui_end_draw_glyphs(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
	dfbInfo *dfbi = GET_DFBI(inst);
	SetTextColor(dfbi->hdc, dfbi->textColor);
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
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	printf("ui_destblt: x: %d y: %d cx: %d cy: %d rop: 0x%X\n", x, y, cx, cy, gdi_rop3_code(opcode));

	BitBlt(dfbi->hdc, x, y, cx, cy, NULL, 0, 0, gdi_rop3_code(opcode));

	dfbi->rect->left = x;
	dfbi->rect->top = y;
	dfbi->rect->right = x + cx;
	dfbi->rect->bottom = y + cy;
	InvalidateRect(dfbi->hwnd, dfbi->rect);
}

static void
l_ui_patblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	
}

static void
l_ui_screenblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy)
{
	//printf("ui_screenblt rop: 0x%X\n", gdi_rop3_code(opcode));
}

static void
l_ui_memblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy)
{
	dfbInfo *dfbi = GET_DFBI(inst);
	
	SelectObject(dfbi->hdcBmp, (HGDIOBJ) src);
	BitBlt(dfbi->hdc, x, y, cx, cy, dfbi->hdcBmp, srcx, srcy, gdi_rop3_code(opcode));

	dfbi->rect->left = x;
	dfbi->rect->top = y;
	dfbi->rect->right = x + cx;
	dfbi->rect->bottom = y + cy;
	InvalidateRect(dfbi->hwnd, dfbi->rect);
}

static void
l_ui_triblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	//printf("ui_triblt opcode: 0x%X\n", gdi_rop3_code(opcode));
}

static int
l_ui_select(struct rdp_inst * inst, int rdp_socket)
{
	return 1;
}

static void
l_ui_set_clip(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
	HRGN clippingRegion;
	dfbInfo *dfbi = GET_DFBI(inst);
	
	clippingRegion = CreateRectRgn(x, y, x + cx, y + cy);
	SelectClipRgn(dfbi->hdc, clippingRegion);
}

static void
l_ui_reset_clip(struct rdp_inst * inst)
{
	dfbInfo *dfbi = GET_DFBI(inst);
	SelectClipRgn(dfbi->hdc, NULL);
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
l_ui_create_cursor(struct rdp_inst * inst, uint32 x, uint32 y, int width, int height, uint8 * andmask, uint8 * xormask, int bpp)
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
	int i;
	HPALETTE palette;
	LOGPALETTE *logicalPalette;

	logicalPalette = (LOGPALETTE*) malloc(sizeof(LOGPALETTE));

	logicalPalette->entries = (PALETTEENTRY*) malloc(sizeof(PALETTEENTRY) * 256);
	memset(logicalPalette->entries, 0, sizeof(PALETTEENTRY) * 256);
	logicalPalette->count = colours->ncolours;

	if (logicalPalette->count > 256)
		logicalPalette->count = 256;

	for (i = logicalPalette->count - 1; i >= 0; i--)
	{
		logicalPalette->entries[i].red = colours->colours[i].red;
		logicalPalette->entries[i].green = colours->colours[i].green;
		logicalPalette->entries[i].blue = colours->colours[i].blue;
	}

	palette = CreatePalette(logicalPalette);

	return (RD_HCOLOURMAP) palette;
}

static void
l_ui_set_colourmap(struct rdp_inst * inst, RD_HCOLOURMAP map)
{
	dfbInfo *dfbi = GET_DFBI(inst);
	dfbi->palette = (HPALETTE) map;
}

static RD_HBITMAP
l_ui_create_surface(struct rdp_inst * inst, int width, int height, RD_HBITMAP old_surface)
{
	HBITMAP new_bitmap;
	HBITMAP old_bitmap;
	dfbInfo *dfbi = GET_DFBI(inst);

	new_bitmap = CreateCompatibleBitmap(dfbi->hdc, width, height);
	old_bitmap = (HBITMAP) old_surface;

	if (old_bitmap != 0)
	{
		SelectObject(dfbi->hdcBmp, (HGDIOBJ) old_bitmap);
		SelectObject(dfbi->hdc, (HGDIOBJ) new_bitmap);
		DeleteObject((HGDIOBJ) old_bitmap);
	}
	else
	{
		dfbi->surface = new_bitmap;
	}

	return (RD_HBITMAP) new_bitmap;
}

static void
l_ui_set_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	dfbInfo *dfbi = GET_DFBI(inst);

	if (surface != 0)
	{
		dfbi->surface = surface;
		SelectObject(dfbi->hdc, (HGDIOBJ) dfbi->surface);
	}
	else
	{
		dfbi->surface = dfbi->backingstore;
		SelectObject(dfbi->hdc, (HGDIOBJ) dfbi->surface);
	}
}

static void
l_ui_destroy_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	DeleteObject((HGDIOBJ) surface);
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
	memset(dfbi, 0, sizeof(dfbInfo));
	SET_DFBI(inst, dfbi);
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
	dfbi->dfb->SetVideoMode(dfbi->dfb, dfbi->width, dfbi->height, dfbi->dstBpp);
	dfbi->dfb->CreateInputEventBuffer(dfbi->dfb, DICAPS_ALL, 0, &(dfbi->event));

	dfbi->dfb->GetDisplayLayer(dfbi->dfb, 0, &(dfbi->layer));
	dfbi->layer->EnableCursor(dfbi->layer, 1);

	dfbi->width = inst->settings->width;
	dfbi->height = inst->settings->height;

	dfbi->dstBpp = 32;
	dfbi->srcBpp = inst->settings->server_depth;
	
	dfbi->screen = malloc(dfbi->width * dfbi->height * 4);
	dfbi->dsc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PREALLOCATED | DSDESC_PIXELFORMAT;
	dfbi->dsc.caps = DSCAPS_SYSTEMONLY;
	dfbi->dsc.width = dfbi->width;
	dfbi->dsc.height = dfbi->height;
	dfbi->dsc.pixelformat = DSPF_AiRGB;
	dfbi->dsc.preallocated[0].data = dfbi->screen;
	dfbi->dsc.preallocated[0].pitch = dfbi->width * 4;
	dfbi->dfb->CreateSurface(dfbi->dfb, &(dfbi->dsc), &(dfbi->screen_surface));

	dfbi->hwnd = (HWND) malloc(sizeof(WND));
	dfbi->hwnd->invalid = (HRECT) malloc(sizeof(RECT));
	memset(dfbi->hwnd->invalid, 0, sizeof(RECT));

	dfbi->rect = (HRECT) malloc(sizeof(RECT));
	memset(dfbi->rect, 0, sizeof(RECT));
	
	dfbi->hdc = GetDC();
	dfbi->hdc->bitsPerPixel = dfbi->dstBpp;
	dfbi->hdc->bytesPerPixel = 4;

	dfbi->backingstore = CreateBitmap(dfbi->width, dfbi->height, dfbi->dstBpp, (char*) dfbi->screen);
	SelectObject(dfbi->hdc, (HGDIOBJ) dfbi->backingstore);
	dfbi->hdcBmp = CreateCompatibleDC(dfbi->hdc);

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
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	dfbi->event->CreateFileDescriptor(dfbi->event, read_fds[*read_count]);
	(*read_count)++;
	return 0;
}

int
dfb_check_fds(rdpInst * inst)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	while (dfbi->event->HasEvent(dfbi->event) == DFB_OK)
	{
		if (dfbi->event->GetEvent(dfbi->event, &(dfbi->events[0])) == 0)
		{
			dfb_process_event(inst, &(dfbi->events[0]));
		}
	}

	return 0;
}
