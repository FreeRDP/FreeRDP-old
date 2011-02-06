/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   NT LAN Manager Security Support Provider (NTLMSSP)

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2011

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

#include "mem.h"

#include <time.h>
#include <openssl/des.h>
#include <openssl/md4.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/engine.h>

#include "ntlmssp.h"

#define NTLMSSP_NEGOTIATE_56				0x00000001 /* 0 */
#define NTLMSSP_NEGOTIATE_KEY_EXCH			0x00000002 /* 1 */
#define NTLMSSP_NEGOTIATE_128				0x00000004 /* 2 */
#define NTLMSSP_NEGOTIATE_VERSION			0x00000040 /* 6 */
#define NTLMSSP_NEGOTIATE_TARGET_INFO			0x00000100 /* 8 */
#define NTLMSSP_REQUEST_NON_NT_SESSION_KEY		0x00000200 /* 9 */
#define NTLMSSP_NEGOTIATE_IDENTIFY			0x00000800 /* 11 */
#define NTLMSSP_NEGOTIATE_EXTENDED_SESSION_SECURITY	0x00001000 /* 12 */
#define NTLMSSP_TARGET_TYPE_SERVER			0x00004000 /* 14 */
#define NTLMSSP_TARGET_TYPE_DOMAIN			0x00008000 /* 15 */
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN			0x00010000 /* 16 */
#define NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED	0x00040000 /* 18 */
#define NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED		0x00080000 /* 19 */
#define NTLMSSP_NEGOTIATE_NTLM				0x00400000 /* 22 */
#define NTLMSSP_NEGOTIATE_LM_KEY			0x01000000 /* 24 */
#define NTLMSSP_NEGOTIATE_DATAGRAM			0x02000000 /* 25 */
#define NTLMSSP_NEGOTIATE_SEAL				0x04000000 /* 26 */
#define NTLMSSP_NEGOTIATE_SIGN				0x08000000 /* 27 */
#define NTLMSSP_REQUEST_TARGET				0x20000000 /* 29 */
#define NTLMSSP_NEGOTIATE_OEM				0x40000000 /* 30 */
#define NTLMSSP_NEGOTIATE_UNICODE			0x80000000 /* 31 */

const char ntlm_signature[] = "NTLMSSP";
const char lm_magic[] = "KGS!@#$%";

const char client_sign_magic[] = "session key to client-to-server signing key magic constant";
const char server_sign_magic[] = "session key to server-to-client signing key magic constant";
const char client_seal_magic[] = "session key to client-to-server sealing key magic constant";
const char server_seal_magic[] = "session key to server-to-client sealing key magic constant";

void ntlmssp_set_username(NTLMSSP *ntlmssp, char* username)
{
	data_blob_free(&ntlmssp->username);

	if (username != NULL)
	{
		int length = strlen((char*) username);
		data_blob_alloc(&ntlmssp->username, length * 2);
		credssp_str_to_wstr(username, ntlmssp->username.data, length);
	}
}

void ntlmssp_set_domain(NTLMSSP *ntlmssp, char* domain)
{
	data_blob_free(&ntlmssp->domain);

	if (domain != NULL)
	{
		int length = strlen((char*) domain);
		data_blob_alloc(&ntlmssp->domain, length * 2);
		credssp_str_to_wstr(domain, ntlmssp->domain.data, length);
	}
}

void ntlmssp_set_password(NTLMSSP *ntlmssp, char* password)
{
	data_blob_free(&ntlmssp->password);

	if (password != NULL)
	{
		int length = strlen((char*) password);
		data_blob_alloc(&ntlmssp->password, length * 2);
		credssp_str_to_wstr(password, ntlmssp->password.data, length);
	}
}

void ntlmssp_generate_client_challenge(NTLMSSP *ntlmssp)
{
	/* ClientChallenge in computation of LMv2 and NTLMv2 responses */
	credssp_nonce(ntlmssp->client_challenge, 8);
}

void ntlmssp_generate_key_exchange_key(NTLMSSP *ntlmssp)
{
	/* In NTLMv2, KeyExchangeKey is the 128-bit SessionBaseKey */
	memcpy(ntlmssp->key_exchange_key, ntlmssp->session_base_key, 16);
}

void ntlmssp_generate_random_session_key(NTLMSSP *ntlmssp)
{
	credssp_nonce(ntlmssp->random_session_key, 16);
}

void ntlmssp_generate_exported_session_key(NTLMSSP *ntlmssp)
{
	//credssp_nonce(ntlmssp->exported_session_key, 16);
	memcpy(ntlmssp->exported_session_key, ntlmssp->random_session_key, 16);
}

