/*
   FreeRDP: A Remote Desktop Protocol client.
   Credential Security Support Provider (CredSSP)

   Copyright 2010 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#include <freerdp/rdpset.h>
#include "frdp.h"
#include "rdp.h"
#include "tls.h"
#include "asn1.h"
#include "secure.h"
#include "stream.h"
#include "types.h"
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
#include "asn1/TSCredentials.h"
#include "asn1/TSPasswordCreds.h"

#include <time.h>
#include <openssl/des.h>
#include <openssl/md4.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/engine.h>

#include "ntlmssp.h"
#include "credssp.h"

static int
asn1_write(const void *buffer, size_t size, void *fd)
{
	/* this is used to get the size of the ASN.1 encoded result */
	return 0;
}

void credssp_ntlmssp_init(rdpCredssp * credssp)
{
	NTLMSSP *ntlmssp = credssp->ntlmssp;
	rdpSet *settings = credssp->sec->rdp->settings;

	ntlmssp_set_password(ntlmssp, settings->password);
	ntlmssp_set_username(ntlmssp, settings->username);

	if (settings->domain != NULL)
	{
		if (strlen(settings->domain) > 0)
		{
			ntlmssp_set_domain(ntlmssp, settings->domain);
		}
	}
	else
	{
		ntlmssp_set_domain(ntlmssp, NULL);
	}

	ntlmssp_set_target_name(ntlmssp, settings->server);

	ntlmssp_generate_client_challenge(ntlmssp);
	ntlmssp_generate_random_session_key(ntlmssp);
	ntlmssp_generate_exported_session_key(ntlmssp);

	ntlmssp->ntlm_v2 = 0;
}

void credssp_get_public_key(rdpCredssp * credssp)
{	
	tls_get_public_key(credssp->sec->ssl, &credssp->public_key);
}

int credssp_authenticate(rdpCredssp * credssp)
{
	NTLMSSP *ntlmssp = credssp->ntlmssp;
	STREAM negoToken = xmalloc(sizeof(struct stream));
	STREAM pubKeyAuth = xmalloc(sizeof(struct stream));
	STREAM authInfo = xmalloc(sizeof(struct stream));
	uint8* negoTokenBuffer = (uint8*) xmalloc(2048);

	credssp_ntlmssp_init(credssp);
	credssp_get_public_key(credssp);

	/* NTLMSSP NEGOTIATE MESSAGE */
	negoToken->p = negoToken->data = negoTokenBuffer;
	negoToken->end = negoToken->p;
	ntlmssp_send(ntlmssp, negoToken);
	credssp_send(credssp, negoToken, NULL, NULL);

	/* NTLMSSP CHALLENGE MESSAGE */
	credssp_recv(credssp, negoToken, NULL, NULL);
	ntlmssp_recv(ntlmssp, negoToken);

	/* NTLMSSP AUTHENTICATE MESSAGE */
	negoToken->p = negoToken->data = negoTokenBuffer;
	negoToken->end = negoToken->p;
	ntlmssp_send(ntlmssp, negoToken);

	/* The last NTLMSSP message is sent with the encrypted public key */
	credssp_encrypt_public_key(credssp, pubKeyAuth);
	credssp_send(credssp, negoToken, pubKeyAuth, NULL);

	/* Encrypted Public Key +1 */
	credssp_recv(credssp, negoToken, pubKeyAuth, NULL);

	if (credssp_verify_public_key(credssp, pubKeyAuth) == 0)
	{
		/* Failed to verify server public key echo */
		return 0; /* DO NOT SEND CREDENTIALS! */
	}

	/* Send encrypted credentials */
	credssp_encode_ts_credentials(credssp);
	credssp_encrypt_ts_credentials(credssp, authInfo);
	credssp_send(credssp, NULL, NULL, authInfo);

	return 1;
}

void credssp_encrypt_public_key(rdpCredssp *credssp, STREAM s)
{
	uint8 signature[16];
	DATABLOB encrypted_public_key;
	NTLMSSP *ntlmssp = credssp->ntlmssp;

	ntlmssp_encrypt_message(ntlmssp, &credssp->public_key, &encrypted_public_key, signature);

	s->data = xmalloc(credssp->public_key.length + 16);
	s->p = s->data;
	s->end = s->p;

#ifdef WITH_DEBUG_NLA
	printf("Public Key (length = %d)\n", credssp->public_key.length);
	hexdump(credssp->public_key.data, credssp->public_key.length);
	printf("\n");

	printf("Encrypted Public Key (length = %d)\n", encrypted_public_key.length);
	hexdump(encrypted_public_key.data, encrypted_public_key.length);
	printf("\n");

	printf("Signature\n");
	hexdump(signature, 16);
	printf("\n");
#endif

	out_uint8p(s, signature, 16); /* Message Signature */
	out_uint8p(s, encrypted_public_key.data, encrypted_public_key.length); /* Encrypted Public Key */

	s_mark_end(s);
}

