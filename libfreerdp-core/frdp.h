/*
   FreeRDP: A Remote Desktop Protocol client.

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

#ifndef __FRDP_H
#define __FRDP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freerdp/types/ui.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#ifndef snprintf
#define snprintf sprintf_s
#endif
#endif

#ifndef MIN
#define MIN(x,y)		(((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y)		(((x) > (y)) ? (x) : (y))
#endif

void
ui_error(rdpInst * inst, char * format, ...);
void
ui_warning(rdpInst * inst, char * format, ...);
void
ui_unimpl(rdpInst * inst, char * format, ...);
int
load_license(unsigned char ** data);
RD_BOOL
rd_lock_file(int fd, int start, int len);
int
rd_lseek_file(int fd, int offset);
int
rd_write_file(int fd, void * ptr, int len);
RD_BOOL
rd_pstcache_mkdir(void);
void
rd_close_file(int fd);
int
rd_read_file(int fd, void * ptr, int len);
int
rd_open_file(char * filename);
void
generate_random(uint8 * random);
void
save_license(unsigned char * data, int length);
void
ui_begin_update(rdpInst * inst);
void
ui_end_update(rdpInst * inst);
void
ui_line(rdpInst * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen);
void
ui_rect(rdpInst * inst, int x, int y, int cx, int cy, uint32 color);
void
ui_polygon(rdpInst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point, int npoints,
	   RD_BRUSH * brush, uint32 bgcolor, uint32 fgcolor);
void
ui_polyline(rdpInst * inst, uint8 opcode, RD_POINT * points, int npoints, RD_PEN * pen);
void
ui_ellipse(rdpInst * inst, uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy,
	   RD_BRUSH * brush, uint32 bgcolor, uint32 fgcolor);
void
ui_start_draw_glyphs(rdpInst * inst, uint32 bgcolor, uint32 fgcolor);
void
ui_draw_glyph(rdpInst * inst, int x, int y, int cx, int cy, RD_HGLYPH glyph);
void
ui_end_draw_glyphs(rdpInst * inst, int x, int y, int cx, int cy);
void
ui_desktop_save(rdpInst * inst, uint32 offset, int x, int y, int cx, int cy);
void
ui_desktop_restore(rdpInst * inst, uint32 offset, int x, int y, int cx, int cy);
uint32
ui_get_toggle_keys_state(rdpInst * inst);
void
ui_bell(rdpInst * inst);
void
ui_destblt(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy);
void
ui_patblt(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH * brush,
	  uint32 bgcolor, uint32 fgcolor);
void
ui_screenblt(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy);
void
ui_memblt(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src,
	  int srcx, int srcy);
void
ui_triblt(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src,
	  int srcx, int srcy, RD_BRUSH * brush, uint32 bgcolor, uint32 fgcolor);
RD_HGLYPH
ui_create_glyph(rdpInst * inst, int width, int height, uint8 * data);
void
ui_destroy_glyph(rdpInst * inst, RD_HGLYPH glyph);
int
ui_select(rdpInst * inst, int rdp_socket);
void
ui_set_clip(rdpInst * inst, int x, int y, int cx, int cy);
void
ui_reset_clip(rdpInst * inst);
void
ui_resize_window(rdpInst * inst);
RD_HCURSOR
ui_create_cursor(rdpInst * inst, unsigned int x, unsigned int y, int width, int height,
		 uint8 * andmask, uint8 * xormask, int bpp);
void
ui_set_cursor(rdpInst * inst, RD_HCURSOR cursor);
void
ui_set_null_cursor(rdpInst * inst);
void
ui_set_default_cursor(rdpInst * inst);
void
ui_destroy_cursor(rdpInst * inst, RD_HCURSOR cursor);
RD_HBITMAP
ui_create_bitmap(rdpInst * inst, int width, int height, uint8 * data);
void
ui_paint_bitmap(rdpInst * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data);
void
ui_destroy_bitmap(rdpInst * inst, RD_HBITMAP bmp);
RD_HPALETTE
ui_create_palette(rdpInst * inst, RD_PALETTE * palette);
void
ui_set_palette(rdpInst * inst, RD_HPALETTE palette);
void
ui_move_pointer(rdpInst * inst, int x, int y);
RD_HBITMAP
ui_create_surface(rdpInst * inst, int width, int height, RD_HBITMAP old);
void
ui_set_surface(rdpInst * inst, RD_HBITMAP surface);
void
ui_destroy_surface(rdpInst * inst, RD_HBITMAP surface);
void
ui_channel_data(rdpInst * inst, int chan_id, char * data, int data_size,
		int flags, int total_size);
RD_BOOL
ui_authenticate(rdpInst * inst);
int
ui_decode(rdpInst * inst, uint8 * data, int data_size);
RD_BOOL
ui_check_certificate(rdpInst * inst, const char * fingerprint,
		const char * subject, const char * issuer, RD_BOOL verified);

#endif