void ntlmssp_encrypt_random_session_key(NTLMSSP *ntlmssp)
{
	/* In NTLMv2, EncryptedRandomSessionKey is the ExportedSessionKey RC4-encrypted with the KeyExchangeKey */
	credssp_rc4k(ntlmssp->key_exchange_key, 16, ntlmssp->random_session_key, ntlmssp->encrypted_random_session_key);
}

void ntlmssp_generate_timestamp(NTLMSSP *ntlmssp)
{
	/* Get the current timestamp */
	credssp_current_time(ntlmssp->timestamp);
}

void ntlmssp_generate_signing_key(uint8* exported_session_key, DATA_BLOB *sign_magic, uint8* signing_key)
{
	int length;
	uint8* value;
	CryptoMd5 md5;

	length = 16 + sign_magic->length;
	value = (uint8*) xmalloc(length);

	/* Concatenate ExportedSessionKey with sign magic */
	memcpy(value, exported_session_key, 16);
	memcpy(&value[16], sign_magic->data, sign_magic->length);

	md5 = crypto_md5_init();
	crypto_md5_update(md5, value, length);
	crypto_md5_final(md5, signing_key);

	xfree(value);
}

void ntlmssp_generate_client_signing_key(NTLMSSP *ntlmssp)
{
	DATA_BLOB sign_magic;
	sign_magic.data = (void*) client_sign_magic;
	sign_magic.length = sizeof(client_sign_magic);
	ntlmssp_generate_signing_key(ntlmssp->exported_session_key, &sign_magic, ntlmssp->client_signing_key);
}

void ntlmssp_generate_sealing_key(uint8* exported_session_key, DATA_BLOB *seal_magic, uint8* sealing_key)
{
	uint8* p;
	CryptoMd5 md5;
	DATA_BLOB blob;

	data_blob_alloc(&blob, 16 + seal_magic->length);
	p = (uint8*) blob.data;

	/* Concatenate ExportedSessionKey with seal magic */
	memcpy(p, exported_session_key, 16);
	memcpy(&p[16], seal_magic->data, seal_magic->length);

	md5 = crypto_md5_init();
	crypto_md5_update(md5, blob.data, blob.length);
	crypto_md5_final(md5, sealing_key);

	data_blob_free(&blob);
}

void ntlmssp_generate_client_sealing_key(NTLMSSP *ntlmssp)
{
	DATA_BLOB seal_magic;
	seal_magic.data = (void*) client_seal_magic;
	seal_magic.length = sizeof(client_seal_magic);
	ntlmssp_generate_signing_key(ntlmssp->exported_session_key, &seal_magic, ntlmssp->client_sealing_key);
}

void ntlmssp_init_rc4_seal_state(NTLMSSP *ntlmssp)
{
	ntlmssp->rc4_seal = crypto_rc4_init(ntlmssp->client_sealing_key, 16);
}

static int get_bit(char* buffer, int bit)
{
	return (buffer[(bit - (bit % 8)) / 8] >> (7 - bit % 8) & 1);
}

static void set_bit(char* buffer, int bit, int value)
{
	buffer[(bit - (bit % 8)) / 8] |= value << (7 - bit % 8);
}

static void ntlmssp_compute_des_key(char* text, char* des_key)
{
	int i, j;
	int bit;
	int nbits;

	/* Convert the 7 bytes into a bit stream, and insert a parity-bit (odd parity) after every seven bits. */

	memset(des_key, '\0', 8);

	for (i = 0; i < 8; i++)
	{
		nbits = 0;

		for (j = 0; j < 7; j++)
		{
			/* copy 7 bits, and count the number of bits that are set */

			bit = get_bit(text, i*7 + j);
			set_bit(des_key, i*7 + i + j, bit);
			nbits += bit;
		}

		/* insert parity bit (odd parity) */

		if (nbits % 2 == 0)
			set_bit(des_key, i*7 + i + j, 1);
	}
}

