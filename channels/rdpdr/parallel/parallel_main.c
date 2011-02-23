/*
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Parallel Port Device Service

   Copyright 2010 O.S. Systems Software Ltda.
   Copyright 2010 Eduardo Fiss Beloni <beloni@ossystems.com.br>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <strings.h>
#include <sys/ioctl.h>

#include "rdpdr_types.h"
#include "rdpdr_constants.h"
#include "devman.h"

#define	MAX_PARALLEL_DEVICES               1
#define FILE_DEVICE_PARALLEL               0x22

#define IOCTL_PAR_QUERY_INFORMATION        0x00160004
#define IOCTL_PAR_SET_INFORMATION          0x00160008
#define IOCTL_PAR_QUERY_DEVICE_ID          0x0016000C
#define IOCTL_PAR_QUERY_DEVICE_ID_SIZE     0x00160010
#define IOCTL_PAR_SET_WRITE_ADDRESS        0x0016001C
#define IOCTL_PAR_SET_READ_ADDRESS         0x00160020
#define IOCTL_PAR_GET_DEVICE_CAPS          0x00160024
#define IOCTL_PAR_GET_DEFAULT_MODES        0x00160028
#define IOCTL_PAR_QUERY_RAW_DEVICE_ID      0x00160030
#define IOCTL_PAR_IS_PORT_FREE             0x00160054


struct _PARALLEL_DEVICE_INFO
{
	PDEVMAN devman;

	PDEVMAN_REGISTER_SERVICE DevmanRegisterService;
	PDEVMAN_UNREGISTER_SERVICE DevmanUnregisterService;
	PDEVMAN_REGISTER_DEVICE DevmanRegisterDevice;
	PDEVMAN_UNREGISTER_DEVICE DevmanUnregisterDevice;

	int file;
	char * path;

/*	char *driver, *printer;*/
/*	uint32 queue_in_size,*/
/*		queue_out_size,*/
/*		wait_mask,*/
/*		read_interval_timeout,*/
/*		read_total_timeout_multiplier,*/
/*		read_total_timeout_constant,*/
/*		write_total_timeout_multiplier,*/
/*		write_total_timeout_constant, posix_wait_mask, bloblen;*/
/*	uint8 *blob;*/
};
typedef struct _PARALLEL_DEVICE_INFO PARALLEL_DEVICE_INFO;

static int
get_error_status(void)
{
	switch (errno)
	{
		case EAGAIN:
			return RD_STATUS_DEVICE_OFF_LINE;
		case ENOSPC:
			return RD_STATUS_DEVICE_PAPER_EMPTY;
		case EIO:
			return RD_STATUS_DEVICE_OFF_LINE;
		default:
			return RD_STATUS_DEVICE_POWERED_OFF;
	}
}

static int
parallel_get_fd(IRP * irp)
{
	return 	((PARALLEL_DEVICE_INFO *) irp->dev->info)->file;
}

static uint32
parallel_control(IRP * irp)
{
	int size = 0, ret = RD_STATUS_SUCCESS;
	char *outbuf = NULL;

	LLOGLN(10, ("parallel_control: id=%d io=%X", irp->fileID, irp->ioControlCode));

	switch (irp->ioControlCode)
	{
		case IOCTL_PAR_QUERY_INFORMATION:

		case IOCTL_PAR_SET_INFORMATION:

		case IOCTL_PAR_QUERY_DEVICE_ID:

		case IOCTL_PAR_QUERY_DEVICE_ID_SIZE:

		case IOCTL_PAR_SET_WRITE_ADDRESS:

		case IOCTL_PAR_SET_READ_ADDRESS:

		case IOCTL_PAR_GET_DEVICE_CAPS:

		case IOCTL_PAR_GET_DEFAULT_MODES:

		case IOCTL_PAR_QUERY_RAW_DEVICE_ID:

		case IOCTL_PAR_IS_PORT_FREE:

		default:
			LLOGLN(10, ("NOT FOUND IoControlCode PARALLEL IOCTL %d", irp->ioControlCode));
			return RD_STATUS_INVALID_PARAMETER;
	}

	irp->outputBuffer = outbuf;
	irp->outputBufferLength = size;

	return ret;
}

