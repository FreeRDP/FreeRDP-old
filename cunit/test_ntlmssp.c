
#include <stdio.h>
#include <stdlib.h>
#include <freerdp/freerdp.h>
#include "ntlmssp.h"
#include "test_ntlmssp.h"

int init_ntlmssp_suite(void)
{
	return 0;
}

int clean_ntlmssp_suite(void)
{
	return 0;
}

int add_ntlmssp_suite(void)
{
	add_test_suite(ntlmssp);

	add_test_function(ntlmssp_compute_lm_hash);
	add_test_function(ntlmssp_compute_ntlm_hash);
	add_test_function(ntlmssp_compute_ntlm_v2_hash);
	add_test_function(ntlmssp_compute_lm_response);
	add_test_function(ntlmssp_compute_lm_v2_response);
	add_test_function(ntlmssp_compute_ntlm_v2_response);
	add_test_function(ntlmssp_generate_client_signing_key);
	add_test_function(ntlmssp_generate_client_sealing_key);
	add_test_function(ntlmssp_encrypt_random_session_key);
	add_test_function(ntlmssp_encrypt_message);

	return 0;
}

void test_ntlmssp_compute_lm_hash(void)
{
	int i;
	int lm_hash_good;
	char lm_hash[16];
	char password[] = "Password";
	char expected_lm_hash[16] = "\xe5\x2c\xac\x67\x41\x9a\x9a\x22\x4a\x3b\x10\x8f\x3f\xa6\xcb\x6d";

	ntlmssp_compute_lm_hash(password, lm_hash);

	lm_hash_good = 1;
	for (i = 0; i < 16; i++) {
		if (lm_hash[i] != expected_lm_hash[i])
			lm_hash_good = 0;
	}

	CU_ASSERT(lm_hash_good == 1);
}

void test_ntlmssp_compute_ntlm_hash(void)
{
	int i;
	NTLMSSP *ntlmssp;
	int ntlm_hash_good;
	char ntlm_hash[16];
	char password[] = "Password";
	char expected_ntlm_hash[16] = "\xa4\xf4\x9c\x40\x65\x10\xbd\xca\xb6\x82\x4e\xe7\xc3\x0f\xd8\x52";

	ntlmssp = ntlmssp_new();
	ntlmssp_set_password(ntlmssp, password);

	ntlmssp_compute_ntlm_hash(&ntlmssp->password, ntlm_hash);

	ntlm_hash_good = 1;
	for (i = 0; i < 16; i++) {
		if (ntlm_hash[i] != expected_ntlm_hash[i])
			ntlm_hash_good = 0;
	}

	CU_ASSERT(ntlm_hash_good == 1);
}

void test_ntlmssp_compute_ntlm_v2_hash(void)
{
	int i;
	NTLMSSP *ntlmssp;
	int ntlm_v2_hash_good;
	char ntlm_v2_hash[16];

	char username[] = "User";
	char password[] = "Password";
	char domain[] = "Domain";
	char expected_ntlm_v2_hash[16] = "\x0c\x86\x8a\x40\x3b\xfd\x7a\x93\xa3\x00\x1e\xf2\x2e\xf0\x2e\x3f";

	ntlmssp = ntlmssp_new();
	ntlmssp_set_password(ntlmssp, password);
	ntlmssp_set_username(ntlmssp, username);
	ntlmssp_set_domain(ntlmssp, domain);

	ntlmssp_compute_ntlm_v2_hash(&ntlmssp->password, &ntlmssp->username, &ntlmssp->domain, ntlm_v2_hash);

	ntlm_v2_hash_good = 1;
	for (i = 0; i < 16; i++) {
		if (ntlm_v2_hash[i] != expected_ntlm_v2_hash[i])
			ntlm_v2_hash_good = 0;
	}

	CU_ASSERT(ntlm_v2_hash_good == 1);
}

