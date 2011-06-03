/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP licensing negotiation

   Copyright (C) Matthew Chapman 1999-2008

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

#include "frdp.h"
#include "crypto.h"
#include "secure.h"
#include "rdp.h"
#include <freerdp/rdpset.h>
#include <freerdp/utils/memory.h>

#include "licence.h"

/* Generate a session key and RC4 keys, given client and server randoms */
static void
licence_generate_keys(rdpLicence * licence, uint8 * client_random, uint8 * server_random,
		      uint8 * pre_master_secret)
{
	uint8 master_secret[48];
	uint8 key_block[48];

	/* Generate master secret and then key material */
	sec_hash_48(master_secret, pre_master_secret, client_random, server_random, 'A');
	sec_hash_48(key_block, master_secret, server_random, client_random, 'A');

	/* Store first 16 bytes of session key as MAC secret */
	memcpy(licence->licence_sign_key, key_block, 16);

	/* Generate RC4 key from next 16 bytes */
	sec_hash_16(licence->licence_key, &key_block[16], client_random, server_random);
}

static void
licence_generate_hwid(rdpLicence * licence, uint8 * hwid)
{
	buf_out_uint32(hwid, 2);
	strncpy((char *) (hwid + 4), licence->sec->rdp->settings->hostname, LICENCE_HWID_SIZE - 4);
}

/* Send a Licensing packet with Client License Information */
static void
licence_present(rdpLicence * licence, uint8 * client_random, uint8 * rsa_data,
		uint8 * licence_data, int licence_size, uint8 * hwid, uint8 * signature)
{
	uint32 sec_flags = SEC_LICENSE_PKT;
	uint16 length =
		16 + SEC_RANDOM_SIZE + SEC_MODULUS_SIZE + SEC_PADDING_SIZE +
		licence_size + LICENCE_HWID_SIZE + LICENCE_SIGNATURE_SIZE;
	STREAM s;

	s = sec_init(licence->sec, sec_flags, length + 4);

	/* Licensing Preamble (LICENSE_PREAMBLE) */
	out_uint8(s, LICENSE_INFO);	/* bMsgType LICENSE_INFO */
	out_uint8(s, 2);	/* bVersion PREAMBLE_VERSION_2_0 */
	out_uint16_le(s, length);

	/* Client License Information: */
	out_uint32_le(s, 1);	/* PreferredKeyExchangeAlg KEY_EXCHANGE_ALG_RSA */
	out_uint16_le(s, 0);	/* PlatformId, unknown platform and ISV */
	out_uint16_le(s, 0x0201);	/* PlatformId, build/version */

	out_uint8p(s, client_random, SEC_RANDOM_SIZE);	/* ClientRandom */

	/* Licensing Binary Blob with EncryptedPreMasterSecret: */
	out_uint16_le(s, 0);	/* wBlobType should be 0x0002 (BB_RANDOM_BLOB) */
	out_uint16_le(s, (SEC_MODULUS_SIZE + SEC_PADDING_SIZE));	/* wBlobLen */
	out_uint8p(s, rsa_data, SEC_MODULUS_SIZE);	/* 48 bit random number encrypted for server */
	out_uint8s(s, SEC_PADDING_SIZE);

	/* Licensing Binary Blob with LicenseInfo: */
	out_uint16_le(s, 1);	/* wBlobType BB_DATA_BLOB */
	out_uint16_le(s, licence_size);	/* wBlobLen */
	out_uint8p(s, licence_data, licence_size);	/* CAL issued by servers license server */

	/* Licensing Binary Blob with EncryptedHWID */
	out_uint16_le(s, 1);	/* wBlobType BB_DATA_BLOB */
	out_uint16_le(s, LICENCE_HWID_SIZE);	/* wBlobLen */
	out_uint8p(s, hwid, LICENCE_HWID_SIZE);	/* RC4-encrypted Client Hardware Identification */

	out_uint8p(s, signature, LICENCE_SIGNATURE_SIZE);	/* MACData */

	s_mark_end(s);
	sec_send(licence->sec, s, sec_flags);
}

