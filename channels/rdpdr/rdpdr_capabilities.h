/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Device Redirection Capability Sets

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

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

#ifndef __RDPDR_CAPABILITIES_H
#define __RDPDR_CAPABILITIES_H

#include "rdpdr.h"

int
rdpdr_out_general_capset(char* data, int size);
int
rdpdr_out_printer_capset(char* data, int size);
int
rdpdr_out_port_capset(char* data, int size);
int
rdpdr_out_drive_capset(char* data, int size);
int
rdpdr_out_smartcard_capset(char* data, int size);

int
rdpdr_process_general_capset(char* data, int size);
int
rdpdr_process_printer_capset(char* data, int size);
int
rdpdr_process_port_capset(char* data, int size);
int
rdpdr_process_drive_capset(char* data, int size);
int
rdpdr_process_smartcard_capset(char* data, int size);

void
rdpdr_process_capabilities(char* data, int size);

#endif /* __RDPDR_CAPABILITIES_H */
