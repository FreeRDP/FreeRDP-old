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

#define NTLMSSP_NEGOTIATE_56				0x00000001
#define NTLMSSP_NEGOTIATE_KEY_EXCH			0x00000002
#define NTLMSSP_NEGOTIATE_128				0x00000004
#define NTLMSSP_NEGOTIATE_VERSION			0x00000040
#define NTLMSSP_NEGOTIATE_TARGET_INFO			0x00000100
#define NTLMSSP_REQUEST_NON_NT_SESSION_KEY		0x00000200
#define NTLMSSP_NEGOTIATE_IDENTIFY			0x00000800
#define NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY	0x00001000
#define NTLMSSP_TARGET_TYPE_SHARE			0x00002000
#define NTLMSSP_TARGET_TYPE_SERVER			0x00004000
#define NTLMSSP_TARGET_TYPE_DOMAIN			0x00008000
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN			0x00010000
#define NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED	0x00040000
#define NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED		0x00080000
#define NTLMSSP_NEGOTIATE_NT_ONLY			0x00200000
#define NTLMSSP_NEGOTIATE_NTLM				0x00400000
#define NTLMSSP_NEGOTIATE_LM_KEY			0x01000000
#define NTLMSSP_NEGOTIATE_DATAGRAM			0x02000000
#define NTLMSSP_NEGOTIATE_SEAL				0x04000000
#define NTLMSSP_NEGOTIATE_SIGN				0x08000000
#define NTLMSSP_REQUEST_TARGET				0x10000000
#define NTLMSSP_NEGOTIATE_OEM				0x40000000
#define NTLMSSP_NEGOTIATE_UNICODE			0x80000000

#define WINDOWS_MAJOR_VERSION_5		0x05
#define WINDOWS_MAJOR_VERSION_6		0x06
#define WINDOWS_MINOR_VERSION_0		0x00
#define WINDOWS_MINOR_VERSION_1		0x01
#define WINDOWS_MINOR_VERSION_2		0x02
#define NTLMSSP_REVISION_W2K3		0x0F

const char ntlm_signature[] = "NTLMSSP";
const char lm_magic[] = "KGS!@#$%";

/* http://davenport.sourceforge.net/ntlm.html is a really nice source of information with great samples */

static int
asn1_write(const void *buffer, size_t size, void *fd)
{
	/* this is used to get the size of the ASN.1 encoded result */
	return 0;
}