void ntlmssp_compute_lm_hash(char* password, char* hash)
{
	int i;
	int maxlen;
	char text[14];
	char des_key1[8];
	char des_key2[8];
	des_key_schedule ks;

	/* LM("password") = E52CAC67419A9A224A3B108F3FA6CB6D */

	maxlen = (strlen(password) < 14) ? strlen(password) : 14;

	/* convert to uppercase */
	for (i = 0; i < maxlen; i++)
	{
		if ((password[i] >= 'a') && (password[i] <= 'z'))
			text[i] = password[i] - 32;
		else
			text[i] = password[i];
	}

	/* pad with nulls up to 14 bytes */
	for (i = maxlen; i < 14; i++)
		text[i] = '\0';

	ntlmssp_compute_des_key(text, des_key1);
	ntlmssp_compute_des_key(&text[7], des_key2);

	DES_set_key((const_DES_cblock*)des_key1, &ks);
	DES_ecb_encrypt((const_DES_cblock*) lm_magic, (DES_cblock*)hash, &ks, DES_ENCRYPT);

	DES_set_key((const_DES_cblock*)des_key2, &ks);
	DES_ecb_encrypt((const_DES_cblock*) lm_magic, (DES_cblock*)&hash[8], &ks, DES_ENCRYPT);
}

void ntlmssp_compute_ntlm_hash(DATA_BLOB* password, char* hash)
{
	/* NTLMv1("password") = 8846F7EAEE8FB117AD06BDD830B7586C */

	MD4_CTX md4_ctx;

	/* Password needs to be in unicode */

	/* Apply the MD4 digest algorithm on the password in unicode, the result is the NTLM hash */

	MD4_Init(&md4_ctx);
	MD4_Update(&md4_ctx, password->data, password->length);
	MD4_Final((void*) hash, &md4_ctx);
}

void ntlmssp_compute_ntlm_v2_hash(DATA_BLOB *password, DATA_BLOB *username, DATA_BLOB *domain, char* hash)
{
	int i;
	char* p;
	DATA_BLOB blob;
	char ntlm_hash[16];

	data_blob_alloc(&blob, username->length + domain->length);
	p = (char*) blob.data;

	/* First, compute the NTLMv1 hash of the password */
	ntlmssp_compute_ntlm_hash(password, ntlm_hash);

	/* Concatenate(Uppercase(username),domain)*/
	memcpy(p, username->data, username->length);
	for (i = 0; i < username->length; i += 2)
	{
		if (p[i] >= 'a' && p[i] <= 'z')
			p[i] = p[i] - 32;
	}

	memcpy(&p[username->length], domain->data, domain->length);

	/* Compute the HMAC-MD5 hash of the above value using the NTLMv1 hash as the key, the result is the NTLMv2 hash */
	HMAC(EVP_md5(), (void*) ntlm_hash, 16, blob.data, blob.length, (void*) hash, NULL);
}

void ntlmssp_compute_lm_response(char* password, char* challenge, char* response)
{
	char hash[21];
	char des_key1[8];
	char des_key2[8];
	char des_key3[8];
	des_key_schedule ks;

	/* A LM hash is 16-bytes long, but the LM response uses a LM hash null-padded to 21 bytes */
	memset(hash, '\0', 21);
	ntlmssp_compute_lm_hash(password, hash);

	/* Each 7-byte third of the 21-byte null-padded LM hash is used to create a DES key */
	ntlmssp_compute_des_key(hash, des_key1);
	ntlmssp_compute_des_key(&hash[7], des_key2);
	ntlmssp_compute_des_key(&hash[14], des_key3);

	/* Encrypt the LM challenge with each key, and concatenate the result. This is the LM response (24 bytes) */
	DES_set_key((const_DES_cblock*)des_key1, &ks);
	DES_ecb_encrypt((const_DES_cblock*)challenge, (DES_cblock*)response, &ks, DES_ENCRYPT);

	DES_set_key((const_DES_cblock*)des_key2, &ks);
	DES_ecb_encrypt((const_DES_cblock*)challenge, (DES_cblock*)&response[8], &ks, DES_ENCRYPT);

	DES_set_key((const_DES_cblock*)des_key3, &ks);
	DES_ecb_encrypt((const_DES_cblock*)challenge, (DES_cblock*)&response[16], &ks, DES_ENCRYPT);
}

void ntlmssp_compute_lm_v2_response(NTLMSSP *ntlmssp)
{
	char *response;
	char value[16];
	char ntlm_v2_hash[16];

	/* Compute the NTLMv2 hash */
	ntlmssp_compute_ntlm_v2_hash(&ntlmssp->password, &ntlmssp->username, &ntlmssp->domain, ntlm_v2_hash);

	/* Concatenate the server and client challenges */
	memcpy(value, ntlmssp->server_challenge, 8);
	memcpy(&value[8], ntlmssp->client_challenge, 8);

	data_blob_alloc(&ntlmssp->lm_challenge_response, 24);
	response = (char*) ntlmssp->lm_challenge_response.data;

	/* Compute the HMAC-MD5 hash of the resulting value using the NTLMv2 hash as the key */
	HMAC(EVP_md5(), (void*) ntlm_v2_hash, 16, (void*) value, 16, (void*) response, NULL);

	/* Concatenate the resulting HMAC-MD5 hash and the client challenge, giving us the LMv2 response (24 bytes) */
	memcpy(&response[16], ntlmssp->client_challenge, 8);
}