/* Send a Licensing packet with Client New License Request */
static void
licence_send_request(rdpLicence * licence, uint8 * client_random, uint8 * rsa_data, char *user,
		     char *host)
{
	uint32 sec_flags = SEC_LICENSE_PKT;
	uint16 userlen = strlen(user) + 1;
	uint16 hostlen = strlen(host) + 1;
	uint16 length = 128 + userlen + hostlen;
	STREAM s;

	s = sec_init(licence->sec, sec_flags, length + 2);

	/* Licensing Preamble (LICENSE_PREAMBLE) */
	out_uint8(s, NEW_LICENSE_REQUEST);	/* NEW_LICENSE_REQUEST */
	out_uint8(s, 2);	/* PREAMBLE_VERSION_2_0 */
	out_uint16_le(s, length);

	out_uint32_le(s, 1);	/* PreferredKeyExchangeAlg KEY_EXCHANGE_ALG_RSA */
	out_uint16_le(s, 0);	/* PlatformId, unknown platform and ISV */
	out_uint16_le(s, 0xff01);	/* PlatformId, build/version */

	out_uint8p(s, client_random, SEC_RANDOM_SIZE);	/* ClientRandom */

	/* Licensing Binary Blob with EncryptedPreMasterSecret: */
	out_uint16_le(s, 0);	/* wBlobType should be 0x0002 (BB_RANDOM_BLOB) */
	out_uint16_le(s, (SEC_MODULUS_SIZE + SEC_PADDING_SIZE));	/* wBlobLen */
	out_uint8p(s, rsa_data, SEC_MODULUS_SIZE);	/* 48 bit random number encrypted for server */
	out_uint8s(s, SEC_PADDING_SIZE);

	/* Licensing Binary Blob with ClientUserName: */
	out_uint16_le(s, LICENCE_TAG_USER);	/* wBlobType BB_CLIENT_USER_NAME_BLOB */
	out_uint16_le(s, userlen);	/* wBlobLen */
	out_uint8p(s, user, userlen);

	/* Licensing Binary Blob with ClientMachineName: */
	out_uint16_le(s, LICENCE_TAG_HOST);	/* wBlobType BB_CLIENT_MACHINE_NAME_BLOB */
	out_uint16_le(s, hostlen);	/* wBlobLen */
	out_uint8p(s, host, hostlen);

	s_mark_end(s);
	sec_send(licence->sec, s, sec_flags);
}

/* Process a Server License Request packet */
static void
licence_process_request(rdpLicence * licence, STREAM s)
{
	uint8 null_data[SEC_MODULUS_SIZE];
	uint8 *server_random;
	uint32 dwVersion;
	uint32 cbCompanyName;
	uint32 cbProductId;
	uint16 wBlobType, wBlobLen;
	uint32 ScopeCount, i;

	uint8 signature[LICENCE_SIGNATURE_SIZE];
	uint8 hwid[LICENCE_HWID_SIZE];
	uint8 *licence_data;
	int licence_size;
	CryptoRc4 crypt_key;

	/* Retrieve the server random from the incoming packet */
	in_uint8p(s, server_random, SEC_RANDOM_SIZE);	/* ServerRandom */
	/* ProductInfo: */
	in_uint32_le(s, dwVersion);
	in_uint32_le(s, cbCompanyName);
	in_uint8s(s, cbCompanyName);	/* pbCompanyName */
	in_uint32_le(s, cbProductId);
	in_uint8s(s, cbProductId);	/* pbProductId - "A02"? */
	/* KeyExchangeList */
	in_uint16_le(s, wBlobType);	/* BB_KEY_EXCHG_ALG_BLOB (0x000D) */
	in_uint16_le(s, wBlobLen);
	in_uint8s(s, wBlobLen);	/* KEY_EXCHANGE_ALG_RSA 0x00000001 */
	/* ServerCertificate */
	in_uint16_le(s, wBlobType);	/* BB_CERTIFICATE_BLOB (0x0003). */
	in_uint16_le(s, wBlobLen);
	in_uint8s(s, wBlobLen);	/* cert to use for licensing instead of the one from MCS Connect Response */
	/* ScopeList */
	in_uint32_le(s, ScopeCount);
	for (i=0; i<ScopeCount; i++)
	{
		in_uint16_le(s, wBlobType);
		in_uint16_le(s, wBlobLen);
		in_uint8s(s, wBlobLen);
	}

	/* We currently use null client keys. This is a bit naughty but, hey,
	   the security of licence negotiation isn't exactly paramount. */
	memset(null_data, 0, sizeof(null_data));
	licence_generate_keys(licence, null_data, server_random, null_data);

	licence_size = load_licence(&licence_data);
	if (licence_size > 0)
	{
		/* Generate a signature for the HWID buffer */
		licence_generate_hwid(licence, hwid);
		sec_sign(signature, 16, licence->licence_sign_key, 16, hwid, sizeof(hwid));

		/* Now encrypt the HWID */
		crypt_key = crypto_rc4_init(licence->licence_key, 16);
		crypto_rc4(crypt_key, sizeof(hwid), hwid, hwid);
		crypto_rc4_free(crypt_key);

		licence_present(licence, null_data, null_data, licence_data, licence_size, hwid, signature);
		xfree(licence_data);
		return;
	}

	licence_send_request(licence, null_data, null_data,
			     licence->sec->rdp->settings->username,
			     licence->sec->rdp->settings->hostname);
}

