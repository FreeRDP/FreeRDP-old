/*
   FreeRDP: A Remote Desktop Protocol client.
   Connection Sequence

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

#include "rdp.h"
#include "nego.h"
#include "security.h"
#include <freerdp/rdpset.h>
#include <freerdp/freerdp.h>
#include <freerdp/utils/memory.h>
#include <freerdp/utils/unicode.h>

#include "connect.h"

static void
connect_output_client_core_data(rdpSec * sec, rdpSet * settings, STREAM s)
{
	char * p;
	size_t len;
	int con_type;
	uint16 highColorDepth;
	uint16 supportedColorDepths;
	uint16 earlyCapabilityFlags;

	out_uint16_le(s, UDH_CS_CORE);	/* User Data Header type */
	out_uint16_le(s, 216);		/* total length */

	out_uint32_le(s, settings->rdp_version >= 5 ? 0x00080004 : 0x00080001);	/* client version */
	out_uint16_le(s, settings->width);		/* desktopWidth */
	out_uint16_le(s, settings->height);		/* desktopHeight */
	out_uint16_le(s, RNS_UD_COLOR_8BPP);		/* colorDepth, ignored because of postBeta2ColorDepth */
	out_uint16_le(s, RNS_UD_SAS_DEL);		/* SASSequence (Secure Access Sequence) */
	out_uint32_le(s, settings->keyboard_layout);	/* keyboardLayout */
	out_uint32_le(s, 2600);				/* clientBuild */

	/* Unicode name of client, truncated to 15 characters */
	p = freerdp_uniconv_out(sec->rdp->uniconv, settings->hostname, &len);
	if (len > 30)
	{
		len = 30;
		p[len] = 0;
		p[len + 1] = 0;
	}
	out_uint8a(s, p, len + 2);
	out_uint8s(s, 32 - len - 2);
	xfree(p);

	out_uint32_le(s, settings->keyboard_type);		/* keyboardType */
	out_uint32_le(s, settings->keyboard_subtype);		/* keyboardSubType */
	out_uint32_le(s, settings->keyboard_functionkeys);	/* keyboardFunctionKey */

	/* Input Method Editor (IME) file name associated with the input locale.
	   Up to 31 Unicode characters plus a NULL terminator */
	/* FIXME: populate this field with real data */
	out_uint8s(s, 64);	/* imeFileName */

	out_uint16_le(s, RNS_UD_COLOR_8BPP); /* postBeta2ColorDepth */
	out_uint16_le(s, 1); /* clientProductID */
	out_uint32_le(s, 0); /* serialNumber (should be initialized to 0) */

	highColorDepth = MIN(settings->server_depth, 24);	/* 32 must be reported as 24 and RNS_UD_CS_WANT_32BPP_SESSION */
	out_uint16_le(s, highColorDepth); /* (requested) highColorDepth */

	supportedColorDepths = RNS_UD_32BPP_SUPPORT | RNS_UD_24BPP_SUPPORT | RNS_UD_16BPP_SUPPORT | RNS_UD_15BPP_SUPPORT;
	out_uint16_le(s, supportedColorDepths); /* supportedColorDepths */

	con_type = 0;
	earlyCapabilityFlags = RNS_UD_CS_SUPPORT_ERRINFO_PDU;
	if (sec->rdp->settings->performanceflags == PERF_FLAG_NONE)
	{
		earlyCapabilityFlags |= RNS_UD_CS_VALID_CONNECTION_TYPE;
		con_type = CONNECTION_TYPE_LAN;
	}
	if (settings->server_depth == 32)
		earlyCapabilityFlags |= RNS_UD_CS_WANT_32BPP_SESSION;

	out_uint16_le(s, earlyCapabilityFlags); /* earlyCapabilityFlags */
	out_uint8s(s, 64); /* clientDigProductId (64 bytes) */
	/* connectionType, only valid when RNS_UD_CS_VALID_CONNECTION_TYPE
		is set in earlyCapabilityFlags */
	out_uint8(s, con_type);
	out_uint8(s, 0); /* pad1octet */
	out_uint32_le(s, sec->net->nego->selected_protocol); /* serverSelectedProtocol */
}

