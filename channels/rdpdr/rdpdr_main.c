/*
   FreeRDP: A Remote Desktop Protocol client.

   Copyright (C) Jay Sorg 2009-2011

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
#include <sys/select.h>

#include "rdpdr_types.h"
#include "rdpdr_scard.h"
#include "rdpdr_constants.h"
#include "rdpdr_capabilities.h"
#include "devman.h"
#include "irp.h"
#include "irp_queue.h"
#include "config.h"
#include <freerdp/utils/stream.h>
#include <freerdp/utils/memory.h>
#include <freerdp/utils/chan_plugin.h>
#include <freerdp/utils/unicode.h>
#include <freerdp/utils/wait_obj.h>

#include "rdpdr_main.h"

#define MAX(x,y)             (((x) > (y)) ? (x) : (y))

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
	size_t computerNameLenW;
	UNICONV * uniconv;
	char * s;

	uniconv = freerdp_uniconv_new();
	if (!plugin->computerName[0])
		gethostname(plugin->computerName, sizeof(plugin->computerName) - 1);
	s = freerdp_uniconv_out(uniconv, plugin->computerName, &computerNameLenW);
	size = 16 + computerNameLenW + 2;
	data = malloc(size);
	memset(data, 0, size);

	SET_UINT16(data, 0, RDPDR_CTYP_CORE);
	SET_UINT16(data, 2, PAKID_CORE_CLIENT_NAME);

	SET_UINT32(data, 4, 1); // unicodeFlag, 0 for ASCII and 1 for Unicode
	SET_UINT32(data, 8, 0); // codePage, must be set to zero
	SET_UINT32(data, 12, computerNameLenW + 2); /* computerNameLen, including null terminator */
	memcpy(data + 16, s, computerNameLenW);
	xfree(s);
	freerdp_uniconv_free(uniconv);

	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
				data, size, data);

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
	int size;
	uint32 error;
	DEVICE* pdev;
	int offset = 0;
	int i;

	size = 8;
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

		/* [MS-RDPEFS] 2.2.1.3 Device Announce Header
		For a smart card device, the DeviceDataLength field MUST be set to zero. */
		if (pdev->service->type == RDPDR_DTYP_SMARTCARD)
			pdev->data_len = 0;

		size += 20 + pdev->data_len;
		out_data = realloc(out_data, size);

		SET_UINT32(out_data, offset, pdev->service->type); /* deviceType */
		SET_UINT32(out_data, offset + 4, pdev->id); /* deviceID */
		offset += 8;

		/* We've got to use "SCARD" as the redirection name. It is not documented though */
		if (pdev->service->type == RDPDR_DTYP_SMARTCARD)
			strncpy(&out_data[offset], "SCARD", 8);
		else /* preferredDosName, Max 8 characters, may not be null terminated */
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

		SET_UINT32(out_data, offset, pdev->data_len);
		offset += 4;
		if (pdev->data_len > 0)
		{
			memcpy(&out_data[offset], pdev->data, pdev->data_len);
			offset += pdev->data_len;
		}
	}

	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
			out_data, offset, out_data);

	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite failed %d", error));
		return 1;
	}

	return 0;
}

static void
rdpdr_add_async_irp(rdpdrPlugin * plugin, IRP * irp, char * data, int data_size)
{
	fd_set *fds = NULL;
	uint32 timeout = 0, itv_timeout = 0;

	irp->length = GET_UINT32(data, 0); /* length */
	irp->offset = GET_UINT64(data, 4); /* offset */
	irp->inputBuffer = NULL;

	if (irp->majorFunction == IRP_MJ_WRITE)
	{
		fds = &plugin->writefds;
		irp->inputBuffer = malloc(data_size - 32);
		memcpy(irp->inputBuffer, data + 32, data_size - 32);
		irp->inputBufferLength = irp->length;
	}
	else
		fds = &plugin->readfds;

	if (irp->dev->service->type == RDPDR_DTYP_SERIAL)
		irp_get_timeouts(irp, &timeout, &itv_timeout);

	/* Check if io request timeout is smaller than current (but not 0). */
	if (timeout && (plugin->select_timeout == 0 || timeout < plugin->select_timeout))
	{
		plugin->select_timeout = timeout;
		plugin->tv.tv_sec = plugin->select_timeout / 1000;
		plugin->tv.tv_usec = (plugin->select_timeout % 1000) * 1000;
	}
	if (itv_timeout && (plugin->select_timeout == 0 || itv_timeout < plugin->select_timeout))
	{
		plugin->select_timeout = itv_timeout;
		plugin->tv.tv_sec = plugin->select_timeout / 1000;
		plugin->tv.tv_usec = (plugin->select_timeout % 1000) * 1000;
	}

	LLOGLN(10, ("RDPDR adding async irp fd %d", irp_file_descriptor(irp)));
	irp->ioStatus = RD_STATUS_PENDING;
	irp_queue_push(plugin->queue, irp);
	wait_obj_set(plugin->plugin_in_event);

	if (irp_file_descriptor(irp) >= 0)
	{
		FD_SET(irp_file_descriptor(irp), fds);
		plugin->nfds = MAX(plugin->nfds, irp_file_descriptor(irp));
	}
}

