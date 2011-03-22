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
#include "tsmf_constants.h"
#include "tsmf_types.h"

static int
tsmf_process_interface_capability_request(IWTSVirtualChannelCallback * pChannelCallback)
{
	TSMF_CHANNEL_CALLBACK * callback = (TSMF_CHANNEL_CALLBACK *) pChannelCallback;
	uint32 CapabilityValue;

	CapabilityValue = GET_UINT32(callback->input_buffer, 0);
	LLOGLN(0, ("tsmf_process_capability_request: server CapabilityValue %d", CapabilityValue));

	callback->output_buffer_size = 8;
	callback->output_buffer = malloc(8);
	SET_UINT32(callback->output_buffer, 0, 1); /* CapabilityValue */
	SET_UINT32(callback->output_buffer, 4, 0); /* Result */

	return 0;
}

static int
tsmf_process_channel_params(IWTSVirtualChannelCallback * pChannelCallback)
{
	TSMF_CHANNEL_CALLBACK * callback = (TSMF_CHANNEL_CALLBACK *) pChannelCallback;
	/* TODO: store the information */

	callback->output_pending = 1;
	return 0;
}

static int
tsmf_process_capability_request(IWTSVirtualChannelCallback * pChannelCallback)
{
	TSMF_CHANNEL_CALLBACK * callback = (TSMF_CHANNEL_CALLBACK *) pChannelCallback;
	char * p;
	uint32 numHostCapabilities;
	uint32 i;
	uint32 CapabilityType;
	uint32 cbCapabilityLength;
	uint32 v;

	callback->output_buffer_size = callback->input_buffer_size + 4;
	callback->output_buffer = malloc(callback->output_buffer_size);
	memcpy(callback->output_buffer, callback->input_buffer, callback->input_buffer_size);
	SET_UINT32(callback->output_buffer, callback->input_buffer_size, 0); /* Result */

	numHostCapabilities = GET_UINT32(callback->output_buffer, 0);
	p = callback->output_buffer + 4;
	for (i = 0; i < numHostCapabilities; i++)
	{
		CapabilityType = GET_UINT32(p, 0);
		cbCapabilityLength = GET_UINT32(p, 4);
		switch (CapabilityType)
		{
			case 1: /* Protocol version request */
				v = GET_UINT32(p, 8);
				LLOGLN(0, ("tsmf_process_capability_request: server protocol version %d", v));
				break;
			case 2: /* Supported platform */
				v = GET_UINT32(p, 8);
				LLOGLN(0, ("tsmf_process_capability_request: server supported platform %d", v));
				/* Claim that we support both MF and DShow platforms. */
				SET_UINT32(p, 8, MMREDIR_CAPABILITY_PLATFORM_MF | MMREDIR_CAPABILITY_PLATFORM_DSHOW);
				break;
			default:
				LLOGLN(0, ("tsmf_process_capability_request: unknown capability type %d", CapabilityType));
				break;
		}
		p += 8 + cbCapabilityLength;
	}
	callback->output_interface_id = TSMF_INTERFACE_DEFAULT | STREAM_ID_STUB;
	return 0;
}

static int
tsmf_on_data_received(IWTSVirtualChannelCallback * pChannelCallback,
	uint32 cbSize,
	char * pBuffer)
{
	TSMF_CHANNEL_CALLBACK * callback = (TSMF_CHANNEL_CALLBACK *) pChannelCallback;
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
	LLOGLN(0, ("tsmf_on_data_received: cbSize=%d InterfaceId=0x%X MessageId=0x%X FunctionId=0x%X",
		cbSize, InterfaceId, MessageId, FunctionId));

	callback->input_buffer = pBuffer + 12;
	callback->input_buffer_size = cbSize - 12;
	callback->output_buffer = NULL;
	callback->output_buffer_size = 0;
	callback->output_pending = 0;
	callback->output_interface_id = InterfaceId;

	switch (InterfaceId)
	{
		case TSMF_INTERFACE_CAPABILITIES | STREAM_ID_NONE:

			switch (FunctionId)
			{
				case RIM_EXCHANGE_CAPABILITY_REQUEST:
					error = tsmf_process_interface_capability_request(pChannelCallback);
					break;

				default:
					break;
			}
			break;

		case TSMF_INTERFACE_DEFAULT | STREAM_ID_PROXY:

			switch (FunctionId)
			{
				case SET_CHANNEL_PARAMS:
					error = tsmf_process_channel_params(pChannelCallback);
					break;

				case EXCHANGE_CAPABILITIES_REQ:
					error = tsmf_process_capability_request(pChannelCallback);
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

	callback->input_buffer = NULL;
	callback->input_buffer_size = 0;

	if (error == -1)
	{
		switch (FunctionId)
		{
			case RIMCALL_RELEASE:
				/* [MS-RDPEXPS] 2.2.2.2 Interface Release (IFACE_RELEASE)
				   This message does not require a reply. */
				error = 0;
				callback->output_pending = 1;
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

	if (error == 0 && !callback->output_pending)
	{
		/* Response packet does not have FunctionId */
		out_size = 8 + callback->output_buffer_size;
		out_data = (char *) malloc(out_size);
		memset(out_data, 0, out_size);
		SET_UINT32(out_data, 0, callback->output_interface_id);
		SET_UINT32(out_data, 4, MessageId);
		if (callback->output_buffer_size > 0)
			memcpy(out_data + 8, callback->output_buffer, callback->output_buffer_size);

		LLOGLN(0, ("tsmf_on_data_received: response size %d", out_size));
		error = callback->channel->Write(callback->channel, out_size, out_data, NULL);
		if (error)
		{
			LLOGLN(0, ("tsmf_on_data_received: response error %d", error));
		}

		free(out_data);
		if (callback->output_buffer_size > 0)
		{
			free(callback->output_buffer);
			callback->output_buffer = NULL;
			callback->output_buffer_size = 0;
		}
	}

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

