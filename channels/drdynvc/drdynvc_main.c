/*
   FreeRDP: A Remote Desktop Protocol client.
   Channels

   Copyright (C) Jay Sorg 2010-2011

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
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include "drdynvc_types.h"
#include "wait_obj.h"
#include "dvcman.h"
#include "drdynvc_main.h"

#define CREATE_REQUEST_PDU     0x01
#define DATA_FIRST_PDU         0x02
#define DATA_PDU               0x03
#define CLOSE_REQUEST_PDU      0x04
#define CAPABILITY_REQUEST_PDU 0x05

struct data_in_item
{
	struct data_in_item * next;
	char * data;
	int data_size;
};

struct drdynvc_plugin
{
	rdpChanPlugin chan_plugin;

	CHANNEL_ENTRY_POINTS ep;
	PVIRTUALCHANNELEVENTPUSH ep_event_push;
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
	int version;
	int PriorityCharge0;
	int PriorityCharge1;
	int PriorityCharge2;
	int PriorityCharge3;

	IWTSVirtualChannelManager * channel_mgr;
};

#if LOG_LEVEL > 10
void
hexdump(char* p, int len)
{
	unsigned char* line;
	int i;
	int thisline;
	int offset;

	line = (unsigned char*)p;
	offset = 0;
	while (offset < len)
	{
		printf("%04x ", offset);
		thisline = len - offset;
		if (thisline > 16)
		{
		  thisline = 16;
		}
		for (i = 0; i < thisline; i++)
		{
		  printf("%02x ", line[i]);
		}
		for (; i < 16; i++)
		{
		  printf("   ");
		}
		for (i = 0; i < thisline; i++)
		{
		  printf("%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');
		}
		printf("\n");
		offset += thisline;
		line += thisline;
	}
}
#else
#define hexdump(p,len)
#endif

static int
set_variable_uint(uint32 val, char * data, uint32 * pos)
{
	int cb;

	if (val <= 0xFF)
	{
		cb = 0;
		SET_UINT8(data, *pos, val);
		*pos += 1;
	}
	else if (val <= 0xFFFF)
	{
		cb = 1;
		SET_UINT16(data, *pos, val);
		*pos += 2;
	}
	else
	{
		cb = 3;
		SET_UINT32(data, *pos, val);
		*pos += 4;
	}
	return cb;
}

int
drdynvc_write_data(drdynvcPlugin * plugin, uint32 ChannelId, char * data, uint32 data_size)
{
	uint32 pos;
	uint32 t;
	int cbChId;
	int cbLen;
	char * out_data = NULL;
	int error;
	uint32 data_pos;

	LLOGLN(10, ("drdynvc_write_data: ChannelId=%d size=%d", ChannelId, data_size));

	out_data = (char *) malloc(CHANNEL_CHUNK_LENGTH);
	memset(out_data, 0, CHANNEL_CHUNK_LENGTH);
	pos = 1;
	cbChId = set_variable_uint(ChannelId, out_data, &pos);

	if (data_size <= CHANNEL_CHUNK_LENGTH - pos)
	{
		SET_UINT8(out_data, 0, 0x30 | cbChId);
		memcpy(out_data + pos, data, data_size);
		hexdump(out_data, data_size + pos);
		error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
			out_data, data_size + pos, out_data);
	}
	else
	{
		/* Fragment the data */
		cbLen = set_variable_uint(data_size, out_data, &pos);
		SET_UINT8(out_data, 0, 0x20 | cbChId | (cbLen << 2));
		data_pos = CHANNEL_CHUNK_LENGTH - pos;
		memcpy(out_data + pos, data, data_pos);
		hexdump(out_data, CHANNEL_CHUNK_LENGTH);
		error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
			out_data, CHANNEL_CHUNK_LENGTH, out_data);

		while (error == CHANNEL_RC_OK && data_pos < data_size)
		{
			out_data = (char *) malloc(CHANNEL_CHUNK_LENGTH);
			memset(out_data, 0, CHANNEL_CHUNK_LENGTH);
			pos = 1;
			cbChId = set_variable_uint(ChannelId, out_data, &pos);

			SET_UINT8(out_data, 0, 0x30 | cbChId);
			t = data_size - data_pos;
			if (t > CHANNEL_CHUNK_LENGTH - pos)
				t = CHANNEL_CHUNK_LENGTH - pos;
			memcpy(out_data + pos, data + data_pos, t);
			data_pos += t;
			hexdump(out_data, t + pos);
			error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
				out_data, t + pos, out_data);
		}
	}
	if (error != CHANNEL_RC_OK)
	{
		if (out_data)
			free(out_data);
		LLOGLN(0, ("drdynvc_write_data: "
			"VirtualChannelWrite "
			"failed %d", error));
		return 1;
	}
	return 0;
}

