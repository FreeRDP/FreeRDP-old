/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   User interface services - X Window System
   Copyright (C) Matthew Chapman 1999-2008
   Copyright 2007-2008 Pierre Ossman <ossman@cendio.se> for Cendio AB

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

#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <strings.h>
#include <stdarg.h> /* va_list va_start va_end */

#include "rdesktop.h"
#include "xkbkeymap.h"
#include "rdp.h"
#include "rdpset.h"
#include "mem.h"

extern rdpRdp * g_rdp; /* in rdesktop.c */
extern rdpSet g_settings; /* in rdesktop.c */

extern int g_xpos;
extern int g_ypos;
extern int g_pos;
extern RD_BOOL g_sendmotion;
extern RD_BOOL g_fullscreen;
extern RD_BOOL g_grab_keyboard;
extern RD_BOOL g_hide_decorations;
extern char g_title[];
extern int g_win_button_size;

Display *g_display;
Time g_last_gesturetime;
static int g_x_socket;
static Screen *g_screen;
Window g_wnd;

extern uint32_t g_embed_wnd;
RD_BOOL g_enable_compose = False;
RD_BOOL g_Unobscured;		/* used for screenblt */
static GC g_gc = NULL;
static GC g_create_bitmap_gc = NULL;
static GC g_create_glyph_gc = NULL;
static XRectangle g_clip_rectangle;
static Visual *g_visual;
/* Color depth of the X11 visual of our window (e.g. 24 for True Color R8G8B visual).
   This may be 32 for R8G8B8 visuals, and then the rest of the bits are undefined
   as far as we're concerned. */
static int g_depth;
/* Bits-per-Pixel of the pixmaps we'll be using to draw on our window.
   This may be larger than g_depth, in which case some of the bits would
   be kept solely for alignment (e.g. 32bpp pixmaps on a 24bpp visual). */
static int g_bpp;
static XIM g_IM;
static XIC g_IC;
static XModifierKeymap *g_mod_map;
/* Maps logical (xmodmap -pp) pointing device buttons (0-based) back
   to physical (1-based) indices. */
static unsigned char g_pointer_log_to_phys_map[32];
static Cursor g_current_cursor;
static RD_HCURSOR g_null_cursor = NULL;
static Atom g_protocol_atom, g_kill_atom;
extern Atom g_net_wm_state_atom;
extern Atom g_net_wm_desktop_atom;
static RD_BOOL g_focused;
static RD_BOOL g_mouse_in_wnd;
/* Indicates that:
   1) visual has 15, 16 or 24 depth and the same color channel masks
      as its RDP equivalent (implies X server is LE),
   2) host is LE
   This will trigger an optimization whose real value is questionable.
*/
static RD_BOOL g_compatible_arch;
/* Indicates whether RDP's bitmaps and our XImages have the same
   binary format. If so, we can avoid an expensive translation.
   Note that this can be true when g_compatible_arch is false,
   e.g.:
   
     RDP(LE) <-> host(BE) <-> X-Server(LE)
     
   ('host' is the machine running rdesktop; the host simply memcpy's
    so its endianess doesn't matter)
 */
static RD_BOOL g_no_translate_image = False;

/* endianness */
static RD_BOOL g_host_be;
static RD_BOOL g_xserver_be;
static int g_red_shift_r, g_blue_shift_r, g_green_shift_r;
static int g_red_shift_l, g_blue_shift_l, g_green_shift_l;

/* software backing store */
extern RD_BOOL g_ownbackstore;
static Pixmap g_backstore = 0;

/* Moving in single app mode */
static RD_BOOL g_moving_wnd;
static int g_move_x_offset = 0;
static int g_move_y_offset = 0;
static RD_BOOL g_using_full_workarea = False;

#ifdef WITH_RDPSND
extern RD_BOOL g_rdpsnd;
#endif

/* MWM decorations */
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define PROP_MOTIF_WM_HINTS_ELEMENTS    5
typedef struct
{
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long inputMode;
	unsigned long status;
}
PropMotifWmHints;

typedef struct
{
	uint32_t red;
	uint32_t green;
	uint32_t blue;
}
PixelColour;

#define FILL_RECTANGLE(x,y,cx,cy)\
{ \
	XFillRectangle(g_display, g_wnd, g_gc, x, y, cx, cy); \
	if (g_ownbackstore) \
		XFillRectangle(g_display, g_backstore, g_gc, x, y, cx, cy); \
}

#define FILL_RECTANGLE_BACKSTORE(x,y,cx,cy)\
{ \
	XFillRectangle(g_display, g_ownbackstore ? g_backstore : g_wnd, g_gc, x, y, cx, cy); \
}

#define FILL_POLYGON(p,np)\
{ \
	XFillPolygon(g_display, g_wnd, g_gc, p, np, Complex, CoordModePrevious); \
	if (g_ownbackstore) \
		XFillPolygon(g_display, g_backstore, g_gc, p, np, Complex, CoordModePrevious); \
}

#define DRAW_ELLIPSE(x,y,cx,cy,m)\
{ \
	switch (m) \
	{ \
		case 0:	/* Outline */ \
			XDrawArc(g_display, g_wnd, g_gc, x, y, cx, cy, 0, 360*64); \
			if (g_ownbackstore) \
				XDrawArc(g_display, g_backstore, g_gc, x, y, cx, cy, 0, 360*64); \
			break; \
		case 1: /* Filled */ \
			XFillArc(g_display, g_wnd, g_gc, x, y, cx, cy, 0, 360*64); \
			if (g_ownbackstore) \
				XFillArc(g_display, g_backstore, g_gc, x, y, cx, cy, 0, 360*64); \
			break; \
	} \
}

/* colour maps */
extern RD_BOOL g_owncolmap;
static Colormap g_xcolmap;
static uint32_t *g_colmap = NULL;

#define TRANSLATE(col)		( g_settings.server_depth != 8 ? translate_colour(col) : g_owncolmap ? col : g_colmap[col] )
#define SET_FOREGROUND(col)	XSetForeground(g_display, g_gc, TRANSLATE(col));
#define SET_BACKGROUND(col)	XSetBackground(g_display, g_gc, TRANSLATE(col));

static int rop2_map[] = {
	GXclear,		/* 0 */
	GXnor,			/* DPon */
	GXandInverted,		/* DPna */
	GXcopyInverted,		/* Pn */
	GXandReverse,		/* PDna */
	GXinvert,		/* Dn */
	GXxor,			/* DPx */
	GXnand,			/* DPan */
	GXand,			/* DPa */
	GXequiv,		/* DPxn */
	GXnoop,			/* D */
	GXorInverted,		/* DPno */
	GXcopy,			/* P */
	GXorReverse,		/* PDno */
	GXor,			/* DPo */
	GXset			/* 1 */
};

#define SET_FUNCTION(rop2)	{ if (rop2 != ROP2_COPY) XSetFunction(g_display, g_gc, rop2_map[rop2]); }
#define RESET_FUNCTION(rop2)	{ if (rop2 != ROP2_COPY) XSetFunction(g_display, g_gc, GXcopy); }

static uint8 g_deskcache[0x38400 * 4];

/* Retrieve desktop data */
static uint8 *
xcache_get_desktop(uint32 offset, int cx, int cy, int bytes_per_pixel)
{
	int length = cx * cy * bytes_per_pixel;

	if (offset > sizeof(g_deskcache))
		offset = 0;

	if ((offset + length) <= sizeof(g_deskcache))
	{
		return &(g_deskcache[offset]);
	}

	ui_error(NULL, "get desktop %d:%d\n", offset, length);
	return NULL;
}

/* Store desktop data */
static void
xcache_put_desktop(uint32 offset, int cx, int cy, int scanline,
		   int bytes_per_pixel, uint8 * data)
{
	int length = cx * cy * bytes_per_pixel;

	if (offset > sizeof(g_deskcache))
		offset = 0;

	if ((offset + length) <= sizeof(g_deskcache))
	{
		cx *= bytes_per_pixel;
		while (cy--)
		{
			memcpy(&(g_deskcache[offset]), data, cx);
			data += scanline;
			offset += cx;
		}
	}
	else
	{
		ui_error(NULL, "put desktop %d:%d\n", offset, length);
	}
}

static void
mwm_hide_decorations(Window wnd)
{
	PropMotifWmHints motif_hints;
	Atom hintsatom;

	/* setup the property */
	motif_hints.flags = MWM_HINTS_DECORATIONS;
	motif_hints.decorations = 0;

	/* get the atom for the property */
	hintsatom = XInternAtom(g_display, "_MOTIF_WM_HINTS", False);
	if (!hintsatom)
	{
		ui_warning(NULL, "Failed to get atom _MOTIF_WM_HINTS: probably your window "
			   "manager does not support MWM hints\n");
		return;
	}

	XChangeProperty(g_display, wnd, hintsatom, hintsatom, 32, PropModeReplace,
			(unsigned char *) &motif_hints, PROP_MOTIF_WM_HINTS_ELEMENTS);

}

#define SPLITCOLOUR15(colour, rv) \
{ \
	rv.red = ((colour >> 7) & 0xf8) | ((colour >> 12) & 0x7); \
	rv.green = ((colour >> 2) & 0xf8) | ((colour >> 8) & 0x7); \
	rv.blue = ((colour << 3) & 0xf8) | ((colour >> 2) & 0x7); \
}

#define SPLITCOLOUR16(colour, rv) \
{ \
	rv.red = ((colour >> 8) & 0xf8) | ((colour >> 13) & 0x7); \
	rv.green = ((colour >> 3) & 0xfc) | ((colour >> 9) & 0x3); \
	rv.blue = ((colour << 3) & 0xf8) | ((colour >> 2) & 0x7); \
} \

#define SPLITCOLOUR24(colour, rv) \
{ \
	rv.blue = (colour & 0xff0000) >> 16; \
	rv.green = (colour & 0x00ff00) >> 8; \
	rv.red = (colour & 0x0000ff); \
}

#define MAKECOLOUR(pc) \
	((pc.red >> g_red_shift_r) << g_red_shift_l) \
		| ((pc.green >> g_green_shift_r) << g_green_shift_l) \
		| ((pc.blue >> g_blue_shift_r) << g_blue_shift_l) \

#define BSWAP16(x) { x = (((x & 0xff) << 8) | (x >> 8)); }
#define BSWAP24(x) { x = (((x & 0xff) << 16) | (x >> 16) | (x & 0xff00)); }
#define BSWAP32(x) { x = (((x & 0xff00ff) << 8) | ((x >> 8) & 0xff00ff)); \
			x = (x << 16) | (x >> 16); }

/* The following macros output the same octet sequences
   on both BE and LE hosts: */

