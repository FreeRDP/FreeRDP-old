/*
   FreeRDP: A Remote Desktop Protocol client.
   Protocol services - TCP layer

   Copyright (C) Jay Sorg 2009-2011

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

#ifndef __TCP_H
#define __TCP_H

#include <freerdp/types/ui.h>
#include "frdp.h"
#include "stream.h"

struct rdp_tcp
{
	int sock;
	char ipaddr[32];
	int tcp_port_rdp;
	struct rdp_network * net;
#ifdef _WIN32
	WSAEVENT wsa_event;
#endif
};
typedef struct rdp_tcp rdpTcp;

void
tcp_write(rdpTcp * tcp, STREAM s);
int
tcp_read(rdpTcp * tcp, char* b, int length);

RD_BOOL
tcp_can_send(int sck, int millis);
RD_BOOL
tcp_can_recv(int sck, int millis);
RD_BOOL
tcp_connect(rdpTcp * tcp, char * server, int port);
void
tcp_disconnect(rdpTcp * tcp);
char *
tcp_get_address(rdpTcp * tcp);
rdpTcp *
tcp_new(struct rdp_network * net);
void
tcp_free(rdpTcp * tcp);

#endif
