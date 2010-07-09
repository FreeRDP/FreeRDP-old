/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - Multipoint Communications Service
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

#include "frdp.h"
#include "iso.h"
#include "mcs.h"
#include "chan.h"
#include "secure.h"
#include "rdp.h"
#include "mem.h"
#include "asn1.h"
#include "rdpset.h"
#include "debug.h"
#include "tcp.h"

/* Output a DOMAIN_PARAMS structure (ASN.1 BER) */
static void
mcs_out_domain_params(STREAM s, int max_channels, int max_users, int max_tokens, int max_pdusize)
{
	ber_out_header(s, MCS_TAG_DOMAIN_PARAMS, 32);
	ber_out_integer(s, max_channels);
	ber_out_integer(s, max_users);
	ber_out_integer(s, max_tokens);
	ber_out_integer(s, 1);	/* num_priorities */
	ber_out_integer(s, 0);	/* min_throughput */
	ber_out_integer(s, 1);	/* max_height */
	ber_out_integer(s, max_pdusize);
	ber_out_integer(s, 2);	/* ver_protocol */
}

/* Parse a DOMAIN_PARAMS structure (ASN.1 BER) */
static RD_BOOL
mcs_parse_domain_params(rdpMcs * mcs, STREAM s)
{
	int length;

	ber_parse_header(mcs, s, MCS_TAG_DOMAIN_PARAMS, &length);
	in_uint8s(s, length);

	return s_check(s);
}

/* Send an MCS_CONNECT_INITIAL message (ASN.1 BER) */
static void
mcs_send_connect_initial(rdpMcs * mcs, STREAM connectdata)
{
	int datalen = connectdata->end - connectdata->data;
	int length = 9 + 3 * 34 + 4 + datalen;
	STREAM s;

	s = iso_init(mcs->iso, length + 5);

	ber_out_header(s, MCS_CONNECT_INITIAL, length);	/* ConnectMCSPDU connect-initial */
	ber_out_header(s, BER_TAG_OCTET_STRING, 1);	/* callingDomainSelector */
	out_uint8(s, 1);
	ber_out_header(s, BER_TAG_OCTET_STRING, 1);	/* calledDomainSelector */
	out_uint8(s, 1);

	ber_out_header(s, BER_TAG_BOOLEAN, 1);	/* upwardFlag */
	out_uint8(s, 0xff);

	mcs_out_domain_params(s, 34, 2, 0, 0xFFFF);	/* targetParameters */
	mcs_out_domain_params(s, 1, 1, 1, 0x420);	/* minimumParameters */
	mcs_out_domain_params(s, 0xFFFF, 0xFC17, 0xFFFF, 0xFFFF);	/* maximumParameters */

	ber_out_header(s, BER_TAG_OCTET_STRING, datalen);	/* userData */
	out_uint8p(s, connectdata->data, datalen);

	s_mark_end(s);
	iso_send(mcs->iso, s);
}

/* Expect a MCS_CONNECT_RESPONSE message (ASN.1 BER) */
static RD_BOOL
mcs_recv_connect_response(rdpMcs * mcs)
{
	uint8 result;
	int length;
	STREAM s;

	s = iso_recv(mcs->iso, NULL);
	if (s == NULL)
		return False;

	ber_parse_header(mcs, s, MCS_CONNECT_RESPONSE, &length);

	ber_parse_header(mcs, s, BER_TAG_RESULT, &length);
	in_uint8(s, result);
	if (result != 0)
	{
		ui_error(mcs->sec->rdp->inst, "MCS connect: %d\n", result);
		return False;
	}

	ber_parse_header(mcs, s, BER_TAG_INTEGER, &length);
	in_uint8s(s, length);	/* connect id */
	mcs_parse_domain_params(mcs, s);

	ber_parse_header(mcs, s, BER_TAG_OCTET_STRING, &length);

	sec_process_mcs_data(mcs->sec, s);
	return s_check_end(s);
}

/* Send an EDrq message (ASN.1 PER) */
static void
mcs_send_edrq(rdpMcs * mcs)
{
	STREAM s;

	s = iso_init(mcs->iso, 5);

	out_uint8(s, (T125_DOMAINMCSPDU_ErectDomainRequest << 2));
	out_uint16_be(s, 1);	/* height */
	out_uint16_be(s, 1);	/* interval */

	s_mark_end(s);
	iso_send(mcs->iso, s);
}

