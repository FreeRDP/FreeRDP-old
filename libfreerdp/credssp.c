/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Credential Security Support Provider (CredSSP)

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

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

#include <freerdp/rdpset.h>
#include "frdp.h"
#include "rdp.h"
#include "tls.h"
#include "asn1.h"
#include "secure.h"
#include "stream.h"
#include "mem.h"
#include "tcp.h"
#include "mcs.h"
#include "iso.h"
#ifndef _WIN32
#include <unistd.h>
#endif

#include "asn1/TSRequest.h"
#include "asn1/NegoData.h"
#include "asn1/NegotiationToken.h"

#include <openssl/des.h>
#include <openssl/md4.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <time.h>

#include "credssp.h"

#define NTLMSSP_NEGOTIATE_56				0x00000001 /* 0 */
#define NTLMSSP_NEGOTIATE_KEY_EXCH		0x00000002 /* 1 */
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
#define NTLMSSP_NEGOTIATE_OEM					0x40000000 /* 30 */
#define NTLMSSP_NEGOTIATE_UNICODE			0x80000000 /* 31 */

#define WINDOWS_MAJOR_VERSION_5		0x05
#define WINDOWS_MAJOR_VERSION_6		0x06
#define WINDOWS_MINOR_VERSION_0		0x00
#define WINDOWS_MINOR_VERSION_1		0x01
#define WINDOWS_MINOR_VERSION_2		0x02
#define NTLMSSP_REVISION_W2K3		0x0F

const char ntlm_signature[] = "NTLMSSP";
const char lm_magic[] = "KGS!@#$%";

const char client_sign_magic[] = "session key to client-to-server signing key magic constant";
const char server_sign_magic[] = "session key to server-to-client signing key magic constant";
const char client_seal_magic[] = "session key to client-to-server sealing key magic constant";
const char server_seal_magic[] = "session key to server-to-client sealing key magic constant";

/* http://davenport.sourceforge.net/ntlm.html is a really nice source of information with great samples */

static int
asn1_write(const void *buffer, size_t size, void *fd)
{
	/* this is used to get the size of the ASN.1 encoded result */
	return 0;
}

int credssp_authenticate(rdpSec * sec)
{
	if (sec->nla->state != NTLM_STATE_INITIAL)
		return 0;

	ntlm_send_negotiate_message(sec);

	if (sec->nla->state != NTLM_STATE_CHALLENGE)
		return 0;

	credssp_recv(sec);

	if (sec->nla->state != NTLM_STATE_AUTHENTICATE)
		return 0;

	ntlm_send_authenticate_message(sec);

	if (sec->nla->state != NTLM_STATE_FINAL)
		return 0;

	credssp_recv(sec);

	return 1;
}

void credssp_send(rdpSec * sec, STREAM negoToken, STREAM pubKeyAuth)
{
	TSRequest_t *ts_request;
	OCTET_STRING_t *nego_token;
	NegotiationToken_t *negotiation_token;
	asn_enc_rval_t enc_rval;

	char* buffer;
	size_t size;

	ts_request = calloc(1, sizeof(TSRequest_t));
	ts_request->negoTokens = calloc(1, sizeof(NegoData_t));
	nego_token = calloc(1, sizeof(OCTET_STRING_t));
	negotiation_token = calloc(1, sizeof(NegotiationToken_t));

	ts_request->version = 2;

	nego_token->buf = negoToken->data;
	nego_token->size = negoToken->end - negoToken->data;

	ASN_SEQUENCE_ADD(ts_request->negoTokens, nego_token);

	if (pubKeyAuth != NULL)
	{
		ts_request->pubKeyAuth = calloc(1, sizeof(OCTET_STRING_t));

		ts_request->pubKeyAuth->buf = pubKeyAuth->data;
		ts_request->pubKeyAuth->size = pubKeyAuth->end - pubKeyAuth->data;
	}

	/* get size of the encoded ASN.1 payload */
	enc_rval = der_encode(&asn_DEF_TSRequest, ts_request, asn1_write, 0);

	if (enc_rval.encoded != -1)
	{
		size = enc_rval.encoded;
		buffer = xmalloc(size);

		enc_rval = der_encode_to_buffer(&asn_DEF_TSRequest, ts_request, buffer, size);

		if (enc_rval.encoded != -1)
		{
			tls_write(sec->ssl, buffer, size);
		}

		asn_DEF_TSRequest.free_struct(&asn_DEF_TSRequest, ts_request, 0);
		xfree(buffer);
	}
}

void credssp_recv(rdpSec * sec)
{
	TSRequest_t *ts_request = 0;
	NegotiationToken_t *negotiation_token = 0;
	asn_dec_rval_t dec_rval;

	int size = 2048;
	int bytes_read;
	char* recv_buffer;

	recv_buffer = xmalloc(size);

	bytes_read = tls_read(sec->ssl, recv_buffer, size);

	dec_rval = ber_decode(0, &asn_DEF_TSRequest, (void **)&ts_request, recv_buffer, bytes_read);

	if(dec_rval.code == RC_OK)
	{
		int i;
		asn_fprint(stdout, &asn_DEF_TSRequest, ts_request);

		for(i = 0; i < ts_request->negoTokens->list.count; i++)
		{
			STREAM negoToken = xmalloc(sizeof(struct stream));

			dec_rval = ber_decode(0, &asn_DEF_NegotiationToken, (void **)&negotiation_token,
				ts_request->negoTokens->list.array[i]->negoToken.buf,
				ts_request->negoTokens->list.array[i]->negoToken.size);

			negoToken->data = (unsigned char*)(ts_request->negoTokens->list.array[i]->negoToken.buf);
			negoToken->size = ts_request->negoTokens->list.array[i]->negoToken.size;
			negoToken->p = negoToken->data;
			negoToken->end = negoToken->p + negoToken->size;

			ntlm_recv(sec, negoToken);

			xfree(negoToken);
		}
	}
	else
	{
		printf("Failed to decode TSRequest\n");
		asn_DEF_TSRequest.free_struct(&asn_DEF_TSRequest, ts_request, 0);
	}
}

static int get_bit(char* buffer, int bit)
{
	return (buffer[(bit - (bit % 8)) / 8] >> (7 - bit % 8) & 1);
}

static void set_bit(char* buffer, int bit, int value)
{
	buffer[(bit - (bit % 8)) / 8] |= value << (7 - bit % 8);
}

static void credssp_des_key(char* text, char* des_key)
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

void credssp_lm_hash(char* password, char* hash)
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

	credssp_des_key(text, des_key1);
	credssp_des_key(&text[7], des_key2);

	DES_set_key((const_DES_cblock*)des_key1, &ks);
	DES_ecb_encrypt((const_DES_cblock*)lm_magic, (DES_cblock*)hash, &ks, DES_ENCRYPT);

	DES_set_key((const_DES_cblock*)des_key2, &ks);
	DES_ecb_encrypt((const_DES_cblock*)lm_magic, (DES_cblock*)&hash[8], &ks, DES_ENCRYPT);
}

void credssp_lm_response(char* password, char* challenge, char* response)
{
	char hash[21];
	char des_key1[8];
	char des_key2[8];
	char des_key3[8];
	des_key_schedule ks;

	/* A LM hash is 16-bytes long, but the LM response uses a LM hash null-padded to 21 bytes */
	memset(hash, '\0', 21);
	credssp_lm_hash(password, hash);

	/* Each 7-byte third of the 21-byte null-padded LM hash is used to create a DES key */
	credssp_des_key(hash, des_key1);
	credssp_des_key(&hash[7], des_key2);
	credssp_des_key(&hash[14], des_key3);

	/* Encrypt the LM challenge with each key, and concatenate the result. This is the LM response (24 bytes) */
	DES_set_key((const_DES_cblock*)des_key1, &ks);
	DES_ecb_encrypt((const_DES_cblock*)challenge, (DES_cblock*)response, &ks, DES_ENCRYPT);

	DES_set_key((const_DES_cblock*)des_key2, &ks);
	DES_ecb_encrypt((const_DES_cblock*)challenge, (DES_cblock*)&response[8], &ks, DES_ENCRYPT);

	DES_set_key((const_DES_cblock*)des_key3, &ks);
	DES_ecb_encrypt((const_DES_cblock*)challenge, (DES_cblock*)&response[16], &ks, DES_ENCRYPT);
}