int
drdynvc_push_event(drdynvcPlugin * plugin, RD_EVENT * event)
{
	int error;

	if (!plugin->ep_event_push)
	{
		LLOGLN(0, ("drdynvc_push_event: channel plugin API does not support extensions."));
		return 1;
	}
	error = plugin->ep_event_push(plugin->open_handle, event);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("drdynvc_push_event: pVirtualChannelEventPush failed %d", error));
		return 1;
	}
	return 0;
}

/* called by main thread
   add item to linked list and inform worker thread that there is data */
static void
signal_data_in(drdynvcPlugin * plugin)
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

static int
process_CAPABILITY_REQUEST_PDU(drdynvcPlugin * plugin, int Sp, int cbChId,
	char * data, int data_size)
{
	int error;
	int size;
	char * out_data;

	LLOGLN(10, ("process_CAPABILITY_REQUEST_PDU:"));
	plugin->version = GET_UINT16(data, 2);
	if (plugin->version == 2)
	{
		plugin->PriorityCharge0 = GET_UINT16(data, 4);
		plugin->PriorityCharge1 = GET_UINT16(data, 6);
		plugin->PriorityCharge2 = GET_UINT16(data, 8);
		plugin->PriorityCharge3 = GET_UINT16(data, 10);
	}
	size = 4;
	out_data = (char *) malloc(size);
	SET_UINT16(out_data, 0, 0x0050); /* Cmd+Sp+cbChId+Pad. Note: MSTSC sends 0x005c */
	SET_UINT16(out_data, 2, plugin->version);
	hexdump(out_data, 4);
	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
	out_data, size, out_data);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("process_CAPABILITY_REQUEST_PDU: "
			"VirtualChannelWrite "
			"failed %d", error));
		return 1;
	}
	return 0;
}

static uint32
get_variable_uint(int cbLen, char * data, int * pos)
{
	uint32 val;

	switch (cbLen)
	{
		case 0:
			val = (uint32) GET_UINT8(data, *pos);
			*pos += 1;
			break;
		case 1:
			val = (uint32) GET_UINT16(data, *pos);
			*pos += 2;
			break;
		default:
			val = (uint32) GET_UINT32(data, *pos);
			*pos += 4;
			break;
	}
	return val;
}

static int
process_CREATE_REQUEST_PDU(drdynvcPlugin * plugin, int Sp, int cbChId,
	char * data, int data_size)
{
	int pos;
	int error;
	int size;
	char * out_data;
	uint32 ChannelId;

	pos = 1;
	ChannelId = get_variable_uint(cbChId, data, &pos);
	LLOGLN(10, ("process_CREATE_REQUEST_PDU: ChannelId=%d ChannelName=%s", ChannelId, data + pos));

	size = pos + 4;
	out_data = (char *) malloc(size);
	SET_UINT8(out_data, 0, 0x10 | cbChId);
	memcpy(out_data + 1, data + 1, pos - 1);
	
	error = dvcman_create_channel(plugin->channel_mgr, ChannelId, data + pos);
	if (error == 0)
	{
		LLOGLN(10, ("process_CREATE_REQUEST_PDU: channel created"));
		SET_UINT32(out_data, pos, 0);
	}
	else
	{
		LLOGLN(10, ("process_CREATE_REQUEST_PDU: no listener"));
		SET_UINT32(out_data, pos, (uint32)(-1));
	}
	hexdump(out_data, size);
	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
	out_data, size, out_data);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("process_CREATE_REQUEST_PDU: "
			"VirtualChannelWrite "
			"failed %d", error));
		return 1;
	}
	return 0;
}

