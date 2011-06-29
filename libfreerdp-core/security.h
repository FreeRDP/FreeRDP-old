/*
   FreeRDP: A Remote Desktop Protocol client.
   Standard RDP Security

   Copyright (C) Jay Sorg 2009-2011

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

#ifndef __SECURITY_H
#define __SECURITY_H

typedef struct rdp_sec rdpSec;

#include "crypto.h"
#include <freerdp/utils/debug.h>
#include <freerdp/constants/constants.h>

#ifndef DISABLE_TLS
#include "tls.h"
#endif

RD_BOOL
sec_global_init(void);
void
sec_global_finish(void);

struct rdp_sec
{
	int rc4_key_len;
	struct rdp_rdp * rdp;
	struct rdp_network * net;
	CryptoRc4 rc4_decrypt_key;
	CryptoRc4 rc4_encrypt_key;
	uint32 server_public_key_len;
	uint8 sec_sign_key[16];
	uint8 sec_decrypt_key[16];
	uint8 sec_encrypt_key[16];
	uint8 sec_decrypt_update_key[16];
	uint8 sec_encrypt_update_key[16];
	uint8 sec_crypted_random[SEC_MAX_MODULUS_SIZE];
	/* These values must be available to reset state - Session Directory */
	int sec_encrypt_use_count;
	int sec_decrypt_use_count;
};

enum sec_recv_type
{
	SEC_RECV_SHARE_CONTROL,
	SEC_RECV_REDIRECT,
	SEC_RECV_LICENSE,
	SEC_RECV_IOCHANNEL, /* other than SEC_RECV_LICENSE */
	SEC_RECV_FAST_PATH
};
typedef enum sec_recv_type secRecvType;

void
sec_hash_48(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2, uint8 salt);
void
sec_hash_16(uint8 * out, uint8 * in, uint8 * salt1, uint8 * salt2);
void
buf_out_uint32(uint8 * buffer, uint32 value);
void
sec_sign(uint8 * signature, int siglen, uint8 * session_key, int keylen,
	 uint8 * data, int datalen);
RD_BOOL
sec_parse_public_key(rdpSec * sec, STREAM s, uint32 len, uint8 * modulus, uint8 * exponent);
RD_BOOL
sec_parse_public_sig(STREAM s, uint32 len);
void
sec_generate_keys(rdpSec * sec, uint8 * client_random, uint8 * server_random, int rc4_key_size);
STREAM
sec_init(rdpSec * sec, uint32 flags, int maxlen);
STREAM
sec_fp_init(rdpSec * sec, uint32 flags, int maxlen);
void
sec_send_to_channel(rdpSec * sec, STREAM s, uint32 flags, uint16 channel);
void
sec_send(rdpSec * sec, STREAM s, uint32 flags);
void
sec_fp_send(rdpSec * sec, STREAM s, uint32 flags);
void
sec_reverse_copy(uint8 * out, uint8 * in, int len);
void
connect_process_mcs_data(rdpSec * sec, STREAM s);
STREAM
sec_recv(rdpSec * sec, secRecvType * type);
void
sec_establish_key(rdpSec * sec);
void
sec_disconnect(rdpSec * sec);
rdpSec *
sec_new(struct rdp_rdp * rdp);
void
sec_free(rdpSec * sec);

#ifdef WITH_DEBUG_SEC
#define DEBUG_SEC(fmt, ...) DEBUG_CLASS(SEC, fmt, ## __VA_ARGS__)
#else
#define DEBUG_SEC(fmt, ...) DEBUG_NULL(fmt, ## __VA_ARGS__)
#endif

#endif