void credssp_ntlm_hash(char* password, char* hash)
{
	/* NTLM("password") = 8846F7EAEE8FB117AD06BDD830B7586C */

	int i;
	int length;
	char* wstr_password;
	MD4_CTX md4_ctx;

	/* convert to "unicode" */

	length = strlen(password);
	wstr_password = malloc(length * 2);

	for (i = 0; i < length; i++)
	{
		wstr_password[i * 2] = password[i];
		wstr_password[i * 2 + 1] = '\0';
	}

	/* Apply the MD4 digest algorithm on the password in unicode, the result is the NTLM hash */

	MD4_Init(&md4_ctx);
	MD4_Update(&md4_ctx, wstr_password, length * 2);
	MD4_Final((void*)hash, &md4_ctx);

	free(wstr_password);
}

void credssp_ntlm_v2_hash(char* password, char* username, char* server, char* hash)
{
	int i;
	int user_length;
	int server_length;
	int value_length;

	char* value;
	char ntlm_hash[16];

	user_length = strlen(username);
	server_length = strlen(server);
	value_length = user_length + server_length;

	value = malloc(value_length * 2);

	/* First, compute the NTLMv1 hash of the password */
	credssp_ntlm_hash(password, ntlm_hash);

	/* Concatenate the username in uppercase unicode */
	for (i = 0; i < user_length; i++)
	{
		if (username[i] > 'a' && username[i] < 'z')
			value[2 * i] = username[i] - 32;
		else
			value[2 * i] = username[i];

		value[2 * i + 1] = '\0';
	}

	/* Concatenate the domain name in unicode */
	for (i = 0; i < server_length; i++)
	{
		value[(user_length + i) * 2] = server[i];
		value[(user_length + i) * 2 + 1] = '\0';
	}

	/* Compute the HMAC-MD5 hash of the above value using the NTLMv1 hash as the key, the result is the NTLMv2 hash */
	HMAC(EVP_md5(), (void*)ntlm_hash, 16, (void*)value, (value_length) * 2, (void*)hash, NULL);

	free(value);
}

void credssp_lm_v2_response(char* password, char* username, char* server, uint8* challenge, uint8* response)
{
	char clientRandom[8];

	/* Generate an 8-byte client random */
	RAND_bytes((void*)clientRandom, 8);

	credssp_lm_v2_response_static(password, username, server, challenge, response, clientRandom);
}

void credssp_lm_v2_response_static(char* password, char* username, char* server, uint8* challenge, uint8* response, char* random)
{
	char ntlm_v2_hash[16];
	char value[16];

	/* Compute the NTLMv2 hash */
	credssp_ntlm_v2_hash(password, username, server, ntlm_v2_hash);

	/* Concatenate the server and client challenges */
	memcpy(value, challenge, 8);
	memcpy(&value[8], random, 8);

	/* Compute the HMAC-MD5 hash of the resulting value using the NTLMv2 hash as the key */
	HMAC(EVP_md5(), (void*)ntlm_v2_hash, 16, (void*)value, 16, (void*)response, NULL);

	/* Concatenate the resulting HMAC-MD5 hash and the client random, giving us the LMv2 response (24 bytes) */
	memcpy(&response[16], random, 8);
}

void credssp_ntlm_current_time(char* timestamp)
{
	uint64 time64;
	
	/* Timestamp (8 bytes), represented as the number of tenths of microseconds since midnight of January 1, 1601 */
	time64 = time(NULL) + 11644473600; /* Seconds since January 1, 1601 */
	time64 *= 10000000; /* Convert timestamp to tenths of a microsecond */

	memcpy(timestamp, &time64, 8); /* Copy into timestamp in little-endian */
}

void credssp_ntlm_nonce(uint8* nonce, int size)
{
	/* Generate random bytes (nonce) */
	RAND_bytes((void*)nonce, size);
}

void credssp_ntlm_signing_key(uint8* random_session_key, uint8* sign_magic, int sign_magic_length, uint8* signing_key)
{
	int length;
	uint8* value;
	CryptoMd5 md5;

	length = 16 + sign_magic_length;
	value = (uint8*) xmalloc(length);

	/* Concatenate RandomSessionKey with sign magic */
	memcpy(value, random_session_key, 16);
	memcpy(&value[16], sign_magic, sign_magic_length);

	md5 = crypto_md5_init();
	crypto_md5_update(md5, value, length);
	crypto_md5_final(md5, signing_key);

	xfree(value);
}

void credssp_ntlm_client_signing_key(uint8* random_session_key, uint8* signing_key)
{
	credssp_ntlm_signing_key(random_session_key, (uint8*) client_sign_magic, sizeof(client_sign_magic), signing_key);
}

void credssp_ntlm_server_signing_key(uint8* random_session_key, uint8* signing_key)
{
	credssp_ntlm_signing_key(random_session_key, (uint8*) server_sign_magic, sizeof(server_sign_magic), signing_key);
}

void credssp_ntlm_sealing_key(uint8* random_session_key, uint8* seal_magic, int seal_magic_length, uint8* sealing_key)
{
	int length;
	uint8* value;
	CryptoMd5 md5;

	length = 16 + seal_magic_length;
	value = (uint8*) xmalloc(length);

	/* Concatenate RandomSessionKey with seal magic */
	memcpy(value, random_session_key, 16);
	memcpy(&value[16], seal_magic, seal_magic_length);

	md5 = crypto_md5_init();
	crypto_md5_update(md5, value, length);
	crypto_md5_final(md5, sealing_key);

	xfree(value);
}

void credssp_ntlm_client_sealing_key(uint8* random_session_key, uint8* sealing_key)
{
	credssp_ntlm_sealing_key(random_session_key, (uint8*) client_seal_magic, sizeof(client_seal_magic), sealing_key);
}

void credssp_ntlm_server_sealing_key(uint8* random_session_key, uint8* sealing_key)
{
	credssp_ntlm_sealing_key(random_session_key, (uint8*) server_seal_magic, sizeof(server_seal_magic), sealing_key);
}

CryptoRc4 credssp_ntlm_init_client_rc4_stream(uint8* sealing_key)
{
	/* Initialize RC4 cipher with sealing key */
	return crypto_rc4_init(sealing_key, 16);
}

void credssp_ntlm_free_client_rc4_stream(CryptoRc4 rc4)
{
	crypto_rc4_free(rc4);
}

void credssp_ntlm_make_signature(uint8* msg, int msg_len, uint8* signing_key, uint8* sealing_key, uint32 seq_num, CryptoRc4 rc4, uint8* signature)
{
	uint8* value;
	uint8 ciphertext[8];
	uint32 version = 1;
	uint8 hmac_md5[16];

	value = xmalloc(msg_len + 4);
	memcpy(value, (void*) &seq_num, 4);
	memcpy(&value[4], msg, msg_len);

	/* Compute the HMAC-MD5 hash of the resulting value using the client signing key */
	HMAC(EVP_md5(), (void*) signing_key, 16, (void*) value, msg_len + 4, (void*) hmac_md5, NULL);
	
	/* Encrypt first 8 bytes of previous HMAC-MD5 using sealing key */
	crypto_rc4(rc4, 8, hmac_md5, ciphertext);

	/* Concatenate version, ciphertext and sequence number to build signature */
	memcpy(signature, (void*) &version, 4);
	memcpy(&signature[4], (void*) ciphertext, 8);
	memcpy(&signature[12], (void*) &seq_num, 4);

	xfree(value);
}

