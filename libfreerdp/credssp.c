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

#include "frdp.h"
#include "rdp.h"
#include "rdpset.h"
#include "ssl.h"
#include "secure.h"
#include "stream.h"
#include "mem.h"
#include "tcp.h"
#include "mcs.h"
#include "iso.h"
#ifndef _WIN32
#include <unistd.h>
#endif

#include "TSRequest.h"
#include "NegoData.h"
#include "NegotiationToken.h"

#include <openssl/des.h>
#include <openssl/md4.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

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

void credssp_send(rdpSec * sec, STREAM s)
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

	nego_token->buf = s->data;
	nego_token->size = s->end - s->data;

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
			STREAM s = xmalloc(sizeof(struct stream));
				
			dec_rval = ber_decode(0, &asn_DEF_NegotiationToken, (void **)&negotiation_token,
				ts_request->negoTokens->list.array[i]->negoToken.buf,
				ts_request->negoTokens->list.array[i]->negoToken.size);

			s->data = (unsigned char*)(ts_request->negoTokens->list.array[i]->negoToken.buf);
			s->size = ts_request->negoTokens->list.array[i]->negoToken.size;
			s->p = s->data;
			s->end = s->p + s->size;

			ntlm_recv(sec, s);
				
			xfree(s);
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

static void compute_des_key(char* text, char* des_key)
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

static void compute_lm_hash(char* password, char* hash)
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

	compute_des_key(text, des_key1);
	compute_des_key(&text[7], des_key2);
	
	DES_set_key((const_DES_cblock*)des_key1, &ks);
	DES_ecb_encrypt((const_DES_cblock*)lm_magic, (DES_cblock*)hash, &ks, DES_ENCRYPT);
	
	DES_set_key((const_DES_cblock*)des_key2, &ks);
	DES_ecb_encrypt((const_DES_cblock*)lm_magic, (DES_cblock*)&hash[8], &ks, DES_ENCRYPT);
}

static void compute_lm_response(char* password, char* challenge, char* response)
{
	char hash[21];
	char des_key1[8];
	char des_key2[8];
	char des_key3[8];
	des_key_schedule ks;
	
	/* A LM hash is 16-bytes long, but the LM response uses a LM hash null-padded to 21 bytes */
	memset(hash, '\0', 21);
	compute_lm_hash(password, hash);

	/* Each 7-byte third of the 21-byte null-padded LM hash is used to create a DES key */
	compute_des_key(hash, des_key1);
	compute_des_key(&hash[7], des_key2);
	compute_des_key(&hash[14], des_key3);
	
	/* Encrypt the LM challenge with each key, and concatenate the result. This is the LM response (24 bytes) */
	DES_set_key((const_DES_cblock*)des_key1, &ks);
	DES_ecb_encrypt((const_DES_cblock*)challenge, (DES_cblock*)response, &ks, DES_ENCRYPT);
	
	DES_set_key((const_DES_cblock*)des_key2, &ks);
	DES_ecb_encrypt((const_DES_cblock*)challenge, (DES_cblock*)&response[8], &ks, DES_ENCRYPT);

	DES_set_key((const_DES_cblock*)des_key3, &ks);
	DES_ecb_encrypt((const_DES_cblock*)challenge, (DES_cblock*)&response[16], &ks, DES_ENCRYPT);
}

static void compute_ntlm_hash(char* password, char* hash)
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

static void compute_ntlm_v2_hash(char* password, char* username, char* server, char* hash)
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
	compute_ntlm_hash(password, ntlm_hash);

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

static void compute_lm_v2_response(char* password, char* username, char* server, uint8* challenge, uint8* response)
{
	char ntlm_v2_hash[16];
	char clientRandom[8];
	char value[16];

	/* Compute the NTLMv2 hash */
	compute_ntlm_v2_hash(password, username, server, ntlm_v2_hash);

	/* Generate an 8-byte client random */
	RAND_bytes((void*)clientRandom, 8);

	/* Concatenate the server and client challenges */
	memcpy(value, challenge, 8);
	memcpy(&value[8], clientRandom, 8);
	
	/* Compute the HMAC-MD5 hash of the resulting value using the NTLMv2 hash as the key */
	HMAC(EVP_md5(), (void*)ntlm_v2_hash, 16, (void*)value, 16, (void*)response, NULL);

	/* Concatenate the resulting HMAC-MD5 hash and the client random, giving us the LMv2 response (24 bytes) */
	memcpy(&response[16], clientRandom, 8);
}

