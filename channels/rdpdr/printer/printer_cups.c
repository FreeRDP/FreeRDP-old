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
#include <cups/cups.h>
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

	int using_xps;
};
typedef struct _PRINTER_DEVICE_INFO PRINTER_DEVICE_INFO;

int
printer_register_device(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints, SERVICE * srv)
{
	DEVICE * dev;
	PRINTER_DEVICE_INFO * info;
	cups_dest_t *dests;
	cups_dest_t *dest;
	int num_dests;
	int i;
	char buf[8];
	uint32 flags;
	char * driver_name;
	int size;
	int offset;
	int len;

	num_dests = cupsGetDests(&dests);
	for (i = 1, dest = dests; i <= num_dests; i++, dest++)
	{
		if (dest->instance == NULL)
		{
			LLOGLN(10, ("printer_register_device: %s (default=%d)", dest->name, dest->is_default));

			info = (PRINTER_DEVICE_INFO *) malloc(sizeof(PRINTER_DEVICE_INFO));
			memset(info, 0, sizeof(PRINTER_DEVICE_INFO));
			info->devman = pDevman;
			info->DevmanRegisterService = pEntryPoints->pDevmanRegisterService;
			info->DevmanUnregisterService = pEntryPoints->pDevmanUnregisterService;
			info->DevmanRegisterDevice = pEntryPoints->pDevmanRegisterDevice;
			info->DevmanUnregisterDevice = pEntryPoints->pDevmanUnregisterDevice;

			driver_name = "HP Color LaserJet 8500 PS";

			snprintf(buf, sizeof(buf) - 1, "PRN%d", i);
			dev = info->DevmanRegisterDevice(pDevman, srv, buf);
			dev->info = info;

			size = 24 + 4 + (strlen(dest->name) + 1) * 2 + (strlen(driver_name) + 1) * 2;
			dev->data = malloc(size);
			memset(dev->data, 0, size);

			flags = RDPDR_PRINTER_ANNOUNCE_FLAG_XPSFORMAT;
			if (dest->is_default)
				flags |= RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER;

			SET_UINT32 (dev->data, 0, flags); /* Flags */
			SET_UINT32 (dev->data, 4, 0); /* CodePage, reserved */
			SET_UINT32 (dev->data, 8, 0); /* PnPNameLen */
			SET_UINT32 (dev->data, 20, 0); /* CachedFieldsLen */
			offset = 24;
			len = set_wstr(&dev->data[offset], size - offset, driver_name, strlen(driver_name) + 1);
			SET_UINT32 (dev->data, 12, len); /* DriverNameLen */
			offset += len;
			len = set_wstr(&dev->data[offset], size - offset, dest->name, strlen(dest->name) + 1);
			SET_UINT32 (dev->data, 16, len); /* PrintNameLen */
			offset += len;

			dev->data_len = offset;
		}
	}
	cupsFreeDests(num_dests, dests);
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

	LLOGLN(10, ("printer_free"));
	info = (PRINTER_DEVICE_INFO *) dev->info;
	free(info);
	if (dev->data)
	{
		free(dev->data);
		dev->data = NULL;
	}
	return 0;
}

