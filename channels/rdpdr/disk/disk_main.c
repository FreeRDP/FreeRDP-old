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
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <fnmatch.h>

#include "rdpdr_types.h"
#include "rdpdr_constants.h"
#include "devman.h"

struct _FILE_INFO
{
	uint32 file_id;
	struct stat file_stat;
	uint32 file_attr;
	int is_dir;
	int file;
	DIR * dir;
	struct _FILE_INFO * next;
	char pattern[256];
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
get_filetime(time_t seconds, uint32 * low, uint32 * high)
{
	unsigned long long ticks;

	ticks = (seconds + 11644473600LL) * 10000000;
	*low = (uint32) ticks;
	*high = (uint32) (ticks >> 32);
}

static int
get_error_status(void)
{
	switch (errno)
	{
	case EACCES:
	case ENOTDIR:
	case ENFILE:
		return RD_STATUS_ACCESS_DENIED;
	case EISDIR:
		return RD_STATUS_FILE_IS_A_DIRECTORY;
	case EEXIST:
		return RD_STATUS_OBJECT_NAME_COLLISION;
	default:
		return RD_STATUS_NO_SUCH_FILE;
	}
}

static uint32
get_file_attribute(const char * filename, struct stat * filestat)
{
	uint32 attr;

	attr = 0;
	if (S_ISDIR(filestat->st_mode))
		attr |= FILE_ATTRIBUTE_DIRECTORY;
	if (filename[0] == '.')
		attr |= FILE_ATTRIBUTE_HIDDEN;
	if (!attr)
		attr |= FILE_ATTRIBUTE_NORMAL;
	if (!(filestat->st_mode & S_IWUSR))
		attr |= FILE_ATTRIBUTE_READONLY;
	return attr;
}

static char *
disk_get_fullpath(DEVICE * dev, const char * path)
{
	DISK_DEVICE_INFO * info;
	char * fullpath;
	int len;
	int i;

	info = (DISK_DEVICE_INFO *) dev->info;
	fullpath = malloc(strlen(info->path) + strlen(path) + 1);
	strcpy(fullpath, info->path);
	strcat(fullpath, path);
	len = strlen(fullpath);
	for (i = 0; i < len; i++)
	{
		if (fullpath[i] == '\\')
			fullpath[i] = '/';
	}
	if (len > 0 && fullpath[len - 1] == '/')
		fullpath[len - 1] = '\0';
		
	return fullpath;
}

static uint32
disk_create_fullpath(IRP * irp, FILE_INFO * finfo, const char * fullpath)
{
	int mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
	int flags = 0;
	char * p;

	finfo->is_dir = ((irp->createOptions & FILE_DIRECTORY_FILE) ? 1 : 0);
	if (finfo->is_dir)
	{
		if (irp->createDisposition == FILE_CREATE)
		{
			if (mkdir(fullpath, mode) != 0)
				return get_error_status();
		}
		finfo->dir = opendir(fullpath);
		if (finfo->dir == NULL)
			return get_error_status();
	}
	else
	{
		switch (irp->createDisposition)
		{
		case FILE_SUPERSEDE:
			flags = O_TRUNC | O_CREAT;
			break;
		case FILE_OPEN:
			break;
		case FILE_CREATE:
			flags |= O_CREAT | O_EXCL;
			break;
		case FILE_OPEN_IF:
			flags |= O_CREAT;
			break;
		case FILE_OVERWRITE:
			flags |= O_TRUNC;
			break;
		case FILE_OVERWRITE_IF:
			flags |= O_TRUNC | O_CREAT;
			break;
		default:
			return RD_STATUS_INVALID_PARAMETER;
		}

		if (irp->desiredAccess & GENERIC_ALL
		    || ((irp->desiredAccess & GENERIC_READ) && (irp->desiredAccess & GENERIC_WRITE)))
		{
			flags |= O_RDWR;
		}
		else if ((irp->desiredAccess & GENERIC_WRITE) && !(irp->desiredAccess & GENERIC_READ))
		{
			flags |= O_WRONLY;
		}
		else
		{
			flags |= O_RDONLY;
		}

		finfo->file = open(fullpath, flags, mode);
		if (finfo->file == -1)
			return get_error_status();
		fcntl(finfo->file, F_SETFL, O_NONBLOCK);
	}

	if (stat(fullpath, &finfo->file_stat) != 0)
	{
		return RD_STATUS_NO_SUCH_FILE;
	}

	p = strrchr(fullpath, '/');
	finfo->file_attr = get_file_attribute((p ? p + 1 : fullpath), &finfo->file_stat);

	return RD_STATUS_SUCCESS;
}

static FILE_INFO *
disk_get_file_info(DEVICE * dev, uint32 file_id)
{
	DISK_DEVICE_INFO * info;
	FILE_INFO * curr;

	info = (DISK_DEVICE_INFO *) dev->info;
	for (curr = info->head; curr; curr = curr->next)
	{
		if (curr->file_id == file_id)
		{
			return curr;
		}
	}
	return NULL;
}

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

