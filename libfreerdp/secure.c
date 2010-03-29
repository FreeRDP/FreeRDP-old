/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
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
#include "credssp.h"
#include "licence.h"
#include "rdp.h"
#include "rdpset.h"
#include "iso.h"
#include "mem.h"
#include "debug.h"
#include "tcp.h"

/* these are read only */
static uint8 pad_54[40] = {
	54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
	54, 54, 54,
	54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
	54, 54, 54
};

static uint8 pad_92[48] = {
	92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
	92, 92, 92, 92, 92, 92, 92,
	92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
	92, 92, 92, 92, 92, 92, 92
};

/*
 * I believe this is based on SSLv3 with the following differences:
 *  MAC algorithm (5.2.3.1) uses only 32-bit length in place of seq_num/type/length fields
 *  MAC algorithm uses SHA1 and MD5 for the two hash functions instead of one or other
 *  key_block algorithm (6.2.2) uses 'X', 'YY', 'ZZZ' instead of 'A', 'BB', 'CCC'
 *  key_block partitioning is different (16 bytes each: MAC secret, decrypt key, encrypt key)
 *  encryption/decryption keys updated every 4096 packets
 * See http://wp.netscape.com/eng/ssl3/draft302.txt
 */

/*
 * 48-byte transformation used to generate master secret (6.1) and key material (6.2.2).
 * Both SHA1 and MD5 algorithms are used.
 */
void
sec_hash_48(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2, uint8 salt)
{
	uint8 shasig[20];
	uint8 pad[4];
	SSL_SHA1 sha1;
	SSL_MD5 md5;
	int i;

	for (i = 0; i < 3; i++)
	{
		memset(pad, salt + i, i + 1);

		ssl_sha1_init(&sha1);
		ssl_sha1_update(&sha1, pad, i + 1);
		ssl_sha1_update(&sha1, in, 48);
		ssl_sha1_update(&sha1, salt1, 32);
		ssl_sha1_update(&sha1, salt2, 32);
		ssl_sha1_final(&sha1, shasig);

		ssl_md5_init(&md5);
		ssl_md5_update(&md5, in, 48);
		ssl_md5_update(&md5, shasig, 20);
		ssl_md5_final(&md5, &out[i * 16]);
	}
}

/*
 * 16-byte transformation used to generate export keys (6.2.2).
 */
void
sec_hash_16(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2)
{
	SSL_MD5 md5;

	ssl_md5_init(&md5);
	ssl_md5_update(&md5, in, 16);
	ssl_md5_update(&md5, salt1, 32);
	ssl_md5_update(&md5, salt2, 32);
	ssl_md5_final(&md5, out);
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
	ssl_rc4_set_key(&(sec->rc4_decrypt_key), sec->sec_decrypt_key, sec->rc4_key_len);
	ssl_rc4_set_key(&(sec->rc4_encrypt_key), sec->sec_encrypt_key, sec->rc4_key_len);
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
	SSL_SHA1 sha1;
	SSL_MD5 md5;

	buf_out_uint32(lenhdr, datalen);

	ssl_sha1_init(&sha1);
	ssl_sha1_update(&sha1, session_key, keylen);
	ssl_sha1_update(&sha1, pad_54, 40);
	ssl_sha1_update(&sha1, lenhdr, 4);
	ssl_sha1_update(&sha1, data, datalen);
	ssl_sha1_final(&sha1, shasig);

	ssl_md5_init(&md5);
	ssl_md5_update(&md5, session_key, keylen);
	ssl_md5_update(&md5, pad_92, 48);
	ssl_md5_update(&md5, shasig, 20);
	ssl_md5_final(&md5, md5sig);

	memcpy(signature, md5sig, siglen);
}

