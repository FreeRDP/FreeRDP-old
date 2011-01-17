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
#include "data_blob.h"

struct _AV_PAIR
{
	uint16 length;
	uint8* value;
};
typedef struct _AV_PAIR AV_PAIR;

struct _AV_PAIRS
{
	AV_PAIR NbComputerName;
	AV_PAIR NbDomainName;
	AV_PAIR DnsComputerName;
	AV_PAIR DnsDomainName;
	AV_PAIR DnsTreeName;
	AV_PAIR Timestamp;
	AV_PAIR Restrictions;
	AV_PAIR TargetName;
	AV_PAIR ChannelBindings;
	uint32 Flags;
};
typedef struct _AV_PAIRS AV_PAIRS;

enum _AV_ID
{
	MsvAvEOL,
	MsvAvNbComputerName,
	MsvAvNbDomainName,
	MsvAvDnsComputerName,
	MsvAvDnsDomainName,
	MsvAvDnsTreeName,
	MsvAvFlags,
	MsvAvTimestamp,
	MsvAvRestrictions,
	MsvAvTargetName,
	MsvChannelBindings
};
typedef enum _AV_ID AV_ID;

enum _NTLM_STATE
{
	NTLM_STATE_INITIAL,
	NTLM_STATE_NEGOTIATE,
	NTLM_STATE_CHALLENGE,
	NTLM_STATE_AUTHENTICATE,
	NTLM_STATE_FINAL
};
typedef enum _NTLM_STATE NTLM_STATE;

struct rdp_nla
{
	NTLM_STATE state;
	struct rdp_sec * sec;
	uint8* public_key;
	int public_key_length;
	uint8* target_name;
	DATA_BLOB target_info;
	AV_PAIRS* av_pairs;
	uint32 negotiate_flags;
	int sequence_number;
	uint8 server_challenge[8];
	uint8 session_base_key[16];
	uint8 exported_session_key[16];
	uint8 client_signing_key[16];
	uint8 server_signing_key[16];
	uint8 client_sealing_key[16];
	uint8 server_sealing_key[16];
	CryptoRc4 rc4_stream;
	DATA_BLOB negotiate_msg;
	DATA_BLOB challenge_msg;
	DATA_BLOB authenticate_msg;
	uint8 message_integrity_check[16];
};
typedef struct rdp_nla rdpNla;

void credssp_send(rdpSec * sec, STREAM negoToken, STREAM pubKeyAuth);
void credssp_recv(rdpSec * sec);

int credssp_authenticate(rdpSec * sec);

void ntlm_send_negotiate_message(rdpSec * sec);
void ntlm_recv_challenge_message(rdpSec * sec, STREAM s);
void ntlm_send_authenticate_message(rdpSec * sec);
void ntlm_recv(rdpSec * sec, STREAM s);

CryptoRc4 credssp_ntlm_init_client_rc4_stream(uint8* sealing_key);
void credssp_ntlm_free_client_rc4_stream(CryptoRc4 rc4);
void credssp_ntlm_client_signing_key(uint8* random_session_key, uint8* signing_key);
void credssp_ntlm_client_sealing_key(uint8* random_session_key, uint8* sealing_key);
void credssp_ntlm_make_signature(uint8* msg, int msg_len, uint8* signing_key, uint8* sealing_key, uint32 seq_num, CryptoRc4 rc4, uint8* signature);
void credssp_ntlm_encrypt_message(uint8* msg, int msg_len, uint8* signing_key, uint8* sealing_key, uint32 seq_num, CryptoRc4 rc4, uint8* encrypted_message);

void credssp_lm_hash(char* password, char* hash);
void credssp_ntlm_hash(char* password, char* hash);
void credssp_ntlm_v2_hash(char* password, char* username, char* domain, char* hash);

void credssp_lm_response(char* password, char* challenge, char* response);
void credssp_lm_v2_response(char* password, char* username, char* server, uint8* challenge, uint8* response);
void credssp_lm_v2_response_static(char* password, char* username, char* server, uint8* challenge, uint8* response, char* random);

void credssp_ntlm_v2_response(char* password, char* username, char* server, uint8* challenge, DATA_BLOB *target_info, uint8* response, uint8* session_key, char* timestamp);
void credssp_ntlm_v2_response_static(char* password, char* username, char* server, uint8* challenge, DATA_BLOB *target_info, uint8* response, uint8* session_key, char* random, char* timestamp);

rdpNla *
nla_new(rdpSec * sec);
void
nla_free(rdpNla * nla);

#endif // __CREDSSP_H
