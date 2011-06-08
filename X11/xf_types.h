/*
   FreeRDP: A Remote Desktop Protocol client.
   UI types

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
#include <freerdp/utils/debug.h>
#include <X11/Xlib.h>

#include "gdi.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define SET_XFI(_inst, _xfi) (_inst)->param1 = _xfi
#define GET_XFI(_inst) ((xfInfo *) ((_inst)->param1))

enum
{
	XF_CODEC_NONE,
	XF_CODEC_REMOTEFX
};

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
	int fs_toggle;
	int fullscreen;
	int percentscreen;
	int keyboard_layout_id;
	int decoration;
	int grab_keyboard;
	char window_title[64];

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
	HCLRCONV clrconv;
	Drawable drw;
	Pixmap bitmap_mono;
	GC gc_mono;
	GC gc_default;
	Cursor null_cursor;
	struct xf_km km[256];
	XModifierKeymap * mod_map;
	RD_BOOL focused;
	RD_BOOL mouse_into;

	/* XVideo stuff */
	long xv_port;
	Atom xv_colorkey_atom;
	int xv_image_size;
	int xv_shmid;
	char * xv_shmaddr;
	uint32 * xv_pixfmts;

	/* RemoteFX */
	int codec;
	void * rfx_context;
};
typedef struct xf_info xfInfo;

/* xf exit codes */
enum XF_EXIT_CODE
{
	/* section 0-15: protocol-independent codes */
	XF_EXIT_SUCCESS = 0,
	XF_EXIT_DISCONNECT = 1,
	XF_EXIT_LOGOFF = 2,
	XF_EXIT_IDLE_TIMEOUT = 3,
	XF_EXIT_LOGON_TIMEOUT = 4,
	XF_EXIT_CONN_REPLACED = 5,
	XF_EXIT_OUT_OF_MEMORY = 6,
	XF_EXIT_CONN_DENIED = 7,
	XF_EXIT_CONN_DENIED_FIPS = 8,
	XF_EXIT_USER_PRIVILEGES = 9,
	XF_EXIT_FRESH_CREDENTIALS_REQUIRED = 10,
	XF_EXIT_DISCONNECT_BY_USER = 11,

	/* section 16-31: license error set */
	XF_EXIT_LICENSE_INTERNAL = 16,
	XF_EXIT_LICENSE_NO_LICENSE_SERVER = 17,
	XF_EXIT_LICENSE_NO_LICENSE = 18,
	XF_EXIT_LICENSE_BAD_CLIENT_MSG = 19,
	XF_EXIT_LICENSE_HWID_DOESNT_MATCH = 20,
	XF_EXIT_LICENSE_BAD_CLIENT = 21,
	XF_EXIT_LICENSE_CANT_FINISH_PROTOCOL = 22,
	XF_EXIT_LICENSE_CLIENT_ENDED_PROTOCOL = 23,
	XF_EXIT_LICENSE_BAD_CLIENT_ENCRYPTION = 24,
	XF_EXIT_LICENSE_CANT_UPGRADE = 25,
	XF_EXIT_LICENSE_NO_REMOTE_CONNECTIONS = 26,

	/* section 32-127: RDP protocol error set */
	XF_EXIT_RDP = 32,

	/* section 128-254: xfreerdp specific exit codes */
	XF_EXIT_WRONG_PARAM = 128,
	XF_EXIT_MEMORY = 129,
	XF_EXIT_PROTOCOL = 130,
	XF_EXIT_CONN_FAILED = 131,

	XF_EXIT_UNKNOWN = 255,
};

#ifdef WITH_DEBUG_X11
#define DEBUG_X11(fmt, ...) DEBUG_CLASS(X11, fmt, ## __VA_ARGS__)
#else
#define DEBUG_X11(fmt, ...) DEBUG_NULL(fmt, ## __VA_ARGS__)
#endif

#ifdef WITH_DEBUG_X11_KBD
#define DEBUG_X11_KBD(fmt, ...) DEBUG_CLASS(X11_KBD, fmt, ## __VA_ARGS__)
#else
#define DEBUG_X11_KBD(fmt, ...) DEBUG_NULL(fmt, ## __VA_ARGS__)
#endif

#endif
