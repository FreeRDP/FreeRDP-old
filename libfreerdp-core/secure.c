/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP encryption and licensing

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
#include "mcs.h"
#include "chan.h"
#include "license.h"
#include "rdp.h"
#include "iso.h"
#include "tcp.h"
#include <freerdp/rdpset.h>
#include <freerdp/utils/memory.h>

#ifndef DISABLE_TLS
#include "tls.h"
#include "credssp.h"
#endif

#include "secure.h"

static RD_BOOL sec_global_initialized = False;

RD_BOOL
sec_global_init(void)
{
	if (!sec_global_initialized)
	{
		sec_global_initialized = crypto_global_init();
	}
	return sec_global_initialized;
}

void
sec_global_finish(void)
{
	crypto_global_finish();
	sec_global_initialized = False;
}

/* these are read only */
static uint8 pad_54[40] = {
	54, 54, 54, 54, 54, 54, 54, 54,
	54, 54, 54, 54, 54, 54, 54, 54,
	54, 54, 54, 54, 54, 54, 54, 54,
	54, 54, 54, 54, 54, 54, 54, 54,
	54, 54, 54, 54, 54, 54, 54, 54
};

static uint8 pad_92[48] = {
	92, 92, 92, 92, 92, 92, 92, 92,
	92, 92, 92, 92, 92, 92, 92, 92,
	92, 92, 92, 92, 92, 92, 92, 92,
	92, 92, 92, 92, 92, 92, 92, 92,
	92, 92, 92, 92, 92, 92, 92, 92,
	92, 92, 92, 92, 92, 92, 92, 92
};

/*
 * 48-byte transformation used to generate master secret (6.1) and key material (6.2.2).
 * Both SHA1 and MD5 algorithms are used.
 */
void
sec_hash_48(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2, uint8 salt)
{
	int i;
	uint8 pad[4];
	uint8 shasig[20];
	CryptoMd5 md5;
	CryptoSha1 sha1;

	for (i = 0; i < 3; i++)
	{
		memset(pad, salt + i, i + 1);

		sha1 = crypto_sha1_init();
		crypto_sha1_update(sha1, pad, i + 1);
		crypto_sha1_update(sha1, in, 48);
		crypto_sha1_update(sha1, salt1, 32);
		crypto_sha1_update(sha1, salt2, 32);
		crypto_sha1_final(sha1, shasig);

		md5 = crypto_md5_init();
		crypto_md5_update(md5, in, 48);
		crypto_md5_update(md5, shasig, 20);
		crypto_md5_final(md5, &out[i * 16]);
	}
}

/*
 * 16-byte transformation used to generate export keys (6.2.2).
 */
void
sec_hash_16(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2)
{
	CryptoMd5 md5 = crypto_md5_init();
	crypto_md5_update(md5, in, 16);
	crypto_md5_update(md5, salt1, 32);
	crypto_md5_update(md5, salt2, 32);
	crypto_md5_final(md5, out);
}

/* Reduce key entropy from 64 to 40 bits */
static void
sec_make_40bit(uint8 * key)
{
	key[0] = 0xd1;
	key[1] = 0x26;
	key[2] = 0x9e;
}

