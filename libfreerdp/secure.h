/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   Protocol services - RDP encryption and licensing
   Copyright (C) Jay Sorg 2009

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

#ifndef __SECURE_H
#define __SECURE_H

#include "ssl.h"
#include "constants.h"

struct rdp_sec
{
	struct rdp_rdp * rdp;
	int rc4_key_len;
	SSL_RC4 rc4_decrypt_key;
	SSL_RC4 rc4_encrypt_key;
	uint32 server_public_key_len;
	uint8 sec_sign_key[16];
	uint8 sec_decrypt_key[16];
	uint8 sec_encrypt_key[16];
	uint8 sec_decrypt_update_key[16];
	uint8 sec_encrypt_update_key[16];
	uint8 sec_crypted_random[SEC_MAX_MODULUS_SIZE];
	uint16 server_rdp_version;
	/* These values must be available to reset state - Session Directory */
	int sec_encrypt_use_count;
	int sec_decrypt_use_count;
	struct rdp_mcs * mcs;
	struct rdp_licence * licence;
	int negotiation_state;
	int nla;
};
typedef struct rdp_sec rdpSec;

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
void
sec_decrypt(rdpSec * sec, uint8 * data, int length);
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
sec_process_mcs_data(rdpSec * sec, STREAM s);
STREAM
sec_recv(rdpSec * sec, secRecvType * type);
RD_BOOL
sec_connect(rdpSec * sec, char *server, char *username, int port);
RD_BOOL
sec_reconnect(rdpSec * sec, char *server, int port);
void
sec_disconnect(rdpSec * sec);
void
sec_reset_state(rdpSec * sec);
rdpSec *
sec_new(struct rdp_rdp * rdp);
void
sec_free(rdpSec * sec);

#endif
