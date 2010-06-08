
#ifndef __XF_EVENT_H
#define __XF_EVENT_H

#include <freerdp/freerdp.h>

#define SET_XFI(_inst, _xfi) (_inst)->param1 = _xfi
#define GET_XFI(_inst) ((xfInfo *) ((_inst)->param1))

struct xf_km
{
	int scancode;
	int flags;
};

struct xf_info
{
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
	int * colourmap;
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

int
xf_handle_event(rdpInst * inst, xfInfo * xfi, XEvent * xevent);

#endif
