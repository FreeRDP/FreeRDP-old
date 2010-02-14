/*
   Copyright (c) 2010 Vic Lee

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

*/

#ifndef __CHAN_PLUGIN_H
#define __CHAN_PLUGIN_H

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

