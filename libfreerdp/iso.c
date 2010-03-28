/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - ISO layer
   Copyright (C) Matthew Chapman 1999-2008

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

#include "tcp.h"
#include "iso.h"
#include "mcs.h"
#include "secure.h"
#include "credssp.h"
#include "rdp.h"
#include "mem.h"

#include <openssl/ssl.h>

/* output TPKT header */

static void
output_tpkt_header(STREAM s, int length)
{
	out_uint8(s, 3);		/* version */
	out_uint8(s, 0);		/* reserved */
	out_uint16_be(s, length);	/* length */
}

static uint16
input_tpkt_header(STREAM s, uint8 * rdpver)
{
	uint8 version;
	uint8 reserved;
	uint16 length;

	in_uint8(s, version);		/* version */
	in_uint8(s, reserved);		/* reserved */
	in_uint16_be(s, length);	/* length */

	if (rdpver != NULL)
		*rdpver = version;

	if (version == 3 && reserved == 0)
	{
		return length;
	}
	else
	{
		/* not a TPKT header, revert changes */
		s->p -= 4;
		return 0;
	}
}

/* Send a self-contained ISO PDU */
static void
iso_send_msg(rdpIso * iso, uint8 code)
{
	STREAM s;

	s = tcp_init(iso->tcp, 11);

	output_tpkt_header(s, 11);

	out_uint8(s, 6);	/* hdrlen */
	out_uint8(s, code);
	out_uint16(s, 0);	/* dst_ref */
	out_uint16(s, 0);	/* src_ref */
	out_uint8(s, 0);	/* class */

	s_mark_end(s);
	tcp_send(iso->tcp, s);
}

static void
iso_send_connection_request(rdpIso * iso, char *username)
{
	STREAM s;
	int length = 30 + strlen(username);

	if (iso->mcs->sec->nla)
		length += 8;

	/*
	 * negotiation_state is used so that tcp_recv() will be able to make
	 * the distinction between an unexpected disconnection and a
	 * disconnection that is to be expected if negotation fails, such as
	 * when the client claims to support TLS with NLA but the server only
	 * supports the legacy encryption.
	 */

	s = tcp_init(iso->tcp, length);

	output_tpkt_header(s, length);

	/* X.224 Connection Request (CR) TPDU */
	out_uint8(s, length - 5);	/* hdrlen */
	out_uint8(s, ISO_PDU_CR);
	out_uint16(s, 0);	/* dst_ref */
	out_uint16(s, 0);	/* src_ref */
	out_uint8(s, 0);	/* class */

	/* cookie */
	out_uint8p(s, "Cookie: mstshash=", strlen("Cookie: mstshash="));
	out_uint8p(s, username, strlen(username));

	/* routingToken */
	out_uint8(s, 0x0D);	/* CR */
	out_uint8(s, 0x0A);	/* LF */

	if (iso->mcs->sec->nla)
	{
		/* When using NLA, the RDP_NEG_DATA field should be present */
		out_uint8(s, 0x01);	/* TYPE_RDP_NEG_REQ */
		out_uint8(s, 0x00);	/* flags, must be set to zero */
		out_uint16(s, 8);	/* RDP_NEG_DATA length (8) */
		out_uint32(s, 0x00000003);	/* requestedProtocols, PROTOCOL_HYBRID_FLAG | PROTOCOL_SSL_FLAG */
	}

	s_mark_end(s);
	tcp_send(iso->tcp, s);
}

/* Process Negotiation Response */
uint32
rdp_process_negotiation_response(rdpIso * iso, STREAM s)
{
	uint8 flags;
	uint16 length;
	uint32 selectedProtocol;

	in_uint8(s, flags);
	in_uint16_le(s, length);
	in_uint32_le(s, selectedProtocol);

	if (iso->mcs->sec->nla)
	{
		switch (selectedProtocol)
		{
			case PROTOCOL_RDP:
				printf("Selected PROTOCOL_RDP Security\n");
				break;
			case PROTOCOL_SSL:
				printf("Selected PROTOCOL_SSL Security\n");
				break;
			case PROTOCOL_HYBRID:
				printf("Selected PROTOCOL_HYBRID Security\n");
				break;
			default:
				printf("Error: Unknown protocol security\n");
				break;
		}
	}

	return selectedProtocol;
}