/* Update an encryption key */
static void
sec_update(rdpSec * sec, uint8 * key, uint8 * update_key)
{
	uint8 shasig[20];
	SSL_SHA1 sha1;
	SSL_MD5 md5;
	SSL_RC4 update;

	ssl_sha1_init(&sha1);
	ssl_sha1_update(&sha1, update_key, sec->rc4_key_len);
	ssl_sha1_update(&sha1, pad_54, 40);
	ssl_sha1_update(&sha1, key, sec->rc4_key_len);
	ssl_sha1_final(&sha1, shasig);

	ssl_md5_init(&md5);
	ssl_md5_update(&md5, update_key, sec->rc4_key_len);
	ssl_md5_update(&md5, pad_92, 48);
	ssl_md5_update(&md5, shasig, 20);
	ssl_md5_final(&md5, key);

	ssl_rc4_set_key(&update, key, sec->rc4_key_len);
	ssl_rc4_crypt(&update, key, key, sec->rc4_key_len);

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
		ssl_rc4_set_key(&(sec->rc4_encrypt_key), sec->sec_encrypt_key, sec->rc4_key_len);
		sec->sec_encrypt_use_count = 0;
	}

	ssl_rc4_crypt(&(sec->rc4_encrypt_key), data, data, length);
	sec->sec_encrypt_use_count++;
}

/* Decrypt data using RC4 */
void
sec_decrypt(rdpSec * sec, uint8 * data, int length)
{
	if (sec->sec_decrypt_use_count == 4096)
	{
		sec_update(sec, sec->sec_decrypt_key, sec->sec_decrypt_update_key);
		ssl_rc4_set_key(&(sec->rc4_decrypt_key), sec->sec_decrypt_key, sec->rc4_key_len);
		sec->sec_decrypt_use_count = 0;
	}

	ssl_rc4_crypt(&(sec->rc4_decrypt_key), data, data, length);
	sec->sec_decrypt_use_count++;
}

/* Perform an RSA public key encryption operation */
static void
sec_rsa_encrypt(uint8 * out, uint8 * in, int len, uint32 modulus_size, uint8 * modulus,
		uint8 * exponent)
{
	ssl_rsa_encrypt(out, in, len, modulus_size, modulus, exponent);
}

/* Initialise secure transport packet */
STREAM
sec_init(rdpSec * sec, uint32 flags, int maxlen)
{
	int hdrlen;
	STREAM s;

	if (!(sec->licence->licence_issued))
		hdrlen = (flags & SEC_ENCRYPT) ? 12 : 4;
	else
		hdrlen = (flags & SEC_ENCRYPT) ? 12 : 0;
	s = mcs_init(sec->mcs, maxlen + hdrlen);
	s_push_layer(s, sec_hdr, hdrlen);

	return s;
}

/* Initialise fast path secure transport packet */
STREAM
sec_fp_init(rdpSec * sec, uint32 flags, int maxlen)
{
	int hdrlen;
	STREAM s;

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
	if (!(sec->licence->licence_issued) || (flags & SEC_ENCRYPT))
		out_uint32_le(s, flags);

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
	uint32 flags = SEC_CLIENT_RANDOM;
	STREAM s;

	s = sec_init(sec, flags, length + 4);

	out_uint32_le(s, length);
	out_uint8p(s, sec->sec_crypted_random, sec->server_public_key_len);
	out_uint8s(s, SEC_PADDING_SIZE);

	s_mark_end(s);
	sec_send(sec, s, flags);
}

