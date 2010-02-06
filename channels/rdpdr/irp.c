/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Device Redirection - Interrupt Request Packet (IRP) Processing

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2009

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include "disk.h"
#include "rdp.h"
#include "rdpset.h"
#include "rdpdr.h"
#include "constants.h"
#include "mcs.h"
#include "channels.h"
#include "secure.h"
#include "mem.h"

#include "irp.h"

extern rdpRdp * g_rdp;
extern VCHANNEL *rdpdr_channel;
extern CHANNEL_ENTRY_POINTS g_ep;
extern RDPDR_DEVICE g_device[RDPDR_MAX_DEVICES];

void
irp_output_device_io_completion_header(char* data, int data_size, uint32 deviceID, uint32 completionID, uint32 ioStatus)
{
	if(data_size < 16)
		return;

	SET_UINT16(data, 0, RDPDR_COMPONENT_TYPE_CORE); /* component */
	SET_UINT16(data, 2, PAKID_CORE_DEVICE_IOCOMPLETION); /* packetID */
	SET_UINT32(data, 4, deviceID); /* deviceID */
	SET_UINT32(data, 8, completionID); /* completionID */
	SET_UINT32(data, 12, ioStatus); /* ioStatus */
}

void
irp_process_create_request(char* data, int data_size, IRP* irp)
{
	uint32 desiredAccess;
	uint32 allocationSizeHigh;
	uint32 allocationSizeLow;
	uint32 fileAttributes;
	uint32 sharedAccess;
	uint32 createDisposition;
	uint32 createOptions;
	uint32 pathLength;
	char path[PATH_MAX];

	desiredAccess = GET_UINT32(data, desiredAccess); /* desiredAccess */
	allocationSizeHigh = GET_UINT32(data, allocationSizeHigh); /* allocationSizeHigh */
	allocationSizeLow = GET_UINT32(data, allocationSizeLow); /* allocationSizeLow */
	fileAttributes = GET_UINT32(data, fileAttributes); /* fileAttributes */
	sharedAccess = GET_UINT32(data, sharedAccess); /* sharedAccess */
	createDisposition = GET_UINT32(data, createDisposition); /* createDisposition */
	createOptions = GET_UINT32(data, createOptions); /* createOptions */
	pathLength = GET_UINT32(data, pathLength); /* pathLength */

	if (pathLength && (pathLength / 2) < 256)
	{
		//rdp_in_unistr(g_rdp, s, path, sizeof(path), pathLength);
		//convert_to_unix_filename(path);
	}
	else
		path[0] = '\0';

	if (!irp->fns->create)
	{
		//printf("RD_STATUS_NOT_SUPPORTED\n");
		irp->ioStatus = RD_STATUS_NOT_SUPPORTED;
	}

	irp->ioStatus =
		irp->fns->create(irp->deviceID, desiredAccess, sharedAccess, createDisposition, createOptions, path, &(irp->fileID));
}

void
irp_send_create_response(IRP* irp)
{
	int error;
	char* data = malloc(26);

	irp_output_device_io_completion_header(data, 21,
		irp->deviceID, irp->completionID, irp->ioStatus);

	SET_UINT32(data, 21, irp->fileID); /* fileID */
	SET_UINT8(data, 25, 0); /* information */

	error = g_ep.pVirtualChannelWrite(g_open_handle[0], data, 26, data);

	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite failed %d", error));
		return 1;
	}
}

void
irp_process_close_request(char* data, int data_size, IRP* irp)
{
	/* 32-byte pad */

	if (!irp->fns->close)
		irp->ioStatus = RD_STATUS_NOT_SUPPORTED;

	irp->ioStatus = irp->fns->close(irp->fileID);
}

void
irp_send_close_response(IRP* irp)
{
	int error;
	char* data = malloc(21);

	irp_output_device_io_completion_header(data, 21,
		irp->deviceID, irp->completionID, irp->ioStatus);

	memset(&data[16], '\0', 5); /* pad */

	error = g_ep.pVirtualChannelWrite(g_open_handle[0], data, 21, data);

	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite failed %d", error));
		return 1;
	}
}