void test_ntlmssp_compute_lm_response(void)
{
	int i;
	int lm_response_good;
	char lm_response[24];
	char password[] = "SecREt01";
	char challenge[] = "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
	char expected_lm_response[24] = "\xC3\x37\xCD\x5C\xBD\x44\xFC\x97\x82\xA6\x67\xAF\x6D\x42\x7C\x6D\xE6\x7C\x20\xC2\xD3\xE7\x7C\x56";

	ntlmssp_compute_lm_response(password, challenge, lm_response);

	lm_response_good = 1;
	for (i = 0; i < 24; i++) {
		if (lm_response[i] != expected_lm_response[i])
			lm_response_good = 0;
	}

	CU_ASSERT(lm_response_good == 1);
}

void test_ntlmssp_compute_lm_v2_response(void)
{
	int i;
	char *p;
	NTLMSSP *ntlmssp;
	int lm_v2_response_good;
	char password[] = "SecREt01";
	char username[] = "user";
	char domain[] = "DOMAIN";
	char server_challenge[8] = "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
	char lm_client_challenge[8] = "\xFF\xFF\xFF\x00\x11\x22\x33\x44";
	char expected_lm_v2_response[24] = "\xD6\xE6\x15\x2E\xA2\x5D\x03\xB7\xC6\xBA\x66\x29\xC2\xD6\xAA\xF0\xFF\xFF\xFF\x00\x11\x22\x33\x44";

	ntlmssp = ntlmssp_new();
	ntlmssp_set_password(ntlmssp, password);
	ntlmssp_set_username(ntlmssp, username);
	ntlmssp_set_domain(ntlmssp, domain);

	memcpy(ntlmssp->server_challenge, server_challenge, 8);
	memcpy(ntlmssp->lm_client_challenge, lm_client_challenge, 8);

	ntlmssp_compute_lm_v2_response(ntlmssp);

	p = (char*) ntlmssp->lm_challenge_response.data;
	lm_v2_response_good = 1;
	for (i = 0; i < 24; i++) {
		if (p[i] != expected_lm_v2_response[i])
			lm_v2_response_good = 0;
	}

	CU_ASSERT(lm_v2_response_good == 1);
}

void test_ntlmssp_compute_ntlm_v2_response(void)
{
	int i;
	char* p;
	NTLMSSP *ntlmssp;
	int session_base_key_good;
	int lm_challenge_response_good;
	int nt_challenge_response_good;
	char password[] = "password";
	char username[] = "username";
	char timestamp[8] = "\x00\xce\xc6\x3d\x8a\xb7\xcb\x01";
	char lm_client_challenge[8] = "\xc3\x64\xc9\x8f\x0c\x5c\xaf\xf6";
	char nt_client_challenge[8] = "\xce\xf7\x93\xf9\x10\x29\x77\x56";
	char server_challenge[8] = "\x82\xee\x83\x3e\xd6\xfd\x88\xa8";

	char target_info_data[64] =
		"\x02\x00\x08\x00\x57\x00\x49\x00\x4e\x00\x37\x00\x01\x00\x08\x00"
		"\x57\x00\x49\x00\x4e\x00\x37\x00\x04\x00\x08\x00\x77\x00\x69\x00"
		"\x6e\x00\x37\x00\x03\x00\x08\x00\x77\x00\x69\x00\x6e\x00\x37\x00"
		"\x07\x00\x08\x00\x6e\x81\x69\x3e\x8a\xb7\xcb\x01\x00\x00\x00\x00";

	char expected_nt_challenge_response[108] =
		"\x4b\xc8\x47\x18\xf3\x59\x41\xe5\x77\x7e\x6f\x0d\x21\x08\x40\x36"
		"\x01\x01\x00\x00\x00\x00\x00\x00\x00\xce\xc6\x3d\x8a\xb7\xcb\x01"
		"\xce\xf7\x93\xf9\x10\x29\x77\x56\x00\x00\x00\x00\x02\x00\x08\x00"
		"\x57\x00\x49\x00\x4e\x00\x37\x00\x01\x00\x08\x00\x57\x00\x49\x00"
		"\x4e\x00\x37\x00\x04\x00\x08\x00\x77\x00\x69\x00\x6e\x00\x37\x00"
		"\x03\x00\x08\x00\x77\x00\x69\x00\x6e\x00\x37\x00\x07\x00\x08\x00"
		"\x6e\x81\x69\x3e\x8a\xb7\xcb\x01\x00\x00\x00\x00";

	char expected_lm_challenge_response[24] =
		"\x94\x0c\x7e\x6a\x01\x3c\x64\xee\xd6\x99\x99\x2b\x9c\xf3\x0e\x9e"
		"\xc3\x64\xc9\x8f\x0c\x5c\xaf\xf6";

	char expected_session_base_key[16] =
		"\x6f\xaf\xfe\xf5\xf4\x0c\x6c\xc8\x56\xda\xec\x5f\x33\x2f\x8d\x1d";

	ntlmssp = ntlmssp_new();
	ntlmssp_set_password(ntlmssp, password);
	ntlmssp_set_username(ntlmssp, username);
	ntlmssp_set_domain(ntlmssp, NULL);

	memcpy(ntlmssp->timestamp, timestamp, 8);
	memcpy(ntlmssp->server_challenge, server_challenge, 8);
	memcpy(ntlmssp->lm_client_challenge, lm_client_challenge, 8);
	memcpy(ntlmssp->nt_client_challenge, nt_client_challenge, 8);

	ntlmssp->target_info.data = target_info_data;
	ntlmssp->target_info.length = sizeof(target_info_data);

	ntlmssp_compute_ntlm_v2_response(ntlmssp);

	session_base_key_good = 1;
	p = (char*) ntlmssp->session_base_key;
	for (i = 0; i < 16; i++) {
		if (p[i] != expected_session_base_key[i])
			session_base_key_good = 0;
	}

	CU_ASSERT(session_base_key_good == 1);

	lm_challenge_response_good = 1;
	p = (char*) ntlmssp->lm_challenge_response.data;
	for (i = 0; i < 24; i++) {
		if (p[i] != expected_lm_challenge_response[i])
			lm_challenge_response_good = 0;
	}
	
	CU_ASSERT(lm_challenge_response_good == 1);

	nt_challenge_response_good = 1;
	p = (char*) ntlmssp->nt_challenge_response.data;
	for (i = 0; i < 84; i++) {
		if (p[i] != expected_nt_challenge_response[i])
			nt_challenge_response_good = 0;
	}

	CU_ASSERT(nt_challenge_response_good == 1);
}

