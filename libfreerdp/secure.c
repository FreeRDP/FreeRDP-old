/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Protocol services - RDP encryption and licensing
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
#include "mcs.h"
#include "chan.h"
#include "secure.h"
#include "licence.h"
#include "rdp.h"
#include "rdpset.h"
#include "iso.h"
#include "mem.h"
#include "debug.h"
#include "tcp.h"

#ifndef DISABLE_TLS
#include "tls.h"
#include "credssp.h"
#endif

RD_BOOL
sec_global_init(void)
{
	return crypto_global_init();
}

void
sec_global_finish(void)
{
	crypto_global_finish();
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
		DEBUG("40-bit encryption enabled\n");
		sec_make_40bit(sec->sec_sign_key);
		sec_make_40bit(sec->sec_decrypt_key);
		sec_make_40bit(sec->sec_encrypt_key);
		sec->rc4_key_len = 8;
	}
	else
	{
		DEBUG("rc_4_key_size == %d, 128-bit encryption enabled\n", rc4_key_size);
		sec->rc4_key_len = 16;
	}

	/* Save initial RC4 keys as update keys */
	memcpy(sec->sec_decrypt_update_key, sec->sec_decrypt_key, 16);
	memcpy(sec->sec_encrypt_update_key, sec->sec_encrypt_key, 16);

	/* Initialise RC4 state arrays */
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

/* Perform an RSA public key encryption operation */
static void
sec_rsa_encrypt(uint8 * out, uint8 * in, int len, uint32 modulus_size, uint8 * modulus, uint8 * exponent)
{
	crypto_rsa_encrypt(len, in, out, modulus_size, modulus, exponent);
}

