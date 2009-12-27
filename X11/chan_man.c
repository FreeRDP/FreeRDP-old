/*
   Copyright (c) 2009 Jay Sorg

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

   MS compatible plugin interface
   reference:
   http://msdn.microsoft.com/en-us/library/aa383580(VS.85).aspx

   Notes on threads:
   Many virtual channel plugins are built using threads.
   Non main threads may call MyVirtualChannelOpen,
   MyVirtualChannelClose, or MyVirtualChannelWrite.
   Since the plugin's VirtualChannelEntry function is called
   from the main thread, MyVirtualChannelInit has to be called
   from the main thread.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "freerdp.h"
#include "chan_man.h"
#include "vchan.h"

/* this is just so we don't have a zero handle */
#define INDEX_TO_DWORD_HANDLE(_in) ((_in) + 0xffff)
#define DWORD_HANDLE_TO_INDEX(_in) ((_in) - 0xffff)

struct lib_data
{
	void * han;
	PVIRTUALCHANNELENTRY entry;
	PCHANNEL_INIT_EVENT_FN init_event_proc;
};

struct chan_data
{
	char name[CHANNEL_NAME_LEN + 1];
	int options;
	int flags; /* 0 nothing 1 init 2 open */
	PCHANNEL_OPEN_EVENT_FN open_event_proc;
	struct lib_data * lib;
};

/* Only the main thread alters these arrays, before any
   library thread is allowed in(post_connect is called)
   so no need to use mutex locking
   After post_connect, each library thread can only access it's
   own array items
   ie, no two threads can access index 0, ... */
static struct lib_data g_libs[CHANNEL_MAX_COUNT];
static int g_num_libs;
static struct chan_data g_chans[CHANNEL_MAX_COUNT];
static int g_num_chans;

/* control for entry into MyVirtualChannelInit */
static int g_can_call_init;
static rdpSet * g_settings;

/* true once chan_man_post_connect is called */
static int g_is_connected;

/* returns struct chan_data for the channel name passed in */
static struct chan_data *
chan_man_find_chan_data(const char * chan_name, int * pindex)
{
	int index;
	struct chan_data * lchan;

	for (index = 0; index < g_num_chans; index++)
	{
		lchan = g_chans + index;
		if (strcmp(chan_name, lchan->name) == 0)
		{
			if (pindex != 0)
			{
				*pindex = index;
			}
			return lchan;
		}
	}
	return 0;
}

/* must be called by same thread that calls chan_man_load_plugin
   according to MS docs
   only called from main thread */
static uint32
MyVirtualChannelInit(void ** ppInitHandle, PCHANNEL_DEF pChannel,
	int channelCount, uint32 versionRequested,
	PCHANNEL_INIT_EVENT_FN pChannelInitEventProc)
{
	int index;
	struct lib_data * llib;
	struct chan_data * lchan;
	struct rdp_chan * lrdp_chan;
	PCHANNEL_DEF lchan_def;

	printf("MyVirtualChannelInit:\n");
	if (!g_can_call_init)
	{
		printf("MyVirtualChannelInit: error not in entry\n");
		return CHANNEL_RC_NOT_IN_VIRTUALCHANNELENTRY;
	}
	if (pChannelInitEventProc == 0)
	{
		printf("MyVirtualChannelInit: error bad proc\n");
		return CHANNEL_RC_BAD_PROC;
	}
	if (ppInitHandle == 0)
	{
		printf("MyVirtualChannelInit: error bad pphan\n");
		return CHANNEL_RC_BAD_INIT_HANDLE;
	}
	if (g_num_chans + channelCount >= CHANNEL_MAX_COUNT)
	{
		printf("MyVirtualChannelInit: error too many channels\n");
		return CHANNEL_RC_TOO_MANY_CHANNELS;
	}
	if (pChannel == 0)
	{
		printf("MyVirtualChannelInit: error bad pchan\n");
		return CHANNEL_RC_BAD_CHANNEL;
	}
	if (g_is_connected)
	{
		printf("MyVirtualChannelInit: error already connected\n");
		return CHANNEL_RC_ALREADY_CONNECTED;
	}
	if (versionRequested != VIRTUAL_CHANNEL_VERSION_WIN2000)
	{
		printf("MyVirtualChannelInit: warning version\n");
	}
	for (index = 0; index < channelCount; index++)
	{
		lchan_def = pChannel + index;
		if (chan_man_find_chan_data(lchan_def->name, 0) != 0)
		{
			printf("MyVirtualChannelInit: error channel already used\n");
			return CHANNEL_RC_BAD_CHANNEL;
		}
	}
	llib = g_libs + g_num_libs;
	*ppInitHandle = llib;
	llib->init_event_proc = pChannelInitEventProc;
	g_num_libs++;
	for (index = 0; index < channelCount; index++)
	{
		lchan_def = pChannel + index;
		lchan = g_chans + g_num_chans;
		lchan->flags = 1; /* init */
		strncpy(lchan->name, lchan_def->name, CHANNEL_NAME_LEN);
		lchan->options = lchan_def->options;
		if (g_settings->num_channels < 16)
		{
			lrdp_chan = g_settings->channels + g_settings->num_channels;
			strncpy(lrdp_chan->name, lchan_def->name, 7);
			lrdp_chan->flags = lchan_def->options;
		}
		else
		{
			printf("MyVirtualChannelInit: warning more than 16 channels\n");
		}
		g_num_chans++;
	}
	return CHANNEL_RC_OK;
}

