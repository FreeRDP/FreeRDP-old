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

#include "rdpdr_types.h"
#include "rdpdr_main.h"
#include "rdpdr_constants.h"
#include "rdpdr_capabilities.h"
#include "devman.h"
#include "irp.h"

/* called by main thread
   add item to linked list and inform worker thread that there is data */
static void
signal_data_in(rdpdrPlugin * plugin)
{
	struct data_in_item * item;

	item = (struct data_in_item *) malloc(sizeof(struct data_in_item));
	item->next = 0;
	item->data = plugin->data_in;
	plugin->data_in = 0;
	item->data_size = plugin->data_in_size;
	plugin->data_in_size = 0;
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

	LLOGLN(0, ("Version Minor: %d", plugin->versionMinor));

#if 0
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
#endif
}

static int
rdpdr_send_client_announce_reply(rdpdrPlugin * plugin)
{
	uint32 error;
	char* out_data = malloc(12);

	SET_UINT16(out_data, 0, RDPDR_CTYP_CORE);
	SET_UINT16(out_data, 2, PAKID_CORE_CLIENTID_CONFIRM);

	SET_UINT16(out_data, 4, 1); /* versionMajor, must be set to 1 */
	SET_UINT16(out_data, 6, plugin->versionMinor); /* versionMinor */
	SET_UINT32(out_data, 8, plugin->clientID); /* clientID, given by the server in a Server Announce Request */

	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle, out_data, 12, out_data);

	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite failed %d", error));
		return 1;
	}

	return 0;
}

static void
rdpdr_process_server_clientid_confirm(rdpdrPlugin * plugin, char* data, int data_size)
{
	uint16 versionMinor;
	uint32 clientID;

	/* versionMajor, must be 1 */
	versionMinor = GET_UINT16(data, 2); /* versionMinor */
	clientID = GET_UINT32(data, 4); /* clientID */

	if (clientID != plugin->clientID)
		plugin->clientID = clientID;

	if (versionMinor != plugin->versionMinor)
		plugin->versionMinor = versionMinor;
}

