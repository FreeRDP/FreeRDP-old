/*
   FreeRDP: A Remote Desktop Protocol client.
   OpenSSL Cryptographic Abstraction Layer

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

#include <freerdp/utils.h>
#include "frdp.h"
#include "crypto.h"
#include "crypto_openssl.h"

RD_BOOL
crypto_global_init(void)
{
	SSL_load_error_strings();
	SSL_library_init();
	return True;
}

void
crypto_global_finish(void)
{
}

CryptoSha1
crypto_sha1_init(void)
{
	CryptoSha1 sha1 = xmalloc(sizeof(*sha1));
	SHA1_Init(&sha1->sha_ctx);
	return sha1;
}

void
crypto_sha1_update(CryptoSha1 sha1, uint8 * data, uint32 len)
{
	SHA1_Update(&sha1->sha_ctx, data, len);
}

void
crypto_sha1_final(CryptoSha1 sha1, uint8 * out_data)
{
	SHA1_Final(out_data, &sha1->sha_ctx);
	xfree(sha1);
}

CryptoMd5
crypto_md5_init(void)
{
	CryptoMd5 md5 = xmalloc(sizeof(*md5));
	MD5_Init(&md5->md5_ctx);
	return md5;
}

void
crypto_md5_update(CryptoMd5 md5, uint8 * data, uint32 len)
{
	MD5_Update(&md5->md5_ctx, data, len);
}

void
crypto_md5_final(CryptoMd5 md5, uint8 * out_data)
{
	MD5_Final(out_data, &md5->md5_ctx);
	xfree(md5);
}

CryptoRc4
crypto_rc4_init(uint8 * key, uint32 len)
{
	CryptoRc4 rc4 = xmalloc(sizeof(*rc4));
	RC4_set_key(&rc4->rc4_key, len, key);
	return rc4;
}

void
crypto_rc4(CryptoRc4 rc4, uint32 len, uint8 * in_data, uint8 * out_data)
{
	RC4(&rc4->rc4_key, len, in_data, out_data);
}

void
crypto_rc4_free(CryptoRc4 rc4)
{
	xfree(rc4);
}

CryptoCert
crypto_cert_read(uint8 * data, uint32 len)
{
	CryptoCert cert = xmalloc(sizeof(*cert));
	/* this will move the data pointer but we don't care, we don't use it again */
	cert->px509 = d2i_X509(NULL, (D2I_X509_CONST unsigned char **) &data, len);
	return cert;
}

void
crypto_cert_free(CryptoCert cert)
{
	X509_free(cert->px509);
	xfree(cert);
}

RD_BOOL
crypto_cert_verify(CryptoCert server_cert, CryptoCert cacert)
{
	/* FIXME: do the actual verification */
	return True;
}

/* check the identity in a certificate against a hostname */
RD_BOOL
crypto_cert_verify_peer_identity(CryptoCert cert, const char * peer)
{
	X509_NAME *subject_name = NULL;
	X509_NAME_ENTRY *entry = NULL;
	ASN1_STRING *asn1str = NULL;
	//GENERAL_NAMES *subjectAltNames = NULL;
	unsigned char *ustr = NULL;
	char *str = NULL;
	int i, len;

#if 0
	/* Check cert for subjectAltName extensions */
	/* things to check: ipv4/ipv6 address, hostname in normal form and in DC= form */
	i = -1;
	for (;;)
	{
		subjectAltNames = X509_get_ext_d2i(cert->px509, NID_subject_alt_name, NULL, NULL);
		if (ext == NULL)
			break;
	}n

	/* Check against ip address of server */
#endif
	/* Check commonName */
	subject_name = X509_get_subject_name(cert->px509);
	if (!subject_name)
	{
		printf("crypto_cert_verify_peer_identity: failed to get subject name\n");
		goto exit;
	}

	/* try to find a common name that matches the peer hostname */
	/* TODO: cn migth also be in DC=www,DC=redhat,DC=com format? */
	i = -1;
	for (;;)
	{
		entry = NULL;
		i = X509_NAME_get_index_by_NID(subject_name, NID_commonName, i);
		if (i == -1)
			break;
		entry = X509_NAME_get_entry(subject_name, i);
		asn1str = X509_NAME_ENTRY_get_data(entry);
		len = ASN1_STRING_to_UTF8(&ustr, asn1str);
		str = (char *)ustr;
		if (strcmp(str, peer) == 0)
			break;	/* found a match */
	}

	if (!entry)
	{
		printf("crypto_cert_verify_peer_identity: certificate belongs to %s, "
			"but connection is to %s\n", str ? str : "unknown id", peer);
		return False;
	}
exit:
	return True;
}

int
crypto_cert_print_fp(FILE * fp, CryptoCert cert)
{
	return X509_print_fp(fp, cert->px509);
}

