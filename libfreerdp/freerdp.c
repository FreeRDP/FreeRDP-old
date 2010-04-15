
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "frdp.h"
#include "freerdp.h"
#include "rdp.h"
#include "secure.h"
#include "mcs.h"
#include "iso.h"
#include "tcp.h"
#include "mem.h"
#include "chan.h"

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
hexdump(unsigned char * p, int len)
{
	unsigned char *line = p;
	int i, thisline, offset = 0;

	while (offset < len)
	{
		printf("%04x ", offset);
		thisline = len - offset;
		if (thisline > 16)
			thisline = 16;

		for (i = 0; i < thisline; i++)
			printf("%02x ", line[i]);

		for (; i < 16; i++)
			printf("   ");

		for (i = 0; i < thisline; i++)
			printf("%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');

		printf("\n");
		offset += thisline;
		line += thisline;
	}
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
	time_t index;

	index = time(NULL);
	srand((unsigned int)index);
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

void
ui_channel_data(void * inst, int chan_id, char * data, int data_size,
		int flags, int total_size)
{
	rdpInst * linst;

	linst = (rdpInst *) inst;
	linst->ui_channel_data(linst, chan_id, data, data_size, flags, total_size);
}

/* returns error */
static int
l_rdp_connect(struct rdp_inst * inst)
{
	rdpRdp * rdp;
	rdpSet * s;
	uint32 connect_flags;
	int index;

	connect_flags = RDP_LOGON_NORMAL;
	rdp = (rdpRdp *) (inst->rdp);
	s = rdp->settings;
	if (s->bulk_compression)
	{
		connect_flags |= (RDP_LOGON_COMPRESSION | RDP_LOGON_COMPRESSION2);
	}
	if (s->autologin)
	{
		connect_flags |= RDP_LOGON_AUTO;
	}
	if (s->leave_audio)
	{
		connect_flags |= RDP_LOGON_LEAVE_AUDIO;
	}
	for (index = 0; index < s->num_channels; index++)
	{
		s->channels[index].chan_id = MCS_GLOBAL_CHANNEL + 1 + index;
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
#ifdef _WIN32
	read_fds[*read_count] = (void *) (rdp->sec->mcs->iso->tcp->wsa_event);
#else
	read_fds[*read_count] = (void *) (rdp->sec->mcs->iso->tcp->sock);
#endif
	(*read_count)++;
	return 0;
}

/* Process receivable fds, return true if connection should live on */
static int
l_rdp_check_fds(struct rdp_inst * inst)
{
	rdpRdp * rdp;
	RD_BOOL deactivated;
	uint32 ext_disc_reason;
	int rv;

	rdp = (rdpRdp *) (inst->rdp);
#ifdef _WIN32
	WSAResetEvent(rdp->sec->mcs->iso->tcp->wsa_event);
#endif
	rv = 0;
	if (tcp_can_recv(rdp->sec->mcs->iso->tcp->sock, 0))
	{
		if (!rdp_loop(rdp, &deactivated, &ext_disc_reason))
		{
			rv = 1;
		}
	}
	if ((rv != 0) && rdp->redirect)
	{
		rdp->redirect = False;
		rdp_disconnect(rdp);
		if (rdp_reconnect(rdp))
		{
			rv = 0;
		}
	}
	return rv;
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

static int
l_rdp_sync_input(struct rdp_inst * inst, int toggle_flags)
{
	rdpRdp * rdp;

	rdp = (rdpRdp *) (inst->rdp);
	rdp_sync_input(rdp, time(NULL), toggle_flags);
	return 0;
}

static int
l_rdp_channel_data(struct rdp_inst * inst, int chan_id, char * data, int data_size)
{
	rdpRdp * rdp;
	rdpChannels * chan;

	rdp = (rdpRdp *) (inst->rdp);
	chan = rdp->sec->mcs->chan;
	return vchan_send(chan, chan_id, data, data_size);
}

static void
l_rdp_disconnect(struct rdp_inst * inst)
{
	rdpRdp * rdp;

	rdp = (rdpRdp *) (inst->rdp);
	rdp_disconnect(rdp);
}

rdpInst *
freerdp_new(rdpSet * settings)
{
	rdpInst * inst;

	inst = (rdpInst *) xmalloc(sizeof(rdpInst));
	inst->version = FREERDP_INTERFACE_VERSION;
	inst->size = sizeof(rdpInst);
	inst->settings = settings;
	inst->rdp_connect = l_rdp_connect;
	inst->rdp_get_fds = l_rdp_get_fds;
	inst->rdp_check_fds = l_rdp_check_fds;
	inst->rdp_send_input = l_rdp_send_input;
	inst->rdp_sync_input = l_rdp_sync_input;
	inst->rdp_channel_data = l_rdp_channel_data;
	inst->rdp_disconnect = l_rdp_disconnect;
	inst->rdp = rdp_new(settings, inst);
	return inst;
}

void
freerdp_free(rdpInst * inst)
{
	if (inst != NULL)
	{
		rdp_free(inst->rdp);
		xfree(inst);
	}
}