static int
rdpdr_send_client_name_request(rdpdrPlugin * plugin)
{
	char * data;
	int size;
	uint32 error;
	char computerName[256];
	uint32 computerNameLen;
	uint32 computerNameLenW;

	gethostname(computerName, sizeof(computerName) - 1);
	computerNameLen = strlen(computerName);
	size = 16 + computerNameLen * 2 + 2;
	data = malloc(size);
	memset(data, 0, size);

	SET_UINT16(data, 0, RDPDR_CTYP_CORE);
	SET_UINT16(data, 2, PAKID_CORE_CLIENT_NAME);

	SET_UINT32(data, 4, 1); // unicodeFlag, 0 for ASCII and 1 for Unicode
	SET_UINT32(data, 8, 0); // codePage, must be set to zero

	computerNameLenW = set_wstr(&data[16], size - 16, computerName, computerNameLen); /* computerName */
	SET_UINT32(data, 12, computerNameLenW + 2); /* computerNameLen, including null terminator */

	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
				data, 16 + computerNameLenW + 2, data);

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

	int size;
	uint32 error;
	DEVICE* pdev;
	int offset = 0;
	int device_data_len;
	int i;

	size = 8 + plugin->devman->count * 256;
	out_data = malloc(size);
	memset(out_data, 0, size);

	SET_UINT16(out_data, 0, RDPDR_CTYP_CORE);
	SET_UINT16(out_data, 2, PAKID_CORE_DEVICELIST_ANNOUNCE);
	SET_UINT32(out_data, 4, plugin->devman->count); /* deviceCount */
	offset = 8;

	LLOGLN(0, ("%d device(s) registered", plugin->devman->count));

	devman_rewind(plugin->devman);

	while (devman_has_next(plugin->devman) != 0)
	{
		pdev = devman_get_next(plugin->devman);

		SET_UINT32(out_data, offset, pdev->service->type); /* deviceType */
		SET_UINT32(out_data, offset + 4, pdev->id); /* deviceID */
		offset += 8;

		/* preferredDosName, Max 8 characters, may not be null terminated */
		strncpy(&out_data[offset], pdev->name, 8);
		for (i = 0; i < 8; i++)
		{
			if (out_data[offset + i] < 0)
			{
				out_data[offset + i] = '_';
			}
		}
		offset += 8;

		LLOGLN(0, ("registered device: %s (type=%d id=%d)", pdev->name, pdev->service->type, pdev->id));

		switch (pdev->service->type)
		{
			case RDPDR_DTYP_PRINT:

				break;

			case RDPDR_DTYP_FILESYSTEM:
				/* [MS-RDPEFS] 2.2.3.1 said this is a unicode string, however, only ASCII works.
				   Any non-ASCII characters simply screw up the whole channel. Long name is supported though.
				   This is yet to be investigated. */

				//device_data_len = set_wstr(&out_data[offset + 4], size - offset - 4, pdev->name, strlen(pdev->name));
				//SET_UINT32(out_data, offset, device_data_len + 2); // deviceDataLength
				//offset += 4 + device_data_len + 2;
				device_data_len = strlen(pdev->name);
				SET_UINT32(out_data, offset, device_data_len + 1);
				strncpy(&out_data[offset + 4], pdev->name, size - offset - 4);
				for (i = 0; i < device_data_len; i++)
				{
					if (out_data[offset + 4 + i] < 0)
					{
						out_data[offset + 4 + i] = '_';
					}
				}
				offset += 4 + device_data_len + 1;

				break;

			case RDPDR_DTYP_SMARTCARD:

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
	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
			out_data, out_data_size, out_data);

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
	char * out;
	int out_size;
	int error;

	memset((void*)&irp, '\0', sizeof(IRP));

	irp.ioStatus = RD_STATUS_SUCCESS;

	/* Device I/O Request Header */
	deviceID = GET_UINT32(data, 0); /* deviceID */
	irp.fileID = GET_UINT32(data, 4); /* fileID */
	irp.completionID = GET_UINT32(data, 8); /* completionID */
	irp.majorFunction = GET_UINT32(data, 12); /* majorFunction */
	irp.minorFunction = GET_UINT32(data, 16); /* minorFunction */

	irp.dev = devman_get_device_by_id(plugin->devman, deviceID);

	LLOGLN(0, ("IRP MAJOR: %d MINOR: %d", irp.majorFunction, irp.minorFunction));

	switch(irp.majorFunction)
	{
		case IRP_MJ_CREATE:
			LLOGLN(0, ("IRP_MJ_CREATE"));
			irp_process_create_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_CLOSE:
			LLOGLN(0, ("IRP_MJ_CLOSE"));
			irp_process_close_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_READ:
			LLOGLN(0, ("IRP_MJ_READ"));
			irp_process_read_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_WRITE:
			LLOGLN(0, ("IRP_MJ_WRITE"));
			irp_process_write_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_QUERY_INFORMATION:
			LLOGLN(0, ("IRP_MJ_QUERY_INFORMATION"));
			irp_process_query_information_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_SET_INFORMATION:
			LLOGLN(0, ("IRP_MJ_SET_INFORMATION"));
			irp_process_set_volume_information_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_QUERY_VOLUME_INFORMATION:
			LLOGLN(0, ("IRP_MJ_QUERY_VOLUME_INFORMATION"));
			irp_process_query_volume_information_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_DIRECTORY_CONTROL:
			LLOGLN(0, ("IRP_MJ_DIRECTORY_CONTROL"));
			irp_process_directory_control_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_DEVICE_CONTROL:
			LLOGLN(0, ("IRP_MJ_DEVICE_CONTROL"));
			irp_process_device_control_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_LOCK_CONTROL:
			LLOGLN(0, ("IRP_MJ_LOCK_CONTROL"));
			irp_process_file_lock_control_request(&irp, &data[20], data_size - 20);
			break;

		default:
			LLOGLN(0, ("IRP majorFunction=0x%x minorFunction=0x%x not supported", irp.majorFunction, irp.minorFunction));
			irp.ioStatus = RD_STATUS_NOT_SUPPORTED;
			break;
	}

	if (irp.ioStatus != RD_STATUS_PENDING)
	{
		out_size = 16 + irp.outputBufferLength;
		out = malloc(out_size);
		irp_output_device_io_completion_header(&irp, out, out_size);
		if (irp.outputBufferLength > 0)
		{
			memcpy(out + 16, irp.outputBuffer, irp.outputBufferLength);
		}
		error = plugin->ep.pVirtualChannelWrite(plugin->open_handle, out, out_size, out);
		if (error != CHANNEL_RC_OK)
		{
			LLOGLN(0, ("rdpdr_process_irp: "
				"VirtualChannelWrite failed %d", error));
		}
	}
	if (irp.outputBuffer)
		free(irp.outputBuffer);
}