int credssp_authenticate(rdpSec * sec)
{
	ntlm_send_negotiate_message(sec);
	credssp_recv(sec);
	ntlm_send_authenticate_message(sec);
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

static void credssp_lm_hash(char* password, char* hash)
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

	/* Concatenate the username and server name in uppercase unicode */
	for (i = 0; i < user_length; i++)
	{
		if (username[i] > 'a' && username[i] < 'z')
			value[2 * i] = username[i] - 32;
		else
			value[2 * i] = username[i];

		value[2 * i + 1] = '\0';
	}

	for (i = 0; i < server_length; i++)
	{
		if (server[i] > 'a' && server[i] < 'z')
			value[(user_length + i) * 2] = server[i] - 32;
		else
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

void credssp_ntlm_v2_response(char* password, char* username, char* server, uint8* challenge, uint8* info, int info_size, uint8* response, uint8* session_key)
{
	uint64 time64;
	char timestamp[8];
	char clientRandom[8];

	/* Timestamp (8 bytes), represented as the number of tenths of microseconds since midnight of January 1, 1601 */
	time64 = time(NULL) + 11644473600; /* Seconds since January 1, 1601 */
	time64 *= 10000000; /* Convert timestamp to tenths of a microsecond */
	
	memcpy(timestamp, &timestamp, 8); /* Copy into timestamp in little-endian */

	/* Generate an 8-byte client random */
	RAND_bytes((void*)clientRandom, 8);

	credssp_ntlm_v2_response_static(password, username, server, challenge, info, info_size, response, session_key, clientRandom, timestamp);
}

void credssp_ntlm_v2_response_static(char* password, char* username, char* server, uint8* challenge, uint8* info, int info_size, uint8* response, uint8* session_key, char* random, char* timestamp)
{
	int blob_size;
	char* ntlm_v2_blob;
	char ntlm_v2_hash[16];
	char* ntlm_v2_challenge_blob;

	blob_size = 32 + info_size;
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

	/* Compute NTLMv2 User Session Key */
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

void ntlm_input_av_pairs(STREAM s, AV_PAIRS* av_pairs)
{
	AV_ID AvId;
	uint16 AvLen;
	uint8* value = NULL;

	do
	{
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
				av_pairs->NbComputerName.length = AvLen;
				av_pairs->NbComputerName.value = value;
				break;

			case MsvAvNbDomainName:
				av_pairs->NbDomainName.length = AvLen;
				av_pairs->NbDomainName.value = value;
				break;

			case MsvAvDnsComputerName:
				av_pairs->DnsComputerName.length = AvLen;
				av_pairs->DnsComputerName.value = value;
				break;

			case MsvAvDnsDomainName:
				av_pairs->DnsDomainName.length = AvLen;
				av_pairs->DnsDomainName.value = value;
				break;

			case MsvAvDnsTreeName:
				av_pairs->DnsTreeName.length = AvLen;
				av_pairs->DnsTreeName.value = value;
				break;

			case MsvAvTimestamp:
				av_pairs->Timestamp.length = AvLen;
				av_pairs->Timestamp.value = value;
				break;

			case MsvAvRestrictions:
				av_pairs->Restrictions.length = AvLen;
				av_pairs->Restrictions.value = value;
				break;

			case MsvAvTargetName:
				av_pairs->TargetName.length = AvLen;
				av_pairs->TargetName.value = value;
				break;

			case MsvChannelBindings:
				av_pairs->ChannelBindings.length = AvLen;
				av_pairs->ChannelBindings.value = value;
				break;

			default:
				xfree(value);
				break;
		}
	}
	while(AvId != MsvAvEOL);
}

#ifdef COMPILE_UNUSED_CODE

static void ntlm_output_av_pairs(STREAM s, AV_PAIRS* av_pairs)
{
	if (av_pairs->NbComputerName.length > 0)
	{
		out_uint16_le(s, MsvAvNbComputerName); /* AvId */
		out_uint16_le(s, av_pairs->NbComputerName.length); /* AvLen */
		out_uint8a(s, av_pairs->NbComputerName.value, av_pairs->NbComputerName.length); /* Value */
	}

	if (av_pairs->NbDomainName.length > 0)
	{
		out_uint16_le(s, MsvAvNbDomainName); /* AvId */
		out_uint16_le(s, av_pairs->NbDomainName.length); /* AvLen */
		out_uint8a(s, av_pairs->NbDomainName.value, av_pairs->NbDomainName.length); /* Value */
	}

	if (av_pairs->DnsComputerName.length > 0)
	{
		out_uint16_le(s, MsvAvDnsComputerName); /* AvId */
		out_uint16_le(s, av_pairs->DnsComputerName.length); /* AvLen */
		out_uint8a(s, av_pairs->DnsComputerName.value, av_pairs->DnsComputerName.length); /* Value */
	}

	if (av_pairs->DnsDomainName.length > 0)
	{
		out_uint16_le(s, MsvAvDnsDomainName); /* AvId */
		out_uint16_le(s, av_pairs->DnsDomainName.length); /* AvLen */
		out_uint8a(s, av_pairs->DnsDomainName.value, av_pairs->DnsDomainName.length); /* Value */
	}

	if (av_pairs->DnsTreeName.length > 0)
	{
		out_uint16_le(s, MsvAvDnsTreeName); /* AvId */
		out_uint16_le(s, av_pairs->DnsTreeName.length); /* AvLen */
		out_uint8a(s, av_pairs->DnsTreeName.value, av_pairs->DnsTreeName.length); /* Value */
	}

	/* MsvAvFlags */
	out_uint16_le(s, MsvAvFlags); /* AvId */
	out_uint16_le(s, 4); /* AvLen */
	out_uint32_le(s, av_pairs->Flags); /* Value */

	if (av_pairs->Timestamp.length > 0)
	{
		out_uint16_le(s, MsvAvTimestamp); /* AvId */
		out_uint16_le(s, av_pairs->Timestamp.length); /* AvLen */
		out_uint8a(s, av_pairs->Timestamp.value, av_pairs->Timestamp.length); /* Value */
	}

	if (av_pairs->Restrictions.length > 0)
	{
		out_uint16_le(s, MsvAvRestrictions); /* AvId */
		out_uint16_le(s, av_pairs->Restrictions.length); /* AvLen */
		out_uint8a(s, av_pairs->Restrictions.value, av_pairs->Restrictions.length); /* Value */
	}

	if (av_pairs->TargetName.length > 0)
	{
		out_uint16_le(s, MsvAvTargetName); /* AvId */
		out_uint16_le(s, av_pairs->TargetName.length); /* AvLen */
		out_uint8a(s, av_pairs->TargetName.value, av_pairs->TargetName.length); /* Value */
	}

	if (av_pairs->ChannelBindings.length > 0)
	{
		out_uint16_le(s, MsvChannelBindings); /* AvId */
		out_uint16_le(s, av_pairs->ChannelBindings.length); /* AvLen */
		out_uint8a(s, av_pairs->ChannelBindings.value, av_pairs->ChannelBindings.length); /* Value */
	}

	/* This endicates the end of the AV_PAIR array */
	out_uint16_le(s, MsvAvEOL); /* AvId */
	out_uint16_le(s, 0); /* AvLen */
}

#endif

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
	if (flags & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY)
		printf("\tNTLMSSP_NEGOTIATE_EXTENDED_SESSION_SECURITY\n");
	if (flags & NTLMSSP_TARGET_TYPE_SHARE)
		printf("\tNTLMSSP_TARGET_TYPE_SHARE\n");
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
	if (flags & NTLMSSP_NEGOTIATE_NT_ONLY)
		printf("\tNTLMSSP_NEGOTIATE_NT_ONLY\n");
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

void ntlm_send_negotiate_message(rdpSec * sec)
{
	STREAM s;
	uint32 negotiateFlags = 0;

	s = tcp_init(sec->mcs->iso->tcp, 20);

	out_uint8a(s, ntlm_signature, 8); /* Signature (8 bytes) */
	out_uint32_le(s, 1); /* MessageType */

	negotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;
	negotiateFlags |= NTLMSSP_NEGOTIATE_VERSION;
	negotiateFlags |= NTLMSSP_NEGOTIATE_IDENTIFY;
	negotiateFlags |= NTLMSSP_NEGOTIATE_LM_KEY;
	negotiateFlags |= NTLMSSP_NEGOTIATE_DATAGRAM;
	negotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
	negotiateFlags |= NTLMSSP_REQUEST_TARGET;
	negotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;

	out_uint32_be(s, negotiateFlags); /* NegotiateFlags (4 bytes) */

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

	credssp_send(sec, s, 0);
}

void ntlm_recv_challenge_message(rdpSec * sec, STREAM s)
{
	uint16 targetNameLen;
	uint16 targetNameMaxLen;
	uint32 targetNameBufferOffset;
	uint16 targetInfoLen;
	uint16 targetInfoMaxLen;
	uint32 targetInfoBufferOffset;

	/* TargetNameFields (8 bytes) */
	in_uint16_le(s, targetNameLen); /* TargetNameLen (2 bytes) */
	in_uint16_le(s, targetNameMaxLen); /* TargetNameMaxLen (2 bytes) */
	in_uint32_le(s, targetNameBufferOffset); /* TargetNameBufferOffset (4 bytes) */

	in_uint32_le(s, sec->nla->negotiate_flags); /* NegotiateFlags (4 bytes) */
	in_uint8a(s, sec->nla->server_challenge, 8); /* ServerChallenge (8 bytes) */
	in_uint8s(s, 8); /* Reserved (8 bytes), should be ignored */

	/* TargetInfoFields (8 bytes) */
	in_uint16_le(s, targetInfoLen); /* TargetInfoLen (2 bytes) */
	in_uint16_le(s, targetInfoMaxLen); /* TargetInfoMaxLen (2 bytes) */
	in_uint32_le(s, targetInfoBufferOffset); /* TargetInfoBufferOffset (4 bytes) */

	ntlm_print_negotiate_flags(sec->nla->negotiate_flags);

	/* only present if NTLMSSP_NEGOTIATE_VERSION is set */

	if (sec->nla->negotiate_flags & NTLMSSP_NEGOTIATE_VERSION)
	{
		in_uint8s(s, 8); /* Version (8 bytes), can be ignored */
	}

	/* Payload (variable) */

	if (targetNameLen > 0)
	{
		sec->nla->target_name = xmalloc(targetNameLen);
		memcpy(sec->nla->target_name, &(s->data[targetNameBufferOffset]), (size_t)targetNameLen);
	}

	if (targetInfoLen > 0)
	{
		s->p = &(s->data[targetInfoBufferOffset]);
		sec->nla->target_info = malloc(targetInfoLen);
		sec->nla->target_info_length = targetInfoLen;
		memcpy(sec->nla->target_info, s->p, targetInfoLen);
		ntlm_input_av_pairs(s, sec->nla->av_pairs);
	}
}

void ntlm_send_authenticate_message(rdpSec * sec)
{
	STREAM s;
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

	uint8* NtChallengeResponseBuffer;
	uint8* EncryptedRandomSessionKeyBuffer;

	rdpSet *settings;
	void * p;
	size_t len;

	s = tcp_init(sec->mcs->iso->tcp, 256);
	settings = sec->rdp->settings;

	DomainNameLen = strlen(settings->domain) * 2;
	UserNameLen = strlen(settings->username) * 2;
	WorkstationLen = strlen(settings->domain) * 2;
	LmChallengeResponseLen = 0;
	NtChallengeResponseLen = sec->nla->target_info_length + 48;
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
	out_uint32_le(s, 1); /* MessageType */

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
	negotiateFlags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
	negotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
	negotiateFlags |= NTLMSSP_REQUEST_TARGET;
	negotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
	negotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;

	out_uint32_be(s, negotiateFlags); /* NegotiateFlags (4 bytes) */

	/* MIC (16 bytes) - not used */

	/* Payload (variable) */

	p = xstrdup_out_unistr(sec->rdp, settings->domain, &len);
	out_uint8a(s, p, len + 2);
	xfree(p);
	p = xstrdup_out_unistr(sec->rdp, settings->username, &len);
	out_uint8a(s, p, len + 2);
	xfree(p);
	p = xstrdup_out_unistr(sec->rdp, settings->domain, &len);
	out_uint8a(s, p, len + 2);
	xfree(p);

	credssp_ntlm_v2_response(settings->password, settings->username, settings->domain,
		sec->nla->server_challenge, sec->nla->target_info, sec->nla->target_info_length,
		NtChallengeResponseBuffer, EncryptedRandomSessionKeyBuffer);

	out_uint8p(s, NtChallengeResponseBuffer, NtChallengeResponseLen);
	out_uint8p(s, EncryptedRandomSessionKeyBuffer, EncryptedRandomSessionKeyLen);

	credssp_send(sec, s, NULL);

	/*
	  Annotated AUTHENTICATE_MESSAGE Packet Sample:
	
	  4e 54 4c 4d 53 53 50 00 "NTLMSSP"
	  03 00 00 00 MessageType
	  18 00 LmChallengeResponseLen (24)
	  18 00 LmChallengeResponseMaxLen (24)
	  70 00 00 00 LmChallengeResponseBufferOffset (112)
	  4a 01 NtChallengeResponseLen (330)
	  4a 01 NtChallengeResponseMaxLen (330)
	  88 00 00 00 NtChallengeResponseBufferOffset (136)
	  08 00 DomainNameLen (8)
	  08 00 DomainNameMaxLen (8)
	  58 00 00 00 DomainNameBufferOffset (88)
	  08 00 UserNameLen (8)
	  08 00 UserNameMaxLen (8)
	  60 00 00 00 UserNameBufferOffset (96)
	  08 00 WorkstationLen (8)
	  08 00 WorkstationMaxLen (8)
	  68 00 00 00 WorkstationBufferOffset (104)
	  10 00 EncryptedRandomSessionKeyLen (16)
	  10 00 EncryptedRandomSessionKeyMaxLen (16)
	  d2 01 00 00 EncryptedRandomSessionKeyBufferOffset (466)
	  35 82 88 e2 NegotiateFlags
	  06 01 b0 1d 00 00 00 0f Version (6.1, Build 7600)
	  
	  Payload (Offset 88)
	
	  e3 eb a3 eb 64 b2 29 f2 DomainName (Length 8, Offset 88)
	  a6 a7 72 40 ec ba 3c 44 UserName (Length 8, Offset 96)
	  57 00 69 00 6e 00 37 00 Workstation (Length 8, Offset 104)
	  
	  LmChallengeResponse (Length 24, Offset 112):
	  75 00 73 00 65 00 72 00 57 00 49 00 4e 00 37 00 00 00 00 00 00 00 00 00
	  
	  NtChallengeResponse (Length 330, Offset 136):
	  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
	  38 1b b0 74 c5 5a 25 8a 7f 65 ba 23 c4 4a 8a 7a
	  01 01 00 00 00 00 00 00 ec e8 26 e4 6b cc ca 01
	  f3 4b f1 fb 28 99 4d 8d 00 00 00 00 02 00 1e 00
	  57 00 49 00 4e 00 2d 00 30 00 38 00 51 00 39 00
	  4b 00 51 00 46 00 50 00 50 00 4f 00 36 00 01 00
	  1e 00 57 00 49 00 4e 00 2d 00 30 00 38 00 51 00
	  39 00 4b 00 51 00 46 00 50 00 50 00 4f 00 36 00
	  04 00 1e 00 57 00 49 00 4e 00 2d 00 30 00 38 00
	  51 00 39 00 4b 00 51 00 46 00 50 00 50 00 4f 00
	  36 00 03 00 1e 00 57 00 49 00 4e 00 2d 00 30 00
	  38 00 51 00 39 00 4b 00 51 00 46 00 50 00 50 00
	  4f 00 36 00 07 00 08 00 ec e8 26 e4 6b cc ca 01
	  06 00 04 00 02 00 00 00 08 00 30 00 30 00 00 00
	  00 00 00 00 01 00 00 00 00 20 00 00 ca 32 de 66
	  8c 9d df a3 77 79 bc 93 61 78 9a c0 14 73 52 86
	  26 da 9f 93 42 0c 3c a1 93 82 3a 01 0a 00 10 00
	  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
	  09 00 2a 00 54 00 45 00 52 00 4d 00 53 00 52 00
	  56 00 2f 00 31 00 39 00 32 00 2e 00 31 00 36 00
	  38 00 2e 00 31 00 2e 00 31 00 30 00 31 00 00 00
	  00 00 00 00 00 00 00 00 00 00
	  
	  EncryptedRandomSessionKey (Length 16, Offset 466):
	  2b eb c2 83 0b 82 85 6a 37 a3 7b aa 0c 39 e2 83
	  
	  Padding (Length 294, Offset 482):
	  a3 82 01 22 04 82 01 1e 01 00 00 00 2f 2e e5 d3
	  b3 11 34 1c 00 00 00 00 9e 94 9b 76 d8 42 31 65
	  0a 0d ab b8 37 b0 32 9e 0d e1 7c 48 a6 20 8b f2
	  49 6b 20 b6 00 ef 94 0c 78 46 4a 5a 3e c4 a3 15
	  94 e7 49 b8 b2 f8 fb bf 83 b0 07 8b b3 1c 0b e8
	  23 5c 25 d4 1b 2a 97 94 fa 6c cf 96 e9 08 a8 14
	  0d bd 71 56 c9 d6 22 61 ab 6f b8 c7 e6 3f 0a 81
	  fc 16 cb 9d 1e 87 64 b5 82 75 40 76 ac d0 99 dc
	  fd ce 2c 9f f8 6f 6c 46 6c d7 f9 91 c6 51 9d 1b
	  27 9b 83 29 c2 77 d4 6f cb e7 96 a2 76 6b eb ce
	  ad ec 9a b9 2e 43 c5 5f 17 7f 2c f3 8b 27 ce 2e
	  c3 9e 7c 5d 2a 6c dd 1b 88 aa df d7 14 c8 34 8a
	  29 9b 7e 39 2a 4b d3 a0 13 cc 85 95 e1 12 5e 6a
	  0e 87 31 91 85 86 0e 1b f6 44 06 5c 79 53 a5 7f
	  38 88 4c f8 9f b1 2d f9 a8 3d cd c7 87 f9 62 71
	  37 52 f6 c2 ee b3 ac ae 7b 33 6d 7b cb b4 02 0c
	  cb 7e da 3a fe b5 91 20 c7 3e 4c 79 64 8a 25 4b
	  1e 77 c3 d4 18 a4 2c 73 ba c0 b8 3e 61 3b d7 34
	  eb 55 3c 97 eb 7b
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
