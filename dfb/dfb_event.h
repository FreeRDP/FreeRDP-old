
#ifndef __DFB_EVENT_H
#define __DFB_EVENT_H

#include <directfb.h>
#include <freerdp/freerdp.h>

#define SET_DFBI(_inst, _dfbi) (_inst)->param1 = _dfbi
#define GET_DFBI(_inst) ((dfbInfo *) ((_inst)->param1))

struct dfb_info
{
	int width;
	int height;
	IDirectFB * dfb;
	DFBRegion region;
	IDirectFBSurface * drw;
	IDirectFBSurface * primary;
	DFBSurfaceDescription dsc;
	IDirectFBEventBuffer * event;
	int update_pending;
	DFBRegion update_region;
	uint8 * colourmap;
};
typedef struct dfb_info dfbInfo;

int
dfb_handle_event(rdpInst * inst);

#endif /* __DFB_EVENT_H */
