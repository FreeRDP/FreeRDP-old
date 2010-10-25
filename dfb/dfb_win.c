/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI Main Window

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <directfb.h>
#include <freerdp/chanman.h>
#include "libfreerdpgdi.h"
#include "dfbfreerdp.h"
#include "dfb_win.h"
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
	GDI *gdi = GET_GDI(inst);
	gdi->primary->hdc->hwnd->invalid->null = 1;
}

static void
l_ui_end_update(struct rdp_inst * inst)
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
	GDI *gdi = GET_GDI(inst);

	gdi->cursor_x = x;
	gdi->cursor_y = y;
	
	inst->rdp_send_input(inst, RDP_INPUT_MOUSE, PTRFLAGS_MOVE, x, y);
}

static void
l_ui_channel_data(struct rdp_inst * inst, int chan_id, char * data, int data_size, int flags, int total_size)
{
}

static int
dfb_register_callbacks(rdpInst * inst)
{
	inst->ui_begin_update = l_ui_begin_update;
	inst->ui_end_update = l_ui_end_update;
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
	dfbInfo *dfbi = (dfbInfo *) malloc(sizeof(dfbInfo));
	memset(dfbi, 0, sizeof(dfbInfo));
	dfb_register_callbacks(inst);
	SET_DFBI(inst, dfbi);
	return 0;
}

int
dfb_post_connect(rdpInst * inst)
{
	GDI *gdi;
	dfbInfo *dfbi = GET_DFBI(inst);

	gdi_init(inst);
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
	dfbi->dsc.pixelformat = DSPF_AiRGB;
	dfbi->dsc.preallocated[0].data = gdi->primary_buffer;
	dfbi->dsc.preallocated[0].pitch = gdi->width * 4;
	dfbi->dfb->CreateSurface(dfbi->dfb, &(dfbi->dsc), &(dfbi->surface));

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

