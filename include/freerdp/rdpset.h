/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   Protocol services - RDP settings
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

#ifndef __RDPSET_H
#define __RDPSET_H

struct rdp_chan
{
	char name[8]; /* ui sets */
	int flags; /* ui sets */
	int chan_id; /* libfreerdp sets */
	void * handle; /* just for ui */
};

struct rdp_ext_set
{
	char name[256]; /* plugin name or path */
	void * data; /* plugin data */
};

struct rdp_set
{
	int tls;
	int width;
	int height;
	char hostname[16];
	char server[64];
	char domain[16];
	char password[64];
	char shell[256];
	char directory[256];
	char username[256];
	int tcp_port_rdp;
	int keyboard_layout;
	int keyboard_type;
	int keyboard_subtype;
	int keyboard_functionkeys;
	char xkb_layout[32];
	char xkb_variant[32];
	int encryption;
	int rdp_version;
	int remote_app;
	int console_session;
	int server_depth;
	int bitmap_cache;
	int bitmap_cache_persist_enable;
	int bitmap_cache_precache;
	int bitmap_compression;
	int performanceflags;
	int desktop_save;
	int polygon_ellipse_orders;
	int autologin;
	int console_audio;
	int off_screen_bitmaps;
	int triblt;
	int new_cursors;
	int bulk_compression;
	int num_channels;
	struct rdp_chan channels[16];
	struct rdp_ext_set extensions[16];
};

#endif
