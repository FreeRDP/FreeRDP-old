/*
   Copyright (c) 2010 Jay Sorg

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
#include <sys/time.h>
#include "drdynvc_types.h"
#include "wait_obj.h"
#include "dvcman.h"

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

typedef struct drdynvc_plugin drdynvcPlugin;
struct drdynvc_plugin
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
	int version;
	int PriorityCharge0;
	int PriorityCharge1;
	int PriorityCharge2;
	int PriorityCharge3;

	IWTSVirtualChannelManager * channel_mgr;
};

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
	SET_UINT16(out_data, 0, 0x0050);
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

static int
process_CREATE_REQUEST_PDU(drdynvcPlugin * plugin, int Sp, int cbChId,
	char * data, int data_size)
{
	int pos;
	int error;
	int size;
	char * out_data;
	uint32 ChannelId;

	LLOGLN(10, ("process_CREATE_REQUEST_PDU:"));
	pos = 1;
	switch (cbChId)
	{
		case 0:
			ChannelId = (uint32) GET_UINT8(data, pos);
			pos += 1;
			break;
		case 1:
			ChannelId = (uint32) GET_UINT16(data, pos);
			pos += 2;
			break;
		default:
			ChannelId = (uint32) GET_UINT32(data, pos);
			pos += 4;
			break;
	}
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

	LLOGLN(10, ("VirtualChannelEntry:"));

	plugin = (drdynvcPlugin *) malloc(sizeof(drdynvcPlugin));
	memset(plugin, 0, sizeof(drdynvcPlugin));

	chan_plugin_init((rdpChanPlugin *) plugin);

	plugin->data_in_size = 0;
	plugin->data_in = 0;
	plugin->ep = *pEntryPoints;
	memset(&(plugin->channel_def), 0, sizeof(plugin->channel_def));
	plugin->channel_def.options = CHANNEL_OPTION_INITIALIZED |
		CHANNEL_OPTION_ENCRYPT_RDP;
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

	plugin->channel_mgr = dvcman_new();
	dvcman_load_plugin(plugin->channel_mgr, "channels/drdynvc/audin/.libs/audin.so");

	return 1;
}