/* Generate encryption keys given client and server randoms */
static void
sec_generate_keys(rdpSec * sec, uint8 * client_random, uint8 * server_random, int rc4_key_size)
{
	uint8 pre_master_secret[48];
	uint8 master_secret[48];
	uint8 key_block[48];

	/* Construct pre-master secret */
	memcpy(pre_master_secret, client_random, 24);
	memcpy(pre_master_secret + 24, server_random, 24);

	/* Generate master secret and then key material */
	sec_hash_48(master_secret, pre_master_secret, client_random, server_random, 'A');
	sec_hash_48(key_block, master_secret, client_random, server_random, 'X');

	/* First 16 bytes of key material is MAC secret */
	memcpy(sec->sec_sign_key, key_block, 16);

	/* Generate export keys from next two blocks of 16 bytes */
	sec_hash_16(sec->sec_decrypt_key, &key_block[16], client_random, server_random);
	sec_hash_16(sec->sec_encrypt_key, &key_block[32], client_random, server_random);

	if (rc4_key_size == 1)
	{
		DEBUG_SEC("40-bit encryption enabled");
		sec_make_40bit(sec->sec_sign_key);
		sec_make_40bit(sec->sec_decrypt_key);
		sec_make_40bit(sec->sec_encrypt_key);
		sec->rc4_key_len = 8;
	}
	else
	{
		DEBUG_SEC("rc_4_key_size == %d, 128-bit encryption enabled", rc4_key_size);
		sec->rc4_key_len = 16;
	}

	/* Save initial RC4 keys as update keys */
	memcpy(sec->sec_decrypt_update_key, sec->sec_decrypt_key, 16);
	memcpy(sec->sec_encrypt_update_key, sec->sec_encrypt_key, 16);

	/* Initialize RC4 state arrays */
	sec->rc4_decrypt_key = crypto_rc4_init(sec->sec_decrypt_key, sec->rc4_key_len);
	sec->rc4_encrypt_key = crypto_rc4_init(sec->sec_encrypt_key, sec->rc4_key_len);
}

/* Output a uint32 into a buffer (little-endian) */
void
buf_out_uint32(uint8 * buffer, uint32 value)
{
	buffer[0] = (value) & 0xff;
	buffer[1] = (value >> 8) & 0xff;
	buffer[2] = (value >> 16) & 0xff;
	buffer[3] = (value >> 24) & 0xff;
}

/* Generate a MAC hash (5.2.3.1), using a combination of SHA1 and MD5 */
void
sec_sign(uint8 * signature, int siglen, uint8 * session_key, int keylen, uint8 * data, int datalen)
{
	uint8 shasig[20];
	uint8 md5sig[16];
	uint8 lenhdr[4];
	CryptoSha1 sha1;
	CryptoMd5 md5;

	buf_out_uint32(lenhdr, datalen);

	sha1 = crypto_sha1_init();
	crypto_sha1_update(sha1, session_key, keylen);
	crypto_sha1_update(sha1, pad_54, 40);
	crypto_sha1_update(sha1, lenhdr, 4);
	crypto_sha1_update(sha1, data, datalen);
	crypto_sha1_final(sha1, shasig);

	md5 = crypto_md5_init();
	crypto_md5_update(md5, session_key, keylen);
	crypto_md5_update(md5, pad_92, 48);
	crypto_md5_update(md5, shasig, 20);
	crypto_md5_final(md5, md5sig);

	memcpy(signature, md5sig, siglen);
}

/* Update an encryption key */
static void
sec_update(rdpSec * sec, uint8 * key, uint8 * update_key)
{
	uint8 shasig[20];
	CryptoSha1 sha1;
	CryptoMd5 md5;
	CryptoRc4 update;

	sha1 = crypto_sha1_init();
	crypto_sha1_update(sha1, update_key, sec->rc4_key_len);
	crypto_sha1_update(sha1, pad_54, 40);
	crypto_sha1_update(sha1, key, sec->rc4_key_len);
	crypto_sha1_final(sha1, shasig);

	md5 = crypto_md5_init();
	crypto_md5_update(md5, update_key, sec->rc4_key_len);
	crypto_md5_update(md5, pad_92, 48);
	crypto_md5_update(md5, shasig, 20);
	crypto_md5_final(md5, key);

	update = crypto_rc4_init(key, sec->rc4_key_len);
	crypto_rc4(update, sec->rc4_key_len, key, key);
	crypto_rc4_free(update);

	if (sec->rc4_key_len == 8)
		sec_make_40bit(key);
}

/* Encrypt data using RC4 */
static void
sec_encrypt(rdpSec * sec, uint8 * data, int length)
{
	if (sec->sec_encrypt_use_count == 4096)
	{
		sec_update(sec, sec->sec_encrypt_key, sec->sec_encrypt_update_key);
		crypto_rc4_free(sec->rc4_encrypt_key);
		sec->rc4_encrypt_key = crypto_rc4_init(sec->sec_encrypt_key, sec->rc4_key_len);
		sec->sec_encrypt_use_count = 0;
	}

	crypto_rc4(sec->rc4_encrypt_key, length, data, data);
	sec->sec_encrypt_use_count++;
}

