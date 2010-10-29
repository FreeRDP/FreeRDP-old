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

#include <stdio.h>
#include <freerdp/types_ui.h>

RD_BOOL
crypto_global_init(void);	/* returns True on success */
void
crypto_global_finish(void);

typedef struct crypto_sha1_struct * CryptoSha1;

CryptoSha1
crypto_sha1_init(void);
void
crypto_sha1_update(CryptoSha1 sha1, uint8 * data, uint32 len);
void
crypto_sha1_final(CryptoSha1 sha1, uint8 * out_data);

typedef struct crypto_md5_struct * CryptoMd5;

CryptoMd5
crypto_md5_init(void);
void
crypto_md5_update(CryptoMd5 md5, uint8 * data, uint32 len);
void
crypto_md5_final(CryptoMd5 md5, uint8 * out_data);

typedef struct crypto_rc4_struct * CryptoRc4;

CryptoRc4
crypto_rc4_init(uint8 * key, uint32 len);
void
crypto_rc4(CryptoRc4 rc4, uint32 len, uint8 * in_data, uint8 * out_data);
void
crypto_rc4_free(CryptoRc4 rc4);

typedef struct crypto_cert_struct * CryptoCert;

CryptoCert
crypto_cert_read(uint8 * data, uint32 len);
void
crypto_cert_free(CryptoCert cert);
RD_BOOL
crypto_cert_verify(CryptoCert server_cert, CryptoCert cacert);
int
crypto_cert_print_fp(FILE * fp, CryptoCert cert);
int
crypto_cert_get_pub_exp_mod(CryptoCert cert, uint32 * key_len,
		uint8 * exponent, uint32 exp_len, uint8 * modulus, uint32 mod_len);

void
crypto_rsa_encrypt(int len, uint8 * in, uint8 * out, uint32 modulus_size, uint8 * modulus, uint8 * exponent);


#endif // __CRYPTO_H
