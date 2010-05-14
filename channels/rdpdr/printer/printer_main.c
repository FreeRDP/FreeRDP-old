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
#include "rdpdr_constants.h"
#include "rdpdr_types.h"
#include "devman.h"
#include "printer_main.h"

static SERVICE * 
printer_register_service(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints)
{
	SERVICE * srv;

	srv = pEntryPoints->pDevmanRegisterService(pDevman);

	srv->create = printer_hw_create;
	srv->close = printer_hw_close;
	srv->read = NULL;
	srv->write = printer_hw_write;
	srv->control = NULL;
	srv->query_volume_info = NULL;
	srv->query_info = NULL;
	srv->set_info = NULL;
	srv->query_directory = NULL;
	srv->notify_change_directory = NULL;
	srv->lock_control = NULL;
	srv->free = printer_free;
	srv->type = RDPDR_DTYP_PRINT;

	return srv;
}

int
printer_register(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints, SERVICE * srv,
	const char * name, const char * driver, int is_default, int * port)
{
	DEVICE * dev;
	char buf[8];
	uint32 flags;
	int size;
	int offset;
	int len;

	LLOGLN(0, ("printer_register: %s (default=%d)", name, is_default));

	if (driver == NULL)
	{
		/* This is a generic PostScript printer driver developed by MS, so it should be good in most cases */
		driver = "MS Publisher Imagesetter";
	}

	snprintf(buf, sizeof(buf) - 1, "PRN%d", *port);
	*port += 1;
	dev = pEntryPoints->pDevmanRegisterDevice(pDevman, srv, buf);
	dev->info = printer_hw_new(name);

	size = 24 + 4 + (strlen(name) + 1) * 2 + (strlen(driver) + 1) * 2;
	dev->data = malloc(size);
	memset(dev->data, 0, size);

	/*flags = RDPDR_PRINTER_ANNOUNCE_FLAG_XPSFORMAT;*/
	flags = 0;
	if (is_default)
		flags |= RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER;

	SET_UINT32 (dev->data, 0, flags); /* Flags */
	SET_UINT32 (dev->data, 4, 0); /* CodePage, reserved */
	SET_UINT32 (dev->data, 8, 0); /* PnPNameLen */
	SET_UINT32 (dev->data, 20, 0); /* CachedFieldsLen */
	offset = 24;
	len = set_wstr(&dev->data[offset], size - offset, (char *) driver, strlen(driver) + 1);
	SET_UINT32 (dev->data, 12, len); /* DriverNameLen */
	offset += len;
	len = set_wstr(&dev->data[offset], size - offset, (char *) name, strlen(name) + 1);
	SET_UINT32 (dev->data, 16, len); /* PrintNameLen */
	offset += len;

	dev->data_len = offset;

	return 0;
}

uint32
printer_free(DEVICE * dev)
{
	printer_hw_free(dev->info);
	if (dev->data)
	{
		free(dev->data);
		dev->data = NULL;
	}
	return 0;
}

int
DeviceServiceEntry(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints)
{
	SERVICE * srv = NULL;
	RD_PLUGIN_DATA * data;
	int port = 1;

	data = (RD_PLUGIN_DATA *) pEntryPoints->pExtendedData;
	while (data && data->size > 0)
	{
		if (strcmp((char*)data->data[0], "printer") == 0)
		{
			if (srv == NULL)
				srv = printer_register_service(pDevman, pEntryPoints);

			if (data->data[1] == NULL)
				printer_hw_register_auto(pDevman, pEntryPoints, srv, &port);
			else
				printer_register(pDevman, pEntryPoints, srv, data->data[1], data->data[2], (port == 1), &port);
			break;
		}
		data = (RD_PLUGIN_DATA *) (((void *) data) + data->size);
	}

	return 1;
}
