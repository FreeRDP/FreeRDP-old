
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "freerdp.h"
#include "chan_man.h"
#include "vchan.h"

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
   according to MS docs */
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
			printf("MyVirtualChannelInit: more than 16 channels\n");
		}
		g_num_chans++;
	}
	return CHANNEL_RC_OK;
}

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
	*pOpenHandle = index;
	return CHANNEL_RC_OK;
}

static uint32
MyVirtualChannelClose(uint32 openHandle)
{
	struct chan_data * lchan;

	if (openHandle >= CHANNEL_MAX_COUNT)
	{
		return CHANNEL_RC_BAD_CHANNEL_HANDLE;
	}
	lchan = g_chans + openHandle;
	if (lchan->flags != 2)
	{
		return CHANNEL_RC_NOT_OPEN;
	}
	lchan->flags = 0;
	return CHANNEL_RC_OK;
}

static uint32
MyVirtualChannelWrite(uint32 openHandle, void * pData, uint32 dataLength,
	void * pUserData)
{
	return CHANNEL_RC_OK;
}

/* this is called shortly after the application starts and
   before any other function in the file */
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

/* this is called when processing the command line parameters */
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

int
chan_man_pre_connect(struct rdp_inst * inst)
{
	printf("chan_man_pre_connect:\n");
	return 0;
}

int
chan_man_post_connect(struct rdp_inst * inst)
{
	printf("chan_man_post_connect:\n");
	g_is_connected = 1;
	return 0;
}

/* data comming from the server to the client */
int
chan_man_data(struct rdp_inst * inst, int chan_id, char * data,
	int data_size, int flags, int total_size)
{
	printf("chan_man_data:\n");
	return 0;
}
