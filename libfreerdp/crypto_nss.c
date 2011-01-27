/* -*- c-basic-offset: 8 -*-
 FreeRDP: A Remote Desktop Protocol client.
 NSS Cryptographic Abstraction Layer

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

/* Define PROTYPES_H to disable obsolete/protypes.h and avoid uint8 collisions */
#define PROTYPES_H
#include <nss.h>
#include <prerror.h>
#include <keyhi.h>
#include <secasn1t.h>
#include <pk11pub.h>
#include <seccomon.h>
#include <cert.h>
#include <prinit.h>

RD_BOOL
crypto_global_init(void)
{
	SECStatus s = NSS_NoDB_Init(NULL);
	return s == SECSuccess ? True : False;
}

void
crypto_global_finish(void)
{
	NSS_Shutdown();
	PL_ArenaFinish();
	PR_Cleanup();
}

static
void check(SECStatus s, char* msg) {
	if (s != SECSuccess) {
		/* http://www.mozilla.org/projects/security/pki/nss/ref/ssl/sslerr.html */
		printf("NSS error %d: %s\n", PR_GetError(), msg);
		ASSERT(s == SECSuccess);
	}
}

struct crypto_sha1_struct
{
	PK11Context * context;
};

CryptoSha1
crypto_sha1_init(void)
{
	CryptoSha1 sha1 = xmalloc(sizeof(*sha1));
	sha1->context = PK11_CreateDigestContext(SEC_OID_SHA1);
	SECStatus s = PK11_DigestBegin(sha1->context);
	check(s, "Error initializing sha1");
	return sha1;
}

void
crypto_sha1_update(CryptoSha1 sha1, uint8 * data, uint32 len)
{
	SECStatus s = PK11_DigestOp(sha1->context, data, len);
	check(s, "Error updating sha1");
}

void
crypto_sha1_final(CryptoSha1 sha1, uint8 * out_data)
{
	unsigned int len;
	SECStatus s = PK11_DigestFinal(sha1->context, out_data, &len, 20);
	check(s, "Error finalizing sha1");
	ASSERT(len == 20);
	PK11_DestroyContext(sha1->context, PR_TRUE);
	xfree(sha1);
}

struct crypto_md5_struct
{
	PK11Context * context;
};

CryptoMd5
crypto_md5_init(void)
{
	CryptoMd5 md5 = xmalloc(sizeof(*md5));
	md5->context = PK11_CreateDigestContext(SEC_OID_MD5);
	SECStatus s = PK11_DigestBegin(md5->context);
	check(s, "Error initializing md5");
	return md5;
}

void
crypto_md5_update(CryptoMd5 md5, uint8 * data, uint32 len)
{
	SECStatus s = PK11_DigestOp(md5->context, data, len);
	check(s, "Error updating md5");
}

void
crypto_md5_final(CryptoMd5 md5, uint8 * out_data)
{
	unsigned int len;
	SECStatus s = PK11_DigestFinal(md5->context, out_data, &len, 16);
	check(s, "Error finalizing md5");
	ASSERT(len == 16);
	PK11_DestroyContext(md5->context, PR_TRUE);
	xfree(md5);
}

struct crypto_rc4_struct
{
	PK11Context * context;
};

CryptoRc4
crypto_rc4_init(uint8 * key, uint32 len)
{
	CryptoRc4 rc4 = xmalloc(sizeof(*rc4));
	CK_MECHANISM_TYPE cipherMech = CKM_RC4;

	PK11SlotInfo* slot = PK11_GetInternalKeySlot();
	ASSERT(slot);

	SECItem keyItem;
	keyItem.type = siBuffer;
	keyItem.data = key;
	keyItem.len = len;

	PK11SymKey* symKey = PK11_ImportSymKey(slot, cipherMech, PK11_OriginUnwrap, CKA_ENCRYPT, &keyItem, NULL);
	ASSERT(symKey);

	SECItem* secParam = PK11_ParamFromIV(cipherMech, NULL);
	ASSERT(secParam);

	rc4->context = PK11_CreateContextBySymKey(cipherMech, CKA_ENCRYPT, symKey, secParam);
	ASSERT(rc4->context);

	PK11_FreeSymKey(symKey);
	SECITEM_FreeItem(secParam, PR_TRUE);
	PK11_FreeSlot(slot);

	return rc4;
}

void
crypto_rc4(CryptoRc4 rc4, uint32 len, uint8 * in_data, uint8 * out_data)
{
	int outlen;
	/* valgrind "Invalid read"? See http://groups.google.com/group/mozilla.dev.tech.crypto/browse_thread/thread/361c017b4aa5226f/43badd163bef22f2 */
	SECStatus s = PK11_CipherOp(rc4->context, out_data, &outlen, len, in_data, len);
	check(s, "Error in rc4 encryption");
	ASSERT(outlen == len);
}

void
crypto_rc4_free(CryptoRc4 rc4)
{
	unsigned int outLen;
	SECStatus s = PK11_DigestFinal(rc4->context, NULL, &outLen, 0);
	check(s, "Error finalizing rc4");
	ASSERT(!outLen);
	PK11_DestroyContext(rc4->context, PR_TRUE);
	xfree(rc4);
}

