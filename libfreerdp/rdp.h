/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   Protocol services - RDP layer
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

#ifndef __RDP_H
#define __RDP_H

#include <time.h>

#include "types.h"
#include "types_ui.h"

struct rdp_rdp
{
	uint8 * next_packet;
	uint32 rdp_shareid;
	uint32 packetno;
	STREAM rdp_s;
	int current_status;
	RD_BOOL iconv_works;
	void* in_iconv_h;
	void* out_iconv_h;
	RDPCOMP mppc_dict;
	struct rdp_sec * sec;
	struct rdp_set * settings; // RDP settings
	struct rdp_orders * orders;
	struct rdp_pcache * pcache;
	struct rdp_cache * cache;
	struct rdp_app * app; // RemoteApp
	/* Session Directory redirection */
	int redirect;
	char redirect_server[64];
	char redirect_domain[16];
	char redirect_password[64];
	char redirect_username[256];
	char redirect_cookie[128];
	int redirect_flags;
	int input_flags;
	int use_input_fast_path;
	struct rdp_inst * inst;
};
typedef struct rdp_rdp rdpRdp;

int
mppc_expand(rdpRdp * rdp, uint8 * data, uint32 clen, uint8 ctype, uint32 * roff, uint32 * rlen);
void
rdp5_process(rdpRdp * rdp, STREAM s);
void
rdp_out_unistr(rdpRdp * rdp, STREAM s, char *string, int len);
int
rdp_in_unistr(rdpRdp * rdp, STREAM s, char *string, int str_len, int in_len);
void
rdp_send_input(rdpRdp * rdp, time_t time, uint16 message_type, uint16 device_flags, uint16 param1,
	       uint16 param2);
void
rdp_sync_input(rdpRdp * rdp, time_t time, uint32 toggle_keys_state);
void
rdp_unicode_input(rdpRdp * rdp, time_t time, uint16 unicode_character);
void
rdp_send_client_window_status(rdpRdp * rdp, int status);
void
process_colour_pointer_pdu(rdpRdp * rdp, STREAM s);
void
process_cached_pointer_pdu(rdpRdp * rdp, STREAM s);
void
process_system_pointer_pdu(rdpRdp * rdp, STREAM s);
void
process_new_pointer_pdu(rdpRdp * rdp, STREAM s);
void
process_bitmap_updates(rdpRdp * rdp, STREAM s);
void
process_palette(rdpRdp * rdp, STREAM s);
void
process_disconnect_pdu(STREAM s, uint32 * ext_disc_reason);
void
rdp_main_loop(rdpRdp * rdp, RD_BOOL * deactivated, uint32 * ext_disc_reason);
RD_BOOL
rdp_loop(rdpRdp * rdp, RD_BOOL * deactivated, uint32 * ext_disc_reason);
RD_BOOL
rdp_connect(rdpRdp * rdp, char * server, uint32 flags, char * domain, char * password,
	    char * command, char * directory, int port, char * username);
RD_BOOL
rdp_reconnect(rdpRdp * rdp, char * server, uint32 flags, char * domain, char * password,
	      char * command, char * directory, char * cookie, int port, char * username);
void
rdp_reset_state(rdpRdp * rdp);
void
rdp_disconnect(rdpRdp * rdp);
rdpRdp *
rdp_new(struct rdp_set * settings);
void
rdp_free(rdpRdp * rdp);

#endif
