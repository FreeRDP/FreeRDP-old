
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types_ui.h"
#include "vchan.h"

static CHANNEL_ENTRY_POINTS g_ep;
static void * g_han;
static CHANNEL_DEF g_channel_def[2];
static uint32 g_open_handle[2];

static void
OpenEvent(uint32 openHandle, uint32 event, void * pData, uint32 dataLength,
	uint32 totalLength, uint32 dataFlags)
{
	printf("OpenEvent: event %d\n", event);
	switch (event)
	{
		case CHANNEL_EVENT_DATA_RECEIVED:
			printf("OpenEvent: receive openHandle %d dataLength %d "
				"totalLength %d dataFlags %d\n",
				openHandle, dataLength, totalLength, dataFlags);
			break;
	}
}

static void
InitEvent(void * pInitHandle, uint32 event, void * pData, uint32 dataLength)
{
	printf("InitEvent: event %d\n", event);
	switch (event)
	{
		case CHANNEL_EVENT_CONNECTED:
			g_ep.pVirtualChannelOpen(g_han, &(g_open_handle[0]),
				"rdpsnd", OpenEvent);
			g_ep.pVirtualChannelOpen(g_han, &(g_open_handle[1]),
				"snddbg", OpenEvent);
			break;
	}
}

int
VirtualChannelEntry(PCHANNEL_ENTRY_POINTS pEntryPoints)
{
	printf("VirtualChannelEntry:\n");
	g_ep = *pEntryPoints;
	memset(&(g_channel_def[0]), 0, sizeof(g_channel_def));
	g_channel_def[0].options = CHANNEL_OPTION_INITIALIZED |
		CHANNEL_OPTION_ENCRYPT_RDP;
	strcpy(g_channel_def[0].name, "rdpsnd");
	g_channel_def[1].options = CHANNEL_OPTION_INITIALIZED |
		CHANNEL_OPTION_ENCRYPT_RDP;
	strcpy(g_channel_def[1].name, "snddbg");
	g_ep.pVirtualChannelInit(&g_han, g_channel_def, 2,
		VIRTUAL_CHANNEL_VERSION_WIN2000, InitEvent);
	return 1;
}
