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

#ifndef __FREERDP_H
#define __FREERDP_H

#include "rdpset.h"
#include "types_ui.h"
#include "constants_ui.h"

#define FREERDP_INTERFACE_VERSION 2

#if defined _WINDOWS
#define FREERDP_API _declspec(dllexport)
#else
#define FREERDP_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct rdp_inst
{
	int version;
	int size;
	rdpSet * settings;
	void * rdp;
	void * param1;
	void * param2;
	void * param3;
	void * param4;
	/* calls from ui to library */
	int (* rdp_connect)(struct rdp_inst * inst);
	int (* rdp_get_fds)(struct rdp_inst * inst, void ** read_fds, int * read_count,
		void ** write_fds, int * write_count);
	int (* rdp_check_fds)(struct rdp_inst * inst);
	int (* rdp_send_input)(struct rdp_inst * inst, int message_type, int device_flags,
		int param1, int param2);
	int (* rdp_sync_input)(struct rdp_inst * inst, int toggle_flags);
	int (* rdp_channel_data)(struct rdp_inst * inst, int chan_id, char * data, int data_size);
	/* calls from library to ui */
	void (* ui_error)(struct rdp_inst * inst, char * text);
	void (* ui_warning)(struct rdp_inst * inst, char * text);
	void (* ui_unimpl)(struct rdp_inst * inst, char * text);
	void (* ui_begin_update)(struct rdp_inst * inst);
	void (* ui_end_update)(struct rdp_inst * inst);
	void (* ui_desktop_save)(struct rdp_inst * inst, int offset, int x, int y,
		int cx, int cy);
	void (* ui_desktop_restore)(struct rdp_inst * inst, int offset, int x, int y,
		int cx, int cy);
	RD_HBITMAP (* ui_create_bitmap)(struct rdp_inst * inst, int width, int height, uint8 * data);
	void (* ui_paint_bitmap)(struct rdp_inst * inst, int x, int y, int cx, int cy, int width,
		int height, uint8 * data);
	void (* ui_destroy_bitmap)(struct rdp_inst * inst, RD_HBITMAP bmp);
	void (* ui_line)(struct rdp_inst * inst, uint8 opcode, int startx, int starty, int endx,
		int endy, RD_PEN * pen);
	void (* ui_rect)(struct rdp_inst * inst, int x, int y, int cx, int cy, int colour);
	void (* ui_polygon)(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point,
		int npoints, RD_BRUSH * brush, int bgcolour, int fgcolour);
	void (* ui_polyline)(struct rdp_inst * inst, uint8 opcode, RD_POINT * points, int npoints,
		RD_PEN * pen);
	void (* ui_ellipse)(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, int x, int y,
		int cx, int cy, RD_BRUSH * brush, int bgcolour, int fgcolour);
	void (* ui_start_draw_glyphs)(struct rdp_inst * inst, int bgcolour, int fgcolour);
	void (* ui_draw_glyph)(struct rdp_inst * inst, int x, int y, int cx, int cy,
		RD_HGLYPH glyph);
	void (* ui_end_draw_glyphs)(struct rdp_inst * inst, int x, int y, int cx, int cy);
	uint32 (* ui_get_toggle_keys_state)(struct rdp_inst * inst);
	void (* ui_bell)(struct rdp_inst * inst);
	void (* ui_destblt)(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy);
	void (* ui_patblt)(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
		RD_BRUSH * brush, int bgcolour, int fgcolour);
	void (* ui_screenblt)(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
		int srcx, int srcy);
	void (* ui_memblt)(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
		RD_HBITMAP src, int srcx, int srcy);
	void (* ui_triblt)(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
		RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, int bgcolour, int fgcolour);
	RD_HGLYPH (* ui_create_glyph)(struct rdp_inst * inst, int width, int height, uint8 * data);
	void (* ui_destroy_glyph)(struct rdp_inst * inst, RD_HGLYPH glyph);
	int (* ui_select)(struct rdp_inst * inst, int rdp_socket);
	void (* ui_set_clip)(struct rdp_inst * inst, int x, int y, int cx, int cy);
	void (* ui_reset_clip)(struct rdp_inst * inst);
	void (* ui_resize_window)(struct rdp_inst * inst);
	void (* ui_set_cursor)(struct rdp_inst * inst, RD_HCURSOR cursor);
	void (* ui_destroy_cursor)(struct rdp_inst * inst, RD_HCURSOR cursor);
	RD_HCURSOR (* ui_create_cursor)(struct rdp_inst * inst, unsigned int x, unsigned int y,
		int width, int height, uint8 * andmask, uint8 * xormask, int bpp);
	void (* ui_set_null_cursor)(struct rdp_inst * inst);
	void (* ui_set_default_cursor)(struct rdp_inst * inst);
	RD_HCOLOURMAP (* ui_create_colourmap)(struct rdp_inst * inst, RD_COLOURMAP * colours);
	void (* ui_move_pointer)(struct rdp_inst * inst, int x, int y);
	void (* ui_set_colourmap)(struct rdp_inst * inst, RD_HCOLOURMAP map);
	RD_HBITMAP (* ui_create_surface)(struct rdp_inst * inst, int width, int height, RD_HBITMAP old);
	void (* ui_set_surface)(struct rdp_inst * inst, RD_HBITMAP surface);
	void (* ui_destroy_surface)(struct rdp_inst * inst, RD_HBITMAP surface);
	void (* ui_channel_data)(struct rdp_inst * inst, int chan_id, char * data, int data_size,
		int flags, int total_size);
};
typedef struct rdp_inst rdpInst;

FREERDP_API rdpInst *
freerdp_new(rdpSet * settings);
FREERDP_API void
freerdp_free(rdpInst * inst);

#ifdef __cplusplus
}
#endif

#endif
