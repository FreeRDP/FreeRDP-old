/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel

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
#include "tsmf_constants.h"

typedef struct _TSMF_LISTENER_CALLBACK TSMF_LISTENER_CALLBACK;
struct _TSMF_LISTENER_CALLBACK
{
	IWTSListenerCallback iface;

	IWTSPlugin * plugin;
	IWTSVirtualChannelManager * channel_mgr;
};

typedef struct _TSMF_CHANNEL_CALLBACK TSMF_CHANNEL_CALLBACK;
struct _TSMF_CHANNEL_CALLBACK
{
	IWTSVirtualChannelCallback iface;

	IWTSPlugin * plugin;
	IWTSVirtualChannelManager * channel_mgr;
	IWTSVirtualChannel * channel;
};

typedef struct _TSMF_PLUGIN TSMF_PLUGIN;
struct _TSMF_PLUGIN
{
	IWTSPlugin iface;

	TSMF_LISTENER_CALLBACK * listener_callback;
};

static int
tsmf_on_data_received(IWTSVirtualChannelCallback * pChannelCallback,
	uint32 cbSize,
	char * pBuffer)
{
	uint32 InterfaceId;
	uint32 MessageId;
	uint32 FunctionId;
	int error = 0;

	/* 2.2.1 Shared Message Header (SHARED_MSG_HEADER) */
	if (cbSize < 12)
	{
		LLOGLN(0, ("tsmf_on_data_received: invalid size. cbSize=%d", cbSize));
		return 1;
	}
	InterfaceId = GET_UINT32(pBuffer, 0);
	MessageId = GET_UINT32(pBuffer, 4);
	FunctionId = GET_UINT32(pBuffer, 8);
	LLOGLN(0, ("tsmf_on_data_received: cbSize=%d InterfaceId=0x%X MessageId=0x%X FunctionId=0x%X",
		cbSize, InterfaceId, MessageId, FunctionId));
	return error;
}

static int
tsmf_on_close(IWTSVirtualChannelCallback * pChannelCallback)
{
	LLOGLN(0, ("tsmf_on_close:"));
	free(pChannelCallback);
	return 0;
}

static int
tsmf_on_new_channel_connection(IWTSListenerCallback * pListenerCallback,
	IWTSVirtualChannel * pChannel,
	char * Data,
	int * pbAccept,
	IWTSVirtualChannelCallback ** ppCallback)
{
	TSMF_LISTENER_CALLBACK * listener_callback = (TSMF_LISTENER_CALLBACK *) pListenerCallback;
	TSMF_CHANNEL_CALLBACK * callback;

	LLOGLN(0, ("tsmf_on_new_channel_connection:"));
	callback = (TSMF_CHANNEL_CALLBACK *) malloc(sizeof(TSMF_CHANNEL_CALLBACK));
	callback->iface.OnDataReceived = tsmf_on_data_received;
	callback->iface.OnClose = tsmf_on_close;
	callback->plugin = listener_callback->plugin;
	callback->channel_mgr = listener_callback->channel_mgr;
	callback->channel = pChannel;
	*ppCallback = (IWTSVirtualChannelCallback *) callback;
	return 0;
}

static int
tsmf_plugin_initialize(IWTSPlugin * pPlugin, IWTSVirtualChannelManager * pChannelMgr)
{
	TSMF_PLUGIN * tsmf = (TSMF_PLUGIN *) pPlugin;

	LLOGLN(0, ("tsmf_plugin_initialize:"));
	tsmf->listener_callback = (TSMF_LISTENER_CALLBACK *) malloc(sizeof(TSMF_LISTENER_CALLBACK));
	memset(tsmf->listener_callback, 0, sizeof(TSMF_LISTENER_CALLBACK));

	tsmf->listener_callback->iface.OnNewChannelConnection = tsmf_on_new_channel_connection;
	tsmf->listener_callback->plugin = pPlugin;
	tsmf->listener_callback->channel_mgr = pChannelMgr;
	return pChannelMgr->CreateListener(pChannelMgr, "TSMF", 0,
		(IWTSListenerCallback *) tsmf->listener_callback, NULL);
}

static int
tsmf_plugin_terminated(IWTSPlugin * pPlugin)
{
	TSMF_PLUGIN * tsmf = (TSMF_PLUGIN *) pPlugin;

	LLOGLN(0, ("tsmf_plugin_terminated:"));
	if (tsmf->listener_callback)
		free(tsmf->listener_callback);
	free(tsmf);
	return 0;
}

int
DVCPluginEntry(IDRDYNVC_ENTRY_POINTS * pEntryPoints)
{
	TSMF_PLUGIN * tsmf;

	tsmf = (TSMF_PLUGIN *) malloc(sizeof(TSMF_PLUGIN));
	memset(tsmf, 0, sizeof(TSMF_PLUGIN));

	tsmf->iface.Initialize = tsmf_plugin_initialize;
	tsmf->iface.Connected = NULL;
	tsmf->iface.Disconnected = NULL;
	tsmf->iface.Terminated = tsmf_plugin_terminated;
	return pEntryPoints->RegisterPlugin(pEntryPoints, (IWTSPlugin *) tsmf);
}

