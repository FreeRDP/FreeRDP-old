/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP settings

   Copyright (C) Jay Sorg 2009-2011

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

#ifndef __FREERDP_H
#define __FREERDP_H

#include "rdpset.h"
#include "types_ui.h"
#include "constants_ui.h"
#include "rdpext.h"

#define FREERDP_INTERFACE_VERSION 4

#if defined _WIN32 || defined __CYGWIN__
  #ifdef FREERDP_EXPORTS
    #ifdef __GNUC__
      #define FREERDP_API __attribute__((dllexport))
    #else
      #define FREERDP_API __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define FREERDP_API __attribute__((dllimport))
    #else
      #define FREERDP_API __declspec(dllimport)
    #endif
  #endif
#else
  #if __GNUC__ >= 4
    #define FREERDP_API   __attribute__ ((visibility("default")))
  #else
    #define FREERDP_API
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

RD_BOOL
freerdp_global_init(void);
void
freerdp_global_finish(void);

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
	uint32 disc_reason;
	/* calls from ui to library */
	int (* rdp_connect)(rdpInst * inst);
	int (* rdp_get_fds)(rdpInst * inst, void ** read_fds, int * read_count,
		void ** write_fds, int * write_count);
	int (* rdp_check_fds)(rdpInst * inst);
	int (* rdp_send_input_scancode)(rdpInst * inst, RD_BOOL up, RD_BOOL extended, uint8 keyCode);
	int (* rdp_send_input_unicode)(rdpInst * inst, uint8 character);
	int (* rdp_send_input_mouse)(rdpInst * inst, uint16 pointerFlags, uint16 xPos, uint16 yPos);
	int (* rdp_sync_input)(rdpInst * inst, int toggle_flags);
	int (* rdp_channel_data)(rdpInst * inst, int chan_id, char * data, int data_size);
	void (* rdp_disconnect)(rdpInst * inst);
	int (* rdp_send_frame_ack)(rdpInst * inst, int frame_id);
	/* calls from library to ui */
	void (* ui_error)(rdpInst * inst, const char * text);
	void (* ui_warning)(rdpInst * inst, const char * text);
	void (* ui_unimpl)(rdpInst * inst, const char * text);
	void (* ui_begin_update)(rdpInst * inst);
	void (* ui_end_update)(rdpInst * inst);
	void (* ui_desktop_save)(rdpInst * inst, int offset, int x, int y,
		int cx, int cy);
	void (* ui_desktop_restore)(rdpInst * inst, int offset, int x, int y,
		int cx, int cy);
	RD_HBITMAP (* ui_create_bitmap)(rdpInst * inst, int width, int height, uint8 * data);
	void (* ui_paint_bitmap)(rdpInst * inst, int x, int y, int cx, int cy, int width,
		int height, uint8 * data);
	void (* ui_destroy_bitmap)(rdpInst * inst, RD_HBITMAP bmp);
	void (* ui_line)(rdpInst * inst, uint8 opcode, int startx, int starty, int endx,
		int endy, RD_PEN * pen);
	void (* ui_rect)(rdpInst * inst, int x, int y, int cx, int cy, int color);
	void (* ui_polygon)(rdpInst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point,
		int npoints, RD_BRUSH * brush, int bgcolor, int fgcolor);
	void (* ui_polyline)(rdpInst * inst, uint8 opcode, RD_POINT * points, int npoints,
		RD_PEN * pen);
	void (* ui_ellipse)(rdpInst * inst, uint8 opcode, uint8 fillmode, int x, int y,
		int cx, int cy, RD_BRUSH * brush, int bgcolor, int fgcolor);
	void (* ui_start_draw_glyphs)(rdpInst * inst, int bgcolor, int fgcolor);
	void (* ui_draw_glyph)(rdpInst * inst, int x, int y, int cx, int cy,
		RD_HGLYPH glyph);
	void (* ui_end_draw_glyphs)(rdpInst * inst, int x, int y, int cx, int cy);
	uint32 (* ui_get_toggle_keys_state)(rdpInst * inst);
	void (* ui_bell)(rdpInst * inst);
	void (* ui_destblt)(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy);
	void (* ui_patblt)(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy,
		RD_BRUSH * brush, int bgcolor, int fgcolor);
	void (* ui_screenblt)(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy,
		int srcx, int srcy);
	void (* ui_memblt)(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy,
		RD_HBITMAP src, int srcx, int srcy);
	void (* ui_triblt)(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy,
		RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, int bgcolor, int fgcolor);
	RD_HGLYPH (* ui_create_glyph)(rdpInst * inst, int width, int height, uint8 * data);
	void (* ui_destroy_glyph)(rdpInst * inst, RD_HGLYPH glyph);
	int (* ui_select)(rdpInst * inst, int rdp_socket);
	void (* ui_set_clip)(rdpInst * inst, int x, int y, int cx, int cy);
	void (* ui_reset_clip)(rdpInst * inst);
	void (* ui_resize_window)(rdpInst * inst);
	void (* ui_set_cursor)(rdpInst * inst, RD_HCURSOR cursor);
	void (* ui_destroy_cursor)(rdpInst * inst, RD_HCURSOR cursor);
	RD_HCURSOR (* ui_create_cursor)(rdpInst * inst, unsigned int x, unsigned int y,
		int width, int height, uint8 * andmask, uint8 * xormask, int bpp);
	void (* ui_set_null_cursor)(rdpInst * inst);
	void (* ui_set_default_cursor)(rdpInst * inst);
	RD_HPALETTE (* ui_create_colormap)(rdpInst * inst, RD_PALETTE * colors);
	void (* ui_move_pointer)(rdpInst * inst, int x, int y);
	void (* ui_set_colormap)(rdpInst * inst, RD_HPALETTE map);
	RD_HBITMAP (* ui_create_surface)(rdpInst * inst, int width, int height, RD_HBITMAP old);
	void (* ui_set_surface)(rdpInst * inst, RD_HBITMAP surface);
	void (* ui_destroy_surface)(rdpInst * inst, RD_HBITMAP surface);
	void (* ui_channel_data)(rdpInst * inst, int chan_id, char * data, int data_size,
		int flags, int total_size);
	RD_BOOL (* ui_authenticate)(rdpInst * inst);
	int (* ui_decode)(rdpInst * inst, uint8 * data, int data_size);
	RD_BOOL (* ui_check_certificate)(rdpInst * inst, const char * fingerprint,
		const char * subject, const char * issuer, RD_BOOL verified);
};

FREERDP_API rdpInst *
freerdp_new(rdpSet * settings);
FREERDP_API void
freerdp_free(rdpInst * inst);

#ifdef __cplusplus
}
#endif

#endif