/* Send an AUrq message (ASN.1 PER) */
static void
mcs_send_aurq(rdpMcs * mcs)
{
	STREAM s;

	s = iso_init(mcs->iso, 1);

	out_uint8(s, (T125_DOMAINMCSPDU_AttachUserRequest << 2));

	s_mark_end(s);
	iso_send(mcs->iso, s);
}

/* Expect a AUcf message (ASN.1 PER) */
static RD_BOOL
mcs_recv_aucf(rdpMcs * mcs, uint16 * mcs_userid)
{
	uint8 opcode, result;
	STREAM s;

	s = iso_recv(mcs->iso, NULL);
	if (s == NULL)
		return False;

	in_uint8(s, opcode);
	if ((opcode >> 2) != T125_DOMAINMCSPDU_AttachUserConfirm)
	{
		ui_error(mcs->sec->rdp->inst, "expected AUcf, got %d\n", opcode);
		return False;
	}

	in_uint8(s, result);
	if (result != 0)
	{
		ui_error(mcs->sec->rdp->inst, "AUrq: %d\n", result);
		return False;
	}

	if (opcode & 2)
		in_uint16_be(s, *mcs_userid);

	return s_check_end(s);
}

/* Send a CJrq message (ASN.1 PER) */
static void
mcs_send_cjrq(rdpMcs * mcs, uint16 chanid)
{
	STREAM s;

	DEBUG_RDP5("Sending CJRQ for channel #%d\n", chanid);

	s = iso_init(mcs->iso, 5);

	out_uint8(s, (T125_DOMAINMCSPDU_ChannelJoinRequest << 2));
	out_uint16_be(s, mcs->mcs_userid);
	out_uint16_be(s, chanid);

	s_mark_end(s);
	iso_send(mcs->iso, s);
}

/* Expect a CJcf message (ASN.1 PER) */
static RD_BOOL
mcs_recv_cjcf(rdpMcs * mcs)
{
	uint8 opcode, result;
	STREAM s;

	s = iso_recv(mcs->iso, NULL);
	if (s == NULL)
		return False;

	in_uint8(s, opcode);
	if ((opcode >> 2) != T125_DOMAINMCSPDU_ChannelJoinConfirm)
	{
		ui_error(mcs->sec->rdp->inst, "expected CJcf, got %d\n", opcode);
		return False;
	}

	in_uint8(s, result);
	if (result != 0)
	{
		ui_error(mcs->sec->rdp->inst, "CJrq: %d\n", result);
		return False;
	}

	in_uint8s(s, 4);	/* mcs_userid, req_chanid */
	if (opcode & 2)
		in_uint8s(s, 2);	/* join_chanid */

	return s_check_end(s);
}

/* Initialise an MCS transport data packet */
STREAM
mcs_init(rdpMcs * mcs, int length)
{
	STREAM s;

	s = iso_init(mcs->iso, length + 8);
	s_push_layer(s, mcs_hdr, 8);

	return s;
}

/* Initialise a fast path data packet */
STREAM
mcs_fp_init(rdpMcs * mcs, int length)
{
	STREAM s;

	s = iso_fp_init(mcs->iso, length);
	return s;
}

/* Send an MCS transport data packet to a specific channel */
void
mcs_send_to_channel(rdpMcs * mcs, STREAM s, uint16 channel)
{
	uint16 length;

	s_pop_layer(s, mcs_hdr);
	length = s->end - s->p - 8;
	length |= 0x8000;

	out_uint8(s, (T125_DOMAINMCSPDU_SendDataRequest << 2));
	out_uint16_be(s, mcs->mcs_userid);
	out_uint16_be(s, channel);
	out_uint8(s, 0x70);	/* flags */
	out_uint16_be(s, length);

	iso_send(mcs->iso, s);
}

/* Send an MCS transport data packet to the global channel */
void
mcs_send(rdpMcs * mcs, STREAM s)
{
	mcs_send_to_channel(mcs, s, MCS_GLOBAL_CHANNEL);
}

/* Send a fast path data packet to the global channel */
void
mcs_fp_send(rdpMcs * mcs, STREAM s, uint32 flags)
{
	iso_fp_send(mcs->iso, s, flags);
}

