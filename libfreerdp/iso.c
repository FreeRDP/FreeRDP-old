/*
   FreeRDP: A Remote Desktop Protocol client.
   ISO layer

   Copyright (C) Matthew Chapman 1999-2008

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

#include "tcp.h"
#include "iso.h"
#include "mcs.h"
#include "nego.h"
#include "secure.h"
#include "credssp.h"
#include "rdp.h"
#include <freerdp/utils.h>
#include <freerdp/rdpset.h>

/* TPKT from T123 - aka ISO DP 8073 */

/* Output TPKT header for length length.
 * Length should include the TPKT header (4 bytes) */
static void
tpkt_output_header(STREAM s, int length)
{
	out_uint8(s, 3);	/* version */
	out_uint8(s, 0);	/* reserved */
	out_uint16_be(s, length);	/* length */
}

/* Try to read TPKT header for X.224 from stream and return length
 * (including the 4 bytes TPKT header already read).
 * If not possible then return untouched stream and length -1. */
static int
tpkt_input_header(STREAM s)
{
	if (*s->p == 3)	/* Peeking is less ugly than rewinding */
	{
		uint8 version;
		uint8 reserved;
		uint16 length;

		in_uint8(s, version);
		in_uint8(s, reserved);
		in_uint16_be(s, length);
		return length;
	}
	return -1;	/* Probably Fast-Path */
}

/* Output and send 7 bytes X.224 headers for
 * Client X.224 Connection Request TPDU (X224_TPDU_CONNECTION_REQUEST)
 * Server X.224 Connection Confirm TPDU
 * FIXME: is this also suitable for X224_TPDU_DISCONNECT_REQUEST ??? */
static void
x224_send_dst_src_class(rdpIso * iso, uint8 code)
{
	STREAM s;

	s = tcp_init(iso->tcp, 11);

	tpkt_output_header(s, 11);

	out_uint8(s, 6);	/* length indicator */
	out_uint8(s, code);
	out_uint16_le(s, 0);	/* dst_ref */
	out_uint16_le(s, 0);	/* src_ref */
	out_uint8(s, 0);	/* class */

	s_mark_end(s);
	tcp_send(iso->tcp, s);
}

/* Output and send X.224 Connection Request TPDU with routing for username */
void
x224_send_connection_request(rdpIso * iso)
{
	STREAM s;
	int length = 11;
	int cookie_length;

	cookie_length = strlen(iso->cookie);

	if (iso->mcs->sec->rdp->redirect_routingtoken)
		/* routingToken */
		length += iso->mcs->sec->rdp->redirect_routingtoken_len;
	else
		/* cookie */
		length += 19 + cookie_length;

	if (iso->nego->requested_protocols > PROTOCOL_RDP)
		length += 8;

	/* FIXME: Use x224_send_dst_src_class */
	s = tcp_init(iso->tcp, length);

	tpkt_output_header(s, length);

	/* X.224 Connection Request (CR) TPDU */
	out_uint8(s, length - 5);	/* length indicator */
	out_uint8(s, X224_TPDU_CONNECTION_REQUEST);
	out_uint16_le(s, 0);	/* dst_ref */
	out_uint16_le(s, 0);	/* src_ref */
	out_uint8(s, 0);	/* class */

	if (iso->mcs->sec->rdp->redirect_routingtoken)
	{
		/* routingToken */
		out_uint8p(s, iso->mcs->sec->rdp->redirect_routingtoken, iso->mcs->sec->rdp->redirect_routingtoken_len);
	}
	else
	{
		/* cookie */
		out_uint8p(s, "Cookie: mstshash=", strlen("Cookie: mstshash="));
		out_uint8p(s, iso->cookie, cookie_length);
		out_uint8(s, 0x0D);	/* CR */
		out_uint8(s, 0x0A);	/* LF */
	}

	if (iso->nego->requested_protocols > PROTOCOL_RDP)
	{
		out_uint8(s, TYPE_RDP_NEG_REQ); /* When using TLS, NLA, or both, RDP_NEG_DATA should be present */
		out_uint8(s, 0x00);	/* flags, must be set to zero */
		out_uint16_le(s, 8);	/* RDP_NEG_DATA length (8) */
		out_uint32_le(s, iso->nego->requested_protocols); /* requestedProtocols */
	}

	s_mark_end(s);
	tcp_send(iso->tcp, s);
}