/* Decrypt data using RC4 */
static void
sec_decrypt(rdpSec * sec, uint8 * data, int length)
{
#ifndef DISABLE_TLS
	if (sec->tls_connected)
		return;
#endif

	if (sec->sec_decrypt_use_count == 4096)
	{
		sec_update(sec, sec->sec_decrypt_key, sec->sec_decrypt_update_key);
		crypto_rc4_free(sec->rc4_decrypt_key);
		sec->rc4_decrypt_key = crypto_rc4_init(sec->sec_decrypt_key, sec->rc4_key_len);
		sec->sec_decrypt_use_count = 0;
	}

	crypto_rc4(sec->rc4_decrypt_key, length, data, data);
	sec->sec_decrypt_use_count++;
}

/* Initialize secure transport packet */
STREAM
sec_init(rdpSec * sec, uint32 flags, int maxlen)
{
	STREAM s;
	int hdrlen;

	if (flags)
	{
		if (!(sec->license->license_issued))
			hdrlen = (flags & SEC_ENCRYPT) ? 12 : 4;
		else
			hdrlen = (flags & SEC_ENCRYPT) ? 12 : 0;
	}
	else
		hdrlen = 0;

	s = mcs_init(sec->mcs, maxlen + hdrlen);
	s_push_layer(s, sec_hdr, hdrlen);

	return s;
}

/* Initialise fast path secure transport packet */
STREAM
sec_fp_init(rdpSec * sec, uint32 flags, int maxlen)
{
	STREAM s;
	int hdrlen;

	hdrlen = (flags & SEC_ENCRYPT) ? 8 : 0;
	s = mcs_fp_init(sec->mcs, maxlen + hdrlen);
	s_push_layer(s, sec_hdr, hdrlen);

	return s;
}

/* Transmit secure transport packet over specified channel */
void
sec_send_to_channel(rdpSec * sec, STREAM s, uint32 flags, uint16 channel)
{
	int datalen;
	s_pop_layer(s, sec_hdr);

	if (flags)
	{
		/* Basic Security Header */
		if (!(sec->license->license_issued) || (flags & SEC_ENCRYPT))
			out_uint32_le(s, flags); /* flags */

		if (flags & SEC_ENCRYPT)
		{
			flags &= ~SEC_ENCRYPT;
			datalen = s->end - s->p - 8;

#if WITH_DEBUG
			DEBUG_SEC("Sending encrypted packet:");
			hexdump(s->p + 8, datalen);
#endif

			sec_sign(s->p, 8, sec->sec_sign_key, sec->rc4_key_len, s->p + 8, datalen);
			sec_encrypt(sec, s->p + 8, datalen);
		}
	}

	mcs_send_to_channel(sec->mcs, s, channel);
}

/* Transmit secure transport packet */

void
sec_send(rdpSec * sec, STREAM s, uint32 flags)
{
	sec_send_to_channel(sec, s, flags, MCS_GLOBAL_CHANNEL);
}

/* Transmit secure fast path packet */
void
sec_fp_send(rdpSec * sec, STREAM s, uint32 flags)
{
	int datalen;
	s_pop_layer(s, sec_hdr);
	if (flags & SEC_ENCRYPT)
	{
		datalen = ((int) (s->end - s->p)) - 8;
		sec_sign(s->p, 8, sec->sec_sign_key, sec->rc4_key_len, s->p + 8, datalen);
		sec_encrypt(sec, s->p + 8, datalen);
	}
	mcs_fp_send(sec->mcs, s, flags);
}

