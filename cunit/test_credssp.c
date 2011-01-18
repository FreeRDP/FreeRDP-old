
#include <stdio.h>
#include <stdlib.h>
#include <freerdp/freerdp.h>
#include "credssp.h"
#include "test_credssp.h"

int init_credssp_suite(void)
{
	return 0;
}

int clean_credssp_suite(void)
{
	return 0;
}

int add_credssp_suite(void)
{
	add_test_suite(credssp);

	add_test_function(credssp_lm_hash);
	add_test_function(credssp_ntlm_hash);
	add_test_function(credssp_ntlm_v2_hash);
	add_test_function(credssp_lm_response);
	add_test_function(credssp_lm_v2_response);
	add_test_function(credssp_ntlm_v2_response);
	add_test_function(credssp_ntlm_client_signing_key);
	add_test_function(credssp_ntlm_client_sealing_key);
	add_test_function(credssp_ntlm_make_signature);
	add_test_function(credssp_ntlm_encrypt_message);

	return 0;
}

/* sample data taken from http://davenport.sourceforge.net/ntlm.html */

void test_credssp_lm_hash(void)
{
	int i;
	int lm_hash_good;
	char lm_hash[16];
	char password[] = "Password";
	char expected_lm_hash[16] = "\xe5\x2c\xac\x67\x41\x9a\x9a\x22\x4a\x3b\x10\x8f\x3f\xa6\xcb\x6d";

	credssp_lm_hash(password, lm_hash);

	lm_hash_good = 1;
	for (i = 0; i < 16; i++) {
		if (lm_hash[i] != expected_lm_hash[i])
			lm_hash_good = 0;
	}

	CU_ASSERT(lm_hash_good == 1);
}

void test_credssp_ntlm_hash(void)
{
	int i;
	int ntlm_hash_good;
	char ntlm_hash[16];
	char password[] = "Password";
	char expected_ntlm_hash[16] = "\xa4\xf4\x9c\x40\x65\x10\xbd\xca\xb6\x82\x4e\xe7\xc3\x0f\xd8\x52";

	credssp_ntlm_hash(password, ntlm_hash);

	ntlm_hash_good = 1;
	for (i = 0; i < 16; i++) {
		if (ntlm_hash[i] != expected_ntlm_hash[i])
			ntlm_hash_good = 0;
	}

	CU_ASSERT(ntlm_hash_good == 1);
}

void test_credssp_ntlm_v2_hash(void)
{
	int i;
	int ntlm_v2_hash_good;
	char ntlm_v2_hash[16];

	DATA_BLOB domain1;
	DATA_BLOB domain2;

	char username1[] = "user";
	char domain1_wstr[] = "\x44\x00\x4F\x00\x4D\x00\x41\x00\x49\x00\x4E\x00";
	char password1[] = "SecREt01";
	char expected_ntlm_v2_hash1[16] = "\x04\xB8\xE0\xBA\x74\x28\x9C\xC5\x40\x82\x6B\xAB\x1D\xEE\x63\xAE";
	domain1.data = (void* )&domain1_wstr;
	domain1.length = sizeof(domain1_wstr);

	char username2[] = "User";
	char password2[] = "Password";
	char domain2_wstr[] = "\x44\x00\x6f\x00\x6d\x00\x61\x00\x69\x00\x6e\x00";
	char expected_ntlm_v2_hash2[16] = "\x0c\x86\x8a\x40\x3b\xfd\x7a\x93\xa3\x00\x1e\xf2\x2e\xf0\x2e\x3f";
	domain2.data = (void* )&domain2_wstr;
	domain2.length = sizeof(domain2_wstr);

	credssp_ntlm_v2_hash(password2, username2, &domain2, ntlm_v2_hash);

	ntlm_v2_hash_good = 1;
	for (i = 0; i < 16; i++) {
		if (ntlm_v2_hash[i] != expected_ntlm_v2_hash2[i])
			ntlm_v2_hash_good = 0;
	}

	CU_ASSERT(ntlm_v2_hash_good == 1);

	credssp_ntlm_v2_hash(password1, username1, &domain1, ntlm_v2_hash);

	ntlm_v2_hash_good = 1;
	for (i = 0; i < 16; i++) {
		if (ntlm_v2_hash[i] != expected_ntlm_v2_hash1[i])
			ntlm_v2_hash_good = 0;
	}

	CU_ASSERT(ntlm_v2_hash_good == 1);
}

