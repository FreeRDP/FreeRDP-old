/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Protocol services - Capability sets

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2009

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

#ifndef __CAPABILITIES_H
#define __CAPABILITIES_H

void
rdp_out_general_capset(rdpRdp * rdp, STREAM s);
void
rdp_out_bitmap_capset(rdpRdp * rdp, STREAM s);
void
rdp_out_order_capset(rdpRdp * rdp, STREAM s);
void
rdp_out_bitmapcache_capset(rdpRdp * rdp, STREAM s);
void
rdp_out_bitmapcache_rev2_capset(rdpRdp * rdp, STREAM s);
void
rdp_out_input_capset(rdpRdp * rdp, STREAM s);
void
rdp_out_font_capset(STREAM s);
void
rdp_out_control_capset(STREAM s);
void
rdp_out_window_activation_capset(STREAM s);
void
rdp_out_pointer_capset(rdpRdp * rdp, STREAM s);
void
rdp_out_share_capset(STREAM s);
void
rdp_out_colorcache_capset(STREAM s);
void
rdp_out_brush_capset(STREAM s);
void
rdp_out_glyphcache_capset(STREAM s);
void
rdp_out_sound_capset(STREAM s);
void
rdp_out_offscreenscache_capset(STREAM s);
void
rdp_out_bitmapcache_hostsupport_capset(rdpRdp * rdp, STREAM s);
void
rdp_out_virtualchannel_capset(STREAM s);
void
rdp_out_drawninegridcache_capset(STREAM s);
void
rdp_out_draw_gdiplus_capset(STREAM s);
void
rdp_out_rail_capset(STREAM s);
void
rdp_out_window_capset(STREAM s);
void
rdp_out_large_pointer_capset(STREAM s);
void
rdp_out_compdesk_capset(STREAM s);
void
rdp_out_multifragmentupdate_capset(STREAM s);
void
rdp_out_surface_commands_capset(STREAM s);
void
rdp_out_bitmap_codecs_capset(STREAM s);

void
rdp_process_bitmap_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_order_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_pointer_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_share_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_colorcache_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_input_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_font_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_bitmapcache_hostsupport_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_virtualchannel_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_draw_gdiplus_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_rail_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_window_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_compdesk_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_multifragmentupdate_capset(rdpRdp * rdp, STREAM s);

/* Device redirection capability sets */
void
rdp_out_dr_general_capset(STREAM s);
void
rdp_out_dr_printer_capset(STREAM s);
void
rdp_out_dr_port_capset(STREAM s);
void
rdp_out_dr_drive_capset(STREAM s);
void
rdp_out_dr_smartcard_capset(STREAM s);

void
rdp_process_dr_general_capset(STREAM s);
void
rdp_process_dr_printer_capset(STREAM s);
void
rdp_process_dr_port_capset(STREAM s);
void
rdp_process_dr_drive_capset(STREAM s);
void
rdp_process_dr_smartcard_capset(STREAM s);

#endif // __CAPABILITIES_H

