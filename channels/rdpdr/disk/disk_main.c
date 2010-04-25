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

#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif

#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#include "rdpdr_types.h"
#include "rdpdr_constants.h"
#include "devman.h"

#ifdef STAT_STATFS3_OSF1
#define STATFS_FN(path, buf) (statfs(path,buf,sizeof(buf)))
#define STATFS_T statfs
#define USE_STATFS
#endif

#ifdef STAT_STATVFS
#define STATFS_FN(path, buf) (statvfs(path,buf))
#define STATFS_T statvfs
#define USE_STATVFS
#endif

#ifdef STAT_STATVFS64
#define STATFS_FN(path, buf) (statvfs64(path,buf))
#define STATFS_T statvfs64
#define USE_STATVFS
#endif

#if (defined(STAT_STATFS2_FS_DATA) || defined(STAT_STATFS2_BSIZE) || defined(STAT_STATFS2_FSIZE))
#define STATFS_FN(path, buf) (statfs(path,buf))
#define STATFS_T statfs
#define USE_STATFS
#endif

#ifdef STAT_STATFS4
#define STATFS_FN(path, buf) (statfs(path,buf,sizeof(buf),0))
#define STATFS_T statfs
#define USE_STATFS
#endif

#if ((defined(USE_STATFS) && defined(HAVE_STRUCT_STATFS_F_NAMEMAX)) || (defined(USE_STATVFS) && defined(HAVE_STRUCT_STATVFS_F_NAMEMAX)))
#define F_NAMELEN(buf) ((buf).f_namemax)
#endif

#if ((defined(USE_STATFS) && defined(HAVE_STRUCT_STATFS_F_NAMELEN)) || (defined(USE_STATVFS) && defined(HAVE_STRUCT_STATVFS_F_NAMELEN)))
#define F_NAMELEN(buf) ((buf).f_namelen)
#endif

#ifndef F_NAMELEN
#define F_NAMELEN(buf) (255)
#endif

/* Dummy statfs fallback */
#ifndef STATFS_T
struct dummy_statfs_t
{
	long f_bfree;
	long f_bsize;
	long f_blocks;
	int f_namelen;
	int f_namemax;
};

static int
dummy_statfs(struct dummy_statfs_t *buf)
{
	buf->f_blocks = 262144;
	buf->f_bfree = 131072;
	buf->f_bsize = 512;
	buf->f_namelen = 255;
	buf->f_namemax = 255;

	return 0;
}

#define STATFS_T dummy_statfs_t
#define STATFS_FN(path,buf) (dummy_statfs(buf))
#endif

struct _FILE_INFO
{
	uint32 file_id;
	struct stat file_stat;
	uint32 file_attr;
	int is_dir;
	int file;
	DIR * dir;
	struct _FILE_INFO * next;
	char * fullpath;
	char * pattern;
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

static uint64
get_filetime(time_t seconds)
{
	return ((uint64)seconds + 11644473600LL) * 10000000LL;
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
		case EBADF:
			return RD_STATUS_INVALID_HANDLE;
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
	struct stat file_stat;

	if (stat(fullpath, &file_stat) == 0)
	{
		finfo->is_dir = S_ISDIR(file_stat.st_mode);
	}
	else
	{
		finfo->is_dir = ((irp->createOptions & FILE_DIRECTORY_FILE) ? 1 : 0);
	}
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

		if ((irp->desiredAccess & GENERIC_ALL)
		    || (irp->desiredAccess & GENERIC_WRITE)
			|| (irp->desiredAccess & FILE_WRITE_DATA)
			|| (irp->desiredAccess & FILE_APPEND_DATA))
		{
			flags |= O_RDWR;
		}
		else
		{
			flags |= O_RDONLY;
		}

		finfo->file = open(fullpath, flags, mode);
		if (finfo->file == -1)
			return get_error_status();
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
			if (curr->fullpath)
				free(curr->fullpath);
			if (curr->pattern)
				free(curr->pattern);

			if (prev == NULL)
				info->head = curr->next;
			else
				prev->next  = curr->next;

			free(curr);
			break;
		}
	}
}

static uint32
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

	if (status == RD_STATUS_SUCCESS)
	{
		finfo->fullpath = fullpath;
		finfo->file_id = info->devman->id_sequence++;
		finfo->next = info->head;
		info->head = finfo;

		irp->fileID = finfo->file_id;
		LLOGLN(0, ("disk_create: %s (id=%d)", path, finfo->file_id));
	}
	else
	{
		free(fullpath);
		free(finfo);
	}

	return status;
}