static void
connect_output_client_security_data(rdpSec * sec, rdpSet * settings, STREAM s)
{
	uint16 encryptionMethods = 0;

	out_uint16_le(s, UDH_CS_SECURITY);	/* User Data Header type */
	out_uint16_le(s, 12);			/* total length */

	if (settings->encryption || sec->net->tls_connected)
		encryptionMethods = ENCRYPTION_40BIT_FLAG | ENCRYPTION_128BIT_FLAG;

	out_uint32_le(s, encryptionMethods);	/* encryptionMethods */
	out_uint32_le(s, 0);			/* extEncryptionMethods */
}

static void
connect_output_client_network_data(rdpSec * sec, rdpSet * settings, STREAM s)
{
	int i;

	DEBUG_SEC("num_channels is %d", settings->num_channels);
	if (settings->num_channels > 0)
	{
		out_uint16_le(s, UDH_CS_NET);	/* User Data Header type */
		out_uint16_le(s, settings->num_channels * 12 + 8);	/* total length */

		out_uint32_le(s, settings->num_channels);	/* channelCount */
		for (i = 0; i < settings->num_channels; i++)
		{
			DEBUG_SEC("Requesting channel %s", settings->channels[i].name);
			out_uint8a(s, settings->channels[i].name, 8); /* name (8 bytes) 7 characters with null terminator */
			out_uint32_le(s, settings->channels[i].flags); /* options (4 bytes) */
		}
	}
}

static void
connect_output_client_cluster_data(rdpSec * sec, rdpSet * settings, STREAM s)
{
	out_uint16_le(s, UDH_CS_CLUSTER);	/* User Data Header type */
	out_uint16_le(s, 12);			/* total length */

	out_uint32_le(s, (settings->console_session || sec->rdp->redirect_session_id) ?
		REDIRECTED_SESSIONID_FIELD_VALID | REDIRECTION_SUPPORTED | REDIRECTION_VERSION4 :
		REDIRECTION_SUPPORTED | REDIRECTION_VERSION4); /* flags */

	out_uint32_le(s, sec->rdp->redirect_session_id); /* RedirectedSessionID */
}

static void
connect_output_client_monitor_data(rdpSec * sec, rdpSet * settings, STREAM s)
{
	int length, n;
	DEBUG_SEC("Setting monitor data... num_monitors: %d", settings->num_monitors);
	if (settings->num_monitors <= 1)
		return;

	DEBUG_SEC("Setting monitor data...");
	out_uint16_le(s, UDH_CS_MONITOR);	/* User Data Header type */

	length = 12 + (20 * settings->num_monitors);
	out_uint16_le(s, length);
	out_uint32_le(s, 0); /* flags (unused) */
	out_uint32_le(s, settings->num_monitors); /* monitorCount */
	for (n = 0; n < settings->num_monitors; n++)
	{
		out_uint32_le(s, settings->monitors[n].x); /* left */
		out_uint32_le(s, settings->monitors[n].y); /* top */
		out_uint32_le(s, settings->monitors[n].x +
						 settings->monitors[n].width-1); /* right */
		out_uint32_le(s, settings->monitors[n].y +
						 settings->monitors[n].height-1); /* bottom */
		out_uint32_le(s, settings->monitors[n].is_primary ? 1 : 0); /* isPrimary */
	}
}

void
connect_output_gcc_conference_create_request(rdpSec * sec, STREAM s)
{
	int length;
	rdpSet * settings = sec->rdp->settings;

	/* See ITU-T Rec. T.124 (02/98) Generic Conference Control */

	/* the part before userData is of a fixed size, making things convenient */
	s->p = s->data + 23;
	connect_output_client_core_data(sec, settings, s);
	connect_output_client_cluster_data(sec, settings, s);
	connect_output_client_security_data(sec, settings, s);
	connect_output_client_network_data(sec, settings, s);
	connect_output_client_monitor_data(sec, settings, s);
	length = (s->p - s->data) - 23;
	s->p = s->data;

	/* t124Identifier = 0.0.20.124.0.1 */
	out_uint16_be(s, 5);
	out_uint16_be(s, 0x14);
	out_uint8(s, 0x7c);
	out_uint16_be(s, 1);

	/* connectPDU octet string */
	out_uint16_be(s, ((length + 14) | 0x8000));		/* connectPDU length in two bytes*/

	/* connectPDU content is ConnectGCCPDU PER encoded: */
	out_uint16_be(s, 8);				/* ConferenceCreateRequest ... */
	out_uint16_be(s, 16);
	out_uint8(s, 0);
	out_uint16_le(s, 0xC001);			/* userData key is h221NonStandard */
	out_uint8(s, 0);				/* 4 bytes: */
	out_uint32_le(s, 0x61637544);			/* "Duca" */
	out_uint16_be(s, (length | 0x8000));		/* userData value length in two bytes */
	s->p = s->data + length + 23;			/* userData (outputted earlier) */

	s_mark_end(s);
}

