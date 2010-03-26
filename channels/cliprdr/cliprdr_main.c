/*
   Copyright (c) 2009-2010 Jay Sorg
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
#include <pthread.h>
#include <unistd.h>
#include <freerdp/types_ui.h>
#include <freerdp/vchan.h>
#include "chan_stream.h"
#include "chan_plugin.h"
#include "wait_obj.h"
#include "cliprdr_main.h"

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

struct data_in_item
{
	struct data_in_item * next;
	char * data;
	int data_size;
};

struct cliprdr_plugin
{
	rdpChanPlugin chan_plugin;

	CHANNEL_ENTRY_POINTS ep;
	CHANNEL_DEF channel_def;
	uint32 open_handle;
	char * data_in;
	int data_in_size;
	int data_in_read;
	struct wait_obj * term_event;
	struct wait_obj * data_in_event;
	struct data_in_item * in_list_head;
	struct data_in_item * in_list_tail;
	/* for locking the linked list */
	pthread_mutex_t * in_mutex;

	int thread_status;

	/* Device specific data */
	void * device_data;
};

/* called by main thread
   add item to linked list and inform worker thread that there is data */
static void
signal_data_in(cliprdrPlugin * plugin)
{
	struct data_in_item * item;

	item = (struct data_in_item *) malloc(sizeof(struct data_in_item));
	item->next = 0;
	item->data = plugin->data_in;
	plugin->data_in = 0;
	item->data_size = plugin->data_in_size;
	plugin->data_in_size = 0;
	pthread_mutex_lock(plugin->in_mutex);
	if (plugin->in_list_tail == 0)
	{
		plugin->in_list_head = item;
		plugin->in_list_tail = item;
	}
	else
	{
		plugin->in_list_tail->next = item;
		plugin->in_list_tail = item;
	}
	pthread_mutex_unlock(plugin->in_mutex);
	wait_obj_set(plugin->data_in_event);
}

int
cliprdr_send_packet(cliprdrPlugin * plugin, int type, int flag,
	char * data, int length)
{
	char * out_data;
	int size;
	uint32 error;

	LLOGLN(10, ("cliprdr_send_packet: type=%d, flag=%d, length=%d",
		type, flag, length));

	size = 12 + length;
	out_data = (char *) malloc(size);
	memset(out_data, 0, size);
	SET_UINT16(out_data, 0, (uint16)type);
	SET_UINT16(out_data, 2, (uint16)flag);
	SET_UINT32(out_data, 4, (uint32)length);
	if (data != 0)
	{
		memcpy(out_data + 8, data, length);
	}

	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
		out_data, size, out_data);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("cliprdr_send_packet: "
			"VirtualChannelWrite "
			"failed %d", error));
		return 1;
	}
	return 0;
}

static int
thread_process_message(cliprdrPlugin * plugin, char * data, int data_size)
{
	uint16 type;
	uint32 flag;
	uint32 length;
	uint32 format;

	type = GET_UINT16(data, 0);
	flag = GET_UINT16(data, 2);
	length = GET_UINT32(data, 4);

	LLOGLN(10, ("cliprdr: thread_process_message: type=%d flag=%d length=%d",
		type, flag, length));

	switch (type)
	{
		case CB_MONITOR_READY:
			clipboard_sync(plugin->device_data);
			break;
		case CB_FORMAT_LIST:
			clipboard_format_list(plugin->device_data, flag,
				data + 8, length);
			cliprdr_send_packet(plugin, CB_FORMAT_LIST_RESPONSE,
				CB_RESPONSE_OK, NULL, 0);
			break;
		case CB_FORMAT_LIST_RESPONSE:
			clipboard_format_list_response(plugin->device_data, flag);
			break;
		case CB_FORMAT_DATA_REQUEST:
			format = GET_UINT32(data, 8);
			clipboard_request_data(plugin->device_data, format);
			break;
		case CB_FORMAT_DATA_RESPONSE:
			clipboard_handle_data(plugin->device_data, flag,
				data + 8, length);
			break;
		case CB_CLIP_CAPS:
			clipboard_handle_caps(plugin->device_data, flag,
				data + 8, length);
			break;
		default:
			LLOGLN(0, ("thread_process_message: type %d not supported",
				type));
			break;
	}

	return 0;
}