static int
process_DATA(drdynvcPlugin * plugin, uint32 ChannelId,
	char * data, int data_size)
{
	return dvcman_receive_channel_data(plugin->channel_mgr,
		ChannelId, data, (uint32) data_size);
}

static int
process_DATA_FIRST_PDU(drdynvcPlugin * plugin, int Sp, int cbChId,
	char * data, int data_size)
{
	int pos;
	uint32 ChannelId;
	uint32 Length;
	int error;

	pos = 1;
	ChannelId = get_variable_uint(cbChId, data, &pos);
	Length = get_variable_uint(Sp, data, &pos);
	LLOGLN(10, ("process_DATA_FIRST_PDU: ChannelId=%d Length=%d", ChannelId, Length));

	error = dvcman_receive_channel_data_first(plugin->channel_mgr,
		ChannelId, Length);
	if (error)
		return error;

	return dvcman_receive_channel_data(plugin->channel_mgr,
		ChannelId, data + pos, (uint32) data_size - pos);
}

static int
process_DATA_PDU(drdynvcPlugin * plugin, int Sp, int cbChId,
	char * data, int data_size)
{
	int pos;
	uint32 ChannelId;

	pos = 1;
	ChannelId = get_variable_uint(cbChId, data, &pos);
	LLOGLN(10, ("process_DATA_PDU: ChannelId=%d", ChannelId));

	return process_DATA(plugin, ChannelId, data + pos, data_size - pos);
}

static int
process_CLOSE_REQUEST_PDU(drdynvcPlugin * plugin, int Sp, int cbChId,
	char * data, int data_size)
{
	int pos;
	uint32 ChannelId;

	pos = 1;
	ChannelId = get_variable_uint(cbChId, data, &pos);
	LLOGLN(10, ("process_CLOSE_REQUEST_PDU: ChannelId=%d", ChannelId));
	dvcman_close_channel(plugin->channel_mgr, ChannelId);

	return 0;
}

static int
thread_process_message(drdynvcPlugin * plugin, char * data, int data_size)
{
	int value;
	int Cmd;
	int Sp;
	int cbChId;
	int rv;

	rv = 0;
	value = GET_UINT8(data, 0);
	Cmd = (value & 0xf0) >> 4;
	Sp = (value & 0x0c) >> 2;
	cbChId = (value & 0x03) >> 0;
	LLOGLN(10, ("thread_process_message: data_size %d cmd 0x%x", data_size, Cmd));
	hexdump(data, data_size);
	switch (Cmd)
	{
		case CAPABILITY_REQUEST_PDU:
			rv = process_CAPABILITY_REQUEST_PDU(plugin, Sp, cbChId, data, data_size);
			break;
		case CREATE_REQUEST_PDU:
			rv = process_CREATE_REQUEST_PDU(plugin, Sp, cbChId, data, data_size);
			break;
		case DATA_FIRST_PDU:
			rv = process_DATA_FIRST_PDU(plugin, Sp, cbChId, data, data_size);
			break;
		case DATA_PDU:
			rv = process_DATA_PDU(plugin, Sp, cbChId, data, data_size);
			break;
		case CLOSE_REQUEST_PDU:
			rv = process_CLOSE_REQUEST_PDU(plugin, Sp, cbChId, data, data_size);
			break;
		default:
			LLOGLN(0, ("thread_process_message: unknown drdynvc cmd 0x%x", Cmd));
			break;
	}
	return rv;
}