/* can be called from any thread
   thread safe because no 2 threads can have the same channel name registered */
static uint32
MyVirtualChannelOpen(void * pInitHandle, uint32 * pOpenHandle,
	char * pChannelName, PCHANNEL_OPEN_EVENT_FN pChannelOpenEventProc)
{
	int index;
	struct lib_data * llib;
	struct chan_data * lchan;

	llib = (struct lib_data *) pInitHandle;
	if (llib == 0)
	{
		printf("MyVirtualChannelOpen: error bad inithan\n");
		return CHANNEL_RC_BAD_INIT_HANDLE;
	}
	if (pOpenHandle == 0)
	{
		printf("MyVirtualChannelOpen: error bad chanhan\n");
		return CHANNEL_RC_BAD_CHANNEL_HANDLE;
	}
	if (pChannelOpenEventProc == 0)
	{
		printf("MyVirtualChannelOpen: error bad proc\n");
		return CHANNEL_RC_BAD_PROC;
	}
	if (!g_is_connected)
	{
		printf("MyVirtualChannelOpen: error not connected\n");
		return CHANNEL_RC_NOT_CONNECTED;
	}
	lchan = chan_man_find_chan_data(pChannelName, &index);
	if (lchan == 0)
	{
		printf("MyVirtualChannelOpen: error chan name\n");
		return CHANNEL_RC_UNKNOWN_CHANNEL_NAME;
	}
	if (lchan->flags == 2)
	{
		printf("MyVirtualChannelOpen: error chan already open\n");
		return CHANNEL_RC_ALREADY_OPEN;
	}
	lchan->flags = 2; /* open */
	lchan->lib = llib;
	lchan->open_event_proc = pChannelOpenEventProc;
	*pOpenHandle = INDEX_TO_DWORD_HANDLE(index);
	return CHANNEL_RC_OK;
}

/* can be called from any thread
   thread safe because no 2 threads can have the same openHandle */
static uint32
MyVirtualChannelClose(uint32 openHandle)
{
	struct chan_data * lchan;
	int index;

	index = DWORD_HANDLE_TO_INDEX(openHandle);
	if ((index < 0) || (index >= CHANNEL_MAX_COUNT))
	{
		printf("MyVirtualChannelClose: error bad chanhan\n");
		return CHANNEL_RC_BAD_CHANNEL_HANDLE;
	}
	if (!g_is_connected)
	{
		printf("MyVirtualChannelClose: error not connected\n");
		return CHANNEL_RC_NOT_CONNECTED;
	}
	lchan = g_chans + index;
	if (lchan->flags != 2)
	{
		printf("MyVirtualChannelClose: error not open\n");
		return CHANNEL_RC_NOT_OPEN;
	}
	lchan->flags = 0;
	return CHANNEL_RC_OK;
}