void credssp_ntlm_encrypt_message(uint8* msg, int msg_len, uint8* signing_key, uint8* sealing_key, uint32 seq_num, CryptoRc4 rc4, uint8* encrypted_message)
{
	uint8 signature[16];

	/* Encrypt message using RC4 with sealing key */
	crypto_rc4(rc4, msg_len, msg, encrypted_message);

	credssp_ntlm_make_signature(msg, msg_len, signing_key, sealing_key, seq_num, rc4, signature);

	/* Concatenate encrypted message with signature */
	memcpy(&encrypted_message[msg_len], (void*) signature, 16);
}

void credssp_ntlm_encrypt_message_with_signature(uint8* msg, int msg_len, uint8* signing_key, uint8* sealing_key, uint32 seq_num, CryptoRc4 rc4, uint8* encrypted_message, uint8* signature)
{
	/* Encrypt message using RC4 with sealing key */
	crypto_rc4(rc4, msg_len, msg, encrypted_message);

	/* Make signature, but don't concatenate */
	credssp_ntlm_make_signature(msg, msg_len, signing_key, sealing_key, seq_num, rc4, signature);
}

void credssp_ntlm_message_integrity_check(uint8* negotiate_msg, int negotiate_msg_length,
	uint8* challenge_msg, int challenge_msg_length, uint8* authenticate_msg, int authenticate_msg_length, uint8* session_key, uint8* mic)
{
	uint8* messages;
	int messages_length = negotiate_msg_length + challenge_msg_length + authenticate_msg_length;

	messages = (uint8*) xmalloc(messages_length);

	memcpy(messages, negotiate_msg, negotiate_msg_length);
	memcpy(&messages[negotiate_msg_length], challenge_msg, challenge_msg_length);
	memcpy(&messages[negotiate_msg_length + challenge_msg_length], authenticate_msg, authenticate_msg_length);

	/* Compute the HMAC-MD5 hash of the concatenated messages using the exported session key */
	HMAC(EVP_md5(), (void*) session_key, 16, (void*) messages, messages_length, (void*) mic, NULL);
}

void credssp_ntlm_v2_response(char* password, char* username, char* server, uint8* challenge, uint8* info, int info_size, uint8* response, uint8* session_key, char* timestamp)
{
	char clientRandom[8];
	char generated_timestamp[8];

	/* Timestamp */
	if (timestamp == NULL)
	{
		credssp_ntlm_current_time(generated_timestamp);
		timestamp = generated_timestamp;
	}

	/* Generate an 8-byte client random */
	credssp_ntlm_nonce((uint8*) clientRandom, 8);

	credssp_ntlm_v2_response_static(password, username, server, challenge, info, info_size, response, session_key, clientRandom, timestamp);
}

void credssp_ntlm_v2_response_static(char* password, char* username, char* server, uint8* challenge, uint8* info, int info_size, uint8* response, uint8* session_key, char* random, char* timestamp)
{
	int blob_size;
	char* ntlm_v2_blob;
	char ntlm_v2_hash[16];
	char* ntlm_v2_challenge_blob;

	blob_size = info_size + 32;
	ntlm_v2_blob = malloc(blob_size);

	/* Compute the NTLMv2 hash */
	credssp_ntlm_v2_hash(password, username, server, ntlm_v2_hash);

	/* NTLMv2_CLIENT_CHALLENGE */
	memset(ntlm_v2_blob, '\0', blob_size);

	ntlm_v2_blob[0] = 1; /* RespType (1 byte) */
	ntlm_v2_blob[1] = 1; /* HighRespType (1 byte) */
	/* Reserved1 (2 bytes) */
	/* Reserved2 (4 bytes) */
	memcpy(&ntlm_v2_blob[8], timestamp, 8); /* Timestamp (8 bytes) */
	memcpy(&ntlm_v2_blob[16], random, 8); /* ChallengeFromClient (8 bytes) */
	/* Reserved3 (4 bytes) */
	memcpy(&ntlm_v2_blob[28], info, info_size);
	/* Reserved4 (4 bytes) */

	/* Concatenate challenge with blob */
	ntlm_v2_challenge_blob = malloc(blob_size + 8);
	memcpy(ntlm_v2_challenge_blob, challenge, 8);
	memcpy(&ntlm_v2_challenge_blob[8], ntlm_v2_blob, blob_size);

	/* Compute the HMAC-MD5 hash of the resulting value using the NTLMv2 hash as the key */
	HMAC(EVP_md5(), (void*) ntlm_v2_hash, 16, (void*) ntlm_v2_challenge_blob, blob_size + 8, (void*) response, NULL);

	/* Compute NTLMv2 User Session Key (SessionBaseKey, also KeyExchangeKey) */
	if (session_key != NULL)
	{
		/* Compute the HMAC-MD5 hash of the resulting value a second time using the NTLMv2 hash as the key */
		HMAC(EVP_md5(), (void*) ntlm_v2_hash, 16, (void*) response, 16, (void*) session_key, NULL);
	}

	/* Concatenate resulting HMAC-MD5 with blob to obtain NTLMv2 response */
	memcpy(&response[16], ntlm_v2_blob, blob_size);

	free(ntlm_v2_challenge_blob);
	free(ntlm_v2_blob);
}

void credssp_ntlm_v2_encrypt_session_key(uint8* session_key, uint8* key_exchange_key, uint8* encrypted_session_key)
{
	CryptoRc4 rc4;

	/* Initialize RC4 cipher with KeyExchangeKey */
	rc4 = crypto_rc4_init((void*) key_exchange_key, 16);

	/* Encrypt Session Key */
	crypto_rc4(rc4, 16, (void*) session_key, (void*) encrypted_session_key);

	/* Free RC4 Cipher */
	crypto_rc4_free(rc4);
}

void credssp_ntlm_output_restriction_encoding(rdpSec * sec, AV_PAIR* restrictions)
{
	STREAM s = xmalloc(sizeof(struct stream));

	uint8 machineID[32] =
		"\x3A\x15\x8E\xA6\x75\x82\xD8\xF7\x3E\x06\xFA\x7A\xB4\xDF\xFD\x43"
		"\x84\x6C\x02\x3A\xFD\x5A\x94\xFE\xCF\x97\x0F\x3D\x19\x2C\x38\x20";

	s->data = restrictions->value;
	s->size = restrictions->length;
	s->end = s->data + s->size;
	s->p = s->data;

	out_uint32_le(s, 48); /* Size */
	out_uint8s(s, 4); /* Z4 (set to zero) */
	
	/* IntegrityLevel (bit 31 set to 1) */
	out_uint8(s, 1);
	out_uint8s(s, 3);

	out_uint32_le(s, 0x20000000); /* SubjectIntegrityLevel */
	out_uint8p(s, machineID, 32); /* MachineID */

	xfree(s);
}

void credssp_ntlm_output_target_name(rdpSec * sec, AV_PAIR* target_name)
{
	size_t len;
	STREAM s;
	char* spn;
	int spn_length;
	char* target;
	int target_length;
	char service[] = "TERMSRV/";
	int service_length = 8;

	s = xmalloc(sizeof(struct stream));

	s->data = target_name->value;
	s->size = target_name->length;
	s->end = s->data + s->size;
	s->p = s->data;

	target = sec->rdp->settings->server;
	target_length = strlen(sec->rdp->settings->server);

	spn_length = target_length + service_length;
	spn = (char*) xmalloc(spn_length);

	memcpy(spn, service, service_length);
	memcpy(&spn[service_length], target, target_length);

	target_name->length = spn_length * 2;
	target_name->value = (uint8*) xstrdup_out_unistr(sec->rdp, spn, &len);

	xfree(spn);
	xfree(s);
}

