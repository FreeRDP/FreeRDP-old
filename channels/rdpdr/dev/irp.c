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
#include "rdesktop.h"
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
extern RDPDR_DEVICE g_rdpdr_device[RDPDR_MAX_DEVICES];

void
irp_process_create_request(STREAM s, IRP* irp)
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

	in_uint32_le(s, desiredAccess); // desiredAccess
	in_uint32_le(s, allocationSizeHigh); // allocationSizeHigh
	in_uint32_le(s, allocationSizeLow); // allocationSizeLow
	in_uint32_le(s, fileAttributes); // fileAttributes
	in_uint32_le(s, sharedAccess); // sharedAccess
	in_uint32_le(s, createDisposition); // createDisposition
	in_uint32_le(s, createOptions); // createOptions
	in_uint32_le(s, pathLength); // pathLength

	if (pathLength && (pathLength / 2) < 256)
	{
		rdp_in_unistr(g_rdp, s, path, sizeof(path), pathLength);
		convert_to_unix_filename(path);
	}
	else
		path[0] = '\0';

	if (!irp->fns->create)
	{
		printf("RD_STATUS_NOT_SUPPORTED\n");
		irp->ioStatus = RD_STATUS_NOT_SUPPORTED;
	}

	irp->ioStatus =
		irp->fns->create(irp->deviceID, desiredAccess, sharedAccess, createDisposition, createOptions, path, &(irp->fileID));
}

void
irp_send_create_response(IRP* irp)
{
	STREAM rsp;
	rsp = channel_init(g_rdp->sec->mcs->chan, rdpdr_channel, 21);

	dr_out_device_io_completion_header(rsp, irp->deviceID, irp->completionID, irp->ioStatus);

	out_uint32_le(rsp, irp->fileID); // fileID
	out_uint8(rsp, 0); // information

	s_mark_end(rsp);
	hexdump(rsp->data, rsp->end - rsp->data);
	channel_send(g_rdp->sec->mcs->chan, rsp, rdpdr_channel);
}

void
irp_process_close_request(STREAM s, IRP* irp)
{
	in_uint8s(s, 32); // pad

	if (!irp->fns->close)
		irp->ioStatus = RD_STATUS_NOT_SUPPORTED;

	irp->ioStatus = irp->fns->close(irp->fileID);
}

void
irp_send_close_response(IRP* irp)
{
	STREAM s;
	s = channel_init(g_rdp->sec->mcs->chan, rdpdr_channel, 21);
	dr_out_device_io_completion_header(s, irp->deviceID, irp->completionID, irp->ioStatus);
	out_uint8s(s, 5); // pad
	s_mark_end(s);
	hexdump(s->data, s->end - s->data);
	channel_send(g_rdp->sec->mcs->chan, s, rdpdr_channel);
}

void
irp_process_read_request(STREAM s, IRP* irp)
{
	uint32 length;
	uint32 offsetLow;
	uint32 offsetHigh;

	uint8* pstBuffer;
	uint32 totalTimeout = 0;
	uint32 intervalTimeout = 0;

	in_uint32_le(s, length); // length
	in_uint32_le(s, offsetLow); // offsetLow
	in_uint32_le(s, offsetHigh); // offsetHigh
	in_uint8s(s, 20); // pad

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
	STREAM s;
	s = channel_init(g_rdp->sec->mcs->chan, rdpdr_channel, 20 + irp->buffer->size);
	dr_out_device_io_completion_header(s, irp->deviceID, irp->completionID, irp->ioStatus);
	s_append_stream(s, irp->buffer);
	s_mark_end(s);
	channel_send(g_rdp->sec->mcs->chan, s, rdpdr_channel);
}

void
irp_process_write_request(STREAM s, IRP* irp)
{
	uint32 length;
	uint32 offsetLow;
	uint32 offsetHigh;

	in_uint32_le(s, length); // length
	in_uint32_le(s, offsetLow); // offsetLow
	in_uint32_le(s, offsetHigh); // offsetHigh
	in_uint8s(s, 20); // pad

	/* writeData */
}

void
irp_process_query_volume_information_request(STREAM s, IRP* irp)
{
	uint32 fsInformationClass;
	uint32 length;

	in_uint32_le(s, fsInformationClass); // fsInformationClass
	in_uint32_le(s, length); // length
	in_uint8s(s, 24); // pad
	
	/* queryVolumeBuffer */
}

void
irp_process_set_volume_information_request(STREAM s, IRP* irp)
{
	uint32 fsInformationClass;
	uint32 length;

	in_uint32_le(s, fsInformationClass); // fsInformationClass
	in_uint32_le(s, length); // length
	in_uint8s(s, 24); // pad
	
	/* setVolumeBuffer */
}

