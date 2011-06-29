/*
   FreeRDP: A Remote Desktop Protocol client.
   Protocol services - ISO layer

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

#include <freerdp/types/ui.h>

#include "network.h"
#include "stream.h"
#include "nego.h"

#ifndef __ISO_H
#define __ISO_H

struct rdp_iso
{
	char* cookie;
	struct _NEGO * nego;
	struct rdp_tcp * tcp;
	struct rdp_mcs * mcs;
	struct rdp_network * net;
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
iso_new(struct rdp_network * net);
void
iso_free(rdpIso * iso);

#endif