/* process the linked list of data that has come in */
static int
thread_process_data_in(cliprdrPlugin * plugin)
{
	char * data;
	int data_size;
	struct data_in_item * item;

	while (1)
	{
		if (wait_obj_is_set(plugin->term_event))
		{
			break;
		}
		pthread_mutex_lock(plugin->in_mutex);
		if (plugin->in_list_head == 0)
		{
			pthread_mutex_unlock(plugin->in_mutex);
			break;
		}
		data = plugin->in_list_head->data;
		data_size = plugin->in_list_head->data_size;
		item = plugin->in_list_head;
		plugin->in_list_head = plugin->in_list_head->next;
		if (plugin->in_list_head == 0)
		{
			plugin->in_list_tail = 0;
		}
		pthread_mutex_unlock(plugin->in_mutex);
		if (data != 0)
		{
			thread_process_message(plugin, data, data_size);
			free(data);
		}
		if (item != 0)
		{
			free(item);
		}
	}
	return 0;
}

static void *
thread_func(void * arg)
{
	cliprdrPlugin * plugin;
	struct wait_obj * listobj[2];
	int numobj;

	plugin = (cliprdrPlugin *) arg;

	plugin->thread_status = 1;
	LLOGLN(10, ("cliprdr_main thread_func: in"));
	while (1)
	{
		listobj[0] = plugin->term_event;
		listobj[1] = plugin->data_in_event;
		numobj = 2;
		wait_obj_select(listobj, numobj, NULL, 0, 500);

		if (wait_obj_is_set(plugin->term_event))
		{
			break;
		}
		if (wait_obj_is_set(plugin->data_in_event))
		{
			wait_obj_clear(plugin->data_in_event);
			/* process data in */
			thread_process_data_in(plugin);
		}
	}
	LLOGLN(10, ("cliprdr_main thread_func: out"));
	plugin->thread_status = -1;
	return 0;
}

static void
OpenEventProcessReceived(uint32 openHandle, void * pData, uint32 dataLength,
	uint32 totalLength, uint32 dataFlags)
{
	cliprdrPlugin * plugin;

	plugin = (cliprdrPlugin *) chan_plugin_find_by_open_handle(openHandle);

	LLOGLN(10, ("OpenEventProcessReceived: receive openHandle %d dataLength %d "
		"totalLength %d dataFlags %d",
		openHandle, dataLength, totalLength, dataFlags));
	if (dataFlags & CHANNEL_FLAG_FIRST)
	{
		plugin->data_in_read = 0;
		if (plugin->data_in != 0)
		{
			free(plugin->data_in);
		}
		plugin->data_in = (char *) malloc(totalLength);
		plugin->data_in_size = totalLength;
	}
	memcpy(plugin->data_in + plugin->data_in_read, pData, dataLength);
	plugin->data_in_read += dataLength;
	if (dataFlags & CHANNEL_FLAG_LAST)
	{
		if (plugin->data_in_read != plugin->data_in_size)
		{
			LLOGLN(0, ("OpenEventProcessReceived: read error"));
		}
		signal_data_in(plugin);
	}
}

static void
OpenEvent(uint32 openHandle, uint32 event, void * pData, uint32 dataLength,
	uint32 totalLength, uint32 dataFlags)
{
	LLOGLN(10, ("OpenEvent: event %d", event));
	switch (event)
	{
		case CHANNEL_EVENT_DATA_RECEIVED:
			OpenEventProcessReceived(openHandle, pData, dataLength,
				totalLength, dataFlags);
			break;
		case CHANNEL_EVENT_WRITE_COMPLETE:
			free(pData);
			break;
	}
}

