/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Disk Device Service

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdpdr.h"
#include "devman.h"

struct _DISK_DEVICE_INFO
{
	PDEVMAN devman;

	PDEVMAN_REGISTER_SERVICE DevmanRegisterService;
	PDEVMAN_UNREGISTER_SERVICE DevmanUnregisterService;
	PDEVMAN_REGISTER_DEVICE DevmanRegisterDevice;
	PDEVMAN_UNREGISTER_DEVICE DevmanUnregisterDevice;

	char * path;
};
typedef struct _DISK_DEVICE_INFO DISK_DEVICE_INFO;

int
disk_create(DEVICE * dev)
{
	printf("disk_create\n");
	return 0;
}

int
disk_close(DEVICE * dev)
{
	printf("disk_close\n");
	return 0;
}

int
disk_read(DEVICE * dev)
{
	printf("disk_read\n");
	return 0;
}

int
disk_write(DEVICE * dev)
{
	printf("disk_write\n");
	return 0;
}

int
disk_control(DEVICE * dev)
{
	printf("disk_control\n");
	return 0;
}

int
disk_free(DEVICE * dev)
{
	printf("disk_free\n");
	free(dev->info);
	return 0;
}

int
DeviceServiceEntry(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints)
{
	SERVICE * srv;
	DEVICE * dev;
	DISK_DEVICE_INFO * info;
	RD_PLUGIN_DATA * data;

	srv = pEntryPoints->pDevmanRegisterService(pDevman);

	srv->create = disk_create;
	srv->close = disk_close;
	srv->read = disk_read;
	srv->write = disk_write;
	srv->control = disk_control;
	srv->free = disk_free;
	srv->type = RDPDR_DTYP_FILESYSTEM;

	data = (RD_PLUGIN_DATA *) pEntryPoints->pExtendedData;
	while (data && data->size > 0)
	{
		if (strcmp((char*)data->data[0], "disk") == 0)
		{
			info = (DISK_DEVICE_INFO *) malloc(sizeof(DISK_DEVICE_INFO));
			info->devman = pDevman;
			info->DevmanRegisterService = pEntryPoints->pDevmanRegisterService;
			info->DevmanUnregisterService = pEntryPoints->pDevmanUnregisterService;
			info->DevmanRegisterDevice = pEntryPoints->pDevmanRegisterDevice;
			info->DevmanUnregisterDevice = pEntryPoints->pDevmanUnregisterDevice;
			info->path = (char *) data->data[2];

			dev = info->DevmanRegisterDevice(pDevman, srv, (char*)data->data[1]);
			dev->info = info;
		}
		data = (RD_PLUGIN_DATA *) (((void *) data) + data->size);
	}

	return 1;
}