/* Output connect initial data blob */
static void
sec_out_mcs_data(rdpSec * sec, STREAM s)
{
	int i;
	rdpSet * settings = sec->rdp->settings;
	int hostlen = 2 * strlen(settings->hostname);
	int length = 158 + 76 + 12 + 4;

	if (settings->num_channels > 0)
		length += settings->num_channels * 12 + 8;

	if (hostlen > 30)
		hostlen = 30;

	/* Generic Conference Control (T.124) ConferenceCreateRequest */
	out_uint16_be(s, 5);
	out_uint16_be(s, 0x14);
	out_uint8(s, 0x7c);
	out_uint16_be(s, 1);

	out_uint16_be(s, (length | 0x8000));	/* remaining length */

	out_uint16_be(s, 8);	/* length? */
	out_uint16_be(s, 16);
	out_uint8(s, 0);
	out_uint16_le(s, 0xc001);
	out_uint8(s, 0);

	out_uint32_le(s, 0x61637544);	/* OEM ID: "Duca", as in Ducati. */
	out_uint16_be(s, ((length - 14) | 0x8000));	/* remaining length */

	/* Client Core Data */

	/* User Data Header */
	out_uint16_le(s, SEC_TAG_CLI_INFO);
	out_uint16_le(s, 212);	/* length */
	/* End of User Data Header */

	/*
		version (4 bytes)
		0x00080001	RDP 4.0
		0x00080004	RDP 5.0, 5.1 and 6.0

		Major version in high two bytes
		Minor version in low two bytes
	*/

	out_uint16_le(s, sec->rdp->settings->rdp_version >= 5 ? 4 : 1);	/* RDP version. 1 == RDP4, 4 == RDP5. */
	out_uint16_le(s, 0x0008); // Major version

	out_uint16_le(s, sec->rdp->settings->width); // desktopWidth
	out_uint16_le(s, sec->rdp->settings->height); // desktopHeight
	out_uint16_le(s, RNS_UD_COLOR_8BPP); // colorDepth

	out_uint16_le(s, RNS_UD_SAS_DEL); // SASSequence (Secure Access Sequence)
	out_uint32_le(s, sec->rdp->settings->keyboard_layout); // keyboardLayout
	out_uint32_le(s, 2600);	// clientBuild

	/* Unicode name of client, padded to 32 bytes */
	rdp_out_unistr(sec->rdp, s, sec->rdp->settings->hostname, hostlen);
	out_uint8s(s, 30 - hostlen);

	/* See
	   http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wceddk40/html/cxtsksupportingremotedesktopprotocol.asp */
	out_uint32_le(s, sec->rdp->settings->keyboard_type);
	out_uint32_le(s, sec->rdp->settings->keyboard_subtype);
	out_uint32_le(s, sec->rdp->settings->keyboard_functionkeys);

	/* Input Method Editor (IME) file name associated with the input locale.
	   Up to 31 Unicode characters plus a NULL terminator */
	//rdp_out_unistr(sec->rdp, s, sec->rdp->settings->keyboard_ime, 2 * strlen(sec->rdp->settings->keyboard_ime));
	//out_uint8s(s, 62 - 2 * strlen(sec->rdp->settings->keyboard_ime)); // imeFileName (64 bytes)
	out_uint8s(s, 64);

	out_uint16_le(s, RNS_UD_COLOR_8BPP); // postBeta2ColorDepth
	out_uint16_le(s, 1); // clientProductID

	out_uint32_le(s, 0); // serialNumber (should be initialized to 0)

	i = MIN(sec->rdp->settings->server_depth, 24);
	out_uint16_le(s, i); // highColorDepth

	i = RNS_UD_32BPP_SUPPORT | RNS_UD_24BPP_SUPPORT | RNS_UD_16BPP_SUPPORT | RNS_UD_15BPP_SUPPORT;
	out_uint16_le(s, i); // supportedColorDepths

	i = RNS_UD_CS_SUPPORT_ERRINFO_PDU;
	if (sec->rdp->settings->server_depth == 32)
	{
		i |= RNS_UD_CS_WANT_32BPP_SESSION;
	}
	out_uint32_le(s, i); // earlyCapabilityFlags

	out_uint8s(s, 64); // clientDigProductId (64 bytes)

	/*
		Optional missing fields:

		pad2octets (2 bytes): A 16-bit, unsigned integer. Padding to align the
		serverSelectedProtocol field on the correct byte boundary. If this field is present, then all of
		the preceding fields MUST also be present. If this field is not present, then none of the
		subsequent fields MUST be present.

		serverSelectedProtocol (4 bytes): A 32-bit, unsigned integer. It contains the value returned
		by the server in the selectedProtocol field of the RDP Negotiation Response structure
		(section 2.2.1.2.1). In the event that an RDP Negotiation Response structure was not sent,
		this field MUST be initialized to PROTOCOL_RDP (0). If this field is present, then all of the
		preceding fields MUST also be present.
	*/

	/* End of Client Core Data */

	/* Client Security Data */

	/* User Data Header */
	out_uint16_le(s, SEC_TAG_CLI_CRYPT); // Type CS_SECURITY (0xC002)
	out_uint16_le(s, 12); // Length
	/* End of User Data Header */

	out_uint32_le(s, sec->rdp->settings->encryption ? ENCRYPTION_40BIT_FLAG | ENCRYPTION_128BIT_FLAG : 0); // encryptionMethods

	/*
		extEncryptionMethods (4 bytes): A 32-bit, unsigned integer.This field is used exclusively for
		the French locale. In French locale clients, encryptionMethods MUST be set to 0 and
		extEncryptionMethods MUST be set to the value to which encryptionMethods would have
		been set. For non-French locale clients, this field MUST be set to 0.
	*/

	out_uint32_le(s, 0); // extEncryptionMethods

	/* End of Client Security Data */

	DEBUG_RDP5("num_channels is %d\n", settings->num_channels);
	if (settings->num_channels > 0)
	{
		/* Client Network Data */

		/* User Data Header */
		out_uint16_le(s, SEC_TAG_CLI_CHANNELS); // CS_NET (0xC003)
		out_uint16_le(s, settings->num_channels * 12 + 8); // Length
		/* End of User Data Header */

		out_uint32_le(s, settings->num_channels);	// channelCount
		for (i = 0; i < settings->num_channels; i++)
		{
			DEBUG_RDP5("Requesting channel %s\n", settings->channels[i].name);
			out_uint8a(s, settings->channels[i].name, 8); // name (8 bytes) 7 characters with null terminator
			out_uint32_be(s, settings->channels[i].flags); // options (4 bytes)
		}
		/* End of Client Network Data */
	}

	/* Client Cluster Data */
	out_uint16_le(s, SEC_TAG_CLI_4); // CS_CLUSTER (0xC004)
	out_uint16_le(s, 12); // Length
	out_uint32_le(s, sec->rdp->settings->console_session ?
		REDIRECTED_SESSIONID_FIELD_VALID | REDIRECTION_SUPPORTED | REDIRECTION_VERSION3 :
		REDIRECTION_SUPPORTED | REDIRECTION_VERSION3); // flags
	out_uint32_le(s, 0); // RedirectedSessionID
	/* End of Client Cluster Data */

	s_mark_end(s);
}

