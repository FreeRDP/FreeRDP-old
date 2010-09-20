/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifndef __DFBFREERDP_H
#define __DFBFREERDP_H

#include <directfb.h>
#include <freerdp/freerdp.h>
#include "libfreerdpgdi.h"

#define SET_DFBI(_inst, _dfbi) (_inst)->param1 = _dfbi
#define GET_DFBI(_inst) ((dfbInfo *) ((_inst)->param1))

struct dfb_info
{
	GDI *gdi;
	DFBResult err;
	IDirectFB *dfb;
	DFBEvent events[16];
	DFBSurfaceDescription dsc;
	IDirectFBSurface *primary;
	IDirectFBEventBuffer *event;
	IDirectFBSurface *surface;
	IDirectFBDisplayLayer *layer;
	DFBRectangle update_rect;
};
typedef struct dfb_info dfbInfo;

int
dfb_process_event(rdpInst * inst, DFBEvent * event);

#endif /* __DFBFREERDP_H */
