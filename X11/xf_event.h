
#ifndef __XF_EVENT_H
#define __XF_EVENT_H

#include "freerdp.h"

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
};
typedef struct xf_info xfInfo;

int
xf_handle_event(rdpInst * inst, xfInfo * xfi, XEvent * xevent);

#endif
