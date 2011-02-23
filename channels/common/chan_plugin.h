/*
   FreeRDP: A Remote Desktop Protocol client.
   Virtual Channel

   Copyright 2010-2011 Vic Lee

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

#ifndef __CHAN_PLUGIN_H
#define __CHAN_PLUGIN_H

#include <freerdp/constants_vchan.h>

typedef struct rdp_chan_plugin rdpChanPlugin;
struct rdp_chan_plugin
{
	void * init_handle;
	int open_handle[CHANNEL_MAX_COUNT];
	int num_open_handles;
};

void
chan_plugin_init(rdpChanPlugin * chan_plugin);
void
chan_plugin_uninit(rdpChanPlugin * chan_plugin);
int
chan_plugin_register_open_handle(rdpChanPlugin * chan_plugin,
	int open_handle);
int
chan_plugin_unregister_open_handle(rdpChanPlugin * chan_plugin,
	int open_handle);
rdpChanPlugin *
chan_plugin_find_by_init_handle(void * init_handle);
rdpChanPlugin *
chan_plugin_find_by_open_handle(int open_handle);

#endif