static uint32
disk_close(IRP * irp)
{
	LLOGLN(0, ("disk_close: id=%d", irp->fileID));
	disk_remove_file(irp->dev, irp->fileID);
	return RD_STATUS_SUCCESS;
}

static uint32
disk_read(IRP * irp)
{
	FILE_INFO * finfo;
	char * buf;
	ssize_t r;

	LLOGLN(0, ("disk_read: id=%d len=%d off=%lld", irp->fileID, irp->length, irp->offset));
	finfo = disk_get_file_info(irp->dev, irp->fileID);
	if (finfo == NULL)
	{
		LLOGLN(0, ("disk_read: invalid file id"));
		return RD_STATUS_INVALID_HANDLE;
	}
	if (finfo->is_dir)
		return RD_STATUS_FILE_IS_A_DIRECTORY;
	if (finfo->file == -1)
		return RD_STATUS_INVALID_HANDLE;

	if (lseek(finfo->file, irp->offset, SEEK_SET) == (off_t) - 1)
		return get_error_status();

	buf = malloc(irp->length);
	memset(buf, 0, irp->length);
	r = read(finfo->file, buf, irp->length);
	if (r == -1)
	{
		free(buf);
		return get_error_status();
	}
	else
	{
		irp->outputBuffer = buf;
		irp->outputBufferLength = r;
		return RD_STATUS_SUCCESS;
	}
}

static uint32
disk_write(IRP * irp)
{
	FILE_INFO * finfo;
	ssize_t r;
	uint32 len;

	LLOGLN(0, ("disk_write: id=%d len=%d off=%lld", irp->fileID, irp->inputBufferLength, irp->offset));
	finfo = disk_get_file_info(irp->dev, irp->fileID);
	if (finfo == NULL)
	{
		LLOGLN(0, ("disk_read: invalid file id"));
		return RD_STATUS_INVALID_HANDLE;
	}
	if (finfo->is_dir)
		return RD_STATUS_FILE_IS_A_DIRECTORY;
	if (finfo->file == -1)
		return RD_STATUS_INVALID_HANDLE;

	if (lseek(finfo->file, irp->offset, SEEK_SET) == (off_t) - 1)
		return get_error_status();

	len = 0;
	while (len < irp->inputBufferLength)
	{
		r = write(finfo->file, irp->inputBuffer, irp->inputBufferLength);
		if (r == -1)
		{
			return get_error_status();
		}
		len += r;
	}
	return RD_STATUS_SUCCESS;
}

static uint32
disk_control(IRP * irp)
{
	LLOGLN(0, ("disk_control: id=%d io=%X", irp->fileID, irp->ioControlCode));
	return RD_STATUS_SUCCESS;
}

