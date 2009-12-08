
#ifndef __CHAN_H
#define __CHAN_H

#include "types_ui.h"
#include "constants_ui.h"
#include "mcs.h"

struct rdp_chan_item
{
	int mcs_id;
	char name[8];
	int flags;
};

struct rdp_channels
{
	struct rdp_mcs * mcs;
	int num_channels;
	struct rdp_chan_item channels[MAX_CHANNELS];
};
typedef struct rdp_channels rdpChannels;

int
channel_register(rdpChannels * chan, char * name, int flags);
int
channel_send(rdpChannels * chan, int mcs_id, char * data, int total_length);
void
channel_process(rdpChannels * chan, STREAM s, int mcs_id);
rdpChannels *
channel_setup(struct rdp_mcs * mcs);
void
channel_cleanup(rdpChannels * chan);

#endif