/* Process Negotiation Failure */
void
rdp_process_negotiation_failure(rdpIso * iso, STREAM s)
{
	uint8 flags;
	uint16 length;
	uint32 failureCode;

	in_uint8(s, flags);
	in_uint16_le(s, length);
	in_uint32_le(s, failureCode);

	if (iso->mcs->sec->nla)
	{
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
				printf("Error: Unknown protocol security error\n");
				break;
		}
	}
}

/* Receive an X.224 TPDU */
static STREAM
x224_recv(rdpIso * iso, STREAM s, int length, uint8* pcode)
{
	uint8 lengthIndicator;
	uint8 code;
	uint8 subcode;
	uint8 type;

	s = tcp_recv(iso->tcp, s, length - 4);

	if (s == NULL)
		return NULL;

	/* X.224 TPDU Header */
	in_uint8(s, lengthIndicator);
	in_uint8(s, code);

	subcode = code & 0x0F; /* get the lower nibble */
	code &= 0xF0; /* take out lower nibble */

	*pcode = code;

	if (code == ISO_PDU_DT)
	{
		in_uint8s(s, 1); /* EOT */
		return s;
	}

	/* dst-ref (2 bytes) */
	/* src-ref (2 bytes) */
	/* class option (1 byte) */
	in_uint8s(s, 5);

	in_uint8(s, type); /* Type */

	switch (code)
	{
		/* Connection Request */
		case ISO_PDU_CR:
			printf("ISO_PDU_CR\n");
			break;

		/* Connection Confirm */
		case ISO_PDU_CC:
			printf("ISO_PDU_CC\n");
			break;

		/* Disconnect Request */
		case ISO_PDU_DR:
			printf("ISO_PDU_DR\n");
			break;

		/* Data */
		case ISO_PDU_DT:
			printf("ISO_PDU_DT\n");
			break;

		/* Error */
		case ISO_PDU_ER:
			printf("ISO_PDU_ER\n");
			break;
	}

	switch (type)
	{
		case TYPE_RDP_NEG_RSP:
			printf("TYPE_RDP_NEG_RSP\n");
			rdp_process_negotiation_response(iso, s);
			break;
		case TYPE_RDP_NEG_FAILURE:
			printf("TYPE_RDP_NEG_FAILURE\n");
			rdp_process_negotiation_failure(iso, s);
			break;
	}

	return s;
}

/* Receive a packet with a TPKT header */
static STREAM
tpkt_recv(rdpIso * iso, uint8* pcode, uint8* rdpver)
{
	STREAM s;
	uint8 version;
	uint16 length;

	s = tcp_recv(iso->tcp, NULL, 4);

	if (s == NULL)
		return NULL;

	length = input_tpkt_header(s, &version);

	if (rdpver != NULL)
		*rdpver = version;

	if (version == 3)
	{
		/* Valid TPKT header, payload is X.224 TPDU */
		return x224_recv(iso, s, length, pcode);
	}
	else
	{
		/* not a TPKT header */

		/* nasty hack from previous spaghetti code */
		in_uint8s(s, 1);
		in_uint8(s, length);

		if (length & 0x80)
		{
			length &= ~0x80;
			next_be(s, length);
		}

		s = tcp_recv(iso->tcp, s, length - 4);

		if (s == NULL)
			return NULL;
	}

	return s;
}

static RD_BOOL
iso_negotiate_encryption(rdpIso * iso, char *username)
{
	uint8 code;
	uint8 version;

	if (iso->mcs->sec->nla == 0)
	{
		/* We do no use NLA, so we won't attempt to negotiate */

		iso->mcs->sec->negotiation_state = 2;
		iso_send_connection_request(iso, username);

		/* Receive negotiation response */
		if (tpkt_recv(iso, &code, &version) == NULL)
			return False;
	}
	else
	{
		/* first negotiation attempt */
		iso->mcs->sec->negotiation_state = 1;

		iso_send_connection_request(iso, username);

		/* Attempt to receive negotiation response */
		if (tpkt_recv(iso, &code, &version) == NULL)
		{
			if (iso->mcs->sec->negotiation_state == -1)
			{
				/* Negotiation failure, downgrade encryption and try again */

				iso->mcs->sec->nla = 0;

				/* second negotiation attempt */
				iso->mcs->sec->negotiation_state = 2;

				iso_send_connection_request(iso, username);

				/* Receive negotiation response */
				if (tpkt_recv(iso, &code, &version) == NULL)
					return False;
			}
		}
	}

	return True;
}

