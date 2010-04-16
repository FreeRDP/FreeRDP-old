/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   Protocol services - TCP layer
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

#ifndef __TCP_H
#define __TCP_H

#include "frdp.h"
#include "types_ui.h"
#include "stream.h"

struct rdp_tcp
{
	struct rdp_iso * iso;
	int sock;
	struct stream in;
	struct stream out;
	int tcp_port_rdp;
	char ipaddr[32];
#ifdef _WIN32
	WSAEVENT wsa_event;
#endif
};
typedef struct rdp_tcp rdpTcp;

RD_BOOL
tcp_can_send(int sck, int millis);
RD_BOOL
tcp_can_recv(int sck, int millis);
STREAM
tcp_init(rdpTcp * tcp, uint32 minsize);
void
tcp_send(rdpTcp * tcp, STREAM s);
STREAM
tcp_recv(rdpTcp * tcp, STREAM s, uint32 length);
RD_BOOL
tcp_connect(rdpTcp * tcp, char * server, int port);
void
tcp_disconnect(rdpTcp * tcp);
char *
tcp_get_address(rdpTcp * tcp);
void
tcp_reset_state(rdpTcp * tcp);
rdpTcp *
tcp_new(struct rdp_iso * iso);
void
tcp_free(rdpTcp * tcp);

#endif