static void
rdpdr_abort_single_io(rdpdrPlugin * plugin, uint32 fd, uint8 abortType, uint32 ioStatus)
{
	IRP * pending = NULL;
	char * out;
	int out_size, error;
	int major = 0;

	switch (abortType)
	{
		case RDPDR_ABORT_IO_NONE:
			major = 0;
			break;

		case RDPDR_ABORT_IO_READ:
			major = IRP_MJ_READ;
			break;

		case RDPDR_ABORT_IO_WRITE:
			major = IRP_MJ_WRITE;
			break;

		default:
			LLOGLN(10, ("rdpdr_abort_single_io: called with an unexpected aborting type '%d'", abortType));
			return;
	}

	for (pending = irp_queue_first(plugin->queue); pending; pending = irp_queue_next(plugin->queue, pending))
	{
		if (irp_file_descriptor(pending) != fd || pending->majorFunction != major)
			continue;

		/* Process the specific fd and majorFunction */
		pending->ioStatus = ioStatus;
		out = irp_output_device_io_completion(pending, &out_size);
		error = plugin->ep.pVirtualChannelWrite(plugin->open_handle, out, out_size, out);
		if (error != CHANNEL_RC_OK)
			LLOGLN(0, ("rdpdr_check_fds: VirtualChannelWrite failed %d", error));

		if (pending->outputBuffer)
			free(pending->outputBuffer);
		irp_queue_remove(plugin->queue, pending);

		break;
	}
}

static int
rdpdr_check_fds(rdpdrPlugin * plugin)
{
	IRP * pending = NULL, * prev = NULL;
	char * out;
	int out_size, error;
	uint32 result;

	if (select(plugin->nfds + 1, &plugin->readfds, &plugin->writefds, NULL, &plugin->tv) <= 0)
		return 0;

	memset(&plugin->tv, 0, sizeof(struct timeval));

	/* scan every pending */
	pending = irp_queue_first(plugin->queue);
	while (pending)
	{
		int isset = 0;
		prev = pending;
		switch (pending->majorFunction)
		{
			case IRP_MJ_READ:
				if (FD_ISSET(irp_file_descriptor(pending), &plugin->readfds))
				{
					irp_process_read_request(pending, NULL, 0);
					isset = 1;
				}
				break;

			case IRP_MJ_WRITE:
				if (FD_ISSET(irp_file_descriptor(pending), &plugin->writefds))
				{
					irp_process_write_request(pending, NULL, 0);
					isset = 1;
				}
				break;

			case IRP_MJ_DEVICE_CONTROL:
				if (irp_get_event(pending, &result))
				{
					pending->ioStatus = RD_STATUS_SUCCESS;
					isset = 1;
				}
				break;

			default:
				LLOGLN(1, ("rdpdr_check_fds: no request found"));
				break;
		}

		if (isset)
		{
			out = irp_output_device_io_completion(pending, &out_size);
			error = plugin->ep.pVirtualChannelWrite(plugin->open_handle, out, out_size, out);
			if (error != CHANNEL_RC_OK)
				LLOGLN(0, ("rdpdr_check_fds: VirtualChannelWrite failed %d", error));

			if (pending->inputBuffer)
				free(pending->inputBuffer);
		}
		pending = irp_queue_next(plugin->queue, pending);
		if (isset)
			irp_queue_remove(plugin->queue, prev);
	}

	return 1;
}

