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

#ifndef __TLS_H
#define	__TLS_H

#include "tcp.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rc4.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/x509v3.h>

#include "data_blob.h"

SSL_CTX*
tls_create_context();
void
tls_destroy_context(SSL_CTX *ctx);
SSL*
tls_connect(SSL_CTX *ctx, int sockfd, char *server);
void
tls_disconnect(SSL *ssl);
int
tls_write(SSL *ssl, char* b, int length);
int
tls_read(SSL *ssl, char* b, int length);
int
tls_get_public_key(SSL *connection, DATA_BLOB *public_key);

#endif	// __TLS_H
