/*
   FreeRDP: A Remote Desktop Protocol client.
   Dynamic Virtual Channel

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
#include <dlfcn.h>
#include "drdynvc_types.h"
#include "dvcman.h"

#define MAX_PLUGINS 10

typedef struct _DVCMAN DVCMAN;
struct _DVCMAN
{
	IWTSVirtualChannelManager iface;

	drdynvcPlugin * drdynvc;

	const char * plugin_names[MAX_PLUGINS];
	IWTSPlugin * plugins[MAX_PLUGINS];
	int num_plugins;

	IWTSListener * listeners[MAX_PLUGINS];
	int num_listeners;

	IWTSVirtualChannel * channel_list_head;
	IWTSVirtualChannel * channel_list_tail;
};

typedef struct _DVCMAN_LISTENER DVCMAN_LISTENER;
struct _DVCMAN_LISTENER
{
	IWTSListener iface;

	DVCMAN * dvcman;
	char * channel_name;
	uint32 flags;
	IWTSListenerCallback * listener_callback;
};

typedef struct _DVCMAN_ENTRY_POINTS DVCMAN_ENTRY_POINTS;
struct _DVCMAN_ENTRY_POINTS
{
	IDRDYNVC_ENTRY_POINTS iface;

	DVCMAN * dvcman;
	RD_PLUGIN_DATA * plugin_data;
};

typedef struct _DVCMAN_CHANNEL DVCMAN_CHANNEL;
struct _DVCMAN_CHANNEL
{
	IWTSVirtualChannel iface;

	DVCMAN * dvcman;
	DVCMAN_CHANNEL * next;
	uint32 channel_id;
	IWTSVirtualChannelCallback * channel_callback;

	char * dvc_data;
	uint32 dvc_data_pos;
	uint32 dvc_data_size;
};

static int
dvcman_get_configuration(IWTSListener * pListener,
	void ** ppPropertyBag)
{
	*ppPropertyBag = NULL;
	return 1;
}

static int
dvcman_create_listener(IWTSVirtualChannelManager * pChannelMgr,
	const char * pszChannelName,
	uint32 ulFlags,
	IWTSListenerCallback * pListenerCallback,
	IWTSListener ** ppListener)
{
	DVCMAN * dvcman = (DVCMAN *) pChannelMgr;
	DVCMAN_LISTENER * listener;

	if (dvcman->num_listeners < MAX_PLUGINS)
	{
		LLOGLN(10, ("dvcman_create_listener: %d.%s.", dvcman->num_listeners, pszChannelName));
		listener = (DVCMAN_LISTENER *) malloc(sizeof(DVCMAN_LISTENER));
		memset(listener, 0, sizeof(DVCMAN_LISTENER));
		listener->iface.GetConfiguration = dvcman_get_configuration;
		listener->dvcman = dvcman;
		listener->channel_name = strdup(pszChannelName);
		listener->flags = ulFlags;
		listener->listener_callback = pListenerCallback;

		if (ppListener)
			*ppListener = (IWTSListener *) listener;
		dvcman->listeners[dvcman->num_listeners++] = (IWTSListener *) listener;
		return 0;
	}
	else
	{
		LLOGLN(0, ("dvcman_create_listener: Maximum DVC listener number reached."));
		return 1;
	}
}

static int
dvcman_push_event(IWTSVirtualChannelManager * pChannelMgr,
	RD_EVENT * pEvent)
{
	DVCMAN * dvcman = (DVCMAN *) pChannelMgr;
	int error;

	error = drdynvc_push_event(dvcman->drdynvc, pEvent);
	if (error == 0)
	{
		LLOGLN(10, ("dvcman_push_event: event_type %d pushed.", pEvent->event_type));
	}
	else
	{
		LLOGLN(0, ("dvcman_push_event: event_type %d push failed.", pEvent->event_type));
	}
	return error;
}

static int
dvcman_register_plugin(IDRDYNVC_ENTRY_POINTS * pEntryPoints,
	const char * name, IWTSPlugin * pPlugin)
{
	DVCMAN * dvcman = ((DVCMAN_ENTRY_POINTS *)pEntryPoints)->dvcman;

	if (dvcman->num_plugins < MAX_PLUGINS)
	{
		LLOGLN(0, ("dvcman_register_plugin: %d", dvcman->num_plugins));
		dvcman->plugin_names[dvcman->num_plugins] = name;
		dvcman->plugins[dvcman->num_plugins++] = pPlugin;
		return 0;
	}
	else
	{
		LLOGLN(0, ("dvcman_register_plugin: Maximum DVC plugin number reached."));
		return 1;
	}
}

IWTSPlugin *
dvcman_get_plugin(IDRDYNVC_ENTRY_POINTS * pEntryPoints,
	const char * name)
{
	DVCMAN * dvcman = ((DVCMAN_ENTRY_POINTS *)pEntryPoints)->dvcman;
	int i;

	for (i = 0; i < dvcman->num_plugins; i++)
	{
		if (dvcman->plugin_names[i] == name ||
			strcmp(dvcman->plugin_names[i], name) == 0)
		{
			return dvcman->plugins[i];
		}
	}
	return NULL;
}

RD_PLUGIN_DATA *
dvcman_get_plugin_data(IDRDYNVC_ENTRY_POINTS * pEntryPoints)
{
	return ((DVCMAN_ENTRY_POINTS *)pEntryPoints)->plugin_data;
}

IWTSVirtualChannelManager *
dvcman_new(drdynvcPlugin * plugin)
{
	DVCMAN * dvcman;

	dvcman = (DVCMAN *) malloc(sizeof(DVCMAN));
	memset(dvcman, 0, sizeof(DVCMAN));
	dvcman->iface.CreateListener = dvcman_create_listener;
	dvcman->iface.PushEvent = dvcman_push_event;
	dvcman->drdynvc = plugin;
	dvcman->num_plugins = 0;
	dvcman->num_listeners = 0;
	dvcman->channel_list_head = NULL;
	dvcman->channel_list_tail = NULL;

	return (IWTSVirtualChannelManager *) dvcman;
}

int
dvcman_load_plugin(IWTSVirtualChannelManager * pChannelMgr, RD_PLUGIN_DATA * data)
{
	char * filename = (char *) data->data[0];
	DVCMAN_ENTRY_POINTS entryPoints;
	void* dl;
	char* fn;
	PDVC_PLUGIN_ENTRY pDVCPluginEntry = NULL;

	if (strchr(filename, '/'))
	{
		fn = strdup(filename);
	}
	else
	{
		fn = malloc(strlen(PLUGIN_PATH) + strlen(filename) + 10);
		sprintf(fn, PLUGIN_PATH "/%s.so", filename);
	}
	dl = dlopen(fn, RTLD_LOCAL | RTLD_LAZY);

	pDVCPluginEntry = (PDVC_PLUGIN_ENTRY)dlsym(dl, "DVCPluginEntry");

	if(pDVCPluginEntry != NULL)
	{
		entryPoints.iface.RegisterPlugin = dvcman_register_plugin;
		entryPoints.iface.GetPlugin = dvcman_get_plugin;
		entryPoints.iface.GetPluginData = dvcman_get_plugin_data;
		entryPoints.dvcman = (DVCMAN *) pChannelMgr;
		entryPoints.plugin_data = data;
		pDVCPluginEntry((IDRDYNVC_ENTRY_POINTS *) &entryPoints);
		LLOGLN(0, ("loaded DVC plugin: %s", fn));
	}
	free(fn);

	return 0;
}

void
dvcman_free(IWTSVirtualChannelManager * pChannelMgr)
{
	DVCMAN * dvcman = (DVCMAN *) pChannelMgr;
	int i;
	IWTSPlugin * pPlugin;
	DVCMAN_LISTENER * listener;

	while (dvcman->channel_list_head)
		dvcman->channel_list_head->Close(dvcman->channel_list_head);
	for (i = 0; i < dvcman->num_listeners; i++)
	{
		listener = (DVCMAN_LISTENER *) dvcman->listeners[i];
		free(listener->channel_name);
		free(listener);
	}
	for (i = 0; i < dvcman->num_plugins; i++)
	{
		pPlugin = dvcman->plugins[i];
		if (pPlugin->Terminated)
			pPlugin->Terminated(pPlugin);
	}
	free(dvcman);
}

int
dvcman_initialize(IWTSVirtualChannelManager * pChannelMgr)
{
	DVCMAN * dvcman = (DVCMAN *) pChannelMgr;
	int i;
	IWTSPlugin * pPlugin;

	for (i = 0; i < dvcman->num_plugins; i++)
	{
		pPlugin = dvcman->plugins[i];
		if (pPlugin->Initialize)
			pPlugin->Initialize(pPlugin, pChannelMgr);
	}
	return 0;
}

static int
dvcman_write_channel(IWTSVirtualChannel * pChannel,
	uint32 cbSize,
	char * pBuffer,
	void * pReserved)
{
	DVCMAN_CHANNEL * channel = (DVCMAN_CHANNEL *) pChannel;

	return drdynvc_write_data(channel->dvcman->drdynvc, channel->channel_id, pBuffer, cbSize);
}

static int
dvcman_close_channel_iface(IWTSVirtualChannel * pChannel)
{
	DVCMAN_CHANNEL * channel = (DVCMAN_CHANNEL *) pChannel;
	DVCMAN * dvcman = channel->dvcman;
	DVCMAN_CHANNEL * prev;
	DVCMAN_CHANNEL * curr;

	LLOGLN(10, ("dvcman_channel_close: id=%d", channel->channel_id));
	for (prev = NULL, curr = (DVCMAN_CHANNEL *) dvcman->channel_list_head; curr; prev = curr, curr = curr->next)
	{
		if (curr == channel)
		{
			if (prev)
				prev->next = curr->next;
			if (dvcman->channel_list_head == pChannel)
				dvcman->channel_list_head = (IWTSVirtualChannel *) curr->next;
			if (dvcman->channel_list_tail == pChannel)
				dvcman->channel_list_tail = (IWTSVirtualChannel *) prev;

			if (channel->channel_callback)
				channel->channel_callback->OnClose(channel->channel_callback);
			free(channel);
			return 0;
		}
	}
	LLOGLN(0, ("dvcman_channel_close: channel not found"));
	return 1;
}

int
dvcman_create_channel(IWTSVirtualChannelManager * pChannelMgr, uint32 ChannelId, const char * ChannelName)
{
	DVCMAN * dvcman = (DVCMAN *) pChannelMgr;
	int i;
	DVCMAN_LISTENER * listener;
	DVCMAN_CHANNEL * channel;
	int bAccept;
	IWTSVirtualChannelCallback * pCallback;

	for (i = 0; i < dvcman->num_listeners; i++)
	{
		listener = (DVCMAN_LISTENER *) dvcman->listeners[i];
		if (strcmp(listener->channel_name, ChannelName) == 0)
		{
			channel = (DVCMAN_CHANNEL *) malloc(sizeof(DVCMAN_CHANNEL));
			memset(channel, 0, sizeof(DVCMAN_CHANNEL));
			channel->iface.Write = dvcman_write_channel;
			channel->iface.Close = dvcman_close_channel_iface;
			channel->dvcman = dvcman;
			channel->next = NULL;
			channel->channel_id = ChannelId;

			bAccept = 1;
			pCallback = NULL;
			if (listener->listener_callback->OnNewChannelConnection(listener->listener_callback,
				(IWTSVirtualChannel *) channel, NULL, &bAccept, &pCallback) == 0 && bAccept == 1)
			{
				LLOGLN(0, ("dvcman_create_channel: listener %s created new channel %d",
					listener->channel_name, channel->channel_id));
				channel->channel_callback = pCallback;
				if (dvcman->channel_list_tail)
				{
					((DVCMAN_CHANNEL *) dvcman->channel_list_tail)->next = channel;
					dvcman->channel_list_tail = (IWTSVirtualChannel *) channel;
				}
				else
				{
					dvcman->channel_list_head = (IWTSVirtualChannel *) channel;
					dvcman->channel_list_tail = (IWTSVirtualChannel *) channel;
				}
				return 0;
			}
			else
			{
				LLOGLN(0, ("dvcman_create_channel: channel rejected by plugin"));
				free(channel);
				return 1;
			}
		}
	}
	return 1;
}

static DVCMAN_CHANNEL *
dvcman_find_channel_by_id(IWTSVirtualChannelManager * pChannelMgr, uint32 ChannelId)
{
	DVCMAN * dvcman = (DVCMAN *) pChannelMgr;
	DVCMAN_CHANNEL * curr;

	for (curr = (DVCMAN_CHANNEL *) dvcman->channel_list_head; curr; curr = curr->next)
	{
		if (curr->channel_id == ChannelId)
		{
			return curr;
		}
	}
	return NULL;
}

int
dvcman_close_channel(IWTSVirtualChannelManager * pChannelMgr, uint32 ChannelId)
{
	DVCMAN_CHANNEL * channel;
	IWTSVirtualChannel * ichannel;

	channel = dvcman_find_channel_by_id(pChannelMgr, ChannelId);
	if (channel == NULL)
	{
		LLOGLN(0, ("dvcman_close_channel: ChannelId %d not found!", ChannelId));
		return 1;
	}
	if (channel->dvc_data)
	{
		free(channel->dvc_data);
		channel->dvc_data = NULL;
	}
	LLOGLN(0, ("dvcman_close_channel: channel %d closed", ChannelId));
	ichannel = (IWTSVirtualChannel *) channel;
	ichannel->Close(ichannel);
	return 0;
}

int
dvcman_receive_channel_data_first(IWTSVirtualChannelManager * pChannelMgr, uint32 ChannelId, uint32 length)
{
	DVCMAN_CHANNEL * channel;

	channel = dvcman_find_channel_by_id(pChannelMgr, ChannelId);
	if (channel == NULL)
	{
		LLOGLN(0, ("dvcman_receive_channel_data: ChannelId %d not found!", ChannelId));
		return 1;
	}
	if (channel->dvc_data)
		free(channel->dvc_data);
	channel->dvc_data = (char *) malloc(length);
	memset(channel->dvc_data, 0, length);
	channel->dvc_data_pos = 0;
	channel->dvc_data_size = length;

	return 0;
}

int
dvcman_receive_channel_data(IWTSVirtualChannelManager * pChannelMgr, uint32 ChannelId, char * data, uint32 data_size)
{
	DVCMAN_CHANNEL * channel;
	int error = 0;

	channel = dvcman_find_channel_by_id(pChannelMgr, ChannelId);
	if (channel == NULL)
	{
		LLOGLN(0, ("dvcman_receive_channel_data: ChannelId %d not found!", ChannelId));
		return 1;
	}

	if (channel->dvc_data)
	{
		/* Fragmented data */
		if (channel->dvc_data_pos + (uint32) data_size > channel->dvc_data_size)
		{
			LLOGLN(0, ("dvcman_receive_channel_data: data exceeding declared length!"));
			free(channel->dvc_data);
			channel->dvc_data = NULL;
			return 1;
		}
		memcpy(channel->dvc_data + channel->dvc_data_pos, data, data_size);
		channel->dvc_data_pos += (uint32) data_size;
		if (channel->dvc_data_pos >= channel->dvc_data_size)
		{
			error = channel->channel_callback->OnDataReceived(channel->channel_callback,
				channel->dvc_data_size, channel->dvc_data);
			free(channel->dvc_data);
			channel->dvc_data = NULL;
		}
	}
	else
	{
		error = channel->channel_callback->OnDataReceived(channel->channel_callback,
			data_size, data);
	}

	return error;
}

