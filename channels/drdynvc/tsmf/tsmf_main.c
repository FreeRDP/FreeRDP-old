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
#include "tsmf_ifman.h"
#include "tsmf_main.h"

typedef struct _TSMF_LISTENER_CALLBACK TSMF_LISTENER_CALLBACK;

typedef struct _TSMF_CHANNEL_CALLBACK TSMF_CHANNEL_CALLBACK;

typedef struct _TSMF_PLUGIN TSMF_PLUGIN;

struct _TSMF_LISTENER_CALLBACK
{
	IWTSListenerCallback iface;

	IWTSPlugin * plugin;
	IWTSVirtualChannelManager * channel_mgr;
};

struct _TSMF_CHANNEL_CALLBACK
{
	IWTSVirtualChannelCallback iface;

	IWTSPlugin * plugin;
	IWTSVirtualChannelManager * channel_mgr;
	IWTSVirtualChannel * channel;

	uint8 presentation_id[16];
	uint32 stream_id;
};

struct _TSMF_PLUGIN
{
	IWTSPlugin iface;

	TSMF_LISTENER_CALLBACK * listener_callback;
};

void
tsmf_playback_ack(IWTSVirtualChannelCallback * pChannelCallback,
	uint32 message_id, uint64 duration, uint32 data_size)
{
	TSMF_CHANNEL_CALLBACK * callback = (TSMF_CHANNEL_CALLBACK *) pChannelCallback;
	uint32 out_size;
	char * out_data;
	int error;

	out_size = 32;
	out_data = (char *) malloc(out_size);
	SET_UINT32(out_data, 0, TSMF_INTERFACE_CLIENT_NOTIFICATIONS | STREAM_ID_PROXY);
	SET_UINT32(out_data, 4, message_id);
	SET_UINT32(out_data, 8, PLAYBACK_ACK); /* FunctionId */
	SET_UINT32(out_data, 12, callback->stream_id); /* StreamId */
	SET_UINT64(out_data, 16, duration); /* DataDuration */
	SET_UINT64(out_data, 24, data_size); /* cbData */
	
	LLOGLN(10, ("tsmf_playback_ack: response size %d", out_size));
	error = callback->channel->Write(callback->channel, out_size, out_data, NULL);
	if (error)
	{
		LLOGLN(0, ("tsmf_playback_ack: response error %d", error));
	}
	free(out_data);
}