void
irp_process_read_request(char* data, int data_size, IRP* irp)
{
	uint32 length;
	uint32 offsetLow;
	uint32 offsetHigh;

	uint8* pstBuffer;
	uint32 totalTimeout = 0;
	uint32 intervalTimeout = 0;

	length = GET_UINT32(data, 0); /* length */
	offsetLow = GET_UINT32(data, 4); /* offsetLow */
	offsetHigh = GET_UINT32(data, 8); /* offsetHigh */
	/* 20-byte pad */

	if (!irp->fns->read)
		irp->ioStatus = RD_STATUS_NOT_SUPPORTED;

	if (!rdpdr_handle_ok(irp->deviceID, irp->fileID))
		irp->ioStatus = RD_STATUS_INVALID_HANDLE;

	if (irp->rwBlocking)	/* Complete read immediately */
	{
		unsigned int bytesRead = 0;
		
		irp->buffer->size = length;
		irp->buffer->data = xmalloc(irp->buffer->size);
		irp->buffer->p = irp->buffer->data;
		irp->buffer->end = irp->buffer->data + irp->buffer->size;

		if (!irp->buffer->data)
			irp->ioStatus = RD_STATUS_CANCELLED;
		
		irp->ioStatus = irp->fns->read(irp->fileID, irp->buffer->p, irp->buffer->size, offsetLow, &bytesRead);
		irp->buffer->size = bytesRead;
	}

	/* Add request to table */
	pstBuffer = (uint8 *) xmalloc(length);
	if (!pstBuffer)
		irp->ioStatus = RD_STATUS_CANCELLED;
	
	serial_get_timeout(irp->fileID, length, &totalTimeout, &intervalTimeout);
	
	if (add_async_iorequest(irp->deviceID, irp->fileID,
		irp->completionID, IRP_MJ_READ, length, irp->fns,
		totalTimeout, intervalTimeout, pstBuffer, offsetLow))
	{
		irp->ioStatus = RD_STATUS_PENDING;
	}
	else
	{
		irp->ioStatus = RD_STATUS_CANCELLED;
	}
}

void
irp_send_read_response(IRP* irp)
{
	int error;
	int data_size = 16 + irp->buffer_size;
	char* data = malloc(data_size);

	irp_output_device_io_completion_header(data, data_size,
		irp->deviceID, irp->completionID, irp->ioStatus);

	memcpy(&data[16], irp->buffer, irp->buffer_size);

	error = g_ep.pVirtualChannelWrite(g_open_handle[0], data, 21, data);

	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite failed %d", error));
		return 1;
	}
}

void
irp_process_write_request(char* data, int data_size, IRP* irp)
{
	uint32 length;
	uint32 offsetLow;
	uint32 offsetHigh;

	length = GET_UINT32(data, 0); /* length */
	offsetLow = GET_UINT32(data, 4); /* offsetLow */
	offsetHigh = GET_UINT32(data, 8); /* offsetHigh */
	/* 20-byte pad */

	/* writeData */
}

void
irp_process_query_volume_information_request(char* data, int data_size, IRP* irp)
{
	uint32 fsInformationClass;
	uint32 length;

	fsInformationClass = GET_UINT32(data, 0); /* fsInformationClass */
	length = GET_UINT32(data, 4); /* length */
	/* 24-byte pad */
	
	/* queryVolumeBuffer */
}

void
irp_process_set_volume_information_request(char* data, int data_size, IRP* irp)
{
	uint32 fsInformationClass;
	uint32 length;

	fsInformationClass = GET_UINT32(data, 0); /* fsInformationClass */
	length = GET_UINT32(data, 4); /* length */
	/* 24-byte pad */
	
	/* setVolumeBuffer */
}

void
irp_process_query_information_request(char* data, int data_size, IRP* irp)
{
	uint32 length;
	char* queryBuffer;

	irp->infoClass = GET_UINT32(data, 0); /* fsInformationClass */
	length = GET_UINT32(data, 4); /* length */
	/* 24-byte pad */

	if(length > 0)
	{
		queryBuffer = (char*)malloc(length);
		//in_uint8a(s, queryBuffer, length); // queryBuffer
	}

	if (g_device[irp->deviceID].deviceType != DEVICE_TYPE_DISK)
	{
		irp->ioStatus = RD_STATUS_INVALID_HANDLE;
		//printf("irp_process_query_information_request: RD_STATUS_INVALID_HANDLE\n");
	}

	//disk_query_information(irp);
}

