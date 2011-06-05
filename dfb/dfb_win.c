/*
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI Main Window

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <directfb.h>
#include <freerdp/chanman.h>
#include <freerdp/types/ui.h>
#include <freerdp/utils/memory.h>
#include "gdi.h"
#include "dfbfreerdp.h"
#include "dfb_win.h"
#include "dfb_keyboard.h"

static void
l_ui_error(struct rdp_inst * inst, const char * text)
{
	printf("ui_error: %s", text);
}

static void
l_ui_warning(struct rdp_inst * inst, const char * text)
{
	printf("ui_warning: %s\n", text);
}

static void
l_ui_unimpl(struct rdp_inst * inst, const char * text)
{
	printf("ui_unimpl: %s\n", text);
}

/* GDI callbacks registered in libfreerdp */

static void
l_ui_gdi_begin_update(struct rdp_inst * inst)
{
	GDI *gdi = GET_GDI(inst);
	gdi->primary->hdc->hwnd->invalid->null = 1;
}

static void
l_ui_gdi_end_update(struct rdp_inst * inst)
{
	dfbInfo *dfbi = GET_DFBI(inst);
	GDI *gdi = GET_GDI(inst);

	if (gdi->primary->hdc->hwnd->invalid->null)
		return;

	dfbi->update_rect.x = gdi->primary->hdc->hwnd->invalid->x;
	dfbi->update_rect.y = gdi->primary->hdc->hwnd->invalid->y;
	dfbi->update_rect.w = gdi->primary->hdc->hwnd->invalid->w;
	dfbi->update_rect.h = gdi->primary->hdc->hwnd->invalid->h;

	dfbi->primary->Blit(dfbi->primary, dfbi->surface, &(dfbi->update_rect), dfbi->update_rect.x, dfbi->update_rect.y);
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
l_ui_create_bitmap(struct rdp_inst * inst, int width, int height, uint8* data)
{
	return (RD_HBITMAP) NULL;
}

static void
l_ui_paint_bitmap(struct rdp_inst * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data)
{

}

static void
l_ui_destroy_bitmap(struct rdp_inst * inst, RD_HBITMAP bmp)
{

}

static void
l_ui_line(struct rdp_inst * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen)
{

}

static void
l_ui_rect(struct rdp_inst * inst, int x, int y, int cx, int cy, uint32 color)
{

}

static void
l_ui_polygon(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point, int npoints, RD_BRUSH * brush, uint32 bgcolor, uint32 fgcolor)
{

}

static void
l_ui_polyline(struct rdp_inst * inst, uint8 opcode, RD_POINT * points, int npoints, RD_PEN * pen)
{

}

static void
l_ui_ellipse(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy, RD_BRUSH * brush, uint32 bgcolor, uint32 fgcolor)
{

}

static void
l_ui_start_draw_glyphs(struct rdp_inst * inst, uint32 bgcolor, uint32 fgcolor)
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

static void
l_ui_destblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy)
{

}

static void
l_ui_patblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH * brush, uint32 bgcolor, uint32 fgcolor)
{

}

static void
l_ui_screenblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy)
{

}

static void
l_ui_memblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy)
{

}

static void
l_ui_mem3blt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, uint32 bgcolor, uint32 fgcolor)
{

}

static RD_HPALETTE
l_ui_create_palette(struct rdp_inst * inst, RD_PALETTE * palette)
{
	return (RD_HPALETTE) NULL;
}

static void
l_ui_set_palette(struct rdp_inst * inst, RD_HPALETTE palette)
{

}

static void
l_ui_set_clipping_region(struct rdp_inst * inst, int x, int y, int cx, int cy)
{

}

static void
l_ui_reset_clipping_region(struct rdp_inst * inst)
{

}

static RD_HBITMAP
l_ui_create_surface(struct rdp_inst * inst, int width, int height, RD_HBITMAP old_surface)
{
	return (RD_HBITMAP) NULL;
}

static void
l_ui_switch_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{

}

static void
l_ui_destroy_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{

}

