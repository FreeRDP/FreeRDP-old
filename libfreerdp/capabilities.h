/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP Capabilities

   Copyright 2009 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __CAPABILITIES_H
#define __CAPABILITIES_H

#include "rdp.h"

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
rdp_out_large_pointer_capset(rdpRdp * rdp, STREAM s);
void
rdp_out_compdesk_capset(STREAM s);
void
rdp_out_multifragmentupdate_capset(rdpRdp * rdp, STREAM s);
void
rdp_out_surface_commands_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_bitmap_codecs_capset(rdpRdp * rdp, STREAM s, int size);
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
rdp_process_large_pointer_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_surface_commands_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_compdesk_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_multifragmentupdate_capset(rdpRdp * rdp, STREAM s);
void
rdp_process_frame_ack_capset(rdpRdp * rdp, STREAM s);
void
rdp_out_frame_ack_capset(rdpRdp * rdp, STREAM s);

#endif // __CAPABILITIES_H
