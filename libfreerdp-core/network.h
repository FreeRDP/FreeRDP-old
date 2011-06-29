/*
   FreeRDP: A Remote Desktop Protocol client.
   Network Transport Abstraction Layer

   Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __NETWORK_H
#define __NETWORK_H

#include <freerdp/freerdp.h>
#include <freerdp/types/ui.h>

#include "tcp.h"
#include "tls.h"
#include "iso.h"
#include "mcs.h"
#include "rdp.h"
#include "security.h"
#include "stream.h"
#include "credssp.h"
#include "license.h"

struct rdp_network
{
	int port;
	char* server;
	char* username;
	struct stream in;
	struct stream out;
	int tls_connected;
	struct _NEGO * nego;
	struct rdp_rdp * rdp;
	struct rdp_tcp * tcp;
	struct rdp_sec * sec;
	struct rdp_tls * tls;
	struct rdp_iso * iso;
	struct rdp_mcs * mcs;
	struct rdp_credssp * credssp;
	struct rdp_license * license;
};
typedef struct rdp_network rdpNetwork;

STREAM
network_stream_init(rdpNetwork * net, uint32 min_size);
RD_BOOL
network_connect(rdpNetwork * net, char* server, char* username, int port);
void
network_disconnect(rdpNetwork * net);

void
network_send(rdpNetwork * net, STREAM s);
STREAM
network_recv(rdpNetwork * net, STREAM s, uint32 length);

rdpNetwork*
network_new(rdpRdp * rdp);
void
network_free(rdpNetwork * net);

#endif /* __NETWORK_H */