void test_ntlmssp_generate_client_signing_key(void)
{
	int i;
	NTLMSSP *ntlmssp;
	int client_signing_key_good;
	uint8 exported_session_key[16] = "\x29\xdd\x4a\x88\xf8\x3a\xda\xd9\x01\x46\x56\xec\xc1\x8b\x6e\xd0";
	uint8 expected_client_signing_key[16] = "\xde\x51\x70\x09\xce\x84\x5a\xa0\x05\x0a\x4c\x76\x80\x34\xaf\x88";

	ntlmssp = ntlmssp_new();
	memcpy(ntlmssp->exported_session_key, exported_session_key, sizeof(exported_session_key));

	ntlmssp_generate_client_signing_key(ntlmssp);

	client_signing_key_good = 1;
	for (i = 0; i < 16; i++) {
		if (ntlmssp->client_signing_key[i] != expected_client_signing_key[i])
			client_signing_key_good = 0;
	}

	CU_ASSERT(client_signing_key_good == 1);
}

void test_ntlmssp_generate_client_sealing_key(void)
{
	int i;
	NTLMSSP *ntlmssp;
	int client_sealing_key_good;
	uint8 exported_session_key[16] = "\x29\xdd\x4a\x88\xf8\x3a\xda\xd9\x01\x46\x56\xec\xc1\x8b\x6e\xd0";
	uint8 expected_client_sealing_key[16] = "\xe6\x35\xff\x73\x9e\xd3\x2c\x19\xd6\xf2\x09\x00\xfd\x4b\x45\x31";

	ntlmssp = ntlmssp_new();
	memcpy(ntlmssp->exported_session_key, exported_session_key, sizeof(exported_session_key));

	ntlmssp_generate_client_sealing_key(ntlmssp);

	client_sealing_key_good = 1;
	for (i = 0; i < 16; i++) {
		if (ntlmssp->client_sealing_key[i] != expected_client_sealing_key[i])
			client_sealing_key_good = 0;
	}

	CU_ASSERT(client_sealing_key_good == 1);
}

