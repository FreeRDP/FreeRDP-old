
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <freerdp/freerdp.h>
#include "frdp.h"
#include "rdp.h"
#include "secure.h"
#include "mcs.h"
#include "iso.h"
#include "tcp.h"
#include "mem.h"
#include "chan.h"
#include "ext.h"

#define RDP_FROM_INST(_inst) ((rdpRdp *) (_inst->rdp))

void
ui_error(rdpInst * inst, char * format, ...)
{
	char * text1;
	char * text2;
	va_list ap;

	text1 = (char *) xmalloc(1024);
	text2 = (char *) xmalloc(1024);
	va_start(ap, format);
	vsnprintf(text1, 1023, format, ap);
	va_end(ap);
	text1[1023] = 0;
	snprintf(text2, 1023, "ERROR: %s", text1);
	text2[1023] = 0;
	inst->ui_error(inst, text2);
	xfree(text1);
	xfree(text2);
}

void
ui_warning(rdpInst * inst, char * format, ...)
{
	char * text1;
	char * text2;
	va_list ap;

	text1 = (char *) xmalloc(1024);
	text2 = (char *) xmalloc(1024);
	va_start(ap, format);
	vsnprintf(text1, 1023, format, ap);
	va_end(ap);
	text1[1023] = 0;
	snprintf(text2, 1023, "WARNING: %s", text1);
	text2[1023] = 0;
	inst->ui_warning(inst, text2);
	xfree(text1);
	xfree(text2);
}

