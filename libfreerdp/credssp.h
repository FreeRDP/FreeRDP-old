/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Credential Security Support Provider (CredSSP)

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

#ifndef __CREDSSP_H
#define __CREDSSP_H

#include "secure.h"

struct _AV_PAIR
{
	uint16 length;
	uint8* value;
};
typedef struct _AV_PAIR AV_PAIR;

struct _AV_PAIRS
{
	AV_PAIR NbComputerName;
	AV_PAIR NbDomainName;
	AV_PAIR DnsComputerName;
	AV_PAIR DnsDomainName;
	AV_PAIR DnsTreeName;
	AV_PAIR Timestamp;
	AV_PAIR Restrictions;
	AV_PAIR TargetName;
	AV_PAIR ChannelBindings;
	uint32 Flags;
};
typedef struct _AV_PAIRS AV_PAIRS;

enum _AV_ID
{
	MsvAvEOL,
	MsvAvNbComputerName,
	MsvAvNbDomainName,
	MsvAvDnsComputerName,
	MsvAvDnsDomainName,
	MsvAvDnsTreeName,
	MsvAvFlags,
	MsvAvTimestamp,
	MsvAvRestrictions,
	MsvAvTargetName,
	MsvChannelBindings
};
typedef enum _AV_ID AV_ID;

struct rdp_nla
{
	struct rdp_sec * sec;
	AV_PAIRS* target_info;
	uint8* target_name;
	uint32 negotiate_flags;
	uint8 server_challenge[8];
};
typedef struct rdp_nla rdpNla;

void
credssp_send(rdpSec * sec, STREAM s);
void
credssp_recv(rdpSec * sec);

void
ntlm_send_negotiate_message(rdpSec * sec);
void
ntlm_recv_challenge_message(rdpSec * sec, STREAM s);
void
ntlm_send_authenticate_message(rdpSec * sec);
void
ntlm_recv(rdpSec * sec, STREAM s);

rdpNla *
nla_new(rdpSec * sec);
void
nla_free(rdpNla * nla);

#endif // __CREDSSP_H
