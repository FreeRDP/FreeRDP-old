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

void
credssp_send(rdpSec * sec, STREAM s);
void
credssp_recv(rdpSec * sec);

void
ntlm_send_negotiate_message(rdpSec * sec);
void
ntlm_recv_challenge_message(rdpSec * sec, STREAM s);
void
ntlm_send_authenticate_message(rdpSec * sec);
void
ntlm_recv(rdpSec * sec, STREAM s);

#endif // __CREDSSP_H