void ntlmssp_compute_ntlm_v2_response(NTLMSSP *ntlmssp)
{
	uint8* blob;
	uint8 ntlm_v2_hash[16];
	uint8 nt_proof_str[16];
	DATA_BLOB ntlm_v2_temp;
	DATA_BLOB ntlm_v2_temp_chal;

	data_blob_alloc(&ntlm_v2_temp, ntlmssp->target_info.length + 28);
	memset(ntlm_v2_temp.data, '\0', ntlm_v2_temp.length);
	blob = (uint8*) ntlm_v2_temp.data;

	/* Compute the NTLMv2 hash */
	ntlmssp_compute_ntlm_v2_hash(&ntlmssp->password,
		&ntlmssp->username, &ntlmssp->domain, (char*) ntlm_v2_hash);

#ifdef WITH_DEBUG_NLA
	printf("Password (length = %d)\n", ntlmssp->password.length);
	hexdump(ntlmssp->password.data, ntlmssp->password.length);
	printf("\n");

	printf("Username (length = %d)\n", ntlmssp->username.length);
	hexdump(ntlmssp->username.data, ntlmssp->username.length);
	printf("\n");

	printf("Domain (length = %d)\n", ntlmssp->domain.length);
	hexdump(ntlmssp->domain.data, ntlmssp->domain.length);
	printf("\n");

	printf("NTOWFv2, NTLMv2 Hash\n");
	hexdump(ntlm_v2_hash, 16);
	printf("\n");
#endif

	/* Construct temp */
	blob[0] = 1; /* RespType (1 byte) */
	blob[1] = 1; /* HighRespType (1 byte) */
	/* Reserved1 (2 bytes) */
	/* Reserved2 (4 bytes) */
	memcpy(&blob[8], ntlmssp->timestamp, 8); /* Timestamp (8 bytes) */
	memcpy(&blob[16], ntlmssp->client_challenge, 8); /* ClientChallenge (8 bytes) */
	/* Reserved3 (4 bytes) */
	memcpy(&blob[28], ntlmssp->target_info.data, ntlmssp->target_info.length);

#ifdef WITH_DEBUG_NLA
	printf("NTLMv2 Response Temp Blob\n");
	hexdump(ntlm_v2_temp.data, ntlm_v2_temp.length);
	printf("\n");
#endif

	/* Concatenate server challenge with temp */
	data_blob_alloc(&ntlm_v2_temp_chal, ntlm_v2_temp.length + 8);
	blob = (uint8*) ntlm_v2_temp_chal.data;
	memcpy(blob, ntlmssp->server_challenge, 8);
	memcpy(&blob[8], ntlm_v2_temp.data, ntlm_v2_temp.length);

	HMAC(EVP_md5(), (void*) ntlm_v2_hash, 16, (void*) ntlm_v2_temp_chal.data,
		ntlm_v2_temp_chal.length, (void*) nt_proof_str, NULL);

	/* NtChallengeResponse, Concatenate NTProofStr with temp */
	data_blob_alloc(&ntlmssp->nt_challenge_response, ntlm_v2_temp.length + 16);
	blob = (uint8*) ntlmssp->nt_challenge_response.data;
	memcpy(blob, nt_proof_str, 16);
	memcpy(&blob[16], ntlm_v2_temp.data, ntlm_v2_temp.length);

	/* Compute SessionBaseKey, the HMAC-MD5 hash of NTProofStr using the NTLMv2 hash as the key */
	HMAC(EVP_md5(), (void*) ntlm_v2_hash, 16,
		(void*) nt_proof_str, 16, (void*) ntlmssp->session_base_key, NULL);
}


void ntlmssp_input_negotiate_flags(STREAM s, uint32 *flags)
{
	uint8* p;
	uint8 tmp;
	uint32 negotiateFlags;

	/*
	 * NegotiateFlags is a 4-byte bit map
	 * Reverse order and then input in Big Endian
	 */

	in_uint32_be(s, negotiateFlags);

	p = (uint8*) &negotiateFlags;
	tmp = p[0];
	p[0] = p[3];
	p[3] = tmp;

	tmp = p[1];
	p[1] = p[2];
	p[2] = tmp;

	*flags = negotiateFlags;
}

