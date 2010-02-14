/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   Protocol services - ISO layer
   Copyright (C) Jay Sorg 2009

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

#include "parse.h"
#include "types_ui.h"

#ifndef __ISO_H
#define __ISO_H

struct rdp_iso
{
	int nla;
	struct rdp_mcs * mcs;
	struct rdp_tcp * tcp;
};
typedef struct rdp_iso rdpIso;

STREAM
iso_init(rdpIso * iso, int length);
void
iso_send(rdpIso * iso, STREAM s);
STREAM
iso_recv(rdpIso * iso, uint8 * rdpver);
RD_BOOL
iso_connect(rdpIso * iso, char * server, char * username, int port);
RD_BOOL
iso_reconnect(rdpIso * iso, char * server, int port);
void
iso_disconnect(rdpIso * iso);
void
iso_reset_state(rdpIso * iso);
rdpIso *
iso_new(struct rdp_mcs * mcs);
void
iso_free(rdpIso * iso);

#endif