/* Receive an X.224 TPDU */
static STREAM
x224_recv(rdpIso * iso, STREAM s, int length, uint8 * pcode)
{
	uint8 lengthIndicator;
	uint8 code;
	uint8 subcode;

	s = tcp_recv(iso->tcp, s, length - 4);

	if (s == NULL)
		return NULL;

	/* X.224 TPDU Header */
	in_uint8(s, lengthIndicator);
	in_uint8(s, code);

	subcode = code & 0x0F;	/* get the lower nibble */
	code &= 0xF0;		/* take out lower nibble */

	*pcode = code;

	if (code == X224_TPDU_DATA)
	{
		in_uint8s(s, 1);	/* EOT */
		return s;
	}

	/* dst-ref (2 bytes) */
	/* src-ref (2 bytes) */
	/* class option (1 byte) */
	in_uint8s(s, 5);

	switch (code)
	{
		/* Connection Request */
		case X224_TPDU_CONNECTION_REQUEST:
			break;

		/* Connection Confirm */
		case X224_TPDU_CONNECTION_CONFIRM:
			break;

		/* Disconnect Request */
		case X224_TPDU_DISCONNECT_REQUEST:
			break;

		/* Data */
		case X224_TPDU_DATA:
			break;

		/* Error */
		case X224_TPDU_ERROR:
			break;
	}

	/* According to X.224 13.4 and [MS-RDPBCGR] 2.2.1.2, the rdpNegData field is optional
	   and its length is included in the X.224 length indicator */
	if (lengthIndicator > 6)
	{
		nego_recv(iso->nego, s);
	}

	return s;
}

/* Receive a packet from tcp and return stream.
 * If no ptype then only TPKT header with X.224 is accepted.
 * If ptype then Fast-Path packets are accepted too.
 * Return NULL on error. */
STREAM
tpkt_recv(rdpIso * iso, uint8 * pcode, isoRecvType * ptype)
{
	STREAM s;
	int length;

	s = tcp_recv(iso->tcp, NULL, 4);

	if (s == NULL)
		return NULL;

	length = tpkt_input_header(s);

	if (length >= 0)
	{
		/* Valid TPKT header, payload is X.224 TPDU */
		if (ptype != NULL)
			*ptype = ISO_RECV_X224;

		s = x224_recv(iso, s, length, pcode);
		return s;
	}
	else if (ptype != NULL)
	{
		/* Fast-Path header */
		uint8 fpInputHeader;

		in_uint8(s, fpInputHeader);
		ASSERT((fpInputHeader & 3) == 0);	/* assume actionCode FASTPATH_OUTPUT_ACTION_FASTPATH */
		*ptype = (fpInputHeader & 0x80) ? ISO_RECV_FAST_PATH_ENCRYPTED : ISO_RECV_FAST_PATH;
		/* TODO: 0x40 FASTPATH_OUTPUT_SECURE_CHECKSUM indicates salted MAC */

		in_uint8(s, length);
		if (length & 0x80)
		{
			length &= ~0x80;
			next_be(s, length);
		}

		s = tcp_recv(iso->tcp, s, length - 4);
		return s;
	}
	return NULL;	/* Fast-Path not allowed */
}

/* Receive a message on the ISO layer, return code */
static STREAM
iso_recv_msg(rdpIso * iso, uint8 * code, isoRecvType * ptype)
{
	return tpkt_recv(iso, code, ptype);
}

/* Initialize ISO transport data packet */
STREAM
iso_init(rdpIso * iso, int length)
{
	STREAM s;
	s = tcp_init(iso->tcp, length + 7);
	s_push_layer(s, iso_hdr, 7);
	return s;
}

/* Initialize fast path data packet */
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

	out_uint8(s, 3);		/* version */
	out_uint8(s, 0);		/* reserved */
	out_uint16_be(s, length);

	out_uint8(s, 2);		/* hdrlen */
	out_uint8(s, X224_TPDU_DATA);	/* code */
	out_uint8(s, 0x80);		/* eot */

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

/* Receive ISO transport data packet
 * If ptype is NULL then only X224 is accepted */
STREAM
iso_recv(rdpIso * iso, isoRecvType * ptype)
{
	STREAM s;
	uint8 code = 0;

	s = iso_recv_msg(iso, &code, ptype);

	if (s == NULL)
		return NULL;

	if ((ptype != NULL) &&
		(*ptype == ISO_RECV_X224) &&
		(code != X224_TPDU_DATA))
	{
		ui_error(iso->mcs->sec->rdp->inst, "expected X224_TPDU_DATA, got 0x%x\n", code);
		return NULL;
	}

	return s;
}

/* Establish a connection up to the ISO layer */
RD_BOOL
iso_connect(rdpIso * iso, char *server, char *username, int port)
{
	if (strlen(iso->mcs->sec->rdp->settings->domain) > 0)
		iso->cookie = iso->mcs->sec->rdp->settings->domain;
	else
		iso->cookie = username;

	iso->nego->port = port;
	iso->nego->hostname = server;
	iso->nego->tcp_connected = 0;

	if (nego_connect(iso->nego) > 0)
	{
		return True;
	}
	else
	{
		printf("Protocol security negotiation failure, disconnecting\n");
		return False;
	}
}

/* Disconnect from the ISO layer */
void
iso_disconnect(rdpIso * iso)
{
	x224_send_dst_src_class(iso, X224_TPDU_DISCONNECT_REQUEST);
#ifndef DISABLE_TLS
	if (iso->mcs->sec->tls)
		tls_disconnect(iso->mcs->sec->tls);
#endif
	tcp_disconnect(iso->tcp);
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
		self->nego = nego_new(self);
	}

	return self;
}

void
iso_free(rdpIso * iso)
{
	if (iso != NULL)
	{
		tcp_free(iso->tcp);
		if (iso->nego != NULL)
			nego_free(iso->nego);
		xfree(iso);
	}
}
