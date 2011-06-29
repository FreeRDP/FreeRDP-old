/*
   FreeRDP: A Remote Desktop Protocol client.
   Standard RDP Security

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

#include "security.h"

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
void
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
		if (!(sec->net->license->license_issued))
			hdrlen = (flags & SEC_ENCRYPT) ? 12 : 4;
		else
			hdrlen = (flags & SEC_ENCRYPT) ? 12 : 0;
	}
	else
		hdrlen = 0;

	s = mcs_init(sec->net->mcs, maxlen + hdrlen);
	s_push_layer(s, sec_hdr, hdrlen);

	return s;
}

/* Initialize fast path secure transport packet */
STREAM
sec_fp_init(rdpSec * sec, uint32 flags, int maxlen)
{
	STREAM s;
	int hdrlen;

	hdrlen = (flags & SEC_ENCRYPT) ? 8 : 0;
	s = mcs_fp_init(sec->net->mcs, maxlen + hdrlen);
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
		if (!(sec->net->license->license_issued) || (flags & SEC_ENCRYPT))
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

	mcs_send_to_channel(sec->net->mcs, s, channel);
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
	mcs_fp_send(sec->net->mcs, s, flags);
}

/* Transfer the client random to the server */
void
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

void
sec_reverse_copy(uint8 * out, uint8 * in, int len)
{
	int i;
	in += len;
	for (i = 0; i < len; i++)
	{
		*out++ = *--in;
	}
}

/* Parse a Server Proprietary Certificate RSA Public Key */
RD_BOOL
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
	sec_reverse_copy(exponent, s->p, SEC_EXPONENT_SIZE);
	in_uint8s(s, SEC_EXPONENT_SIZE);
	sec_reverse_copy(modulus, s->p, modulus_len);
	in_uint8s(s, modulus_len);
	in_uint8s(s, SEC_PADDING_SIZE);	/* zero padding - included in modulus_len but not in modulus_bits */
	sec->server_public_key_len = modulus_len;

	return s_check(s);
}

/* Parse a Proprietary Certificate signature */
RD_BOOL
sec_parse_public_sig(STREAM s, uint32 len)
{
	/* The Proprietary Certificate signature uses a static published private key.
	 * That is completely nonsense, so we won't bother checking it. */

	in_uint8s(s, len);
	return len == 72;
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

	while ((s = mcs_recv(sec->net->mcs, &iso_type, &channel)) != NULL)
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
		if (sec->rdp->settings->encryption || !sec->net->license->license_issued)
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
				license_process(sec->net->license, s);
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
			vchan_process(sec->net->mcs->chan, s, channel);
			*type = SEC_RECV_IOCHANNEL;
			return s;
		}
		*type = SEC_RECV_SHARE_CONTROL;
		return s;
	}

	return NULL;
}

/* Disconnect a connection */
void
sec_disconnect(rdpSec * sec)
{
	mcs_disconnect(sec->net->mcs);

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
		self->rc4_decrypt_key = NULL;
		self->rc4_encrypt_key = NULL;
		self->net = rdp->net;
	}
	return self;
}

void
sec_free(rdpSec * sec)
{
	if (sec != NULL)
	{
		license_free(sec->net->license);
		mcs_free(sec->net->mcs);
		xfree(sec);
	}
}
