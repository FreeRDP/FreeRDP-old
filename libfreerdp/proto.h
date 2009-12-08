/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Copyright (C) Matthew Chapman 1999-2008

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

#ifndef RDESKTOP_PROTO_H
#define RDESKTOP_PROTO_H

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */
/* cliprdr.c */
void cliprdr_send_simple_native_format_announce(uint32 format);
void cliprdr_send_native_format_announce(uint8 * formats_data, uint32 formats_data_length);
void cliprdr_send_data_request(uint32 format);
void cliprdr_send_data(uint8 * data, uint32 length);
void cliprdr_set_mode(const char *optarg);
RD_BOOL cliprdr_init(void);
/* disk.c */
int disk_enum_devices(uint32 * id, char *optarg);
RD_NTSTATUS disk_query_information(RD_NTHANDLE handle, uint32 info_class, STREAM out);
RD_NTSTATUS disk_set_information(RD_NTHANDLE handle, uint32 info_class, STREAM in, STREAM out);
RD_NTSTATUS disk_check_notify(RD_NTHANDLE handle);
RD_NTSTATUS disk_create_notify(RD_NTHANDLE handle, uint32 info_class);
RD_NTSTATUS disk_query_volume_information(RD_NTHANDLE handle, uint32 info_class, STREAM out);
RD_NTSTATUS disk_query_directory(RD_NTHANDLE handle, uint32 info_class, char *pattern, STREAM out);
/* ewmhints.c */
int get_current_workarea(uint32 * x, uint32 * y, uint32 * width, uint32 * height);
void ewmh_init(void);
/* parallel.c */
int parallel_enum_devices(uint32 * id, char *optarg);
/* printer.c */
int printer_enum_devices(uint32 * id, char *optarg);
/* printercache.c */
int printercache_load_blob(char *printer_name, uint8 ** data);
void printercache_process(STREAM s);
/* rdesktop.c */
int main(int argc, char *argv[]);
void generate_random(uint8 * random);
void exit_if_null(void *ptr);
void hexdump(unsigned char *p, unsigned int len);
char *next_arg(char *src, char needle);
void toupper_str(char *p);
RD_BOOL str_startswith(const char *s, const char *prefix);
RD_BOOL str_handle_lines(const char *input, char **rest, str_handle_lines_t linehandler,
			 void *data);
RD_BOOL subprocess(char *const argv[], str_handle_lines_t linehandler, void *data);
char *l_to_a(long N, int base);
int load_licence(unsigned char **data);
void save_licence(unsigned char *data, int length);
RD_BOOL rd_pstcache_mkdir(void);
int rd_open_file(char *filename);
void rd_close_file(int fd);
int rd_read_file(int fd, void *ptr, int len);
int rd_write_file(int fd, void *ptr, int len);
int rd_lseek_file(int fd, int offset);
RD_BOOL rd_lock_file(int fd, int start, int len);
/* rdpdr.c */
int get_device_index(RD_NTHANDLE handle);
void convert_to_unix_filename(char *filename);
void rdpdr_send_completion(uint32 device, uint32 id, uint32 status, uint32 result, uint8 * buffer,
			   uint32 length);
RD_BOOL rdpdr_init(void);
void rdpdr_add_fds(int *n, fd_set * rfds, fd_set * wfds, struct timeval *tv, RD_BOOL * timeout);
struct async_iorequest *rdpdr_remove_iorequest(struct async_iorequest *prev,
					       struct async_iorequest *iorq);
void rdpdr_check_fds(fd_set * rfds, fd_set * wfds, RD_BOOL timed_out);
RD_BOOL rdpdr_abort_io(uint32 fd, uint32 major, RD_NTSTATUS status);
/* rdpsnd.c */
void rdpsnd_record(const void *data, unsigned int size);
RD_BOOL rdpsnd_init(char *optarg);
void rdpsnd_show_help(void);
void rdpsnd_add_fds(int *n, fd_set * rfds, fd_set * wfds, struct timeval *tv);
void rdpsnd_check_fds(fd_set * rfds, fd_set * wfds);
struct audio_packet *rdpsnd_queue_current_packet(void);
RD_BOOL rdpsnd_queue_empty(void);
void rdpsnd_queue_next(unsigned long completed_in_us);
int rdpsnd_queue_next_tick(void);
/* serial.c */
int serial_enum_devices(uint32 * id, char *optarg);
RD_BOOL serial_get_event(RD_NTHANDLE handle, uint32 * result);
RD_BOOL serial_get_timeout(RD_NTHANDLE handle, uint32 length, uint32 * timeout,
			   uint32 * itv_timeout);
