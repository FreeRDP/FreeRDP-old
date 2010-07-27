/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Simple Cryptographic Abstraction Layer

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
#include "crypto.h"
#include "mem.h"

/* TODO: Merge with ssl.c */
#include "ssl.h"

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
	struct sha1_context data;
};

CryptoSha1
crypto_sha1_init(void)
{
	CryptoSha1 sha1 = xmalloc(sizeof(*sha1));
	ssl_sha1_init(&sha1->data);
	return sha1;
}

void
crypto_sha1_update(CryptoSha1 sha1, uint8 * data, uint32 len)
{
	ssl_sha1_update(&sha1->data, data, len);
}

void
crypto_sha1_final(CryptoSha1 sha1, uint8 * out_data)
{
	ssl_sha1_final(&sha1->data, out_data);
	xfree(sha1);
}

struct crypto_md5_struct
{
	struct md5_context data;
};

CryptoMd5
crypto_md5_init(void)
{
	CryptoMd5 md5 = xmalloc(sizeof(*md5));
	ssl_md5_init(&md5->data);
	return md5;
}

void
crypto_md5_update(CryptoMd5 md5, uint8 * data, uint32 len)
{
	ssl_md5_update(&md5->data, data, len);
}

void
crypto_md5_final(CryptoMd5 md5, uint8 * out_data)
{
	ssl_md5_final(&md5->data, out_data);
	xfree(md5);
}

struct crypto_rc4_struct
{
	struct rc4_state data;
};

CryptoRc4
crypto_rc4_init(uint8 * key, uint32 len)
{
	CryptoRc4 rc4 = xmalloc(sizeof(*rc4));
	ssl_rc4_set_key(&rc4->data, key, len);
	return rc4;
}

void
crypto_rc4(CryptoRc4 rc4, uint32 len, uint8 * in_data, uint8 * out_data)
{
	ssl_rc4_crypt(&rc4->data, in_data, out_data, len);
}

void
crypto_rc4_free(CryptoRc4 rc4)
{
	xfree(rc4);
}

struct crypto_cert_struct
{
	char * data;
};

CryptoCert
crypto_cert_read(uint8 * data, uint32 len)
{
	CryptoCert cert = xmalloc(sizeof(*cert));
	cert->data = ssl_cert_read(data, len);
	return cert;
}

void
crypto_cert_free(CryptoCert cert)
{
	ssl_cert_free(cert->data);
    xfree(cert);
}

RD_BOOL
crypto_cert_verify(CryptoCert server_cert, CryptoCert cacert)
{
	/* FIXME: do the actual verification */
	return ssl_cert_verify(server_cert->data, cacert->data);
}

int
crypto_cert_print_fp(FILE * fp, CryptoCert cert)
{
	return ssl_cert_print_fp(fp, cert->data);
}

struct crypto_public_key_struct
{
	char * data;
};

CryptoPublicKey
crypto_cert_get_public_key(CryptoCert cert, uint32 * key_len)
{
	CryptoPublicKey public_key = xmalloc(sizeof(*public_key));
	public_key->data = ssl_cert_get_public_key(cert->data, key_len);
	return public_key;
}

void
crypto_public_key_free(CryptoPublicKey public_key)
{
	ssl_public_key_free(public_key->data);
	xfree(public_key);
}

int
crypto_public_key_get_exp_mod(CryptoPublicKey public_key, uint8 * exponent, uint32 max_exp_len, uint8 * modulus, uint32 max_mod_len)
{
	return ssl_public_key_get_exp_mod(public_key->data, exponent, max_exp_len, modulus, max_mod_len);
}

void
crypto_rsa_encrypt(int len, uint8 * in, uint8 * out, uint32 modulus_size, uint8 * modulus, uint8 * exponent)
{
	ssl_rsa_encrypt(out, in, len, modulus_size, modulus, exponent);
}
