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

#ifndef __CRYPTO_OPENSSL_H
#define __CRYPTO_OPENSSL_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rc4.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/x509v3.h>
#include <openssl/rand.h>

#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x0090800f)
#define D2I_X509_CONST const
#else
#define D2I_X509_CONST
#endif

struct crypto_sha1_struct
{
	SHA_CTX sha_ctx;
};

struct crypto_md5_struct
{
	MD5_CTX md5_ctx;
};

struct crypto_rc4_struct
{
	RC4_KEY rc4_key;
};

struct crypto_cert_struct
{
	X509 * px509;
};

#endif

