/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Transport Layer Security (TLS) encryption

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
#include "ssl.h"
#include "secure.h"
#include "mcs.h"
#include "iso.h"
#include "tcp.h"
#include "mem.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rc4.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/x509v3.h>

#include "tls.h"

/* TODO: Implement SSL verify enforcement, disconnect when verify fails */

/* check the identity in a certificate against a hostname */
static RD_BOOL
tls_verify_peer_identity(X509 *cert, const char *peer)
{
	X509_NAME *subject_name = NULL;
	X509_NAME_ENTRY *entry = NULL;
	ASN1_STRING *asn1str = NULL;
	//GENERAL_NAMES *subjectAltNames = NULL;
	unsigned char *ustr = NULL;
	char *str = NULL;
	int i, len;

#if 0
	/* Check cert for subjectAltName extensions */
	/* things to check: ipv4/ipv6 address, hostname in normal form and in DC= form */
	i = -1;
	for (;;)
	{
		subjectAltNames = X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
		if (ext == NULL)
			break;
	}n

	/* Check against ip address of server */
#endif
	/* Check commonName */
	subject_name = X509_get_subject_name(cert);
	if (!subject_name)
	{
		printf("ssl_verify_peer_identity: failed to get subject name\n");
		goto exit;
	}

	/* try to find a common name that matches the peer hostname */
	/* TODO: cn migth also be in DC=www,DC=redhat,DC=com format? */
	i = -1;
	for (;;)
	{
		entry = NULL;
		i = X509_NAME_get_index_by_NID(subject_name, NID_commonName, i);
		if (i == -1)
			break;
		entry = X509_NAME_get_entry(subject_name, i);
		asn1str = X509_NAME_ENTRY_get_data(entry);
		len = ASN1_STRING_to_UTF8(&ustr, asn1str);
		str = (char *)ustr;
		if (strcmp(str, peer) == 0)
			break;	/* found a match */
	}

	if (!entry)
	{
		printf("ssl_verify_peer_identity: certificate belongs to %s, but connection is to %s\n", str ? str : "unknown id", peer);
		return False;
	}
exit:
	return True;
}

/* verify SSL/TLS connection integrity. 2 checks are carried out. First make sure that the
 * certificate is assigned to the server we're connected to, and second make sure that the
 * certificate is signed by a trusted certification authority
 */

static RD_BOOL
tls_verify(SSL *connection, const char *server)
{
	/* TODO: Check for eku extension with server authentication purpose */

	RD_BOOL verified = False;
	X509 *server_cert = NULL;
	int ret;

	server_cert = SSL_get_peer_certificate(connection);
	if (!server_cert)
	{
		printf("ssl_verify: failed to get the server SSL certificate\n");
		goto exit;
	}

	ret = SSL_get_verify_result(connection);
	if (ret != X509_V_OK)
	{
		printf("ssl_verify: error %d (see 'man 1 verify' for more information)\n", ret);
		printf("certificate details:\n  Subject:\n");
		X509_NAME_print_ex_fp(stdout, X509_get_subject_name(server_cert), 4,
				XN_FLAG_MULTILINE);
		printf("\n  Issued by:\n");
		X509_NAME_print_ex_fp(stdout, X509_get_issuer_name(server_cert), 4,
				XN_FLAG_MULTILINE);
		printf("\n");

	}
	else
	{
		verified = tls_verify_peer_identity(server_cert, server);
	}

exit:
	if (!verified)
		printf("The server could not be authenticated. Connection security may be compromised!\n");

	if (server_cert)
	{
		X509_free(server_cert);
		server_cert = NULL;
	}

	return verified;
}

int
tls_get_public_key(SSL *connection, uint8** public_key, int *public_key_length)
{
	int success = 1;
	X509 *cert = NULL;
	EVP_PKEY *pkey = NULL;

	cert = SSL_get_peer_certificate(connection);

	if (!cert)
	{
		printf("tls_get_public_key: SSL_get_peer_certificate() failed\n");
		success = 0;
		goto exit;
	}

	pkey = X509_get_pubkey(cert);

	if (!cert)
	{
		printf("tls_get_public_key: X509_get_pubkey() failed\n");
		success = 0;
		goto exit;
	}

	*public_key_length = i2d_PublicKey(pkey, NULL);
	*public_key = (uint8*) xmalloc(*public_key_length);
	i2d_PublicKey(pkey, public_key);

	exit:
		if (cert)
			X509_free(cert);
		if (pkey)
			EVP_PKEY_free(pkey);

	return success;
}

