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

#include <freerdp/utils.h>
#include "frdp.h"
#include "ssl.h"
#include "secure.h"
#include "mcs.h"
#include "iso.h"
#include "tcp.h"
#include "rdp.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rc4.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/x509v3.h>

#include "tls.h"

struct rdp_tls
{
	rdpSec * sec;
	SSL_CTX * ctx;
	SSL * ssl;
};

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
tls_verify(rdpTls * tls, const char *server)
{
	/* TODO: Check for eku extension with server authentication purpose */

	RD_BOOL verified = False;
	X509 *server_cert = NULL;
	int ret;
	unsigned char fp_buf[20];
	unsigned int fp_len = sizeof(fp_buf);
	unsigned int i;
	char fingerprint[100];
	char * p;
	char * subject;
	char * issuer;

	server_cert = SSL_get_peer_certificate(tls->ssl);
	if (!server_cert)
	{
		printf("ssl_verify: failed to get the server SSL certificate\n");
		goto exit;
	}

	subject = X509_NAME_oneline(X509_get_subject_name(server_cert), NULL, 0);
	issuer = X509_NAME_oneline(X509_get_issuer_name(server_cert), NULL, 0);
	X509_digest(server_cert, EVP_sha1(), fp_buf, &fp_len);
	for (i = 0, p = fingerprint; i < fp_len; i++)
	{
		snprintf(p, sizeof(fingerprint) - (p - fingerprint), "%02x", fp_buf[i]);
		p += 2;
		if (i < fp_len - 1)
			*p++ = '-';
	}
	*p = '\0';

	ret = SSL_get_verify_result(tls->ssl);
	if (ret == X509_V_OK)
	{
		verified = tls_verify_peer_identity(server_cert, server);
	}

	verified = ui_check_certificate(tls->sec->rdp->inst, fingerprint, subject, issuer, verified);
	free(subject);
	free(issuer);

exit:
	if (server_cert)
	{
		X509_free(server_cert);
		server_cert = NULL;
	}

	return verified;
}

int
tls_get_public_key(rdpTls * tls, DATABLOB * public_key)
{
	int length;
	int success = 1;
	X509 *cert = NULL;
	EVP_PKEY *pkey = NULL;
	unsigned char *p;

	cert = SSL_get_peer_certificate(tls->ssl);

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

	length = i2d_PublicKey(pkey, NULL);

	if (length < 1)
	{
		printf("tls_get_public_key: i2d_PublicKey() failed\n");
		success = 0;
		goto exit;
	}

	datablob_alloc(public_key, length);
	p = (unsigned char*) public_key->data;
	i2d_PublicKey(pkey, &p);

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
rdpTls *
tls_new(struct rdp_sec * sec)
{
	rdpTls * tls;

	tls = (rdpTls *) malloc(sizeof(rdpTls));
	memset(tls, 0, sizeof(rdpTls));
	tls->sec = sec;

	SSL_load_error_strings();
	SSL_library_init();

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
tls_connect(rdpTls * tls, int sockfd, char *server)
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

	if (!tls_verify(tls, server))
		return False;

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
int tls_write(rdpTls * tls, char* b, int length)
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
int tls_read(rdpTls * tls, char* b, int length)
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
