/*
   FreeRDP: A Remote Desktop Protocol client.
   PolarSSL Cryptographic Abstraction Layer

   Copyright 2010 Mads Kiilerich <mads@kiilerich.com>

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
#include "crypto.h"
#include "debug.h"

#include <polarssl/sha1.h>
#include <polarssl/md5.h>
#include <polarssl/arc4.h>
#include <polarssl/x509.h>
#include <polarssl/rsa.h>

RD_BOOL
crypto_global_init(void)
{
	return True;
}

void
crypto_global_finish(void)
{
}


struct crypto_sha1_struct
{
	sha1_context ctx;
};

CryptoSha1
crypto_sha1_init(void)
{
	CryptoSha1 sha1 = xmalloc(sizeof(*sha1));
	sha1_starts(&sha1->ctx);
	return sha1;
}

void
crypto_sha1_update(CryptoSha1 sha1, uint8 * data, uint32 len)
{
	sha1_update(&sha1->ctx, data, len);
}

void
crypto_sha1_final(CryptoSha1 sha1, uint8 * out_data)
{
	sha1_finish(&sha1->ctx, out_data);
	xfree(sha1);
}


struct crypto_md5_struct
{
	md5_context ctx;
};

CryptoMd5
crypto_md5_init(void)
{
	CryptoMd5 md5 = xmalloc(sizeof(*md5));
	md5_starts(&md5->ctx);
	return md5;
}

void
crypto_md5_update(CryptoMd5 md5, uint8 * data, uint32 len)
{
	md5_update(&md5->ctx, data, len);
}

void
crypto_md5_final(CryptoMd5 md5, uint8 * out_data)
{
	md5_finish(&md5->ctx, out_data);
	xfree(md5);
}


struct crypto_rc4_struct
{
	arc4_context ctx;
};

CryptoRc4
crypto_rc4_init(uint8 * key, uint32 len)
{
	CryptoRc4 rc4 = xmalloc(sizeof(*rc4));
	arc4_setup(&rc4->ctx, key, len);
	return rc4;
}

void
crypto_rc4(CryptoRc4 rc4, uint32 len, uint8 * in_data, uint8 * out_data)
{
	arc4_crypt(&rc4->ctx, len, in_data, out_data);
}

void
crypto_rc4_free(CryptoRc4 rc4)
{
	xfree(rc4);
}

struct crypto_cert_struct
{
};


CryptoCert
crypto_cert_read(uint8 * data, uint32 len)
{
	return NULL;
}

void
crypto_cert_free(CryptoCert cert)
{
	xfree(cert);
}

RD_BOOL
crypto_cert_verify(CryptoCert server_cert, CryptoCert cacert)
{
	return False;
}

int
crypto_cert_print_fp(FILE * fp, CryptoCert cert)
{
	return False;
}

int
crypto_cert_get_pub_exp_mod(CryptoCert cert, uint32 * key_len,
		uint8 * exponent, uint32 max_exp_len, uint8 * modulus, uint32 max_mod_len)
{
	return False;
}

void
crypto_rsa_encrypt(int len, uint8 * in, uint8 * out, uint32 modulus_size, uint8 * modulus, uint8 * exponent)
{
	rsa_context ctx;
	rsa_init(&ctx, 0, 0);
	ctx.len = modulus_size;
	mpi_init(&ctx.N, &ctx.E, NULL);
	mpi_read_binary(&ctx.N, modulus, modulus_size);
	mpi_read_binary(&ctx.E, exponent, SEC_EXPONENT_SIZE);
	ASSERT(!rsa_check_pubkey( &ctx ));

	ASSERT(modulus_size <= SEC_MAX_MODULUS_SIZE);
	uint8 in2[SEC_MAX_MODULUS_SIZE];
	memset(in2, 0, modulus_size - len);
	memcpy(in2 + modulus_size - len, in, len);
	int err = rsa_public(&ctx, in2, out);
	ASSERT(!err);
	mpi_free(&ctx.N, &ctx.E, NULL);
	rsa_free(&ctx);
}