void credssp_ntlm_populate_av_pairs(rdpSec * sec, AV_PAIRS* av_pairs)
{
	/* MsvAvFlags */
	av_pairs->Flags = 0x00000002; /* Indicates the present of a Message Integrity Check (MIC) */

	/* MsvAvRestrictions */
	av_pairs->Restrictions.length = 48;
	av_pairs->Restrictions.value = (uint8*) xmalloc(48);
	credssp_ntlm_output_restriction_encoding(sec, &(av_pairs->Restrictions));;

	/* MsvChannelBindings */
	av_pairs->ChannelBindings.length = 16;
	av_pairs->ChannelBindings.value = (uint8*) xmalloc(16);
	memset(av_pairs->ChannelBindings.value, '\0', 16);

	/* MsvAvTargetName */
	credssp_ntlm_output_target_name(sec, &(av_pairs->TargetName));
}

void ntlm_input_av_pairs(STREAM s, AV_PAIRS* av_pairs)
{
	AV_ID AvId;
	uint16 AvLen;
	uint8* value;

	do
	{
		value = NULL;
		in_uint16_le(s, AvId);
		in_uint16_le(s, AvLen);

		if (AvLen > 0)
		{
			if (AvId != MsvAvFlags)
			{
				value = xmalloc(AvLen);
				in_uint8a(s, value, AvLen);
			}
			else
			{
				in_uint32_le(s, av_pairs->Flags);
			}
		}

		switch (AvId)
		{
			case MsvAvNbComputerName:
				printf("AvId: MsvAvNbComputerName, AvLen: %d\n", AvLen);
				av_pairs->NbComputerName.length = AvLen;
				av_pairs->NbComputerName.value = value;
				break;

			case MsvAvNbDomainName:
				printf("AvId: MsvAvNbDomainName, AvLen: %d\n", AvLen);
				av_pairs->NbDomainName.length = AvLen;
				av_pairs->NbDomainName.value = value;
				break;

			case MsvAvDnsComputerName:
				printf("AvId: MsvAvDnsComputerName, AvLen: %d\n", AvLen);
				av_pairs->DnsComputerName.length = AvLen;
				av_pairs->DnsComputerName.value = value;
				break;

			case MsvAvDnsDomainName:
				printf("AvId: MsvAvDnsDomainName, AvLen: %d\n", AvLen);
				av_pairs->DnsDomainName.length = AvLen;
				av_pairs->DnsDomainName.value = value;
				break;

			case MsvAvDnsTreeName:
				printf("AvId: MsvAvDnsTreeName, AvLen: %d\n", AvLen);
				av_pairs->DnsTreeName.length = AvLen;
				av_pairs->DnsTreeName.value = value;
				break;

			case MsvAvTimestamp:
				printf("AvId: MsvAvTimestamp, AvLen: %d\n", AvLen);
				av_pairs->Timestamp.length = AvLen;
				av_pairs->Timestamp.value = value;
				break;

			case MsvAvRestrictions:
				printf("AvId: MsvAvRestrictions, AvLen: %d\n", AvLen);
				av_pairs->Restrictions.length = AvLen;
				av_pairs->Restrictions.value = value;
				break;

			case MsvAvTargetName:
				printf("AvId: MsvAvTargetName, AvLen: %d\n", AvLen);
				av_pairs->TargetName.length = AvLen;
				av_pairs->TargetName.value = value;
				break;

			case MsvChannelBindings:
				printf("AvId: MsvAvChannelBindings, AvLen: %d\n", AvLen);
				av_pairs->ChannelBindings.length = AvLen;
				av_pairs->ChannelBindings.value = value;
				break;

			default:
				if (value != NULL)
					xfree(value);
				break;
		}
	}
	while(AvId != MsvAvEOL);
}

void ntlm_output_av_pairs(STREAM s, AV_PAIRS* av_pairs)
{
	if (av_pairs->NbDomainName.length > 0)
	{
		out_uint16_le(s, MsvAvNbDomainName); /* AvId */
		out_uint16_le(s, av_pairs->NbDomainName.length); /* AvLen */
		out_uint8a(s, av_pairs->NbDomainName.value, av_pairs->NbDomainName.length); /* Value */
	}

	if (av_pairs->NbComputerName.length > 0)
	{
		out_uint16_le(s, MsvAvNbComputerName); /* AvId */
		out_uint16_le(s, av_pairs->NbComputerName.length); /* AvLen */
		out_uint8a(s, av_pairs->NbComputerName.value, av_pairs->NbComputerName.length); /* Value */
	}

	if (av_pairs->DnsDomainName.length > 0)
	{
		out_uint16_le(s, MsvAvDnsDomainName); /* AvId */
		out_uint16_le(s, av_pairs->DnsDomainName.length); /* AvLen */
		out_uint8a(s, av_pairs->DnsDomainName.value, av_pairs->DnsDomainName.length); /* Value */
	}

	if (av_pairs->DnsComputerName.length > 0)
	{
		out_uint16_le(s, MsvAvDnsComputerName); /* AvId */
		out_uint16_le(s, av_pairs->DnsComputerName.length); /* AvLen */
		out_uint8a(s, av_pairs->DnsComputerName.value, av_pairs->DnsComputerName.length); /* Value */
	}

	if (av_pairs->DnsTreeName.length > 0)
	{
		out_uint16_le(s, MsvAvDnsTreeName); /* AvId */
		out_uint16_le(s, av_pairs->DnsTreeName.length); /* AvLen */
		out_uint8a(s, av_pairs->DnsTreeName.value, av_pairs->DnsTreeName.length); /* Value */
	}

	if (av_pairs->Timestamp.length > 0)
	{
		out_uint16_le(s, MsvAvTimestamp); /* AvId */
		out_uint16_le(s, av_pairs->Timestamp.length); /* AvLen */
		out_uint8a(s, av_pairs->Timestamp.value, av_pairs->Timestamp.length); /* Value */
	}

	if (av_pairs->Flags > 0)
	{
		out_uint16_le(s, MsvAvFlags); /* AvId */
		out_uint16_le(s, 4); /* AvLen */
		out_uint32_le(s, av_pairs->Flags); /* Value */
	}

	if (av_pairs->Restrictions.length > 0)
	{
		out_uint16_le(s, MsvAvRestrictions); /* AvId */
		out_uint16_le(s, av_pairs->Restrictions.length); /* AvLen */
		out_uint8a(s, av_pairs->Restrictions.value, av_pairs->Restrictions.length); /* Value */
	}

	if (av_pairs->ChannelBindings.length > 0)
	{
		out_uint16_le(s, MsvChannelBindings); /* AvId */
		out_uint16_le(s, av_pairs->ChannelBindings.length); /* AvLen */
		out_uint8a(s, av_pairs->ChannelBindings.value, av_pairs->ChannelBindings.length); /* Value */
	}

	if (av_pairs->TargetName.length > 0)
	{
		out_uint16_le(s, MsvAvTargetName); /* AvId */
		out_uint16_le(s, av_pairs->TargetName.length); /* AvLen */
		out_uint8a(s, av_pairs->TargetName.value, av_pairs->TargetName.length); /* Value */
	}

	/* This endicates the end of the AV_PAIR array */
	out_uint16_le(s, MsvAvEOL); /* AvId */
	out_uint16_le(s, 0); /* AvLen */
}