static void
InitEventProcessConnected(void * pInitHandle, void * pData, uint32 dataLength)
{
	cliprdrPlugin * plugin;
	uint32 error;
	pthread_t thread;

	plugin = (cliprdrPlugin *) chan_plugin_find_by_init_handle(pInitHandle);
	if (plugin == NULL)
	{
		LLOGLN(0, ("InitEventProcessConnected: error no match"));
		return;
	}

	error = plugin->ep.pVirtualChannelOpen(pInitHandle, &(plugin->open_handle),
		plugin->channel_def.name, OpenEvent);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("InitEventProcessConnected: Open failed"));
		return;
	}
	chan_plugin_register_open_handle((rdpChanPlugin *) plugin, plugin->open_handle);

	pthread_create(&thread, 0, thread_func, plugin);
	pthread_detach(thread);
}

static void
InitEventProcessTerminated(void * pInitHandle)
{
	cliprdrPlugin * plugin;
	int index;
	struct data_in_item * in_item;

	plugin = (cliprdrPlugin *) chan_plugin_find_by_init_handle(pInitHandle);
	if (plugin == NULL)
	{
		LLOGLN(0, ("InitEventProcessConnected: error no match"));
		return;
	}

	wait_obj_set(plugin->term_event);
	index = 0;
	while ((plugin->thread_status > 0) && (index < 100))
	{
		index++;
		usleep(250 * 1000);
	}
	wait_obj_free(plugin->term_event);
	wait_obj_free(plugin->data_in_event);

	pthread_mutex_destroy(plugin->in_mutex);
	free(plugin->in_mutex);

	/* free the un-processed in/out queue */
	while (plugin->in_list_head != 0)
	{
		in_item = plugin->in_list_head;
		plugin->in_list_head = in_item->next;
		free(in_item->data);
		free(in_item);
	}
	if (plugin->data_in != 0)
	{
		free(plugin->data_in);
	}

	clipboard_free(plugin->device_data);
	chan_plugin_uninit((rdpChanPlugin *) plugin);
	free(plugin);
}

static void
InitEvent(void * pInitHandle, uint32 event, void * pData, uint32 dataLength)
{
	LLOGLN(10, ("InitEvent: event %d", event));
	switch (event)
	{
		case CHANNEL_EVENT_CONNECTED:
			InitEventProcessConnected(pInitHandle, pData, dataLength);
			break;
		case CHANNEL_EVENT_DISCONNECTED:
			break;
		case CHANNEL_EVENT_TERMINATED:
			InitEventProcessTerminated(pInitHandle);
			break;
	}
}

/* this registers two channels but only cliprdr is used */
int
VirtualChannelEntry(PCHANNEL_ENTRY_POINTS pEntryPoints)
{
	cliprdrPlugin * plugin;

	LLOGLN(10, ("VirtualChannelEntry:"));

	plugin = (cliprdrPlugin *) malloc(sizeof(cliprdrPlugin));
	memset(plugin, 0, sizeof(cliprdrPlugin));

	chan_plugin_init((rdpChanPlugin *) plugin);

	plugin->data_in_size = 0;
	plugin->data_in = 0;
	plugin->ep = *pEntryPoints;
	memset(&(plugin->channel_def), 0, sizeof(plugin->channel_def));
	plugin->channel_def.options = CHANNEL_OPTION_INITIALIZED |
		CHANNEL_OPTION_ENCRYPT_RDP | CHANNEL_OPTION_COMPRESS_RDP |
		CHANNEL_OPTION_SHOW_PROTOCOL;
	strcpy(plugin->channel_def.name, "cliprdr");
	plugin->in_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(plugin->in_mutex, 0);
	plugin->in_list_head = 0;
	plugin->in_list_tail = 0;
	plugin->term_event = wait_obj_new("freerdpcliprdrterm");
	plugin->data_in_event = wait_obj_new("freerdpcliprdrdatain");
	plugin->ep.pVirtualChannelInit(&plugin->chan_plugin.init_handle, &plugin->channel_def, 1,
		VIRTUAL_CHANNEL_VERSION_WIN2000, InitEvent);
	plugin->device_data = clipboard_new(plugin);
	return 1;
}
