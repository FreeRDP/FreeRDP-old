
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rdesktop.h"
#include "chan.h"
#include "mcs.h"
#include "mem.h"
#include "secure.h"
#include "rdp.h"
#include "rdpset.h"

int
channel_register(rdpChannels * chan, char * name, int flags)
{
	struct rdp_chan_item * channel;

	if (chan->num_channels >= MAX_CHANNELS)
	{
		ui_error(chan->mcs->sec->rdp->inst, "too many channels\n");
		return 0;
	}
	channel = &(chan->channels[chan->num_channels]);
	channel->mcs_id = MCS_GLOBAL_CHANNEL + 1 + chan->num_channels;
	strncpy(channel->name, name, 8);
	channel->flags = flags;
	chan->num_channels++;
	return channel->mcs_id;
}

int
channel_send(rdpChannels * chan, int mcs_id, char * data, int total_length)
{
	STREAM s;
	int sec_flags;
	int length;
	int sent;
	int chan_flags;
	int chan_index;
	struct rdp_chan_item * channel;

	/* lock mutex */
	chan_index = (mcs_id - MCS_GLOBAL_CHANNEL) - 1;
	if ((chan_index < 0) || (chan_index >= chan->num_channels))
	{
		ui_error(chan->mcs->sec->rdp->inst, "error\n");
		return 0;
	}
	channel = &(chan->channels[chan_index]);
	chan_flags = CHANNEL_FLAG_FIRST;
	sent = 0;
	sec_flags = chan->mcs->sec->rdp->settings->encryption ? SEC_ENCRYPT : 0;
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
	/* unlock mutex */
	return sent;
}

void
channel_process(rdpChannels * chan, STREAM s, int mcs_id)
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
channel_setup(struct rdp_mcs * mcs)
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
channel_cleanup(rdpChannels * chan)
{
	if (chan != NULL)
	{
		xfree(chan);
	}
}