static int
tsmf_on_data_received(IWTSVirtualChannelCallback * pChannelCallback,
	uint32 cbSize,
	char * pBuffer)
{
	TSMF_CHANNEL_CALLBACK * callback = (TSMF_CHANNEL_CALLBACK *) pChannelCallback;
	TSMF_IFMAN ifman;
	uint32 InterfaceId;
	uint32 MessageId;
	uint32 FunctionId;
	int error = -1;
	uint32 out_size;
	char * out_data;

	/* 2.2.1 Shared Message Header (SHARED_MSG_HEADER) */
	if (cbSize < 12)
	{
		LLOGLN(0, ("tsmf_on_data_received: invalid size. cbSize=%d", cbSize));
		return 1;
	}
	InterfaceId = GET_UINT32(pBuffer, 0);
	MessageId = GET_UINT32(pBuffer, 4);
	FunctionId = GET_UINT32(pBuffer, 8);
	LLOGLN(10, ("tsmf_on_data_received: cbSize=%d InterfaceId=0x%X MessageId=0x%X FunctionId=0x%X",
		cbSize, InterfaceId, MessageId, FunctionId));

	memset(&ifman, 0, sizeof(TSMF_IFMAN));
	ifman.channel_callback = pChannelCallback;
	memcpy(ifman.presentation_id, callback->presentation_id, 16);
	ifman.stream_id = callback->stream_id;
	ifman.message_id = MessageId;
	ifman.input_buffer = (uint8 *) (pBuffer + 12);
	ifman.input_buffer_size = cbSize - 12;
	ifman.output_buffer = NULL;
	ifman.output_buffer_size = 0;
	ifman.output_pending = 0;
	ifman.output_interface_id = InterfaceId;

	switch (InterfaceId)
	{
		case TSMF_INTERFACE_CAPABILITIES | STREAM_ID_NONE:

			switch (FunctionId)
			{
				case RIM_EXCHANGE_CAPABILITY_REQUEST:
					error = tsmf_ifman_rim_exchange_capability_request(&ifman);
					break;

				default:
					break;
			}
			break;

		case TSMF_INTERFACE_DEFAULT | STREAM_ID_PROXY:

			switch (FunctionId)
			{
				case SET_CHANNEL_PARAMS:
					memcpy(callback->presentation_id, ifman.input_buffer, 16);
					callback->stream_id = GET_UINT32(ifman.input_buffer, 16);
					LLOGLN(10, ("tsmf_on_data_received: SET_CHANNEL_PARAMS StreamId=%d", callback->stream_id));
					ifman.output_pending = 1;
					error = 0;
					break;

				case EXCHANGE_CAPABILITIES_REQ:
					error = tsmf_ifman_exchange_capability_request(&ifman);
					break;

				case CHECK_FORMAT_SUPPORT_REQ:
					error = tsmf_ifman_check_format_support_request(&ifman);
					break;

				case ON_NEW_PRESENTATION:
					error = tsmf_ifman_on_new_presentation(&ifman);
					break;

				case ADD_STREAM:
					error = tsmf_ifman_add_stream(&ifman);
					break;

				case SET_TOPOLOGY_REQ:
					error = tsmf_ifman_set_topology_request(&ifman);
					break;

				case REMOVE_STREAM:
					error = tsmf_ifman_remove_stream(&ifman);
					break;

				case SHUTDOWN_PRESENTATION_REQ:
					error = tsmf_ifman_shutdown_presentation(&ifman);
					break;

				case ON_STREAM_VOLUME:
					error = tsmf_ifman_on_stream_volume(&ifman);
					break;

				case ON_CHANNEL_VOLUME:
					error = tsmf_ifman_on_channel_volume(&ifman);
					break;

				case SET_VIDEO_WINDOW:
					error = tsmf_ifman_set_video_window(&ifman);
					break;

				case UPDATE_GEOMETRY_INFO:
					error = tsmf_ifman_update_geometry_info(&ifman);
					break;

				case SET_ALLOCATOR:
					error = tsmf_ifman_set_allocator(&ifman);
					break;

				case NOTIFY_PREROLL:
					error = tsmf_ifman_notify_preroll(&ifman);
					break;

				case ON_SAMPLE:
					error = tsmf_ifman_on_sample(&ifman);
					break;

				case ON_FLUSH:
					error = tsmf_ifman_on_flush(&ifman);
					break;

				case ON_END_OF_STREAM:
					error = tsmf_ifman_on_end_of_stream(&ifman);
					break;

				case ON_PLAYBACK_STARTED:
					error = tsmf_ifman_on_playback_started(&ifman);
					break;

				case ON_PLAYBACK_PAUSED:
					error = tsmf_ifman_on_playback_paused(&ifman);
					break;

				case ON_PLAYBACK_RESTARTED:
					error = tsmf_ifman_on_playback_restarted(&ifman);
					break;

				case ON_PLAYBACK_STOPPED:
					error = tsmf_ifman_on_playback_stopped(&ifman);
					break;

				case ON_PLAYBACK_RATE_CHANGED:
					error = tsmf_ifman_on_playback_rate_changed(&ifman);
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	ifman.input_buffer = NULL;
	ifman.input_buffer_size = 0;

	if (error == -1)
	{
		switch (FunctionId)
		{
			case RIMCALL_RELEASE:
				/* [MS-RDPEXPS] 2.2.2.2 Interface Release (IFACE_RELEASE)
				   This message does not require a reply. */
				error = 0;
				ifman.output_pending = 1;
				break;

			case RIMCALL_QUERYINTERFACE:
				/* [MS-RDPEXPS] 2.2.2.1.2 Query Interface Response (QI_RSP)
				   This message is not supported in this channel. */
				error = 0;
				break;
		}

		if (error == -1)
		{
			LLOGLN(0, ("tsmf_on_data_received: InterfaceId 0x%X FunctionId 0x%X not processed.",
				InterfaceId, FunctionId));
			/* When a request is not implemented we return empty response indicating error */
		}
		error = 0;
	}

	if (error == 0 && !ifman.output_pending)
	{
		/* Response packet does not have FunctionId */
		out_size = 8 + ifman.output_buffer_size;
		out_data = (char *) malloc(out_size);
		memset(out_data, 0, out_size);
		SET_UINT32(out_data, 0, ifman.output_interface_id);
		SET_UINT32(out_data, 4, MessageId);
		if (ifman.output_buffer_size > 0)
			memcpy(out_data + 8, ifman.output_buffer, ifman.output_buffer_size);

		LLOGLN(10, ("tsmf_on_data_received: response size %d", out_size));
		error = callback->channel->Write(callback->channel, out_size, out_data, NULL);
		if (error)
		{
			LLOGLN(0, ("tsmf_on_data_received: response error %d", error));
		}

		free(out_data);
		if (ifman.output_buffer_size > 0)
		{
			free(ifman.output_buffer);
			ifman.output_buffer = NULL;
			ifman.output_buffer_size = 0;
		}
	}

	return error;
}

static int
tsmf_on_close(IWTSVirtualChannelCallback * pChannelCallback)
{
	LLOGLN(10, ("tsmf_on_close:"));
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

	LLOGLN(10, ("tsmf_on_new_channel_connection:"));
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

	LLOGLN(10, ("tsmf_plugin_initialize:"));
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

	LLOGLN(10, ("tsmf_plugin_terminated:"));
	if (tsmf->listener_callback)
		free(tsmf->listener_callback);
	free(tsmf);
	return 0;
}

int
DVCPluginEntry(IDRDYNVC_ENTRY_POINTS * pEntryPoints)
{
	TSMF_PLUGIN * tsmf;
	int ret = 0;

	tsmf = (TSMF_PLUGIN *) pEntryPoints->GetPlugin(pEntryPoints, "tsmf");
	if (tsmf == NULL)
	{
		tsmf = (TSMF_PLUGIN *) malloc(sizeof(TSMF_PLUGIN));
		memset(tsmf, 0, sizeof(TSMF_PLUGIN));

		tsmf->iface.Initialize = tsmf_plugin_initialize;
		tsmf->iface.Connected = NULL;
		tsmf->iface.Disconnected = NULL;
		tsmf->iface.Terminated = tsmf_plugin_terminated;
		ret = pEntryPoints->RegisterPlugin(pEntryPoints, "tsmf", (IWTSPlugin *) tsmf);
	}
	return ret;
}

