/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   Protocol services - Multipoint Communications Service
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

#ifndef __MCS_H
#define __MCS_H

#include "iso.h"

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

#endif