int
crypto_cert_get_pub_exp_mod(CryptoCert cert, uint32 * key_len,
		uint8 * exponent, uint32 exp_len, uint8 * modulus, uint32 mod_len)
{
	int len;
	int nid;
	EVP_PKEY *epk = NULL;

	/* For some reason, Microsoft sets the OID of the Public RSA key to
	   the oid for "MD5 with RSA Encryption" instead of "RSA Encryption"

	   Kudos to Richard Levitte for the following (intuitive)
	   lines of code that resets the OID and lets us extract the key. */

	nid = OBJ_obj2nid(cert->px509->cert_info->key->algor->algorithm);

	if ((nid == NID_md5WithRSAEncryption) || (nid == NID_shaWithRSAEncryption))
	{
		ASN1_OBJECT_free(cert->px509->cert_info->key->algor->algorithm);
		cert->px509->cert_info->key->algor->algorithm = OBJ_nid2obj(NID_rsaEncryption);
	}

	epk = X509_get_pubkey(cert->px509);

	if (NULL == epk)
		return 1;

	if ((BN_num_bytes(((RSA *) epk->pkey.ptr)->e) > (int) exp_len) ||
			(BN_num_bytes(((RSA *) epk->pkey.ptr)->n) > (int) mod_len))
		return 1;

	*key_len = RSA_size((RSA *) epk->pkey.ptr);

	len = BN_bn2bin(((RSA *) epk->pkey.ptr)->e, exponent);
	// assert len <= exp_len
	memmove(exponent + exp_len - len, exponent, len);
	memset(exponent, 0, exp_len - len);

	// assert *key_len <= mod_len
	len = BN_bn2bin(((RSA *) epk->pkey.ptr)->n, modulus);
	// assert len <= mod_len
	// assert len == *key_len
	memmove(modulus + *key_len - len, modulus, len);
	memset(modulus, 0, *key_len - len);

	EVP_PKEY_free(epk);

	return 0;
}

char *
crypto_cert_get_subject(CryptoCert cert)
{
	return X509_NAME_oneline(X509_get_subject_name(cert->px509), NULL, 0);
}

char *
crypto_cert_get_issuer(CryptoCert cert)
{
	return X509_NAME_oneline(X509_get_issuer_name(cert->px509), NULL, 0);
}

char *
crypto_cert_get_fingerprint(CryptoCert cert)
{
	unsigned char fp_buf[20];
	unsigned int fp_len = sizeof(fp_buf);
	int i;
	char * fingerprint;
	char * p;

	X509_digest(cert->px509, EVP_sha1(), fp_buf, &fp_len);
	fingerprint = malloc(fp_len * 3);
	for (i = 0, p = fingerprint; i < fp_len; i++)
	{
		sprintf(p, "%02x", fp_buf[i]);
		p += 2;
		if (i < fp_len - 1)
			*p++ = ':';
	}
	*p = '\0';

	return fingerprint;
}

int
crypto_cert_get_public_key(CryptoCert cert, DATABLOB * public_key)
{
	int length;
	int success = 1;
	EVP_PKEY *pkey = NULL;
	unsigned char *p;

	pkey = X509_get_pubkey(cert->px509);
	if (!pkey)
	{
		printf("crypto_cert_get_public_key: X509_get_pubkey() failed\n");
		success = 0;
		goto exit;
	}

	length = i2d_PublicKey(pkey, NULL);

	if (length < 1)
	{
		printf("crypto_cert_get_public_key: i2d_PublicKey() failed\n");
		success = 0;
		goto exit;
	}

	datablob_alloc(public_key, length);
	p = (unsigned char*) public_key->data;
	i2d_PublicKey(pkey, &p);

exit:
	if (pkey)
		EVP_PKEY_free(pkey);

	return success;
}

void
crypto_rsa_encrypt(int len, uint8 * in, uint8 * out, uint32 modulus_size, uint8 * modulus, uint8 * exponent)
{
	BN_CTX *ctx;
	BIGNUM mod, exp, x, y;
	int outlen;

	ctx = BN_CTX_new();
	BN_init(&mod);
	BN_init(&exp);
	BN_init(&x);
	BN_init(&y);

	BN_bin2bn(modulus, modulus_size, &mod);
	BN_bin2bn(exponent, SEC_EXPONENT_SIZE, &exp);
	BN_bin2bn(in, len, &x);
	BN_mod_exp(&y, &x, &exp, &mod, ctx);
	outlen = BN_bn2bin(&y, out);
	/* ASSERT(outlen == modulus_size); */

	BN_free(&y);
	BN_clear_free(&x);
	BN_free(&exp);
	BN_free(&mod);
	BN_CTX_free(ctx);
}

void
crypto_nonce(uint8 * nonce, int size)
{
	RAND_bytes((void*) nonce, size);
}