void ntlmssp_output_negotiate_flags(STREAM s, uint32 flags)
{
	uint8* p;
	uint8 tmp;

	/*
	 * NegotiateFlags is a 4-byte bit map
	 * Output in Big Endian and then reverse order
	 */

	p = s->p;
	out_uint32_be(s, flags);

	tmp = p[0];
	p[0] = p[3];
	p[3] = tmp;

	tmp = p[1];
	p[1] = p[2];
	p[2] = tmp;
}

void ntlmssp_encrypt_message(NTLMSSP *ntlmssp, DATA_BLOB *msg, DATA_BLOB *encrypted_msg, uint8* signature)
{
	HMAC_CTX hmac_ctx;
	uint8 digest[16];
	uint8 checksum[8];
	uint32 version = 1;

	/* Compute the HMAC-MD5 hash of ConcatenationOf(seq_num,msg) using the client signing key */
	HMAC_CTX_init(&hmac_ctx);
	HMAC_Init_ex(&hmac_ctx, ntlmssp->client_signing_key, 16, EVP_md5(), NULL);
	HMAC_Update(&hmac_ctx, (void*) &ntlmssp->send_seq_num, 4);
	HMAC_Update(&hmac_ctx, msg->data, msg->length);
	HMAC_Final(&hmac_ctx, digest, NULL);

	/* Allocate space for encrypted message */
	data_blob_alloc(encrypted_msg, msg->length);

	/* Encrypt message using with RC4 */
	crypto_rc4(ntlmssp->rc4_seal, msg->length, msg->data, encrypted_msg->data);

	/* RC4-encrypt first 8 bytes of digest */
	crypto_rc4(ntlmssp->rc4_seal, 8, digest, checksum);

	/* Concatenate version, ciphertext and sequence number to build signature */
	memcpy(signature, (void*) &version, 4);
	memcpy(&signature[4], (void*) checksum, 8);
	memcpy(&signature[12], (void*) &(ntlmssp->send_seq_num), 4);

	ntlmssp->send_seq_num++;
}

void ntlmssp_send_negotiate_message(NTLMSSP *ntlmssp, STREAM s)
{
	int length;
	uint32 negotiateFlags = 0;

	out_uint8a(s, ntlm_signature, 8); /* Signature (8 bytes) */
	out_uint32_le(s, 1); /* MessageType */

	negotiateFlags |= NTLMSSP_NEGOTIATE_56;
	negotiateFlags |= NTLMSSP_NEGOTIATE_128;
	negotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
	negotiateFlags |= NTLMSSP_REQUEST_TARGET;
	negotiateFlags |= NTLMSSP_TARGET_TYPE_DOMAIN;
	negotiateFlags |= NTLMSSP_REQUEST_NON_NT_SESSION_KEY;
	negotiateFlags |= NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED;
	negotiateFlags |= 0x00000030;

	ntlmssp_output_negotiate_flags(s, negotiateFlags); /* NegotiateFlags (4 bytes) */

	/* only set if NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED is set */

	/* DomainNameFields (8 bytes) */
	out_uint16_le(s, 0); /* DomainNameLen */
	out_uint16_le(s, 0); /* DomainNameMaxLen */
	out_uint32_le(s, 32); /* DomainNameBufferOffset */

	/* only set if NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED is set */

	/* WorkstationFields (8 bytes) */
	out_uint16_le(s, 0); /* WorkstationLen */
	out_uint16_le(s, 0); /* WorkstationMaxLen */
	out_uint32_le(s, 32); /* WorkstationBufferOffset */

	s_mark_end(s);

	length = s->end - s->data;

#ifdef WITH_DEBUG_NLA
	printf("NEGOTIATE_MESSAGE (length = %d)\n", length);
	hexdump(s->data, length);
	printf("\n");
#endif

	ntlmssp->state = NTLMSSP_STATE_CHALLENGE;
}

