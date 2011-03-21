/*
   FreeRDP: A Remote Desktop Protocol client.
   Cryptographic Abstraction Layer

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
RD_BOOL
crypto_cert_verify_peer_identity(CryptoCert cert, const char * peer);
int
crypto_cert_print_fp(FILE * fp, CryptoCert cert);
int
crypto_cert_get_pub_exp_mod(CryptoCert cert, uint32 * key_len,
		uint8 * exponent, uint32 exp_len, uint8 * modulus, uint32 mod_len);
char *
crypto_cert_get_subject(CryptoCert cert);
char *
crypto_cert_get_issuer(CryptoCert cert);
char *
crypto_cert_get_fingerprint(CryptoCert cert);
int
crypto_cert_get_public_key(CryptoCert cert, DATABLOB * public_key);

void
crypto_rsa_encrypt(int len, uint8 * in, uint8 * out, uint32 modulus_size, uint8 * modulus, uint8 * exponent);

void
crypto_nonce(uint8 * nonce, int size);

#endif // __CRYPTO_H
