/*
   FreeRDP: A Remote Desktop Protocol client.
   Plug and Play Device Virtual Channel

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drdynvc_types.h"
#include <freerdp/utils/stream.h>

#define IRPDR_ID_VERSION            0x00000065
#define IRPDR_ID_REDIRECT_DEVICES   0x00000066
#define IRPDR_ID_SERVER_LOGON       0x00000067
#define IRPDR_ID_UNREDIRECT_DEVICE  0x00000068

typedef struct _PNPDR_LISTENER_CALLBACK PNPDR_LISTENER_CALLBACK;
struct _PNPDR_LISTENER_CALLBACK
{
	IWTSListenerCallback iface;

	IWTSPlugin * plugin;
	IWTSVirtualChannelManager * channel_mgr;
};

typedef struct _PNPDR_CHANNEL_CALLBACK PNPDR_CHANNEL_CALLBACK;
struct _PNPDR_CHANNEL_CALLBACK
{
	IWTSVirtualChannelCallback iface;

	IWTSPlugin * plugin;
	IWTSVirtualChannelManager * channel_mgr;
	IWTSVirtualChannel * channel;
};

typedef struct _PNPDR_PLUGIN PNPDR_PLUGIN;
struct _PNPDR_PLUGIN
{
	IWTSPlugin iface;

	PNPDR_LISTENER_CALLBACK * listener_callback;
};

static int
pnpdr_process_version(IWTSVirtualChannelCallback * pChannelCallback,
	char * data, uint32 data_size)
{
	PNPDR_CHANNEL_CALLBACK * callback = (PNPDR_CHANNEL_CALLBACK *) pChannelCallback;
	uint32 MajorVersion;
	uint32 MinorVersion;
	uint32 Capabilities;
	uint32 out_size;
	char * out_data;

	MajorVersion = GET_UINT32(data, 0);
	MinorVersion = GET_UINT32(data, 4);
	Capabilities = GET_UINT32(data, 8);
	LLOGLN(10, ("pnpdr_process_version: MajorVersion=%d MinorVersion=%d Capabilities=%d",
		MajorVersion, MinorVersion, Capabilities));

	out_size = 20;
	out_data = (char *) malloc(out_size);
	memset(out_data, 0, out_size);
	SET_UINT32(out_data, 0, out_size);
	SET_UINT32(out_data, 4, IRPDR_ID_VERSION);
	SET_UINT32(out_data, 8, MajorVersion);
	SET_UINT32(out_data, 12, MinorVersion);
	SET_UINT32(out_data, 16, Capabilities);
	callback->channel->Write(callback->channel, out_size, out_data, NULL);
	free(out_data);

	return 0;
}

static int
pnpdr_on_data_received(IWTSVirtualChannelCallback * pChannelCallback,
	uint32 cbSize,
	char * pBuffer)
{
	uint32 Size;
	uint32 PacketId;
	int error = 0;

	Size = GET_UINT32(pBuffer, 0);
	PacketId = GET_UINT32(pBuffer, 4);
	if (Size != cbSize)
	{
		LLOGLN(0, ("pnpdr_on_data_received: invalid size. cbSize=%d Size=%d", cbSize, Size));
		return 1;
	}
	LLOGLN(10, ("pnpdr_on_data_received: Size=%d PacketId=0x%X", Size, PacketId));
	switch (PacketId)
	{
		case IRPDR_ID_VERSION:
			error = pnpdr_process_version(pChannelCallback, pBuffer + 8, Size - 8);
			break;
		case IRPDR_ID_SERVER_LOGON:
			LLOGLN(10, ("pnpdr_on_data_received: IRPDR_ID_SERVER_LOGON"));
			break;
		default:
			LLOGLN(0, ("pnpdr_on_data_received: unknown PacketId 0x%X", PacketId));
			error = 1;
			break;
	}
	return error;
}

static int
pnpdr_on_close(IWTSVirtualChannelCallback * pChannelCallback)
{
	LLOGLN(10, ("pnpdr_on_close:"));
	free(pChannelCallback);
	return 0;
}

static int
pnpdr_on_new_channel_connection(IWTSListenerCallback * pListenerCallback,
	IWTSVirtualChannel * pChannel,
	char * Data,
	int * pbAccept,
	IWTSVirtualChannelCallback ** ppCallback)
{
	PNPDR_LISTENER_CALLBACK * listener_callback = (PNPDR_LISTENER_CALLBACK *) pListenerCallback;
	PNPDR_CHANNEL_CALLBACK * callback;

	LLOGLN(10, ("pnpdr_on_new_channel_connection:"));
	callback = (PNPDR_CHANNEL_CALLBACK *) malloc(sizeof(PNPDR_CHANNEL_CALLBACK));
	callback->iface.OnDataReceived = pnpdr_on_data_received;
	callback->iface.OnClose = pnpdr_on_close;
	callback->plugin = listener_callback->plugin;
	callback->channel_mgr = listener_callback->channel_mgr;
	callback->channel = pChannel;
	*ppCallback = (IWTSVirtualChannelCallback *) callback;
	return 0;
}

static int
pnpdr_plugin_initialize(IWTSPlugin * pPlugin, IWTSVirtualChannelManager * pChannelMgr)
{
	PNPDR_PLUGIN * pnpdr = (PNPDR_PLUGIN *) pPlugin;

	LLOGLN(10, ("pnpdr_plugin_initialize:"));
	pnpdr->listener_callback = (PNPDR_LISTENER_CALLBACK *) malloc(sizeof(PNPDR_LISTENER_CALLBACK));
	memset(pnpdr->listener_callback, 0, sizeof(PNPDR_LISTENER_CALLBACK));

	pnpdr->listener_callback->iface.OnNewChannelConnection = pnpdr_on_new_channel_connection;
	pnpdr->listener_callback->plugin = pPlugin;
	pnpdr->listener_callback->channel_mgr = pChannelMgr;
	return pChannelMgr->CreateListener(pChannelMgr, "PNPDR", 0,
		(IWTSListenerCallback *) pnpdr->listener_callback, NULL);
}

static int
pnpdr_plugin_terminated(IWTSPlugin * pPlugin)
{
	PNPDR_PLUGIN * pnpdr = (PNPDR_PLUGIN *) pPlugin;

	LLOGLN(10, ("pnpdr_plugin_terminated:"));
	if (pnpdr->listener_callback)
		free(pnpdr->listener_callback);
	free(pnpdr);
	return 0;
}

int
DVCPluginEntry(IDRDYNVC_ENTRY_POINTS * pEntryPoints)
{
	PNPDR_PLUGIN * pnpdr;
	int ret = 0;

	pnpdr = (PNPDR_PLUGIN *) pEntryPoints->GetPlugin(pEntryPoints, "pnpdr");
	if (pnpdr == NULL)
	{
		pnpdr = (PNPDR_PLUGIN *) malloc(sizeof(PNPDR_PLUGIN));
		memset(pnpdr, 0, sizeof(PNPDR_PLUGIN));

		pnpdr->iface.Initialize = pnpdr_plugin_initialize;
		pnpdr->iface.Connected = NULL;
		pnpdr->iface.Disconnected = NULL;
		pnpdr->iface.Terminated = pnpdr_plugin_terminated;
		ret = pEntryPoints->RegisterPlugin(pEntryPoints, "pnpdr", (IWTSPlugin *) pnpdr);
	}
	return ret;
}