/* Transfer the client random to the server */
static void
sec_establish_key(rdpSec * sec)
{
	uint32 length = sec->server_public_key_len + SEC_PADDING_SIZE;
	uint32 flags = SEC_EXCHANGE_PKT;
	STREAM s;

	s = sec_init(sec, flags, length + 4);

	out_uint32_le(s, length);
	out_uint8p(s, sec->sec_crypted_random, sec->server_public_key_len);
	out_uint8s(s, SEC_PADDING_SIZE);

	s_mark_end(s);
	sec_send(sec, s, flags);
}

static void
sec_out_client_core_data(rdpSec * sec, rdpSet * settings, STREAM s)
{
	char * p;
	size_t len;
	uint16 highColorDepth;
	uint16 supportedColorDepths;
	uint16 earlyCapabilityFlags;
	int con_type;

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
	out_uint32_le(s, sec->mcs->iso->nego->selected_protocol); /* serverSelectedProtocol */
}

static void
sec_out_client_security_data(rdpSec * sec, rdpSet * settings, STREAM s)
{
	uint16 encryptionMethods = 0;

	out_uint16_le(s, UDH_CS_SECURITY);	/* User Data Header type */
	out_uint16_le(s, 12);			/* total length */

	if (settings->encryption || sec->tls_connected)
		encryptionMethods = ENCRYPTION_40BIT_FLAG | ENCRYPTION_128BIT_FLAG;

	out_uint32_le(s, encryptionMethods);	/* encryptionMethods */
	out_uint32_le(s, 0);			/* extEncryptionMethods */
}

static void
sec_out_client_network_data(rdpSec * sec, rdpSet * settings, STREAM s)
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
sec_out_client_cluster_data(rdpSec * sec, rdpSet * settings, STREAM s)
{
	out_uint16_le(s, UDH_CS_CLUSTER);	/* User Data Header type */
	out_uint16_le(s, 12);			/* total length */

	out_uint32_le(s, (settings->console_session || sec->rdp->redirect_session_id) ?
		REDIRECTED_SESSIONID_FIELD_VALID | REDIRECTION_SUPPORTED | REDIRECTION_VERSION4 :
		REDIRECTION_SUPPORTED | REDIRECTION_VERSION4); /* flags */

	out_uint32_le(s, sec->rdp->redirect_session_id); /* RedirectedSessionID */
}