void test_credssp_lm_response(void)
{
	int i;
	int lm_response_good;
	char lm_response[24];
	char password[] = "SecREt01";
	char challenge[] = "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
	char expected_lm_response[24] = "\xC3\x37\xCD\x5C\xBD\x44\xFC\x97\x82\xA6\x67\xAF\x6D\x42\x7C\x6D\xE6\x7C\x20\xC2\xD3\xE7\x7C\x56";

	credssp_lm_response(password, challenge, lm_response);

	lm_response_good = 1;
	for (i = 0; i < 24; i++) {
		if (lm_response[i] != expected_lm_response[i])
			lm_response_good = 0;
	}

	CU_ASSERT(lm_response_good == 1);
}

void test_credssp_lm_v2_response(void)
{
	int i;
	DATA_BLOB domain;
	int lm_v2_response_good;
	char lm_v2_response[24];
	char password[] = "SecREt01";
	char username[] = "user";
	char challenge[] = "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
	char random[] = "\xFF\xFF\xFF\x00\x11\x22\x33\x44";
	char domain_wstr[] = "\x44\x00\x4F\x00\x4D\x00\x41\x00\x49\x00\x4E\x00";
	char expected_lm_v2_response[24] = "\xD6\xE6\x15\x2E\xA2\x5D\x03\xB7\xC6\xBA\x66\x29\xC2\xD6\xAA\xF0\xFF\xFF\xFF\x00\x11\x22\x33\x44";

	domain.data = (void* )&domain_wstr;
	domain.length = sizeof(domain_wstr);

	credssp_lm_v2_response_static(password, username, &domain, (uint8*) challenge, (uint8*) lm_v2_response, random);

	lm_v2_response_good = 1;
	for (i = 0; i < 24; i++) {
		if (lm_v2_response[i] != expected_lm_v2_response[i])
			lm_v2_response_good = 0;
	}

	CU_ASSERT(lm_v2_response_good == 1);
}

void test_credssp_ntlm_v2_response(void)
{
	int i;
	char* p;
	int session_base_key_good;
	int lm_challenge_response_good;
	int nt_challenge_response_good;
	char session_base_key[16];
	char password[] = "Password";
	char username[] = "User";
	char client_challenge[8] = "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa";
	char server_challenge[8] = "\x01\x23\x45\x67\x89\xab\xcd\xef";
	char timestamp[8] = "\x00\x00\x00\x00\x00\x00\x00\x00";
	char domain_wstr[12] = "\x44\x00\x6f\x00\x6d\x00\x61\x00\x69\x00\x6e\x00";

	DATA_BLOB domain;
	DATA_BLOB target_info;
	DATA_BLOB nt_challenge_response;
	DATA_BLOB lm_challenge_response;

	char target_info_data[36] =
		"\x02\x00\x0c\x00\x44\x00\x6f\x00\x6d\x00\x61\x00\x69\x00\x6e\x00"
		"\x01\x00\x0c\x00\x53\x00\x65\x00\x72\x00\x76\x00\x65\x00\x72\x00"
		"\x00\x00\x00\x00";

	char expected_lm_challenge_response[24] =
		"\x86\xc3\x50\x97\xac\x9c\xec\x10\x25\x54\x76\x4a\x57\xcc\xcc\x19"
		"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa";

	char expected_nt_challenge_response[84] =
		"\x68\xcd\x0a\xb8\x51\xe5\x1c\x96\xaa\xbc\x92\x7b\xeb\xef\x6a\x1c"
		"\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\x00\x00\x00\x00\x02\x00\x0c\x00"
		"\x44\x00\x6f\x00\x6d\x00\x61\x00\x69\x00\x6e\x00\x01\x00\x0c\x00"
		"\x53\x00\x65\x00\x72\x00\x76\x00\x65\x00\x72\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00";

	char expected_session_base_key[16] =
		"\x8d\xe4\x0c\xca\xdb\xc1\x4a\x82\xf1\x5c\xb0\xad\x0d\xe9\x5c\xa3";

	target_info.data = (void*) target_info_data;
	target_info.length = sizeof(target_info_data);

	domain.data = (void*) &domain_wstr;
	domain.length = sizeof(domain_wstr);

	credssp_ntlm_v2_response_static(password, username, &domain, (uint8*) client_challenge, (uint8*) server_challenge,
		&target_info, (uint8*) session_base_key, timestamp, &nt_challenge_response, &lm_challenge_response);

	session_base_key_good = 1;
	p = (char*) session_base_key;

	for (i = 0; i < 16; i++) {
		if (p[i] != expected_session_base_key[i])
			session_base_key_good = 0;
	}

	CU_ASSERT(session_base_key_good == 1);

	lm_challenge_response_good = 1;
	p = (char*) lm_challenge_response.data;
	for (i = 0; i < 24; i++) {
		if (p[i] != expected_lm_challenge_response[i])
			lm_challenge_response_good = 0;
	}
	
	CU_ASSERT(lm_challenge_response_good == 1);

	nt_challenge_response_good = 1;
	p = (char*) nt_challenge_response.data;
	for (i = 0; i < 84; i++) {
		if (p[i] != expected_nt_challenge_response[i])
			nt_challenge_response_good = 0;
	}

	CU_ASSERT(nt_challenge_response_good == 1);
}