#define BOUT16(o, x) { *(o++) = x >> 8; *(o++) = x; }
#define BOUT24(o, x) { *(o++) = x >> 16; *(o++) = x >> 8; *(o++) = x; }
#define BOUT32(o, x) { *(o++) = x >> 24; *(o++) = x >> 16; *(o++) = x >> 8; *(o++) = x; }
#define LOUT16(o, x) { *(o++) = x; *(o++) = x >> 8; }
#define LOUT24(o, x) { *(o++) = x; *(o++) = x >> 8; *(o++) = x >> 16; }
#define LOUT32(o, x) { *(o++) = x; *(o++) = x >> 8; *(o++) = x >> 16; *(o++) = x >> 24; }

static uint32_t
translate_colour(uint32_t colour)
{
	PixelColour pc;
	switch (g_settings.server_depth)
	{
		case 15:
			SPLITCOLOUR15(colour, pc);
			break;
		case 16:
			SPLITCOLOUR16(colour, pc);
			break;
		case 24:
		case 32:
			SPLITCOLOUR24(colour, pc);
			break;
		default:
			/* Avoid warning */
			pc.red = 0;
			pc.green = 0;
			pc.blue = 0;
			break;
	}
	return MAKECOLOUR(pc);
}

/* indent is confused by UNROLL8 */
/* *INDENT-OFF* */

/* repeat and unroll, similar to bitmap.c */
/* potentialy any of the following translate */
/* functions can use repeat but just doing */
/* the most common ones */

#define UNROLL8(stm) { stm stm stm stm stm stm stm stm }
/* 2 byte output repeat */
#define REPEAT2(stm) \
{ \
	while (out <= end - 8 * 2) \
		UNROLL8(stm) \
	while (out < end) \
		{ stm } \
}
/* 3 byte output repeat */
#define REPEAT3(stm) \
{ \
	while (out <= end - 8 * 3) \
		UNROLL8(stm) \
	while (out < end) \
		{ stm } \
}
/* 4 byte output repeat */
#define REPEAT4(stm) \
{ \
	while (out <= end - 8 * 4) \
		UNROLL8(stm) \
	while (out < end) \
		{ stm } \
}
/* *INDENT-ON* */

static void
translate8to8(const uint8_t * data, uint8_t * out, uint8_t * end)
{
	while (out < end)
		*(out++) = (uint8_t) g_colmap[*(data++)];
}

