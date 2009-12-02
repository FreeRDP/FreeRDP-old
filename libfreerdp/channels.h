/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   Protocol services - Virtual channels
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

#ifndef __CHANNELS_H
#define __CHANNELS_H

#define MAX_CHANNELS			6
#define CHANNEL_CHUNK_LENGTH		1600
#define CHANNEL_FLAG_FIRST		0x01
#define CHANNEL_FLAG_LAST		0x02
#define CHANNEL_FLAG_SHOW_PROTOCOL	0x10

struct rdp_channels
{
	struct rdp_mcs * mcs;
	RD_BOOL encryption;
	VCHANNEL channels[MAX_CHANNELS];
	unsigned int num_channels;
};
typedef struct rdp_channels rdpChannels;

/* channels.c */
VCHANNEL *
channel_register(rdpChannels * chan, char *name, uint32 flags, void (*callback) (STREAM));
STREAM
channel_init(rdpChannels * chan, VCHANNEL * channel, uint32 length);
void
channel_send(rdpChannels * chan, STREAM s, VCHANNEL * channel);
void
channel_process(rdpChannels * chan, STREAM s, uint16 mcs_channel);
rdpChannels *
channels_setup(struct rdp_mcs * mcs);
void
channels_cleanup(rdpChannels * chan);

#endif