static uint32
parallel_read(IRP * irp)
{
	PARALLEL_DEVICE_INFO *info;
	char *buf;
	ssize_t r;

	info = (PARALLEL_DEVICE_INFO *) irp->dev->info;
	buf = malloc(irp->length);
	memset(buf, 0, irp->length);

	r = read(info->file, buf, irp->length);
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
parallel_write(IRP * irp)
{
	PARALLEL_DEVICE_INFO * info;
	ssize_t r;
	uint32 len;

	info = (PARALLEL_DEVICE_INFO *) irp->dev->info;

	len = 0;
	while (len < irp->inputBufferLength)
	{
		r = write(info->file, irp->inputBuffer, irp->inputBufferLength);
		if (r == -1)
			return get_error_status();

		len += r;
	}
	LLOGLN(10, ("parallel_write: id=%d len=%d off=%lld", irp->fileID, irp->inputBufferLength, irp->offset));
	return RD_STATUS_SUCCESS;
}

static uint32
parallel_free(DEVICE * dev)
{
	LLOGLN(10, ("parallel_free"));

	free(dev->info);
	if (dev->data)
	{
		free(dev->data);
		dev->data = NULL;
	}
	return 0;
}

static uint32
parallel_create(IRP * irp, const char * path)
{
	PARALLEL_DEVICE_INFO *info;

	info = (PARALLEL_DEVICE_INFO *) irp->dev->info;

	info->file = open(info->path, O_RDWR);
	if (info->file == -1)
	{
		perror("parallel open");
		return RD_STATUS_ACCESS_DENIED;
	}

	/* all read and writes should be non blocking */
	if (fcntl(info->file, F_SETFL, O_NONBLOCK) == -1)
		perror("fcntl");
	LLOGLN(10, ("parallel create"));

/*	info->read_total_timeout_constant = 5;*/
	return RD_STATUS_SUCCESS;
}

static uint32
parallel_close(IRP * irp)
{
	PARALLEL_DEVICE_INFO *info = (PARALLEL_DEVICE_INFO *) irp->dev->info;

	close(info->file);

	LLOGLN(10, ("parallel_close: id=%d", irp->fileID));
	return RD_STATUS_SUCCESS;
}

static SERVICE *
parallel_register_service(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints)
{
	SERVICE * srv;

	srv = pEntryPoints->pDevmanRegisterService(pDevman);

	srv->create = parallel_create;
	srv->close = parallel_close;
	srv->read = parallel_read;
	srv->write = parallel_write;
	srv->control = parallel_control;
	srv->query_volume_info = NULL;
	srv->query_info = NULL;
	srv->set_info = NULL;
	srv->query_directory = NULL;
	srv->notify_change_directory = NULL;
	srv->lock_control = NULL;
	srv->free = parallel_free;
	srv->type = RDPDR_DTYP_PARALLEL;
	srv->get_event = NULL;
	srv->file_descriptor = parallel_get_fd;
	srv->get_timeouts = NULL;

	return srv;
}

int
DeviceServiceEntry(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints)
{
	SERVICE * srv = NULL;
	DEVICE * dev;
	PARALLEL_DEVICE_INFO * info;
	RD_PLUGIN_DATA * data;
	int i;

	data = (RD_PLUGIN_DATA *) pEntryPoints->pExtendedData;
	while (data && data->size > 0)
	{
		if (strcmp((char*)data->data[0], "parallel") == 0)
		{
			if (srv == NULL)
				srv = parallel_register_service(pDevman, pEntryPoints);

			info = (PARALLEL_DEVICE_INFO *) malloc(sizeof(PARALLEL_DEVICE_INFO));
			memset(info, 0, sizeof(PARALLEL_DEVICE_INFO));
			info->devman = pDevman;
			info->DevmanRegisterService = pEntryPoints->pDevmanRegisterService;
			info->DevmanUnregisterService = pEntryPoints->pDevmanUnregisterService;
			info->DevmanRegisterDevice = pEntryPoints->pDevmanRegisterDevice;
			info->DevmanUnregisterDevice = pEntryPoints->pDevmanUnregisterDevice;
			info->path = (char *) data->data[2];

			dev = info->DevmanRegisterDevice(pDevman, srv, (char*)data->data[1]);
			dev->info = info;

			/* [MS-RDPEFS] 2.2.3.1 said this is a unicode string, however, only ASCII works.
			   Any non-ASCII characters simply screw up the whole channel. Long name is supported though.
			   This is yet to be investigated. */
			dev->data_len = strlen(dev->name) + 1;
			dev->data = strdup(dev->name);
			for (i = 0; i < dev->data_len; i++)
			{
				if (dev->data[i] < 0)
				{
					dev->data[i] = '_';
				}
			}
		}
		data = (RD_PLUGIN_DATA *) (((void *) data) + data->size);
	}

	return 1;
}
