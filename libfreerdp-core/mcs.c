/*
   FreeRDP: A Remote Desktop Protocol client.
   Multipoint Communications Service

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

#include "frdp.h"
#include "iso.h"
#include "chan.h"
#include "rdp.h"
#include "asn1.h"
#include "tcp.h"
#include "connect.h"
#include "security.h"
#include <freerdp/rdpset.h>
#include <freerdp/utils/memory.h>

#include "mcs.h"

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
mcs_send_connect_initial(rdpMcs * mcs)
{
	STREAM s;
	int length;
	int gccCCrq_length;
	struct stream gccCCrq;

	gccCCrq.size = 512;
	gccCCrq.p = gccCCrq.data = (uint8 *) xmalloc(gccCCrq.size);
	gccCCrq.end = gccCCrq.data + gccCCrq.size;

	connect_output_gcc_conference_create_request(mcs->net->sec, &gccCCrq);
	gccCCrq_length = gccCCrq.end - gccCCrq.data;
	length = 9 + 3 * 34 + 4 + gccCCrq_length;

	s = iso_init(mcs->iso, length + 5);

	ber_out_header(s, MCS_CONNECT_INITIAL, length);	/* ConnectMCSPDU connect-initial */
	ber_out_header(s, BER_TAG_OCTET_STRING, 1);	/* callingDomainSelector */
	out_uint8(s, 1);
	ber_out_header(s, BER_TAG_OCTET_STRING, 1);	/* calledDomainSelector */
	out_uint8(s, 1);

	ber_out_header(s, BER_TAG_BOOLEAN, 1);	/* upwardFlag */
	out_uint8(s, 0xFF);

	mcs_out_domain_params(s, 34, 2, 0, 0xFFFF);	/* targetParameters */
	mcs_out_domain_params(s, 1, 1, 1, 0x420);	/* minimumParameters */
	mcs_out_domain_params(s, 0xFFFF, 0xFC17, 0xFFFF, 0xFFFF);	/* maximumParameters */

	ber_out_header(s, BER_TAG_OCTET_STRING, gccCCrq_length);	/* userData */
	out_uint8p(s, gccCCrq.data, gccCCrq_length);
	xfree(gccCCrq.data);

	s_mark_end(s);
	iso_send(mcs->iso, s);
}

/* Expect a MCS_CONNECT_RESPONSE message (ASN.1 BER) */
static RD_BOOL
mcs_recv_connect_response(rdpMcs * mcs)
{
	STREAM s;
	int length;
	uint8 result;

	s = iso_recv(mcs->iso, NULL);

	if (s == NULL)
		return False;

	ber_parse_header(mcs, s, MCS_CONNECT_RESPONSE, &length);

	ber_parse_header(mcs, s, BER_TAG_RESULT, &length);
	in_uint8(s, result);
	if (result != 0)
	{
		ui_error(mcs->net->rdp->inst, "MCS connect: %d\n", result);
		return False;
	}

	ber_parse_header(mcs, s, BER_TAG_INTEGER, &length);
	in_uint8s(s, length);	/* connect id */
	mcs_parse_domain_params(mcs, s);

	ber_parse_header(mcs, s, BER_TAG_OCTET_STRING, &length);

	connect_process_mcs_data(mcs->net->sec, s);
	return s_check_end(s);
}

