
#ifndef __DFB_EVENT_H
#define __DFB_EVENT_H

#include <directfb.h>
#include <freerdp/freerdp.h>

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
	char* screen;
	DFBResult err;
	IDirectFB * dfb;
	DFBRegion region;
	IDirectFBSurface * drw;
	DFBSurfaceDescription dsc;
	IDirectFBSurface * primary;
	IDirectFBEventBuffer * event;
	IDirectFBSurface * screen_surface;
	DFBRectangle update_rect;
	int update_pending;
	int bytes_per_pixel;
	int * colourmap;
	PIXEL pixel;
};
typedef struct dfb_info dfbInfo;

int
dfb_handle_event(rdpInst * inst);

#endif /* __DFB_EVENT_H */