/* Parse Server Security Data */
static RD_BOOL
connect_process_server_security_data(rdpSec * sec, STREAM s, uint32 * encryptionMethod, uint8 server_random[SEC_RANDOM_SIZE], uint8 * modulus, uint8 * exponent)
{
	uint32 encryptionLevel;
	uint32 serverRandomLen;
	uint32 serverCertLen;
	uint32 certChainVersion;
	uint32 dwVersion;

	/**
	 * encryptionMethod:
	 *
	 * ENCRYPTION_METHOD_NONE	0
	 * ENCRYPTION_METHOD_40BIT	1
	 * ENCRYPTION_METHOD_128BIT	2
	 * ENCRYPTION_METHOD_56BIT	8
	 * ENCRYPTION_METHOD_FIPS	16
	 *
	 */
	in_uint32_le(s, *encryptionMethod); /* encryptionMethod, 0 for TLS/NLA */

	/**
	 * encryptionLevel:
	 *
	 * ENCRYPTION_LEVEL_NONE		0
	 * ENCRYPTION_LEVEL_LOW			1
	 * ENCRYPTION_LEVEL_CLIENT_COMPATIBLE	2
	 * ENCRYPTION_LEVEL_HIGH		3
	 * ENCRYPTION_LEVEL_FIPS		4
	 *
	 */
	in_uint32_le(s, encryptionLevel); /* encryptionLevel, 0 for TLS/NLA */

	if (encryptionLevel == 0) /* no encryption */
		return False;

	in_uint32_le(s, serverRandomLen); /* serverRandomLen */
	in_uint32_le(s, serverCertLen); /* serverCertLen */

	if (serverRandomLen != SEC_RANDOM_SIZE)
	{
		ui_error(sec->rdp->inst, "serverRandomLen %d, expected %d\n", serverRandomLen, SEC_RANDOM_SIZE);
		return False;
	}

	in_uint8a(s, server_random, SEC_RANDOM_SIZE); /* serverRandom */

	/* Server Certificate: */
	in_uint32_le(s, dwVersion); /* bit 0x80000000 = temporary certificate */
	certChainVersion = dwVersion & 0x7FFFFFFF;

	if (certChainVersion == 1)	 /* Server Proprietary Certificate */
	{
		sec_parse_cert_chain_v1(sec, s, modulus, exponent);
	}
	else if (certChainVersion == 2)	 /* X.509 */
	{
		sec_parse_cert_chain_v2(sec, s, modulus, exponent);
	}
	else
	{
		ui_error(sec->rdp->inst, "invalid cert chain version: %d\n", certChainVersion);
		return False;
	}

	return s_check_end(s);
}

/* Process Server Security Data */
static void
connect_input_server_security_data(rdpSec * sec, STREAM s)
{
	uint32 rc4_key_size;
	uint8 server_random[SEC_RANDOM_SIZE];
	uint8 client_random[SEC_RANDOM_SIZE];
	uint8 modulus[SEC_MAX_MODULUS_SIZE];
	uint8 exponent[SEC_EXPONENT_SIZE];
	uint8 client_random_rev[SEC_RANDOM_SIZE];
	uint8 crypted_random_rev[SEC_MAX_MODULUS_SIZE];

	memset(modulus, 0, sizeof(modulus));
	memset(exponent, 0, sizeof(exponent));
	if (!connect_process_server_security_data(sec, s, &rc4_key_size, server_random, modulus, exponent))
	{
		/* encryptionMethod (rc4_key_size) = 0 means TLS */
		if (rc4_key_size > 0)
		{
			DEBUG_SEC("Failed to parse crypt info");
		}
		return;
	}

	DEBUG_SEC("Generating client random");
	generate_random(client_random);
	sec_reverse_copy(client_random_rev, client_random, SEC_RANDOM_SIZE);
	crypto_rsa_encrypt(SEC_RANDOM_SIZE, client_random_rev, crypted_random_rev,
			sec->server_public_key_len, modulus, exponent);
	sec_reverse_copy(sec->sec_crypted_random, crypted_random_rev, sec->server_public_key_len);
	sec_generate_keys(sec, client_random, server_random, rc4_key_size);
}

