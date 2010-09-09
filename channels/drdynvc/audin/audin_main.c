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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drdynvc_types.h"

#define MSG_SNDIN_VERSION       0x01
#define MSG_SNDIN_FORMATS       0x02
#define MSG_SNDIN_OPEN          0x03
#define MSG_SNDIN_OPEN_REPLY    0x04
#define MSG_SNDIN_DATA_INCOMING 0x05
#define MSG_SNDIN_DATA          0x06
#define MSG_SNDIN_FORMATCHANGE  0x07

typedef struct _AUDIN_LISTENER_CALLBACK AUDIN_LISTENER_CALLBACK;
struct _AUDIN_LISTENER_CALLBACK
{
	IWTSListenerCallback iface;

	IWTSPlugin * plugin;
	IWTSVirtualChannelManager * channel_mgr;
};

typedef struct _AUDIN_CHANNEL_CALLBACK AUDIN_CHANNEL_CALLBACK;
struct _AUDIN_CHANNEL_CALLBACK
{
	IWTSVirtualChannelCallback iface;

	IWTSPlugin * plugin;
	IWTSVirtualChannelManager * channel_mgr;
	IWTSVirtualChannel * channel;
};

typedef struct _AUDIN_PLUGIN AUDIN_PLUGIN;
struct _AUDIN_PLUGIN
{
	IWTSPlugin iface;

	AUDIN_LISTENER_CALLBACK * listener_callback;
};

static int
audin_process_version(IWTSVirtualChannelCallback * pChannelCallback,
	char * data, uint32 data_size)
{
	AUDIN_CHANNEL_CALLBACK * callback = (AUDIN_CHANNEL_CALLBACK *) pChannelCallback;
	uint32 Version;
	uint32 out_size;
	char * out_data;

	Version = GET_UINT32(data, 0);
	LLOGLN(10, ("audin_process_version: Version=%d", Version));

	out_size = 5;
	out_data = (char *) malloc(out_size);
	memset(out_data, 0, out_size);
	SET_UINT8(out_data, 0, MSG_SNDIN_VERSION);
	SET_UINT32(out_data, 1, Version);
	callback->channel->Write(callback->channel, out_size, out_data, NULL);
	free(out_data);

	return 0;
}

static int
audin_on_data_received(IWTSVirtualChannelCallback * pChannelCallback,
	uint32 cbSize,
	char * pBuffer)
{
	uint8 MessageId;

	MessageId = GET_UINT8(pBuffer, 0);
	LLOGLN(10, ("audin_on_data_received: MessageId=0x%x", MessageId));
	switch (MessageId)
	{
		case MSG_SNDIN_VERSION:
			audin_process_version(pChannelCallback, pBuffer + 1, cbSize - 1);
			break;
		default:
			LLOGLN(0, ("audin_on_data_received: unknown MessageId=0x%x", MessageId));
			break;
	}
	return 0;
}

static int
audin_on_close(IWTSVirtualChannelCallback * pChannelCallback)
{
	LLOGLN(10, ("audin_on_close:"));
	free(pChannelCallback);
	return 0;
}

static int
audin_on_new_channel_connection(IWTSListenerCallback * pListenerCallback,
	IWTSVirtualChannel * pChannel,
	char * Data,
	int * pbAccept,
	IWTSVirtualChannelCallback ** ppCallback)
{
	AUDIN_LISTENER_CALLBACK * listener_callback = (AUDIN_LISTENER_CALLBACK *) pListenerCallback;
	AUDIN_CHANNEL_CALLBACK * callback;

	LLOGLN(10, ("audin_on_new_channel_connection:"));
	callback = (AUDIN_CHANNEL_CALLBACK *) malloc(sizeof(AUDIN_CHANNEL_CALLBACK));
	callback->iface.OnDataReceived = audin_on_data_received;
	callback->iface.OnClose = audin_on_close;
	callback->plugin = listener_callback->plugin;
	callback->channel_mgr = listener_callback->channel_mgr;
	callback->channel = pChannel;
	*ppCallback = (IWTSVirtualChannelCallback *) callback;
	return 0;
}

static int
audin_plugin_initialize(IWTSPlugin * pPlugin, IWTSVirtualChannelManager * pChannelMgr)
{
	AUDIN_PLUGIN * audin = (AUDIN_PLUGIN *) pPlugin;

	LLOGLN(10, ("audin_plugin_initialize:"));
	audin->listener_callback = (AUDIN_LISTENER_CALLBACK *) malloc(sizeof(AUDIN_LISTENER_CALLBACK));
	memset(audin->listener_callback, 0, sizeof(AUDIN_LISTENER_CALLBACK));

	audin->listener_callback->iface.OnNewChannelConnection = audin_on_new_channel_connection;
	audin->listener_callback->plugin = pPlugin;
	audin->listener_callback->channel_mgr = pChannelMgr;
	return pChannelMgr->CreateListener(pChannelMgr, "AUDIO_INPUT", 0,
		(IWTSListenerCallback *) audin->listener_callback, NULL);
}

static int
audin_plugin_terminated(IWTSPlugin * pPlugin)
{
	AUDIN_PLUGIN * audin = (AUDIN_PLUGIN *) pPlugin;

	LLOGLN(10, ("audin_plugin_terminated:"));
	if (audin->listener_callback)
		free(audin->listener_callback);
	free(audin);
	return 0;
}

int
DVCPluginEntry(IDRDYNVC_ENTRY_POINTS * pEntryPoints)
{
	AUDIN_PLUGIN * audin;

	audin = (AUDIN_PLUGIN *) malloc(sizeof(AUDIN_PLUGIN));
	memset(audin, 0, sizeof(AUDIN_PLUGIN));

	audin->iface.Initialize = audin_plugin_initialize;
	audin->iface.Connected = NULL;
	audin->iface.Disconnected = NULL;
	audin->iface.Terminated = audin_plugin_terminated;
	return pEntryPoints->RegisterPlugin(pEntryPoints, (IWTSPlugin *) audin);
}