/* Send a Licensing packet with Platform Challenge Response */
static void
licence_send_authresp(rdpLicence * licence, uint8 * token, uint8 * crypt_hwid, uint8 * signature)
{
	uint32 sec_flags = SEC_LICENSE_PKT;
	uint16 length = 58;
	STREAM s;

	s = sec_init(licence->sec, sec_flags, length + 2);

	/* Licensing Preamble (LICENSE_PREAMBLE) */
	out_uint8(s, PLATFORM_CHALLENGE_RESPONSE);	/* PLATFORM_CHALLENGE_RESPONSE */
	out_uint8(s, 2);	/* PREAMBLE_VERSION_2_0 */
	out_uint16_le(s, length);

	/* Licensing Binary BLOB with EncryptedPlatformChallengeResponse: */
	out_uint16_le(s, 1);	/* wBlobType should be 0x0009 (BB_ENCRYPTED_DATA_BLOB) */
	out_uint16_le(s, LICENCE_TOKEN_SIZE);	/* wBlobLen */
	out_uint8p(s, token, LICENCE_TOKEN_SIZE);	/* RC4-encrypted challenge data */

	/* Licensing Binary BLOB with EncryptedHWID: */
	out_uint16_le(s, 1);	/* wBlobType should be 0x0009 (BB_ENCRYPTED_DATA_BLOB) */
	out_uint16_le(s, LICENCE_HWID_SIZE);	/* wBlobLen */
	out_uint8p(s, crypt_hwid, LICENCE_HWID_SIZE);	/* RC4-encrypted Client Hardware Identification */

	out_uint8p(s, signature, LICENCE_SIGNATURE_SIZE);	/* MACData */

	s_mark_end(s);
	sec_send(licence->sec, s, sec_flags);
}

/* Parse a Server Platform Challenge packet */
static RD_BOOL
licence_parse_authreq(rdpLicence * licence, STREAM s, uint8 ** token, uint8 ** signature)
{
	uint16 tokenlen;

	in_uint8s(s, 4);	/* ConnectFlags (unused) */

	/* Licensing Binary BLOB with EncryptedPlatformChallenge: */
	in_uint8s(s, 2);	/* wBlobType (unused) */
	in_uint16_le(s, tokenlen);	/* wBlobLen */
	if (tokenlen != LICENCE_TOKEN_SIZE)
	{
		ui_error(licence->sec->rdp->inst, "token len %d\n", tokenlen);
		return False;
	}
	in_uint8p(s, *token, tokenlen);	/* RC4-encrypted challenge data */

	in_uint8p(s, *signature, LICENCE_SIGNATURE_SIZE);	/* MACData for decrypted challenge data */

	return s_check_end(s);
}

/* Process a Server Platform Challenge packet */
static void
licence_process_platform_challenge(rdpLicence * licence, STREAM s)
{
	uint8 *in_token = NULL, *in_sig;
	uint8 out_token[LICENCE_TOKEN_SIZE], decrypt_token[LICENCE_TOKEN_SIZE];
	uint8 hwid[LICENCE_HWID_SIZE], crypt_hwid[LICENCE_HWID_SIZE];
	uint8 sealed_buffer[LICENCE_TOKEN_SIZE + LICENCE_HWID_SIZE];
	uint8 out_sig[LICENCE_SIGNATURE_SIZE];
	CryptoRc4 crypt_key;

	/* Parse incoming packet and save the encrypted token */
	licence_parse_authreq(licence, s, &in_token, &in_sig);
	memcpy(out_token, in_token, LICENCE_TOKEN_SIZE);

	/* Decrypt the token. It should read TEST in Unicode. */
	crypt_key = crypto_rc4_init(licence->licence_key, 16);
	crypto_rc4(crypt_key, LICENCE_TOKEN_SIZE, in_token, decrypt_token);
	crypto_rc4_free(crypt_key);

	/* Generate a signature for a buffer of token and HWID */
	licence_generate_hwid(licence, hwid);
	memcpy(sealed_buffer, decrypt_token, LICENCE_TOKEN_SIZE);
	memcpy(sealed_buffer + LICENCE_TOKEN_SIZE, hwid, LICENCE_HWID_SIZE);
	sec_sign(out_sig, 16, licence->licence_sign_key, 16, sealed_buffer, sizeof(sealed_buffer));

	/* Now encrypt the HWID */
	crypt_key = crypto_rc4_init(licence->licence_key, 16);
	crypto_rc4(crypt_key, LICENCE_HWID_SIZE, hwid, crypt_hwid);
	crypto_rc4_free(crypt_key);

	licence_send_authresp(licence, out_token, crypt_hwid, out_sig);
}