static void ntlm_input_mic(STREAM s, MIC* mic)
{
	AV_ID AvId;
	uint16 AvLen;
	uint8* value;
	
	do
	{
		in_uint16_le(s, AvId);

		if (AvLen > 0)
		{
			if (AvId != MsvAvFlags)
			{
				value = xmalloc(AvLen);
				in_uint8a(s,value, AvLen);
			}
			else
			{
				in_uint32_le(s, mic->Flags);
			}
		}
		
		switch (AvId)
		{
			case MsvAvNbComputerName:
				mic->NbComputerName.length = AvLen;
				mic->NbComputerName.value = value;
				break;

			case MsvAvNbDomainName:
				mic->NbDomainName.length = AvLen;
				mic->NbDomainName.value = value;
				break;

			case MsvAvDnsComputerName:
				mic->DnsComputerName.length = AvLen;
				mic->DnsComputerName.value = value;
				break;

			case MsvAvDnsDomainName:
				mic->DnsDomainName.length = AvLen;
				mic->DnsDomainName.value = value;
				break;

			case MsvAvDnsTreeName:
				mic->DnsTreeName.length = AvLen;
				mic->DnsTreeName.value = value;
				break;

			case MsvAvTimestamp:
				mic->Timestamp.length = AvLen;
				mic->Timestamp.value = value;
				break;

			case MsvAvRestrictions:
				mic->Restrictions.length = AvLen;
				mic->Restrictions.value = value;
				break;

			case MsvAvTargetName:
				mic->TargetName.length = AvLen;
				mic->TargetName.value = value;
				break;

			case MsvChannelBindings:
				mic->ChannelBindings.length = AvLen;
				mic->ChannelBindings.value = value;
				break;

			default:
				xfree(value);
				break;
		}
	}
	while(AvId != MsvAvEOL);
}

static void ntlm_output_mic(STREAM s, MIC* mic)
{
	if (mic->NbComputerName.length > 0)
	{
		out_uint16_le(s, MsvAvNbComputerName); /* AvId */
		out_uint16_le(s, mic->NbComputerName.length); /* AvLen */
		out_uint8a(s, mic->NbComputerName.value, mic->NbComputerName.length); /* Value */
	}

	if (mic->NbDomainName.length > 0)
	{
		out_uint16_le(s, MsvAvNbDomainName); /* AvId */
		out_uint16_le(s, mic->NbDomainName.length); /* AvLen */
		out_uint8a(s, mic->NbDomainName.value, mic->NbDomainName.length); /* Value */
	}

	if (mic->DnsComputerName.length > 0)
	{
		out_uint16_le(s, MsvAvDnsComputerName); /* AvId */
		out_uint16_le(s, mic->DnsComputerName.length); /* AvLen */
		out_uint8a(s, mic->DnsComputerName.value, mic->DnsComputerName.length); /* Value */
	}

	if (mic->DnsDomainName.length > 0)
	{
		out_uint16_le(s, MsvAvDnsDomainName); /* AvId */
		out_uint16_le(s, mic->DnsDomainName.length); /* AvLen */
		out_uint8a(s, mic->DnsDomainName.value, mic->DnsDomainName.length); /* Value */
	}

	if (mic->DnsTreeName.length > 0)
	{
		out_uint16_le(s, MsvAvDnsTreeName); /* AvId */
		out_uint16_le(s, mic->DnsTreeName.length); /* AvLen */
		out_uint8a(s, mic->DnsTreeName.value, mic->DnsTreeName.length); /* Value */
	}

	/* MsvAvFlags */
	out_uint16_le(s, MsvAvFlags); /* AvId */
	out_uint16_le(s, 4); /* AvLen */
	out_uint32_le(s, mic->Flags); /* Value */

	if (mic->Timestamp.length > 0)
	{
		out_uint16_le(s, MsvAvTimestamp); /* AvId */
		out_uint16_le(s, mic->Timestamp.length); /* AvLen */
		out_uint8a(s, mic->Timestamp.value, mic->Timestamp.length); /* Value */
	}

	if (mic->Restrictions.length > 0)
	{
		out_uint16_le(s, MsvAvRestrictions); /* AvId */
		out_uint16_le(s, mic->Restrictions.length); /* AvLen */
		out_uint8a(s, mic->Restrictions.value, mic->Restrictions.length); /* Value */
	}

	if (mic->TargetName.length > 0)
	{
		out_uint16_le(s, MsvAvTargetName); /* AvId */
		out_uint16_le(s, mic->TargetName.length); /* AvLen */
		out_uint8a(s, mic->TargetName.value, mic->TargetName.length); /* Value */
	}

	if (mic->ChannelBindings.length > 0)
	{
		out_uint16_le(s, MsvChannelBindings); /* AvId */
		out_uint16_le(s, mic->ChannelBindings.length); /* AvLen */
		out_uint8a(s, mic->ChannelBindings.value, mic->ChannelBindings.length); /* Value */
	}

	/* This endicates the end of the AV_PAIR array */
	out_uint16_le(s, MsvAvEOL); /* AvId */
	out_uint16_le(s, 0); /* AvLen */
}