/* Receive a message on the ISO layer, return code */
static STREAM
iso_recv_msg(rdpIso * iso, uint8 * code, uint8 * rdpver)
{	
	return tpkt_recv(iso, code, rdpver);
}

/* Initialise ISO transport data packet */
STREAM
iso_init(rdpIso * iso, int length)
{
	STREAM s;

	s = tcp_init(iso->tcp, length + 7);
	s_push_layer(s, iso_hdr, 7);

	return s;
}

/* Initialise fast path data packet */
STREAM
iso_fp_init(rdpIso * iso, int length)
{
	STREAM s;

	s = tcp_init(iso->tcp, length + 3);
	s_push_layer(s, iso_hdr, 3);
	return s;
}

/* Send an ISO data PDU */
void
iso_send(rdpIso * iso, STREAM s)
{
	uint16 length;

	s_pop_layer(s, iso_hdr);
	length = s->end - s->p;

	out_uint8(s, 3);	/* version */
	out_uint8(s, 0);	/* reserved */
	out_uint16_be(s, length);

	out_uint8(s, 2);	/* hdrlen */
	out_uint8(s, ISO_PDU_DT);	/* code */
	out_uint8(s, 0x80);	/* eot */

	tcp_send(iso->tcp, s);
}

/* Send an fast path data PDU */
void
iso_fp_send(rdpIso * iso, STREAM s, uint32 flags)
{
	int fp_flags;
	int len;
	int index;

	fp_flags = (1 << 2) | 0;	/* one event, fast path */
	if (flags & SEC_ENCRYPT)
	{
		fp_flags |= 2 << 6;	/* FASTPATH_INPUT_ENCRYPTED */
	}
	s_pop_layer(s, iso_hdr);
	len = (int) (s->end - s->p);
	out_uint8(s, fp_flags);
	if (len >= 128)
	{
		out_uint16_be(s, len | 0x8000);
	}
	else
	{
		/* copy the bits up to pack and save 1 byte */
		for (index = 3; index < len; index++)
		{
			s->data[index - 1] = s->data[index];
		}
		len--;
		s->end--;
		out_uint8(s, len);
	}
	tcp_send(iso->tcp, s);
}

/* Receive ISO transport data packet */
STREAM
iso_recv(rdpIso * iso, uint8 * rdpver)
{
	STREAM s;
	uint8 code = 0;

	s = iso_recv_msg(iso, &code, rdpver);

	if (s == NULL)
		return NULL;

	if (rdpver != NULL)
		if (*rdpver != 3)
			return s;

	if (code != ISO_PDU_DT)
	{
		ui_error(iso->mcs->sec->rdp->inst, "expected DT, got 0x%x\n", code);
		return NULL;
	}

	return s;
}

/* Establish a connection up to the ISO layer */
RD_BOOL
iso_connect(rdpIso * iso, char *server, char *username, int port)
{
	if (!tcp_connect(iso->tcp, server, port))
		return False;

	return iso_negotiate_encryption(iso, username);
}

/* Establish a reconnection up to the ISO layer */
RD_BOOL
iso_reconnect(rdpIso * iso, char *server, int port)
{
	uint8 code = 0;

	if (!tcp_connect(iso->tcp, server, port))
		return False;

	iso_send_msg(iso, ISO_PDU_CR);

	if (iso_recv_msg(iso, &code, NULL) == NULL)
		return False;

	if (code != ISO_PDU_CC)
	{
		ui_error(iso->mcs->sec->rdp->inst, "expected CC, got 0x%x\n", code);
		tcp_disconnect(iso->tcp);
		return False;
	}

	return True;
}

/* Disconnect from the ISO layer */
void
iso_disconnect(rdpIso * iso)
{
	iso_send_msg(iso, ISO_PDU_DR);
	tcp_disconnect(iso->tcp);
}

/* reset the state to support reconnecting */
void
iso_reset_state(rdpIso * iso)
{
	tcp_reset_state(iso->tcp);
}

rdpIso *
iso_new(struct rdp_mcs *mcs)
{
	rdpIso *self;

	self = (rdpIso *) xmalloc(sizeof(rdpIso));

	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpIso));
		self->mcs = mcs;
		self->tcp = tcp_new(self);
	}

	return self;
}

void
iso_free(rdpIso * iso)
{
	if (iso != NULL)
	{
		tcp_free(iso->tcp);
		xfree(iso);
	}
}
