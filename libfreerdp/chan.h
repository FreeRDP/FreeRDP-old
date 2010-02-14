
#ifndef __CHAN_H
#define __CHAN_H

#include "types_ui.h"
#include "constants_ui.h"
#include "mcs.h"

struct rdp_channels
{
	struct rdp_mcs * mcs;
};
typedef struct rdp_channels rdpChannels;

int
channel_send(rdpChannels * chan, int mcs_id, char * data, int total_length);
void
channel_process(rdpChannels * chan, STREAM s, int mcs_id);
rdpChannels *
channel_new(struct rdp_mcs * mcs);
void
channel_free(rdpChannels * chan);

#endif
