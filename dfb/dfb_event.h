
#ifndef __DFB_EVENT_H
#define __DFB_EVENT_H

#include <directfb.h>
#include <freerdp/freerdp.h>
#include "dfb_gdi.h"

#define SET_DFBI(_inst, _dfbi) (_inst)->param1 = _dfbi
#define GET_DFBI(_inst) ((dfbInfo *) ((_inst)->param1))

struct _PIXEL
{
	int red;
	int green;
	int blue;
	int alpha;
};
typedef struct _PIXEL PIXEL;

struct dfb_info
{
	int bpp;
	int width;
	int height;
	int cursor_x;
	int cursor_y;
	char* screen;
	DFBResult err;
	IDirectFB * dfb;
	DFBRegion region;
	DFBEvent events[16];
	IDirectFBSurface * drw;
	DFBSurfaceDescription dsc;
	IDirectFBSurface * primary;
	IDirectFBEventBuffer * event;
	IDirectFBSurface * screen_surface;
	IDirectFBDisplayLayer * layer;
	DFBRectangle update_rect;
	int bytes_per_pixel;
	int * colourmap;
	PIXEL bgcolour;
	PIXEL fgcolour;
	PIXEL pixel;
	HDC hdc;
};
typedef struct dfb_info dfbInfo;

int
dfb_process_event(rdpInst * inst, DFBEvent * event);

#endif /* __DFB_EVENT_H */