static void ntlm_free_av_pairs(AV_PAIRS* av_pairs)
{
	if (av_pairs != NULL)
	{
		if (av_pairs->NbComputerName.value != NULL)
			xfree(av_pairs->NbComputerName.value);
		if (av_pairs->NbDomainName.value != NULL)
			xfree(av_pairs->NbDomainName.value);
		if (av_pairs->DnsComputerName.value != NULL)
			xfree(av_pairs->DnsComputerName.value);
		if (av_pairs->DnsDomainName.value != NULL)
			xfree(av_pairs->DnsDomainName.value);
		if (av_pairs->DnsTreeName.value != NULL)
			xfree(av_pairs->DnsTreeName.value);
		if (av_pairs->Timestamp.value != NULL)
			xfree(av_pairs->Timestamp.value);
		if (av_pairs->Restrictions.value != NULL)
			xfree(av_pairs->Restrictions.value);
		if (av_pairs->TargetName.value != NULL)
			xfree(av_pairs->TargetName.value);
		if (av_pairs->ChannelBindings.value != NULL)
			xfree(av_pairs->ChannelBindings.value);

		xfree(av_pairs);
	}
}

static void ntlm_output_version(STREAM s)
{
	/* The following version information was observed with Windows 7 */

	out_uint8(s, WINDOWS_MAJOR_VERSION_6); /* ProductMajorVersion (1 byte) */
	out_uint8(s, WINDOWS_MINOR_VERSION_1); /* ProductMinorVersion (1 byte) */
	out_uint16_le(s, 7600); /* ProductBuild (2 bytes) */
	out_uint8s(s, 3); /* Reserved (3 bytes) */
	out_uint8(s, NTLMSSP_REVISION_W2K3); /* NTLMRevisionCurrent (1 byte) */
}

#if 0
static void ntlm_print_negotiate_flags(uint32 flags)
{
	printf("negotiateFlags {\n");
	if (flags & NTLMSSP_NEGOTIATE_56)
		printf("\tNTLMSSP_NEGOTIATE_56\n");
	if (flags & NTLMSSP_NEGOTIATE_KEY_EXCH)
		printf("\tNTLMSSP_NEGOTIATE_KEY_EXCH\n");
	if (flags & NTLMSSP_NEGOTIATE_128)
		printf("\tNTLMSSP_NEGOTIATE_128\n");
	if (flags & NTLMSSP_NEGOTIATE_VERSION)
		printf("\tNTLMSSP_NEGOTIATE_VERSION\n");
	if (flags & NTLMSSP_NEGOTIATE_TARGET_INFO)
		printf("\tNTLMSSP_NEGOTIATE_TARGET_INFO\n");
	if (flags & NTLMSSP_REQUEST_NON_NT_SESSION_KEY)
		printf("\tNTLMSSP_NEGOTIATE_NON_NT_SESSION_KEY\n");
	if (flags & NTLMSSP_NEGOTIATE_IDENTIFY)
		printf("\tNTLMSSP_NEGOTIATE_IDENTIFY\n");
	if (flags & NTLMSSP_NEGOTIATE_EXTENDED_SESSION_SECURITY)
		printf("\tNTLMSSP_NEGOTIATE_EXTENDED_SESSION_SECURITY\n");
	if (flags & NTLMSSP_TARGET_TYPE_SERVER)
		printf("\tNTLMSSP_TARGET_TYPE_SERVER\n");
	if (flags & NTLMSSP_TARGET_TYPE_DOMAIN)
		printf("\tNTLMSSP_TARGET_TYPE_DOMAIN\n");
	if (flags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN)
		printf("\tNTLMSSP_NEGOTIATE_ALWAYS_SIGN\n");
	if (flags & NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED)
		printf("\tNTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED\n");
	if (flags & NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED)
		printf("\tNTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED\n");
	if (flags & NTLMSSP_NEGOTIATE_NTLM)
		printf("\tNTLMSSP_NEGOTIATE_NTLM\n");
	if (flags & NTLMSSP_NEGOTIATE_LM_KEY)
		printf("\tNTLMSSP_NEGOTIATE_LM_KEY\n");
	if (flags & NTLMSSP_NEGOTIATE_DATAGRAM)
		printf("\tNTLMSSP_NEGOTIATE_DATAGRAM\n");
	if (flags & NTLMSSP_NEGOTIATE_SEAL)
		printf("\tNTLMSSP_NEGOTIATE_SEAL\n");
	if (flags & NTLMSSP_NEGOTIATE_SIGN)
		printf("\tNTLMSSP_NEGOTIATE_SIGN\n");
	if (flags & NTLMSSP_REQUEST_TARGET)
		printf("\tNTLMSSP_REQUEST_TARGET\n");
	if (flags & NTLMSSP_NEGOTIATE_OEM)
		printf("\tNTLMSSP_NEGOTIATE_OEM\n");
	if (flags & NTLMSSP_NEGOTIATE_UNICODE)
		printf("\tNTLMSSP_NEGOTIATE_UNICODE\n");
	printf("}\n");
}
#endif

void ntlm_output_negotiate_flags(STREAM s, uint32 flags)
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

void ntlm_input_negotiate_flags(STREAM s, uint32 *flags)
{
	uint8* p;
	uint8 tmp;

	/* 
	 * NegotiateFlags is a 4-byte bit map
	 * Reverse order and then input in Big Endian
	 */

	p = s->p;
	tmp = p[0];
	p[0] = p[3];
	p[3] = tmp;

	tmp = p[1];
	p[1] = p[2];
	p[2] = tmp;

	in_uint32_be(s, *flags);
	
	p = s->p;
	tmp = p[0];
	p[0] = p[3];
	p[3] = tmp;
	
	tmp = p[1];
	p[1] = p[2];
	p[2] = tmp;
}

void ntlm_send_negotiate_message(rdpSec * sec)
{
	STREAM s;
	uint32 negotiateFlags = 0;

	s = tcp_init(sec->mcs->iso->tcp, 20);

	out_uint8a(s, ntlm_signature, 8); /* Signature (8 bytes) */
	out_uint32_le(s, 1); /* MessageType */

	negotiateFlags |= NTLMSSP_NEGOTIATE_56;
	negotiateFlags |= NTLMSSP_NEGOTIATE_128;
	negotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;
	negotiateFlags |= NTLMSSP_REQUEST_NON_NT_SESSION_KEY;
	negotiateFlags |= NTLMSSP_TARGET_TYPE_DOMAIN;
	negotiateFlags |= NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED;
	negotiateFlags |= NTLMSSP_NEGOTIATE_DATAGRAM;
	negotiateFlags |= NTLMSSP_REQUEST_TARGET;
	negotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
	negotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;
	negotiateFlags |= 0x000000B0;

	ntlm_output_negotiate_flags(s, negotiateFlags); /* NegotiateFlags (4 bytes) */

	/* only set if NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED is set */

	/* DomainNameFields (8 bytes) */
	out_uint16_le(s, 0); /* DomainNameLen */
	out_uint16_le(s, 0); /* DomainNameMaxLen */
	out_uint32_le(s, 0); /* DomainNameBufferOffset */

	/* only set if NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED is set */

	/* WorkstationFields (8 bytes) */
	out_uint16_le(s, 0); /* WorkstationLen */
	out_uint16_le(s, 0); /* WorkstationMaxLen */
	out_uint32_le(s, 0); /* WorkstationBufferOffset */

	/* Version is present because NTLMSSP_NEGOTIATE_VERSION is set */
	ntlm_output_version(s); /* Version (8 bytes) */

	s_mark_end(s);

	sec->nla->negotiate_message_length = s->end - s->data;
	sec->nla->negotiate_message = (uint8*) xmalloc(sec->nla->negotiate_message_length);
	memcpy(sec->nla->negotiate_message, s->data, sec->nla->negotiate_message_length);

	credssp_send(sec, s, NULL);
	
	//sec->nla->sequence_number++;
	sec->nla->state = NTLM_STATE_CHALLENGE;

	/*
		NEGOTIATE_MESSAGE

		4e 54 4c 4d 53 53 50 00 Signature "NTLMSSP"
		01 00 00 00 MessageType (NEGOTIATE)
		b7 82 08 e2 NegotiateFlags
		00 00 DomainNameLen (0)
		00 00 DomainNameMaxLen (0)
		00 00 00 00 DomainNameBufferOffset (0)
		00 00 WorkstationLen (0)
		00 00 WorkstationMaxLen (0)
		00 00 00 00 WorkstationBufferOffset (0)
		06 01 b0 1d 00 00 00 0f Version (6.1, Build 7600)
	 */
}