/* xclip.c */
void ui_clip_format_announce(uint8 * data, uint32 length);
void ui_clip_handle_data(uint8 * data, uint32 length);
void ui_clip_request_failed(void);
void ui_clip_request_data(uint32 format);
void ui_clip_sync(void);
void ui_clip_set_mode(const char *optarg);
void xclip_init(void);
void xclip_deinit(void);
/* xwin.c */
//RD_BOOL get_key_state(unsigned int state, uint32 keysym);
RD_BOOL ui_init(void);
void ui_deinit(void * inst);
RD_BOOL ui_create_window(void * inst);
void ui_resize_window(void * inst);
void ui_destroy_window(void);
void xwin_toggle_fullscreen(void * inst);
int ui_select(void * inst, int rdp_socket);
void ui_move_pointer(void * inst, int x, int y);
RD_HBITMAP ui_create_bitmap(void * inst, int width, int height, uint8 * data);
void ui_paint_bitmap(void * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data);
void ui_destroy_bitmap(void * inst, RD_HBITMAP bmp);
RD_HGLYPH ui_create_glyph(void * inst, int width, int height, uint8 * data);
void ui_destroy_glyph(void * inst, RD_HGLYPH glyph);
RD_HCURSOR ui_create_cursor(void * inst, unsigned int x, unsigned int y, int width, int height, uint8 * andmask,
			    uint8 * xormask, int bpp);
void ui_set_cursor(void * inst, RD_HCURSOR cursor);
void ui_destroy_cursor(void * inst, RD_HCURSOR cursor);
void ui_set_null_cursor(void * inst);
void ui_set_default_cursor(void * inst);
RD_HCOLOURMAP ui_create_colourmap(void * inst, RD_COLOURMAP * colours);
void ui_destroy_colourmap(void * inst, RD_HCOLOURMAP map);
void ui_set_colourmap(void * inst, RD_HCOLOURMAP map);
void ui_set_clip(void * inst, int x, int y, int cx, int cy);
void ui_reset_clip(void * inst);
void ui_bell(void * inst);
void ui_destblt(void * inst, uint8 opcode, int x, int y, int cx, int cy);
void ui_patblt(void * inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH * brush, int bgcolour,
	       int fgcolour);
void ui_screenblt(void * inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy);
void ui_memblt(void * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy);
void ui_triblt(void * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy,
	       RD_BRUSH * brush, int bgcolour, int fgcolour);
void ui_line(void * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen);
void ui_rect(void * inst, int x, int y, int cx, int cy, int colour);
void ui_polygon(void * inst, uint8 opcode, uint8 fillmode, RD_POINT * point, int npoints, RD_BRUSH * brush,
		int bgcolour, int fgcolour);
void ui_polyline(void * inst, uint8 opcode, RD_POINT * points, int npoints, RD_PEN * pen);
void ui_ellipse(void * inst, uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy, RD_BRUSH * brush,
		int bgcolour, int fgcolour);
void ui_start_draw_glyphs(void * inst, int bgcolour, int fgcolour);
void ui_draw_glyph(void * inst, int x, int y, int cx, int cy, RD_HGLYPH glyph);
void ui_end_draw_glyphs(void * inst, int x, int y, int cx, int cy);
//void ui_desktop_save(void * inst, uint32 offset, int x, int y, int cx, int cy);
//void ui_desktop_restore(void * inst, uint32 offset, int x, int y, int cx, int cy);
void ui_begin_update(void * inst);
void ui_end_update(void * inst);
uint32 ui_get_toggle_keys_state(void * inst);
unsigned int read_keyboard_state(void);
void ui_error(void * inst, char *format, ...);
void ui_warning(void * inst, char *format, ...);
void ui_unimpl(void * inst, char *format, ...);
RD_HBITMAP ui_create_surface(void * inst, int width, int height, RD_HBITMAP old);
void ui_set_surface(void * inst, RD_HBITMAP surface);
void ui_destroy_surface(void * inst, RD_HBITMAP surface);
void ui_channel_data(void * inst, int chan_id, char * data, int data_size,
		int flags, int total_size);
/* lspci.c */
/* scard.c */
void scard_lock(int lock);
void scard_unlock(int lock);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif
