/*
   Copyright (c) 2009-2010 Jay Sorg

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

#include "rdpdr.h"
#include "irp.h"
#include "devman.h"
#include "types.h"
#include "types_ui.h"
#include "vchan.h"
#include "chan_stream.h"
#include "chan_plugin.h"
#include "wait_obj.h"
#include "constants_rdpdr.h"

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

typedef struct rdpdr_plugin rdpdrPlugin;
struct rdpdr_plugin
{
	rdpChanPlugin chan_plugin;

	CHANNEL_ENTRY_POINTS ep;
	CHANNEL_DEF channel_def[2];
	uint32 open_handle[2];
	char * data_in[2];
	int data_in_size[2];
	int data_in_read[2];
	struct wait_obj * term_event;
	struct wait_obj * data_in_event;
	struct data_in_item * volatile list_head;
	struct data_in_item * volatile list_tail;
	/* for locking the linked list */
	pthread_mutex_t * mutex;
	volatile int thread_status;

	uint16 versionMinor;
	uint16 clientID;
	DEVMAN* devman;
};

/* called by main thread
   add item to linked list and inform worker thread that there is data */
static void
signal_data_in(rdpdrPlugin * plugin)
{
	struct data_in_item * item;

	item = (struct data_in_item *) malloc(sizeof(struct data_in_item));
	item->next = 0;
	item->data = plugin->data_in[0];
	plugin->data_in[0] = 0;
	item->data_size = plugin->data_in_size[0];
	plugin->data_in_size[0] = 0;
	pthread_mutex_lock(plugin->mutex);
	if (plugin->list_tail == 0)
	{
		plugin->list_head = item;
		plugin->list_tail = item;
	}
	else
	{
		plugin->list_tail->next = item;
		plugin->list_tail = item;
	}
	pthread_mutex_unlock(plugin->mutex);
	wait_obj_set(plugin->data_in_event);
}

static void
rdpdr_process_server_announce_request(rdpdrPlugin * plugin, char* data, int data_size)
{
	/* versionMajor, must be 1 */
	plugin->versionMinor = GET_UINT16(data, 2); /* versionMinor */
	plugin->clientID = GET_UINT32(data, 4); /* clientID */

	LLOGLN(0, ("Version Minor: %d\n", plugin->versionMinor));

	switch(plugin->versionMinor)
	{
		case 0x000C:
			LLOGLN(0, ("Windows Vista, Windows Vista SP1, Windows Server 2008, Windows 7, and Windows Server 2008 R2"));
			break;

		case 0x000A:
			LLOGLN(0, ("Windows Server 2003 SP2"));
			break;

		case 0x0006:
			LLOGLN(0, ("Windows XP SP3"));
			break;

		case 0x0005:
			LLOGLN(0, ("Windows XP, Windows XP SP1, Windows XP SP2, Windows Server 2003, and Windows Server 2003 SP1"));
			break;

		case 0x0002:
			LLOGLN(0, ("Windows 2000"));
			break;
	}
}

static int
rdpdr_send_client_announce_reply(rdpdrPlugin * plugin)
{
	uint32 error;
	char* out_data = malloc(12);

	SET_UINT16(out_data, 0, RDPDR_COMPONENT_TYPE_CORE);
	SET_UINT16(out_data, 2, PAKID_CORE_CLIENTID_CONFIRM);

	SET_UINT16(out_data, 4, 1); /* versionMajor, must be set to 1 */
	SET_UINT16(out_data, 6, plugin->versionMinor); /* versionMinor */
	SET_UINT32(out_data, 8, plugin->clientID); /* clientID, given by the server in a Server Announce Request */

	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle[0], out_data, 12, out_data);

	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite failed %d", error));
		return 1;
	}

	return 0;
}

static int
rdpdr_send_client_name_request(rdpdrPlugin * plugin)
{
	char* out_data;
	int out_data_size;
	uint32 error;
	uint32 computerNameLen;

	computerNameLen = 1;
	out_data_size = 16 + computerNameLen * 2;
	out_data = malloc(out_data_size);

	SET_UINT16(out_data, 0, RDPDR_COMPONENT_TYPE_CORE);
	SET_UINT16(out_data, 2, PAKID_CORE_CLIENT_NAME);

	SET_UINT32(out_data, 4, 1); // unicodeFlag, 0 for ASCII and 1 for Unicode
	SET_UINT32(out_data, 8, 0); // codePage, must be set to zero

	/* this part is a hardcoded test, while waiting for a unicode string output function */
	/* we also need to figure out a way of passing settings from the freerdp core */

	SET_UINT32(out_data, 12, computerNameLen); /* computerNameLen */
	SET_UINT16(out_data, 16, 0x0041); /* computerName */

	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle[0], out_data, out_data_size, out_data);

	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite failed %d", error));
		return 1;
	}

	return 0;
}

