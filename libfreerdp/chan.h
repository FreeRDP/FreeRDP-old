#ifndef __CHAN_H
#define __CHAN_H

#include "mcs.h"

struct rdp_channels
{
	struct rdp_mcs * mcs;
};
typedef struct rdp_channels rdpChannels;

int
vchan_send(rdpChannels * chan, int mcs_id, char * data, int total_length);
void
vchan_process(rdpChannels * chan, STREAM s, int mcs_id);
rdpChannels *
vchan_new(struct rdp_mcs * mcs);
void
vchan_free(rdpChannels * chan);

#endif