/* can be called from any thread */
static uint32
MyVirtualChannelWrite(uint32 openHandle, void * pData, uint32 dataLength,
	void * pUserData)
{
	struct chan_data * lchan;
	int index;

	index = DWORD_HANDLE_TO_INDEX(openHandle);
	if ((index < 0) || (index >= CHANNEL_MAX_COUNT))
	{
		printf("MyVirtualChannelWrite: error bad chanhan\n");
		return CHANNEL_RC_BAD_CHANNEL_HANDLE;
	}
	if (!g_is_connected)
	{
		printf("MyVirtualChannelWrite: error not connected\n");
		return CHANNEL_RC_NOT_CONNECTED;
	}
	if (pData == 0)
	{
		printf("MyVirtualChannelWrite: error bad pData\n");
		return CHANNEL_RC_NULL_DATA;
	}
	if (dataLength == 0)
	{
		printf("MyVirtualChannelWrite: error bad dataLength\n");
		return CHANNEL_RC_ZERO_LENGTH;
	}
	lchan = g_chans + index;
	if (lchan->flags != 2)
	{
		printf("MyVirtualChannelWrite: error not open\n");
		return CHANNEL_RC_NOT_OPEN;
	}
	return CHANNEL_RC_OK;
}

/* this is called shortly after the application starts and
   before any other function in the file
   called only from main thread */
int
chan_man_init(void)
{
	memset(g_libs, 0, sizeof(g_libs));
	memset(g_chans, 0, sizeof(g_chans));
	g_num_libs = 0;
	g_num_chans = 0;
	g_can_call_init = 0;
	g_settings = 0;
	g_is_connected = 0;
	return 0;
}

/* this is called when processing the command line parameters
   called only from main thread */
int
chan_man_load_plugin(rdpSet * settings, const char * filename)
{
	struct lib_data * lib;
	CHANNEL_ENTRY_POINTS ep;
	int ok;

	printf("chan_man_load_plugin: filename %s\n", filename);
	if (g_num_libs + 1 >= CHANNEL_MAX_COUNT)
	{
		printf("chan_man_load_plugin: too many channels\n");
		return 1;
	}
	lib = g_libs + g_num_libs;
	lib->han = dlopen(filename, RTLD_LOCAL | RTLD_LAZY);
	if (lib->han == 0)
	{
		printf("chan_man_load_plugin: failed to load library\n");
		return 1;
	}
	lib->entry = (PVIRTUALCHANNELENTRY)
		dlsym(lib->han, CHANNEL_EXPORT_FUNC_NAME);
	if (lib->entry == 0)
	{
		printf("chan_man_load_plugin: failed to find export function\n");
		dlclose(lib->han);
		return 1;
	}
	ep.cbSize = sizeof(ep);
	ep.protocolVersion = VIRTUAL_CHANNEL_VERSION_WIN2000;
	ep.pVirtualChannelInit = MyVirtualChannelInit;
	ep.pVirtualChannelOpen = MyVirtualChannelOpen;
	ep.pVirtualChannelClose = MyVirtualChannelClose;
	ep.pVirtualChannelWrite = MyVirtualChannelWrite;
	/* enable MyVirtualChannelInit */
	g_can_call_init = 1;
	g_settings = settings;
	ok = lib->entry(&ep);
	/* disable MyVirtualChannelInit */
	g_settings = 0;
	g_can_call_init = 0;
	if (!ok)
	{
		printf("chan_man_load_plugin: export function call failed\n");
		dlclose(lib->han);
		return 1;
	}
	return 0;
}

/* called only from main thread */
int
chan_man_pre_connect(struct rdp_inst * inst)
{
	printf("chan_man_pre_connect:\n");
	return 0;
}

/* go through and inform all the libraries that we are connected
   called only from main thread */
int
chan_man_post_connect(struct rdp_inst * inst)
{
	printf("chan_man_post_connect:\n");
	g_is_connected = 1;
	return 0;
}

/* data comming from the server to the client
   called only from main thread */
int
chan_man_data(struct rdp_inst * inst, int chan_id, char * data,
	int data_size, int flags, int total_size)
{
	printf("chan_man_data:\n");
	return 0;
}