/* Parse a public key structure */
static RD_BOOL
sec_parse_public_key(rdpSec * sec, STREAM s, uint8 * modulus, uint8 * exponent)
{
	uint32 magic, modulus_len;

	in_uint32_le(s, magic);
	if (magic != SEC_RSA_MAGIC)
	{
		ui_error(sec->rdp->inst, "RSA magic 0x%x\n", magic);
		return False;
	}

	in_uint32_le(s, modulus_len);
	modulus_len -= SEC_PADDING_SIZE;
	if ((modulus_len < SEC_MODULUS_SIZE) || (modulus_len > SEC_MAX_MODULUS_SIZE))
	{
		ui_error(sec->rdp->inst, "Bad server public key size (%u bits)\n", modulus_len * 8);
		return False;
	}

	in_uint8s(s, 8);	/* modulus_bits, unknown */
	in_uint8a(s, exponent, SEC_EXPONENT_SIZE);
	in_uint8a(s, modulus, modulus_len);
	in_uint8s(s, SEC_PADDING_SIZE);
	sec->server_public_key_len = modulus_len;

	return s_check(s);
}

/* Parse a public signature structure */
static RD_BOOL
sec_parse_public_sig(rdpSec * sec, STREAM s, uint32 len, uint8 * modulus, uint8 * exponent)
{
	uint8 signature[SEC_MAX_MODULUS_SIZE];
	uint32 sig_len;

	if (len != 72)
	{
		return True;
	}
	memset(signature, 0, sizeof(signature));
	sig_len = len - 8;
	in_uint8a(s, signature, sig_len);
	return ssl_sig_ok(exponent, SEC_EXPONENT_SIZE, modulus, sec->server_public_key_len,
			  signature, sig_len);
}