static void
translate8to16(const uint8_t * data, uint8_t * out, uint8_t * end)
{
	uint16_t value;

	if (g_compatible_arch)
	{
		/* *INDENT-OFF* */
		REPEAT2
		(
			*((uint16_t *) out) = g_colmap[*(data++)];
			out += 2;
		)
		/* *INDENT-ON* */
	}
	else if (g_xserver_be)
	{
		while (out < end)
		{
			value = (uint16_t) g_colmap[*(data++)];
			BOUT16(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			value = (uint16_t) g_colmap[*(data++)];
			LOUT16(out, value);
		}
	}
}

/* little endian - conversion happens when colourmap is built */
static void
translate8to24(const uint8_t * data, uint8_t * out, uint8_t * end)
{
	uint32_t value;

	if (g_compatible_arch)
	{
		while (out < end)
		{
			value = g_colmap[*(data++)];
			BOUT24(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			value = g_colmap[*(data++)];
			LOUT24(out, value);
		}
	}
}

static void
translate8to32(const uint8_t * data, uint8_t * out, uint8_t * end)
{
	uint32_t value;

	if (g_compatible_arch)
	{
		/* *INDENT-OFF* */
		REPEAT4
		(
			*((uint32_t *) out) = g_colmap[*(data++)];
			out += 4;
		)
		/* *INDENT-ON* */
	}
	else if (g_xserver_be)
	{
		while (out < end)
		{
			value = g_colmap[*(data++)];
			BOUT32(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			value = g_colmap[*(data++)];
			LOUT32(out, value);
		}
	}
}

static void
translate15to16(const uint16_t * data, uint8_t * out, uint8_t * end)
{
	uint16_t pixel;
	uint16_t value;
	PixelColour pc;

	if (g_xserver_be)
	{
		while (out < end)
		{
			pixel = *(data++);
			if (g_host_be)
			{
				BSWAP16(pixel);
			}
			SPLITCOLOUR15(pixel, pc);
			value = MAKECOLOUR(pc);
			BOUT16(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			pixel = *(data++);
			if (g_host_be)
			{
				BSWAP16(pixel);
			}
			SPLITCOLOUR15(pixel, pc);
			value = MAKECOLOUR(pc);
			LOUT16(out, value);
		}
	}
}

static void
translate15to24(const uint16_t * data, uint8_t * out, uint8_t * end)
{
	uint32_t value;
	uint16_t pixel;
	PixelColour pc;

	if (g_compatible_arch)
	{
		/* *INDENT-OFF* */
		REPEAT3
		(
			pixel = *(data++);
			SPLITCOLOUR15(pixel, pc);
			*(out++) = pc.blue;
			*(out++) = pc.green;
			*(out++) = pc.red;
		)
		/* *INDENT-ON* */
	}
	else if (g_xserver_be)
	{
		while (out < end)
		{
			pixel = *(data++);
			if (g_host_be)
			{
				BSWAP16(pixel);
			}
			SPLITCOLOUR15(pixel, pc);
			value = MAKECOLOUR(pc);
			BOUT24(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			pixel = *(data++);
			if (g_host_be)
			{
				BSWAP16(pixel);
			}
			SPLITCOLOUR15(pixel, pc);
			value = MAKECOLOUR(pc);
			LOUT24(out, value);
		}
	}
}

static void
translate15to32(const uint16_t * data, uint8_t * out, uint8_t * end)
{
	uint16_t pixel;
	uint32_t value;
	PixelColour pc;

	if (g_compatible_arch)
	{
		/* *INDENT-OFF* */
		REPEAT4
		(
			pixel = *(data++);
			SPLITCOLOUR15(pixel, pc);
			*(out++) = pc.blue;
			*(out++) = pc.green;
			*(out++) = pc.red;
			*(out++) = 0;
		)
		/* *INDENT-ON* */
	}
	else if (g_xserver_be)
	{
		while (out < end)
		{
			pixel = *(data++);
			if (g_host_be)
			{
				BSWAP16(pixel);
			}
			SPLITCOLOUR15(pixel, pc);
			value = MAKECOLOUR(pc);
			BOUT32(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			pixel = *(data++);
			if (g_host_be)
			{
				BSWAP16(pixel);
			}
			SPLITCOLOUR15(pixel, pc);
			value = MAKECOLOUR(pc);
			LOUT32(out, value);
		}
	}
}

static void
translate16to16(const uint16_t * data, uint8_t * out, uint8_t * end)
{
	uint16_t pixel;
	uint16_t value;
	PixelColour pc;

	if (g_xserver_be)
	{
		if (g_host_be)
		{
			while (out < end)
			{
				pixel = *(data++);
				BSWAP16(pixel);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				BOUT16(out, value);
			}
		}
		else
		{
			while (out < end)
			{
				pixel = *(data++);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				BOUT16(out, value);
			}
		}
	}
	else
	{
		if (g_host_be)
		{
			while (out < end)
			{
				pixel = *(data++);
				BSWAP16(pixel);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				LOUT16(out, value);
			}
		}
		else
		{
			while (out < end)
			{
				pixel = *(data++);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				LOUT16(out, value);
			}
		}
	}
}

static void
translate16to24(const uint16_t * data, uint8_t * out, uint8_t * end)
{
	uint32_t value;
	uint16_t pixel;
	PixelColour pc;

	if (g_compatible_arch)
	{
		/* *INDENT-OFF* */
		REPEAT3
		(
			pixel = *(data++);
			SPLITCOLOUR16(pixel, pc);
			*(out++) = pc.blue;
			*(out++) = pc.green;
			*(out++) = pc.red;
		)
		/* *INDENT-ON* */
	}
	else if (g_xserver_be)
	{
		if (g_host_be)
		{
			while (out < end)
			{
				pixel = *(data++);
				BSWAP16(pixel);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				BOUT24(out, value);
			}
		}
		else
		{
			while (out < end)
			{
				pixel = *(data++);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				BOUT24(out, value);
			}
		}
	}
	else
	{
		if (g_host_be)
		{
			while (out < end)
			{
				pixel = *(data++);
				BSWAP16(pixel);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				LOUT24(out, value);
			}
		}
		else
		{
			while (out < end)
			{
				pixel = *(data++);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				LOUT24(out, value);
			}
		}
	}
}

static void
translate16to32(const uint16_t * data, uint8_t * out, uint8_t * end)
{
	uint16_t pixel;
	uint32_t value;
	PixelColour pc;

	if (g_compatible_arch)
	{
		/* *INDENT-OFF* */
		REPEAT4
		(
			pixel = *(data++);
			SPLITCOLOUR16(pixel, pc);
			*(out++) = pc.blue;
			*(out++) = pc.green;
			*(out++) = pc.red;
			*(out++) = 0;
		)
		/* *INDENT-ON* */
	}
	else if (g_xserver_be)
	{
		if (g_host_be)
		{
			while (out < end)
			{
				pixel = *(data++);
				BSWAP16(pixel);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				BOUT32(out, value);
			}
		}
		else
		{
			while (out < end)
			{
				pixel = *(data++);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				BOUT32(out, value);
			}
		}
	}
	else
	{
		if (g_host_be)
		{
			while (out < end)
			{
				pixel = *(data++);
				BSWAP16(pixel);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				LOUT32(out, value);
			}
		}
		else
		{
			while (out < end)
			{
				pixel = *(data++);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				LOUT32(out, value);
			}
		}
	}
}

static void
translate24to16(const uint8_t * data, uint8_t * out, uint8_t * end)
{
	uint32_t pixel = 0;
	uint16_t value;
	PixelColour pc;

	while (out < end)
	{
		pixel = *(data++) << 16;
		pixel |= *(data++) << 8;
		pixel |= *(data++);
		SPLITCOLOUR24(pixel, pc);
		value = MAKECOLOUR(pc);
		if (g_xserver_be)
		{
			BOUT16(out, value);
		}
		else
		{
			LOUT16(out, value);
		}
	}
}

static void
translate24to24(const uint8_t * data, uint8_t * out, uint8_t * end)
{
	uint32_t pixel;
	uint32_t value;
	PixelColour pc;

	if (g_xserver_be)
	{
		while (out < end)
		{
			pixel = *(data++) << 16;
			pixel |= *(data++) << 8;
			pixel |= *(data++);
			SPLITCOLOUR24(pixel, pc);
			value = MAKECOLOUR(pc);
			BOUT24(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			pixel = *(data++) << 16;
			pixel |= *(data++) << 8;
			pixel |= *(data++);
			SPLITCOLOUR24(pixel, pc);
			value = MAKECOLOUR(pc);
			LOUT24(out, value);
		}
	}
}

static void
translate24to32(const uint8_t * data, uint8_t * out, uint8_t * end)
{
	uint32_t pixel;
	uint32_t value;
	PixelColour pc;

	if (g_compatible_arch)
	{
		/* *INDENT-OFF* */
#ifdef NEED_ALIGN
		REPEAT4
		(
			*(out++) = *(data++);
			*(out++) = *(data++);
			*(out++) = *(data++);
			*(out++) = 0;
		)
#else
		REPEAT4
		(
		 /* Only read 3 bytes. Reading 4 bytes means reading beyond buffer. */
		 *((uint32_t *) out) = *((uint16_t *) data) + (*((uint8_t *) data + 2) << 16);
		 out += 4;
		 data += 3;
		)
#endif
		/* *INDENT-ON* */
	}
	else if (g_xserver_be)
	{
		while (out < end)
		{
			pixel = *(data++) << 16;
			pixel |= *(data++) << 8;
			pixel |= *(data++);
			SPLITCOLOUR24(pixel, pc);
			value = MAKECOLOUR(pc);
			BOUT32(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			pixel = *(data++) << 16;
			pixel |= *(data++) << 8;
			pixel |= *(data++);
			SPLITCOLOUR24(pixel, pc);
			value = MAKECOLOUR(pc);
			LOUT32(out, value);
		}
	}
}

static uint8_t *
translate_image(int width, int height, uint8_t * data)
{
	int size;
	uint8_t *out;
	uint8_t *end;

	/*
	   If RDP depth and X Visual depths match,
	   and arch(endian) matches, no need to translate:
	   just return data.
	   Note: select_visual should've already ensured g_no_translate
	   is only set for compatible depths, but the RDP depth might've
	   changed during connection negotiations.
	 */

	/* todo */
	if (g_settings.server_depth == 32 && g_depth == 24)
	{
		return data;
	}

	if (g_no_translate_image)
	{
		if ((g_depth == 15 && g_settings.server_depth == 15) ||
		    (g_depth == 16 && g_settings.server_depth == 16) ||
		    (g_depth == 24 && g_settings.server_depth == 24))
			return data;
	}

	size = width * height * (g_bpp / 8);
	out = (uint8_t *) xmalloc(size);
	end = out + size;

	switch (g_settings.server_depth)
	{
		case 24:
			switch (g_bpp)
			{
				case 32:
					translate24to32(data, out, end);
					break;
				case 24:
					translate24to24(data, out, end);
					break;
				case 16:
					translate24to16(data, out, end);
					break;
			}
			break;
		case 16:
			switch (g_bpp)
			{
				case 32:
					translate16to32((uint16_t *) data, out, end);
					break;
				case 24:
					translate16to24((uint16_t *) data, out, end);
					break;
				case 16:
					translate16to16((uint16_t *) data, out, end);
					break;
			}
			break;
		case 15:
			switch (g_bpp)
			{
				case 32:
					translate15to32((uint16_t *) data, out, end);
					break;
				case 24:
					translate15to24((uint16_t *) data, out, end);
					break;
				case 16:
					translate15to16((uint16_t *) data, out, end);
					break;
			}
			break;
		case 8:
			switch (g_bpp)
			{
				case 8:
					translate8to8(data, out, end);
					break;
				case 16:
					translate8to16(data, out, end);
					break;
				case 24:
					translate8to24(data, out, end);
					break;
				case 32:
					translate8to32(data, out, end);
					break;
			}
			break;
	}
	return out;
}

static void
xwin_refresh_pointer_map(void)
{
	unsigned char phys_to_log_map[sizeof(g_pointer_log_to_phys_map)];
	int i, pointer_buttons;

	pointer_buttons = XGetPointerMapping(g_display, phys_to_log_map, sizeof(phys_to_log_map));
	if (pointer_buttons > sizeof(phys_to_log_map))
		pointer_buttons = sizeof(phys_to_log_map);

	/* if multiple physical buttons map to the same logical button, then
	 * use the lower numbered physical one */
	for (i = pointer_buttons - 1; i >= 0; i--)
	{
		/* a user could specify arbitrary values for the logical button
		 * number, ignore any that are abnormally large */
		if (phys_to_log_map[i] > sizeof(g_pointer_log_to_phys_map))
			continue;
		g_pointer_log_to_phys_map[phys_to_log_map[i] - 1] = i + 1;
	}
}

RD_BOOL
get_key_state(unsigned int state, uint32_t keysym)
{
	int modifierpos, key, keysymMask = 0;
	int offset;

	KeyCode keycode = XKeysymToKeycode(g_display, keysym);

	if (keycode == NoSymbol)
		return False;

	for (modifierpos = 0; modifierpos < 8; modifierpos++)
	{
		offset = g_mod_map->max_keypermod * modifierpos;

		for (key = 0; key < g_mod_map->max_keypermod; key++)
		{
			if (g_mod_map->modifiermap[offset + key] == keycode)
				keysymMask |= 1 << modifierpos;
		}
	}

	return (state & keysymMask) ? True : False;
}

static void
calculate_shifts(uint32_t mask, int *shift_r, int *shift_l)
{
	*shift_l = ffs(mask) - 1;
	mask >>= *shift_l;
	*shift_r = 8 - ffs(mask & ~(mask >> 1));
}

/* Given a mask of a colour channel (e.g. XVisualInfo.red_mask),
   calculates the bits-per-pixel of this channel (a.k.a. colour weight).
 */
static unsigned
calculate_mask_weight(uint32_t mask)
{
	unsigned weight = 0;
	do
	{
		weight += (mask & 1);
	}
	while (mask >>= 1);
	return weight;
}

static RD_BOOL
select_visual(int screen_num)
{
	XPixmapFormatValues *pfm;
	int pixmap_formats_count, visuals_count;
	XVisualInfo *vmatches = NULL;
	XVisualInfo template;
	int i;
	unsigned red_weight, blue_weight, green_weight;

	red_weight = blue_weight = green_weight = 0;

	if (g_settings.server_depth == -1)
	{
		g_settings.server_depth = DisplayPlanes(g_display, DefaultScreen(g_display));
	}

	pfm = XListPixmapFormats(g_display, &pixmap_formats_count);
	if (pfm == NULL)
	{
		ui_error(NULL, "Unable to get list of pixmap formats from display.\n");
		XCloseDisplay(g_display);
		return False;
	}

	/* Search for best TrueColor visual */
	template.class = TrueColor;
	template.screen = screen_num;
	vmatches =
		XGetVisualInfo(g_display, VisualClassMask | VisualScreenMask, &template,
			       &visuals_count);
	g_visual = NULL;
	g_no_translate_image = False;
	g_compatible_arch = False;
	if (vmatches != NULL)
	{
		for (i = 0; i < visuals_count; ++i)
		{
			XVisualInfo *visual_info = &vmatches[i];
			RD_BOOL can_translate_to_bpp = False;
			int j;

			/* Try to find a no-translation visual that'll
			   allow us to use RDP bitmaps directly as ZPixmaps. */
			if (!g_xserver_be && (((visual_info->depth == 15) &&
					       /* R5G5B5 */
					       (visual_info->red_mask == 0x7c00) &&
					       (visual_info->green_mask == 0x3e0) &&
					       (visual_info->blue_mask == 0x1f)) ||
					      ((visual_info->depth == 16) &&
					       /* R5G6B5 */
					       (visual_info->red_mask == 0xf800) &&
					       (visual_info->green_mask == 0x7e0) &&
					       (visual_info->blue_mask == 0x1f)) ||
					      ((visual_info->depth == 24) &&
					       /* R8G8B8 */
					       (visual_info->red_mask == 0xff0000) &&
					       (visual_info->green_mask == 0xff00) &&
					       (visual_info->blue_mask == 0xff))))
			{
				g_visual = visual_info->visual;
				g_depth = visual_info->depth;
				g_compatible_arch = !g_host_be;
				g_no_translate_image = (visual_info->depth == g_settings.server_depth);
				if (g_no_translate_image)
					/* We found the best visual */
					break;
			}
			else
			{
				g_compatible_arch = False;
			}

			if (visual_info->depth > 24)
			{
				/* Avoid 32-bit visuals and likes like the plague.
				   They're either untested or proven to work bad
				   (e.g. nvidia's Composite 32-bit visual).
				   Most implementation offer a 24-bit visual anyway. */
				continue;
			}

			/* Only care for visuals, for whose BPPs (not depths!)
			   we have a translateXtoY function. */
			for (j = 0; j < pixmap_formats_count; ++j)
			{
				if (pfm[j].depth == visual_info->depth)
				{
					if ((pfm[j].bits_per_pixel == 16) ||
					    (pfm[j].bits_per_pixel == 24) ||
					    (pfm[j].bits_per_pixel == 32))
					{
						can_translate_to_bpp = True;
					}
					break;
				}
			}

			/* Prefer formats which have the most colour depth.
			   We're being truly aristocratic here, minding each
			   weight on its own. */
			if (can_translate_to_bpp)
			{
				unsigned vis_red_weight =
					calculate_mask_weight(visual_info->red_mask);
				unsigned vis_green_weight =
					calculate_mask_weight(visual_info->green_mask);
				unsigned vis_blue_weight =
					calculate_mask_weight(visual_info->blue_mask);
				if ((vis_red_weight >= red_weight)
				    && (vis_green_weight >= green_weight)
				    && (vis_blue_weight >= blue_weight))
				{
					red_weight = vis_red_weight;
					green_weight = vis_green_weight;
					blue_weight = vis_blue_weight;
					g_visual = visual_info->visual;
					g_depth = visual_info->depth;
				}
			}
		}
		XFree(vmatches);
	}

	if (g_visual != NULL)
	{
		g_owncolmap = False;
		calculate_shifts(g_visual->red_mask, &g_red_shift_r, &g_red_shift_l);
		calculate_shifts(g_visual->green_mask, &g_green_shift_r, &g_green_shift_l);
		calculate_shifts(g_visual->blue_mask, &g_blue_shift_r, &g_blue_shift_l);
	}
	else
	{
		template.class = PseudoColor;
		template.depth = 8;
		template.colormap_size = 256;
		vmatches =
			XGetVisualInfo(g_display,
				       VisualClassMask | VisualDepthMask | VisualColormapSizeMask,
				       &template, &visuals_count);
		if (vmatches == NULL)
		{
			ui_error(NULL, "No usable TrueColor or PseudoColor visuals on this display.\n");
			XCloseDisplay(g_display);
			XFree(pfm);
			return False;
		}

		/* we use a colourmap, so the default visual should do */
		g_owncolmap = True;
		g_visual = vmatches[0].visual;
		g_depth = vmatches[0].depth;
	}

	g_bpp = 0;
	for (i = 0; i < pixmap_formats_count; ++i)
	{
		XPixmapFormatValues *pf = &pfm[i];
		if (pf->depth == g_depth)
		{
			g_bpp = pf->bits_per_pixel;

			if (g_no_translate_image)
			{
				switch (g_settings.server_depth)
				{
					case 15:
					case 16:
						if (g_bpp != 16)
							g_no_translate_image = False;
						break;
					case 24:
						/* Yes, this will force image translation
						   on most modern servers which use 32 bits
						   for R8G8B8. */
						if (g_bpp != 24)
							g_no_translate_image = False;
						break;
					default:
						g_no_translate_image = False;
						break;
				}
			}

			/* Pixmap formats list is a depth-to-bpp mapping --
			   there's just a single entry for every depth,
			   so we can safely break here */
			break;
		}
	}
	XFree(pfm);
	pfm = NULL;
	return True;
}

static XErrorHandler g_old_error_handler;
static RD_BOOL g_error_expected = False;

static int
error_handler(Display * dpy, XErrorEvent * eev)
{
	if (g_error_expected)
		return 0;

	return g_old_error_handler(dpy, eev);
}

RD_BOOL
ui_init(void)
{
	int screen_num;

	g_display = XOpenDisplay(NULL);
	if (g_display == NULL)
	{
		ui_error(NULL, "Failed to open display: %s\n", XDisplayName(NULL));
		return False;
	}

	{
		uint16_t endianess_test = 1;
		g_host_be = !(RD_BOOL) (*(uint8_t *) (&endianess_test));
	}

	g_old_error_handler = XSetErrorHandler(error_handler);
	g_xserver_be = (ImageByteOrder(g_display) == MSBFirst);
	screen_num = DefaultScreen(g_display);
	g_x_socket = ConnectionNumber(g_display);
	g_screen = ScreenOfDisplay(g_display, screen_num);
	g_depth = DefaultDepthOfScreen(g_screen);

	if (!select_visual(screen_num))
		return False;

	if (g_no_translate_image)
	{
		DEBUG(("Performance optimization possible: avoiding image translation (colour depth conversion).\n"));
	}

	if (g_settings.server_depth > g_bpp)
	{
		ui_warning(NULL, "Remote desktop colour depth %d higher than display colour "
			   "depth %d.\n", g_settings.server_depth, g_bpp);
	}

	DEBUG(("RDP depth: %d, display depth: %d, display bpp: %d, X server BE: %d, host BE: %d\n",
	       g_settings.server_depth, g_depth, g_bpp, g_xserver_be, g_host_be));

	if (!g_owncolmap)
	{
		g_xcolmap =
			XCreateColormap(g_display, RootWindowOfScreen(g_screen), g_visual,
					AllocNone);
		if (g_depth <= 8)
			ui_warning(NULL, "Display colour depth is %d bit: you may want to use -C for a private colourmap.\n", g_depth);
	}

	if ((!g_ownbackstore) && (DoesBackingStore(g_screen) != Always))
	{
		ui_warning(NULL, "External BackingStore not available. Using internal.\n");
		g_ownbackstore = True;
	}

	/*
	 * Determine desktop size
	 */
	if (g_fullscreen)
	{
		g_settings.width = WidthOfScreen(g_screen);
		g_settings.height = HeightOfScreen(g_screen);
		g_using_full_workarea = True;
	}
	else if (g_settings.width < 0)
	{
		/* Percent of screen */
		if (-g_settings.width >= 100)
			g_using_full_workarea = True;
		g_settings.height = HeightOfScreen(g_screen) * (-g_settings.width) / 100;
		g_settings.width = WidthOfScreen(g_screen) * (-g_settings.width) / 100;
	}
	else if (g_settings.width == 0)
	{
		ui_warning(NULL, "Failed to get workarea: probably your window manager does not support extended hints\n");
		g_settings.width = WidthOfScreen(g_screen);
		g_settings.height = HeightOfScreen(g_screen);
	}

	/* make sure width is a multiple of 4 */
	g_settings.width = (g_settings.width + 3) & ~3;

	g_mod_map = XGetModifierMapping(g_display);
	xwin_refresh_pointer_map();

	xkbkeymap_init();

	xclip_init();

	DEBUG_RDP5(("server bpp %d client bpp %d depth %d\n", g_settings.server_depth, g_bpp, g_depth));

	return True;
}

void
ui_deinit(void * inst)
{
	xclip_deinit();

	if (g_IM != NULL)
		XCloseIM(g_IM);

	if (g_null_cursor != NULL)
		ui_destroy_cursor(inst, g_null_cursor);

	XFreeModifiermap(g_mod_map);

	if (g_ownbackstore)
		XFreePixmap(g_display, g_backstore);

	XFreeGC(g_display, g_gc);
	XCloseDisplay(g_display);
	g_display = NULL;
}


static void
get_window_attribs(XSetWindowAttributes * attribs)
{
	attribs->background_pixel = BlackPixelOfScreen(g_screen);
	attribs->background_pixel = WhitePixelOfScreen(g_screen);
	attribs->border_pixel = WhitePixelOfScreen(g_screen);
	attribs->backing_store = g_ownbackstore ? NotUseful : Always;
	attribs->override_redirect = g_fullscreen;
	attribs->colormap = g_xcolmap;
}

static void
get_input_mask(long *input_mask)
{
	*input_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
		VisibilityChangeMask | FocusChangeMask | StructureNotifyMask;

	if (g_sendmotion)
		*input_mask |= PointerMotionMask;
	if (g_ownbackstore)
		*input_mask |= ExposureMask;
	if (g_fullscreen || g_grab_keyboard)
		*input_mask |= EnterWindowMask;
	if (g_grab_keyboard)
		*input_mask |= LeaveWindowMask;
}

RD_BOOL
ui_create_window(void * inst)
{
	uint8_t null_pointer_mask[1] = { 0x80 };
	uint8_t null_pointer_data[24] = { 0x00 };

	XSetWindowAttributes attribs;
	XClassHint *classhints;
	XSizeHints *sizehints;
	int wndwidth, wndheight;
	long input_mask, ic_input_mask;
	XEvent xevent;

	wndwidth = g_fullscreen ? WidthOfScreen(g_screen) : g_settings.width;
	wndheight = g_fullscreen ? HeightOfScreen(g_screen) : g_settings.height;

	/* Handle -x-y portion of geometry string */
	if (g_xpos < 0 || (g_xpos == 0 && (g_pos & 2)))
		g_xpos = WidthOfScreen(g_screen) + g_xpos - g_settings.width;
	if (g_ypos < 0 || (g_ypos == 0 && (g_pos & 4)))
		g_ypos = HeightOfScreen(g_screen) + g_ypos - g_settings.height;

	get_window_attribs(&attribs);

	g_wnd = XCreateWindow(g_display, RootWindowOfScreen(g_screen), g_xpos, g_ypos, wndwidth,
			      wndheight, 0, g_depth, InputOutput, g_visual,
			      CWBackPixel | CWBackingStore | CWOverrideRedirect | CWColormap |
			      CWBorderPixel, &attribs);

	if (g_gc == NULL)
	{
		g_gc = XCreateGC(g_display, g_wnd, 0, NULL);
		ui_reset_clip(inst);
	}

	if (g_create_bitmap_gc == NULL)
		g_create_bitmap_gc = XCreateGC(g_display, g_wnd, 0, NULL);

	if ((g_ownbackstore) && (g_backstore == 0))
	{
		g_backstore = XCreatePixmap(g_display, g_wnd, g_settings.width, g_settings.height, g_depth);

		/* clear to prevent rubbish being exposed at startup */
		XSetForeground(g_display, g_gc, BlackPixelOfScreen(g_screen));
		XFillRectangle(g_display, g_backstore, g_gc, 0, 0, g_settings.width, g_settings.height);
	}

	XStoreName(g_display, g_wnd, g_title);

	if (g_hide_decorations)
		mwm_hide_decorations(g_wnd);

	classhints = XAllocClassHint();
	if (classhints != NULL)
	{
		classhints->res_name = classhints->res_class = "rdesktop";
		XSetClassHint(g_display, g_wnd, classhints);
		XFree(classhints);
	}

	sizehints = XAllocSizeHints();
	if (sizehints)
	{
		sizehints->flags = PMinSize | PMaxSize;
		if (g_pos)
			sizehints->flags |= PPosition;
		sizehints->min_width = sizehints->max_width = g_settings.width;
		sizehints->min_height = sizehints->max_height = g_settings.height;
		XSetWMNormalHints(g_display, g_wnd, sizehints);
		XFree(sizehints);
	}

	if (g_embed_wnd)
	{
		XReparentWindow(g_display, g_wnd, (Window) g_embed_wnd, 0, 0);
	}

	get_input_mask(&input_mask);

	if (g_IM != NULL)
	{
		g_IC = XCreateIC(g_IM, XNInputStyle, (XIMPreeditNothing | XIMStatusNothing),
				 XNClientWindow, g_wnd, XNFocusWindow, g_wnd, NULL);

		if ((g_IC != NULL)
		    && (XGetICValues(g_IC, XNFilterEvents, &ic_input_mask, NULL) == NULL))
			input_mask |= ic_input_mask;
	}

	XSelectInput(g_display, g_wnd, input_mask);
	XMapWindow(g_display, g_wnd);

	/* wait for VisibilityNotify */
	do
	{
		XMaskEvent(g_display, VisibilityChangeMask, &xevent);
	}
	while (xevent.type != VisibilityNotify);
	g_Unobscured = xevent.xvisibility.state == VisibilityUnobscured;

	g_focused = False;
	g_mouse_in_wnd = False;

	/* handle the WM_DELETE_WINDOW protocol */
	g_protocol_atom = XInternAtom(g_display, "WM_PROTOCOLS", True);
	g_kill_atom = XInternAtom(g_display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(g_display, g_wnd, &g_kill_atom, 1);

	/* create invisible 1x1 cursor to be used as null cursor */
	if (g_null_cursor == NULL)
		g_null_cursor = ui_create_cursor(inst, 0, 0, 1, 1, null_pointer_mask, null_pointer_data, 24);

	return True;
}

void
ui_resize_window(void * inst)
{
	XSizeHints *sizehints;
	Pixmap bs;

	sizehints = XAllocSizeHints();
	if (sizehints)
	{
		sizehints->flags = PMinSize | PMaxSize;
		sizehints->min_width = sizehints->max_width = g_settings.width;
		sizehints->min_height = sizehints->max_height = g_settings.height;
		XSetWMNormalHints(g_display, g_wnd, sizehints);
		XFree(sizehints);
	}

	if (!(g_fullscreen || g_embed_wnd))
	{
		XResizeWindow(g_display, g_wnd, g_settings.width, g_settings.height);
	}

	/* create new backstore pixmap */
	if (g_backstore != 0)
	{
		bs = XCreatePixmap(g_display, g_wnd, g_settings.width, g_settings.height, g_depth);
		XSetForeground(g_display, g_gc, BlackPixelOfScreen(g_screen));
		XFillRectangle(g_display, bs, g_gc, 0, 0, g_settings.width, g_settings.height);
		XCopyArea(g_display, g_backstore, bs, g_gc, 0, 0, g_settings.width, g_settings.height, 0, 0);
		XFreePixmap(g_display, g_backstore);
		g_backstore = bs;
	}
}

void
ui_destroy_window(void)
{
	if (g_IC != NULL)
		XDestroyIC(g_IC);

	XDestroyWindow(g_display, g_wnd);
}

void
xwin_toggle_fullscreen(void * inst)
{
	Pixmap contents = 0;

	if (!g_ownbackstore)
	{
		/* need to save contents of window */
		contents = XCreatePixmap(g_display, g_wnd, g_settings.width, g_settings.height, g_depth);
		XCopyArea(g_display, g_wnd, contents, g_gc, 0, 0, g_settings.width, g_settings.height, 0, 0);
	}

	ui_destroy_window();
	g_fullscreen = !g_fullscreen;
	ui_create_window(inst);

	XDefineCursor(g_display, g_wnd, g_current_cursor);

	if (!g_ownbackstore)
	{
		XCopyArea(g_display, contents, g_wnd, g_gc, 0, 0, g_settings.width, g_settings.height, 0, 0);
		XFreePixmap(g_display, contents);
	}
}

static void
handle_button_event(XEvent xevent, RD_BOOL down)
{
	uint16_t button, flags = 0;
	g_last_gesturetime = xevent.xbutton.time;
	/* Reverse the pointer button mapping, e.g. in the case of
	   "left-handed mouse mode"; the RDP session expects to
	   receive physical buttons (true in mstsc as well) and
	   logical button behavior depends on the remote desktop's own
	   mouse settings */
	xevent.xbutton.button = g_pointer_log_to_phys_map[xevent.xbutton.button - 1];
	button = xkb_translate_button(xevent.xbutton.button);

	if (button == 0)
		return;

	if (down)
		flags = MOUSE_FLAG_DOWN;

	/* Stop moving window when button is released, regardless of cursor position */
	if (g_moving_wnd && (xevent.type == ButtonRelease))
		g_moving_wnd = False;

	/* If win_button_size is nonzero, enable single app mode */
	if (xevent.xbutton.y < g_win_button_size)
	{
		/*  Check from right to left: */
		if (xevent.xbutton.x >= g_settings.width - g_win_button_size)
		{
			/* The close button, continue */
			;
		}
		else if (xevent.xbutton.x >= g_settings.width - g_win_button_size * 2)
		{
			/* The maximize/restore button. Do not send to
			   server.  It might be a good idea to change the
			   cursor or give some other visible indication
			   that rdesktop inhibited this click */
			if (xevent.type == ButtonPress)
				return;
		}
		else if (xevent.xbutton.x >= g_settings.width - g_win_button_size * 3)
		{
			/* The minimize button. Iconify window. */
			if (xevent.type == ButtonRelease)
			{
				/* Release the mouse button outside the minimize button, to prevent the
				   actual minimazation to happen */
				rdp_send_input(g_rdp, time(NULL), RDP_INPUT_MOUSE, button, 1, 1);
				XIconifyWindow(g_display, g_wnd, DefaultScreen(g_display));
				return;
			}
		}
		else if (xevent.xbutton.x <= g_win_button_size)
		{
			/* The system menu. Ignore. */
			if (xevent.type == ButtonPress)
				return;
		}
		else
		{
			/* The title bar. */
			if (xevent.type == ButtonPress)
			{
				if (!g_fullscreen && g_hide_decorations && !g_using_full_workarea)
				{
					g_moving_wnd = True;
					g_move_x_offset = xevent.xbutton.x;
					g_move_y_offset = xevent.xbutton.y;
				}
				return;
			}
		}
	}

	if (xevent.xmotion.window == g_wnd)
	{
		rdp_send_input(g_rdp, time(NULL), RDP_INPUT_MOUSE,
			       flags | button, xevent.xbutton.x, xevent.xbutton.y);
	}
}


/* Process events in Xlib queue
   Returns 0 after user quit, 1 otherwise */
static int
xwin_process_events(void)
{
	XEvent xevent;
	uint32_t ev_time;
	int events = 0;

	while ((XPending(g_display) > 0) && events++ < 20)
	{
		XNextEvent(g_display, &xevent);

		if ((g_IC != NULL) && (XFilterEvent(&xevent, None) == True))
		{
			DEBUG_KBD(("Filtering event\n"));
			continue;
		}

		switch (xevent.type)
		{
			case VisibilityNotify:
				if (xevent.xvisibility.window == g_wnd)
					g_Unobscured =
						xevent.xvisibility.state == VisibilityUnobscured;

				break;

			case ClientMessage:

				/* The window manager told us to quit */
				if ((xevent.xclient.message_type == g_protocol_atom) && ((Atom)xevent.xclient.data.l[0] == g_kill_atom))
				{
					ui_deinit(g_rdp->inst);
					exit(0);
				}

				break;

			case KeyPress:
				g_last_gesturetime = xevent.xkey.time;
				ev_time = time(NULL);

				xkb_handle_special_keys(g_rdp->inst, ev_time, RDP_KEYPRESS, xevent.xkey.keycode);
				xkbkeymap_send_key(ev_time, RDP_KEYPRESS, xevent.xkey.keycode);

				break;

			case KeyRelease:
				g_last_gesturetime = xevent.xkey.time;
				ev_time = time(NULL);
				xkb_handle_special_keys(g_rdp->inst, ev_time, RDP_KEYRELEASE, xevent.xkey.keycode);
				xkbkeymap_send_key(ev_time, RDP_KEYRELEASE, xevent.xkey.keycode);

				break;

			case ButtonPress:
				handle_button_event(xevent, True);
				break;

			case ButtonRelease:
				handle_button_event(xevent, False);
				break;

			case MotionNotify:
				if (g_moving_wnd)
				{
					XMoveWindow(g_display, g_wnd,
						    xevent.xmotion.x_root - g_move_x_offset,
						    xevent.xmotion.y_root - g_move_y_offset);
					break;
				}

				if (g_fullscreen && !g_focused)
					XSetInputFocus(g_display, g_wnd, RevertToPointerRoot,
						       CurrentTime);

				if (xevent.xmotion.window == g_wnd)
				{
					rdp_send_input(g_rdp, time(NULL), RDP_INPUT_MOUSE, MOUSE_FLAG_MOVE,
						       xevent.xmotion.x, xevent.xmotion.y);
				}
				break;

			case FocusIn:
				if (xevent.xfocus.mode == NotifyGrab)
					break;
				g_focused = True;

				if (g_grab_keyboard && g_mouse_in_wnd)
					XGrabKeyboard(g_display, g_wnd, True,
						      GrabModeAsync, GrabModeAsync, CurrentTime);

				ev_time = time(NULL);
				xkb_handle_focus_in(g_rdp->inst, ev_time);
				break;

			case FocusOut:
				if (xevent.xfocus.mode == NotifyUngrab)
					break;
				g_focused = False;
				if (xevent.xfocus.mode == NotifyWhileGrabbed)
					XUngrabKeyboard(g_display, CurrentTime);
				break;

			case EnterNotify:
				/* we only register for this event when in fullscreen mode */
				/* or grab_keyboard */
				g_mouse_in_wnd = True;
				if (g_fullscreen)
				{
					XSetInputFocus(g_display, g_wnd, RevertToPointerRoot,
						       CurrentTime);
					break;
				}
				if (g_focused)
					XGrabKeyboard(g_display, g_wnd, True,
						      GrabModeAsync, GrabModeAsync, CurrentTime);
				break;

			case LeaveNotify:
				/* we only register for this event when grab_keyboard */
				g_mouse_in_wnd = False;
				XUngrabKeyboard(g_display, CurrentTime);
				break;

			case Expose:
				if (xevent.xexpose.window == g_wnd)
				{
					XCopyArea(g_display, g_backstore, xevent.xexpose.window,
						  g_gc,
						  xevent.xexpose.x, xevent.xexpose.y,
						  xevent.xexpose.width, xevent.xexpose.height,
						  xevent.xexpose.x, xevent.xexpose.y);
				}

				break;

			case MappingNotify:
				/* Refresh keyboard mapping if it has changed. This is important for
				   Xvnc, since it allocates keycodes dynamically */
				if (xevent.xmapping.request == MappingKeyboard
				    || xevent.xmapping.request == MappingModifier)
					XRefreshKeyboardMapping(&xevent.xmapping);

				if (xevent.xmapping.request == MappingModifier)
				{
					XFreeModifiermap(g_mod_map);
					g_mod_map = XGetModifierMapping(g_display);
				}

				if (xevent.xmapping.request == MappingPointer)
				{
					xwin_refresh_pointer_map();
				}

				break;

				/* clipboard stuff */
			case SelectionNotify:
				xclip_handle_SelectionNotify(&xevent.xselection);
				break;
			case SelectionRequest:
				xclip_handle_SelectionRequest(&xevent.xselectionrequest);
				break;
			case SelectionClear:
				xclip_handle_SelectionClear();
				break;
			case PropertyNotify:
				xclip_handle_PropertyNotify(&xevent.xproperty);
				if (xevent.xproperty.window == g_wnd)
					break;
				if (xevent.xproperty.window == DefaultRootWindow(g_display))
					break;
				break;
		}
	}
	/* Keep going */
	return 1;
}

/* Returns 0 after user quit, 1 otherwise */
int
ui_select(void * inst, int rdp_socket)
{
	int n;
	fd_set rfds, wfds;
	struct timeval tv;
	RD_BOOL s_timeout = False;

	while (True)
	{
		n = (rdp_socket > g_x_socket) ? rdp_socket : g_x_socket;
		/* Process any events already waiting */
		if (!xwin_process_events())
			/* User quit */
			return 0;

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(rdp_socket, &rfds);
		FD_SET(g_x_socket, &rfds);

		/* default timeout */
		tv.tv_sec = 60;
		tv.tv_usec = 0;

#ifdef WITH_RDPSND
		rdpsnd_add_fds(&n, &rfds, &wfds, &tv);
#endif

		/* add redirection handles */
		rdpdr_add_fds(&n, &rfds, &wfds, &tv, &s_timeout);

		n++;

		switch (select(n, &rfds, &wfds, NULL, &tv))
		{
			case -1:
				ui_error(NULL, "select: %s\n", strerror(errno));

			case 0:
#ifdef WITH_RDPSND
				rdpsnd_check_fds(&rfds, &wfds);
#endif

				/* Abort serial read calls */
				if (s_timeout)
					rdpdr_check_fds(&rfds, &wfds, (RD_BOOL) True);
				continue;
		}

#ifdef WITH_RDPSND
		rdpsnd_check_fds(&rfds, &wfds);
#endif

		rdpdr_check_fds(&rfds, &wfds, (RD_BOOL) False);

		if (FD_ISSET(rdp_socket, &rfds))
			return 1;

	}
}

void
ui_move_pointer(void * inst, int x, int y)
{
	XWarpPointer(g_display, g_wnd, g_wnd, 0, 0, 0, 0, x, y);
}

RD_HBITMAP
ui_create_bitmap(void * inst, int width, int height, uint8_t * data)
{
	XImage *image;
	Pixmap bitmap;
	uint8_t *tdata;
	int bitmap_pad;

	if (g_settings.server_depth == 8)
	{
		bitmap_pad = 8;
	}
	else
	{
		bitmap_pad = g_bpp;

		if (g_bpp == 24)
			bitmap_pad = 32;
	}

	tdata = (g_owncolmap ? data : translate_image(width, height, data));
	bitmap = XCreatePixmap(g_display, g_wnd, width, height, g_depth);
	image = XCreateImage(g_display, g_visual, g_depth, ZPixmap, 0,
			     (char *) tdata, width, height, bitmap_pad, 0);

	XPutImage(g_display, bitmap, g_create_bitmap_gc, image, 0, 0, 0, 0, width, height);

	XFree(image);
	if (tdata != data)
		xfree(tdata);
	return (RD_HBITMAP) bitmap;
}

void
ui_paint_bitmap(void * inst, int x, int y, int cx, int cy, int width, int height, uint8_t * data)
{
	XImage *image;
	uint8_t *tdata;
	int bitmap_pad;

	if (g_settings.server_depth == 8)
	{
		bitmap_pad = 8;
	}
	else
	{
		bitmap_pad = g_bpp;

		if (g_bpp == 24)
			bitmap_pad = 32;
	}

	tdata = (g_owncolmap ? data : translate_image(width, height, data));
	image = XCreateImage(g_display, g_visual, g_depth, ZPixmap, 0,
			     (char *) tdata, width, height, bitmap_pad, 0);

	if (g_ownbackstore)
	{
		XPutImage(g_display, g_backstore, g_gc, image, 0, 0, x, y, cx, cy);
		XCopyArea(g_display, g_backstore, g_wnd, g_gc, x, y, cx, cy, x, y);
	}
	else
	{
		XPutImage(g_display, g_wnd, g_gc, image, 0, 0, x, y, cx, cy);
	}

	XFree(image);
	if (tdata != data)
		xfree(tdata);
}

void
ui_destroy_bitmap(void * inst, RD_HBITMAP bmp)
{
	XFreePixmap(g_display, (Pixmap) bmp);
}

RD_HGLYPH
ui_create_glyph(void * inst, int width, int height, uint8_t * data)
{
	XImage * image;
	Pixmap bitmap;
	int scanline;

	scanline = (width + 7) / 8;

	bitmap = XCreatePixmap(g_display, g_wnd, width, height, 1);
	if (g_create_glyph_gc == 0)
		g_create_glyph_gc = XCreateGC(g_display, bitmap, 0, NULL);

	image = XCreateImage(g_display, g_visual, 1, ZPixmap, 0, (char *) data,
			     width, height, 8, scanline);
	image->byte_order = MSBFirst;
	image->bitmap_bit_order = MSBFirst;
	XInitImage(image);

	XPutImage(g_display, bitmap, g_create_glyph_gc, image, 0, 0, 0, 0, width, height);

	XFree(image);
	return (RD_HGLYPH) bitmap;
}

void
ui_destroy_glyph(void * inst, RD_HGLYPH glyph)
{
	XFreePixmap(g_display, (Pixmap) glyph);
}

RD_HCURSOR
ui_create_cursor(void * inst, unsigned int x, unsigned int y, int width, int height,
		 uint8_t * andmask, uint8_t * xormask, int bpp)
{
	RD_HGLYPH maskglyph, cursorglyph;
	XColor bg, fg;
	Cursor xcursor;
	uint8_t *cursor, *pcursor;
	uint8_t *mask, *pmask;
	uint8_t nextbit;
	int scanline, offset;
	int i, j;

	scanline = (width + 7) / 8;
	offset = scanline * height;

	cursor = (uint8_t *) xmalloc(offset);
	memset(cursor, 0, offset);

	mask = (uint8_t *) xmalloc(offset);
	memset(mask, 0, offset);

	/* approximate AND and XOR masks with a monochrome X pointer */
	for (i = 0; i < height; i++)
	{
		offset -= scanline;
		pcursor = &cursor[offset];
		pmask = &mask[offset];

		for (j = 0; j < scanline; j++)
		{
			for (nextbit = 0x80; nextbit != 0; nextbit >>= 1)
			{
				if (xormask[0] || xormask[1] || xormask[2])
				{
					*pcursor |= (~(*andmask) & nextbit);
					*pmask |= nextbit;
				}
				else
				{
					*pcursor |= ((*andmask) & nextbit);
					*pmask |= (~(*andmask) & nextbit);
				}

				xormask += 3;
			}

			andmask++;
			pcursor++;
			pmask++;
		}
	}

	fg.red = fg.blue = fg.green = 0xffff;
	bg.red = bg.blue = bg.green = 0x0000;
	fg.flags = bg.flags = DoRed | DoBlue | DoGreen;

	cursorglyph = ui_create_glyph(inst, width, height, cursor);
	maskglyph = ui_create_glyph(inst, width, height, mask);

	xcursor =
		XCreatePixmapCursor(g_display, (Pixmap) cursorglyph,
				    (Pixmap) maskglyph, &fg, &bg, x, y);

	ui_destroy_glyph(inst, maskglyph);
	ui_destroy_glyph(inst, cursorglyph);
	xfree(mask);
	xfree(cursor);
	return (RD_HCURSOR) xcursor;
}

void
ui_set_cursor(void * inst, RD_HCURSOR cursor)
{
	g_current_cursor = (Cursor) cursor;
	XDefineCursor(g_display, g_wnd, g_current_cursor);
}

void
ui_destroy_cursor(void * inst, RD_HCURSOR cursor)
{
	XFreeCursor(g_display, (Cursor) cursor);
}

void
ui_set_null_cursor(void * inst)
{
	ui_set_cursor(inst, g_null_cursor);
}

void
ui_set_default_cursor(void * inst)
{
        /* Set default cursor */
}

#define MAKE_XCOLOR(xc,c) \
		(xc)->red   = ((c)->red   << 8) | (c)->red; \
		(xc)->green = ((c)->green << 8) | (c)->green; \
		(xc)->blue  = ((c)->blue  << 8) | (c)->blue; \
		(xc)->flags = DoRed | DoGreen | DoBlue;


RD_HCOLOURMAP
ui_create_colourmap(void * inst, RD_COLOURMAP * colours)
{
	RD_COLOURENTRY *entry;
	int i, ncolours = colours->ncolours;
	if (!g_owncolmap)
	{
		uint32_t *map = (uint32_t *) xmalloc(sizeof(*g_colmap) * ncolours);
		XColor xentry;
		XColor xc_cache[256];
		uint32_t colour;
		int colLookup = 256;
		for (i = 0; i < ncolours; i++)
		{
			entry = &colours->colours[i];
			MAKE_XCOLOR(&xentry, entry);

			if (XAllocColor(g_display, g_xcolmap, &xentry) == 0)
			{
				/* Allocation failed, find closest match. */
				int j = 256;
				int nMinDist = 3 * 256 * 256;
				long nDist = nMinDist;

				/* only get the colors once */
				while (colLookup--)
				{
					xc_cache[colLookup].pixel = colLookup;
					xc_cache[colLookup].red = xc_cache[colLookup].green =
						xc_cache[colLookup].blue = 0;
					xc_cache[colLookup].flags = 0;
					XQueryColor(g_display,
						    DefaultColormap(g_display,
								    DefaultScreen(g_display)),
						    &xc_cache[colLookup]);
				}
				colLookup = 0;

				/* approximate the pixel */
				while (j--)
				{
					if (xc_cache[j].flags)
					{
						nDist = ((long) (xc_cache[j].red >> 8) -
							 (long) (xentry.red >> 8)) *
							((long) (xc_cache[j].red >> 8) -
							 (long) (xentry.red >> 8)) +
							((long) (xc_cache[j].green >> 8) -
							 (long) (xentry.green >> 8)) *
							((long) (xc_cache[j].green >> 8) -
							 (long) (xentry.green >> 8)) +
							((long) (xc_cache[j].blue >> 8) -
							 (long) (xentry.blue >> 8)) *
							((long) (xc_cache[j].blue >> 8) -
							 (long) (xentry.blue >> 8));
					}
					if (nDist < nMinDist)
					{
						nMinDist = nDist;
						xentry.pixel = j;
					}
				}
			}
			colour = xentry.pixel;

			/* update our cache */
			if (xentry.pixel < 256)
			{
				xc_cache[xentry.pixel].red = xentry.red;
				xc_cache[xentry.pixel].green = xentry.green;
				xc_cache[xentry.pixel].blue = xentry.blue;

			}

			map[i] = colour;
		}
		return map;
	}
	else
	{
		XColor *xcolours, *xentry;
		Colormap map;

		xcolours = (XColor *) xmalloc(sizeof(XColor) * ncolours);
		for (i = 0; i < ncolours; i++)
		{
			entry = &colours->colours[i];
			xentry = &xcolours[i];
			xentry->pixel = i;
			MAKE_XCOLOR(xentry, entry);
		}

		map = XCreateColormap(g_display, g_wnd, g_visual, AllocAll);
		XStoreColors(g_display, map, xcolours, ncolours);

		xfree(xcolours);
		return (RD_HCOLOURMAP) map;
	}
}

void
ui_destroy_colourmap(void * inst, RD_HCOLOURMAP map)
{
	if (!g_owncolmap)
		xfree(map);
	else
		XFreeColormap(g_display, (Colormap) map);
}

void
ui_set_colourmap(void * inst, RD_HCOLOURMAP map)
{
	if (!g_owncolmap)
	{
		if (g_colmap)
			xfree(g_colmap);

		g_colmap = (uint32_t *) map;
	}
	else
	{
		XSetWindowColormap(g_display, g_wnd, (Colormap) map);
	}
}

void
ui_set_clip(void * inst, int x, int y, int cx, int cy)
{
	g_clip_rectangle.x = x;
	g_clip_rectangle.y = y;
	g_clip_rectangle.width = cx;
	g_clip_rectangle.height = cy;
	XSetClipRectangles(g_display, g_gc, 0, 0, &g_clip_rectangle, 1, YXBanded);
}

void
ui_reset_clip(void * inst)
{
	g_clip_rectangle.x = 0;
	g_clip_rectangle.y = 0;
	g_clip_rectangle.width = g_settings.width;
	g_clip_rectangle.height = g_settings.height;
	XSetClipRectangles(g_display, g_gc, 0, 0, &g_clip_rectangle, 1, YXBanded);
}

void
ui_bell(void * inst)
{
	XBell(g_display, 0);
}

void
ui_destblt(void * inst, uint8_t opcode,
	   /* dest */ int x, int y, int cx, int cy)
{
	opcode = ROP2_S(opcode);
	SET_FUNCTION(opcode);
	FILL_RECTANGLE(x, y, cx, cy);
	RESET_FUNCTION(opcode);
}

static uint8_t hatch_patterns[] = {
	0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,	/* 0 - bsHorizontal */
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,	/* 1 - bsVertical */
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,	/* 2 - bsFDiagonal */
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,	/* 3 - bsBDiagonal */
	0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08,	/* 4 - bsCross */
	0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81	/* 5 - bsDiagCross */
};

void
ui_patblt(void * inst, uint8_t opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* brush */ RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	Pixmap fill;
	uint8_t i, ipattern[8];

	opcode = ROP2_P(opcode);
	SET_FUNCTION(opcode);

	switch (brush->style)
	{
		case 0:	/* Solid */
			SET_FOREGROUND(fgcolour);
			FILL_RECTANGLE_BACKSTORE(x, y, cx, cy);
			break;

		case 2:	/* Hatch */
			fill = (Pixmap) ui_create_glyph(inst, 8, 8,
							hatch_patterns + brush->pattern[0] * 8);
			SET_FOREGROUND(fgcolour);
			SET_BACKGROUND(bgcolour);
			XSetFillStyle(g_display, g_gc, FillOpaqueStippled);
			XSetStipple(g_display, g_gc, fill);
			XSetTSOrigin(g_display, g_gc, brush->xorigin, brush->yorigin);
			FILL_RECTANGLE_BACKSTORE(x, y, cx, cy);
			XSetFillStyle(g_display, g_gc, FillSolid);
			XSetTSOrigin(g_display, g_gc, 0, 0);
			ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			break;

		case 3:	/* Pattern */
			if (brush->bd == 0)	/* rdp4 brush */
			{
				for (i = 0; i != 8; i++)
					ipattern[7 - i] = brush->pattern[i];
				fill = (Pixmap) ui_create_glyph(inst, 8, 8, ipattern);
				SET_FOREGROUND(bgcolour);
				SET_BACKGROUND(fgcolour);
				XSetFillStyle(g_display, g_gc, FillOpaqueStippled);
				XSetStipple(g_display, g_gc, fill);
				XSetTSOrigin(g_display, g_gc, brush->xorigin, brush->yorigin);
				FILL_RECTANGLE_BACKSTORE(x, y, cx, cy);
				XSetFillStyle(g_display, g_gc, FillSolid);
				XSetTSOrigin(g_display, g_gc, 0, 0);
				ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			}
			else if (brush->bd->colour_code > 1)	/* > 1 bpp */
			{
				fill = (Pixmap) ui_create_bitmap(inst, 8, 8, brush->bd->data);
				XSetFillStyle(g_display, g_gc, FillTiled);
				XSetTile(g_display, g_gc, fill);
				XSetTSOrigin(g_display, g_gc, brush->xorigin, brush->yorigin);
				FILL_RECTANGLE_BACKSTORE(x, y, cx, cy);
				XSetFillStyle(g_display, g_gc, FillSolid);
				XSetTSOrigin(g_display, g_gc, 0, 0);
				ui_destroy_bitmap(inst, (RD_HBITMAP) fill);
			}
			else
			{
				fill = (Pixmap) ui_create_glyph(inst, 8, 8, brush->bd->data);
				SET_FOREGROUND(bgcolour);
				SET_BACKGROUND(fgcolour);
				XSetFillStyle(g_display, g_gc, FillOpaqueStippled);
				XSetStipple(g_display, g_gc, fill);
				XSetTSOrigin(g_display, g_gc, brush->xorigin, brush->yorigin);
				FILL_RECTANGLE_BACKSTORE(x, y, cx, cy);
				XSetFillStyle(g_display, g_gc, FillSolid);
				XSetTSOrigin(g_display, g_gc, 0, 0);
				ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			}
			break;

		default:
			ui_unimpl(inst, "brush %d\n", brush->style);
	}

	RESET_FUNCTION(opcode);

	if (g_ownbackstore)
		XCopyArea(g_display, g_backstore, g_wnd, g_gc, x, y, cx, cy, x, y);
}

void
ui_screenblt(void * inst, uint8_t opcode,
	     /* dest */ int x, int y, int cx, int cy,
	     /* src */ int srcx, int srcy)
{
	opcode = ROP2_S(opcode);
	SET_FUNCTION(opcode);
	if (g_ownbackstore)
	{
		XCopyArea(g_display, g_Unobscured ? g_wnd : g_backstore,
			  g_wnd, g_gc, srcx, srcy, cx, cy, x, y);
		XCopyArea(g_display, g_backstore, g_backstore, g_gc, srcx, srcy, cx, cy, x, y);
	}
	else
	{
		XCopyArea(g_display, g_wnd, g_wnd, g_gc, srcx, srcy, cx, cy, x, y);
	}

	RESET_FUNCTION(opcode);
}

void
ui_memblt(void * inst, uint8_t opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* src */ RD_HBITMAP src, int srcx, int srcy)
{
	opcode = ROP2_S(opcode);
	SET_FUNCTION(opcode);
	XCopyArea(g_display, (Pixmap) src, g_wnd, g_gc, srcx, srcy, cx, cy, x, y);

	if (g_ownbackstore)
		XCopyArea(g_display, (Pixmap) src, g_backstore, g_gc, srcx, srcy, cx, cy, x, y);
	RESET_FUNCTION(opcode);
}

void
ui_triblt(void * inst, uint8_t opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* src */ RD_HBITMAP src, int srcx, int srcy,
	  /* brush */ RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	/* This is potentially difficult to do in general. Until someone
	   comes up with a more efficient way of doing it I am using cases. */

	switch (opcode)
	{
		case 0x69:	/* PDSxxn */
			ui_memblt(inst, ROP2_XOR, x, y, cx, cy, src, srcx, srcy);
			ui_patblt(inst, ROP2_NXOR, x, y, cx, cy, brush, bgcolour, fgcolour);
			break;

		case 0xb8:	/* PSDPxax */
			ui_patblt(inst, ROP2_XOR, x, y, cx, cy, brush, bgcolour, fgcolour);
			ui_memblt(inst, ROP2_AND, x, y, cx, cy, src, srcx, srcy);
			ui_patblt(inst, ROP2_XOR, x, y, cx, cy, brush, bgcolour, fgcolour);
			break;

		case 0xc0:	/* PSa */
			ui_memblt(inst, ROP2_COPY, x, y, cx, cy, src, srcx, srcy);
			ui_patblt(inst, ROP2_AND, x, y, cx, cy, brush, bgcolour, fgcolour);
			break;

		default:
			ui_unimpl(inst, "triblt 0x%x\n", opcode);
			ui_memblt(inst, ROP2_COPY, x, y, cx, cy, src, srcx, srcy);
	}
}

void
ui_line(void * inst, uint8_t opcode,
	/* dest */ int startx, int starty, int endx, int endy,
	/* pen */ RD_PEN * pen)
{
	opcode--;
	SET_FUNCTION(opcode);
	SET_FOREGROUND(pen->colour);
	XDrawLine(g_display, g_wnd, g_gc, startx, starty, endx, endy);

	if (g_ownbackstore)
		XDrawLine(g_display, g_backstore, g_gc, startx, starty, endx, endy);
	RESET_FUNCTION(opcode);
}

void
ui_rect(void * inst,
	       /* dest */ int x, int y, int cx, int cy,
	       /* brush */ int colour)
{
	SET_FOREGROUND(colour);
	FILL_RECTANGLE(x, y, cx, cy);
}

void
ui_polygon(void * inst, uint8_t opcode,
	   /* mode */ uint8_t fillmode,
	   /* dest */ RD_POINT * point, int npoints,
	   /* brush */ RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	uint8_t style, i, ipattern[8];
	Pixmap fill;

	opcode--;
	SET_FUNCTION(opcode);

	switch (fillmode)
	{
		case ALTERNATE:
			XSetFillRule(g_display, g_gc, EvenOddRule);
			break;
		case WINDING:
			XSetFillRule(g_display, g_gc, WindingRule);
			break;
		default:
			ui_unimpl(inst, "fill mode %d\n", fillmode);
	}

	if (brush)
		style = brush->style;
	else
		style = 0;

	switch (style)
	{
		case 0:	/* Solid */
			SET_FOREGROUND(fgcolour);
			FILL_POLYGON((XPoint *) point, npoints);
			break;

		case 2:	/* Hatch */
			fill = (Pixmap) ui_create_glyph(inst, 8, 8,
							hatch_patterns + brush->pattern[0] * 8);
			SET_FOREGROUND(fgcolour);
			SET_BACKGROUND(bgcolour);
			XSetFillStyle(g_display, g_gc, FillOpaqueStippled);
			XSetStipple(g_display, g_gc, fill);
			XSetTSOrigin(g_display, g_gc, brush->xorigin, brush->yorigin);
			FILL_POLYGON((XPoint *) point, npoints);
			XSetFillStyle(g_display, g_gc, FillSolid);
			XSetTSOrigin(g_display, g_gc, 0, 0);
			ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			break;

		case 3:	/* Pattern */
			if (brush->bd == 0)	/* rdp4 brush */
			{
				for (i = 0; i != 8; i++)
					ipattern[7 - i] = brush->pattern[i];
				fill = (Pixmap) ui_create_glyph(inst, 8, 8, ipattern);
				SET_FOREGROUND(bgcolour);
				SET_BACKGROUND(fgcolour);
				XSetFillStyle(g_display, g_gc, FillOpaqueStippled);
				XSetStipple(g_display, g_gc, fill);
				XSetTSOrigin(g_display, g_gc, brush->xorigin, brush->yorigin);
				FILL_POLYGON((XPoint *) point, npoints);
				XSetFillStyle(g_display, g_gc, FillSolid);
				XSetTSOrigin(g_display, g_gc, 0, 0);
				ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			}
			else if (brush->bd->colour_code > 1)	/* > 1 bpp */
			{
				fill = (Pixmap) ui_create_bitmap(inst, 8, 8, brush->bd->data);
				XSetFillStyle(g_display, g_gc, FillTiled);
				XSetTile(g_display, g_gc, fill);
				XSetTSOrigin(g_display, g_gc, brush->xorigin, brush->yorigin);
				FILL_POLYGON((XPoint *) point, npoints);
				XSetFillStyle(g_display, g_gc, FillSolid);
				XSetTSOrigin(g_display, g_gc, 0, 0);
				ui_destroy_bitmap(inst, (RD_HBITMAP) fill);
			}
			else
			{
				fill = (Pixmap) ui_create_glyph(inst, 8, 8, brush->bd->data);
				SET_FOREGROUND(bgcolour);
				SET_BACKGROUND(fgcolour);
				XSetFillStyle(g_display, g_gc, FillOpaqueStippled);
				XSetStipple(g_display, g_gc, fill);
				XSetTSOrigin(g_display, g_gc, brush->xorigin, brush->yorigin);
				FILL_POLYGON((XPoint *) point, npoints);
				XSetFillStyle(g_display, g_gc, FillSolid);
				XSetTSOrigin(g_display, g_gc, 0, 0);
				ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			}
			break;

		default:
			ui_unimpl(inst, "brush %d\n", brush->style);
	}

	RESET_FUNCTION(opcode);
}

void
ui_polyline(void * inst, uint8_t opcode,
	    /* dest */ RD_POINT * points, int npoints,
	    /* pen */ RD_PEN * pen)
{
	/* TODO: set join style */
	opcode--;
	SET_FUNCTION(opcode);
	SET_FOREGROUND(pen->colour);
	XDrawLines(g_display, g_wnd, g_gc, (XPoint *) points, npoints, CoordModePrevious);
	if (g_ownbackstore)
		XDrawLines(g_display, g_backstore, g_gc, (XPoint *) points, npoints,
			   CoordModePrevious);

	RESET_FUNCTION(opcode);
}

void
ui_ellipse(void * inst, uint8_t opcode,
	   /* mode */ uint8_t fillmode,
	   /* dest */ int x, int y, int cx, int cy,
	   /* brush */ RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	uint8_t style, i, ipattern[8];
	Pixmap fill;

	opcode--;
	SET_FUNCTION(opcode);

	if (brush)
		style = brush->style;
	else
		style = 0;

	switch (style)
	{
		case 0:	/* Solid */
			SET_FOREGROUND(fgcolour);
			DRAW_ELLIPSE(x, y, cx, cy, fillmode);
			break;

		case 2:	/* Hatch */
			fill = (Pixmap) ui_create_glyph(inst, 8, 8,
							hatch_patterns + brush->pattern[0] * 8);
			SET_FOREGROUND(fgcolour);
			SET_BACKGROUND(bgcolour);
			XSetFillStyle(g_display, g_gc, FillOpaqueStippled);
			XSetStipple(g_display, g_gc, fill);
			XSetTSOrigin(g_display, g_gc, brush->xorigin, brush->yorigin);
			DRAW_ELLIPSE(x, y, cx, cy, fillmode);
			XSetFillStyle(g_display, g_gc, FillSolid);
			XSetTSOrigin(g_display, g_gc, 0, 0);
			ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			break;

		case 3:	/* Pattern */
			if (brush->bd == 0)	/* rdp4 brush */
			{
				for (i = 0; i != 8; i++)
					ipattern[7 - i] = brush->pattern[i];
				fill = (Pixmap) ui_create_glyph(inst, 8, 8, ipattern);
				SET_FOREGROUND(bgcolour);
				SET_BACKGROUND(fgcolour);
				XSetFillStyle(g_display, g_gc, FillOpaqueStippled);
				XSetStipple(g_display, g_gc, fill);
				XSetTSOrigin(g_display, g_gc, brush->xorigin, brush->yorigin);
				DRAW_ELLIPSE(x, y, cx, cy, fillmode);
				XSetFillStyle(g_display, g_gc, FillSolid);
				XSetTSOrigin(g_display, g_gc, 0, 0);
				ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			}
			else if (brush->bd->colour_code > 1)	/* > 1 bpp */
			{
				fill = (Pixmap) ui_create_bitmap(inst, 8, 8, brush->bd->data);
				XSetFillStyle(g_display, g_gc, FillTiled);
				XSetTile(g_display, g_gc, fill);
				XSetTSOrigin(g_display, g_gc, brush->xorigin, brush->yorigin);
				DRAW_ELLIPSE(x, y, cx, cy, fillmode);
				XSetFillStyle(g_display, g_gc, FillSolid);
				XSetTSOrigin(g_display, g_gc, 0, 0);
				ui_destroy_bitmap(inst, (RD_HBITMAP) fill);
			}
			else
			{
				fill = (Pixmap) ui_create_glyph(inst, 8, 8, brush->bd->data);
				SET_FOREGROUND(bgcolour);
				SET_BACKGROUND(fgcolour);
				XSetFillStyle(g_display, g_gc, FillOpaqueStippled);
				XSetStipple(g_display, g_gc, fill);
				XSetTSOrigin(g_display, g_gc, brush->xorigin, brush->yorigin);
				DRAW_ELLIPSE(x, y, cx, cy, fillmode);
				XSetFillStyle(g_display, g_gc, FillSolid);
				XSetTSOrigin(g_display, g_gc, 0, 0);
				ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			}
			break;

		default:
			ui_unimpl(inst, "brush %d\n", brush->style);
	}

	RESET_FUNCTION(opcode);
}

void
ui_start_draw_glyphs(void * inst, int bgcolour, int fgcolour)
{
	SET_FOREGROUND(fgcolour);
	SET_BACKGROUND(bgcolour);
	XSetFillStyle(g_display, g_gc, FillStippled);
}

/* warning, this function only draws on wnd or backstore, not both */
void
ui_draw_glyph(void * inst, int x, int y, int cx, int cy, RD_HGLYPH glyph)
{
	XSetStipple(g_display, g_gc, (Pixmap) glyph);
	XSetTSOrigin(g_display, g_gc, x, y);
	FILL_RECTANGLE_BACKSTORE(x, y, cx, cy);
}

void
ui_end_draw_glyphs(void * inst, int x, int y, int cx, int cy)
{
	XSetFillStyle(g_display, g_gc, FillSolid);
	if (g_ownbackstore)
	{
		XCopyArea(g_display, g_backstore, g_wnd, g_gc, x, y, cx, cy, x, y);
	}
}

void
ui_desktop_save(void * inst, uint32_t offset, int x, int y, int cx, int cy)
{
	Pixmap pix;
	XImage *image;

	if (g_ownbackstore)
	{
		image = XGetImage(g_display, g_backstore, x, y, cx, cy, AllPlanes, ZPixmap);
		exit_if_null(image);
	}
	else
	{
		pix = XCreatePixmap(g_display, g_wnd, cx, cy, g_depth);
		XCopyArea(g_display, g_wnd, pix, g_gc, x, y, cx, cy, 0, 0);
		image = XGetImage(g_display, pix, 0, 0, cx, cy, AllPlanes, ZPixmap);
		exit_if_null(image);
		XFreePixmap(g_display, pix);
	}

	offset *= g_bpp / 8;
	xcache_put_desktop(offset, cx, cy, image->bytes_per_line, g_bpp / 8,
			   (uint8_t *) image->data);

	XDestroyImage(image);
}

void
ui_desktop_restore(void * inst, uint32_t offset, int x, int y, int cx, int cy)
{
	XImage *image;
	uint8_t *data;

	offset *= g_bpp / 8;
	data = xcache_get_desktop(offset, cx, cy, g_bpp / 8);
	if (data == NULL)
		return;

	image = XCreateImage(g_display, g_visual, g_depth, ZPixmap, 0,
			     (char *) data, cx, cy, g_bpp, 0);

	if (g_ownbackstore)
	{
		XPutImage(g_display, g_backstore, g_gc, image, 0, 0, x, y, cx, cy);
		XCopyArea(g_display, g_backstore, g_wnd, g_gc, x, y, cx, cy, x, y);
	}
	else
	{
		XPutImage(g_display, g_wnd, g_gc, image, 0, 0, x, y, cx, cy);
	}

	XFree(image);
}

/* these do nothing here but are used in uiports */
void
ui_begin_update(void * inst)
{
}

void
ui_end_update(void * inst)
{
}

uint32
ui_get_toggle_keys_state(void * inst)
{
	uint32 toggle_keys_state = 0;
	uint32 state;

	state = read_keyboard_state();

	if(get_key_state(state, XK_Scroll_Lock))
		toggle_keys_state |= KBD_SYNC_SCROLL_LOCK;

	if(get_key_state(state, XK_Num_Lock))
		toggle_keys_state |= KBD_SYNC_NUM_LOCK;

	if(get_key_state(state, XK_Caps_Lock))
		toggle_keys_state |= KBD_SYNC_CAPS_LOCK;

	if(get_key_state(state, XK_Kana_Lock))
		toggle_keys_state |= KBD_SYNC_KANA_LOCK;

	return toggle_keys_state;
}

unsigned int
read_keyboard_state(void)
{
#ifdef RDP2VNC
	return 0;
#else
	unsigned int state;
	Window wdummy;
	int dummy;

	XQueryPointer(g_display, g_wnd, &wdummy, &wdummy, &dummy, &dummy, &dummy, &dummy, &state);
	return state;
#endif
}

/* report an error */
void
ui_error(void * inst, char *format, ...)
{
	va_list ap;

	fprintf(stderr, "ERROR: ");

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

/* report a warning */
void
ui_warning(void * inst, char *format, ...)
{
	va_list ap;

	fprintf(stderr, "WARNING: ");

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

/* report an unimplemented protocol feature */
void
ui_unimpl(void * inst, char *format, ...)
{
	va_list ap;

	fprintf(stderr, "NOT IMPLEMENTED: ");

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

RD_HBITMAP
ui_create_surface(void * inst, int width, int height, RD_HBITMAP old)
{
	return 0;
}

void
ui_set_surface(void * inst, RD_HBITMAP surface)
{
}

void
ui_destroy_surface(void * inst, RD_HBITMAP surface)
{
}