void ntlm_recv_challenge_message(rdpSec * sec, STREAM s)
{
	uint8* start_offset;
	uint8* end_offset;
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

	ntlm_input_negotiate_flags(s, &(sec->nla->negotiate_flags)); /* NegotiateFlags (4 bytes) */

	in_uint8a(s, sec->nla->server_challenge, 8); /* ServerChallenge (8 bytes) */
	in_uint8s(s, 8); /* Reserved (8 bytes), should be ignored */

	/* TargetInfoFields (8 bytes) */
	in_uint16_le(s, targetInfoLen); /* TargetInfoLen (2 bytes) */
	in_uint16_le(s, targetInfoMaxLen); /* TargetInfoMaxLen (2 bytes) */
	in_uint32_le(s, targetInfoBufferOffset); /* TargetInfoBufferOffset (4 bytes) */
	
	/* only present if NTLMSSP_NEGOTIATE_VERSION is set */

	if (sec->nla->negotiate_flags & NTLMSSP_NEGOTIATE_VERSION)
	{
		in_uint8s(s, 8); /* Version (8 bytes), can be ignored */
	}

	/* Payload (variable) */

	if (targetNameLen > 0)
	{
		s->p = start_offset + targetNameBufferOffset;
		sec->nla->target_name = xmalloc(targetNameLen);
		in_uint8a(s, sec->nla->target_name, targetNameLen);
	}

	if (targetInfoLen > 0)
	{
		s->p = start_offset + targetInfoBufferOffset;
		sec->nla->target_info = malloc(targetInfoLen);
		sec->nla->target_info_length = targetInfoLen;
		memcpy(sec->nla->target_info, s->p, targetInfoLen);
		ntlm_input_av_pairs(s, sec->nla->av_pairs);
	}

	end_offset = s->p;

	sec->nla->challenge_message_length = end_offset - start_offset;
	sec->nla->challenge_message = (uint8*) xmalloc(sec->nla->challenge_message_length);
	memcpy(sec->nla->challenge_message, start_offset, sec->nla->challenge_message_length);

	/* Generate ExportedSessionKey */
	credssp_ntlm_nonce(sec->nla->exported_session_key, 16);

	/* Generate Client and Server Signing Keys */
	credssp_ntlm_client_signing_key(sec->nla->exported_session_key, (uint8*) sec->nla->client_signing_key);
	credssp_ntlm_server_signing_key(sec->nla->exported_session_key, (uint8*) sec->nla->server_signing_key);

	/* Generate Client and Server Sealing Keys */
	credssp_ntlm_client_sealing_key(sec->nla->exported_session_key, (uint8*) sec->nla->client_sealing_key);
	credssp_ntlm_server_sealing_key(sec->nla->exported_session_key, (uint8*) sec->nla->server_sealing_key);

	sec->nla->state = NTLM_STATE_AUTHENTICATE;

	/*
		CHALLENGE_MESSAGE

		4e 54 4c 4d 53 53 50 00 Signature "NTLMSSP"
		02 00 00 00 MessageType (CHALLENGE)
		16 00 TargetNameLen (22)
		16 00 TargetNameMaxLen (22)
		38 00 00 00 TargetNameBufferOffset (56)
		35 82 89 e2 NegotiateFlags
		28 d8 ce b7 71 7d 27 db ServerChallenge
		00 00 00 00 00 00 00 00 Reserved
		c4 00 TargetInfoLen (196)
		c4 00 TargetInfoMaxLen (196)
		4e 00 00 00 TargetInfoBufferOffset (78)
		06 01 b0 1d 00 00 00 0f Version (6.1, Build 7600)

		Payload (offset 56)

		TargetName "AWAKECODING" (offset 56, length 22)
		41 00 57 00 41 00 4b 00 45 00 43 00 4f 00 44 00 49 00 4e 00 47 00

		TargetInfo (offset 78, length 196)

		  02 00 AvId (MsvAvNbDomainName)
		  16 00 AvLen (22)
		  Value "AWAKECODING"
		  41 00 57 00 41 00 4b 00 45 00 43 00 4f 00 44 00 49 00 4e 00 47 00

		  01 00 AvId (MsvAvNbComputerName)
		  0e 00 AvLen (14)
		  Value "VBOXDEV"
		  56 00 42 00 4f 00 58 00 44 00 45 00 56 00

		  04 00 AvId (MsvAvDnsDomainName)
		  24 00 AvLen (36)
		  Value "awakecoding.ath.cx"
		  61 00 77 00 61 00 6b 00 65 00 63 00 6f 00 64 00
		  69 00 6e 00 67 00 2e 00 61 00 74 00 68 00 2e 00
		  63 00 78 00

		  03 00 AvId (MsvAvDnsComputerName)
		  34 00 AvLen (52)
		  Value "vboxdev.awakecoding.ath.cx"
		  76 00 62 00 6f 00 78 00 64 00 65 00 76 00 2e 00
		  61 00 77 00 61 00 6b 00 65 00 63 00 6f 00 64 00
		  69 00 6e 00 67 00 2e 00 61 00 74 00 68 00 2e 00
		  63 00 78 00

		  05 00 AvId (MsvAvDnsTreeName)
		  24 00 AvLen (36)
		  Value "awakecoding.ath.cx"
		  61 00 77 00 61 00 6b 00 65 00 63 00 6f 00 64 00
		  69 00 6e 00 67 00 2e 00 61 00 74 00 68 00 2e 00
		  63 00 78 00

		  07 00 AvId (MsvAvTimestamp)
		  08 00 AvLen (8)
		  95 04 0e b3 31 a6 cb 01

		  00 00 AvId (MsvAvEOL)
		  00 00 AvLen (0)
	 */
}