int credssp_verify_public_key(rdpCredssp *credssp, STREAM s)
{
	uint8 *p1, *p2;
	uint8 *signature;
	DATABLOB public_key;
	DATABLOB encrypted_public_key;

	signature = s->data;
	encrypted_public_key.data = (void*) signature + 16;
	encrypted_public_key.length = s->size - 16;

	ntlmssp_decrypt_message(credssp->ntlmssp, &encrypted_public_key, &public_key, signature);

	p1 = (uint8*) credssp->public_key.data;
	p2 = (uint8*) public_key.data;

	p2[0]--;

	if (memcmp(p1, p2, public_key.length) != 0)
	{
		printf("Could not verify server's public key echo\n");
		return 0;
	}

	p2[0]++;
	return 1;
}

void credssp_encrypt_ts_credentials(rdpCredssp *credssp, STREAM s)
{
	uint8 signature[16];
	DATABLOB encrypted_ts_credentials;
	NTLMSSP *ntlmssp = credssp->ntlmssp;

	ntlmssp_encrypt_message(ntlmssp, &credssp->ts_credentials, &encrypted_ts_credentials, signature);

	s->data = xmalloc(credssp->ts_credentials.length + 16);
	s->p = s->data;
	s->end = s->p;

#ifdef WITH_DEBUG_NLA
	printf("TSCredentials (length = %d)\n", credssp->ts_credentials.length);
	hexdump(credssp->ts_credentials.data, credssp->ts_credentials.length);
	printf("\n");

	printf("Encrypted TSCredentials (length = %d)\n", encrypted_ts_credentials.length);
	hexdump(encrypted_ts_credentials.data, encrypted_ts_credentials.length);
	printf("\n");

	printf("Signature\n");
	hexdump(signature, 16);
	printf("\n");
#endif

	out_uint8p(s, signature, 16); /* Message Signature */
	out_uint8p(s, encrypted_ts_credentials.data, encrypted_ts_credentials.length); /* Encrypted TSCredentials */

	s_mark_end(s);
}

void credssp_encode_ts_credentials(rdpCredssp *credssp)
{
	asn_enc_rval_t enc_rval;
	TSCredentials_t *ts_credentials;
	TSPasswordCreds_t *ts_password_creds;
	DATABLOB ts_password_creds_buffer;

	ts_credentials = calloc(1, sizeof(TSCredentials_t));
	ts_credentials->credType = 1; /* TSPasswordCreds */

	ts_password_creds = calloc(1, sizeof(TSPasswordCreds_t));

	/* Domain */
	ts_password_creds->domainName.buf = credssp->ntlmssp->domain.data;
	ts_password_creds->domainName.size = credssp->ntlmssp->domain.length;

	/* Username */
	ts_password_creds->userName.buf = credssp->ntlmssp->username.data;
	ts_password_creds->userName.size = credssp->ntlmssp->username.length;

	/* Password */
	ts_password_creds->password.buf = credssp->ntlmssp->password.data;
	ts_password_creds->password.size = credssp->ntlmssp->password.length;

	/* get size ASN.1 encoded TSPasswordCreds */
	enc_rval = der_encode(&asn_DEF_TSPasswordCreds, ts_password_creds, asn1_write, 0);

	if (enc_rval.encoded != -1)
	{
		datablob_alloc(&ts_password_creds_buffer, enc_rval.encoded);

		enc_rval = der_encode_to_buffer(&asn_DEF_TSPasswordCreds, ts_password_creds,
			ts_password_creds_buffer.data, ts_password_creds_buffer.length);
	}

	ts_credentials->credentials.buf = ts_password_creds_buffer.data;
	ts_credentials->credentials.size = ts_password_creds_buffer.length;

	/* get size ASN.1 encoded TSCredentials */
	enc_rval = der_encode(&asn_DEF_TSCredentials, ts_credentials, asn1_write, 0);

	if (enc_rval.encoded != -1)
	{
		datablob_alloc(&credssp->ts_credentials, enc_rval.encoded);

		enc_rval = der_encode_to_buffer(&asn_DEF_TSCredentials, ts_credentials,
			credssp->ts_credentials.data, credssp->ts_credentials.length);
	}
}

