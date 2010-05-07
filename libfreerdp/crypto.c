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

#include "frdp.h"
#include "crypto.h"

void
crypto_sha1_init(CRYPTO_SHA1 * sha1)
{
#ifdef CRYPTO_OPENSSL
	SHA1_Init(sha1);
#else /* built-in crypto */
	ssl_sha1_init(sha1);
#endif
}

void
crypto_sha1_update(CRYPTO_SHA1 * sha1, uint8 * data, uint32 len)
{
#ifdef CRYPTO_OPENSSL
	SHA1_Update(sha1, data, len);
#else /* built-in crypto */
	ssl_sha1_update(sha1, data, len);
#endif
}

void
crypto_sha1_final(CRYPTO_SHA1 * sha1, uint8 * out_data)
{
#ifdef CRYPTO_OPENSSL
	SHA1_Final(out_data, sha1);
#else /* built-in crypto */
	ssl_sha1_final(sha1, out_data);
#endif	
}

void
crypto_md5_init(CRYPTO_MD5 * md5)
{
#ifdef CRYPTO_OPENSSL
	MD5_Init(md5);
#else /* built-in crypto */
	ssl_md5_init(md5);
#endif
}

void
crypto_md5_update(CRYPTO_MD5 * md5, uint8 * data, uint32 len)
{
#ifdef CRYPTO_OPENSSL
	MD5_Update(md5, data, len);
#else /* built-in crypto */
	ssl_md5_update(md5, data, len);
#endif
}

void
crypto_md5_final(CRYPTO_MD5 * md5, uint8 * out_data)
{
#ifdef CRYPTO_OPENSSL
	MD5_Final(out_data, md5);
#else /* built-in crypto */
	ssl_md5_final(md5, out_data);
#endif
}

void
crypto_rc4_set_key(CRYPTO_RC4 * rc4, uint8 * key, uint32 len)
{
#ifdef CRYPTO_OPENSSL
	RC4_set_key(rc4, len, key);
#else /* built-in crypto */
	ssl_rc4_set_key(rc4, key, len);
#endif
}

void
crypto_rc4(CRYPTO_RC4 * rc4, uint32 len, uint8 * in_data, uint8 * out_data)
{
#ifdef CRYPTO_OPENSSL
	RC4(rc4, len, in_data, out_data);
#else /* built-in crypto */
	ssl_rc4_crypt(rc4, in_data, out_data, len);
#endif
}

#ifdef CRYPTO_OPENSSL
static void
reverse(uint8 * p, int len)
{
	int i, j;
	uint8 temp;

	for (i = 0, j = len - 1; i < j; i++, j--)
	{
		temp = p[i];
		p[i] = p[j];
		p[j] = temp;
	}
}
#endif

void
crypto_rsa_encrypt(int len, uint8 * in, uint8 * out, uint32 modulus_size, uint8 * modulus, uint8 * exponent)
{
#ifdef CRYPTO_OPENSSL

	BN_CTX *ctx;
	BIGNUM mod, exp, x, y;
	uint8 inr[SEC_MAX_MODULUS_SIZE];
	int outlen;

	reverse(modulus, modulus_size);
	reverse(exponent, SEC_EXPONENT_SIZE);
	memcpy(inr, in, len);
	reverse(inr, len);

	ctx = BN_CTX_new();
	BN_init(&mod);
	BN_init(&exp);
	BN_init(&x);
	BN_init(&y);

	BN_bin2bn(modulus, modulus_size, &mod);
	BN_bin2bn(exponent, SEC_EXPONENT_SIZE, &exp);
	BN_bin2bn(inr, len, &x);
	BN_mod_exp(&y, &x, &exp, &mod, ctx);
	outlen = BN_bn2bin(&y, out);
	reverse(out, outlen);
	if (outlen < (int) modulus_size)
		memset(out + outlen, 0, modulus_size - outlen);

	BN_free(&y);
	BN_clear_free(&x);
	BN_free(&exp);
	BN_free(&mod);
	BN_CTX_free(ctx);
	
#else /* built-in crypto */

	ssl_rsa_encrypt(out, in, len, modulus_size, modulus, exponent);
	
#endif	
}

