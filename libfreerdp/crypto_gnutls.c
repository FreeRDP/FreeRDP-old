/* -*- c-basic-offset: 8 -*-
 FreeRDP: A Remote Desktop Protocol client.
 GnuTLS Cryptographic Abstraction Layer

 Copyright (C) Mads Kiilerich <mads@kiilerich.com> 2010

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
#include "debug.h""

#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/x509.h>
#include <gcrypt.h>

RD_BOOL
crypto_global_init(void)
{
	/* TODO: gnutls_check_version? gnutls_global_set_log_level? gnutls_global_set_log_function? */
	return gnutls_global_init() == GNUTLS_E_SUCCESS ? True : False;
}

void
crypto_global_finish(void)
{
	gnutls_global_deinit();
}

struct crypto_sha1_struct
{
	gnutls_hash_hd_t dig;
};

CryptoSha1
crypto_sha1_init(void)
{
	CryptoSha1 sha1 = xmalloc(sizeof(*sha1));
	int x = gnutls_hash_init(&sha1->dig, GNUTLS_DIG_SHA1);
	ASSERT(!x);
	return sha1;
}

void
crypto_sha1_update(CryptoSha1 sha1, uint8 * data, uint32 len)
{
	int x = gnutls_hash(sha1->dig, data, len);
	ASSERT(!x);
}

void
crypto_sha1_final(CryptoSha1 sha1, uint8 * out_data)
{
	gnutls_hash_deinit(sha1->dig, out_data);
	xfree(sha1);
}

struct crypto_md5_struct
{
	gnutls_hash_hd_t dig;
};

CryptoMd5
crypto_md5_init(void)
{
	CryptoMd5 md5 = xmalloc(sizeof(*md5));
	int x = gnutls_hash_init(&md5->dig, GNUTLS_DIG_MD5);
	ASSERT(!x);
	return md5;
}

void
crypto_md5_update(CryptoMd5 md5, uint8 * data, uint32 len)
{
	int x = gnutls_hash(md5->dig, data, len);
	ASSERT(!x);
}

void
crypto_md5_final(CryptoMd5 md5, uint8 * out_data)
{
	/* Assuming out_data has room for gnutls_hash_get_len(GNUTLS_DIG_MD5) */
	gnutls_hash_deinit(md5->dig, out_data);
	xfree(md5);
}

struct crypto_rc4_struct
{
	gnutls_cipher_hd_t handle;
};

CryptoRc4
crypto_rc4_init(uint8 * key, uint32 len)
{
	CryptoRc4 rc4 = xmalloc(sizeof(*rc4));
	gnutls_datum_t key_datum;
	key_datum.size = len;
	key_datum.data = key;
	gnutls_datum_t iv_datum;
	iv_datum.size = 0;
	iv_datum.data = NULL;
	int x = gnutls_cipher_init(&rc4->handle, GNUTLS_CIPHER_ARCFOUR_40, &key_datum, &iv_datum);
	ASSERT(!x);
	return rc4;
}

void
crypto_rc4(CryptoRc4 rc4, uint32 len, uint8 * in_data, uint8 * out_data)
{
	if (out_data != in_data)
		memcpy(out_data, in_data, len);
	int x = gnutls_cipher_encrypt (rc4->handle, out_data, len);
	ASSERT(!x);
}

void
crypto_rc4_free(CryptoRc4 rc4)
{
	gnutls_cipher_deinit(rc4->handle);
	xfree(rc4);
}

struct crypto_cert_struct
{
	gnutls_x509_crt_t cert;
};

CryptoCert
crypto_cert_read(uint8 * data, uint32 len)
{
	CryptoCert cert = xmalloc(sizeof(*cert));
	int x = gnutls_x509_crt_init(&cert->cert);
	ASSERT(!x);
	gnutls_datum_t datum;
	datum.data = data;
	datum.size = len;
	x = gnutls_x509_crt_import(cert->cert, &datum, GNUTLS_X509_FMT_DER);
	ASSERT(!x);
	crypto_cert_print_fp(stdout, cert);
	return cert;
}

void
crypto_cert_free(CryptoCert cert)
{
	gnutls_x509_crt_deinit(cert->cert);
    xfree(cert);
}