static int
rdpdr_send_capabilities(rdpdrPlugin * plugin)
{
	int size;
	int offset;
	char* data;
	uint32 error;

	size = 256;
	data = (char*)malloc(size);
	memset(data, 0, size);

	SET_UINT16(data, 0, RDPDR_CTYP_CORE);
	SET_UINT16(data, 2, PAKID_CORE_CLIENT_CAPABILITY);

	SET_UINT16(data, 4, 5); /* numCapabilities */
	SET_UINT16(data, 6, 0); /* pad */

	offset = 8;

	offset += rdpdr_out_general_capset(&data[offset], size - offset);
	offset += rdpdr_out_printer_capset(&data[offset], size - offset);
	offset += rdpdr_out_port_capset(&data[offset], size - offset);
	offset += rdpdr_out_drive_capset(&data[offset], size - offset);
	offset += rdpdr_out_smartcard_capset(&data[offset], size - offset);

	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
			data, offset, data);

	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite failed %d", error));

		return -1;
	}

	return 0;
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

	if (component == RDPDR_CTYP_CORE)
	{
		LLOGLN(10, ("RDPDR_CTYP_CORE"));
		switch (packetID)
		{
			case PAKID_CORE_SERVER_ANNOUNCE:
				LLOGLN(0, ("PAKID_CORE_SERVER_ANNOUNCE"));
				rdpdr_process_server_announce_request(plugin, &data[4], data_size - 4);
				rdpdr_send_client_announce_reply(plugin);
				rdpdr_send_client_name_request(plugin);
				break;

			case PAKID_CORE_SERVER_CAPABILITY:
				/* server capabilities */
				LLOGLN(0, ("PAKID_CORE_SERVER_CAPABILITY"));
				rdpdr_process_capabilities(&data[4], data_size - 4);
				rdpdr_send_capabilities(plugin);
				break;

			case PAKID_CORE_CLIENTID_CONFIRM:
				LLOGLN(0, ("PAKID_CORE_CLIENTID_CONFIRM"));
				rdpdr_process_server_clientid_confirm(plugin, &data[4], data_size - 4);
				break;

			case PAKID_CORE_USER_LOGGEDON:
				LLOGLN(0, ("PAKID_CORE_USER_LOGGEDON"));
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

			default:
				LLOGLN(0, ("unknown packetID: 0x%02X", packetID));
				break;

		}
	}
	else if (component == RDPDR_CTYP_PRN)
	{
		LLOGLN(0, ("RDPDR_CTYP_PRN"));

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
		if (wait_obj_is_set(plugin->term_event))
		{
			break;
		}

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

	plugin = (rdpdrPlugin *) chan_plugin_find_by_open_handle(openHandle);

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
	rdpdrPlugin * plugin;
	uint32 error;
	pthread_t thread;

	plugin = (rdpdrPlugin *) chan_plugin_find_by_init_handle(pInitHandle);
	if (plugin == NULL)
	{
		LLOGLN(0, ("InitEventProcessConnected: error no match"));
	}

	error = plugin->ep.pVirtualChannelOpen(pInitHandle, &(plugin->open_handle),
		plugin->channel_def.name, OpenEvent);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("InitEventProcessConnected: Open failed"));
	}
	chan_plugin_register_open_handle((rdpChanPlugin *) plugin, plugin->open_handle);

	pthread_create(&thread, 0, thread_func, plugin);
	pthread_detach(thread);
}

static void
InitEventProcessTerminated(void * pInitHandle)
{
	rdpdrPlugin * plugin;
	int index;
	struct data_in_item * in_item;

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

	/* free the un-processed in/out queue */
	while (plugin->list_head != 0)
	{
		in_item = plugin->list_head;
		plugin->list_head = in_item->next;
		free(in_item->data);
		free(in_item);
	}

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
	void * data;

	LLOGLN(10, ("VirtualChannelEntry:"));

	plugin = (rdpdrPlugin *) malloc(sizeof(rdpdrPlugin));
	memset(plugin, 0, sizeof(rdpdrPlugin));

	chan_plugin_init((rdpChanPlugin *) plugin);

	plugin->data_in_size = 0;
	plugin->data_in = 0;
	plugin->ep = *pEntryPoints;

	memset(&(plugin->channel_def), 0, sizeof(plugin->channel_def));
	plugin->channel_def.options = CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP;
	strcpy(plugin->channel_def.name, "rdpdr");

	plugin->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(plugin->mutex, 0);
	plugin->list_head = 0;
	plugin->list_tail = 0;

	plugin->term_event = wait_obj_new("freerdprdpdrterm");
	plugin->data_in_event = wait_obj_new("freerdprdpdrdatain");

	plugin->thread_status = 0;

	if (pEntryPoints->cbSize >= sizeof(CHANNEL_ENTRY_POINTS_EX))
	{
		data = (((PCHANNEL_ENTRY_POINTS_EX)pEntryPoints)->pExtendedData);
	}
	else
	{
		data = NULL;
	}
	plugin->devman = devman_new(data);
	devman_load_device_service(plugin->devman, "../channels/rdpdr/disk/.libs/disk.so");

	plugin->ep.pVirtualChannelInit(&plugin->chan_plugin.init_handle, &plugin->channel_def, 1,
		VIRTUAL_CHANNEL_VERSION_WIN2000, InitEvent);

	return 1;
}