static void
sec_out_client_monitor_data(rdpSec * sec, rdpSet * settings, STREAM s)
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
sec_out_gcc_conference_create_request(rdpSec * sec, STREAM s)
{
	int length;
	rdpSet * settings = sec->rdp->settings;

	/* See ITU-T Rec. T.124 (02/98) Generic Conference Control */

	/* the part before userData is of a fixed size, making things convenient */
	s->p = s->data + 23;
	sec_out_client_core_data(sec, settings, s);
	sec_out_client_cluster_data(sec, settings, s);
	sec_out_client_security_data(sec, settings, s);
	sec_out_client_network_data(sec, settings, s);
	sec_out_client_monitor_data(sec, settings, s);
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

static void
revcpy(uint8 * out, uint8 * in, int len)
{
	int i;
	in += len;
	for (i = 0; i < len; i++)
	{
		*out++ = *--in;
	}
}

/* Parse a Server Proprietary Certificate RSA Public Key */
static RD_BOOL
sec_parse_public_key(rdpSec * sec, STREAM s, uint32 len, uint8 * modulus, uint8 * exponent)
{
	uint32 magic;
	uint32 modulus_len;

	in_uint32_le(s, magic);
	if (magic != SEC_RSA_MAGIC)
	{
		ui_error(sec->rdp->inst, "RSA magic 0x%x\n", magic);
		return False;
	}

	in_uint32_le(s, modulus_len);
	if (4 + 4 + 4 + 4 + SEC_EXPONENT_SIZE + modulus_len != len)
	{
		ui_error(sec->rdp->inst, "Inconsistent Server Proprietary Certificate public key size\n");
		return False;
	}
	modulus_len -= SEC_PADDING_SIZE;
	if ((modulus_len < SEC_MODULUS_SIZE) || (modulus_len > SEC_MAX_MODULUS_SIZE))
	{
		ui_error(sec->rdp->inst, "Bad Server Proprietary Certificate public key size (%u bits)\n", modulus_len * 8);
		return False;
	}

	in_uint8s(s, 4);	/* modulus_bits - must match modulus_len */
	in_uint8s(s, 4);	/* datalen - how much data can be encoded */
	revcpy(exponent, s->p, SEC_EXPONENT_SIZE);
	in_uint8s(s, SEC_EXPONENT_SIZE);
	revcpy(modulus, s->p, modulus_len);
	in_uint8s(s, modulus_len);
	in_uint8s(s, SEC_PADDING_SIZE);	/* zero padding - included in modulus_len but not in modulus_bits */
	sec->server_public_key_len = modulus_len;

	return s_check(s);
}

/* Parse a Proprietary Certificate signature */
static RD_BOOL
sec_parse_public_sig(STREAM s, uint32 len)
{
	/* The Proprietary Certificate signature uses a static published private key.
	 * That is completely nonsense, so we won't bother checking it. */

	in_uint8s(s, len);
	return len == 72;
}

/* Parse Server Security Data */
static RD_BOOL
sec_parse_server_security_data(rdpSec * sec, STREAM s, uint32 * encryptionMethod, uint8 server_random[SEC_RANDOM_SIZE], uint8 * modulus, uint8 * exponent)
{
	uint32 encryptionLevel;
	uint32 serverRandomLen;
	uint32 serverCertLen;
	uint32 certChainVersion;
	uint32 dwVersion;

	in_uint32_le(s, *encryptionMethod);	/* 1 = 40-bit, 2 = 128-bit, 0 for TLS/CredSSP */
	in_uint32_le(s, encryptionLevel);	/* 1 = low, 2 = client compatible, 3 = high */
	if (encryptionLevel == 0)		/* no encryption */
		return False;
	in_uint32_le(s, serverRandomLen);
	in_uint32_le(s, serverCertLen);

	if (serverRandomLen != SEC_RANDOM_SIZE)
	{
		ui_error(sec->rdp->inst, "random len %d, expected %d\n", serverRandomLen, SEC_RANDOM_SIZE);
		return False;
	}

	in_uint8a(s, server_random, SEC_RANDOM_SIZE);

	/* Server Certificate: */
	in_uint32_le(s, dwVersion); /* bit 0x80000000 = temporary certificate */
	certChainVersion = dwVersion & 0x7FFFFFFF;

	if (certChainVersion == 1)	 /* Server Proprietary Certificate */
	{
		uint16 wPublicKeyBlobType, wPublicKeyBlobLen;
		uint16 wSignatureBlobType, wSignatureBlobLen;

		DEBUG_SEC("We're going for a Server Proprietary Certificate (no TS license)");
		in_uint8s(s, 4);	/* dwSigAlgId must be 1 (SIGNATURE_ALG_RSA) */
		in_uint8s(s, 4);	/* dwKeyAlgId must be 1 (KEY_EXCHANGE_ALG_RSA ) */

		in_uint16_le(s, wPublicKeyBlobType);
		if (wPublicKeyBlobType != BB_RSA_KEY_BLOB)
			return False;

		in_uint16_le(s, wPublicKeyBlobLen);

		if (!sec_parse_public_key(sec, s, wPublicKeyBlobLen, modulus, exponent))
			return False;

		in_uint16_le(s, wSignatureBlobType);
		if (wSignatureBlobType != BB_RSA_SIGNATURE_BLOB)
			return False;

		in_uint16_le(s, wSignatureBlobLen);
		if (!sec_parse_public_sig(s, wSignatureBlobLen))
			return False;
	}
	else if (certChainVersion == 2)	 /* X.509 */
	{
		uint32 cert_total_count, cert_counter;
		uint32 license_cert_len, ts_cert_len;
		CryptoCert license_cert, ts_cert;

		DEBUG_SEC("We're going for a X.509 Certificate (TS license)");
		in_uint32_le(s, cert_total_count);	/* Number of certificates */
		DEBUG_SEC("Cert chain length: %d", cert_total_count);
		if (cert_total_count < 2)
		{
			ui_error(sec->rdp->inst, "Server didn't send enough X509 certificates\n");
			return False;
		}
		/* X.509 Certificate Chain: */
		/* Only the 2 last certificates in chain are _really_ interesting */
		for (cert_counter=0; cert_counter < cert_total_count - 2; cert_counter++)
		{
			uint32 ignorelen;
			CryptoCert ignorecert;

			DEBUG_SEC("Ignoring cert: %d", cert_counter);
			in_uint32_le(s, ignorelen);
			DEBUG_SEC("Ignored Certificate length is %d", ignorelen);
			ignorecert = crypto_cert_read(s->p, ignorelen);
			in_uint8s(s, ignorelen);
			if (ignorecert == NULL)
			{
				ui_error(sec->rdp->inst, "Couldn't read certificate %d from server certificate chain\n", cert_counter);
				return False;
			}

#ifdef WITH_DEBUG_SEC
			DEBUG_SEC("cert #%d (ignored):", cert_counter);
			crypto_cert_print_fp(stdout, ignorecert);
#endif
			/* TODO: Verify the certificate chain all the way from CA root to prevent MITM attacks */
			crypto_cert_free(ignorecert);
		}
		/* The second to last certificate is the license server */
		in_uint32_le(s, license_cert_len);
		DEBUG_SEC("License Server Certificate length is %d", license_cert_len);
		license_cert = crypto_cert_read(s->p, license_cert_len);
		in_uint8s(s, license_cert_len);
		if (NULL == license_cert)
		{
			ui_error(sec->rdp->inst, "Couldn't load License Server Certificate from server\n");
			return False;
		}
#ifdef WITH_DEBUG_SEC
		crypto_cert_print_fp(stdout, license_cert);
#endif
		/* The last certificate is the Terminal Server */
		in_uint32_le(s, ts_cert_len);
		DEBUG_SEC("TS Certificate length is %d", ts_cert_len);
		ts_cert = crypto_cert_read(s->p, ts_cert_len);
		in_uint8s(s, ts_cert_len);
		if (NULL == ts_cert)
		{
			crypto_cert_free(license_cert);
			ui_error(sec->rdp->inst, "Couldn't load TS Certificate from server\n");
			return False;
		}
#ifdef WITH_DEBUG_SEC
		crypto_cert_print_fp(stdout, ts_cert);
#endif
		if (!crypto_cert_verify(ts_cert, license_cert))
		{
			crypto_cert_free(ts_cert);
			crypto_cert_free(license_cert);
			ui_error(sec->rdp->inst, "TS Certificate not signed with License Certificate\n");
			return False;
		}
		crypto_cert_free(license_cert);

		if (crypto_cert_get_pub_exp_mod(ts_cert, &(sec->server_public_key_len),
				exponent, SEC_EXPONENT_SIZE, modulus, SEC_MAX_MODULUS_SIZE) != 0)
		{
			ui_error(sec->rdp->inst, "Problem extracting RSA key from TS Certificate\n");
			crypto_cert_free(ts_cert);
			return False;
		}
		crypto_cert_free(ts_cert);
		if ((sec->server_public_key_len < SEC_MODULUS_SIZE) ||
		    (sec->server_public_key_len > SEC_MAX_MODULUS_SIZE))
		{
			ui_error(sec->rdp->inst, "Bad TS Certificate public key size (%u bits)\n",
			         sec->server_public_key_len * 8);
			return False;
		}
		in_uint8s(s, 8 + 4 * cert_total_count); /* Padding */
	}
	else
	{
		ui_error(sec->rdp->inst, "Invalid cert chain version\n");
		return False;
	}

	return s_check_end(s);
}

/* Process Server Security Data */
static void
sec_process_server_security_data(rdpSec * sec, STREAM s)
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
	if (!sec_parse_server_security_data(sec, s, &rc4_key_size, server_random, modulus, exponent))
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
	revcpy(client_random_rev, client_random, SEC_RANDOM_SIZE);
	crypto_rsa_encrypt(SEC_RANDOM_SIZE, client_random_rev, crypted_random_rev,
			sec->server_public_key_len, modulus, exponent);
	revcpy(sec->sec_crypted_random, crypted_random_rev, sec->server_public_key_len);
	sec_generate_keys(sec, client_random, server_random, rc4_key_size);
}