void test_credssp_ntlm_client_signing_key(void)
{
	int i;
	int client_signing_key_good;
	uint8 client_signing_key[16];
	uint8 random_session_key[16] = "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x00";
	uint8 expected_client_signing_key[16] = "\xF7\xF9\x7A\x82\xEC\x39\x0F\x9C\x90\x3D\xAC\x4F\x6A\xCE\xB1\x32";

	credssp_ntlm_client_signing_key(random_session_key, client_signing_key);

	client_signing_key_good = 1;
	for (i = 0; i < 16; i++) {
		if (client_signing_key[i] != expected_client_signing_key[i])
			client_signing_key_good = 0;
	}

	CU_ASSERT(client_signing_key_good == 1);
}

void test_credssp_ntlm_client_sealing_key(void)
{
	int i;
	int client_sealing_key_good;
	uint8 client_sealing_key[16];
	uint8 random_session_key[16] = "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x00";
	uint8 expected_client_sealing_key[16] =  "\x27\x85\xF5\x95\x29\x3F\x3E\x28\x13\x43\x9D\x73\xA2\x23\x81\x0D";

	credssp_ntlm_client_sealing_key(random_session_key, client_sealing_key);

	client_sealing_key_good = 1;
	for (i = 0; i < 16; i++) {
		if (client_sealing_key[i] != expected_client_sealing_key[i])
			client_sealing_key_good = 0;
	}

	CU_ASSERT(client_sealing_key_good == 1);
}

void test_credssp_ntlm_make_signature(void)
{
	int i;
	CryptoRc4 rc4;
	int signature_good;
	uint8 signature[16];
	uint8 message[5] = "\x6A\x43\x49\x46\x53";
	uint8 expected_signature[16] = "\x01\x00\x00\x00\xE3\x7F\x97\xF2\x54\x4F\x4D\x7E\x00\x00\x00\x00";
	uint8 client_signing_key[16] = "\xF7\xF9\x7A\x82\xEC\x39\x0F\x9C\x90\x3D\xAC\x4F\x6A\xCE\xB1\x32";
	uint8 client_sealing_key[16] = "\x27\x85\xF5\x95\x29\x3F\x3E\x28\x13\x43\x9D\x73\xA2\x23\x81\x0D";

	rc4 = credssp_ntlm_init_client_rc4_stream(client_sealing_key);
	credssp_ntlm_make_signature(message, 5, client_signing_key, client_sealing_key, 0, rc4, signature);

	signature_good = 1;
	for (i = 0; i < 16; i++) {
		if (signature[i] != expected_signature[i])
			signature_good = 0;
	}

	CU_ASSERT(signature_good == 1);

	credssp_ntlm_free_client_rc4_stream(rc4);
}

void test_credssp_ntlm_encrypt_message(void)
{
	int i;
	CryptoRc4 rc4;
	int encrypted_message_good;
	uint8 encrypted_message[21];
	uint8 message[5] = "\x6A\x43\x49\x46\x53";
	uint8 expected_encrypted_message[21] = "\xCF\x0E\xB0\xA9\x39\x01\x00\x00\x00\x88\x4B\x14\x80\x9E\x53\xBF\xE7\x00\x00\x00\x00";
	uint8 client_signing_key[16] = "\xF7\xF9\x7A\x82\xEC\x39\x0F\x9C\x90\x3D\xAC\x4F\x6A\xCE\xB1\x32";
	uint8 client_sealing_key[16] = "\x6F\x0D\x99\x53\x50\x33\x95\x1C\xBE\x49\x9C\xD1\x91\x4F\xE9\xEE";

	rc4 = credssp_ntlm_init_client_rc4_stream(client_sealing_key);
	credssp_ntlm_encrypt_message(message, 5, client_signing_key, client_sealing_key, 0, rc4, encrypted_message);

	encrypted_message_good = 1;
	for (i = 0; i < 21; i++) {
		if (encrypted_message[i] != expected_encrypted_message[i])
			encrypted_message_good = 0;
	}

	CU_ASSERT(encrypted_message_good == 1);

	credssp_ntlm_free_client_rc4_stream(rc4);
}
