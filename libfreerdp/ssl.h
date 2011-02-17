/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Secure sockets abstraction layer
   Copyright (C) Matthew Chapman 1999-2008
   Copyright (C) Jay Sorg 2006-2010

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

#ifndef _SSL_H
#define _SSL_H

#include <stdio.h>
#include <freerdp/types_ui.h>

struct rc4_state
{
	int x;
	int y;
	int m[256];
};

struct sha1_context
{
	int total[2];
	int state[5];
	char buffer[64];
};

struct md5_context
{
	int total[2];
	int state[4];
	char buffer[64];
};

#define SSL_RC4 struct rc4_state
#define SSL_SHA1 struct sha1_context
#define SSL_MD5 struct md5_context
#define SSL_CERT char
#define SSL_PUBLIC_KEY char

void
ssl_sha1_init(SSL_SHA1 * sha1);
void
ssl_sha1_update(SSL_SHA1 * sha1, uint8 * data, uint32 len);
void
ssl_sha1_final(SSL_SHA1 * sha1, uint8 * out_data);

void
ssl_md5_init(SSL_MD5 * md5);
void
ssl_md5_update(SSL_MD5 * md5, uint8 * data, uint32 len);
void
ssl_md5_final(SSL_MD5 * md5, uint8 * out_data);

void
ssl_rc4_set_key(SSL_RC4 * rc4, uint8 * key, uint32 len);
void
ssl_rc4_crypt(SSL_RC4 * rc4, uint8 * in_data, uint8 * out_data, uint32 len);

void
ssl_rsa_encrypt(uint8 * out, uint8 * in, int len, uint32 modulus_size,
                uint8 * modulus, uint8 * exponent);

SSL_CERT *
ssl_cert_read(uint8 * data, uint32 len);
void
ssl_cert_free(SSL_CERT * cert);
RD_BOOL
ssl_cert_verify(SSL_CERT * server_cert, SSL_CERT * cacert);
int
ssl_cert_print_fp(FILE * fp, SSL_CERT * cert);

#endif
