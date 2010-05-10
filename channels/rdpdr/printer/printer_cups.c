/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Printer Device Service

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010
   Copyright (C) Vic Lee <llyzs@163.com> 2010

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rdpdr_types.h"
#include "devman.h"
#include "chan_stream.h"
#include "printer_main.h"

struct _PRINTER_DEVICE_INFO
{
	PDEVMAN devman;

	PDEVMAN_REGISTER_SERVICE DevmanRegisterService;
	PDEVMAN_UNREGISTER_SERVICE DevmanUnregisterService;
	PDEVMAN_REGISTER_DEVICE DevmanRegisterDevice;
	PDEVMAN_UNREGISTER_DEVICE DevmanUnregisterDevice;
};
typedef struct _PRINTER_DEVICE_INFO PRINTER_DEVICE_INFO;

int
printer_register_device(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints, SERVICE * srv)
{
	DEVICE * dev;
	PRINTER_DEVICE_INFO * info;
	uint32 flag;
	char * driver_name;
	char * printer_name;
	int size;
	int offset;
	int len;

	LLOGLN(0, ("printer_register_device"));

	info = (PRINTER_DEVICE_INFO *) malloc(sizeof(PRINTER_DEVICE_INFO));
	memset(info, 0, sizeof(PRINTER_DEVICE_INFO));
	info->devman = pDevman;
	info->DevmanRegisterService = pEntryPoints->pDevmanRegisterService;
	info->DevmanUnregisterService = pEntryPoints->pDevmanUnregisterService;
	info->DevmanRegisterDevice = pEntryPoints->pDevmanRegisterDevice;
	info->DevmanUnregisterDevice = pEntryPoints->pDevmanUnregisterDevice;

	flag = RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER;// | RDPDR_PRINTER_ANNOUNCE_FLAG_XPSFORMAT;
	driver_name = "HP Color LaserJet 8500 PS";
	printer_name = "Test Printer";

	dev = info->DevmanRegisterDevice(pDevman, srv, "PRN1");
	dev->info = info;

	size = 24 + 4 + (strlen(printer_name) + 1) * 2 + (strlen(driver_name) + 1) * 2;
	dev->data = malloc(size);
	memset(dev->data, 0, size);

	SET_UINT32 (dev->data, 0, flag);
	SET_UINT32 (dev->data, 4, 0); /* CodePage, reserved */
	SET_UINT32 (dev->data, 8, 0); /* PnPNameLen */
	SET_UINT32 (dev->data, 20, 0); /* CachedFieldsLen */
	offset = 24;
	len = set_wstr(&dev->data[offset], size - offset, driver_name, strlen(driver_name) + 1);
	SET_UINT32 (dev->data, 12, len); /* DriverNameLen */
	offset += len;
	len = set_wstr(&dev->data[offset], size - offset, printer_name, strlen(printer_name) + 1);
	SET_UINT32 (dev->data, 16, len); /* PrintNameLen */
	offset += len;

	dev->data_len = offset;

printf ("data_len %i\n", dev->data_len);
for (len = 0; len < dev->data_len; len++) printf ("%02X ", (unsigned char) dev->data[len]);

	return 0;
}

uint32
printer_create(IRP * irp, const char * path)
{
	return 0;
}

uint32
printer_close(IRP * irp)
{
	return 0;
}

uint32
printer_write(IRP * irp)
{
	return 0;
}

uint32
printer_free(DEVICE * dev)
{
	PRINTER_DEVICE_INFO * info;

	LLOGLN(0, ("printer_free"));
	info = (PRINTER_DEVICE_INFO *) dev->info;
	free(info);
	return 0;
}

