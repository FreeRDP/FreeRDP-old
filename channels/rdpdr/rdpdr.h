/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Protocol Services - Device Redirection

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

#ifndef __RDPDR_H
#define __RDPDR_H

void
dr_out_rdpdr_header(STREAM s, uint16 component, uint16 packetID);
void
dr_out_device_io_completion_header(STREAM s, uint32 deviceID, uint32 completionID, uint32 ioStatus);
void
convert_to_unix_filename(char *filename);
RD_BOOL
rdpdr_handle_ok(int device, int handle);
RD_BOOL
add_async_iorequest(uint32 device, uint32 file, uint32 id, uint32 major, uint32 length,
		    DEVICE_FNS * fns, uint32 total_timeout, uint32 interval_timeout, uint8 * buffer,
		    uint32 offset);

#endif // __RDPDR_H


