/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Credential Security Support Provider (CredSSP)

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

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

#ifndef __CREDSSP_H
#define __CREDSSP_H

#include "secure.h"
#include "ntlmssp.h"
#include "data_blob.h"

struct rdp_credssp
{
	int send_seq_num;
	DATA_BLOB public_key;
	DATA_BLOB ts_credentials;
	CryptoRc4 rc4_seal_state;
	struct _NTLMSSP *ntlmssp;
	struct rdp_sec * sec;
};
typedef struct rdp_credssp rdpCredssp;

int credssp_authenticate(rdpCredssp *credssp);

void credssp_send(rdpCredssp *credssp, STREAM negoToken, STREAM pubKeyAuth, STREAM authInfo);
void credssp_recv(rdpCredssp *credssp, STREAM negoToken, STREAM pubKeyAuth, STREAM authInfo);

void credssp_encrypt_public_key(rdpCredssp *credssp, STREAM s);
void credssp_encode_ts_credentials(rdpCredssp *credssp);

void credssp_nonce(uint8* nonce, int size);
void credssp_current_time(uint8* timestamp);
void credssp_str_to_wstr(char* str, uint8* wstr, int length);
void credssp_rc4k(uint8* key, int length, uint8* plaintext, uint8* ciphertext);

rdpCredssp* credssp_new(rdpSec *sec);
void credssp_free(rdpCredssp *credssp);

#endif // __CREDSSP_H