static int
rdpdr_send_device_list_announce_request(rdpdrPlugin * plugin)
{
	char* out_data;
	int out_data_size;

	uint32 error;
	int offset = 0;
	DEVICE* pdev;	

	out_data = malloc(64);

	SET_UINT16(out_data, 0, RDPDR_COMPONENT_TYPE_CORE);
	SET_UINT16(out_data, 2, PAKID_CORE_DEVICELIST_ANNOUNCE);
	SET_UINT16(out_data, 4, plugin->devman->count); // deviceCount
	offset += 6;

	devman_rewind(plugin->devman);

	while (devman_has_next(plugin->devman) != 0)
	{
		pdev = devman_get_next(plugin->devman);

		SET_UINT16(out_data, offset, pdev->service->type); /* deviceType */
		SET_UINT16(out_data, offset, pdev->id); /* deviceID */
		//out_uint8p(s, plugin->device[i].name, 8); // preferredDosName, Max 8 characters, may not be null terminated
		offset += 12;

		switch (pdev->service->type)
		{
			case DEVICE_TYPE_PRINTER:

				break;

			case DEVICE_TYPE_DISK:

				break;

			case DEVICE_TYPE_SMARTCARD:

				/*
				 * According to [MS-RDPEFS] the deviceDataLength field for
				 * the smart card device type must be set to zero
				 */
				
				SET_UINT32(out_data, offset, 0); // deviceDataLength
				offset += 4;
				break;

			default:
				SET_UINT32(out_data, offset, 0);
				offset += 4;
		}		
	}

	out_data_size = offset;
	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle[0], out_data, out_data_size, out_data);

	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite failed %d", error));
		return 1;
	}

	return 0;
}

static void
rdpdr_process_irp(rdpdrPlugin * plugin, char* data, int data_size)
{
	IRP irp;
	int deviceID;

	memset((void*)&irp, '\0', sizeof(IRP));

	irp.ioStatus = RD_STATUS_SUCCESS;

	/* Device I/O Request Header */
	deviceID = GET_UINT32(data, 0); /* deviceID */
	irp.fileID = GET_UINT32(data, 4); /* fileID */
	irp.completionID = GET_UINT32(data, 8); /* completionID */
	irp.majorFunction = GET_UINT32(data, 12); /* majorFunction */
	irp.minorFunction = GET_UINT32(data, 16); /* minorFunction */

	irp.dev = devman_get_device_by_id(plugin->devman, deviceID);

	LLOGLN(0, ("IRP MAJOR: %d MINOR: %d\n", irp.majorFunction, irp.minorFunction));

	switch(irp.majorFunction)
	{
		case IRP_MJ_CREATE:
			LLOGLN(0, ("IRP_MJ_CREATE\n"));
			irp_process_create_request(&data[20], data_size - 20, &irp);
			irp_send_create_response(&irp);
			break;

		case IRP_MJ_CLOSE:
			LLOGLN(0, ("IRP_MJ_CLOSE\n"));
			irp_process_close_request(&data[20], data_size - 20, &irp);
			irp_send_close_response(&irp);
			break;

		case IRP_MJ_READ:
			LLOGLN(0, ("IRP_MJ_READ\n"));
			irp_process_read_request(&data[20], data_size - 20, &irp);
			break;

		case IRP_MJ_WRITE:
			LLOGLN(0, ("IRP_MJ_WRITE\n"));
			irp_process_write_request(&data[20], data_size - 20, &irp);
			break;

		case IRP_MJ_QUERY_INFORMATION:
			LLOGLN(0, ("IRP_MJ_QUERY_INFORMATION\n"));
			irp_process_query_information_request(&data[20], data_size - 20, &irp);
			irp_send_query_information_response(&irp);
			break;

		case IRP_MJ_SET_INFORMATION:
			LLOGLN(0, ("IRP_MJ_SET_INFORMATION\n"));
			irp_process_set_volume_information_request(&data[20], data_size - 20, &irp);
			break;

		case IRP_MJ_QUERY_VOLUME_INFORMATION:
			LLOGLN(0, ("IRP_MJ_QUERY_VOLUME_INFORMATION\n"));
			irp_process_query_volume_information_request(&data[20], data_size - 20, &irp);
			break;

		case IRP_MJ_DIRECTORY_CONTROL:
			LLOGLN(0, ("IRP_MJ_DIRECTORY_CONTROL\n"));
			irp_process_directory_control_request(&data[20], data_size - 20, &irp);
			break;

		case IRP_MJ_DEVICE_CONTROL:
			LLOGLN(0, ("IRP_MJ_DEVICE_CONTROL\n"));
			irp_process_device_control_request(&data[20], data_size - 20, &irp);
			break;

		case IRP_MJ_LOCK_CONTROL:
			LLOGLN(0, ("IRP_MJ_LOCK_CONTROL\n"));
			irp_process_file_lock_control_request(&data[20], data_size - 20, &irp);
			break;

		default:
			//ui_unimpl(NULL, "IRP majorFunction=0x%x minorFunction=0x%x\n", irp.majorFunction, irp.minorFunction);
			return;
	}

	if (irp.buffer)
		free(irp.buffer);
}

