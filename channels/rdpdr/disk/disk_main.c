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

PDEVMAN devman;
DEVICE* disk_device;
SERVICE* disk_service;

PDEVMAN_REGISTER_SERVICE DevmanRegisterService;
PDEVMAN_UNREGISTER_SERVICE DevmanUnregisterService;
PDEVMAN_REGISTER_DEVICE DevmanRegisterDevice;
PDEVMAN_UNREGISTER_DEVICE DevmanUnregisterDevice;

int disk_create()
{
	printf("disk_create\n");
	return 0;
}

int disk_close()
{
	printf("disk_close\n");
	return 0;
}

int disk_read()
{
	printf("disk_read\n");
	return 0;
}

int disk_write()
{
	printf("disk_write\n");
	return 0;
}

int disk_control()
{
	printf("disk_control\n");
	return 0;
}

int
DeviceServiceEntry(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints)
{
	devman = pDevman;
	DevmanRegisterService = pEntryPoints->pDevmanRegisterService;
	DevmanUnregisterService = pEntryPoints->pDevmanUnregisterService;
	DevmanRegisterDevice = pEntryPoints->pDevmanRegisterDevice;
	DevmanUnregisterDevice = pEntryPoints->pDevmanUnregisterDevice;

	disk_service = DevmanRegisterService(devman);

	disk_service->create = disk_create;
	disk_service->close = disk_close;
	disk_service->read = disk_read;
	disk_service->write = disk_write;
	disk_service->control = disk_control;
	disk_service->type = RDPDR_DTYP_FILESYSTEM;

	disk_device = DevmanRegisterDevice(devman, disk_service, "disk");

	return 1;
}
