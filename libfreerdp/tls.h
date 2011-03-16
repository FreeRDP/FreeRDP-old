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

typedef struct rdp_tls rdpTls;

#include "tcp.h"
#include "types.h"

rdpTls *
tls_new(struct rdp_sec * sec);
void
tls_free(rdpTls * tls);
RD_BOOL
tls_connect(rdpTls * tls, int sockfd, char * server);
void
tls_disconnect(rdpTls * tls);
int
tls_write(rdpTls * tls, char * b, int length);
int
tls_read(rdpTls * tls, char * b, int length);
int
tls_get_public_key(rdpTls * tls, DATABLOB * public_key);

#endif	// __TLS_H
