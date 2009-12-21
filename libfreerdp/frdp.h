
#ifndef __FRDP_H
#define __FRDP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types_ui.h"
#include "types.h"

#ifdef WIN32
#include "winsock.h"
#define snprintf sprintf_s
#define strcasecmp lstrcmpi
#endif

#ifndef MIN
#define MIN(x,y)		(((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y)		(((x) > (y)) ? (x) : (y))
#endif

void
ui_error(void * inst, char * format, ...);
void
ui_warning(void * inst, char * format, ...);
void
ui_unimpl(void * inst, char * format, ...);
void
hexdump(unsigned char * p, int len);
int
load_licence(unsigned char ** data);
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
save_licence(unsigned char * data, int length);
void
ui_begin_update(void * inst);
void
ui_end_update(void * inst);
void
ui_line(void * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen);
void
ui_rect(void * inst, int x, int y, int cx, int cy, int colour);
void
ui_polygon(void * inst, uint8 opcode, uint8 fillmode, RD_POINT * point, int npoints,
	   RD_BRUSH * brush, int bgcolour, int fgcolour);
void
ui_polyline(void * inst, uint8 opcode, RD_POINT * points, int npoints, RD_PEN * pen);
void
ui_ellipse(void * inst, uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy,
	   RD_BRUSH * brush, int bgcolour, int fgcolour);
void
ui_start_draw_glyphs(void * inst, int bgcolour, int fgcolour);
void
ui_draw_glyph(void * inst, int x, int y, int cx, int cy, RD_HGLYPH glyph);
void
ui_end_draw_glyphs(void * inst, int x, int y, int cx, int cy);
void
ui_desktop_save(void * inst, uint32 offset, int x, int y, int cx, int cy);
void
ui_desktop_restore(void * inst, uint32 offset, int x, int y, int cx, int cy);
uint32
ui_get_toggle_keys_state(void * inst);
void
ui_bell(void * inst);
void
ui_destblt(void * inst, uint8 opcode, int x, int y, int cx, int cy);
void
ui_patblt(void * inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH * brush,
	  int bgcolour, int fgcolour);
void
ui_screenblt(void * inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy);
void
ui_memblt(void * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src,
	  int srcx, int srcy);
void
ui_triblt(void * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src,
	  int srcx, int srcy, RD_BRUSH * brush, int bgcolour, int fgcolour);
RD_HGLYPH
ui_create_glyph(void * inst, int width, int height, uint8 * data);
void
ui_destroy_glyph(void * inst, RD_HGLYPH glyph);
int
ui_select(void * inst, int rdp_socket);
void
ui_set_clip(void * inst, int x, int y, int cx, int cy);
void
ui_reset_clip(void * inst);
void
ui_resize_window(void * inst);
RD_HCURSOR
ui_create_cursor(void * inst, unsigned int x, unsigned int y, int width, int height,
		 uint8 * andmask, uint8 * xormask, int bpp);
void
ui_set_cursor(void * inst, RD_HCURSOR cursor);
void
ui_set_null_cursor(void * inst);
void
ui_set_default_cursor(void * inst);
void
ui_destroy_cursor(void * inst, RD_HCURSOR cursor);
RD_HBITMAP
ui_create_bitmap(void * inst, int width, int height, uint8 * data);
void
ui_paint_bitmap(void * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data);
void
ui_destroy_bitmap(void * inst, RD_HBITMAP bmp);
RD_HCOLOURMAP
ui_create_colourmap(void * inst, RD_COLOURMAP * colours);
void
ui_move_pointer(void * inst, int x, int y);
void
ui_set_colourmap(void * inst, RD_HCOLOURMAP map);
RD_HBITMAP
ui_create_surface(void * inst, int width, int height, RD_HBITMAP old);
void
ui_set_surface(void * inst, RD_HBITMAP surface);
void
ui_destroy_surface(void * inst, RD_HBITMAP surface);
void
ui_channel_data(void * inst, int chan_id, char * data, int data_size,
		int flags, int total_size);

#endif