/* Process Server Core Data */
static void
connect_input_server_core_data(rdpSec * sec, STREAM s, uint16 length)
{
	uint32 server_rdp_version, clientRequestedProtocols;
	in_uint32_le(s, server_rdp_version);

	if(server_rdp_version == 0x00080001)
	{
		sec->rdp->settings->rdp_version = 4;
		sec->rdp->settings->server_depth = 8;
	}
	else if(server_rdp_version == 0x00080004)
	{
		sec->rdp->settings->rdp_version = 5;	/* FIXME: We can't just upgrade the RDP version! */
	}
	else
	{
		ui_error(sec->rdp->inst, "Invalid server rdp version %ul\n", server_rdp_version);
	}

	DEBUG_SEC("Server RDP version is %d", sec->rdp->settings->rdp_version);
	if (length >= 12)
	{
		in_uint32_le(s, clientRequestedProtocols);
	}
}

/* Process Server Network Data */
static void
connect_input_server_network_data(rdpSec * sec, STREAM s)
{
	int i;
	uint16 MCSChannelId;
	uint16 channelCount;

	in_uint16_le(s, MCSChannelId); /* MCSChannelId */
	if (MCSChannelId != MCS_GLOBAL_CHANNEL)
		ui_error(sec->rdp->inst, "expected IO channel 0x%x=%d but got 0x%x=%d\n",
				MCS_GLOBAL_CHANNEL, MCS_GLOBAL_CHANNEL, MCSChannelId, MCSChannelId);
	in_uint16_le(s, channelCount); /* channelCount */

	/* TODO: Check that it matches rdp->settings->num_channels */
	if (channelCount != sec->rdp->settings->num_channels)
	{
		ui_error(sec->rdp->inst, "client requested %d channels, server replied with %d channels",
		         sec->rdp->settings->num_channels, channelCount);
	}

	/* channelIdArray */
	for (i = 0; i < channelCount; i++)
	{
		uint16 channelId;
		in_uint16_le(s, channelId);	/* Channel ID allocated to requested channel number i */

		/* TODO: Assign channel ids here instead of in freerdp.c l_rdp_connect */
		if (channelId != sec->rdp->settings->channels[i].chan_id)
		{
			ui_error(sec->rdp->inst, "channel %d is %d but should have been %d\n",
			         i, channelId, sec->rdp->settings->channels[i].chan_id);
		}
	}

	if (channelCount % 2 == 1)
		in_uint8s(s, 2);	/* Padding */
}

/* Process connect response data blob */
void
connect_process_mcs_data(rdpSec * sec, STREAM s)
{
	uint8 byte;
	uint16 type;
	uint16 length;
	uint16 totalLength;
	uint8 *next_tag;

	in_uint8s(s, 21);	/* TODO: T.124 ConferenceCreateResponse userData with key h221NonStandard McDn */

	in_uint8(s, byte);
	totalLength = (uint16) byte;

	if (byte & 0x80)
	{
		totalLength &= ~0x80;
		totalLength <<= 8;
		in_uint8(s, byte);
		totalLength += (uint16) byte;
	}

	while (s->p < s->end)
	{
		in_uint16_le(s, type);
		in_uint16_le(s, length);

		if (length <= 4)
			return;

		next_tag = s->p + length - 4;

		switch (type)
		{
			case UDH_SC_CORE: /* Server Core Data */
				connect_input_server_core_data(sec, s, length);
				break;

			case UDH_SC_NET: /* Server Network Data */
				connect_input_server_network_data(sec, s);
				break;

			case UDH_SC_SECURITY: /* Server Security Data */
				connect_input_server_security_data(sec, s);
				break;
		}

		s->p = next_tag;
	}
}