void ntlmssp_recv_challenge_message(NTLMSSP *ntlmssp, STREAM s)
{
	uint8* p;
	int length;
	uint8* start_offset;
	uint16 targetNameLen;
	uint16 targetNameMaxLen;
	uint32 targetNameBufferOffset;
	uint16 targetInfoLen;
	uint16 targetInfoMaxLen;
	uint32 targetInfoBufferOffset;

	start_offset = s->p - 12;

	/* TargetNameFields (8 bytes) */
	in_uint16_le(s, targetNameLen); /* TargetNameLen (2 bytes) */
	in_uint16_le(s, targetNameMaxLen); /* TargetNameMaxLen (2 bytes) */
	in_uint32_le(s, targetNameBufferOffset); /* TargetNameBufferOffset (4 bytes) */

	ntlmssp_input_negotiate_flags(s, &(ntlmssp->negotiate_flags)); /* NegotiateFlags (4 bytes) */

	in_uint8a(s, ntlmssp->server_challenge, 8); /* ServerChallenge (8 bytes) */
	in_uint8s(s, 8); /* Reserved (8 bytes), should be ignored */

	/* TargetInfoFields (8 bytes) */
	in_uint16_le(s, targetInfoLen); /* TargetInfoLen (2 bytes) */
	in_uint16_le(s, targetInfoMaxLen); /* TargetInfoMaxLen (2 bytes) */
	in_uint32_le(s, targetInfoBufferOffset); /* TargetInfoBufferOffset (4 bytes) */

	/* only present if NTLMSSP_NEGOTIATE_VERSION is set */

	if (ntlmssp->negotiate_flags & NTLMSSP_NEGOTIATE_VERSION)
	{
		in_uint8s(s, 8); /* Version (8 bytes), can be ignored */
	}

	/* Payload (variable) */

	if (targetNameLen > 0)
	{
		p = start_offset + targetNameBufferOffset;
		data_blob_alloc(&ntlmssp->target_name, targetNameLen);
		memcpy(ntlmssp->target_name.data, p, targetNameLen);

#ifdef WITH_DEBUG_NLA
		printf("targetName (length = %d, offset = %d)\n", targetNameLen, targetNameBufferOffset);
		hexdump(ntlmssp->target_name.data, ntlmssp->target_name.length);
		printf("\n");
#endif
	}

	if (targetInfoLen > 0)
	{
		p = start_offset + targetInfoBufferOffset;
		data_blob_alloc(&ntlmssp->target_info, targetInfoLen);
		memcpy(ntlmssp->target_info.data, p, targetInfoLen);

#ifdef WITH_DEBUG_NLA
		printf("targetInfo (length = %d, offset = %d)\n", targetInfoLen, targetInfoBufferOffset);
		hexdump(ntlmssp->target_info.data, ntlmssp->target_info.length);
		printf("\n");
#endif
	}

	p = s->p + targetNameLen + targetInfoLen;
	length = p - start_offset;

#ifdef WITH_DEBUG_NLA
	printf("CHALLENGE_MESSAGE (length = %d)\n", length);
	hexdump(start_offset, length);
	printf("\n");
#endif

	/* Timestamp */
	ntlmssp_generate_timestamp(ntlmssp);

	/* LmChallengeResponse */
	ntlmssp_compute_lm_v2_response(ntlmssp);

	/* NtChallengeResponse */
	ntlmssp_compute_ntlm_v2_response(ntlmssp);

	/* KeyExchangeKey */
	ntlmssp_generate_key_exchange_key(ntlmssp);

	/* EncryptedRandomSessionKey */
	ntlmssp_encrypt_random_session_key(ntlmssp);

	/* Generate client signing key */
	ntlmssp_generate_client_signing_key(ntlmssp);

	/* Generate client sealing key */
	ntlmssp_generate_client_sealing_key(ntlmssp);

	/* Initialize RC4 seal state using client sealing key */
	ntlmssp_init_rc4_seal_state(ntlmssp);

#ifdef WITH_DEBUG_NLA
	printf("ClientChallenge\n");
	hexdump(ntlmssp->client_challenge, 8);
	printf("\n");

	printf("ServerChallenge\n");
	hexdump(ntlmssp->server_challenge, 8);
	printf("\n");

	printf("SessionBaseKey\n");
	hexdump(ntlmssp->session_base_key, 16);
	printf("\n");

	printf("KeyExchangeKey\n");
	hexdump(ntlmssp->key_exchange_key, 16);
	printf("\n");

	printf("ExportedSessionKey\n");
	hexdump(ntlmssp->exported_session_key, 16);
	printf("\n");

	printf("RandomSessionKey\n");
	hexdump(ntlmssp->random_session_key, 16);
	printf("\n");

	printf("ClientSignKey\n");
	hexdump(ntlmssp->client_signing_key, 16);
	printf("\n");

	printf("ClientSealingKey\n");
	hexdump(ntlmssp->client_sealing_key, 16);
	printf("\n");

	printf("Timestamp\n");
	hexdump(ntlmssp->timestamp, 8);
	printf("\n");
#endif

	ntlmssp->state = NTLMSSP_STATE_AUTHENTICATE;
}

