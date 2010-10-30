/*
   Copyright (c) 2009-2010 Jay Sorg

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

#ifndef __XF_TYPES_H
#define __XF_TYPES_H

#include <freerdp/freerdp.h>
#include <freerdp/chanman.h>
#include <X11/Xlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define SET_XFI(_inst, _xfi) (_inst)->param1 = _xfi
#define GET_XFI(_inst) ((xfInfo *) ((_inst)->param1))

struct xf_km
{
	int scancode;
	int flags;
};

struct xf_info
{
	/* RDP stuff */
	rdpSet * settings;
	rdpChanMan * chan_man;
	rdpInst * inst;

	/* UI settings */
	int fullscreen;
	int fs_toggle;
	int keyboard_layout_id;

	/* X11 stuff */
	Window wnd;
	GC gc;
	Display * display;
	Screen * screen;
	Colormap xcolmap;
	int x_socket;
	int depth;
	int bpp;
	int screen_num;
	Pixmap backstore;
	int unobscured;
	Visual * visual;
	int xserver_be;
	int bitmap_pad;
	int red_mask;
	int green_mask;
	int blue_mask;
	int * colormap;
	Drawable drw;
	Pixmap bitmap_mono;
	GC gc_mono;
	GC gc_default;
	Cursor null_cursor;
	struct xf_km km[256];
	int pause_key;
	int tab_key;
	XModifierKeymap * mod_map;
	RD_BOOL focused;
	RD_BOOL mouse_into;
};
typedef struct xf_info xfInfo;

#ifdef WITH_DEBUG
#define DEBUG(fmt, ...)	fprintf(stderr, "DBG %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG(fmt, ...) do { } while (0)
#endif

#endif
