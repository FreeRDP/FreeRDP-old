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

#ifndef __IRP_H
#define __IRP_H

typedef struct _IRP
{
	uint32 deviceID;
	uint32 fileID;
	uint32 completionID;
	uint32 majorFunction;
	uint32 minorFunction;
	DEVICE_FNS* fns;
	RD_BOOL rwBlocking;
	uint32 ioStatus;
}
IRP;

void
irp_process_create_request(STREAM s, IRP* irp);
void
irp_send_create_response(IRP* irp, uint32 fileID, uint8 information);
void
irp_process_close_request(STREAM s, IRP* irp);
void
irp_process_read_request(STREAM s, IRP* irp);
void
irp_process_write_request(STREAM s, IRP* irp);
void
irp_process_query_volume_information_request(STREAM s, IRP* irp);
void
irp_process_set_volume_information_request(STREAM s, IRP* irp);
void
irp_process_query_information_request(STREAM s, IRP* irp);
void
irp_send_query_information_response(STREAM s, IRP* irp);
void
irp_process_set_information_request(STREAM s, IRP* irp);
void
irp_process_directory_control_request(STREAM s, IRP* irp);
void
irp_process_device_control_request(STREAM s, IRP* irp);
void
irp_process_file_lock_control_request(STREAM s, IRP* irp);
void
irp_process_query_directory_request(STREAM s, IRP* irp);
void
irp_process_notify_change_directory_request(STREAM s, IRP* irp);

#endif // __IRP_H