static int
thread_process_message(rdpdrPlugin * plugin, char * data, int data_size)
{
	uint16 component;
	uint16 packetID;
	uint32 deviceID;
	uint32 status;

	component = GET_UINT16(data, 0);
	packetID = GET_UINT16(data, 2);

	if (component == RDPDR_COMPONENT_TYPE_CORE)
	{
		LLOGLN(0, ("RDPDR_COMPONENT_TYPE_CORE"));
		switch (packetID)
		{
			case PAKID_CORE_SERVER_ANNOUNCE:
				LLOGLN(0, ("PAKID_CORE_SERVER_ANNOUNCE"));
				rdpdr_process_server_announce_request(plugin, &data[4], data_size - 4);
				rdpdr_send_client_announce_reply(plugin);
				rdpdr_send_client_name_request(plugin);
				break;

			case PAKID_CORE_CLIENTID_CONFIRM:
				LLOGLN(0, ("PAKID_CORE_CLIENTID_CONFIRM"));
				rdpdr_send_device_list_announce_request(plugin);
				break;

			case PAKID_CORE_DEVICE_REPLY:
				/* connect to a specific resource */
				LLOGLN(0, ("PAKID_CORE_DEVICE_REPLY"));
				deviceID = GET_UINT32(data, 4);
				status = GET_UINT32(data, 8);
				break;

			case PAKID_CORE_DEVICE_IOREQUEST:
				LLOGLN(0, ("PAKID_CORE_DEVICE_IOREQUEST"));
				rdpdr_process_irp(plugin, &data[4], data_size - 4);
				break;

			case PAKID_CORE_SERVER_CAPABILITY:
				/* server capabilities */
				LLOGLN(0, ("PAKID_CORE_SERVER_CAPABILITY"));
				//rdpdr_process_capabilities(plugin, s);
				//rdpdr_send_capabilities(plugin);
				break;

			default:
				//ui_unimpl(NULL, "RDPDR core component, packetID: 0x%02X\n", packetID);
				break;

		}
	}
	else if (component == RDPDR_COMPONENT_TYPE_PRINTING)
	{
		LLOGLN(0, ("RDPDR_COMPONENT_TYPE_PRINTING"));

		switch (packetID)
		{
			case PAKID_PRN_CACHE_DATA:
				LLOGLN(0, ("PAKID_PRN_CACHE_DATA"));
				//printercache_process(s);
				break;

			default:
				//ui_unimpl(NULL, "RDPDR printer component, packetID: 0x%02X\n", packetID);
				break;
		}
	}
	//else
		//ui_unimpl(NULL, "RDPDR component: 0x%02X packetID: 0x%02X\n", component, packetID);

	return 0;
}