RD_BOOL
crypto_cert_verify(CryptoCert server_cert, CryptoCert cacert)
{
	/* FIXME: check more here ... */
	unsigned int verify;	/* What is this? */
	return gnutls_x509_crt_check_issuer(server_cert->cert,cacert->cert) &&
			gnutls_x509_crt_verify(server_cert->cert, &cacert->cert, 1, 0, &verify) == GNUTLS_E_SUCCESS;
}

int
crypto_cert_print_fp(FILE * fp, CryptoCert cert)
{
	gnutls_datum_t out;
	int x = gnutls_x509_crt_print(cert->cert, GNUTLS_CRT_PRINT_FULL, &out);
	ASSERT(!x);
	fwrite(out.data, 1, out.size, fp);
	gnutls_free(out.data);
	return True;
}

int
crypto_cert_get_pub_exp_mod(CryptoCert cert, uint32 * key_len,
		uint8 * exponent, uint32 exp_len, uint8 * modulus, uint32 mod_len)
{
	gnutls_datum_t m;
	gnutls_datum_t e;
	/* GnuTLS 2.10.1 contains patches for "MD5 with RSA Encryption" and "SHA with RSA Encryption" */
	int x = gnutls_x509_crt_get_pk_rsa_raw(cert->cert, &m, &e);
	ASSERT(!x);
	*key_len = m.size;

	size_t l = e.size;
	ASSERT(l <= exp_len);
	memset(exponent, 0, exp_len - l);
	memcpy(exponent + exp_len - l, e.data, l);
	gnutls_free(e.data);

	l = m.size;
	ASSERT(l <= *key_len);
	memset(modulus, 0, *key_len - l);
	memcpy(modulus + *key_len - l, m.data, l);
	gnutls_free(m.data);

	return 0;
}

void
crypto_rsa_encrypt(int len, uint8 * in, uint8 * out, uint32 modulus_size, uint8 * modulus, uint8 * exponent)
{
	/* GnuTLS do not expose raw RSA, so we use the underlying gcrypt lib instead */
	ASSERT(modulus_size <= SEC_MAX_MODULUS_SIZE);

	gcry_mpi_t m;
	gcry_error_t rc = gcry_mpi_scan(&m, GCRYMPI_FMT_USG, modulus, modulus_size, NULL);
	ASSERT(!rc);

	gcry_mpi_t e;
	rc = gcry_mpi_scan(&e, GCRYMPI_FMT_USG, exponent, SEC_EXPONENT_SIZE, NULL);

	gcry_sexp_t publickey_sexp;
	rc = gcry_sexp_build(&publickey_sexp, NULL, "(public-key(rsa(n%m)(e%m)))", m, e);
	ASSERT(!rc);

	gcry_mpi_release(m);
	gcry_mpi_release(e);

	gcry_mpi_t in_gcry;
	rc = gcry_mpi_scan(&in_gcry, GCRYMPI_FMT_USG, in, len, NULL);
	ASSERT(!rc);

	gcry_sexp_t in_sexp;
	rc = gcry_sexp_build(&in_sexp, NULL, "%m", in_gcry);
	ASSERT(!rc);

	gcry_sexp_t out_sexp;
	rc = gcry_pk_encrypt(&out_sexp, in_sexp, publickey_sexp);
	ASSERT(!rc);

	gcry_sexp_t out_list_sexp;
	out_list_sexp = gcry_sexp_find_token(out_sexp, "a", 0);
	ASSERT(out_list_sexp);

	gcry_mpi_t out_gcry = gcry_sexp_nth_mpi(out_list_sexp, 1, GCRYMPI_FMT_NONE);
	ASSERT(out_gcry);

	size_t s;
	rc = gcry_mpi_print(GCRYMPI_FMT_USG, out, modulus_size, &s, out_gcry);
	ASSERT(!rc);
	ASSERT(s == modulus_size);

	gcry_mpi_release(out_gcry);
	gcry_sexp_release(out_list_sexp);
	gcry_mpi_release(in_gcry);
	gcry_sexp_release(out_sexp);
	gcry_sexp_release(in_sexp);
	gcry_sexp_release(publickey_sexp);
}