/* Process Server Core Data */
static void
sec_process_server_core_data(rdpSec * sec, STREAM s, uint16 length)
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
sec_process_server_network_data(rdpSec * sec, STREAM s)
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
sec_process_mcs_data(rdpSec * sec, STREAM s)
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
				sec_process_server_core_data(sec, s, length);
				break;

			case UDH_SC_NET: /* Server Network Data */
				sec_process_server_network_data(sec, s);
				break;

			case UDH_SC_SECURITY: /* Server Security Data */
				sec_process_server_security_data(sec, s);
				break;
		}

		s->p = next_tag;
	}
}

/* Receive secure transport packet
 * Some package types are processed internally.
 * If s is returned a package of *type must be processed by the caller */
STREAM
sec_recv(rdpSec * sec, secRecvType * type)
{
	STREAM s;
	uint16 channel;
	uint32 sec_flags;
	isoRecvType iso_type;

	while ((s = mcs_recv(sec->mcs, &iso_type, &channel)) != NULL)
	{
		if ((iso_type == ISO_RECV_FAST_PATH) ||
			(iso_type == ISO_RECV_FAST_PATH_ENCRYPTED))
		{
			*type = SEC_RECV_FAST_PATH;
			if (iso_type == ISO_RECV_FAST_PATH_ENCRYPTED)
			{
				in_uint8s(s, 8);	/* dataSignature */ /* TODO: Check signature! */
				sec_decrypt(sec, s->p, s->end - s->p);
			}
			return s;
		}
		if (iso_type != ISO_RECV_X224)
		{
			ui_error(sec->rdp->inst, "expected ISO_RECV_X224, got %d\n", iso_type);
			return NULL;
		}
		if (sec->rdp->settings->encryption || !sec->license->license_issued)
		{
			/* basicSecurityHeader: */
			in_uint32_le(s, sec_flags);

			if ((sec_flags & SEC_ENCRYPT) || (sec_flags & SEC_REDIRECTION_PKT))
			{
				in_uint8s(s, 8);	/* dataSignature */ /* TODO: Check signature! */
				sec_decrypt(sec, s->p, s->end - s->p);
			}

			if (sec_flags & SEC_LICENSE_PKT)
			{
				*type = SEC_RECV_LICENSE;
				license_process(sec->license, s);
				continue;
			}

			if (sec_flags & SEC_REDIRECTION_PKT)
			{
				*type = SEC_RECV_REDIRECT;
				return s;
			}
		}

		if (channel != MCS_GLOBAL_CHANNEL)
		{
			vchan_process(sec->mcs->chan, s, channel);
			*type = SEC_RECV_IOCHANNEL;
			return s;
		}
		*type = SEC_RECV_SHARE_CONTROL;
		return s;
	}

	return NULL;
}