			if (curr->file != -1)
				close(curr->file);
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

uint32
disk_create(IRP * irp, const char * path)
{
	DISK_DEVICE_INFO * info;
	FILE_INFO * finfo;
	char * fullpath;
	uint32 status;

	info = (DISK_DEVICE_INFO *) irp->dev->info;
	finfo = (FILE_INFO *) malloc(sizeof(FILE_INFO));
	memset(finfo, 0, sizeof(FILE_INFO));
	finfo->file = -1;

	fullpath = disk_get_fullpath(irp->dev, path);
	status = disk_create_fullpath(irp, finfo, fullpath);
	free(fullpath);

	if (status == RD_STATUS_SUCCESS)
	{
		finfo->file_id = info->devman->id_sequence++;
		finfo->next = info->head;
		info->head = finfo;

		irp->fileID = finfo->file_id;
		LLOGLN(0, ("disk_create: %s (id=%d)", path, finfo->file_id));
	}
	else
	{
		free(finfo);
	}

	return status;
}

uint32
disk_close(IRP * irp)
{
	LLOGLN(0, ("disk_close: id=%d", irp->fileID));
	disk_remove_file(irp->dev, irp->fileID);
	return RD_STATUS_SUCCESS;
}

uint32
disk_read(IRP * irp)
{
	printf("disk_read\n");
	return 0;
}

uint32
disk_write(IRP * irp)
{
	printf("disk_write\n");
	return 0;
}

uint32
disk_control(IRP * irp)
{
	LLOGLN(0, ("disk_control: id=%d io=%X", irp->fileID, irp->ioControlCode));
	return RD_STATUS_SUCCESS;
}

uint32
disk_query_volume_info(IRP * irp)
{
	uint32 status;
	int size;
	char * buf;
	int len;

	LLOGLN(0, ("disk_query_volume_info: class=%d id=%d", irp->infoClass, irp->fileID));
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
		len = set_wstr(buf + 17, size - 17, "FREERDP", strlen("FREERDP") + 1);
		SET_UINT32(buf, 12, len); /* VolumeLabelLength */
		SET_UINT8(buf, 16, 0);	/* SupportsObjects */
		size = 17 + len;
		break;
	default:
		size = 0;
		status = RD_STATUS_INVALID_PARAMETER;
		break;
	}

	irp->outputBuffer = buf;
	irp->outputBufferLength = size;

	return status;
}

uint32
disk_query_info(IRP * irp)
{
	FILE_INFO *finfo;
	uint32 status;
	int size;
	char * buf;
	uint32 tlow;
	uint32 thigh;

	LLOGLN(0, ("disk_query_info: class=%d id=%d", irp->infoClass, irp->fileID));
	finfo = disk_get_file_info(irp->dev, irp->fileID);
	if (finfo == NULL)
		return RD_STATUS_INVALID_HANDLE;

	size = 256;
	buf = malloc(size);
	memset(buf, 0, size);

	status = RD_STATUS_SUCCESS;

	switch (irp->infoClass)
	{
	case FileBasicInformation:
		get_filetime((finfo->file_stat.st_ctime < finfo->file_stat.st_mtime ?
			finfo->file_stat.st_ctime : finfo->file_stat.st_mtime), &tlow, &thigh);
		SET_UINT32(buf, 0, tlow); /* CreationTime */
		SET_UINT32(buf, 4, thigh);
		get_filetime(finfo->file_stat.st_atime, &tlow, &thigh);
		SET_UINT32(buf, 8, tlow); /* LastAccessTime */
		SET_UINT32(buf, 12, thigh);
		get_filetime(finfo->file_stat.st_mtime, &tlow, &thigh);
		SET_UINT32(buf, 16, tlow); /* LastWriteTime */
		SET_UINT32(buf, 20, thigh);
		get_filetime(finfo->file_stat.st_ctime, &tlow, &thigh);
		SET_UINT32(buf, 24, tlow); /* ChangeTime */
		SET_UINT32(buf, 28, thigh);
		SET_UINT32(buf, 32, finfo->file_attr); /* FileAttributes */
		size = 36;
		break;

	case FileStandardInformation:
		SET_UINT32(buf, 0, finfo->file_stat.st_size); /* AllocationSize */
		SET_UINT32(buf, 4, 0);
		SET_UINT32(buf, 8, finfo->file_stat.st_size); /* EndOfFile */
		SET_UINT32(buf, 12, 0);
		SET_UINT32(buf, 16, finfo->file_stat.st_nlink); /* NumberOfLinks */
		SET_UINT8(buf, 20, 0); /* DeletePending */
		SET_UINT8(buf, 21, finfo->is_dir); /* Directory */
		size = 22;
		break;

	case FileObjectIdInformation:
		SET_UINT32(buf, 0, finfo->file_attr); /* FileAttributes */
		SET_UINT32(buf, 4, 0);	/* ReparseTag */
		size = 8;
		break;

	default:
		size = 0;
		status = RD_STATUS_INVALID_PARAMETER;
		break;
	}

	irp->outputBuffer = buf;
	irp->outputBufferLength = size;

	return status;
}

