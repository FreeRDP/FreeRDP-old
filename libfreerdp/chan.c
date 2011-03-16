/*
   FreeRDP: A Remote Desktop Protocol client.
   Channels

   Copyright (C) Jay Sorg 2009-2011

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

#include <freerdp/utils.h>
#include <freerdp/rdpset.h>
#include <freerdp/constants_vchan.h>
#include "frdp.h"
#include "chan.h"
#include "mcs.h"
#include "secure.h"
#include "rdp.h"

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
