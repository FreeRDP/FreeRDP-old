/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Cryptographic Abstraction Layer

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

#ifndef __CRYPTO_H
#define __CRYPTO_H

#ifdef DISABLE_TLS
	#undef CRYPTO_OPENSSL
#else
	#define CRYPTO_OPENSSL
#endif

#ifdef CRYPTO_OPENSSL

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rc4.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/x509v3.h>

#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x0090800f)
#define D2I_X509_CONST const
#else
#define D2I_X509_CONST
#endif

#define CRYPTO_RC4 RC4_KEY
#define CRYPTO_SHA1 SHA_CTX
#define CRYPTO_MD5 MD5_CTX
#define CRYPTO_CERT X509
#define CRYPTO_RKEY RSA

#else /* built-in crypto */

#include "ssl.h"

#define CRYPTO_RC4 struct rc4_state
#define CRYPTO_SHA1 struct sha1_context
#define CRYPTO_MD5 struct md5_context
#define CRYPTO_CERT char
#define CRYPTO_RKEY char

#endif

void
crypto_sha1_init(CRYPTO_SHA1 * sha1);
void
crypto_sha1_update(CRYPTO_SHA1 * sha1, uint8 * data, uint32 len);
void
crypto_sha1_final(CRYPTO_SHA1 * sha1, uint8 * out_data);

void
crypto_md5_init(CRYPTO_MD5 * md5);
void
crypto_md5_update(CRYPTO_MD5 * md5, uint8 * data, uint32 len);
void
crypto_md5_final(CRYPTO_MD5 * md5, uint8 * out_data);

void
crypto_rc4_set_key(CRYPTO_RC4 * rc4, uint8 * key, uint32 len);
void
crypto_rc4(CRYPTO_RC4 * rc4, uint32 len, uint8 * in_data, uint8 * out_data);

void
crypto_rsa_encrypt(int len, uint8 * in, uint8 * out, uint32 modulus_size, uint8 * modulus, uint8 * exponent);

CRYPTO_CERT *
crypto_cert_read(uint8 * data, uint32 len);
void
crypto_cert_free(CRYPTO_CERT * cert);
CRYPTO_RKEY *
crypto_cert_to_rkey(CRYPTO_CERT * cert, uint32 * key_len);
RD_BOOL
crypto_cert_verify(CRYPTO_CERT * server_cert, CRYPTO_CERT * cacert);
int
crypto_cert_print_fp(FILE * fp, CRYPTO_CERT * cert);

void
crypto_rkey_free(CRYPTO_RKEY * rkey);
int
crypto_rkey_get_exp_mod(CRYPTO_RKEY * rkey, uint8 * exponent, uint32 max_exp_len, uint8 * modulus, uint32 max_mod_len);

#endif // __CRYPTO_H