void
irp_process_query_information_request(STREAM s, IRP* irp)
{
	uint32 length;
	uint8* queryBuffer = NULL;

	in_uint32_le(s, irp->infoClass); // fsInformationClass
	in_uint32_le(s, length); // length
	in_uint8s(s, 24); // pad

	if(length > 0)
	{
		queryBuffer = (uint8*)xmalloc(length);
		in_uint8a(s, queryBuffer, length); // queryBuffer
	}

	if (g_rdpdr_device[irp->deviceID].device_type != DEVICE_TYPE_DISK)
	{
		irp->ioStatus = RD_STATUS_INVALID_HANDLE;
		printf("irp_process_query_information_request: RD_STATUS_INVALID_HANDLE\n");
	}

	disk_query_information(irp);
}

void
irp_send_query_information_response(IRP* irp)
{
	STREAM rsp;
	rsp = channel_init(g_rdp->sec->mcs->chan, rdpdr_channel, irp->buffer->size + 16);

	dr_out_device_io_completion_header(rsp, irp->deviceID, irp->completionID, irp->ioStatus);
	out_uint32_le(rsp, irp->buffer->size);
	s_append_stream(rsp, irp->buffer);
	s_mark_end(rsp);

	hexdump(rsp->data, rsp->end - rsp->data);

	channel_send(g_rdp->sec->mcs->chan, rsp, rdpdr_channel);
}

void
irp_process_set_information_request(STREAM s, IRP* irp)
{
	uint32 fsInformationClass;
	uint32 length;

	in_uint32_le(s, fsInformationClass); // fsInformationClass
	in_uint32_le(s, length); // length
	in_uint8s(s, 24); // pad
	
	/* setBuffer */
}

void
irp_process_directory_control_request(STREAM s, IRP* irp)
{
	switch(irp->minorFunction)
	{
		case IRP_MN_QUERY_DIRECTORY:
			printf("IRP_MN_QUERY_DIRECTORY\n");
			irp_process_query_directory_request(s, irp);
			irp_send_query_directory_response(irp);
			break;
		
		case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
			printf("IRP_MN_NOTIFY_CHANGE_DIRECTORY\n");
			irp_process_notify_change_directory_request(s, irp);
			break;

		default:
			ui_unimpl(NULL, "IRP majorFunction=0x%x minorFunction=0x%x\n", irp->majorFunction, irp->minorFunction);
			return;
	}
}

void
irp_process_device_control_request(STREAM s, IRP* irp)
{
	uint32 outputBufferLength;
	uint32 inputBufferLength;
	uint32 ioControlCode;
	
	in_uint32_le(s, outputBufferLength); // outputBufferLength
	in_uint32_le(s, inputBufferLength); // inputBufferLength
	in_uint32_le(s, ioControlCode); // ioControlCode
	in_uint8s(s, 20); // pad

	/* inputBuffer */
}

void
irp_process_file_lock_control_request(STREAM s, IRP* irp)
{
	uint32 operation;
	uint8 f;
	uint32 numLocks;

	in_uint32_le(s, operation); // operation
	in_uint8(s, f); // f (first bit)
	in_uint8s(s, 3); // pad (f + pad = 32 bits)
	in_uint32_le(s, numLocks); // numLocks
	in_uint8s(s, 20);

	/* locks */
}

void
irp_process_query_directory_request(STREAM s, IRP* irp)
{
	uint8 initialQuery;	
	uint32 pathLength;
	char* path = NULL;

	in_uint32_le(s, irp->infoClass); // fsInformationClass
	in_uint8(s, initialQuery); // initialQuery
	in_uint32_le(s, pathLength); // pathLength
	in_uint8s(s, 23); // pad

	if(pathLength > 0 && pathLength < 2 * 255)
	{
		path = (char*)xmalloc(pathLength);
		rdp_in_unistr(g_rdp, s, path, pathLength, pathLength);
		convert_to_unix_filename(path);
	}

	disk_query_directory(irp, initialQuery, path);
}

void
irp_send_query_directory_response(IRP* irp)
{
	STREAM rsp;
	rsp = channel_init(g_rdp->sec->mcs->chan, rdpdr_channel, 26 + irp->buffer->size);
	dr_out_device_io_completion_header(rsp, irp->deviceID, irp->completionID, irp->ioStatus);
	out_uint32_le(rsp, irp->buffer->size);
	s_append_stream(rsp, irp->buffer);
	s_mark_end(rsp);
	hexdump(rsp->data, rsp->end - rsp->data);
	channel_send(g_rdp->sec->mcs->chan, rsp, rdpdr_channel);
}

void
irp_process_notify_change_directory_request(STREAM s, IRP* irp)
{
	uint8 watchQuery;	
	uint32 completionQuery;

	in_uint8(s, watchQuery); // watchQuery
	in_uint32_le(s, completionQuery); // completionQuery
	in_uint8s(s, 27); // pad
}


