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
#include <freerdp/types_ui.h>
#include "types.h"

RD_BOOL
rdp_global_init(void);
void
rdp_global_finish(void);

struct rdp_rdp
{
	uint8 * next_packet;
	uint32 rdp_serverid;
	uint32 rdp_shareid;
	uint32 packetno;
	STREAM rdp_s;
	int current_status;
#ifdef HAVE_ICONV
	void* in_iconv_h;	/* non-thread-safe converter to DEFAULT_CODEPAGE from WINDOWS_CODEPAGE */
	void* out_iconv_h;	/* non-thread-safe converter to WINDOWS_CODEPAGE from DEFAULT_CODEPAGE */
#endif
	RDPCOMP mppc_dict;
	struct rdp_sec * sec;
	struct rdp_set * settings; // RDP settings
	struct rdp_orders * orders;
	struct rdp_pcache * pcache;
	struct rdp_cache * cache;
	struct rdp_app * app; // RemoteApp
	struct rdp_ext * ext;
	/* Session Directory redirection */
	int redirect;
	uint32 redirect_session_id;
	char* redirect_server;
	char* redirect_domain;
	char* redirect_password;
	uint32 redirect_password_len;
	char* redirect_username;
	char* redirect_routingtoken;
	uint32 redirect_routingtoken_len;
	char* redirect_target_fqdn;
	char* redirect_target_netbios_name;
	char* redirect_target_net_addresses;
	uint32 redirect_target_net_addresses_len;
	int input_flags;
	int use_input_fast_path;
	rdpInst * inst;
	void* buffer;
	size_t buffer_size;
	/* large pointers */
	int got_large_pointer_caps;
	int large_pointers;
	/* surface commands */
	int got_surface_commands_caps;
	int surface_commands;
	/* frame ack */
	int got_frame_ack_caps;
	int frame_ack;
	/* fragment */
	int got_multifragmentupdate_caps;
	int multifragmentupdate_request_size;
	STREAM fragment_data;
};
typedef struct rdp_rdp rdpRdp;

int
mppc_expand(rdpRdp * rdp, uint8 * data, uint32 clen, uint8 ctype, uint32 * roff, uint32 * rlen);
void
rdp5_process(rdpRdp * rdp, STREAM s);
char*
xstrdup_out_unistr(rdpRdp * rdp, char *str, size_t *pout_len);
char*
xstrdup_in_unistr(rdpRdp * rdp, unsigned char* pin, size_t in_len);
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
process_color_pointer_pdu(rdpRdp * rdp, STREAM s);
void
process_cached_pointer_pdu(rdpRdp * rdp, STREAM s);
void
process_new_pointer_pdu(rdpRdp * rdp, STREAM s);
void
process_bitmap_updates(rdpRdp * rdp, STREAM s);
void
process_palette(rdpRdp * rdp, STREAM s);
void
rdp_main_loop(rdpRdp * rdp, RD_BOOL * deactivated, uint32 * ext_disc_reason);
RD_BOOL
rdp_loop(rdpRdp * rdp, RD_BOOL * deactivated);
RD_BOOL
rdp_connect(rdpRdp * rdp);
RD_BOOL
rdp_reconnect(rdpRdp * rdp);
void
rdp_disconnect(rdpRdp * rdp);
rdpRdp *
rdp_new(rdpSet * settings, rdpInst * inst);
void
rdp_free(rdpRdp * rdp);

#endif