static uint32
disk_query_volume_info(IRP * irp)
{
	FILE_INFO * finfo;
	struct STATFS_T stat_fs;
	uint32 status;
	int size;
	char * buf;
	int len;

	LLOGLN(0, ("disk_query_volume_info: class=%d id=%d", irp->infoClass, irp->fileID));
	finfo = disk_get_file_info(irp->dev, irp->fileID);
	if (finfo == NULL)
	{
		LLOGLN(0, ("disk_query_volume_info: invalid file id"));
		return RD_STATUS_INVALID_HANDLE;
	}
	if (STATFS_FN(finfo->fullpath, &stat_fs) != 0)
	{
		LLOGLN(0, ("disk_query_volume_info: statfs failed"));
		return RD_STATUS_ACCESS_DENIED;
	}

	size = 0;
	buf = NULL;
	status = RD_STATUS_SUCCESS;

	switch (irp->infoClass)
	{
		case FileFsVolumeInformation:
			buf = malloc(256);
			memset(buf, 0, 256);
			SET_UINT64(buf, 0, 0); /* VolumeCreationTime */
			SET_UINT32(buf, 8, 0); /* VolumeSerialNumber */
			len = set_wstr(buf + 17, size - 17, "FREERDP", strlen("FREERDP") + 1);
			SET_UINT32(buf, 12, len); /* VolumeLabelLength */
			SET_UINT8(buf, 16, 0);	/* SupportsObjects */
			size = 17 + len;
			break;

		case FileFsSizeInformation:
			size = 24;
			buf = malloc(size);
			memset(buf, 0, size);
			SET_UINT64(buf, 0, stat_fs.f_blocks); /* TotalAllocationUnits */
			SET_UINT64(buf, 8, stat_fs.f_bfree); /* AvailableAllocationUnits */
			SET_UINT32(buf, 16, stat_fs.f_bsize / 0x200); /* SectorsPerAllocationUnit */
			SET_UINT32(buf, 20, 0x200); /* BytesPerSector */
			break;

		case FileFsAttributeInformation:
			buf = malloc(256);
			memset(buf, 0, 256);
			SET_UINT32(buf, 0, FILE_CASE_SENSITIVE_SEARCH | FILE_CASE_PRESERVED_NAMES | FILE_UNICODE_ON_DISK); /* FileSystemAttributes */
			SET_UINT32(buf, 4, F_NAMELEN(stat_fs)); /* MaximumComponentNameLength */
			len = set_wstr(buf + 12, 256 - 12, "FREERDP", 8);
			SET_UINT32(buf, 8, len); /* FileSystemNameLength */
			size = 12 + len;
			break;

		case FileFsFullSizeInformation:
			size = 32;
			buf = malloc(size);
			memset(buf, 0, size);
			SET_UINT64(buf, 0, stat_fs.f_blocks); /* TotalAllocationUnits */
			SET_UINT64(buf, 8, stat_fs.f_bfree); /* CallerAvailableAllocationUnits */
			SET_UINT64(buf, 16, stat_fs.f_bfree); /* ActualAvailableAllocationUnits */
			SET_UINT32(buf, 24, stat_fs.f_bsize / 0x200); /* SectorsPerAllocationUnit */
			SET_UINT32(buf, 28, 0x200); /* BytesPerSector */
			break;

		case FileFsDeviceInformation:
			size = 8;
			buf = malloc(size);
			memset(buf, 0, size);
			SET_UINT32(buf, 0, FILE_DEVICE_DISK); /* DeviceType */
			SET_UINT32(buf, 4, 0); /* BytesPerSector */
			break;

		default:
			LLOGLN(0, ("disk_query_volume_info: invalid info class"));
			status = RD_STATUS_NOT_SUPPORTED;
			break;
	}

	irp->outputBuffer = buf;
	irp->outputBufferLength = size;

	return status;
}

static uint32
disk_query_info(IRP * irp)
{
	FILE_INFO *finfo;
	uint32 status;
	int size;
	char * buf;

	LLOGLN(0, ("disk_query_info: class=%d id=%d", irp->infoClass, irp->fileID));
	finfo = disk_get_file_info(irp->dev, irp->fileID);
	if (finfo == NULL)
	{
		LLOGLN(0, ("disk_query_info: invalid file id"));
		return RD_STATUS_INVALID_HANDLE;
	}

	size = 256;
	buf = malloc(size);
	memset(buf, 0, size);

	status = RD_STATUS_SUCCESS;

	switch (irp->infoClass)
	{
		case FileBasicInformation:
			SET_UINT64(buf, 0, get_filetime(finfo->file_stat.st_ctime < finfo->file_stat.st_mtime ?
				finfo->file_stat.st_ctime : finfo->file_stat.st_mtime)); /* CreationTime */
			SET_UINT64(buf, 8, get_filetime(finfo->file_stat.st_atime)); /* LastAccessTime */
			SET_UINT64(buf, 16, get_filetime(finfo->file_stat.st_mtime)); /* LastWriteTime */
			SET_UINT64(buf, 24, get_filetime(finfo->file_stat.st_ctime)); /* ChangeTime */
			SET_UINT32(buf, 32, finfo->file_attr); /* FileAttributes */
			size = 36;
			break;

		case FileStandardInformation:
			SET_UINT64(buf, 0, finfo->file_stat.st_size); /* AllocationSize */
			SET_UINT64(buf, 8, finfo->file_stat.st_size); /* EndOfFile */
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
			LLOGLN(0, ("disk_query_info: invalid info class"));
			size = 0;
			status = RD_STATUS_NOT_SUPPORTED;
			break;
	}

	irp->outputBuffer = buf;
	irp->outputBufferLength = size;

	return status;
}