/* Parse a crypto information structure */
static RD_BOOL
sec_parse_crypt_info(rdpSec * sec, STREAM s, uint32 * rc4_key_size,
		     uint8 ** server_random, uint8 * modulus, uint8 * exponent)
{
	uint32 crypt_level, random_len, rsa_info_len;
	uint32 cacert_len, cert_len, flags;
	SSL_CERT *cacert, *server_cert;
	SSL_RKEY *server_public_key;
	uint16 tag, length;
	uint8 *next_tag, *end;

	in_uint32_le(s, *rc4_key_size);	/* 1 = 40-bit, 2 = 128-bit */
	in_uint32_le(s, crypt_level);	/* 1 = low, 2 = medium, 3 = high */
	if (crypt_level == 0)	/* no encryption */
		return False;
	in_uint32_le(s, random_len);
	in_uint32_le(s, rsa_info_len);

	if (random_len != SEC_RANDOM_SIZE)
	{
		ui_error(sec->rdp->inst, "random len %d, expected %d\n", random_len, SEC_RANDOM_SIZE);
		return False;
	}

	in_uint8p(s, *server_random, random_len);

	/* RSA info */
	end = s->p + rsa_info_len;
	if (end > s->end)
		return False;

	in_uint32_le(s, flags);	/* 1 = RDP4-style, 0x80000002 = X.509 */
	if (flags & 1)
	{
		DEBUG_RDP5("We're going for the RDP4-style encryption\n");
		in_uint8s(s, 8);	/* unknown */

		while (s->p < end)
		{
			in_uint16_le(s, tag);
			in_uint16_le(s, length);

			next_tag = s->p + length;

			switch (tag)
			{
				case SEC_TAG_PUBKEY:
					if (!sec_parse_public_key(sec, s, modulus, exponent))
						return False;
					DEBUG_RDP5("Got Public key, RDP4-style\n");

					break;

				case SEC_TAG_KEYSIG:
					if (!sec_parse_public_sig(sec, s, length, modulus, exponent))
						return False;
					break;

				default:
					ui_unimpl(NULL, "crypt tag 0x%x\n", tag);
			}

			s->p = next_tag;
		}
	}
	else
	{
		uint32 certcount;

		DEBUG_RDP5("We're going for the RDP5-style encryption\n");
		in_uint32_le(s, certcount);	/* Number of certificates */
		if (certcount < 2)
		{
			ui_error(sec->rdp->inst, "Server didn't send enough X509 certificates\n");
			return False;
		}
		for (; certcount > 2; certcount--)
		{		/* ignore all the certificates between the root and the signing CA */
			uint32 ignorelen;
			SSL_CERT *ignorecert;

			DEBUG_RDP5("Ignored certs left: %d\n", certcount);
			in_uint32_le(s, ignorelen);
			DEBUG_RDP5("Ignored Certificate length is %d\n", ignorelen);
			ignorecert = ssl_cert_read(s->p, ignorelen);
			in_uint8s(s, ignorelen);
			if (ignorecert == NULL)
			{	/* XXX: error out? */
				DEBUG_RDP5("got a bad cert: this will probably screw up the rest of the communication\n");
			}

#ifdef WITH_DEBUG_RDP5
			DEBUG_RDP5("cert #%d (ignored):\n", certcount);
			ssl_cert_print_fp(stdout, ignorecert);
#endif
		}
		/* Do da funky X.509 stuffy

		   "How did I find out about this?  I looked up and saw a
		   bright light and when I came to I had a scar on my forehead
		   and knew about X.500"
		   - Peter Gutman in a early version of
		   http://www.cs.auckland.ac.nz/~pgut001/pubs/x509guide.txt
		 */
		in_uint32_le(s, cacert_len);
		DEBUG_RDP5("CA Certificate length is %d\n", cacert_len);
		cacert = ssl_cert_read(s->p, cacert_len);
		in_uint8s(s, cacert_len);
		if (NULL == cacert)
		{
			ui_error(sec->rdp->inst, "Couldn't load CA Certificate from server\n");
			return False;
		}
		in_uint32_le(s, cert_len);
		DEBUG_RDP5("Certificate length is %d\n", cert_len);
		server_cert = ssl_cert_read(s->p, cert_len);
		in_uint8s(s, cert_len);
		if (NULL == server_cert)
		{
			ssl_cert_free(cacert);
			ui_error(sec->rdp->inst, "Couldn't load Certificate from server\n");
			return False;
		}
		if (!ssl_certs_ok(server_cert, cacert))
		{
			ssl_cert_free(server_cert);
			ssl_cert_free(cacert);
			ui_error(sec->rdp->inst, "Security error CA Certificate invalid\n");
			return False;
		}
		ssl_cert_free(cacert);
		in_uint8s(s, 16);	/* Padding */
		server_public_key = ssl_cert_to_rkey(server_cert, &(sec->server_public_key_len));
		if (NULL == server_public_key)
		{
			DEBUG_RDP5("Didn't parse X509 correctly\n");
			ssl_cert_free(server_cert);
			return False;
		}
		ssl_cert_free(server_cert);
		if ((sec->server_public_key_len < SEC_MODULUS_SIZE) ||
		    (sec->server_public_key_len > SEC_MAX_MODULUS_SIZE))
		{
			ui_error(sec->rdp->inst, "Bad server public key size (%u bits)\n",
			         sec->server_public_key_len * 8);
			ssl_rkey_free(server_public_key);
			return False;
		}
		if (ssl_rkey_get_exp_mod(server_public_key, exponent, SEC_EXPONENT_SIZE,
					 modulus, SEC_MAX_MODULUS_SIZE) != 0)
		{
			ui_error(sec->rdp->inst, "Problem extracting RSA exponent, modulus");
			ssl_rkey_free(server_public_key);
			return False;
		}
		ssl_rkey_free(server_public_key);
		return True;	/* There's some garbage here we don't care about */
	}
	return s_check_end(s);
}

