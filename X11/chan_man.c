
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
};

struct chan_data
{
	char name[CHANNEL_NAME_LEN + 1];
	int flags;
};

static struct lib_data g_libs[CHANNEL_MAX_COUNT];
static int g_num_libs;
static struct chan_data g_chans[CHANNEL_MAX_COUNT];
static int g_num_chans;

static uint32
MyVirtualChannelInit(void ** ppInitHandle, PCHANNEL_DEF pChannel,
	int channelCount, uint32 versionRequested,
	PCHANNEL_INIT_EVENT_FN pChannelInitEventProc)
{
	return CHANNEL_RC_OK;
}

static uint32
MyVirtualChannelOpen(void * pInitHandle, uint32 * pOpenHandle,
	char * pChannelName, PCHANNEL_OPEN_EVENT_FN pChannelOpenEventProc)
{
	return CHANNEL_RC_OK;
}

static uint32
MyVirtualChannelClose(uint32 openHandle)
{
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
	return 0;
}

/* this is called when processing the command line parameters */
int
chan_man_load_plugin(rdpSet * settings, const char * filename)
{
	struct lib_data * lib;
	CHANNEL_ENTRY_POINTS ep;

	if (g_num_libs + 1 >= CHANNEL_MAX_COUNT)
	{
		return 1;
	}
	lib = g_libs + g_num_libs;
	lib->han = dlopen(filename, RTLD_LOCAL | RTLD_LAZY);
	if (lib->han == 0)
	{
		return 1;
	}
	lib->entry = (PVIRTUALCHANNELENTRY)
		dlsym(lib->han, CHANNEL_EXPORT_FUNC_NAME);
	if (lib->entry == 0)
	{
		dlclose(lib->han);
		return 1;
	}
	ep.cbSize = sizeof(ep);
	ep.protocolVersion = VIRTUAL_CHANNEL_VERSION_WIN2000;
	ep.pVirtualChannelInit = MyVirtualChannelInit;
	ep.pVirtualChannelOpen = MyVirtualChannelOpen;
	ep.pVirtualChannelClose = MyVirtualChannelClose;
	ep.pVirtualChannelWrite = MyVirtualChannelWrite;
	if (!lib->entry(&ep))
	{
		dlclose(lib->han);
		return 1;
	}
	return 0;
}
