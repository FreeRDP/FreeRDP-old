
#include "test_freerdp.h"

int init_ntlmssp_suite(void);
int clean_ntlmssp_suite(void);
int add_ntlmssp_suite(void);

void test_ntlmssp_compute_lm_hash(void);
void test_ntlmssp_compute_ntlm_hash(void);
void test_ntlmssp_compute_ntlm_v2_hash(void);
void test_ntlmssp_compute_lm_response(void);
void test_ntlmssp_compute_lm_v2_response(void);
void test_ntlmssp_compute_ntlm_v2_response(void);
void test_ntlmssp_generate_client_signing_key(void);
void test_ntlmssp_generate_server_signing_key(void);
void test_ntlmssp_generate_client_sealing_key(void);
void test_ntlmssp_generate_server_sealing_key(void);
void test_ntlmssp_encrypt_random_session_key(void);
void test_ntlmssp_compute_message_integrity_check(void);
void test_ntlmssp_encrypt_message(void);
void test_ntlmssp_decrypt_message(void);