/* Process crypto information blob */
static void
sec_process_crypt_info(rdpSec * sec, STREAM s)
{
	uint8 *server_random = NULL;
	uint8 client_random[SEC_RANDOM_SIZE];
	uint8 modulus[SEC_MAX_MODULUS_SIZE];
	uint8 exponent[SEC_EXPONENT_SIZE];
	uint32 rc4_key_size;

	memset(modulus, 0, sizeof(modulus));
	memset(exponent, 0, sizeof(exponent));
	if (!sec_parse_crypt_info(sec, s, &rc4_key_size, &server_random, modulus, exponent))
	{
		DEBUG("Failed to parse crypt info\n");
		return;
	}
	DEBUG("Generating client random\n");
	generate_random(client_random);
	sec_rsa_encrypt(sec->sec_crypted_random, client_random, SEC_RANDOM_SIZE,
			sec->server_public_key_len, modulus, exponent);
	sec_generate_keys(sec, client_random, server_random, rc4_key_size);
}


/* Process SRV_INFO, find RDP version supported by server */
static void
sec_process_srv_info(rdpSec * sec, STREAM s)
{
	in_uint16_le(s, sec->server_rdp_version);
	DEBUG_RDP5("Server RDP version is %d\n", sec->server_rdp_version);

	if(sec->server_rdp_version == 1)
	{
		sec->rdp->settings->rdp_version = 4;
		sec->rdp->settings->server_depth = 8;
	}
        else if(sec->server_rdp_version == 4)
	{
		sec->rdp->settings->rdp_version = 5;
	}
        else
        {
                sec->rdp->settings->rdp_version = 5;
        }
}