#ifndef DISABLE_TLS

/* verify SSL/TLS connection integrity. 2 checks are carried out. First make sure that the
 * certificate is assigned to the server we're connected to, and second make sure that the
 * certificate is signed by a trusted certification authority
 */

static RD_BOOL
sec_verify_tls(rdpSec * sec, const char * server)
{
	RD_BOOL verified = False;
	CryptoCert cert;
	char * fingerprint;
	char * subject;
	char * issuer;

	cert = tls_get_certificate(sec->tls);
	if (!cert)
	{
		goto exit;
	}

	subject = crypto_cert_get_subject(cert);
	issuer = crypto_cert_get_issuer(cert);
	fingerprint = crypto_cert_get_fingerprint(cert);

	verified = tls_verify(sec->tls, server);
	if (verified)
	{
		verified = crypto_cert_verify_peer_identity(cert, server);
	}
	verified = ui_check_certificate(sec->rdp->inst, fingerprint, subject, issuer, verified);

	free(subject);
	free(issuer);
	free(fingerprint);

exit:
	if (cert)
	{
		crypto_cert_free(cert);
		cert = NULL;
	}

	return verified;
}
#endif

/* Establish a secure connection */
RD_BOOL
sec_connect(rdpSec * sec, char *server, char *username, int port)
{
	NEGO *nego = sec->mcs->iso->nego;

	sec->license->license_issued = 0;
	if (sec->rdp->settings->nla_security)
		nego->enabled_protocols[PROTOCOL_NLA] = 1;
	if (sec->rdp->settings->tls_security)
		nego->enabled_protocols[PROTOCOL_TLS] = 1;
	if (sec->rdp->settings->rdp_security)
		nego->enabled_protocols[PROTOCOL_RDP] = 1;

	if (!iso_connect(sec->mcs->iso, server, username, port))
		return False;

#ifndef DISABLE_TLS
	if(nego->selected_protocol & PROTOCOL_NLA)
	{
		/* TLS with NLA was successfully negotiated */
		RD_BOOL status = 1;
		printf("TLS encryption with NLA negotiated\n");
		sec->tls = tls_new();
		if (!tls_connect(sec->tls, sec->mcs->iso->tcp->sock))
			return False;
		if (!sec_verify_tls(sec, server))
			return False;
		sec->tls_connected = 1;
		sec->rdp->settings->encryption = 0;

		if (!sec->rdp->settings->autologin)
			if (!ui_authenticate(sec->rdp->inst))
				return False;

		sec->credssp = credssp_new(sec);

		if (credssp_authenticate(sec->credssp) < 0)
		{
			printf("Authentication failure, check credentials.\n"
					"If credentials are valid, the NTLMSSP implementation may be to blame.\n");
			credssp_free(sec->credssp);
			return 0;
		}

		credssp_free(sec->credssp);

		status = mcs_connect(sec->mcs);
		return status;
	}
	else if(nego->selected_protocol & PROTOCOL_TLS)
	{
		/* TLS without NLA was successfully negotiated */
		RD_BOOL success;
		printf("TLS encryption negotiated\n");
		sec->tls = tls_new();
		if (!tls_connect(sec->tls, sec->mcs->iso->tcp->sock))
			return False;
		if (!sec_verify_tls(sec, server))
			return False;
		sec->tls_connected = 1;
		sec->rdp->settings->encryption = 0;
		success = mcs_connect(sec->mcs);
		return success;
	}
	else
#endif
	{
		RD_BOOL success;

		printf("Standard RDP encryption negotiated\n");

		success = mcs_connect(sec->mcs);

		if (success && sec->rdp->settings->encryption)
			sec_establish_key(sec);

		return success;
	}

	return 0;
}

/* Disconnect a connection */
void
sec_disconnect(rdpSec * sec)
{
	mcs_disconnect(sec->mcs);

#ifndef DISABLE_TLS
	if (sec->tls)
		tls_free(sec->tls);
	sec->tls = NULL;
	sec->tls_connected = 0;
#endif

	if (sec->rc4_decrypt_key)
		crypto_rc4_free(sec->rc4_decrypt_key);
	sec->rc4_decrypt_key = NULL;
	if (sec->rc4_encrypt_key)
		crypto_rc4_free(sec->rc4_encrypt_key);
	sec->rc4_encrypt_key = NULL;
}

rdpSec *
sec_new(struct rdp_rdp * rdp)
{
	rdpSec * self;

	self = (rdpSec *) xmalloc(sizeof(rdpSec));
	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpSec));
		self->rdp = rdp;
		self->mcs = mcs_new(self);
		self->license = license_new(self);
		self->rc4_decrypt_key = NULL;
		self->rc4_encrypt_key = NULL;
	}
	return self;
}

void
sec_free(rdpSec * sec)
{
	if (sec != NULL)
	{
		license_free(sec->license);
		mcs_free(sec->mcs);
		xfree(sec);
	}
}