struct crypto_cert_struct
{
	CERTCertificate * cert;
};

CryptoCert
crypto_cert_read(uint8 * data, uint32 len)
{
	CryptoCert crypto_cert = xmalloc(sizeof(*crypto_cert));

	CERTCertDBHandle * handle = CERT_GetDefaultCertDB();
	SECItem derCert;
	derCert.type = siBuffer;
	derCert.data = data;
	derCert.len = len;
	crypto_cert->cert = CERT_NewTempCertificate(handle, &derCert, NULL, PR_FALSE, PR_TRUE);
	ASSERT(crypto_cert->cert);

	return crypto_cert;
}

void
crypto_cert_free(CryptoCert cert)
{
	CERT_DestroyCertificate(cert->cert);
	xfree(cert);
}

RD_BOOL
crypto_cert_verify(CryptoCert server_cert, CryptoCert cacert)
{
	/* TODO: Use CERT_VerifySignedDataWithPublicKeyInfo or PK11_Verify */
	return True;
}

int
crypto_cert_print_fp(FILE * fp, CryptoCert cert)
{
	/* TODO: Do more than this ... */
	fprintf(fp, "subject: %s\n", cert->cert->subjectName);
	return True;
}

int
crypto_cert_get_pub_exp_mod(CryptoCert cert, uint32 * key_len,
		uint8 * exponent, uint32 exp_len, uint8 * modulus, uint32 mod_len)
{
	SECKEYPublicKey * pubkey;

	SECOidTag tag = SECOID_GetAlgorithmTag(&cert->cert->subjectPublicKeyInfo.algorithm);
	if ((tag == SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION) || (tag == SEC_OID_ISO_SHA_WITH_RSA_SIGNATURE))
	{
		/* For some reason, Microsoft sets the OID of the Public RSA key to
		 the oid for "MD5 with RSA Encryption" instead of "RSA Encryption". */
		SECAlgorithmID org = cert->cert->subjectPublicKeyInfo.algorithm;
		SECStatus s = SECOID_SetAlgorithmID(cert->cert->subjectPublicKeyInfo.arena,
				&cert->cert->subjectPublicKeyInfo.algorithm,
				SEC_OID_PKCS1_RSA_ENCRYPTION, NULL);
		check(s, "Error setting temp algo oid");
		pubkey = SECKEY_ExtractPublicKey(&cert->cert->subjectPublicKeyInfo);
		SECOID_DestroyAlgorithmID(&cert->cert->subjectPublicKeyInfo.algorithm, False);
		cert->cert->subjectPublicKeyInfo.algorithm = org;
	}
	else
	{
		pubkey = SECKEY_ExtractPublicKey(&cert->cert->subjectPublicKeyInfo);
	}
	ASSERT(pubkey);
	ASSERT(pubkey->keyType == rsaKey);

	*key_len = SECKEY_PublicKeyStrength(pubkey);

	size_t l = pubkey->u.rsa.publicExponent.len;
	ASSERT(l <= exp_len);
	memset(exponent, 0, exp_len - l);
	memcpy(exponent + exp_len - l, pubkey->u.rsa.publicExponent.data, l);

	l = pubkey->u.rsa.modulus.len;
	ASSERT(l <= mod_len);
	ASSERT(*key_len <= mod_len);
	memset(modulus, 0, *key_len - l);
	memcpy(modulus + *key_len - l, pubkey->u.rsa.modulus.data, l);

	SECKEY_DestroyPublicKey(pubkey);
	return 0;
}

void
crypto_rsa_encrypt(int len, uint8 * in, uint8 * out, uint32 modulus_size, uint8 * modulus, uint8 * exponent)
{
	SECKEYPublicKey pubKey;
	pubKey.arena = NULL;
	pubKey.keyType = rsaKey;
	pubKey.pkcs11Slot = NULL;
	pubKey.pkcs11ID = CK_INVALID_HANDLE;
	pubKey.u.rsa.arena = NULL;
	pubKey.u.rsa.modulus.type = siUnsignedInteger;
	pubKey.u.rsa.modulus.data = modulus;
	pubKey.u.rsa.modulus.len = modulus_size;
	pubKey.u.rsa.publicExponent.type = siUnsignedInteger;
	pubKey.u.rsa.publicExponent.data = exponent;
	pubKey.u.rsa.publicExponent.len = SEC_EXPONENT_SIZE;

	ASSERT(modulus_size <= SEC_MAX_MODULUS_SIZE);
	uint8 in_be[SEC_MAX_MODULUS_SIZE];
	memset(in_be, 0, modulus_size - len); /* must be padded to modulus_size */
	memcpy(in_be + modulus_size - len, in, len);

	SECStatus s = PK11_PubEncryptRaw(&pubKey, out, in_be, modulus_size, NULL);
	check(s, "Error rsa-encrypting");

	ASSERT(pubKey.pkcs11Slot);
	PK11_FreeSlot(pubKey.pkcs11Slot);
}