void test_ntlmssp_encrypt_message(void)
{
	int i;
	char* p;
	NTLMSSP *ntlmssp;
	DATA_BLOB message;
	int signature_good;
	uint8 signature[16];
	int encrypted_message_good;
	DATA_BLOB encrypted_message;
	uint8 client_signing_key[16] = "\x17\x6a\x40\xf7\xc2\xc9\x6a\xf5\x19\x94\x0a\x6a\xe1\xe6\x04\xaa";
	uint8 client_sealing_key[16] = "\x0c\x90\xb6\x9f\x81\x35\x9b\x7d\x1a\x0f\x24\xf7\x19\xdc\x9c\x3d";

	uint8 message_data[270] =
		"\x30\x82\x01\x0a\x02\x82\x01\x01\x00\xc2\x1c\x54\xaf\x07\xf1\x16"
		"\x97\xc3\x0f\x6b\xa6\x33\x2e\xdd\x1e\xe4\xb2\x9c\xe4\x12\x7f\xda"
		"\x58\x21\xc0\x68\xe6\xd3\xf5\x20\x1c\xba\x06\x64\x7d\x7f\x44\xb5"
		"\xbf\xe3\xd5\xc7\xa4\x86\x8b\xbc\x6f\xca\x25\x78\xdf\xeb\xcf\x5a"
		"\x96\xf6\xc7\x00\xbe\x7d\x6d\x06\x1f\x1d\x7f\x30\xaf\xc4\x59\x4f"
		"\x91\x6d\x97\xe8\x55\x8b\x39\x01\x68\x50\x59\xbb\xe4\x65\x71\x32"
		"\x76\x9e\x1b\xcf\x58\xfc\x52\xd9\x43\x01\x8e\x33\xc1\x74\x14\xbc"
		"\x1f\x5c\x1d\xdb\x0e\xbd\xbb\x37\x50\x13\x78\x57\x93\x34\x3b\x73"
		"\xc9\x5c\x44\x1f\x16\xe6\x2e\x00\x57\xa3\xe6\x5c\x6a\x2c\x90\xdc"
		"\xa3\x6d\x7f\x92\xdf\x2f\xe5\x97\xae\x3b\x07\x23\x03\x91\x71\xd4"
		"\xf2\x50\x3a\x3a\xb9\xde\x1f\xb1\xd5\xa1\x38\x7c\xf7\x07\x49\x83"
		"\x68\xaa\xdf\xad\xfd\x1a\xe9\xb5\x0a\x1e\x8b\xf3\x88\xae\x3f\x32"
		"\xd0\x3b\xd8\xc7\x50\x11\xf7\xad\x3b\x11\xe6\x92\xbb\x2a\x73\x8b"
		"\xed\xfd\x45\x29\x50\xbf\x0d\x1e\x47\xfd\x61\x1d\x18\x27\x58\xa2"
		"\xb2\x1f\xb5\x2d\x84\x18\x2f\x88\x8e\x7f\x70\xed\x4e\xbf\x14\x5d"
		"\x1b\xbc\x0b\x47\x66\x16\x3a\x7b\x6d\x8e\xcf\x55\xe8\x8c\x8a\xfe"
		"\x24\xce\x19\x99\xc3\x5a\xe5\xc2\xf3\x02\x03\x01\x00\x01";

	uint8 expected_encrypted_message[270] =
		"\xe8\x55\xd4\x9a\x00\x9d\xc1\x86\x28\x2b\x1e\xa3\xa8\x34\xd1\x5b"
		"\x93\x98\xf9\x20\x6b\x67\x7b\x61\x58\x73\xea\x49\x13\x85\x51\xf1"
		"\x01\x8b\x52\x8e\xa7\x25\x92\x90\xa7\x04\x09\x0d\x45\xaf\xa6\x8a"
		"\x7d\xda\x8d\x5b\xd1\xd4\x06\xd9\x49\x13\xae\x45\x68\xde\x12\xef"
		"\xea\x06\x4c\x37\xe6\x31\xf4\x8e\xdf\x13\x90\x18\x36\xb1\xb3\x9f"
		"\xac\x15\x92\xef\xc9\xb0\xa1\xec\x41\x18\x29\x84\xa1\x5a\xc4\x58"
		"\xc4\x73\x07\x1e\x2e\x03\xa5\x0e\x44\xa5\x65\xfd\x6d\x4e\x42\x64"
		"\x1c\x0f\xcc\xfb\x34\x24\x94\x5f\x9c\x09\x89\xfd\x66\xe5\x7f\x86"
		"\xe5\x4f\x57\x84\xd0\xe6\x74\xe4\xfa\x7c\x07\x0a\x50\xac\x6c\x41"
		"\x42\xd7\x1c\x2b\x0f\xa1\x4d\x57\x22\x75\x54\x01\x0f\x59\x75\xc0"
		"\x79\x1f\x1d\x0f\x14\x04\x3e\x39\x93\xdb\x7e\xf7\x54\xcf\x4a\x8c"
		"\xe6\x55\xc5\x62\xaa\x8f\x80\x37\xed\xaa\x0f\x57\xd3\x0c\xfe\xcf"
		"\x55\x29\x4e\xbc\x64\x00\x18\xd5\xa7\x9b\xf5\x17\x4b\x75\xf7\xf9"
		"\x79\x93\xf3\x63\xb0\xcc\x9c\x52\xb8\xc1\x2c\x18\x89\xc3\xfb\x48"
		"\x14\xea\xb2\x41\xae\xf8\xa1\xf6\x95\x87\xba\x79\x99\x54\xeb\x9b"
		"\xa2\x24\xde\xfb\xbe\x0e\xce\xb5\x5d\x99\x1f\x9a\x12\xcc\x01\xee"
		"\xdf\x67\xef\x6d\x35\x9a\x9a\x16\xdd\x00\xd5\x43\xe4\xfb";

	uint8 expected_signature[16] =
		"\x01\x00\x00\x00\xc7\xe8\x49\x1b\x25\xd6\x6b\x56\x00\x00\x00\x00";

	message.data = message_data;
	message.length = sizeof(message);

	ntlmssp = ntlmssp_new();
	memcpy(ntlmssp->client_signing_key, client_signing_key, 16);
	memcpy(ntlmssp->client_sealing_key, client_sealing_key, 16);

	ntlmssp->rc4_seal = crypto_rc4_init(ntlmssp->client_sealing_key, 16);
	
	ntlmssp_encrypt_message(ntlmssp, &message, &encrypted_message, signature);

	p = (char*) encrypted_message.data;
	encrypted_message_good = 1;
	for (i = 0; i < encrypted_message.length; i++) {
		if (p[i] != expected_encrypted_message[i])
			encrypted_message_good = 0;
	}

	CU_ASSERT(encrypted_message_good == 1);

	signature_good = 1;
	for (i = 0; i < 16; i++) {
		if (signature[i] != expected_signature[i])
			signature_good = 0;
	}

	CU_ASSERT(signature_good == 1);
}

void test_ntlmssp_encrypt_random_session_key(void)
{
	int i;
	char* p;
	NTLMSSP *ntlmssp;
	int encrypted_random_session_key_good;
	uint8 key_exchange_key[16] = "\x8d\xe4\x0c\xca\xdb\xc1\x4a\x82\xf1\x5c\xb0\xad\x0d\xe9\x5c\xa3";
	uint8 random_session_key[16] = "\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55";
	uint8 expected_encrypted_random_session_key[16] = "\xc5\xda\xd2\x54\x4f\xc9\x79\x90\x94\xce\x1c\xe9\x0b\xc9\xd0\x3e";

	ntlmssp = ntlmssp_new();
	memcpy(ntlmssp->key_exchange_key, key_exchange_key, 16);
	memcpy(ntlmssp->random_session_key, random_session_key, 16);

	ntlmssp_encrypt_random_session_key(ntlmssp);

	encrypted_random_session_key_good = 1;
	p = (char*) ntlmssp->encrypted_random_session_key;
	for (i = 0; i < 16; i++) {
		if (p[i] != expected_encrypted_random_session_key[i])
			encrypted_random_session_key_good = 0;
	}

	CU_ASSERT(encrypted_random_session_key_good == 1);
}