void ntlmssp_send_authenticate_message(NTLMSSP *ntlmssp, STREAM s)
{
	int length;
	uint32 negotiateFlags;

	uint16 DomainNameLen;
	uint16 UserNameLen;
	uint16 WorkstationLen;
	uint16 LmChallengeResponseLen;
	uint16 NtChallengeResponseLen;
	uint16 EncryptedRandomSessionKeyLen;

	uint32 DomainNameBufferOffset;
	uint32 UserNameBufferOffset;
	uint32 WorkstationBufferOffset;
	uint32 LmChallengeResponseBufferOffset;
	uint32 NtChallengeResponseBufferOffset;
	uint32 EncryptedRandomSessionKeyBufferOffset;

	uint8* UserNameBuffer;
	uint8* DomainNameBuffer;
	uint8* EncryptedRandomSessionKeyBuffer;

	WorkstationLen = 0;

	DomainNameLen = ntlmssp->domain.length;
	DomainNameBuffer = ntlmssp->domain.data;

	UserNameLen = ntlmssp->username.length;
	UserNameBuffer = ntlmssp->username.data;

	LmChallengeResponseLen = ntlmssp->lm_challenge_response.length;
	NtChallengeResponseLen = ntlmssp->nt_challenge_response.length;

	EncryptedRandomSessionKeyLen = 16;
	EncryptedRandomSessionKeyBuffer = ntlmssp->encrypted_random_session_key;

	LmChallengeResponseBufferOffset = 64; /* starting buffer offset */
	NtChallengeResponseBufferOffset = LmChallengeResponseBufferOffset + LmChallengeResponseLen;
	DomainNameBufferOffset = NtChallengeResponseBufferOffset + NtChallengeResponseLen;
	UserNameBufferOffset = DomainNameBufferOffset + DomainNameLen;
	WorkstationBufferOffset = UserNameBufferOffset + UserNameLen;
	EncryptedRandomSessionKeyBufferOffset = WorkstationBufferOffset + WorkstationLen;

	out_uint8a(s, ntlm_signature, 8); /* Signature (8 bytes) */
	out_uint32_le(s, 3); /* MessageType */

	/* LmChallengeResponseFields (8 bytes) */
	out_uint16_le(s, LmChallengeResponseLen); /* LmChallengeResponseLen */
	out_uint16_le(s, LmChallengeResponseLen); /* LmChallengeResponseMaxLen */
	out_uint32_le(s, LmChallengeResponseBufferOffset); /* LmChallengeResponseBufferOffset */

	/* NtChallengeResponseFields (8 bytes) */
	out_uint16_le(s, NtChallengeResponseLen); /* NtChallengeResponseLen */
	out_uint16_le(s, NtChallengeResponseLen); /* NtChallengeResponseMaxLen */
	out_uint32_le(s, NtChallengeResponseBufferOffset); /* NtChallengeResponseBufferOffset */

	/* only set if NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED is set */

	/* DomainNameFields (8 bytes) */
	out_uint16_le(s, DomainNameLen); /* DomainNameLen */
	out_uint16_le(s, DomainNameLen); /* DomainNameMaxLen */
	out_uint32_le(s, DomainNameBufferOffset); /* DomainNameBufferOffset */

	/* UserNameFields (8 bytes) */
	out_uint16_le(s, UserNameLen); /* UserNameLen */
	out_uint16_le(s, UserNameLen); /* UserNameMaxLen */
	out_uint32_le(s, UserNameBufferOffset); /* UserNameBufferOffset */

	/* only set if NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED is set */

	/* WorkstationFields (8 bytes) */
	out_uint16_le(s, WorkstationLen); /* WorkstationLen */
	out_uint16_le(s, WorkstationLen); /* WorkstationMaxLen */
	out_uint32_le(s, WorkstationBufferOffset); /* WorkstationBufferOffset */

	/* EncryptedRandomSessionKeyFields (8 bytes) */
	out_uint16_le(s, EncryptedRandomSessionKeyLen); /* EncryptedRandomSessionKeyLen */
	out_uint16_le(s, EncryptedRandomSessionKeyLen); /* EncryptedRandomSessionKeyMaxLen */
	out_uint32_le(s, EncryptedRandomSessionKeyBufferOffset); /* EncryptedRandomSessionKeyBufferOffset */

	negotiateFlags = 0;
	negotiateFlags |= NTLMSSP_NEGOTIATE_56;
	negotiateFlags |= NTLMSSP_NEGOTIATE_128;
	negotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
	negotiateFlags |= NTLMSSP_REQUEST_TARGET;
	negotiateFlags |= NTLMSSP_TARGET_TYPE_DOMAIN;
	negotiateFlags |= NTLMSSP_REQUEST_NON_NT_SESSION_KEY;
	negotiateFlags |= NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED;
	negotiateFlags |= 0x00000030;

	ntlmssp_output_negotiate_flags(s, negotiateFlags); /* NegotiateFlags (4 bytes) */

	/* Payload (offset 64) */

	/* LmChallengeResponse */
	out_uint8p(s, ntlmssp->lm_challenge_response.data, LmChallengeResponseLen);

#ifdef WITH_DEBUG_NLA
	printf("LmChallengeResponse (length = %d, offset = %d)\n", LmChallengeResponseLen, LmChallengeResponseBufferOffset);
	hexdump(ntlmssp->lm_challenge_response.data, LmChallengeResponseLen);
	printf("\n");
#endif

	/* NtChallengeResponse */
	out_uint8p(s, ntlmssp->nt_challenge_response.data, NtChallengeResponseLen);

#ifdef WITH_DEBUG_NLA
	printf("NtChallengeResponse (length = %d, offset = %d)\n", NtChallengeResponseLen, NtChallengeResponseBufferOffset);
	hexdump(ntlmssp->nt_challenge_response.data, NtChallengeResponseLen);
	printf("\n");
#endif

	/* DomainName */
	if (DomainNameLen > 0)
	{
#ifdef WITH_DEBUG_NLA
		printf("DomainName (length = %d, offset = %d)\n", DomainNameLen, DomainNameBufferOffset);
		hexdump(DomainNameBuffer, DomainNameLen);
		printf("\n");
#endif
	}

	/* UserName */
	out_uint8p(s, UserNameBuffer, UserNameLen);

#ifdef WITH_DEBUG_NLA
	printf("UserName (length = %d, offset = %d)\n", UserNameLen, UserNameBufferOffset);
	hexdump(UserNameBuffer, UserNameLen);
	printf("\n");
#endif

	/* EncryptedRandomSessionKey */
	out_uint8p(s, EncryptedRandomSessionKeyBuffer, EncryptedRandomSessionKeyLen);
	s_mark_end(s);

#ifdef WITH_DEBUG_NLA
	printf("EncryptedRandomSessionKey (length = %d, offset = %d)\n", EncryptedRandomSessionKeyLen, EncryptedRandomSessionKeyBufferOffset);
	hexdump(EncryptedRandomSessionKeyBuffer, EncryptedRandomSessionKeyLen);
	printf("\n");
#endif

	length = s->end - s->data;

#ifdef WITH_DEBUG_NLA
	printf("AUTHENTICATE_MESSAGE (length = %d)\n", length);
	hexdump(s->data, length);
	printf("\n");
#endif

	ntlmssp->state = NTLMSSP_STATE_FINAL;
}

