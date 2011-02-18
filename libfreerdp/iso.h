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

#include <freerdp/types_ui.h>
#include "stream.h"
#include "nego.h"

#ifndef __ISO_H
#define __ISO_H

struct rdp_iso
{
        char* cookie;
	struct _NEGO * nego;
	struct rdp_mcs * mcs;
	struct rdp_tcp * tcp;
};
typedef struct rdp_iso rdpIso;

enum iso_recv_type
{
	ISO_RECV_X224,
	ISO_RECV_FAST_PATH,
	ISO_RECV_FAST_PATH_ENCRYPTED
};
typedef enum iso_recv_type isoRecvType;

STREAM
iso_init(rdpIso * iso, int length);
STREAM
iso_fp_init(rdpIso * iso, int length);
void
iso_send(rdpIso * iso, STREAM s);
void
iso_fp_send(rdpIso * iso, STREAM s, uint32 flags);
void
x224_send_connection_request(rdpIso * iso);
STREAM
iso_recv(rdpIso * iso, isoRecvType * ptype);
STREAM
tpkt_recv(rdpIso * iso, uint8 * pcode, isoRecvType * ptype);
RD_BOOL
iso_connect(rdpIso * iso, char * server, char * username, int port);
void
iso_disconnect(rdpIso * iso);
rdpIso *
iso_new(struct rdp_mcs * mcs);
void
iso_free(rdpIso * iso);

#endif
