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

#ifndef __CHAN_H
#define __CHAN_H

#include "mcs.h"
#include "network.h"

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