static void
rdpdr_process_irp(rdpdrPlugin * plugin, char* data, int data_size)
{
	IRP irp;
	int deviceID;
	char * out;
	int out_size;
	int error;
	uint32 result;

	memset((void*)&irp, '\0', sizeof(IRP));

	irp.ioStatus = RD_STATUS_SUCCESS;
	irp.abortIO = RDPDR_ABORT_IO_NONE;

	/* Device I/O Request Header */
	deviceID = GET_UINT32(data, 0); /* deviceID */
	irp.fileID = GET_UINT32(data, 4); /* fileID */
	irp.completionID = GET_UINT32(data, 8); /* completionID */
	irp.majorFunction = GET_UINT32(data, 12); /* majorFunction */
	irp.minorFunction = GET_UINT32(data, 16); /* minorFunction */

	irp.dev = devman_get_device_by_id(plugin->devman, deviceID);
	switch (irp.dev->service->type)
	{
		case RDPDR_DTYP_SERIAL:
		case RDPDR_DTYP_SMARTCARD:
			irp.rwBlocking = 0;
			break;

		/* parallel, filesystem and smart card are supposed to be non-blocking
			but my goal so far is to deal only with the serial stuff */
		case RDPDR_DTYP_PARALLEL:
			irp.rwBlocking = 1;
			break;

		case RDPDR_DTYP_FILESYSTEM:
			irp.rwBlocking = 1;
			break;

		case RDPDR_DTYP_PRINT:
			irp.rwBlocking = 1;
			break;

		default:
			irp.rwBlocking = 1;
			break;
	}

	LLOGLN(10, ("IRP MAJOR: %d MINOR: %d", irp.majorFunction, irp.minorFunction));

	switch(irp.majorFunction)
	{
		case IRP_MJ_CREATE:
			LLOGLN(10, ("IRP_MJ_CREATE"));
			irp_process_create_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_CLOSE:
			LLOGLN(10, ("IRP_MJ_CLOSE"));
			irp_process_close_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_READ:
			LLOGLN(10, ("IRP_MJ_READ"));
			if (irp.rwBlocking)
				irp_process_read_request(&irp, &data[20], data_size - 20);
			else
				rdpdr_add_async_irp(plugin, &irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_WRITE:
			LLOGLN(10, ("IRP_MJ_WRITE"));
			if (irp.rwBlocking)
				irp_process_write_request(&irp, &data[20], data_size - 20);
			else
				rdpdr_add_async_irp(plugin, &irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_QUERY_INFORMATION:
			LLOGLN(10, ("IRP_MJ_QUERY_INFORMATION"));
			irp_process_query_information_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_SET_INFORMATION:
			LLOGLN(10, ("IRP_MJ_SET_INFORMATION"));
			irp_process_set_information_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_QUERY_VOLUME_INFORMATION:
			LLOGLN(10, ("IRP_MJ_QUERY_VOLUME_INFORMATION"));
			irp_process_query_volume_information_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_DIRECTORY_CONTROL:
			LLOGLN(10, ("IRP_MJ_DIRECTORY_CONTROL"));
			irp_process_directory_control_request(&irp, &data[20], data_size - 20);
			break;

		case IRP_MJ_DEVICE_CONTROL:
			LLOGLN(10, ("IRP_MJ_DEVICE_CONTROL"));
			irp_process_device_control_request(&irp, &data[20], data_size - 20);
			if (irp.ioStatus == RD_STATUS_PENDING)
				irp_queue_push(plugin->queue, &irp);
			break;

		case IRP_MJ_LOCK_CONTROL:
			LLOGLN(10, ("IRP_MJ_LOCK_CONTROL"));
			irp_process_file_lock_control_request(&irp, &data[20], data_size - 20);
			break;

		default:
			LLOGLN(0, ("IRP majorFunction=0x%x minorFunction=0x%x not supported", irp.majorFunction, irp.minorFunction));
			irp.ioStatus = RD_STATUS_NOT_SUPPORTED;
			break;
	}

	if (irp.abortIO)
	{
		if (irp.abortIO & RDPDR_ABORT_IO_WRITE)
			rdpdr_abort_single_io(plugin, irp_file_descriptor(&irp), RDPDR_ABORT_IO_WRITE, RD_STATUS_CANCELLED);
		if (irp.abortIO & RDPDR_ABORT_IO_READ)
			rdpdr_abort_single_io(plugin, irp_file_descriptor(&irp), RDPDR_ABORT_IO_READ, RD_STATUS_CANCELLED);
	}

	if (irp.ioStatus == (RD_STATUS_PENDING | 0xC0000000)) /* smart card */
	{
		irp.ioStatus = RD_STATUS_PENDING; /* this is going to be handled by the smart card plugin */
		LLOGLN(10, ("smart card irp must not be stored into plgugin->queue"));
	}
	else if (irp.ioStatus != RD_STATUS_PENDING)
	{
		out = irp_output_device_io_completion(&irp, &out_size);
		error = plugin->ep.pVirtualChannelWrite(plugin->open_handle, out, out_size, out);
		if (error != CHANNEL_RC_OK)
		{
			LLOGLN(0, ("rdpdr_process_irp: "
				"VirtualChannelWrite failed %d", error));
		}
		if (irp.outputBuffer)
		{
			free(irp.outputBuffer);
			irp.outputBuffer = NULL;
			irp.outputBufferLength = 0;
		}
	}

	if (irp_get_event(&irp, &result) && irp.rwBlocking)
	{
		LLOGLN(10, ("IRP process pending events"));
		IRP * pending = 0;
		while (!irp_queue_empty(plugin->queue))
		{
			pending = irp_queue_first(plugin->queue);
			pending->ioStatus = RD_STATUS_SUCCESS;
			out = irp_output_device_io_completion(pending, &out_size);
			error = plugin->ep.pVirtualChannelWrite(plugin->open_handle, out, out_size, out);
			if (pending->outputBuffer)
				free(pending->outputBuffer);
			irp_queue_pop(plugin->queue);
		}
	}
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

static void
rdpdr_process_prn(rdpdrPlugin * plugin, int type, char * data, int data_size)
{
	SERVICE * srv;

	/* Pass the control to the printer sub-plugin service, if registered */
	srv = devman_get_service_by_type(plugin->devman, RDPDR_DTYP_PRINT);
	if (srv == NULL)
	{
		LLOGLN(0, ("rdpdr_process_prn: printer service not register"));
		return;
	}
	if (srv->process_data == NULL)
	{
		LLOGLN(0, ("rdpdr_process_prn: printer service does not support"));
		return;
	}
	srv->process_data(srv, type, data, data_size);
}

static int
thread_process_message(rdpdrPlugin * plugin, char * data, int data_size)
{
	uint16 component;
	uint16 packetID;
	uint32 deviceID;
	uint32 status;
	SERVICE * scard_srv;

	component = GET_UINT16(data, 0);
	packetID = GET_UINT16(data, 2);

	if (component == RDPDR_CTYP_CORE)
	{
		LLOGLN(10, ("RDPDR_CTYP_CORE"));
		switch (packetID)
		{
			case PAKID_CORE_SERVER_ANNOUNCE:
				LLOGLN(10, ("PAKID_CORE_SERVER_ANNOUNCE"));
				rdpdr_process_server_announce_request(plugin, &data[4], data_size - 4);
				rdpdr_send_client_announce_reply(plugin);
				rdpdr_send_client_name_request(plugin);
				break;

			case PAKID_CORE_SERVER_CAPABILITY:
				/* server capabilities */
				LLOGLN(10, ("PAKID_CORE_SERVER_CAPABILITY"));
				rdpdr_process_capabilities(&data[4], data_size - 4);
				rdpdr_send_capabilities(plugin);
				break;

			case PAKID_CORE_CLIENTID_CONFIRM:
				LLOGLN(10, ("PAKID_CORE_CLIENTID_CONFIRM"));
				rdpdr_process_server_clientid_confirm(plugin, &data[4], data_size - 4);

				scard_srv = devman_get_service_by_type(plugin->devman, RDPDR_DTYP_SMARTCARD);

				/* versionMinor 0x0005 doesn't send PAKID_CORE_USER_LOGGEDON,
					so we have to send it here */
				if (plugin->versionMinor == 0x0005 || scard_srv)
					rdpdr_send_device_list_announce_request(plugin);
				break;

			case PAKID_CORE_USER_LOGGEDON:
				LLOGLN(10, ("PAKID_CORE_USER_LOGGEDON"));
				scard_srv = devman_get_service_by_type(plugin->devman, RDPDR_DTYP_SMARTCARD);
				if (!scard_srv)
					rdpdr_send_device_list_announce_request(plugin);
				break;

			case PAKID_CORE_DEVICE_REPLY:
				/* connect to a specific resource */
				deviceID = GET_UINT32(data, 4);
				status = GET_UINT32(data, 8);
				LLOGLN(10, ("PAKID_CORE_DEVICE_REPLY (deviceID=%d status=%d)", deviceID, status));
				break;

			case PAKID_CORE_DEVICE_IOREQUEST:
				LLOGLN(10, ("PAKID_CORE_DEVICE_IOREQUEST"));
				rdpdr_process_irp(plugin, &data[4], data_size - 4);
				break;

			default:
				LLOGLN(0, ("unknown packetID: 0x%02X", packetID));
				break;

		}
	}
	else if (component == RDPDR_CTYP_PRN)
	{
		LLOGLN(10, ("RDPDR_CTYP_PRN"));
		rdpdr_process_prn(plugin, packetID, &data[4], data_size - 4);
	}
	else
	{
		LLOGLN(0, ("RDPDR component: 0x%02X packetID: 0x%02X\n", component, packetID));
	}

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
	struct wait_obj * listobj[3];
	int numobj;
	SERVICE * scard_srv;

	if (arg == NULL)
	{
		LLOGLN(0, ("thread_func: arg is null"));
		return NULL;
	}

	plugin = (rdpdrPlugin *) arg;
	plugin->queue = irp_queue_new();
	plugin->thread_status = 1;

	scard_srv = devman_get_service_by_type(plugin->devman, RDPDR_DTYP_SMARTCARD);
	if (scard_srv)
	{
		if (scard_srv->create(NULL, NULL) == RD_STATUS_SUCCESS)
			pthread_create(&scard_thread, NULL, &rdpdr_scard_finished_scanner, plugin);
		else
			scard_srv->control = NULL; /* disallow any smart card operation */
	}

	LLOGLN(10, ("thread_func: in"));

	while (1)
	{
		listobj[0] = plugin->term_event;
		listobj[1] = plugin->data_in_event;
		listobj[2] = plugin->plugin_in_event;
		numobj = 3;
		wait_obj_select(listobj, numobj, NULL, 0, -1);

		plugin->nfds = 1;
		FD_ZERO(&plugin->readfds);
		FD_ZERO(&plugin->writefds);

		/* default timeout */
		plugin->tv.tv_sec = 0;
		plugin->tv.tv_usec = 20;
		plugin->select_timeout = 0;

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
		if (wait_obj_is_set(plugin->plugin_in_event))
		{
			if (rdpdr_check_fds(plugin) == 1)
				wait_obj_clear(plugin->plugin_in_event);
		}
	}

	LLOGLN(10, ("thread_func: out"));
	plugin->thread_status = -1;
	irp_queue_free(plugin->queue);
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
	wait_obj_free(plugin->plugin_in_event);
	pthread_mutex_destroy(plugin->mutex);
	free(plugin->mutex);

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
	RD_PLUGIN_DATA *plugin_data;

	LLOGLN(10, ("VirtualChannelEntry:"));

	plugin = (rdpdrPlugin *) malloc(sizeof(rdpdrPlugin));
	memset(plugin, 0, sizeof(rdpdrPlugin));

	chan_plugin_init((rdpChanPlugin *) plugin);

	plugin->data_in_size = 0;
	plugin->data_in = 0;
	plugin->ep = *pEntryPoints;

	memset(&(plugin->channel_def), 0, sizeof(plugin->channel_def));
	plugin->channel_def.options = CHANNEL_OPTION_INITIALIZED |
		CHANNEL_OPTION_ENCRYPT_RDP | CHANNEL_OPTION_COMPRESS_RDP;
	strcpy(plugin->channel_def.name, "rdpdr");

	plugin->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(plugin->mutex, 0);
	plugin->list_head = 0;
	plugin->list_tail = 0;

	plugin->term_event = wait_obj_new("freerdprdpdrterm");
	plugin->data_in_event = wait_obj_new("freerdprdpdrdatain");
	plugin->plugin_in_event = wait_obj_new("freerdprdpdrpluginin");

	plugin->thread_status = 0;

	if (pEntryPoints->cbSize >= sizeof(CHANNEL_ENTRY_POINTS_EX))
	{
		data = (((PCHANNEL_ENTRY_POINTS_EX)pEntryPoints)->pExtendedData);
	}
	else
	{
		data = NULL;
	}

	plugin_data = (RD_PLUGIN_DATA *) data;
	while (plugin_data && plugin_data->size > 0)
	{
		if (strcmp((char *) plugin_data->data[0], "clientname") == 0)
		{
			strcpy(plugin->computerName, (char *) plugin_data->data[1]);
			break;
		}

		plugin_data = (RD_PLUGIN_DATA *) (((void *) plugin_data) + plugin_data->size);
	}

	plugin->devman = devman_new(data);
	devman_load_device_service(plugin->devman, "disk");
	devman_load_device_service(plugin->devman, "printer");
	devman_load_device_service(plugin->devman, "serial");
	devman_load_device_service(plugin->devman, "parallel");
	devman_load_device_service(plugin->devman, "scard");

	plugin->ep.pVirtualChannelInit(&plugin->chan_plugin.init_handle, &plugin->channel_def, 1,
		VIRTUAL_CHANNEL_VERSION_WIN2000, InitEvent);

	return 1;
}

