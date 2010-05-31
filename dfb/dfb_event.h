
#ifndef __DFB_EVENT_H
#define __DFB_EVENT_H

#include <directfb.h>
#include <freerdp/freerdp.h>

#define SET_DFBI(_inst, _dfbi) (_inst)->param1 = _dfbi
#define GET_DFBI(_inst) ((dfbInfo *) ((_inst)->param1))

struct dfb_info
{
	IDirectFB * dfb;
	int width;
	int height;
	DFBSurfaceDescription dsc;
	IDirectFBSurface * primary;
	IDirectFBEventBuffer * event;
};
typedef struct dfb_info dfbInfo;

int
dfb_handle_event(rdpInst * inst);

#endif /* __DFB_EVENT_H */