uint32
disk_query_directory(IRP * irp, uint8 initialQuery, const char * path)
{
	DISK_DEVICE_INFO * info;
	FILE_INFO *finfo;
	char * p;
	uint32 status;
	char * buf;
	int size;
	int len;
	struct dirent * pdirent;
	struct stat file_stat;
	uint32 attr;
	uint32 tlow;
	uint32 thigh;

	LLOGLN(0, ("disk_query_directory: class=%d id=%d init=%d path=%s", irp->infoClass, irp->fileID,
		initialQuery, path));
	finfo = disk_get_file_info(irp->dev, irp->fileID);
	if (finfo == NULL || finfo->dir == NULL)
		return RD_STATUS_INVALID_HANDLE;
	info = (DISK_DEVICE_INFO *) irp->dev->info;

	if (initialQuery)
	{
		p = strrchr(path, '\\');
		strncpy(finfo->pattern, (p ? p + 1 : path), sizeof(finfo->pattern) - 1);
		rewinddir(finfo->dir);
	}

	status = RD_STATUS_SUCCESS;
	buf = NULL;
	size = 0;

	switch (irp->infoClass)
	{
	case FileBothDirectoryInformation:
		pdirent = readdir(finfo->dir);
		while (pdirent && finfo->pattern[0] && fnmatch(finfo->pattern, pdirent->d_name, 0) != 0)
			pdirent = readdir(finfo->dir);
		if (pdirent == NULL)
		{
			status = RD_STATUS_NO_MORE_FILES;
			/* [MS-RDPEFS] said it's an optional padding, however it's *required* for this last query!!! */
			buf = malloc(1);
			buf[0] = 0;
			size = 1;
			break;
		}		

		p = malloc(strlen(info->path) + strlen(pdirent->d_name) + 2);
		sprintf(p, "%s/%s", info->path, pdirent->d_name);
		stat(p, &file_stat);
		free(p);

		attr = get_file_attribute(pdirent->d_name, &file_stat);

		size = 93 + strlen(pdirent->d_name) * 2;
		buf = malloc(size);
		memset(buf, 0, size);

		SET_UINT32(buf, 0, 0); /* NextEntryOffset */
		SET_UINT32(buf, 4, 0); /* FileIndex */
		get_filetime((file_stat.st_ctime < file_stat.st_mtime ?
			file_stat.st_ctime : file_stat.st_mtime), &tlow, &thigh);
		SET_UINT32(buf, 8, tlow); /* CreationTime */
		SET_UINT32(buf, 12, thigh);
		get_filetime(file_stat.st_atime, &tlow, &thigh);
		SET_UINT32(buf, 16, tlow); /* LastAccessTime */
		SET_UINT32(buf, 20, thigh);
		get_filetime(file_stat.st_mtime, &tlow, &thigh);
		SET_UINT32(buf, 24, tlow); /* LastWriteTime */
		SET_UINT32(buf, 28, thigh);
		get_filetime(file_stat.st_ctime, &tlow, &thigh);
		SET_UINT32(buf, 32, tlow); /* ChangeTime */
		SET_UINT32(buf, 36, thigh);
		SET_UINT32(buf, 40, file_stat.st_size); /* EndOfFile */
		SET_UINT32(buf, 44, 0);
		SET_UINT32(buf, 48, file_stat.st_size); /* AllocationSize */
		SET_UINT32(buf, 52, 0);
		SET_UINT32(buf, 56, attr); /* FileAttributes */
		SET_UINT32(buf, 64, 0); /* EaSize */
		SET_UINT8(buf, 68, 0); /* ShortNameLength */
		/* [MS-FSCC] has one byte padding here but RDP does not! */
		//SET_UINT8(buf, 69, 0); /* Reserved */
		/* ShortName 24  bytes */
		len = set_wstr(buf + 93, size - 93, pdirent->d_name, strlen(pdirent->d_name));
		SET_UINT32(buf, 60, len); /* FileNameLength */
		size = 93 + len;
		break;

	default:
		status = RD_STATUS_INVALID_PARAMETER;
		break;
	}

	irp->outputBuffer = buf;
	irp->outputBufferLength = size;

	return status;
}

uint32
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
	srv->query_info = disk_query_info;
	srv->query_directory = disk_query_directory;
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