/* Process connect response data blob */
void
sec_process_mcs_data(rdpSec * sec, STREAM s)
{
	uint16 tag, length;
	uint8 *next_tag;
	uint8 len;

	in_uint8s(s, 21);	/* header (T.124 ConferenceCreateResponse) */
	in_uint8(s, len);
	if (len & 0x80)
		in_uint8(s, len);

	while (s->p < s->end)
	{
		in_uint16_le(s, tag);
		in_uint16_le(s, length);

		if (length <= 4)
			return;

		next_tag = s->p + length - 4;

		switch (tag)
		{
			case SEC_TAG_SRV_INFO:
				sec_process_srv_info(sec, s);
				break;

			case SEC_TAG_SRV_CRYPT:
				sec_process_crypt_info(sec, s);
				break;

			case SEC_TAG_SRV_CHANNELS:
				/* FIXME: We should parse this information and
				   use it to map RDP5 channels to MCS
				   channels */
				break;

			default:
				ui_unimpl(NULL, "response tag 0x%x\n", tag);
		}

		s->p = next_tag;
	}
}

/* Receive secure transport packet */
STREAM
sec_recv(rdpSec * sec, secRecvType * type)
{
	uint8 rdpver;
	uint32 sec_flags;
	uint16 channel;
	STREAM s;

	while ((s = mcs_recv(sec->mcs, &channel, &rdpver)) != NULL)
	{
		if (rdpver != 3)
		{
			*type = SEC_RECV_FAST_PATH;
			if (rdpver & 0x80)
			{
				in_uint8s(s, 8);	/* dataSignature */
				sec_decrypt(sec, s->p, s->end - s->p);
			}
			return s;
		}
		if (sec->rdp->settings->encryption || !(sec->licence->licence_issued))
		{
			/* basicSecurityHeader: */
			in_uint32_le(s, sec_flags);

			if ((sec_flags & SEC_ENCRYPT) || (sec_flags & SEC_REDIRECTION_PKT))
			{
				in_uint8s(s, 8);	/* dataSignature */
				sec_decrypt(sec, s->p, s->end - s->p);
			}

			if (sec_flags & SEC_LICENCE_NEG)
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
			channel_process(sec->mcs->chan, s, channel);
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
	struct stream mcs_data;
	mcs_data.size = 512;
	mcs_data.p = mcs_data.data = (uint8 *) xmalloc(mcs_data.size);
	sec_out_mcs_data(sec, &mcs_data);
	
	/* sec->nla = 1; */

	if (!iso_connect(sec->mcs->iso, server, username, port))
		return False;

	if(sec->nla)
	{
		/* TLS with NLA was successfully negotiated */

		tls_connect(sec->connection, sec->mcs->iso->tcp->sock, server);

		ntlm_send_negotiate_message(sec);
	}
	else
	{
		/* We exchange some RDP data during the MCS-Connect */

		if (!mcs_connect(sec->mcs, server, &mcs_data, username, port))
			return False;

		/*      sec_process_mcs_data(&mcs_data); */
		if (sec->rdp->settings->encryption)
			sec_establish_key(sec);
		
		xfree(mcs_data.data);
	}
	
	return True;
}

/* Establish a secure connection */
RD_BOOL
sec_reconnect(rdpSec * sec, char *server, int port)
{
	struct stream mcs_data;

	/* We exchange some RDP data during the MCS-Connect */
	mcs_data.size = 512;
	mcs_data.p = mcs_data.data = (uint8 *) xmalloc(mcs_data.size);
	sec_out_mcs_data(sec, &mcs_data);

	if (!mcs_reconnect(sec->mcs, server, &mcs_data, port))
		return False;

	/*      sec_process_mcs_data(&mcs_data); */
	if (sec->rdp->settings->encryption)
		sec_establish_key(sec);
	xfree(mcs_data.data);
	return True;
}

/* Disconnect a connection */
void
sec_disconnect(rdpSec * sec)
{
	mcs_disconnect(sec->mcs);
}

/* reset the state of the sec layer */
void
sec_reset_state(rdpSec * sec)
{
	sec->server_rdp_version = 0;
	sec->sec_encrypt_use_count = 0;
	sec->sec_decrypt_use_count = 0;
	mcs_reset_state(sec->mcs);
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
		self->connection = NULL;
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
		xfree(sec);
	}
}