static uint32
disk_query_directory(IRP * irp, uint8 initialQuery, const char * path)
{
	DISK_DEVICE_INFO * info;
	FILE_INFO * finfo;
	char * p;
	uint32 status;
	char * buf;
	int size;
	int len;
	struct dirent * pdirent;
	struct stat file_stat;
	uint32 attr;

	LLOGLN(0, ("disk_query_directory: class=%d id=%d init=%d path=%s", irp->infoClass, irp->fileID,
		initialQuery, path));
	finfo = disk_get_file_info(irp->dev, irp->fileID);
	if (finfo == NULL || finfo->dir == NULL)
	{
		LLOGLN(0, ("disk_query_directory: invalid file id"));
		return RD_STATUS_INVALID_HANDLE;
	}
	info = (DISK_DEVICE_INFO *) irp->dev->info;

	if (initialQuery)
	{
		if (finfo->pattern)
			free(finfo->pattern);
		p = strrchr(path, '\\');
		p = (p ? p + 1 : (char *)path);
		finfo->pattern = malloc(strlen(p) + 1);
		strcpy(finfo->pattern, p);
		rewinddir(finfo->dir);
	}

	status = RD_STATUS_SUCCESS;
	buf = NULL;
	size = 0;

	pdirent = readdir(finfo->dir);
	while (pdirent && finfo->pattern[0] && fnmatch(finfo->pattern, pdirent->d_name, 0) != 0)
		pdirent = readdir(finfo->dir);
	if (pdirent == NULL)
	{
		return RD_STATUS_NO_MORE_FILES;
	}		

	memset(&file_stat, 0, sizeof(struct stat));
	p = malloc(strlen(finfo->fullpath) + strlen(pdirent->d_name) + 2);
	sprintf(p, "%s/%s", finfo->fullpath, pdirent->d_name);
	if (stat(p, &file_stat) != 0)
	{
		LLOGLN(0, ("disk_query_directory: stat %s failed (%i)\n", p, errno));
	}
	free(p);

	attr = get_file_attribute(pdirent->d_name, &file_stat);

	switch (irp->infoClass)
	{
		case FileBothDirectoryInformation:
			size = 93 + strlen(pdirent->d_name) * 2;
			buf = malloc(size);
			memset(buf, 0, size);

			SET_UINT32(buf, 0, 0); /* NextEntryOffset */
			SET_UINT32(buf, 4, 0); /* FileIndex */
			SET_UINT64(buf, 8, get_filetime(file_stat.st_ctime < file_stat.st_mtime ?
				file_stat.st_ctime : file_stat.st_mtime)); /* CreationTime */
			SET_UINT64(buf, 16, get_filetime(file_stat.st_atime)); /* LastAccessTime */
			SET_UINT64(buf, 24, get_filetime(file_stat.st_mtime)); /* LastWriteTime */
			SET_UINT64(buf, 32, get_filetime(file_stat.st_ctime)); /* ChangeTime */
			SET_UINT64(buf, 40, file_stat.st_size); /* EndOfFile */
			SET_UINT64(buf, 48, file_stat.st_size); /* AllocationSize */
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

		case FileFullDirectoryInformation:
			size = 68 + strlen(pdirent->d_name) * 2;
			buf = malloc(size);
			memset(buf, 0, size);

			SET_UINT32(buf, 0, 0); /* NextEntryOffset */
			SET_UINT32(buf, 4, 0); /* FileIndex */
			SET_UINT64(buf, 8, get_filetime(file_stat.st_ctime < file_stat.st_mtime ?
				file_stat.st_ctime : file_stat.st_mtime)); /* CreationTime */
			SET_UINT64(buf, 16, get_filetime(file_stat.st_atime)); /* LastAccessTime */
			SET_UINT64(buf, 24, get_filetime(file_stat.st_mtime)); /* LastWriteTime */
			SET_UINT64(buf, 32, get_filetime(file_stat.st_ctime)); /* ChangeTime */
			SET_UINT64(buf, 40, file_stat.st_size); /* EndOfFile */
			SET_UINT64(buf, 48, file_stat.st_size); /* AllocationSize */
			SET_UINT32(buf, 56, attr); /* FileAttributes */
			SET_UINT32(buf, 64, 0); /* EaSize */
			len = set_wstr(buf + 68, size - 68, pdirent->d_name, strlen(pdirent->d_name));
			SET_UINT32(buf, 60, len); /* FileNameLength */
			size = 68 + len;
			break;

		default:
			LLOGLN(0, ("disk_query_directory: invalid info class"));
			status = RD_STATUS_NOT_SUPPORTED;
			break;
	}

	irp->outputBuffer = buf;
	irp->outputBufferLength = size;

	return status;
}

static uint32
disk_notify_change_directory(IRP * irp)
{
	return RD_STATUS_PENDING;
}

static uint32
disk_lock_control(IRP * irp)
{
	return RD_STATUS_SUCCESS;
}

static uint32
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
	srv->notify_change_directory = disk_notify_change_directory;
	srv->lock_control = disk_lock_control;
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
