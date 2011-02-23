/*
   FreeRDP: A Remote Desktop Protocol client.
   Audio Input Redirection Virtual Channel

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
#include "audin_main.h"

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

	/* Hardware-specific data */
	void * device_data;
	/* The supported format list sent back to the server, which needs to
	   be stored as reference when the server sends the format index in
	   Open PDU and Format Change PDU */
	char ** formats_data;
	int formats_count;
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
	int error;

	Version = GET_UINT32(data, 0);
	LLOGLN(10, ("audin_process_version: Version=%d", Version));

	out_size = 5;
	out_data = (char *) malloc(out_size);
	memset(out_data, 0, out_size);
	SET_UINT8(out_data, 0, MSG_SNDIN_VERSION);
	SET_UINT32(out_data, 1, Version);
	error = callback->channel->Write(callback->channel, out_size, out_data, NULL);
	free(out_data);

	return error;
}

static int
audin_send_incoming_data_pdu(IWTSVirtualChannelCallback * pChannelCallback)
{
	AUDIN_CHANNEL_CALLBACK * callback = (AUDIN_CHANNEL_CALLBACK *) pChannelCallback;
	char out_data[1];

	SET_UINT8(out_data, 0, MSG_SNDIN_DATA_INCOMING);
	return callback->channel->Write(callback->channel, 1, out_data, NULL);
}

static int
audin_process_formats(IWTSVirtualChannelCallback * pChannelCallback,
	char * data, uint32 data_size)
{
	AUDIN_CHANNEL_CALLBACK * callback = (AUDIN_CHANNEL_CALLBACK *) pChannelCallback;
	uint32 NumFormats;
	uint32 i;
	int size;
	int out_size;
	char * ldata;
	char * out_data;
	char * lout_formats;
	int out_format_count;
	int error;

	NumFormats = GET_UINT32(data, 0);
	if ((NumFormats < 1) || (NumFormats > 1000))
	{
		LLOGLN(0, ("audin_process_formats: bad NumFormats %d",
			NumFormats));
		return 1;
	}
	/* Ignore cbSizeFormatsPacket */

	size = sizeof(char *) * (NumFormats + 1);
	callback->formats_data = (char **) malloc(size);
	memset(callback->formats_data, 0, size);

	out_size = data_size + 1;
	out_data = (char *) malloc(out_size);
	memset(out_data, 0, out_size);

	lout_formats = out_data + 9;
	/* remainder is sndFormats (variable) */
	ldata = data + 8;
	out_format_count = 0;
	for (i = 0; i < NumFormats; i++)
	{
		size = 18 + GET_UINT16(ldata, 16);
		if (wave_in_format_supported(callback->device_data, ldata, size))
		{
			/* Store the agreed format in the corresponding index */
			callback->formats_data[out_format_count] = (char *) malloc(size);
			memcpy(callback->formats_data[out_format_count], ldata, size);
			/* Put the format to output buffer */
			memcpy(lout_formats, ldata, size);
			lout_formats += size;
			out_format_count++;
		}
		ldata += size;
	}
	callback->formats_count = out_format_count;

	audin_send_incoming_data_pdu(pChannelCallback);

	/* cbSizeFormatsPacket: the size of the entire PDU minus the size of ExtraData */
	size = lout_formats - out_data;	
	SET_UINT8(out_data, 0, MSG_SNDIN_FORMATS);
	SET_UINT32(out_data, 1, out_format_count);
	SET_UINT32(out_data, 5, size);
	error = callback->channel->Write(callback->channel, size, out_data, NULL);
	free(out_data);

	return error;
}

static int
audin_send_format_change_pdu(IWTSVirtualChannelCallback * pChannelCallback, uint32 NewFormat)
{
	AUDIN_CHANNEL_CALLBACK * callback = (AUDIN_CHANNEL_CALLBACK *) pChannelCallback;
	char out_data[5];

	SET_UINT8(out_data, 0, MSG_SNDIN_FORMATCHANGE);
	SET_UINT32(out_data, 1, NewFormat);
	return callback->channel->Write(callback->channel, 5, out_data, NULL);
}

static int
audin_send_open_reply_pdu(IWTSVirtualChannelCallback * pChannelCallback, uint32 Result)
{
	AUDIN_CHANNEL_CALLBACK * callback = (AUDIN_CHANNEL_CALLBACK *) pChannelCallback;
	char out_data[5];

	SET_UINT8(out_data, 0, MSG_SNDIN_OPEN_REPLY);
	SET_UINT32(out_data, 1, Result);
	return callback->channel->Write(callback->channel, 5, out_data, NULL);
}

