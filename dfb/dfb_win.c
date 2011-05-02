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
#include <freerdp/types_ui.h>
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

static int
l_ui_decode(struct rdp_inst * inst, uint8 * data, int data_size)
{
	printf("l_ui_decode: size %d\n", data_size);
	return 0;
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

	gdi_init(inst, CLRCONV_ALPHA);
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

