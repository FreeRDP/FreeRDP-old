
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

	add_test_function(compute_ntlm_hash);
	add_test_function(compute_ntlm_v2_hash);
	add_test_function(compute_lm_response);
	add_test_function(compute_lm_v2_response);
	add_test_function(compute_ntlm_v2_response);

	return 0;
}

/* sample data taken from http://davenport.sourceforge.net/ntlm.html */

void test_compute_ntlm_hash(void)
{
	int i;
	int ntlm_hash_good;
	char ntlm_hash[16];
	char password[] = "password";
	char expected_ntlm_hash[16] = "\x88\x46\xF7\xEA\xEE\x8F\xB1\x17\xAD\x06\xBD\xD8\x30\xB7\x58\x6C";

	compute_ntlm_hash(password, ntlm_hash);

	ntlm_hash_good = 1;
	for (i = 0; i < 16; i++) {
		if (ntlm_hash[i] != expected_ntlm_hash[i])
			ntlm_hash_good = 0;
	}

	CU_ASSERT(ntlm_hash_good == 1);
}

void test_compute_ntlm_v2_hash(void)
{
	int i;
	int ntlm_v2_hash_good;
	char ntlm_v2_hash[16];
	char username[] = "user";
	char server[] = "DOMAIN";
	char password[] = "SecREt01";
	char expected_ntlm_v2_hash[16] = "\x04\xB8\xE0\xBA\x74\x28\x9C\xC5\x40\x82\x6B\xAB\x1D\xEE\x63\xAE";

	compute_ntlm_v2_hash(password, username, server, ntlm_v2_hash);

	ntlm_v2_hash_good = 1;
	for (i = 0; i < 16; i++) {
		if (ntlm_v2_hash[i] != expected_ntlm_v2_hash[i])
			ntlm_v2_hash_good = 0;
	}

	CU_ASSERT(ntlm_v2_hash_good == 1);
}

void test_compute_lm_response(void)
{
	int i;
	int lm_response_good;
	char lm_response[24];
	char password[] = "SecREt01";
	char challenge[] = "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
	char expected_lm_response[24] = "\xC3\x37\xCD\x5C\xBD\x44\xFC\x97\x82\xA6\x67\xAF\x6D\x42\x7C\x6D\xE6\x7C\x20\xC2\xD3\xE7\x7C\x56";

	compute_lm_response(password, challenge, lm_response);

	lm_response_good = 1;
	for (i = 0; i < 24; i++) {
		if (lm_response[i] != expected_lm_response[i])
			lm_response_good = 0;
	}

	CU_ASSERT(lm_response_good == 1);
}

void test_compute_lm_v2_response(void)
{
	int i;
	int lm_v2_response_good;
	char lm_v2_response[24];
	char password[] = "SecREt01";
	char username[] = "user";
	char server[] = "DOMAIN";
	char challenge[] = "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
	char random[] = "\xFF\xFF\xFF\x00\x11\x22\x33\x44";
	char expected_lm_v2_response[24] = "\xD6\xE6\x15\x2E\xA2\x5D\x03\xB7\xC6\xBA\x66\x29\xC2\xD6\xAA\xF0\xFF\xFF\xFF\x00\x11\x22\x33\x44";

	compute_lm_v2_response_random(password, username, server, (uint8*) challenge, (uint8*) lm_v2_response, random);

	lm_v2_response_good = 1;
	for (i = 0; i < 24; i++) {
		if (lm_v2_response[i] != expected_lm_v2_response[i])
			lm_v2_response_good = 0;
	}

	CU_ASSERT(lm_v2_response_good == 1);
}

void test_compute_ntlm_v2_response(void)
{
	int i;
	int ntlm_v2_response_good;
	char ntlm_v2_response[146];
	char password[] = "SecREt01";
	char username[] = "user";
	char server[] = "DOMAIN";
	char challenge[] = "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
	char random[] = "\xFF\xFF\xFF\x00\x11\x22\x33\x44";
	char timestamp[] = "\x00\x90\xD3\x36\xB7\x34\xC3\x01";

	char target_info[98] =
		"\x02\x00\x0C\x00\x44\x00\x4F\x00"
		"\x4D\x00\x41\x00\x49\x00\x4E\x00"
		"\x01\x00\x0C\x00\x53\x00\x45\x00"
		"\x52\x00\x56\x00\x45\x00\x52\x00"
		"\x04\x00\x14\x00\x64\x00\x6F\x00"
		"\x6D\x00\x61\x00\x69\x00\x6E\x00"
		"\x2E\x00\x63\x00\x6F\x00\x6D\x00"
		"\x03\x00\x22\x00\x73\x00\x65\x00"
		"\x72\x00\x76\x00\x65\x00\x72\x00"
		"\x2E\x00\x64\x00\x6F\x00\x6D\x00"
		"\x61\x00\x69\x00\x6E\x00\x2E\x00"
		"\x63\x00\x6F\x00\x6D\x00\x00\x00"
		"\x00\x00";

	char expected_ntlm_v2_response[146] =
		"\xCB\xAB\xBC\xA7\x13\xEB\x79\x5D\x04\xC9\x7A\xBC\x01\xEE\x49\x83"
		"\x01\x01\x00\x00\x00\x00\x00\x00\x00\x90\xD3\x36\xB7\x34\xC3\x01"
		"\xFF\xFF\xFF\x00\x11\x22\x33\x44\x00\x00\x00\x00\x02\x00\x0C\x00"
		"\x44\x00\x4F\x00\x4D\x00\x41\x00\x49\x00\x4E\x00\x01\x00\x0C\x00"
		"\x53\x00\x45\x00\x52\x00\x56\x00\x45\x00\x52\x00\x04\x00\x14\x00"
		"\x64\x00\x6F\x00\x6D\x00\x61\x00\x69\x00\x6E\x00\x2E\x00\x63\x00"
		"\x6F\x00\x6D\x00\x03\x00\x22\x00\x73\x00\x65\x00\x72\x00\x76\x00"
		"\x65\x00\x72\x00\x2E\x00\x64\x00\x6F\x00\x6D\x00\x61\x00\x69\x00"
		"\x6E\x00\x2E\x00\x63\x00\x6F\x00\x6D\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00";

	compute_ntlm_v2_response_random(password, username, server, (uint8*) challenge, target_info, 98, (uint8*) ntlm_v2_response, random, timestamp);

	ntlm_v2_response_good = 1;
	for (i = 0; i < 146; i++) {
		if (ntlm_v2_response[i] != expected_ntlm_v2_response[i])
			ntlm_v2_response_good = 0;
	}

	CU_ASSERT(ntlm_v2_response_good == 1);
}