static int
audin_receive_wave_data(char * wave_data, int size, void * user_data)
{
	AUDIN_CHANNEL_CALLBACK * callback = (AUDIN_CHANNEL_CALLBACK *) user_data;
	int out_size;
	char * out_data;
	int error;

	error = audin_send_incoming_data_pdu((IWTSVirtualChannelCallback *) callback);
	if (error != 0)
		return error;

	out_size = size + 1;
	out_data = (char *) malloc(out_size);
	SET_UINT8(out_data, 0, MSG_SNDIN_DATA);
	memcpy(out_data + 1, wave_data, size);
	error = callback->channel->Write(callback->channel, out_size, out_data, NULL);
	free(out_data);
	return error;
}

static int
audin_process_open(IWTSVirtualChannelCallback * pChannelCallback,
	char * data, uint32 data_size)
{
	AUDIN_CHANNEL_CALLBACK * callback = (AUDIN_CHANNEL_CALLBACK *) pChannelCallback;
	uint32 FramesPerPacket;
	uint32 initialFormat;
	char * format;
	int size;
	int result;

	FramesPerPacket = GET_UINT32(data, 0);
	initialFormat = GET_UINT32(data, 4);
	LLOGLN(10, ("audin_process_open: FramesPerPacket=%d initialFormat=%d",
		FramesPerPacket, initialFormat));
	if (initialFormat >= callback->formats_count)
	{
		LLOGLN(0, ("audin_process_open: invalid format index %d (total %d)",
			initialFormat, callback->formats_count));
		return 1;
	}
	format = callback->formats_data[initialFormat];
	size = 18 + GET_UINT16(format, 16);
	wave_in_set_format(callback->device_data, FramesPerPacket, format, size);
	result = wave_in_open(callback->device_data,
		audin_receive_wave_data, callback);

	if (result == 0)
	{
		audin_send_format_change_pdu(pChannelCallback, initialFormat);
	}
	audin_send_open_reply_pdu(pChannelCallback, result);

	return 0;
}

static int
audin_process_format_change(IWTSVirtualChannelCallback * pChannelCallback,
	char * data, uint32 data_size)
{
	AUDIN_CHANNEL_CALLBACK * callback = (AUDIN_CHANNEL_CALLBACK *) pChannelCallback;
	uint32 NewFormat;
	char * format;
	int size;

	NewFormat = GET_UINT32(data, 0);
	LLOGLN(10, ("audin_process_format_change: NewFormat=%d",
		NewFormat));
	if (NewFormat >= callback->formats_count)
	{
		LLOGLN(0, ("audin_process_format_change: invalid format index %d (total %d)",
			NewFormat, callback->formats_count));
		return 1;
	}

	wave_in_close(callback->device_data);

	format = callback->formats_data[NewFormat];
	size = 18 + GET_UINT16(format, 16);
	wave_in_set_format(callback->device_data, 0, format, size);
	audin_send_format_change_pdu(pChannelCallback, NewFormat);

	wave_in_open(callback->device_data,
		audin_receive_wave_data, callback);

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
		case MSG_SNDIN_FORMATS:
			audin_process_formats(pChannelCallback, pBuffer + 1, cbSize - 1);
			break;
		case MSG_SNDIN_OPEN:
			audin_process_open(pChannelCallback, pBuffer + 1, cbSize - 1);
			break;
		case MSG_SNDIN_FORMATCHANGE:
			audin_process_format_change(pChannelCallback, pBuffer + 1, cbSize - 1);
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
	AUDIN_CHANNEL_CALLBACK * callback = (AUDIN_CHANNEL_CALLBACK *) pChannelCallback;
	int i;

	LLOGLN(10, ("audin_on_close:"));
	wave_in_close(callback->device_data);
	wave_in_free(callback->device_data);
	if (callback->formats_data)
	{
		for (i = 0; i < callback->formats_count; i++)
		{
			free(callback->formats_data[i]);
		}
		free(callback->formats_data);
	}
	free(callback);
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
	memset(callback, 0, sizeof(AUDIN_CHANNEL_CALLBACK));

	callback->iface.OnDataReceived = audin_on_data_received;
	callback->iface.OnClose = audin_on_close;
	callback->plugin = listener_callback->plugin;
	callback->channel_mgr = listener_callback->channel_mgr;
	callback->channel = pChannel;
	callback->device_data = wave_in_new();

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