void credssp_send(rdpCredssp *credssp, STREAM negoToken, STREAM pubKeyAuth, STREAM authInfo)
{
	TSRequest_t *ts_request;
	OCTET_STRING_t *nego_token;
	NegotiationToken_t *negotiation_token;
	asn_enc_rval_t enc_rval;

	char* buffer;
	size_t size;

	ts_request = calloc(1, sizeof(TSRequest_t));
	ts_request->version = 2;

	if (negoToken != NULL)
	{
		ts_request->negoTokens = calloc(1, sizeof(NegoData_t));
		nego_token = calloc(1, sizeof(OCTET_STRING_t));
		negotiation_token = calloc(1, sizeof(NegotiationToken_t));
		nego_token->buf = negoToken->data;
		nego_token->size = negoToken->end - negoToken->data;
		ASN_SEQUENCE_ADD(ts_request->negoTokens, nego_token);
	}

	if (pubKeyAuth != NULL)
	{
		ts_request->pubKeyAuth = calloc(1, sizeof(OCTET_STRING_t));
		ts_request->pubKeyAuth->buf = pubKeyAuth->data;
		ts_request->pubKeyAuth->size = pubKeyAuth->end - pubKeyAuth->data;
	}

	if (authInfo != NULL)
	{
		ts_request->authInfo = calloc(1, sizeof(OCTET_STRING_t));
		ts_request->authInfo->buf = authInfo->data;
		ts_request->authInfo->size = authInfo->end - authInfo->data;
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
			tls_write(credssp->sec->ssl, buffer, size);
		}

		asn_DEF_TSRequest.free_struct(&asn_DEF_TSRequest, ts_request, 0);
		xfree(buffer);
	}
}

void credssp_recv(rdpCredssp *credssp, STREAM negoToken, STREAM pubKeyAuth, STREAM authInfo)
{
	TSRequest_t *ts_request = 0;
	NegotiationToken_t *negotiation_token = 0;
	asn_dec_rval_t dec_rval;

	int size = 2048;
	int bytes_read;
	char* recv_buffer;

	recv_buffer = xmalloc(size);

	bytes_read = tls_read(credssp->sec->ssl, recv_buffer, size);

	dec_rval = ber_decode(0, &asn_DEF_TSRequest, (void **)&ts_request, recv_buffer, bytes_read);

	if(dec_rval.code == RC_OK)
	{
		if (ts_request->negoTokens != NULL)
		{
			int i;
			for(i = 0; i < ts_request->negoTokens->list.count; i++)
			{
				dec_rval = ber_decode(0, &asn_DEF_NegotiationToken, (void **)&negotiation_token,
					ts_request->negoTokens->list.array[i]->negoToken.buf,
					ts_request->negoTokens->list.array[i]->negoToken.size);

				negoToken->data = (unsigned char*)(ts_request->negoTokens->list.array[i]->negoToken.buf);
				negoToken->size = ts_request->negoTokens->list.array[i]->negoToken.size;
				negoToken->p = negoToken->data;
				negoToken->end = negoToken->p + negoToken->size;
			}
		}

		if (ts_request->pubKeyAuth != NULL)
		{
			pubKeyAuth->data = ts_request->pubKeyAuth->buf;
			pubKeyAuth->size = ts_request->pubKeyAuth->size;
			pubKeyAuth->p = pubKeyAuth->data;
		}
	}
	else
	{
		printf("Failed to decode TSRequest\n");
		asn_DEF_TSRequest.free_struct(&asn_DEF_TSRequest, ts_request, 0);
	}
}

void credssp_str_to_wstr(char* str, uint8* wstr, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		wstr[i * 2] = str[i];
		wstr[i * 2 + 1] = '\0';
	}
}

void credssp_rc4k(uint8* key, int length, uint8* plaintext, uint8* ciphertext)
{
	CryptoRc4 rc4;

	/* Initialize RC4 cipher with key */
	rc4 = crypto_rc4_init((void*) key, 16);

	/* Encrypt plaintext with key */
	crypto_rc4(rc4, length, (void*) plaintext, (void*) ciphertext);

	/* Free RC4 Cipher */
	crypto_rc4_free(rc4);
}

void credssp_nonce(uint8* nonce, int size)
{
	/* Generate random bytes (nonce) */
	RAND_bytes((void*) nonce, size);
}

void credssp_current_time(uint8* timestamp)
{
	uint64 time64;

	/* Timestamp (8 bytes), represented as the number of tenths of microseconds since midnight of January 1, 1601 */
	time64 = time(NULL) + 11644473600; /* Seconds since January 1, 1601 */
	time64 *= 10000000; /* Convert timestamp to tenths of a microsecond */

	memcpy(timestamp, &time64, 8); /* Copy into timestamp in little-endian */
}

rdpCredssp *
credssp_new(struct rdp_sec * sec)
{
	rdpCredssp * self;

	self = (rdpCredssp *) xmalloc(sizeof(rdpCredssp));
	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpCredssp));
		self->sec = sec;
		self->send_seq_num = 0;
		self->ntlmssp = ntlmssp_new();
	}
	return self;
}

void
credssp_free(rdpCredssp * credssp)
{
	if (credssp != NULL)
	{
		xfree(credssp);
	}
}