CRYPTO_CERT *
crypto_cert_read(uint8 * data, uint32 len)
{
#ifdef CRYPTO_OPENSSL
	
	/* this will move the data pointer but we don't care, we don't use it again */
	return d2i_X509(NULL, (D2I_X509_CONST unsigned char **) &data, len);
	
#else /* built-in crypto */

	return ssl_cert_read(data, len);

#endif
}

void
crypto_cert_free(CRYPTO_CERT * cert)
{
#ifdef CRYPTO_OPENSSL
	X509_free(cert);
#else /* built-in crypto */
	ssl_cert_free(cert);
#endif
}

CRYPTO_RKEY *
crypto_cert_to_rkey(CRYPTO_CERT * cert, uint32 * key_len)
{
#ifdef CRYPTO_OPENSSL
	
	int nid;
	CRYPTO_RKEY *lkey;
	EVP_PKEY *epk = NULL;

	/* For some reason, Microsoft sets the OID of the Public RSA key to
	   the oid for "MD5 with RSA Encryption" instead of "RSA Encryption"

	   Kudos to Richard Levitte for the following (intuitive)
	   lines of code that resets the OID and lets us extract the key. */
	
	nid = OBJ_obj2nid(cert->cert_info->key->algor->algorithm);
	
	if ((nid == NID_md5WithRSAEncryption) || (nid == NID_shaWithRSAEncryption))
	{
		ASN1_OBJECT_free(cert->cert_info->key->algor->algorithm);
		cert->cert_info->key->algor->algorithm = OBJ_nid2obj(NID_rsaEncryption);
	}
	
	epk = X509_get_pubkey(cert);
	
	if (NULL == epk)
		return NULL;

	lkey = RSAPublicKey_dup((RSA *) epk->pkey.ptr);
	*key_len = RSA_size(lkey);
	EVP_PKEY_free(epk);
	
	return lkey;

#else /* built-in crypto */

	return ssl_cert_to_rkey(cert, key_len);
	
#endif
}

RD_BOOL
crypto_cert_verify(CRYPTO_CERT * server_cert, CRYPTO_CERT * cacert)
{
	/* FIXME: do the actual verification */
	
#ifdef CRYPTO_OPENSSL
	return True;
#else /* built-in crypto */
	return ssl_certs_ok(server_cert, cacert);
#endif
}

int
crypto_cert_print_fp(FILE * fp, CRYPTO_CERT * cert)
{
#ifdef CRYPTO_OPENSSL
	return X509_print_fp(fp, cert);
#else /* built-in crypto */
	return ssl_cert_print_fp(fp, cert);
#endif
}

void
crypto_rkey_free(CRYPTO_RKEY * rkey)
{
#ifdef CRYPTO_OPENSSL
	RSA_free(rkey);
#else /* built-in crypto */
	ssl_rkey_free(rkey);
#endif
}

int
crypto_rkey_get_exp_mod(CRYPTO_RKEY * rkey, uint8 * exponent, uint32 max_exp_len, uint8 * modulus, uint32 max_mod_len)
{
#ifdef CRYPTO_OPENSSL
	
	int len;

	if ((BN_num_bytes(rkey->e) > (int) max_exp_len) || (BN_num_bytes(rkey->n) > (int) max_mod_len))
		return 1;
	
	len = BN_bn2bin(rkey->e, exponent);
	reverse(exponent, len);
	
	len = BN_bn2bin(rkey->n, modulus);
	reverse(modulus, len);
	
	return 0;
	
#else /* built-in crypto */

	return ssl_rkey_get_exp_mod(rkey, exponent, max_exp_len, modulus, max_mod_len);
	
#endif
}

RD_BOOL
crypto_sig_ok(uint8 * exponent, uint32 exp_len, uint8 * modulus, uint32 mod_len, uint8 * signature, uint32 sig_len)
{
	/* FIXME: check the signature for real */
	
#ifdef CRYPTO_OPENSSL
	return True;
#else /* built-in crypto */
	return True;
#endif
}