/* Process a Server New (or Upgrade) License packet */
static void
licence_process_new_license(rdpLicence * licence, STREAM s)
{
	int i;
	uint32 length;
	uint32 os_major;
	uint32 os_minor;
	CryptoRc4 crypt_key;

	/* Licensing Binary BLOB with EncryptedLicenseInfo: */
	in_uint8s(s, 2);	/* wBlobType should be 0x0009 (BB_ENCRYPTED_DATA_BLOB) */
	in_uint16_le(s, length);	/* wBlobLen */

	/* RC4-encrypted New License Information */
	if (!s_check_rem(s, length))
		return;

	crypt_key = crypto_rc4_init(licence->licence_key, 16);
	crypto_rc4(crypt_key, length, s->p, s->p);	/* decrypt in place */
	crypto_rc4_free(crypt_key);

	/* dwVersion */
	in_uint16_le(s, os_major);	/* OS major version */
	in_uint16_le(s, os_minor);	/* OS minor version */

	/* Skip Scope, CompanyName and ProductId */
	for (i = 0; i < 3; i++)
	{
		in_uint32_le(s, length);
		if (!s_check_rem(s, length))
			return;
		in_uint8s(s, length);
	}

	/* LicenseInfo - CAL from license server */
	in_uint32_le(s, length);
	if (!s_check_rem(s, length))
		return;
	licence->licence_issued = True;
	save_licence(s->p, length);
}

/* Process a Licensing packet */
void
licence_process(rdpLicence * licence, STREAM s)
{
	uint8 tag;
	uint16 wMsgSize;
	uint8* license_start = s->p;

	/* Licensing Preamble */
	in_uint8(s, tag);	/* bMsgType */
	in_uint8s(s, 1);	/* Ignoring bVersion */
	in_uint16_le(s, wMsgSize);
	/* Now pointing at LicensingMessage */

	switch (tag)
	{
		case LICENSE_REQUEST:
			DEBUG_LICENSE("LICENSE_REQUEST\n");
			licence_process_request(licence, s);
			ASSERT(s->p == license_start + wMsgSize);
			break;

		case LICENSE_PLATFORM_CHALLENGE:
			DEBUG_LICENSE("LICENCE PLATFORM_CHALLENGE\n");
			licence_process_platform_challenge(licence, s);
			break;

		case NEW_LICENSE:
			DEBUG_LICENSE("NEW_LICENSE\n");
			licence_process_new_license(licence, s);
			break;

		case UPGRADE_LICENSE:
			DEBUG_LICENSE("UPGRADE_LICENSE\n");
			break;

		case LICENCE_ERROR_ALERT:
			DEBUG_LICENSE("LICENCE ERROR_ALERT - assuming it is a license grant\n");
			{
				uint32 dwErrorCode, dwStateTransition;
				uint32 wBlobType, wBlobLen;
				in_uint32_le(s, dwErrorCode);
				in_uint32_le(s, dwStateTransition);
				DEBUG_LICENSE("dwErrorCode %x dwStateTransition %x\n", dwErrorCode, dwStateTransition);
				in_uint16_le(s, wBlobType);
				in_uint16_le(s, wBlobLen);
				DEBUG_LICENSE("bbErrorInfo: wBlobType %x wBlobLen %x\n", wBlobType, wBlobLen);
				/* hexdump(s->p, wBlobLen); */
			}
			licence->licence_issued = True;	/* TODO ... */
			break;

		default:
			ui_unimpl(licence->sec->rdp->inst, "Unknown licence tag 0x%x\n", tag);
	}
	s->p = license_start + wMsgSize;	/* FIXME: Shouldn't be necessary if parsed properly */
	ASSERT(s->p <= s->end);
}

rdpLicence *
licence_new(struct rdp_sec *sec)
{
	rdpLicence *self;

	self = (rdpLicence *) xmalloc(sizeof(rdpLicence));
	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpLicence));
		self->sec = sec;
	}
	return self;
}

void
licence_free(rdpLicence * licence)
{
	if (licence != NULL)
	{
		xfree(licence);
	}
}