void ntlm_send_authenticate_message(rdpSec * sec)
{
	size_t len;
	STREAM s;
	rdpSet *settings;
	uint32 negotiateFlags;

	char* timestamp;
	uint8* mic_offset;
	uint8 signature[16];
	AV_PAIRS* av_pairs;
	STREAM target_info;
	STREAM pubKeyAuth;
	uint8* encrypted_public_key;

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

	uint8* DomainNameBuffer;
	uint8* UserNameBuffer;
	uint8* WorkstationBuffer;
	uint8* NtChallengeResponseBuffer;
	uint8* EncryptedRandomSessionKeyBuffer;

	s = xmalloc(sizeof(struct stream));
	s->data = xmalloc(1024);
	s->p = s->data;
	s->end = s->p;

	settings = sec->rdp->settings;
	av_pairs = sec->nla->av_pairs;

	if (av_pairs->Timestamp.value != NULL)
		timestamp = (char*) av_pairs->Timestamp.value;
	else
		timestamp = NULL;

	target_info = xmalloc(sizeof(struct stream));
	target_info->data = xmalloc(512);
	target_info->p = target_info->data;
	target_info->end = target_info->p;

	credssp_ntlm_populate_av_pairs(sec, av_pairs);
	ntlm_output_av_pairs(target_info, av_pairs);
	s_mark_end(target_info);

	hexdump(sec->nla->target_info, sec->nla->target_info_length);

	sec->nla->target_info = target_info->data;
	sec->nla->target_info_length = target_info->end - target_info->data;

	DomainNameBuffer = (uint8*) xstrdup_out_unistr(sec->rdp, settings->domain, &len);
	DomainNameLen = len;

	UserNameBuffer = (uint8*) xstrdup_out_unistr(sec->rdp, settings->username, &len);
	UserNameLen = len;

	WorkstationBuffer = (uint8*) xstrdup_out_unistr(sec->rdp, settings->server, &len);
	WorkstationLen = len;

	LmChallengeResponseLen = 24;
	NtChallengeResponseLen = sec->nla->target_info_length + 52;
	EncryptedRandomSessionKeyLen = 16;

	DomainNameBufferOffset = 88; /* starting buffer offset */
	UserNameBufferOffset = DomainNameBufferOffset + DomainNameLen;
	WorkstationBufferOffset = UserNameBufferOffset + UserNameLen;
	LmChallengeResponseBufferOffset = WorkstationBufferOffset + WorkstationLen;
	NtChallengeResponseBufferOffset = LmChallengeResponseBufferOffset + LmChallengeResponseLen;
	EncryptedRandomSessionKeyBufferOffset = NtChallengeResponseBufferOffset + NtChallengeResponseLen;

	NtChallengeResponseBuffer = (uint8*) malloc(NtChallengeResponseLen);
	EncryptedRandomSessionKeyBuffer = (uint8*) malloc(EncryptedRandomSessionKeyLen);

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
	negotiateFlags |= NTLMSSP_REQUEST_NON_NT_SESSION_KEY;
	negotiateFlags |= NTLMSSP_TARGET_TYPE_DOMAIN;
	negotiateFlags |= NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED;
	negotiateFlags |= NTLMSSP_NEGOTIATE_DATAGRAM;
	negotiateFlags |= NTLMSSP_REQUEST_TARGET;
	negotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
	negotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;
	negotiateFlags |= 0x00800030;

	ntlm_output_negotiate_flags(s, negotiateFlags); /* NegotiateFlags (4 bytes) */
	ntlm_output_version(s); /* Version (8 bytes) */

	/* MIC (16 bytes) */
	mic_offset = s->p; /* save pointer for later */
	out_uint8s(s, 16); /* this is set to zero first, then the MIC is calculated and set */

	/* Payload (offset 88) */

	/* DomainName */
	out_uint8p(s, DomainNameBuffer,  DomainNameLen);

	/* UserName */
	out_uint8p(s, UserNameBuffer,  UserNameLen);

	/* Workstation */
	out_uint8p(s, WorkstationBuffer,  WorkstationLen);

	/* LmChallengeResponse */
	out_uint8s(s, LmChallengeResponseLen); /* this is simply set to zero */

	credssp_ntlm_v2_response(settings->password, settings->username, settings->domain,
		sec->nla->server_challenge, sec->nla->target_info, sec->nla->target_info_length,
		NtChallengeResponseBuffer, sec->nla->session_base_key, timestamp);

	credssp_ntlm_v2_encrypt_session_key(sec->nla->exported_session_key,
		sec->nla->session_base_key, EncryptedRandomSessionKeyBuffer);

	out_uint8p(s, NtChallengeResponseBuffer, NtChallengeResponseLen - 4);
	out_uint8s(s, 4); /* unknown padding */

	out_uint8p(s, EncryptedRandomSessionKeyBuffer, EncryptedRandomSessionKeyLen);
	s_mark_end(s);

	sec->nla->authenticate_message_length = s->end - s->data;
	sec->nla->authenticate_message = (uint8*) xmalloc(sec->nla->authenticate_message_length);
	memcpy(sec->nla->authenticate_message, s->data, sec->nla->authenticate_message_length);

	credssp_ntlm_message_integrity_check(
		sec->nla->negotiate_message, sec->nla->negotiate_message_length,
		sec->nla->challenge_message, sec->nla->challenge_message_length,
		sec->nla->authenticate_message, sec->nla->authenticate_message_length,
		sec->nla->exported_session_key, sec->nla->message_integrity_check);

	s->p = mic_offset;
	out_uint8p(s, sec->nla->message_integrity_check, 16); /* output real MIC which was previously set to zero */
	s->p = s->end;

	pubKeyAuth = xmalloc(sizeof(struct stream));
	pubKeyAuth->data = xmalloc(sec->nla->public_key_length + 16);
	pubKeyAuth->p = pubKeyAuth->data;
	pubKeyAuth->end = pubKeyAuth->p;

	encrypted_public_key = xmalloc(sec->nla->public_key_length);
	sec->nla->rc4_stream = credssp_ntlm_init_client_rc4_stream(sec->nla->client_sealing_key);

	credssp_ntlm_encrypt_message_with_signature(sec->nla->public_key, sec->nla->public_key_length, sec->nla->client_signing_key,
		sec->nla->client_sealing_key, sec->nla->sequence_number, sec->nla->rc4_stream, encrypted_public_key, signature);

	out_uint8p(pubKeyAuth, signature, 16); /* Message Signature */
	out_uint8p(pubKeyAuth, encrypted_public_key, sec->nla->public_key_length); /* Encrypted Public Key */

	s_mark_end(pubKeyAuth);

	credssp_send(sec, s, pubKeyAuth);

	sec->nla->sequence_number++;
	sec->nla->state = NTLM_STATE_FINAL;

	xfree(DomainNameBuffer);
	xfree(UserNameBuffer);
	xfree(WorkstationBuffer);

	/*
		AUTHENTICATE_MESSAGE

		4e 54 4c 4d 53 53 50 00 Signature "NTLMSSP"
		03 00 00 00 MessageType (AUTHENTICATE)
		18 00 LmChallengeResponseLen (24)
		18 00 LmChallengeResponseMaxLen (24)
		8e 00 00 00 LmChallengeResponseBufferOffset (142)
		76 01 NtChallengeResponseLen (374)
		76 01 NtChallengeResponseMaxLen (374)
		a6 00 00 00 NtChallengeResponseBufferOffset (166)
		16 00 DomainNameLen (22)
		16 00 DomainNameMaxLen (22)
		58 00 00 00 DomainNameBufferOffset (88)
		16 00 UserNameLen (22)
		16 00 UserNameMaxLen (22)
		6e 00 00 00 UserNameBufferOffset (110)
		0a 00 WorkstationLen (10)
		0a 00 WorkstationMaxLen (10)
		84 00 00 00 WorkstationBufferOffset (132)
		10 00 EncryptedRandomSessionKeyLen (16)
		10 00 EncryptedRandomSessionKeyMaxLen (16)
		1c 02 00 00 EncryptedRandomSessionKeyBufferOffset (540)
		35 82 88 e2 NegotiateFlags
		06 01 b0 1d 00 00 00 0f Version (6.1, Build 7600)
		7c b2 31 88 4c ba a1 94 bb 02 6d fb fd fc 95 19 MIC, length 16

		Payload, offset 88

		41 00 57 00 41 00 4b 00 45 00 43 00 4f 00 44 00 49 00 4e 00 47 00 DomainName "AWAKECODING" (offset 88, length 22)
		61 00 77 00 61 00 6b 00 65 00 63 00 6f 00 64 00 69 00 6e 00 67 00 UserName "awakecoding" (offset 110, length 22)
		41 00 57 00 41 00 4b 00 45 00 "AWAKE" Workstation (offset 132, length 10)
		00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 LmChallengeResponse (offset 142, length 24)

		NtChallengeResponse (offset 166, length 374)

		  Response, NTProofStr (length 16)
		  cd ea 0b 85 66 8b 8f 2e d4 64 c3 5f 67 0a d9 cf

		  NTLMv2_CLIENT_CHALLENGE

		  01 RespType (1)
		  01 HighRespType (1)
		  00 00 Reserved1
		  00 00 00 00 Reserved2
		  95 04 0e b3 31 a6 cb 01 Timestamp
		  5a 62 97 bb 4b f3 b7 f4 ChallengeFromClient
		  00 00 00 00 Reserved3

		    TargetInfo (AV_PAIRs)

		      02 00 AvId (MsvAvNbDomainName)
		      16 00 AvLen (22)
		      Value "AWAKECODING"
		      41 00 57 00 41 00 4b 00 45 00 43 00 4f 00 44 00 49 00 4e 00 47 00

		      01 00 AvId (MsvAvNbComputerName)
		      0e 00 AvLen (14)
		      Value "VBOXDEV"
		      56 00 42 00 4f 00 58 00 44 00 45 00 56 00

		      04 00 AvId (MsvAvDnsDomainName)
		      24 00 AvLen (36)
		      Value "awakecoding.ath.cx"
		      61 00 77 00 61 00 6b 00 65 00 63 00 6f 00 64 00
		      69 00 6e 00 67 00 2e 00 61 00 74 00 68 00 2e 00
		      63 00 78 00

		      03 00 AvId (MsvAvDnsComputerName)
		      34 00 AvLen (52)
		      Value "vboxdev.awakecoding.ath.cx"
		      76 00 62 00 6f 00 78 00 64 00 65 00 76 00 2e 00
		      61 00 77 00 61 00 6b 00 65 00 63 00 6f 00 64 00
		      69 00 6e 00 67 00 2e 00 61 00 74 00 68 00 2e 00
		      63 00 78 00

		      05 00 AvId (MsvAvDnsTreeName)
		      24 00 AvLen (36)
		      Value "awakecoding.ath.cx"
		      61 00 77 00 61 00 6b 00 65 00 63 00 6f 00 64 00
		      69 00 6e 00 67 00 2e 00 61 00 74 00 68 00 2e 00
		      63 00 78 00

		      07 00 AvId (MsvAvTimestamp)
		      08 00 AvLen (8)
		      95 04 0e b3 31 a6 cb 01

		      06 00 AvId (MsvAvFlags)
		      04 00 AvLen (4)
		      02 00 00 00

		      08 00 AvId (MsvAvRestrictions)
		      30 00 AvLen (48)

		      Value (Restriction_Encoding)

			30 00 00 00 Size (48)
			00 00 00 00 Z4 (0)
			01 00 00 00 IntegrityLevel (1)
			00 20 00 00 SubjectIntegrityLevel (0x20000000)

			MachineID
			3a 15 8e a6 75 82 d8 f7 3e 06 fa 7a b4 df fd 43
			84 6c 02 3a fd 5a 94 fe cf 97 0f 3d 19 2c 38 20

		      0a 00 AvId (MsvChannelBindings)
		      10 00 AvLen (16)
		      00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

		      09 00 AvId (MsvAvTargetName)
		      2a 00 AvLen (42)
		      Value "TERMSRV/192.168.1.150"
		      54 00 45 00 52 00 4d 00 53 00 52 00 56 00 2f 00
		      31 00 39 00 32 00 2e 00 31 00 36 00 38 00 2e 00
		      31 00 2e 00 31 00 35 00 30 00

		      00 00 AvId (MsvAvEOL)
		      00 00 AvLen (0)

		    00 00 00 00 Reserved4
		    00 00 00 00 Unknown (4 bytes)

		EncryptedRandomSessionKey (offset 540, length 16)
		d1 e8 22 84 32 c1 76 0c 9b fd 4b 03 de 8b ab 49

		PubKeyAuth (length 286)
                    
                    NTLM_MESSAGE_SIGNATURE (length 16)
                    01 00 00 00 Version
                    e7 80 15 43 01 a3 41 12 Checksum
                    00 00 00 00 SeqNum

                    Encrypted SubjectPublicKey (length 270)
                    ae 2e 8b 3e 08 c7 2f 0c 9d b2 d6 0b 55 d9 b3 39
                    16 4a 08 6d 1e 4c d3 57 96 3e 32 26 dd df 7d 3c
                    26 58 e6 06 88 a8 99 ac cf e7 26 b4 ec a0 f8 a4
                    14 62 12 8d 65 b9 51 22 3e 78 31 79 08 93 c7 bd
                    f5 a4 06 a2 82 cd 7d 07 99 d8 46 a3 f7 57 31 f2
                    46 c0 d5 24 79 ac 30 3b 39 b4 74 45 b6 0d ed f8
                    fd cf ec b8 fa 21 a7 b8 69 06 13 79 e4 17 fc 2a
                    a5 68 72 50 65 cf 55 38 13 20 ba 3d ae e3 62 a5
                    f0 a6 64 47 cc 50 05 06 8b 66 1c 58 32 b7 87 70
                    52 2f a0 f6 66 2c 92 07 f9 e7 71 06 dc c2 73 79
                    1a 3b 21 84 df 53 6e 11 f5 e4 ea 3d f9 a1 ee 29
                    fc c3 02 c2 2d 77 ef 6f 8d 48 17 85 a9 19 89 e3
                    7f 5d 16 46 dc 4f a5 c0 d6 95 bb 89 bc 07 dd d3
                    31 59 6b 46 aa e2 4b 59 c2 19 2d a5 d1 b5 da 31
                    7f ba aa a8 f4 4b 56 82 e7 fc 16 a2 17 9c 48 14
                    88 f6 46 65 b1 1e 9f 1e 0a 8d af 28 fc be 2c 31
                    b1 7d b8 41 2c 95 4c a2 1c 7b bd f4 cc 5c
	*/
}