void
ui_unimpl(rdpInst * inst, char * format, ...)
{
	char * text1;
	char * text2;
	va_list ap;

	text1 = (char *) xmalloc(1024);
	text2 = (char *) xmalloc(1024);
	va_start(ap, format);
	vsnprintf(text1, 1023, format, ap);
	va_end(ap);
	text1[1023] = 0;
	snprintf(text2, 1023, "NOT IMPLEMENTED: %s", text1);
	text2[1023] = 0;
	inst->ui_unimpl(inst, text2);
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
ui_begin_update(rdpInst * inst)
{
	inst->ui_begin_update(inst);
}

void
ui_end_update(rdpInst * inst)
{
	inst->ui_end_update(inst);
}

void
ui_line(rdpInst * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen)
{
	inst->ui_line(inst, opcode, startx, starty, endx, endy, pen);
}

void
ui_rect(rdpInst * inst, int x, int y, int cx, int cy, int color)
{
	inst->ui_rect(inst, x, y, cx, cy, color);
}

void
ui_polygon(rdpInst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point, int npoints,
	   RD_BRUSH * brush, int bgcolor, int fgcolor)
{
	inst->ui_polygon(inst, opcode, fillmode, point, npoints, brush, bgcolor, fgcolor);
}

void
ui_polyline(rdpInst * inst, uint8 opcode, RD_POINT * points, int npoints, RD_PEN * pen)
{
	inst->ui_polyline(inst, opcode, points, npoints, pen);
}

void
ui_ellipse(rdpInst * inst, uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy,
	   RD_BRUSH * brush, int bgcolor, int fgcolor)
{
	inst->ui_ellipse(inst, opcode, fillmode, x, y, cx, cy, brush, bgcolor, fgcolor);
}

void
ui_start_draw_glyphs(rdpInst * inst, int bgcolor, int fgcolor)
{
	inst->ui_start_draw_glyphs(inst, bgcolor, fgcolor);
}

void
ui_draw_glyph(rdpInst * inst, int x, int y, int cx, int cy, RD_HGLYPH glyph)
{
	inst->ui_draw_glyph(inst, x, y, cx, cy, glyph);
}

void
ui_end_draw_glyphs(rdpInst * inst, int x, int y, int cx, int cy)
{
	inst->ui_end_draw_glyphs(inst, x, y, cx, cy);
}

void
ui_desktop_save(rdpInst * inst, uint32 offset, int x, int y, int cx, int cy)
{
	inst->ui_desktop_save(inst, offset, x, y, cx, cy);
}

void
ui_desktop_restore(rdpInst * inst, uint32 offset, int x, int y, int cx, int cy)
{
	inst->ui_desktop_save(inst, offset, x, y, cx, cy);
}

uint32
ui_get_toggle_keys_state(rdpInst * inst)
{
	return inst->ui_get_toggle_keys_state(inst);
}

void
ui_bell(rdpInst * inst)
{
	inst->ui_bell(inst);
}

void
ui_destblt(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy)
{
	inst->ui_destblt(inst, opcode, x, y, cx, cy);
}

void
ui_patblt(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH * brush,
	  int bgcolor, int fgcolor)
{
	inst->ui_patblt(inst, opcode, x, y, cx, cy, brush, bgcolor, fgcolor);
}

void
ui_screenblt(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy)
{
	inst->ui_screenblt(inst, opcode, x, y, cx, cy, srcx, srcy);
}

void
ui_memblt(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src,
	  int srcx, int srcy)
{
	inst->ui_memblt(inst, opcode, x, y, cx, cy, src, srcx, srcy);
}

void
ui_triblt(rdpInst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src,
	  int srcx, int srcy, RD_BRUSH * brush, int bgcolor, int fgcolor)
{
	inst->ui_triblt(inst, opcode, x, y, cx, cy, src, srcx, srcy, brush, bgcolor, fgcolor);
}

RD_HGLYPH
ui_create_glyph(rdpInst * inst, int width, int height, uint8 * data)
{
	return inst->ui_create_glyph(inst, width, height, data);
}

void
ui_destroy_glyph(rdpInst * inst, RD_HGLYPH glyph)
{
	inst->ui_destroy_glyph(inst, glyph);
}

int
ui_select(rdpInst * inst, int rdp_socket)
{
	return inst->ui_select(inst, rdp_socket);
}

void
ui_set_clip(rdpInst * inst, int x, int y, int cx, int cy)
{
	inst->ui_set_clip(inst, x, y, cx, cy);
}

void
ui_reset_clip(rdpInst * inst)
{
	inst->ui_reset_clip(inst);
}

void
ui_resize_window(rdpInst * inst)
{
	inst->ui_resize_window(inst);
}

RD_HCURSOR
ui_create_cursor(rdpInst * inst, unsigned int x, unsigned int y, int width, int height,
		 uint8 * andmask, uint8 * xormask, int bpp)
{
	return inst->ui_create_cursor(inst, x, y, width, height, andmask, xormask, bpp);
}

void
ui_set_cursor(rdpInst * inst, RD_HCURSOR cursor)
{
	inst->ui_set_cursor(inst, cursor);
}

void
ui_set_null_cursor(rdpInst * inst)
{
	inst->ui_set_null_cursor(inst);
}

void
ui_set_default_cursor(rdpInst * inst)
{
	inst->ui_set_default_cursor(inst);
}

void
ui_destroy_cursor(rdpInst * inst, RD_HCURSOR cursor)
{
	inst->ui_destroy_cursor(inst, cursor);
}

RD_HBITMAP
ui_create_bitmap(rdpInst * inst, int width, int height, uint8 * data)
{
	return inst->ui_create_bitmap(inst, width, height, data);
}

void
ui_paint_bitmap(rdpInst * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
	inst->ui_paint_bitmap(inst, x, y, cx, cy, width,  height, data);
}

void
ui_destroy_bitmap(rdpInst * inst, RD_HBITMAP bmp)
{
	inst->ui_destroy_bitmap(inst, bmp);
}

RD_HPALETTE
ui_create_colormap(rdpInst * inst, RD_PALETTE * colors)
{
	return inst->ui_create_colormap(inst, colors);
}

void
ui_move_pointer(rdpInst * inst, int x, int y)
{
	inst->ui_move_pointer(inst, x, y);
}

void
ui_set_colormap(rdpInst * inst, RD_HPALETTE map)
{
	inst->ui_set_colormap(inst, map);
}

RD_HBITMAP
ui_create_surface(rdpInst * inst, int width, int height, RD_HBITMAP old)
{
	return inst->ui_create_surface(inst, width, height, old);
}

void
ui_set_surface(rdpInst * inst, RD_HBITMAP surface)
{
	inst->ui_set_surface(inst, surface);
}

void
ui_destroy_surface(rdpInst * inst, RD_HBITMAP surface)
{
	inst->ui_destroy_surface(inst, surface);
}

void
ui_channel_data(rdpInst * inst, int chan_id, char * data, int data_size,
		int flags, int total_size)
{
	inst->ui_channel_data(inst, chan_id, data, data_size, flags, total_size);
}

RD_BOOL
ui_authenticate(rdpInst * inst)
{
	return inst->ui_authenticate(inst);
}

/* returns error */
static int
l_rdp_connect(rdpInst * inst)
{
	rdpRdp * rdp;
	int index;

	rdp = RDP_FROM_INST(inst);
	for (index = 0; index < rdp->settings->num_channels; index++)
	{
		rdp->settings->channels[index].chan_id = MCS_GLOBAL_CHANNEL + 1 + index;
	}
	ext_pre_connect(rdp->ext);
	if (rdp_connect(rdp))
	{
		ext_post_connect(rdp->ext);
		return 0;
	}
	return 1;
}

static int
l_rdp_get_fds(rdpInst * inst, void ** read_fds, int * read_count,
	void ** write_fds, int * write_count)
{
	rdpRdp * rdp;

	rdp = RDP_FROM_INST(inst);
#ifdef _WIN32
	read_fds[*read_count] = (void *) (rdp->sec->mcs->iso->tcp->wsa_event);
#else
	read_fds[*read_count] = (void *)(long) (rdp->sec->mcs->iso->tcp->sock);
#endif
	(*read_count)++;
	return 0;
}

/* Process receivable fds, return true if connection should live on */
static int
l_rdp_check_fds(rdpInst * inst)
{
	rdpRdp * rdp;
	RD_BOOL deactivated;
	int rv;

	rdp = RDP_FROM_INST(inst);
#ifdef _WIN32
	WSAResetEvent(rdp->sec->mcs->iso->tcp->wsa_event);
#endif
	rv = 0;
	if (tcp_can_recv(rdp->sec->mcs->iso->tcp->sock, 0))
	{
		if (!rdp_loop(rdp, &deactivated))
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
l_rdp_send_input(rdpInst * inst, int message_type, int device_flags,
	int param1, int param2)
{
	rdpRdp * rdp;
	rdp = RDP_FROM_INST(inst);
	rdp_send_input(rdp, time(NULL), message_type, device_flags, param1, param2);
	return 0;
}

static int
l_rdp_sync_input(rdpInst * inst, int toggle_flags)
{
	rdpRdp * rdp;
	rdp = (rdpRdp *) (inst->rdp);
	rdp_sync_input(rdp, time(NULL), toggle_flags);
	return 0;
}

static int
l_rdp_channel_data(rdpInst * inst, int chan_id, char * data, int data_size)
{
	rdpRdp * rdp;
	rdpChannels * chan;

	rdp = RDP_FROM_INST(inst);
	chan = rdp->sec->mcs->chan;
	return vchan_send(chan, chan_id, data, data_size);
}

static void
l_rdp_disconnect(rdpInst * inst)
{
	rdpRdp * rdp;

	rdp = RDP_FROM_INST(inst);
	rdp_disconnect(rdp);
}

RD_BOOL
freerdp_global_init(void)
{
	return rdp_global_init();
}

void
freerdp_global_finish(void)
{
	rdp_global_finish();
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
	inst->rdp = (void *) rdp_new(settings, inst);
	return inst;
}

void
freerdp_free(rdpInst * inst)
{
	rdpRdp * rdp;

	if (inst != NULL)
	{
		rdp = RDP_FROM_INST(inst);
		rdp_free(rdp);
		xfree(inst);
	}
}