void
irp_send_query_information_response(IRP* irp)
{
	int error;
	int data_size = 16 + irp->buffer_size;
	char* data = malloc(data_size);

	irp_output_device_io_completion_header(data, data_size,
		irp->deviceID, irp->completionID, irp->ioStatus);

	SET_UINT32(data, irp->buffer_size);
	memcpy(&data[20], irp->buffer, irp->buffer_size);

	error = g_ep.pVirtualChannelWrite(g_open_handle[0], data, data_size, data);

	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite failed %d", error));
		return 1;
	}
}

void
irp_process_set_information_request(char* data, int data_size, IRP* irp)
{
	uint32 fsInformationClass;
	uint32 length;

	fsInformationClass = GET_UINT32(data, 0); /* fsInformationClass */
	length = GET_UINT32(data, 4); /* length */
	/* 24-byte pad */
	
	/* setBuffer */
}

void
irp_process_directory_control_request(char* data, int data_size, IRP* irp)
{
	switch(irp->minorFunction)
	{
		case IRP_MN_QUERY_DIRECTORY:
			LLOGLN(0, ("IRP_MN_QUERY_DIRECTORY\n"));
			irp_process_query_directory_request(data, data_size, irp);
			irp_send_query_directory_response(irp);
			break;
		
		case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
			LLOGLN(0, ("IRP_MN_NOTIFY_CHANGE_DIRECTORY\n"));
			irp_process_notify_change_directory_request(data, data_size, irp);
			break;

		default:
			//ui_unimpl(NULL, "IRP majorFunction=0x%x minorFunction=0x%x\n", irp->majorFunction, irp->minorFunction);
			return;
	}
}

void
irp_process_device_control_request(char* data, int data_size, IRP* irp)
{
	uint32 outputBufferLength;
	uint32 inputBufferLength;
	uint32 ioControlCode;
	
	outputBufferLength = GET_UINT32(data, 0); /* outputBufferLength */
	inputBufferLength = GET_UINT32(data, 4); /* inputBufferLength */
	ioControlCode = GET_UINT32(data, 8); /* ioControlCode */
	/* 20-byte pad */

	/* inputBuffer */
}

void
irp_process_file_lock_control_request(char* data, int data_size, IRP* irp)
{
	uint8 f;
	uint32 numLocks;
	uint32 operation;

	operation = GET_UINT32(data, 0); /* operation */
	f = GET_UINT8(data, 4); /* f (first bit) */
	/* pad (f + pad = 32 bits) */
	numLocks = GET_UINT32(data, 8); /* numLocks */
	/* 20-byte pad */

	/* locks */
}

void
irp_process_query_directory_request(char* data, int data_size, IRP* irp)
{
	uint8 initialQuery;	
	uint32 pathLength;
	char* path = NULL;

	irp->infoClass = GET_UINT32(data, 0); /* fsInformationClass */
	initialQuery = GET_UINT8(data, 4); /* initialQuery */
	pathLength = GET_UINT32(data, 5); /* pathLength */
	/* 23-byte pad */

	if(pathLength > 0 && pathLength < 2 * 255)
	{
		path = (char*)malloc(pathLength);
		//rdp_in_unistr(g_rdp, s, path, pathLength, pathLength);
		//convert_to_unix_filename(path);
	}

	//disk_query_directory(irp, initialQuery, path);
}

void
irp_send_query_directory_response(IRP* irp)
{
	int error;
	int data_size = 16 + irp->buffer_size;
	char* data = malloc(data_size);

	irp_output_device_io_completion_header(data, data_size,
		irp->deviceID, irp->completionID, irp->ioStatus);

	SET_UINT32(data, irp->buffer_size);
	memcpy(&data[20], irp->buffer, irp->buffer_size);

	error = g_ep.pVirtualChannelWrite(g_open_handle[0], data, data_size, data);

	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite failed %d", error));
		return 1;
	}
}

void
irp_process_notify_change_directory_request(char* data, int data_size, IRP* irp)
{
	uint8 watchQuery;	
	uint32 completionQuery;

	watchQuery = GET_UINT8(data, 0); /* watchQuery */
	completionQuery = GET_UINT32(data, 1); /* completionQuery */
	/* 27-byte pad */
}


