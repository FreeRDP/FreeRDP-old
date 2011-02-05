/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   RDP Protocol Security Negotiation

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2011

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

#ifndef __NEGO_H
#define __NEGO_H

#include "frdp.h"
#include "stream.h"
#include "iso.h"

enum _NEGO_STATE
{
	NEGO_STATE_INITIAL,
	NEGO_STATE_NLA, /* Network Level Authentication (TLS implicit) */
	NEGO_STATE_TLS, /* TLS Encryption without NLA */
	NEGO_STATE_RDP, /* Standard Legacy RDP Encryption */
	NEGO_STATE_FAIL, /* Negotiation failure */
	NEGO_STATE_FINAL
};
typedef enum _NEGO_STATE NEGO_STATE;

struct _NEGO
{
	NEGO_STATE state;
	struct rdp_iso * iso;
	uint32 selected_protocol;
	uint32 requested_protocols;
};
typedef struct _NEGO NEGO;

int nego_connect(NEGO *nego);

void nego_attempt_nla(NEGO *nego);
void nego_attempt_tls(NEGO *nego);
void nego_attempt_rdp(NEGO *nego);

void nego_send(NEGO *nego);
void nego_recv(NEGO *nego, STREAM s);

void nego_process_negotiation_response(NEGO *nego, STREAM s);
void nego_process_negotiation_failure(NEGO *nego, STREAM s);

NEGO* nego_new(rdpIso * iso);
void nego_init(NEGO *nego);
void nego_free(NEGO *nego);

#endif /* __NEGO_H */
