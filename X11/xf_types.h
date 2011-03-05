/*
   FreeRDP: A Remote Desktop Protocol client.
   Channels

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
	int percentscreen;
	int fs_toggle;
	int keyboard_layout_id;
	int decoration;

	/* X11 stuff */
	Window embed;
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
#define DEBUG(fmt, ...)	printf("DBG %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG(fmt, ...) do { } while (0)
#endif

#endif
