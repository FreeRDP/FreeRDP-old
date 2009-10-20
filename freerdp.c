
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "rdesktop.h"
#include "freerdp.h"
#include "rdp.h"
#include "secure.h"
#include "mcs.h"
#include "iso.h"
#include "tcp.h"
#include "mem.h"

void
ui_error(void * inst, char * format, ...)
{
	char * text1;
	char * text2;
	rdpInst * linst;
	va_list ap;

	text1 = (char *) xmalloc(1024);
	text2 = (char *) xmalloc(1024);
	va_start(ap, format);
	vsnprintf(text1, 1023, format, ap);
	va_end(ap);
	text1[1023] = 0;
	snprintf(text2, 1023, "ERROR: %s", text1);
	text2[1023] = 0;
	linst = (rdpInst *) inst;
	linst->ui_error(linst, text2);
	xfree(text1);
	xfree(text2);
}

void
ui_warning(void * inst, char * format, ...)
{
	char * text1;
	char * text2;
	rdpInst * linst;
	va_list ap;

	text1 = (char *) xmalloc(1024);
	text2 = (char *) xmalloc(1024);
	va_start(ap, format);
	vsnprintf(text1, 1023, format, ap);
	va_end(ap);
	text1[1023] = 0;
	snprintf(text2, 1023, "WARNING: %s", text1);
	text2[1023] = 0;
	linst = (rdpInst *) inst;
	linst->ui_warning(linst, text2);
	xfree(text1);
	xfree(text2);
}

void
ui_unimpl(void * inst, char * format, ...)
{
	char * text1;
	char * text2;
	rdpInst * linst;
	va_list ap;

	text1 = (char *) xmalloc(1024);
	text2 = (char *) xmalloc(1024);
	va_start(ap, format);
	vsnprintf(text1, 1023, format, ap);
	va_end(ap);
	text1[1023] = 0;
	snprintf(text2, 1023, "NOT IMPLEMENTED: %s", text1);
	text2[1023] = 0;
	linst = (rdpInst *) inst;
	linst->ui_unimpl(linst, text2);
	xfree(text1);
	xfree(text2);
}

void
hexdump(unsigned char * p, unsigned int len)
{
}

int
load_licence(unsigned char ** data)
{
	return 0;
}

RD_BOOL
rd_lock_file(int fd, int start, int len)
{
	return 0;
}

int
rd_lseek_file(int fd, int offset)
{
	return 0;
}

int
rd_write_file(int fd, void * ptr, int len)
{
	return 0;
}

RD_BOOL
rd_pstcache_mkdir(void)
{
	return 0;
}

void
rd_close_file(int fd)
{
}

int
rd_read_file(int fd, void * ptr, int len)
{
	return 0;
}

int
rd_open_file(char * filename)
{
	return 0;
}

void
generate_random(uint8 * random)
{
	int index;

	index = time(NULL);
	srand(index);
	for (index = 0; index < 32; index++)
	{
		random[index] = rand();
	}
}

void
save_licence(unsigned char * data, int length)
{
}

void
ui_begin_update(void * inst)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_begin_update(linst);
}

void
ui_end_update(void * inst)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_end_update(linst);
}

void
ui_line(void * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_line(linst, opcode, startx, starty, endx, endy, pen);
}

void
ui_rect(void * inst, int x, int y, int cx, int cy, int colour)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_rect(linst, x, y, cx, cy, colour);
}

void
ui_polygon(void * inst, uint8 opcode, uint8 fillmode, RD_POINT * point, int npoints,
	   RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_polygon(linst, opcode, fillmode, point, npoints, brush, bgcolour, fgcolour);
}

void
ui_polyline(void * inst, uint8 opcode, RD_POINT * points, int npoints, RD_PEN * pen)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_polyline(linst, opcode, points, npoints, pen);
}

void
ui_ellipse(void * inst, uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy,
	   RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_ellipse(linst, opcode, fillmode, x, y, cx, cy, brush, bgcolour, fgcolour);
}

void
ui_start_draw_glyphs(void * inst, int bgcolour, int fgcolour)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_start_draw_glyphs(linst, bgcolour, fgcolour);
}

void
ui_draw_glyph(void * inst, int x, int y, int cx, int cy, RD_HGLYPH glyph)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_draw_glyph(linst, x, y, cx, cy, glyph);
}

void
ui_end_draw_glyphs(void * inst, int x, int y, int cx, int cy)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_end_draw_glyphs(linst, x, y, cx, cy);
}

void
ui_desktop_save(void * inst, uint32 offset, int x, int y, int cx, int cy)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_desktop_save(linst, offset, x, y, cx, cy);
}

void
ui_desktop_restore(void * inst, uint32 offset, int x, int y, int cx, int cy)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_desktop_save(linst, offset, x, y, cx, cy);
}

uint32
ui_get_toggle_keys_state(void * inst)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	return linst->ui_get_toggle_keys_state(linst);
}

void
ui_bell(void * inst)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_bell(linst);
}

void
ui_destblt(void * inst, uint8 opcode, int x, int y, int cx, int cy)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_destblt(linst, opcode, x, y, cx, cy);
}

void
ui_patblt(void * inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH * brush,
	  int bgcolour, int fgcolour)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_patblt(linst, opcode, x, y, cx, cy, brush, bgcolour, fgcolour);
}

void
ui_screenblt(void * inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_screenblt(linst, opcode, x, y, cx, cy, srcx, srcy);
}

