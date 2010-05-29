
#ifndef __DFB_EVENT_H
#define __DFB_EVENT_H

#include <freerdp/freerdp.h>

#define SET_XFI(_inst, _xfi) (_inst)->param1 = _xfi
#define GET_XFI(_inst) ((xfInfo *) ((_inst)->param1))

typedef void xfInfo;

int
dfb_handle_event(rdpInst * inst);

#endif /* __DFB_EVENT_H */
