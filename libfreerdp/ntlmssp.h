/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   NT LAN Manager Security Support Provider (NTLMSSP)

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2011

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

#ifndef __NTLMSSP_H
#define __NTLMSSP_H

#include "secure.h"
#include "credssp.h"
#include "data_blob.h"

enum _NTLMSSP_STATE
{
	NTLMSSP_STATE_INITIAL,
	NTLMSSP_STATE_NEGOTIATE,
	NTLMSSP_STATE_CHALLENGE,
	NTLMSSP_STATE_AUTHENTICATE,
	NTLMSSP_STATE_FINAL
};
typedef enum _NTLMSSP_STATE NTLMSSP_STATE;

struct _NTLMSSP
{
	NTLMSSP_STATE state;
	DATA_BLOB password;
	DATA_BLOB username;
	DATA_BLOB domain;
	DATA_BLOB target_name;
	DATA_BLOB target_info;
	uint32 negotiate_flags;
	uint8 timestamp[8];
	uint8 server_challenge[8];
	uint8 client_challenge[8];
	uint8 session_base_key[16];
	uint8 key_exchange_key[16];
	uint8 random_session_key[16];
	uint8 exported_session_key[16];
	uint8 encrypted_random_session_key[16];
	uint8 client_signing_key[16];
	uint8 client_sealing_key[16];
	DATA_BLOB nt_challenge_response;
	DATA_BLOB lm_challenge_response;
	CryptoRc4 rc4_seal;
	int send_seq_num;
};
typedef struct _NTLMSSP NTLMSSP;

void ntlmssp_set_username(NTLMSSP *ntlmssp, char* username);
void ntlmssp_set_domain(NTLMSSP *ntlmssp, char* domain);
void ntlmssp_set_password(NTLMSSP *ntlmssp, char* password);

void ntlmssp_generate_client_challenge(NTLMSSP *ntlmssp);
void ntlmssp_generate_key_exchange_key(NTLMSSP *ntlmssp);
void ntlmssp_generate_random_session_key(NTLMSSP *ntlmssp);
void ntlmssp_generate_exported_session_key(NTLMSSP *ntlmssp);
void ntlmssp_encrypt_random_session_key(NTLMSSP *ntlmssp);

void ntlmssp_generate_client_signing_key(NTLMSSP *ntlmssp);
void ntlmssp_generate_client_sealing_key(NTLMSSP *ntlmssp);
void ntlmssp_init_rc4_seal_state(NTLMSSP *ntlmssp);

void ntlmssp_compute_lm_hash(char* password, char* hash);
void ntlmssp_compute_ntlm_hash(DATA_BLOB* password, char* hash);
void ntlmssp_compute_ntlm_v2_hash(DATA_BLOB *password, DATA_BLOB *username, DATA_BLOB *domain, char* hash);

void ntlmssp_compute_lm_response(char* password, char* challenge, char* response);
void ntlmssp_compute_lm_v2_response(NTLMSSP *ntlmssp);
void ntlmssp_compute_ntlm_v2_response(NTLMSSP *ntlmssp);

void ntlmssp_encrypt_message(NTLMSSP *ntlmssp, DATA_BLOB *msg, DATA_BLOB *encrypted_msg, uint8* signature);

int ntlmssp_recv(NTLMSSP *ntlmssp, STREAM s);
int ntlmssp_send(NTLMSSP *ntlmssp, STREAM s);

NTLMSSP* ntlmssp_new();
void ntlmssp_init(NTLMSSP *ntlmssp);
void ntlmssp_free(NTLMSSP *ntlmssp);

#endif /* __NTLMSSP_H */