void
ui_memblt(void * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src,
	  int srcx, int srcy)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_memblt(linst, opcode, x, y, cx, cy, src, srcx, srcy);
}

void
ui_triblt(void * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src,
	  int srcx, int srcy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_triblt(linst, opcode, x, y, cx, cy, src, srcx, srcy, brush, bgcolour, fgcolour);
}

RD_HGLYPH
ui_create_glyph(void * inst, int width, int height, uint8 * data)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	return 	linst->ui_create_glyph(linst, width, height, data);
;
}

void
ui_destroy_glyph(void * inst, RD_HGLYPH glyph)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_destroy_glyph(linst, glyph);
}

int
ui_select(void * inst, int rdp_socket)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	return linst->ui_select(linst, rdp_socket);
}

void
ui_set_clip(void * inst, int x, int y, int cx, int cy)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_set_clip(linst, x, y, cx, cy);
}

void
ui_reset_clip(void * inst)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_reset_clip(linst);
}

void
ui_resize_window(void * inst)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_resize_window(linst);
}

RD_HCURSOR
ui_create_cursor(void * inst, unsigned int x, unsigned int y, int width, int height,
		 uint8 * andmask, uint8 * xormask, int bpp)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	return linst->ui_create_cursor(linst, x, y, width, height, andmask, xormask, bpp);
}

void
ui_set_cursor(void * inst, RD_HCURSOR cursor)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_set_cursor(linst, cursor);
}

void
ui_set_null_cursor(void * inst)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_set_null_cursor(linst);
}

void
ui_set_default_cursor(void * inst)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_set_default_cursor(linst);
}

void
ui_destroy_cursor(void * inst, RD_HCURSOR cursor)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_destroy_cursor(linst, cursor);
}

RD_HBITMAP
ui_create_bitmap(void * inst, int width, int height, uint8 * data)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	return linst->ui_create_bitmap(linst, width, height, data);
}

void
ui_paint_bitmap(void * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_paint_bitmap(linst, x, y, cx, cy, width,  height, data);
}

void
ui_destroy_bitmap(void * inst, RD_HBITMAP bmp)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_destroy_bitmap(linst, bmp);
}

RD_HCOLOURMAP
ui_create_colourmap(void * inst, RD_COLOURMAP * colours)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	return linst->ui_create_colourmap(linst, colours);
}

void
ui_move_pointer(void * inst, int x, int y)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_move_pointer(linst, x, y);
}

void
ui_set_colourmap(void * inst, RD_HCOLOURMAP map)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_set_colourmap(linst, map);
}

RD_HBITMAP
ui_create_surface(void * inst, int width, int height, RD_HBITMAP old)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	return linst->ui_create_surface(linst, width, height, old);
}

void
ui_set_surface(void * inst, RD_HBITMAP surface)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_set_surface(linst, surface);
}

void
ui_destroy_surface(void * inst, RD_HBITMAP surface)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_destroy_surface(linst, surface);
}

/* returns error */
static int
l_rdp_connect(struct rdp_inst * inst)
{
	rdpRdp * rdp;
	rdpSet * s;
	uint32 connect_flags;

	connect_flags = RDP_LOGON_NORMAL;
	rdp = (rdpRdp *) (inst->rdp);
	s = rdp->settings;
	if (s->autologin)
	{
		connect_flags |= RDP_LOGON_AUTO;
	}
	if (rdp_connect(rdp, s->server, connect_flags, s->domain, s->password, s->shell,
			s->directory, s->tcp_port_rdp, s->username))
	{
		return 0;
	}
	return 1;
}

static int
l_rdp_get_fds(struct rdp_inst * inst, void ** read_fds, int * read_count,
	void ** write_fds, int * write_count)
{
	rdpRdp * rdp;

	rdp = (rdpRdp *) (inst->rdp);
	read_fds[*read_count] = (void *) (rdp->sec->mcs->iso->tcp->sock);
	(*read_count)++;
	return 0;
}

static int
l_rdp_check_fds(struct rdp_inst * inst)
{
	rdpRdp * rdp;
	RD_BOOL deactivated;
	uint32 ext_disc_reason;

	rdp = (rdpRdp *) (inst->rdp);
	if (tcp_can_recv(rdp->sec->mcs->iso->tcp->sock, 0))
	{
		if (!rdp_loop(rdp, &deactivated, &ext_disc_reason))
		{
			return 1;
		}
	}
	return 0;
}

static int
l_rdp_send_input(struct rdp_inst * inst, int message_type, int device_flags,
	int param1, int param2)
{
	rdpRdp * rdp;

	rdp = (rdpRdp *) (inst->rdp);
	rdp_send_input(rdp, time(NULL), message_type, device_flags, param1, param2);
	return 0;
}

rdpInst *
freerdp_init(rdpSet * settings)
{
	rdpInst * inst;
	rdpRdp * rdp;

	rdp = rdp_setup(settings);
	inst = (rdpInst *) xmalloc(sizeof(rdpInst));
	inst->version = 1;
	inst->size = sizeof(rdpInst);
	inst->settings = settings;
	rdp->inst = inst;
	inst->rdp = rdp;
	inst->rdp_connect = l_rdp_connect;
	inst->rdp_get_fds = l_rdp_get_fds;
	inst->rdp_check_fds = l_rdp_check_fds;
	inst->rdp_send_input = l_rdp_send_input;
	return inst;
}

void
freerdp_deinit(rdpInst * inst)
{
	if (inst != NULL)
	{
		rdp_cleanup(inst->rdp);
		xfree(inst);
	}
}