/* Send an EDrq message (ASN.1 PER) */
static void
mcs_send_edrq(rdpMcs * mcs)
{
	STREAM s;

	s = iso_init(mcs->iso, 5);

	out_uint8(s, (T125_DOMAINMCSPDU_ErectDomainRequest << 2));
	out_uint16_le(s, 1);	/* height */
	out_uint16_le(s, 1);	/* interval */

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
		ui_error(mcs->net->rdp->inst, "expected AUcf, got %d\n", opcode);
		return False;
	}

	in_uint8(s, result);
	if (result != 0)
	{
		ui_error(mcs->net->rdp->inst, "AUrq: %d\n", result);
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

	DEBUG_MCS("Sending CJRQ for channel #%d", chanid);

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
		ui_error(mcs->net->rdp->inst, "expected CJcf, got %d\n", opcode);
		return False;
	}

	in_uint8(s, result);
	if (result != 0)
	{
		ui_error(mcs->net->rdp->inst, "CJrq: %d\n", result);
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
mcs_recv(rdpMcs * mcs, isoRecvType * ptype, uint16 * channel)
{
	STREAM s;
	uint8 byte;
	uint8 pduType;
	uint16 length;

	*channel = (uint16)-1;	/* default: bogus value */
	s = iso_recv(mcs->iso, ptype);

	if (s == NULL)
		return NULL;

	if (*ptype == ISO_RECV_X224)
	{
		/* Parse mcsSDin (MCS Send Data Indication PDU, see [T125] section 7, part 7): */

		in_uint8(s, pduType);
		pduType >>= 2;

		if (pduType != T125_DOMAINMCSPDU_SendDataIndication)
		{
			if (pduType != T125_DOMAINMCSPDU_DisconnectProviderUltimatum)
			{
				ui_error(mcs->net->rdp->inst, "expected data, got %d\n", pduType);
			}
			return NULL;
		}

		in_uint8s(s, 2);		/* initiator */
		in_uint16_be(s, *channel);	/* channelId */
		in_uint8s(s, 1);		/* flags */

		in_uint8(s, byte);
		length = (uint16) byte;

		if (byte & 0x80)
		{
			length &= ~0x80;
			length <<= 8;
			in_uint8(s, byte);
			length += (uint16) byte;
		}
	}

	return s;
}

/* Establish a connection up to the MCS layer */
RD_BOOL
mcs_connect(rdpMcs * mcs)
{
	int i;
	int mcs_id;
	rdpSet * settings;

	mcs_send_connect_initial(mcs);
	if (!mcs_recv_connect_response(mcs))
	{
		ui_error(mcs->net->rdp->inst, "invalid mcs_recv_connect_response\n");
		goto error;
	}

	mcs_send_edrq(mcs);

	mcs_send_aurq(mcs);
	if (!mcs_recv_aucf(mcs, &(mcs->mcs_userid)))
	{
		ui_error(mcs->net->rdp->inst, "invalid mcs_recv_aucf\n");
		goto error;
	}

	mcs_send_cjrq(mcs, mcs->mcs_userid + MCS_USERCHANNEL_BASE);

	if (!mcs_recv_cjcf(mcs))
	{
		ui_error(mcs->net->rdp->inst, "invalid mcs_recv_cjcf\n");
		goto error;
	}

	mcs_send_cjrq(mcs, MCS_GLOBAL_CHANNEL);
	if (!mcs_recv_cjcf(mcs))
	{
		ui_error(mcs->net->rdp->inst, "invalid mcs_recv_cjcf\n");
		goto error;
	}

	settings = mcs->net->rdp->settings;
	for (i = 0; i < settings->num_channels; i++)
	{
		mcs_id = settings->channels[i].chan_id;
		if (mcs_id >= mcs->mcs_userid + MCS_USERCHANNEL_BASE)
		{
			ui_warning(mcs->net->rdp->inst, "channel %d got id %d >= %d\n", i, mcs_id, mcs->mcs_userid + MCS_USERCHANNEL_BASE);
		}
		mcs_send_cjrq(mcs, mcs_id);
		if (!mcs_recv_cjcf(mcs))
		{
			ui_error(mcs->net->rdp->inst, "channel %d id %d invalid mcs_recv_cjcf\n", i, mcs_id);
			goto error;
		}
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
mcs_new(struct rdp_network * net)
{
	rdpMcs * self;

	self = (rdpMcs *) xmalloc(sizeof(rdpMcs));

	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpMcs));
		self->net = net;
		self->iso = iso_new(net);
		self->chan = vchan_new(self);
		self->net->iso = self->iso;
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