/* process the linked list of data that has come in */
static int
thread_process_data_in(drdynvcPlugin * plugin)
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
	drdynvcPlugin * plugin;
	struct wait_obj * listobj[2];
	int numobj;
	int timeout;

	plugin = (drdynvcPlugin *) arg;

	plugin->thread_status = 1;
	LLOGLN(10, ("thread_func: in"));
	while (1)
	{
		listobj[0] = plugin->term_event;
		listobj[1] = plugin->data_in_event;
		numobj = 2;
		timeout = -1;
		wait_obj_select(listobj, numobj, NULL, 0, timeout);
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
	LLOGLN(10, ("thread_func: out"));
	plugin->thread_status = -1;
	return 0;
}

static void
OpenEventProcessReceived(uint32 openHandle, void * pData, uint32 dataLength,
	uint32 totalLength, uint32 dataFlags)
{
	drdynvcPlugin * plugin;

	plugin = (drdynvcPlugin *) chan_plugin_find_by_open_handle(openHandle);

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
		/* Add a padding to avoid invalid memory read in some plugin using some kind of optimization */
		plugin->data_in = (char *) malloc(totalLength + DRDYNVC_BUFFER_PADDING);
		plugin->data_in_size = totalLength;
		memset(plugin->data_in + totalLength, 0, DRDYNVC_BUFFER_PADDING);
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
	drdynvcPlugin * plugin;
	uint32 error;
	pthread_t thread;

	plugin = (drdynvcPlugin *) chan_plugin_find_by_init_handle(pInitHandle);
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

	dvcman_initialize(plugin->channel_mgr);

	pthread_create(&thread, 0, thread_func, plugin);
	pthread_detach(thread);
}

static void
InitEventProcessTerminated(void * pInitHandle)
{
	drdynvcPlugin * plugin;
	int index;
	struct data_in_item * in_item;

	plugin = (drdynvcPlugin *) chan_plugin_find_by_init_handle(pInitHandle);
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

	dvcman_free(plugin->channel_mgr);

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

int
VirtualChannelEntry(PCHANNEL_ENTRY_POINTS pEntryPoints)
{
	drdynvcPlugin * plugin;
	RD_PLUGIN_DATA * data;

	LLOGLN(10, ("VirtualChannelEntry:"));

	plugin = (drdynvcPlugin *) malloc(sizeof(drdynvcPlugin));
	memset(plugin, 0, sizeof(drdynvcPlugin));

	chan_plugin_init((rdpChanPlugin *) plugin);

	plugin->data_in_size = 0;
	plugin->data_in = 0;
	plugin->ep = *pEntryPoints;
	memset(&(plugin->channel_def), 0, sizeof(plugin->channel_def));
	plugin->channel_def.options = CHANNEL_OPTION_INITIALIZED |
		CHANNEL_OPTION_ENCRYPT_RDP | CHANNEL_OPTION_COMPRESS_RDP;
	strcpy(plugin->channel_def.name, "drdynvc");
	plugin->in_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(plugin->in_mutex, 0);
	plugin->in_list_head = 0;
	plugin->in_list_tail = 0;
	plugin->term_event = wait_obj_new("freerdprdrynvcterm");
	plugin->data_in_event = wait_obj_new("freerdpdrdynvcdatain");
	plugin->thread_status = 0;
	plugin->ep.pVirtualChannelInit(&plugin->chan_plugin.init_handle, &plugin->channel_def, 1,
		VIRTUAL_CHANNEL_VERSION_WIN2000, InitEvent);

	plugin->channel_mgr = dvcman_new(plugin);

	if (pEntryPoints->cbSize >= sizeof(CHANNEL_ENTRY_POINTS_EX))
	{
		plugin->ep_event_push = ((PCHANNEL_ENTRY_POINTS_EX)pEntryPoints)->pVirtualChannelEventPush;
		data = (RD_PLUGIN_DATA *) (((PCHANNEL_ENTRY_POINTS_EX)pEntryPoints)->pExtendedData);
		while (data && data->size > 0)
		{
			dvcman_load_plugin(plugin->channel_mgr, data);
			data = (RD_PLUGIN_DATA *) (((void *) data) + data->size);
		}
	}

	return 1;
}
