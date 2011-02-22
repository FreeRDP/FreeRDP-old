/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP Protocol Security Negotiation

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

#include <stdio.h>
#include <string.h>

#include "frdp.h"
#include "rdp.h"
#include "mem.h"
#include "stream.h"

#include "nego.h"

int nego_connect(NEGO *nego)
{
	if (nego->state == NEGO_STATE_INITIAL)
	{
		if (nego->enabled_protocols[PROTOCOL_NLA] > 0)
			nego->state = NEGO_STATE_NLA;
		else if (nego->enabled_protocols[PROTOCOL_TLS] > 0)
			nego->state = NEGO_STATE_TLS;
		else if (nego->enabled_protocols[PROTOCOL_RDP] > 0)
			nego->state = NEGO_STATE_RDP;
		else
			nego->state = NEGO_STATE_FAIL;
	}

	while (nego->state != NEGO_STATE_FINAL)
	{
		nego_send(nego);

		if (nego->state == NEGO_STATE_FAIL)
		{
			nego->state = NEGO_STATE_FINAL;
			return 0;
		}
	}

	return 1;
}

void nego_attempt_nla(NEGO *nego)
{
	uint8 code;
	nego->requested_protocols = PROTOCOL_NLA | PROTOCOL_TLS;
	x224_send_connection_request(nego->iso);
	tpkt_recv(nego->iso, &code, NULL);

	if (nego->state != NEGO_STATE_FINAL)
		nego->state = NEGO_STATE_TLS;
}

void nego_attempt_tls(NEGO *nego)
{
	uint8 code;
	nego->requested_protocols = PROTOCOL_TLS;
	x224_send_connection_request(nego->iso);
	tpkt_recv(nego->iso, &code, NULL);

	if (nego->state != NEGO_STATE_FINAL)
		nego->state = NEGO_STATE_RDP;
}

void nego_attempt_rdp(NEGO *nego)
{
	uint8 code;
	nego->requested_protocols = PROTOCOL_RDP;
	x224_send_connection_request(nego->iso);

	if (tpkt_recv(nego->iso, &code, NULL) == NULL)
		nego->state = NEGO_STATE_FAIL;
	else
		nego->state = NEGO_STATE_FINAL;
}

void nego_recv(NEGO *nego, STREAM s)
{
	uint8 type;
	in_uint8(s, type); /* Type */

	switch (type)
	{
		case TYPE_RDP_NEG_RSP:
			nego_process_negotiation_response(nego, s);
			break;
		case TYPE_RDP_NEG_FAILURE:
			nego_process_negotiation_failure(nego, s);
			break;
	}
}

void nego_send(NEGO *nego)
{
	if (nego->state == NEGO_STATE_NLA)
		nego_attempt_nla(nego);
	else if (nego->state == NEGO_STATE_TLS)
		nego_attempt_tls(nego);
	else if (nego->state == NEGO_STATE_RDP)
		nego_attempt_rdp(nego);
}

/* Process Negotiation Response from Connection Confirm */
void nego_process_negotiation_response(NEGO *nego, STREAM s)
{
	uint8 flags;
	uint16 length;
	uint32 selectedProtocol;

	in_uint8(s, flags);
	in_uint16_le(s, length);
	in_uint32_le(s, selectedProtocol);

	if (selectedProtocol == PROTOCOL_NLA)
		nego->selected_protocol = PROTOCOL_NLA;
	else if (selectedProtocol == PROTOCOL_TLS)
		nego->selected_protocol = PROTOCOL_TLS;
	else if (selectedProtocol == PROTOCOL_RDP)
		nego->selected_protocol = PROTOCOL_RDP;

	nego->state = NEGO_STATE_FINAL;
}

/* Process Negotiation Failure from Connection Confirm */
void nego_process_negotiation_failure(NEGO *nego, STREAM s)
{
	uint8 flags;
	uint16 length;
	uint32 failureCode;

	in_uint8(s, flags);
	in_uint16_le(s, length);
	in_uint32_le(s, failureCode);

	switch (failureCode)
	{
		case SSL_REQUIRED_BY_SERVER:
			printf("Error: SSL_REQUIRED_BY_SERVER\n");
			break;
		case SSL_NOT_ALLOWED_BY_SERVER:
			printf("Error: SSL_NOT_ALLOWED_BY_SERVER\n");
			break;
		case SSL_CERT_NOT_ON_SERVER:
			printf("Error: SSL_CERT_NOT_ON_SERVER\n");
			break;
		case INCONSISTENT_FLAGS:
			printf("Error: INCONSISTENT_FLAGS\n");
			break;
		case HYBRID_REQUIRED_BY_SERVER:
			printf("Error: HYBRID_REQUIRED_BY_SERVER\n");
			break;
		default:
			printf("Error: Unknown protocol security error %d\n", failureCode);
			break;
	}
}

NEGO* nego_new(struct rdp_iso * iso)
{
	/* Create new NEGO state machine instance */

	NEGO *nego = (NEGO*) xmalloc(sizeof(NEGO));

	if (nego != NULL)
	{
		memset(nego, '\0', sizeof(NEGO));
		nego->iso = iso;
		nego_init(nego);
	}

	return nego;
}

void nego_init(NEGO *nego)
{
	nego->state = NEGO_STATE_INITIAL;
	nego->requested_protocols = PROTOCOL_RDP;
}

void nego_free(NEGO *nego)
{
	xfree(nego);
}
