/*
   FreeRDP: A Remote Desktop Protocol client.
   Transport Layer Security (TLS) encryption

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

#include "frdp.h"
#include "secure.h"
#include "mcs.h"
#include "iso.h"
#include "tcp.h"
#include "rdp.h"
#include "crypto.h"
#include "crypto_openssl.h"
#include <freerdp/utils/memory.h>

#include "tls.h"

struct rdp_tls
{
	SSL_CTX * ctx;
	SSL * ssl;
};

RD_BOOL
tls_verify(rdpTls * tls, const char * server)
{
	/* TODO: Check for eku extension with server authentication purpose */

	RD_BOOL verified = False;
	int ret;

	ret = SSL_get_verify_result(tls->ssl);
	if (ret == X509_V_OK)
	{
		verified = True;
	}

	return verified;
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
rdpTls *
tls_new(void)
{
	rdpTls * tls;

	tls = (rdpTls *) malloc(sizeof(rdpTls));
	memset(tls, 0, sizeof(rdpTls));

	tls->ctx = SSL_CTX_new(TLSv1_client_method());

	if (tls->ctx == NULL)
	{
		printf("SSL_CTX_new failed\n");
		free(tls);
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

	SSL_CTX_set_options(tls->ctx, SSL_OP_ALL);

	return tls;
}

void
tls_free(rdpTls * tls)
{
	SSL_CTX_free(tls->ctx);
	free(tls);
}

/* Initiate TLS handshake on socket */
RD_BOOL
tls_connect(rdpTls * tls, int sockfd)
{
	int connection_status;

	tls->ssl = SSL_new(tls->ctx);

	if (tls->ssl == NULL)
	{
		printf("SSL_new failed\n");
		return False;
	}

	if (SSL_set_fd(tls->ssl, sockfd) < 1)
	{
		printf("SSL_set_fd failed\n");
		return False;
	}

	do
	{
		/* SSL_WANT_READ errors are normal, just try again if it happens */
		connection_status = SSL_connect(tls->ssl);
	}
	while (SSL_get_error(tls->ssl, connection_status) == SSL_ERROR_WANT_READ);

	if (connection_status < 0)
	{
		if (tls_printf("SSL_connect", tls->ssl, connection_status))
			return False;
	}

	printf("TLS connection established\n");

	return True;
}

/* Free TLS resources */
void
tls_disconnect(rdpTls * tls)
{
	int ret;

	if (!tls->ssl)
		return;

	while (True)
	{
		ret = SSL_shutdown(tls->ssl);
		if (ret >= 0)
			break;
		if (tls_printf("ssl_disconnect", tls->ssl, ret))
			break;
	}
	SSL_free(tls->ssl);
	tls->ssl = NULL;
}

/* Write data over TLS connection */
int
tls_write(rdpTls * tls, char* b, int length)
{
	int write_status;
	int bytesWritten = 0;

	while (bytesWritten < length)
	{
		write_status = SSL_write(tls->ssl, b, length);

		switch (SSL_get_error(tls->ssl, write_status))
		{
			case SSL_ERROR_NONE:
				bytesWritten += write_status;
				break;

			case SSL_ERROR_WANT_WRITE:
				break;

			default:
				tls_printf("SSL_write", tls->ssl, write_status);
				return -1;
				break;
		}
	}
	return bytesWritten;
}

/* Read data over TLS connection */
int
tls_read(rdpTls * tls, char* b, int length)
{
	int status;

	while (True)
	{
		status = SSL_read(tls->ssl, b, length);

		switch (SSL_get_error(tls->ssl, status))
		{
			case SSL_ERROR_NONE:
				return status;
				break;

			case SSL_ERROR_WANT_READ:
				break;

			default:
				tls_printf("SSL_read", tls->ssl, status);
				return -1;
				break;
		}
	}

	return 0;
}

CryptoCert
tls_get_certificate(rdpTls * tls)
{
	CryptoCert cert;
	X509 * server_cert;

	server_cert = SSL_get_peer_certificate(tls->ssl);
	if (!server_cert)
	{
		printf("ssl_verify: failed to get the server SSL certificate\n");
		cert = NULL;
	}
	else
	{
		cert = xmalloc(sizeof(*cert));
		cert->px509 = server_cert;
	}
	return cert;
}

