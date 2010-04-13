/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Channels
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frdp.h"
#include "chan.h"
#include "mcs.h"
#include "mem.h"
#include "secure.h"
#include "rdp.h"
#include "rdpset.h"

int
vchan_send(rdpChannels * chan, int mcs_id, char * data, int total_length)
{
	STREAM s;
	int sec_flags;
	int length;
	int sent;
	int chan_flags;
	int chan_index;
	rdpSet * settings;
	struct rdp_chan * channel;

	settings = chan->mcs->sec->rdp->settings;
	chan_index = (mcs_id - MCS_GLOBAL_CHANNEL) - 1;
	if ((chan_index < 0) || (chan_index >= settings->num_channels))
	{
		ui_error(chan->mcs->sec->rdp->inst, "error\n");
		return 0;
	}
	channel = &(settings->channels[chan_index]);
	chan_flags = CHANNEL_FLAG_FIRST;
	sent = 0;
	sec_flags = settings->encryption ? SEC_ENCRYPT : 0;
	while (sent < total_length)
	{
		length = MIN(CHANNEL_CHUNK_LENGTH, total_length);
		length = MIN(total_length - sent, length);
		if ((sent + length) >= total_length)
		{
			chan_flags |= CHANNEL_FLAG_LAST;
		}
		if (channel->flags & CHANNEL_OPTION_SHOW_PROTOCOL)
		{
			chan_flags |= CHANNEL_FLAG_SHOW_PROTOCOL;
		}
		s = sec_init(chan->mcs->sec, sec_flags, length + 8);
		out_uint32_le(s, total_length);
		out_uint32_le(s, chan_flags);
		out_uint8p(s, data + sent, length);
		s_mark_end(s);
		sec_send_to_channel(chan->mcs->sec, s, sec_flags, mcs_id);
		sent += length;
		chan_flags = 0;
	}
	return sent;
}

void
vchan_process(rdpChannels * chan, STREAM s, int mcs_id)
{
	int length;
	int total_length;
	int flags;
	char * data;

	in_uint32_le(s, total_length);
	in_uint32_le(s, flags);
	length = (int) (s->end - s->p);
	data = (char *) (s->p);
	s->p += length;
	ui_channel_data(chan->mcs->sec->rdp->inst, mcs_id, data, length, flags, total_length);
}

rdpChannels *
vchan_new(struct rdp_mcs * mcs)
{
	rdpChannels * self;

	self = (rdpChannels *) xmalloc(sizeof(rdpChannels));
	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpChannels));
		self->mcs = mcs;
	}
	return self;
}

void
vchan_free(rdpChannels * chan)
{
	if (chan != NULL)
	{
		xfree(chan);
	}
}