int ntlmssp_send(NTLMSSP *ntlmssp, STREAM s)
{
	if (ntlmssp->state == NTLMSSP_STATE_INITIAL)
		ntlmssp->state = NTLMSSP_STATE_NEGOTIATE;

	if (ntlmssp->state == NTLMSSP_STATE_NEGOTIATE)
		ntlmssp_send_negotiate_message(ntlmssp, s);
	else if (ntlmssp->state == NTLMSSP_STATE_AUTHENTICATE)
		ntlmssp_send_authenticate_message(ntlmssp, s);

	return (ntlmssp->state == NTLMSSP_STATE_FINAL) ? 0 : 1;
}

int ntlmssp_recv(NTLMSSP *ntlmssp, STREAM s)
{
	char signature[8]; /* Signature, "NTLMSSP" */
	uint32 messageType; /* MessageType */

	in_uint8a(s, signature, 8);
	in_uint32_le(s, messageType);

	if (messageType == 2 && ntlmssp->state == NTLMSSP_STATE_CHALLENGE)
		ntlmssp_recv_challenge_message(ntlmssp, s);

	return 1;
}

NTLMSSP* ntlmssp_new()
{
	/* Create new NTLMSSP state machine instance */
	
	NTLMSSP *ntlmssp = (NTLMSSP*) xmalloc(sizeof(NTLMSSP));

	if (ntlmssp != NULL)
	{
		memset(ntlmssp, '\0', sizeof(NTLMSSP));
		ntlmssp_init(ntlmssp);
	}

	return ntlmssp;
}

void ntlmssp_init(NTLMSSP *ntlmssp)
{
	/* Initialize NTLMSSP state machine */
	ntlmssp->state = NTLMSSP_STATE_INITIAL;
}

void ntlmssp_free(NTLMSSP *ntlmssp)
{
	/* Free NTLMSSP state machine */
	xfree(ntlmssp);
}