static void ntlm_free_mic(MIC* mic)
{
	if (mic != NULL)
	{
		if (mic->NbComputerName.value != NULL)
			xfree(mic->NbComputerName.value);
		if (mic->NbDomainName.value != NULL)
			xfree(mic->NbDomainName.value);
		if (mic->DnsComputerName.value != NULL)
			xfree(mic->DnsComputerName.value);
		if (mic->DnsDomainName.value != NULL)
			xfree(mic->DnsDomainName.value);
		if (mic->DnsTreeName.value != NULL)
			xfree(mic->DnsTreeName.value);
		if (mic->Timestamp.value != NULL)
			xfree(mic->Timestamp.value);
		if (mic->Restrictions.value != NULL)
			xfree(mic->Restrictions.value);
		if (mic->TargetName.value != NULL)
			xfree(mic->TargetName.value);
		if (mic->ChannelBindings.value != NULL)
			xfree(mic->ChannelBindings.value);
		
		xfree(mic);
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
		
	credssp_send(sec, s);
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
		sec->nla->target_info = xmalloc(targetInfoLen);
		memcpy((void*)sec->nla->target_info, &(s->data[targetInfoBufferOffset]), (size_t)targetInfoLen);
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

	rdpSet *settings;
	uint8 LmChallengeResponse[24];
		
	s = tcp_init(sec->mcs->iso->tcp, 256);
	settings = sec->rdp->settings;

	DomainNameLen = strlen(settings->domain) * 2;
	UserNameLen = strlen(settings->username) * 2;
	WorkstationLen = strlen(settings->domain) * 2;

	DomainNameBufferOffset = 88; /* starting buffer offset */
	UserNameBufferOffset = DomainNameBufferOffset + DomainNameLen;
	WorkstationBufferOffset = UserNameBufferOffset + UserNameLen;
	LmChallengeResponseBufferOffset = WorkstationBufferOffset + 24;
	
	compute_lm_v2_response(settings->password, settings->username,
		settings->server, sec->nla->server_challenge, LmChallengeResponse);
	
	out_uint8a(s, ntlm_signature, 8); /* Signature (8 bytes) */
	out_uint32_le(s, 1); /* MessageType */

	/* LmChallengeResponseFields (8 bytes) */
	out_uint16_le(s, 24); /* LmChallengeResponseLen */
	out_uint16_le(s, 24); /* LmChallengeResponseMaxLen */
	out_uint32_le(s, 0); /* LmChallengeResponseBufferOffset */

	/* NtChallengeResponseFields (8 bytes) */
	out_uint16_le(s, 0); /* NtChallengeResponseLen */
	out_uint16_le(s, 0); /* NtChallengeResponseMaxLen */
	out_uint32_le(s, 0); /* NtChallengeResponseBufferOffset */

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
	out_uint16_le(s, 0); /* EncryptedRandomSessionKeyLen */
	out_uint16_le(s, 0); /* EncryptedRandomSessionKeyMaxLen */
	out_uint32_le(s, 0); /* EncryptedRandomSessionKeyBufferOffset */

	negotiateFlags = 0;
	negotiateFlags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
	negotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
	negotiateFlags |= NTLMSSP_REQUEST_TARGET;
	negotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
	negotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;
	
	out_uint32_be(s, negotiateFlags); /* NegotiateFlags (4 bytes) */

	/* MIC (16 bytes) */

	/* Payload (variable) */

	rdp_out_unistr(sec->rdp, s, settings->domain);
	rdp_out_unistr(sec->rdp, s, settings->username);
	rdp_out_unistr(sec->rdp, s, settings->domain);
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
		self->mic = (MIC*)xmalloc(sizeof(MIC));
	}
	return self;
}

void
nla_free(rdpNla * nla)
{
	if (nla != NULL)
	{
		ntlm_free_mic(nla->mic);
		xfree(nla);
	}
}