/* process the linked list of data that has come in */
static int
thread_process_data(rdpdrPlugin * plugin)
{
	char * data;
	int data_size;
	struct data_in_item * item;

	while (1)
	{
		pthread_mutex_lock(plugin->mutex);

		if (plugin->list_head == 0)
		{
			pthread_mutex_unlock(plugin->mutex);
			break;
		}

		data = plugin->list_head->data;
		data_size = plugin->list_head->data_size;
		item = plugin->list_head;
		plugin->list_head = plugin->list_head->next;

		if (plugin->list_head == 0)
		{
			plugin->list_tail = 0;
		}

		pthread_mutex_unlock(plugin->mutex);
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
	rdpdrPlugin * plugin;
	struct wait_obj * listobj[2];
	int numobj;

	plugin = (rdpdrPlugin *) arg;

	plugin->thread_status = 1;
	LLOGLN(10, ("thread_func: in"));

	while (1)
	{
		listobj[0] = plugin->term_event;
		listobj[1] = plugin->data_in_event;
		numobj = 2;
		wait_obj_select(listobj, numobj, NULL, 0, -1);

		if (wait_obj_is_set(plugin->term_event))
		{
			break;
		}
		if (wait_obj_is_set(plugin->data_in_event))
		{
			wait_obj_clear(plugin->data_in_event);
			/* process data in */
			thread_process_data(plugin);
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
	rdpdrPlugin * plugin;
	int index;

	plugin = (rdpdrPlugin *) chan_plugin_find_by_open_handle(openHandle);
	index = (openHandle == plugin->open_handle[0]) ? 0 : 1;

	LLOGLN(10, ("OpenEventProcessReceived: receive openHandle %d dataLength %d "
		"totalLength %d dataFlags %d",
		openHandle, dataLength, totalLength, dataFlags));

	if (dataFlags & CHANNEL_FLAG_FIRST)
	{
		plugin->data_in_read[index] = 0;
		if (plugin->data_in[index] != 0)
		{
			free(plugin->data_in[index]);
		}
		plugin->data_in[index] = (char *) malloc(totalLength);
		plugin->data_in_size[index] = totalLength;
	}

	memcpy(plugin->data_in[index] + plugin->data_in_read[index], pData, dataLength);
	plugin->data_in_read[index] += dataLength;

	if (dataFlags & CHANNEL_FLAG_LAST)
	{
		if (plugin->data_in_read[index] != plugin->data_in_size[index])
		{
			LLOGLN(0, ("OpenEventProcessReceived: read error"));
		}
		if (index == 0)
		{
			signal_data_in(plugin);
		}
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
	rdpdrPlugin * plugin;
	uint32 error;
	pthread_t thread;

	plugin = (rdpdrPlugin *) chan_plugin_find_by_init_handle(pInitHandle);
	if (plugin == NULL)
	{
		LLOGLN(0, ("InitEventProcessConnected: error no match"));
	}

	error = plugin->ep.pVirtualChannelOpen(pInitHandle, &(plugin->open_handle[0]),
		plugin->channel_def[0].name, OpenEvent);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("InitEventProcessConnected: Open failed"));
	}
	chan_plugin_register_open_handle((rdpChanPlugin *) plugin, plugin->open_handle[0]);

	error = plugin->ep.pVirtualChannelOpen(pInitHandle, &(plugin->open_handle[1]),
		plugin->channel_def[1].name, OpenEvent);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("InitEventProcessConnected: Open failed"));
	}
	chan_plugin_register_open_handle((rdpChanPlugin *) plugin, plugin->open_handle[1]);

	pthread_create(&thread, 0, thread_func, plugin);
	pthread_detach(thread);
}

static void
InitEventProcessTerminated(void * pInitHandle)
{
	rdpdrPlugin * plugin;
	int index;

	plugin = (rdpdrPlugin *) chan_plugin_find_by_init_handle(pInitHandle);
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

	devman_free(plugin->devman);
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
	rdpdrPlugin * plugin;

	LLOGLN(10, ("VirtualChannelEntry:"));

	plugin = (rdpdrPlugin *) malloc(sizeof(rdpdrPlugin));
	memset(plugin, 0, sizeof(rdpdrPlugin));

	chan_plugin_init((rdpChanPlugin *) plugin);

	plugin->data_in_size[0] = 0;
	plugin->data_in_size[1] = 0;
	plugin->data_in[0] = 0;
	plugin->data_in[1] = 0;
	plugin->ep = *pEntryPoints;

	memset(&(plugin->channel_def[0]), 0, sizeof(plugin->channel_def));
	plugin->channel_def[0].options = CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP;
	strcpy(plugin->channel_def[0].name, "rdpdr");

	plugin->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(plugin->mutex, 0);
	plugin->list_head = 0;
	plugin->list_tail = 0;

	plugin->term_event = wait_obj_new("freerdprdpdrterm");
	plugin->data_in_event = wait_obj_new("freerdprdpdrdatain");

	plugin->thread_status = 0;
	plugin->devman = devman_new();

	/*devman_load_device_service(plugin->devman, "channels/rdpdr/disk/.libs/libdisk.so");*/

	plugin->ep.pVirtualChannelInit(&plugin->chan_plugin.init_handle, plugin->channel_def, 2,
		VIRTUAL_CHANNEL_VERSION_WIN2000, InitEvent);

	return 1;
}