/* Receive an MCS transport data packet */
STREAM
mcs_recv(rdpMcs * mcs, uint16 * channel, isoRecvType * ptype)
{
	uint8 opcode, pdu_type, length;
	STREAM s;

	s = iso_recv(mcs->iso, ptype);
	if (s == NULL)
		return NULL;
	if (*ptype == ISO_RECV_X224)
	{
		/* Parse mcsSDin (MCS Send Data Indication PDU, see [T125] section 7, part 7): */
		in_uint8(s, opcode);
		pdu_type = opcode >> 2;
		if (pdu_type != T125_DOMAINMCSPDU_SendDataIndication)
		{
			if (pdu_type != T125_DOMAINMCSPDU_DisconnectProviderUltimatum)
			{
				ui_error(mcs->sec->rdp->inst, "expected data, got %d\n", opcode);
			}
			return NULL;
		}
		in_uint8s(s, 2);	/* initiator */
		in_uint16_be(s, *channel);
		in_uint8s(s, 1);	/* dataPriority and segmentation flags */
		in_uint8(s, length);	/* length of userData in 1 or two bytes */
		if (length & 0x80)
			in_uint8s(s, 1);	/* second byte of length */
	}
	return s;
}

/* Establish a connection up to the MCS layer */
RD_BOOL
mcs_connect(rdpMcs * mcs, STREAM connectdata)
{
	int i;
	int mcs_id;
	rdpSet * settings;

	mcs_send_connect_initial(mcs, connectdata);
	if (!mcs_recv_connect_response(mcs))
		goto error;

	mcs_send_edrq(mcs);

	mcs_send_aurq(mcs);
	if (!mcs_recv_aucf(mcs, &(mcs->mcs_userid)))
		goto error;

	mcs_send_cjrq(mcs, mcs->mcs_userid + MCS_USERCHANNEL_BASE);

	if (!mcs_recv_cjcf(mcs))
		goto error;

	mcs_send_cjrq(mcs, MCS_GLOBAL_CHANNEL);
	if (!mcs_recv_cjcf(mcs))
		goto error;

	settings = mcs->sec->rdp->settings;
	for (i = 0; i < settings->num_channels; i++)
	{
		mcs_id = settings->channels[i].chan_id;
		if (mcs_id >= mcs->mcs_userid + MCS_USERCHANNEL_BASE)
			goto error;
		mcs_send_cjrq(mcs, mcs_id);
		if (!mcs_recv_cjcf(mcs))
			goto error;
	}
	return True;

      error:
	iso_disconnect(mcs->iso);
	return False;
}

/* Establish a connection up to the MCS layer */
RD_BOOL
mcs_reconnect(rdpMcs * mcs, STREAM connectdata)
{
	int i;
	int mcs_id;
	rdpSet * settings;

	mcs_send_connect_initial(mcs, connectdata);
	if (!mcs_recv_connect_response(mcs))
		goto error;

	mcs_send_edrq(mcs);

	mcs_send_aurq(mcs);
	if (!mcs_recv_aucf(mcs, &(mcs->mcs_userid)))
		goto error;

	mcs_send_cjrq(mcs, mcs->mcs_userid + MCS_USERCHANNEL_BASE);

	if (!mcs_recv_cjcf(mcs))
		goto error;

	mcs_send_cjrq(mcs, MCS_GLOBAL_CHANNEL);
	if (!mcs_recv_cjcf(mcs))
		goto error;

	settings = mcs->sec->rdp->settings;
	for (i = 0; i < settings->num_channels; i++)
	{
		mcs_id = settings->channels[i].chan_id;
		if (mcs_id >= mcs->mcs_userid + MCS_USERCHANNEL_BASE)
			goto error;
		mcs_send_cjrq(mcs, mcs_id);
		if (!mcs_recv_cjcf(mcs))
			goto error;
	}
	return True;

      error:
	iso_disconnect(mcs->iso);
	return False;
}

/* Disconnect from the MCS layer */
void
mcs_disconnect(rdpMcs * mcs)
{
	iso_disconnect(mcs->iso);
}

rdpMcs *
mcs_new(struct rdp_sec * sec)
{
	rdpMcs * self;

	self = (rdpMcs *) xmalloc(sizeof(rdpMcs));
	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpMcs));
		self->sec = sec;
		self->iso = iso_new(self);
		self->chan = vchan_new(self);
	}
	return self;
}

void
mcs_free(rdpMcs * mcs)
{
	if (mcs != NULL)
	{
		vchan_free(mcs->chan);
		iso_free(mcs->iso);
		xfree(mcs);
	}
}
