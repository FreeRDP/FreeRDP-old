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
#include <dirent.h>

#include "rdpdr_types.h"
#include "rdpdr_constants.h"
#include "devman.h"

struct _FILE_INFO
{
	uint32 file_id;
	FILE * file;
	DIR * dir;
	struct _FILE_INFO * next;
};
typedef struct _FILE_INFO FILE_INFO;

struct _DISK_DEVICE_INFO
{
	PDEVMAN devman;

	PDEVMAN_REGISTER_SERVICE DevmanRegisterService;
	PDEVMAN_UNREGISTER_SERVICE DevmanUnregisterService;
	PDEVMAN_REGISTER_DEVICE DevmanRegisterDevice;
	PDEVMAN_UNREGISTER_DEVICE DevmanUnregisterDevice;

	char * path;

	FILE_INFO * head;
};
typedef struct _DISK_DEVICE_INFO DISK_DEVICE_INFO;

static void
disk_remove_file(DEVICE * dev, uint32 file_id)
{
	DISK_DEVICE_INFO * info;
	FILE_INFO * curr;
	FILE_INFO * prev;

	info = (DISK_DEVICE_INFO *) dev->info;
	for (prev = NULL, curr = info->head; curr; prev = curr, curr = curr->next)
	{
		if (curr->file_id == file_id)
		{
			LLOGLN(0, ("disk_remove_file: id=%d", curr->file_id));
			if (curr->file)
				fclose(curr->file);
			if (curr->dir)
				closedir(curr->dir);
			if (prev == NULL)
				info->head = curr->next;
			else
				prev->next  = curr->next;
			free(curr);			
			break;
		}
	}
}

int
disk_create(IRP * irp, const char * path)
{
	DISK_DEVICE_INFO * info;
	FILE_INFO * finfo;

	info = (DISK_DEVICE_INFO *) irp->dev->info;
	finfo = (FILE_INFO *) malloc(sizeof(FILE_INFO));
	memset(finfo, 0, sizeof(FILE_INFO));
	finfo->file_id = info->devman->id_sequence++;
	finfo->next = info->head;
	info->head = finfo;

	irp->fileID = finfo->file_id;
	LLOGLN(0, ("disk_create: %s (id=%d)", path, finfo->file_id));

	return RD_STATUS_SUCCESS;
}

int
disk_close(IRP * irp)
{
	printf("disk_close\n");
	return 0;
}

int
disk_read(IRP * irp)
{
	printf("disk_read\n");
	return 0;
}

int
disk_write(IRP * irp)
{
	printf("disk_write\n");
	return 0;
}

int
disk_control(IRP * irp)
{
	printf("disk_control\n");
	return 0;
}

int
disk_query_volume_info(IRP * irp)
{
	int status;
	int size;
	char * buf;
	int len;

	LLOGLN(0, ("disk_query_volume_info: class=%d\n", irp->infoClass));
	size = 256;
	buf = malloc(size);
	memset(buf, 0, size);

	status = RD_STATUS_SUCCESS;

	switch (irp->infoClass)
	{
	case FileFsVolumeInformation:
		SET_UINT32(buf, 0, 0); /* VolumeCreationTime (low) */
		SET_UINT32(buf, 4, 0); /* VolumeCreationTime (high) */
		SET_UINT32(buf, 8, 0); /* VolumeSerialNumber */
		len = set_wstr(buf, size - 17, "FREERDP", strlen("FREERDP") + 1);
		SET_UINT32(buf, 12, len); /* VolumeLabelLength */
		SET_UINT8(buf, 16, 0);	/* SupportsObjects */
		size = 17 + len;
		break;
	default:
		size = 0;
		status = RD_STATUS_INVALID_PARAMETER;
		break;
	}

	irp->buffer = buf;
	irp->buffer_size = size;

	return status;
}

int
disk_free(DEVICE * dev)
{
	DISK_DEVICE_INFO * info;

	LLOGLN(10, ("disk_free"));
	info = (DISK_DEVICE_INFO *) dev->info;
	while (info->head)
	{
		disk_remove_file(dev, info->head->file_id);
	}
	free(info);
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
	srv->query_volume_info = disk_query_volume_info;
	srv->free = disk_free;
	srv->type = RDPDR_DTYP_FILESYSTEM;

	data = (RD_PLUGIN_DATA *) pEntryPoints->pExtendedData;
	while (data && data->size > 0)
	{
		if (strcmp((char*)data->data[0], "disk") == 0)
		{
			info = (DISK_DEVICE_INFO *) malloc(sizeof(DISK_DEVICE_INFO));
			memset(info, 0, sizeof(DISK_DEVICE_INFO));
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