void ntlm_recv(rdpSec * sec, STREAM s)
{
	char signature[8]; /* Signature, "NTLMSSP" */
	uint32 messageType; /* MessageType */

	in_uint8a(s, signature, 8);
	in_uint32_le(s, messageType);

	switch (messageType)
	{
		/* NEGOTIATE_MESSAGE */
		case 0x00000001:
			printf("NEGOTIATE_MESSAGE\n");
			/* We should be sending that one, not receiving it! */
			break;

		/* CHALLENGE_MESSAGE */
		case 0x00000002:
			printf("CHALLENGE_MESSAGE\n");
			ntlm_recv_challenge_message(sec, s);
			break;

		/* AUTHENTICATE_MESSAGE */
		case 0x00000003:
			/* Again, we shouldn't be receiving this one */
			printf("AUTHENTICATE_MESSAGE\n");
			break;
	}
}

rdpNla *
nla_new(struct rdp_sec * sec)
{
	rdpNla * self;

	self = (rdpNla *) xmalloc(sizeof(rdpNla));
	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpNla));
		self->sec = sec;
		self->av_pairs = (AV_PAIRS*)xmalloc(sizeof(AV_PAIRS));
		memset(self->av_pairs, 0, sizeof(AV_PAIRS));
		self->state = NTLM_STATE_INITIAL;
		self->sequence_number = 0;
	}
	return self;
}

void
nla_free(rdpNla * nla)
{
	if (nla != NULL)
	{
		if (nla->target_info)
			free(nla->target_info);

		ntlm_free_av_pairs(nla->av_pairs);
		xfree(nla);
	}
}
