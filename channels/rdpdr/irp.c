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
	
	uint32 fileID;

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
		irp->ioStatus = RD_STATUS_NOT_SUPPORTED;

	irp->ioStatus = irp->fns->create(irp->deviceID, desiredAccess, sharedAccess, createDisposition, createOptions, path, &fileID);

	irp_send_create_response(irp, fileID, 0);
}

void
irp_send_create_response(IRP* irp, uint32 fileID, uint8 information)
{
	STREAM s;
	s = channel_init(g_rdp->sec->mcs->chan, rdpdr_channel, 21);

	dr_out_device_io_completion_header(s, irp->deviceID, irp->completionID, irp->ioStatus);

	out_uint32_le(s, fileID); // fileID
	out_uint8(s, information); // information
	s_mark_end(s);

	channel_send(g_rdp->sec->mcs->chan, s, rdpdr_channel);
}

void
irp_process_close_request(STREAM s, IRP* irp)
{
	in_uint8s(s, 32); // pad
}

void
irp_process_read_request(STREAM s, IRP* irp)
{
	uint32 length;
	uint32 offsetLow;
	uint32 offsetHigh;

	in_uint32_le(s, length); // length
	in_uint32_le(s, offsetLow); // offsetLow
	in_uint32_le(s, offsetHigh); // offsetHigh
	in_uint8s(s, 20); // pad
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
	uint32 fsInformationClass;
	uint32 length;
	uint8* queryBuffer;

	printf("1337 h4x\n");

	in_uint32_le(s, fsInformationClass); // fsInformationClass
	in_uint32_le(s, length); // length
	in_uint8s(s, 24); // pad

	queryBuffer = (uint8*)xmalloc(length);
	in_uint8a(s, queryBuffer, length); // queryBuffer

	if (g_rdpdr_device[irp->deviceID].device_type != DEVICE_TYPE_DISK)
		irp->ioStatus = RD_STATUS_INVALID_HANDLE;

	STREAM out;
	out = channel_init(g_rdp->sec->mcs->chan, rdpdr_channel, 20 + length);

	dr_out_device_io_completion_header(out, irp->deviceID, irp->completionID, irp->ioStatus);

	irp->ioStatus = disk_query_information(irp->fileID, fsInformationClass, out);

	// irp_send_query_information_response(out, irp);

	s_mark_end(out);

	channel_send(g_rdp->sec->mcs->chan, out, rdpdr_channel);
}

void
irp_send_query_information_response(STREAM s, IRP* irp)
{
	s_mark_end(s);

	channel_send(g_rdp->sec->mcs->chan, s, rdpdr_channel);
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
			irp_process_query_directory_request(s, irp);
			break;
		
		case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
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
	uint32 fsInformationClass;
	uint8 initialQuery;	
	uint32 pathLength;

	in_uint32_le(s, fsInformationClass); // fsInformationClass
	in_uint8(s, initialQuery); // initialQuery
	in_uint32_le(s, pathLength); // pathLength
	in_uint8s(s, 23); // pad

	/* path */
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