/* Initialise secure transport packet */
STREAM
sec_init(rdpSec * sec, uint32 flags, int maxlen)
{
	STREAM s;
	int hdrlen;

	if (flags)
	{
		if (!(sec->licence->licence_issued))
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
		if (!(sec->licence->licence_issued) || (flags & SEC_ENCRYPT))
			out_uint32_le(s, flags); /* flags */

		if (flags & SEC_ENCRYPT)
		{
			flags &= ~SEC_ENCRYPT;
			datalen = s->end - s->p - 8;

#if WITH_DEBUG
			DEBUG("Sending encrypted packet:\n");
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
	p = xstrdup_out_unistr(sec->rdp, settings->hostname, &len);
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

	earlyCapabilityFlags = RNS_UD_CS_SUPPORT_ERRINFO_PDU;

	if (settings->server_depth == 32)
		earlyCapabilityFlags |= RNS_UD_CS_WANT_32BPP_SESSION;
	
	out_uint16_le(s, earlyCapabilityFlags); /* earlyCapabilityFlags */
	out_uint8s(s, 64); /* clientDigProductId (64 bytes) */
	out_uint8(s, 0); /* connectionType, only valid when RNS_UD_CS_VALID_CONNECTION_TYPE is set in earlyCapabilityFlags */
	out_uint8(s, 0); /* pad1octet */
	out_uint32_le(s, sec->negotiated_protocol); /* serverSelectedProtocol */
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
	
	DEBUG_RDP5("num_channels is %d\n", settings->num_channels);
	if (settings->num_channels > 0)
	{
		out_uint16_le(s, UDH_CS_NET);	/* User Data Header type */
		out_uint16_le(s, settings->num_channels * 12 + 8);	/* total length */

		out_uint32_le(s, settings->num_channels);	/* channelCount */
		for (i = 0; i < settings->num_channels; i++)
		{
			DEBUG_RDP5("Requesting channel %s\n", settings->channels[i].name);
			out_uint8a(s, settings->channels[i].name, 8); /* name (8 bytes) 7 characters with null terminator */
			out_uint32_be(s, settings->channels[i].flags); /* options (4 bytes) */
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
	s->p = s->data + length + 23; 			/* userData (outputted earlier) */

	s_mark_end(s);
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
	in_uint8a(s, exponent, SEC_EXPONENT_SIZE);
	in_uint8a(s, modulus, modulus_len);
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

		DEBUG_RDP5("We're going for a Server Proprietary Certificate (no TS license)\n");
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
		CryptoPublicKey server_public_key;

		DEBUG_RDP5("We're going for a X.509 Certificate (TS license)\n");
		in_uint32_le(s, cert_total_count);	/* Number of certificates */
		DEBUG_RDP5("Cert chain length: %d\n", cert_total_count);
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

			DEBUG_RDP5("Ignoring cert: %d\n", cert_counter);
			in_uint32_le(s, ignorelen);
			DEBUG_RDP5("Ignored Certificate length is %d\n", ignorelen);
			ignorecert = crypto_cert_read(s->p, ignorelen);
			in_uint8s(s, ignorelen);
			if (ignorecert == NULL)
			{
				ui_error(sec->rdp->inst, "Couldn't read certificate %d from server certificate chain\n", cert_counter);
				return False;
			}

#ifdef WITH_DEBUG_RDP5
			DEBUG_RDP5("cert #%d (ignored):\n", cert_counter);
			crypto_cert_print_fp(stdout, ignorecert);
#endif
			/* TODO: Verify the certificate chain all the way from CA root to prevent MITM attacks */
			crypto_cert_free(ignorecert);
		}
		/* The second to last certificate is the license server */
		in_uint32_le(s, license_cert_len);
		DEBUG_RDP5("License Server Certificate length is %d\n", license_cert_len);
		license_cert = crypto_cert_read(s->p, license_cert_len);
		in_uint8s(s, license_cert_len);
		if (NULL == license_cert)
		{
			ui_error(sec->rdp->inst, "Couldn't load License Server Certificate from server\n");
			return False;
		}
#ifdef WITH_DEBUG_RDP5
		crypto_cert_print_fp(stdout, license_cert);
#endif
		/* The last certificate is the Terminal Server */
		in_uint32_le(s, ts_cert_len);
		DEBUG_RDP5("TS Certificate length is %d\n", ts_cert_len);
		ts_cert = crypto_cert_read(s->p, ts_cert_len);
		in_uint8s(s, ts_cert_len);
		if (NULL == ts_cert)
		{
			crypto_cert_free(license_cert);
			ui_error(sec->rdp->inst, "Couldn't load TS Certificate from server\n");
			return False;
		}
#ifdef WITH_DEBUG_RDP5
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

		server_public_key = crypto_cert_get_public_key(ts_cert, &(sec->server_public_key_len));
		if (NULL == server_public_key)
		{
			DEBUG_RDP5("Could not read RSA key from TS Certificate\n");
			crypto_cert_free(ts_cert);
			return False;
		}
		crypto_cert_free(ts_cert);
		if ((sec->server_public_key_len < SEC_MODULUS_SIZE) ||
		    (sec->server_public_key_len > SEC_MAX_MODULUS_SIZE))
		{
			ui_error(sec->rdp->inst, "Bad TS Certificate public key size (%u bits)\n",
			         sec->server_public_key_len * 8);
			crypto_public_key_free(server_public_key);
			return False;
		}
		if (crypto_public_key_get_exp_mod(server_public_key, exponent, SEC_EXPONENT_SIZE,
					 modulus, SEC_MAX_MODULUS_SIZE) != 0)
		{
			ui_error(sec->rdp->inst, "Problem extracting RSA exponent, modulus\n");
			crypto_public_key_free(server_public_key);
			return False;
		}
		crypto_public_key_free(server_public_key);
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
	
	memset(modulus, 0, sizeof(modulus));
	memset(exponent, 0, sizeof(exponent));
	if (!sec_parse_server_security_data(sec, s, &rc4_key_size, server_random, modulus, exponent))
	{
		/* encryptionMethod (rc4_key_size) = 0 means TLS */
		if (rc4_key_size > 0)
		{
			DEBUG("Failed to parse crypt info\n");
		}
		return;
	}
	
	DEBUG("Generating client random\n");
	generate_random(client_random);
	sec_rsa_encrypt(sec->sec_crypted_random, client_random, SEC_RANDOM_SIZE,
			sec->server_public_key_len, modulus, exponent);
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
	
	DEBUG_RDP5("Server RDP version is %d\n", sec->rdp->settings->rdp_version);
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
	uint16 channelCount;
	uint16 io_channel_id;

	in_uint16_le(s, io_channel_id);
	in_uint16_le(s, channelCount);
	/* TODO: Check that it matches rdp->settings->num_channels */

	for (i = 0; i < channelCount; i++)
	{
		uint16 channel_id;
		in_uint16_le(s, channel_id);	/* Channel id allocated to requested channel number i */
		/* TODO: Assign channel ids here instead of in freerdp.c l_rdp_connect */
		if (channel_id != sec->rdp->settings->channels[i].chan_id)
		{
			ui_error(sec->rdp->inst, "channel %d is %d but should have been %d\n", i, channel_id, sec->rdp->settings->channels[i].chan_id);
		}
	}
	if (channelCount & 1)
		in_uint8s(s, 2);	/* Padding */
}

/* Process connect response data blob */
void
sec_process_mcs_data(rdpSec * sec, STREAM s)
{
	uint16 type;
	uint16 length;
	uint8 *next_tag;
	uint8 value_len;

	in_uint8s(s, 21);	/* TODO: T.124 ConferenceCreateResponse userData with key h221NonStandard McDn */
	in_uint8(s, value_len);
	if (value_len & 0x80)
		in_uint8(s, value_len);

	/* Server Core Data structure with User Data Header */
	in_uint16_le(s, type);
	in_uint16_le(s, length);
	next_tag = s->p + length - 4;
	if (type != UDH_SC_CORE)
	{
		ui_error(sec->rdp->inst, "UDH_SC_CORE response tag 0x%x\n", type);
		return;
	}
	sec_process_server_core_data(sec, s, length);
	if(s->p != next_tag)
		ui_error(sec->rdp->inst, "Server Core Data length error\n");

	/* Server Network Data structure with User Data Header */
	in_uint16_le(s, type);
	in_uint16_le(s, length);
	next_tag = s->p + length - 4;
	if (type != UDH_SC_NET)
	{
		ui_error(sec->rdp->inst, "UDH_SC_NET response tag 0x%x\n", type);
		return;
	}
	sec_process_server_network_data(sec, s);
	if(s->p != next_tag)
		ui_error(sec->rdp->inst, "Server Network Data length error\n");

	/* Server Security Data structure with User Data Header */
	in_uint16_le(s, type);
	in_uint16_le(s, length);
	next_tag = s->p + length - 4;
	if (type != UDH_SC_SECURITY)
	{
		ui_error(sec->rdp->inst, "UDH_SC_SECURITY response tag 0x%x\n", type);
		return;
	}
	sec_process_server_security_data(sec, s);
	if(s->p != next_tag)
		ui_error(sec->rdp->inst, "Server Security Data length error\n");
}

/* Receive secure transport packet */
STREAM
sec_recv(rdpSec * sec, secRecvType * type)
{
	STREAM s;
	uint16 channel;
	uint32 sec_flags;
	isoRecvType iso_type;
	
	while ((s = mcs_recv(sec->mcs, &channel, &iso_type)) != NULL)
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
		if (sec->rdp->settings->encryption || (!(sec->licence->licence_issued) && !(sec->tls_connected)))
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
				licence_process(sec->licence, s);
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

/* Establish a secure connection */
RD_BOOL
sec_connect(rdpSec * sec, char *server, char *username, int port)
{
	/* Don't forget to set this *before* iso_connect(), otherwise you'll bang your head on the wall */
	sec->requested_protocol = PROTOCOL_RDP;

	if (!iso_connect(sec->mcs->iso, server, username, port))
		return False;

#ifndef DISABLE_TLS
	if(sec->negotiated_protocol == PROTOCOL_NLA)
	{
		/* TLS with NLA was successfully negotiated */
		printf("PROTOCOL_NLA negotiated\n");
		sec->ctx = tls_create_context();
		sec->ssl = tls_connect(sec->ctx, sec->mcs->iso->tcp->sock, server);
		sec->tls_connected = 1;
		ntlm_send_negotiate_message(sec);
		credssp_recv(sec);
		exit(0); /* not implemented from this point */
	}
	else if(sec->negotiated_protocol == PROTOCOL_TLS)
	{
		/* TLS without NLA was successfully negotiated */
		RD_BOOL success;
		printf("PROTOCOL_TLS negotiated\n");
		sec->ctx = tls_create_context();
		sec->ssl = tls_connect(sec->ctx, sec->mcs->iso->tcp->sock, server);
		sec->tls_connected = 1;
		sec->rdp->settings->encryption = 0;
		success = mcs_connect(sec->mcs);
		
		if (success && sec->rdp->settings->encryption)
			sec_establish_key(sec);
		
		return success;
	}
	else
#endif
	{
		RD_BOOL success;
		printf("PROTOCOL_RDP negotiated\n");
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
		self->licence = licence_new(self);

#ifndef DISABLE_TLS
		self->nla = nla_new(self);
#endif

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
		licence_free(sec->licence);
		mcs_free(sec->mcs);

#ifndef DISABLE_TLS
		nla_free(sec->nla);
#endif

		if (sec->rc4_decrypt_key)
			crypto_rc4_free(sec->rc4_decrypt_key);
		if (sec->rc4_encrypt_key)
			crypto_rc4_free(sec->rc4_encrypt_key);

		xfree(sec);
	}
}