/* Handle an SSL error and returns True if the caller should abort (error was fatal) */
/* TODO: Use openssl error description functions */
static RD_BOOL
tls_printf(char *func, SSL *connection, int val)
{
	switch (SSL_get_error(connection, val))
	{
		case SSL_ERROR_ZERO_RETURN:
			printf("%s: Server closed TLS connection\n", func);
			return True;
		case SSL_ERROR_WANT_READ:
			printf("SSL_ERROR_WANT_READ\n");
			//if (!ui_select(SSL_get_fd(connection)))
				/* User quit */
				//return True;
			return False;
		case SSL_ERROR_WANT_WRITE:
			//tcp_can_send(SSL_get_fd(connection), 100);
			printf("SSL_ERROR_WANT_WRITE\n");
			return False;
		case SSL_ERROR_SYSCALL:
			printf("%s: I/O error\n", func);
			return True;
		case SSL_ERROR_SSL:
			printf("%s: Failure in SSL library (protocol error?)\n", func);
			return True;
		default:
			printf("%s: Unknown error\n", func);
			return True;
	}
}

/* Create TLS context */
SSL_CTX*
tls_create_context()
{
	SSL_CTX* ctx;

	SSL_load_error_strings();
	SSL_library_init();

	ctx = SSL_CTX_new(TLSv1_client_method());

	if (ctx == NULL)
	{
		printf("SSL_CTX_new failed\n");
		return NULL;
	}

	/*
	 * This is necessary, because the Microsoft TLS implementation is not perfect.
	 * SSL_OP_ALL enables a couple of workarounds for buggy TLS implementations,
	 * but the most important workaround being SSL_OP_TLS_BLOCK_PADDING_BUG.
	 * As the size of the encrypted payload may give hints about its contents,
	 * block padding is normally used, but the Microsoft TLS implementation
	 * won't recognize it and will disconnect you after sending a TLS alert.
	 */

	SSL_CTX_set_options(ctx, SSL_OP_ALL);

	return ctx;
}

void
tls_destroy_context(SSL_CTX *ctx)
{
	SSL_CTX_free(ctx);
}

/* Initiate TLS handshake on socket */
SSL*
tls_connect(SSL_CTX *ctx, int sockfd, char *server)
{
	SSL *ssl;
	int connection_status;

	ssl = SSL_new(ctx);

	if (ssl == NULL)
	{
		printf("SSL_new failed\n");
		return NULL;
	}

	if (SSL_set_fd(ssl, sockfd) < 1)
	{
		printf("SSL_set_fd failed\n");
		return NULL;
	}

	do
	{
		/* SSL_WANT_READ errors are normal, just try again if it happens */
		connection_status = SSL_connect(ssl);
	}
	while (SSL_get_error(ssl, connection_status) == SSL_ERROR_WANT_READ);

	if (connection_status < 0)
	{
		if (tls_printf("SSL_connect", ssl, connection_status))
			return NULL;
	}

	tls_verify(ssl, server);

	printf("TLS connection established\n");

	return ssl;
}

/* Free TLS resources */
void
tls_disconnect(SSL *ssl)
{
	int ret;

	if (!ssl)
		return;

	while (True)
	{
		ret = SSL_shutdown(ssl);
		if (ret >= 0)
			break;
		if (tls_printf("ssl_disconnect", ssl, ret))
			break;
	}
	SSL_free(ssl);
	ssl = NULL;
}

/* Write data over TLS connection */
int tls_write(SSL *ssl, char* b, int length)
{
	int write_status;
	int bytesWritten = 0;

	write_status = SSL_write(ssl, b, length);

	switch (SSL_get_error(ssl, write_status))
	{
		case SSL_ERROR_NONE:
			bytesWritten += write_status;
			break;

		default:
			tls_printf("SSL_write", ssl, write_status);
			break;
	}

	if (bytesWritten < length)
		return bytesWritten += tls_write(ssl, &b[bytesWritten], length - bytesWritten);
	else
		return bytesWritten;
}

/* Read data over TLS connection */
int tls_read(SSL *ssl, char* b, int length)
{
	int status;

	while (True)
	{
		status = SSL_read(ssl, b, length);

		switch (SSL_get_error(ssl, status))
		{
			case SSL_ERROR_NONE:
				return status;
				break;

			case SSL_ERROR_WANT_READ:
				break;

			default:
				tls_printf("SSL_read", ssl, status);
				return -1;
				break;
		}
	}
}
