/*
   FreeRDP: A Remote Desktop Protocol client.
   Protocol services - Multipoint Communications Service

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

#ifndef __MCS_H
#define __MCS_H

#include "iso.h"
#include <freerdp/utils/debug.h>

struct rdp_mcs
{
	struct rdp_sec * sec;
	uint16 mcs_userid;
	struct rdp_iso * iso;
	struct rdp_channels * chan;
};
typedef struct rdp_mcs rdpMcs;

STREAM
mcs_init(rdpMcs * mcs, int length);
STREAM
mcs_fp_init(rdpMcs * mcs, int length);
void
mcs_send_to_channel(rdpMcs * mcs, STREAM s, uint16 channel);
void
mcs_send(rdpMcs * mcs, STREAM s);
void
mcs_fp_send(rdpMcs * mcs, STREAM s, uint32 flags);
STREAM
mcs_recv(rdpMcs * mcs, isoRecvType * ptype, uint16 * channel);
RD_BOOL
mcs_connect(rdpMcs * mcs);
void
mcs_disconnect(rdpMcs * mcs);
rdpMcs *
mcs_new(struct rdp_sec * secure);
void
mcs_free(rdpMcs * mcs);

#ifdef WITH_DEBUG_MCS
#define DEBUG_MCS(fmt, ...) DEBUG_CLASS(MCS, fmt, ...)
#else
#define DEBUG_MCS(fmt, ...) DEBUG_NULL(fmt, ...)
#endif

#endif
