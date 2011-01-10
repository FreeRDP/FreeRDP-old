
#include "test_freerdp.h"

int init_credssp_suite(void);
int clean_credssp_suite(void);
int add_credssp_suite(void);

void test_credssp_lm_hash(void);
void test_credssp_ntlm_hash(void);
void test_credssp_ntlm_v2_hash(void);
void test_credssp_lm_response(void);
void test_credssp_lm_v2_response(void);
void test_credssp_ntlm_v2_response(void);
void test_credssp_ntlm_client_signing_key(void);
void test_credssp_ntlm_client_sealing_key(void);
void test_credssp_ntlm_make_signature(void);
void test_credssp_ntlm_encrypt_message(void);