static int
l_ui_decode(struct rdp_inst * inst, uint8 * data, int size)
{
	return 0;
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

static int
l_ui_select(struct rdp_inst * inst, int rdp_socket)
{
	return 1;
}

static void
l_ui_resize_window(struct rdp_inst * inst)
{
}

static void
l_ui_set_cursor(struct rdp_inst * inst, RD_HCURSOR cur)
{
	CursorInfo *cursor = (CursorInfo*) cur;
	dfbInfo *dfbi = GET_DFBI(inst);
	dfbi->layer->SetCooperativeLevel(dfbi->layer, DLSCL_ADMINISTRATIVE);
	DFBResult ret = dfbi->layer->SetCursorShape(dfbi->layer, cursor->surface, cursor->hotx, cursor->hoty);

	if (ret != DFB_OK)
		DirectFBErrorFatal("Error SetCursorShape", ret);

	dfbi->layer->SetCooperativeLevel(dfbi->layer, DLSCL_SHARED);
}

static void
l_ui_destroy_cursor(struct rdp_inst * inst, RD_HCURSOR cur)
{
	CursorInfo *cursor = (CursorInfo*) cur;

	if (cursor == NULL)
		return;

	cursor->surface->Release(cursor->surface);
	free(cursor);
}

static RD_HCURSOR
l_ui_create_cursor(struct rdp_inst * inst, uint32 x, uint32 y, int width, int height, uint8 * andmask, uint8 * xormask, int bpp)
{
	DFBResult ret;
	CursorInfo *cursor;
	DFBSurfaceDescription dsc;
	dfbInfo *dfbi = GET_DFBI(inst);
	GDI *gdi = GET_GDI(inst);

	cursor = (CursorInfo*) xmalloc(sizeof(CursorInfo));
	dsc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
	dsc.caps = DSCAPS_SYSTEMONLY;
	dsc.width = width;
	dsc.height = height;
	dsc.pixelformat = DSPF_ARGB;

	ret = dfbi->dfb->CreateSurface(dfbi->dfb, &dsc, &cursor->surface);

	if (ret == DFB_OK)
	{
		int pitch;
		uint8* point = NULL;

		cursor->hotx = x;
		cursor->hoty = y;

		/* copy the data */
		ret = cursor->surface->Lock(cursor->surface, DSLF_WRITE, (void**) &point, &pitch);

		if (ret != DFB_OK)
			goto out;
    
		gdi_alpha_cursor_convert(point, xormask, andmask, width, height, bpp, gdi->clrconv);
		cursor->surface->Unlock(cursor->surface);
	}

out:
	if (ret != DFB_OK)
	{
		DirectFBErrorFatal("Error create cursor surface", ret);
		return (RD_HCURSOR) NULL;
	}

	return (RD_HCURSOR) cursor;
}

static void
l_ui_set_null_cursor(struct rdp_inst * inst)
{
	dfbInfo *dfbi = GET_DFBI(inst);
	dfbi->layer->EnableCursor(dfbi->layer, 0);
}

static void
l_ui_set_default_cursor(struct rdp_inst * inst)
{
	dfbInfo *dfbi = GET_DFBI(inst);
	dfbi->layer->SetCooperativeLevel(dfbi->layer, DLSCL_ADMINISTRATIVE);
	DFBResult ret = dfbi->layer->SetCursorShape(dfbi->layer, NULL, 0, 0);

	if (ret != DFB_OK)
		DirectFBErrorFatal("Error SetCursorShape", ret);

	dfbi->layer->SetCooperativeLevel(dfbi->layer, DLSCL_SHARED);
}

static void
l_ui_move_pointer(struct rdp_inst * inst, int x, int y)
{
	GDI *gdi = GET_GDI(inst);

	gdi->cursor_x = x;
	gdi->cursor_y = y;

	inst->rdp_send_input_mouse(inst, PTRFLAGS_MOVE, x, y);
}

static void
l_ui_channel_data(struct rdp_inst * inst, int chan_id, char * data, int data_size, int flags, int total_size)
{
	freerdp_chanman_data(inst, chan_id, data, data_size, flags, total_size);
}

static RD_BOOL
l_ui_authenticate(struct rdp_inst * inst)
{
	char * pass;
	int l;

	printf("Please enter credentials for network level authentication.\n");

	printf("User name:");
	if (inst->settings->username[0] == 0)
	{
		if (fgets(inst->settings->username, sizeof(inst->settings->username), stdin) == NULL)
		{
		}
		l = strlen(inst->settings->username);
		if (l > 0 && inst->settings->username[l - 1] == '\n')
			inst->settings->username[l - 1] = 0;
	}
	else
		printf("%s\n", inst->settings->username);

	printf("Domain:");
	if (inst->settings->domain[0] == 0)
	{
		if (fgets(inst->settings->domain, sizeof(inst->settings->domain), stdin) == NULL)
		{
		}
		l = strlen(inst->settings->domain);
		if (l > 0 && inst->settings->domain[l - 1] == '\n')
			inst->settings->domain[l - 1] = 0;
	}
	else
		printf("%s\n", inst->settings->domain);

	if (!inst->settings->password[0])
	{
		pass = getpass("Password:");
		strncpy(inst->settings->password, pass, sizeof(inst->settings->password) - 1);
	}
	return 1;
}

RD_BOOL
l_ui_check_certificate(rdpInst * inst, const char * fingerprint,
	const char * subject, const char * issuer, RD_BOOL verified)
{
	printf("certificate details:\n");
	printf("  Subject:\n    %s\n", subject);
	printf("  Issued by:\n    %s\n", issuer);
	printf("  Fingerprint:\n    %s\n",  fingerprint);

	if (!verified)
		printf("The server could not be authenticated. Connection security may be compromised!\n");

	return 1;
}

static int
dfb_register_callbacks(rdpInst * inst)
{
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
	inst->ui_destblt = l_ui_destblt;
	inst->ui_patblt = l_ui_patblt;
	inst->ui_screenblt = l_ui_screenblt;
	inst->ui_memblt = l_ui_memblt;
	inst->ui_triblt = l_ui_mem3blt;
	inst->ui_create_palette = l_ui_create_palette;
	inst->ui_set_palette = l_ui_set_palette;
	inst->ui_create_glyph = l_ui_create_glyph;
	inst->ui_destroy_glyph = l_ui_destroy_glyph;
	inst->ui_set_clip = l_ui_set_clipping_region;
	inst->ui_reset_clip = l_ui_reset_clipping_region;
	inst->ui_create_surface = l_ui_create_surface;
	inst->ui_set_surface = l_ui_switch_surface;
	inst->ui_destroy_surface = l_ui_destroy_surface;
	inst->ui_error = l_ui_error;
	inst->ui_warning = l_ui_warning;
	inst->ui_unimpl = l_ui_unimpl;
	inst->ui_end_update = l_ui_end_update;
	inst->ui_get_toggle_keys_state = l_ui_get_toggle_keys_state;
	inst->ui_bell = l_ui_bell;
	inst->ui_select = l_ui_select;
	inst->ui_resize_window = l_ui_resize_window;
	inst->ui_set_cursor = l_ui_set_cursor;
	inst->ui_destroy_cursor = l_ui_destroy_cursor;
	inst->ui_create_cursor = l_ui_create_cursor;
	inst->ui_set_null_cursor = l_ui_set_null_cursor;
	inst->ui_set_default_cursor = l_ui_set_default_cursor;
	inst->ui_move_pointer = l_ui_move_pointer;
	inst->ui_channel_data = l_ui_channel_data;
	inst->ui_authenticate = l_ui_authenticate;
	inst->ui_decode = l_ui_decode;
	inst->ui_check_certificate = l_ui_check_certificate;
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
	dfbInfo *dfbi = (dfbInfo *) xmalloc(sizeof(dfbInfo));
	memset(dfbi, 0, sizeof(dfbInfo));
	dfb_register_callbacks(inst);
	SET_DFBI(inst, dfbi);

	return 0;
}

int
dfb_post_connect(rdpInst * inst)
{
	dfbInfo *dfbi = GET_DFBI(inst);

	if (inst->settings->software_gdi == 1)
	{
		GDI *gdi;
		gdi_init(inst, CLRCONV_ALPHA | CLRBUF_16BPP | CLRBUF_32BPP);
		gdi = GET_GDI(inst);

		dfbi->err = DirectFBCreate(&(dfbi->dfb));

		dfbi->dsc.flags = DSDESC_CAPS;
		dfbi->dsc.caps = DSCAPS_PRIMARY;
		dfbi->err = dfbi->dfb->CreateSurface(dfbi->dfb, &(dfbi->dsc), &(dfbi->primary));
		dfbi->err = dfbi->primary->GetSize(dfbi->primary, &(gdi->width), &(gdi->height));
		dfbi->dfb->SetVideoMode(dfbi->dfb, gdi->width, gdi->height, gdi->dstBpp);
		dfbi->dfb->CreateInputEventBuffer(dfbi->dfb, DICAPS_ALL, DFB_TRUE, &(dfbi->event_buffer));
		dfbi->event_buffer->CreateFileDescriptor(dfbi->event_buffer, &(dfbi->read_fds));

		dfbi->dfb->GetDisplayLayer(dfbi->dfb, 0, &(dfbi->layer));
		dfbi->layer->EnableCursor(dfbi->layer, 1);

		dfbi->dsc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PREALLOCATED | DSDESC_PIXELFORMAT;
		dfbi->dsc.caps = DSCAPS_SYSTEMONLY;
		dfbi->dsc.width = gdi->width;
		dfbi->dsc.height = gdi->height;

		if (gdi->dstBpp == 32 || gdi->dstBpp == 24)
			dfbi->dsc.pixelformat = DSPF_AiRGB;
		else if (gdi->dstBpp == 16 || gdi->dstBpp == 15)
			dfbi->dsc.pixelformat = DSPF_RGB16;
		else if (gdi->dstBpp == 8)
			dfbi->dsc.pixelformat = DSPF_RGB332;
		else
			dfbi->dsc.pixelformat = DSPF_AiRGB;

		dfbi->dsc.preallocated[0].data = gdi->primary_buffer;
		dfbi->dsc.preallocated[0].pitch = gdi->width * gdi->bytesPerPixel;
		dfbi->dfb->CreateSurface(dfbi->dfb, &(dfbi->dsc), &(dfbi->surface));

		inst->ui_begin_update = l_ui_gdi_begin_update;
		inst->ui_end_update = l_ui_gdi_end_update;
	}
	else
	{
		/* DirectFB-specific GDI Implementation */
	}

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
	dfbInfo *dfbi = GET_DFBI(inst);

	read_fds[*read_count] = (void *)(long)(dfbi->read_fds);
	(*read_count)++;

	return 0;
}

int
dfb_check_fds(rdpInst * inst, fd_set *set)
{
	dfbInfo *dfbi = GET_DFBI(inst);

	if (!FD_ISSET(dfbi->read_fds, set))
		return 0;

	if (read(dfbi->read_fds, &(dfbi->event), sizeof(dfbi->event)) > 0)
		dfb_process_event(inst, &(dfbi->event));

	return 0;
}

