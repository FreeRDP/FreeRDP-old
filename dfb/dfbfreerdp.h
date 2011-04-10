/*
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI

   Copyright 2010 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __DFBFREERDP_H
#define __DFBFREERDP_H

#include <directfb.h>
#include <freerdp/freerdp.h>
#include "gdi.h"

#define SET_DFBI(_inst, _dfbi) (_inst)->param1 = _dfbi
#define GET_DFBI(_inst) ((dfbInfo *) ((_inst)->param1))

struct dfb_info
{
	DFBResult err;
	IDirectFB *dfb;
	DFBEvent event;
	DFBSurfaceDescription dsc;
	IDirectFBSurface *primary;
	IDirectFBSurface *surface;
	IDirectFBDisplayLayer *layer;
	DFBRectangle update_rect;
	IDirectFBEventBuffer *event_buffer;
	int read_fds;
};
typedef struct dfb_info dfbInfo;

int
dfb_process_event(rdpInst * inst, DFBEvent * event);

#endif /* __DFBFREERDP_H */
