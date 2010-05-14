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
#include <pthread.h>
#include <time.h>
#include <cups/cups.h>
#include "rdpdr_constants.h"
#include "rdpdr_types.h"
#include "devman.h"
#include "chan_stream.h"
#include "printer_main.h"

struct _PRINTER_DEVICE_INFO
{
	char * printer_name;

	http_t * printjob_http_t;
	int printjob_id;
};
typedef struct _PRINTER_DEVICE_INFO PRINTER_DEVICE_INFO;

int
printer_hw_register_auto(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints, SERVICE * srv,
	int * port)
{
	cups_dest_t *dests;
	cups_dest_t *dest;
	int num_dests;
	int i;

	num_dests = cupsGetDests(&dests);
	for (i = 0, dest = dests; i < num_dests; i++, dest++)
	{
		if (dest->instance == NULL)
		{
			printer_register(pDevman, pEntryPoints, srv, dest->name, NULL, dest->is_default, port);
		}
	}
	cupsFreeDests(num_dests, dests);
	return 0;
}

void *
printer_hw_new(const char * name)
{
	PRINTER_DEVICE_INFO * info;

	info = (PRINTER_DEVICE_INFO *) malloc(sizeof(PRINTER_DEVICE_INFO));
	memset(info, 0, sizeof(PRINTER_DEVICE_INFO));

	info->printer_name = strdup(name);

	return info;
}

uint32
printer_hw_create(IRP * irp, const char * path)
{
	PRINTER_DEVICE_INFO * info;
	time_t tt;
	struct tm * t;
	char buf[100];

	info = (PRINTER_DEVICE_INFO *) irp->dev->info;

	/* Server's print queue will ensure no two print jobs will be sent to the same printer.
	   However, we still want to do a simple locking just to ensure we are safe. */
	if (info->printjob_http_t)
	{
		return RD_STATUS_DEVICE_BUSY;
	}
	info->printjob_http_t = httpConnectEncrypt(cupsServer(), ippPort(), HTTP_ENCRYPT_IF_REQUESTED);
	if (info->printjob_http_t == NULL)
	{
		LLOGLN(0, ("printer_create: httpConnectEncrypt: %s", cupsLastErrorString()));
		return RD_STATUS_DEVICE_BUSY;
	}

	tt = time(NULL);
	t = localtime(&tt);
	snprintf(buf, sizeof(buf) - 1, "FreeRDP Print Job %d%02d%02d%02d%02d%02d",
		t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec);
	info->printjob_id = cupsCreateJob(info->printjob_http_t,
		info->printer_name, buf,
		0, NULL);

	LLOGLN(10, ("printe_create: %s id=%d", info->printer_name, irp->fileID));

	if (info->printjob_id == 0)
	{
		LLOGLN(0, ("printer_create: cupsCreateJob: %s", cupsLastErrorString()));
		httpClose(info->printjob_http_t);
		info->printjob_http_t = NULL;
		/* Should get the right return code based on printer status */
		return RD_STATUS_DEVICE_BUSY;
	}
	cupsStartDocument(info->printjob_http_t,
		info->printer_name, info->printjob_id, buf,
		CUPS_FORMAT_POSTSCRIPT, 1);

	irp->fileID = info->printjob_id;

	return RD_STATUS_SUCCESS;
}

uint32
printer_hw_close(IRP * irp)
{
	PRINTER_DEVICE_INFO * info;

	info = (PRINTER_DEVICE_INFO *) irp->dev->info;
	LLOGLN(10, ("printe_close: %s id=%d", info->printer_name, irp->fileID));

	if (irp->fileID != info->printjob_id)
	{
		LLOGLN(0, ("printer_write: invalid file id"));
		return RD_STATUS_INVALID_HANDLE;
	}

	cupsFinishDocument(info->printjob_http_t, info->printer_name);
	info->printjob_id = 0;
	httpClose(info->printjob_http_t);
	info->printjob_http_t = NULL;

	return RD_STATUS_SUCCESS;
}

uint32
printer_hw_write(IRP * irp)
{
	PRINTER_DEVICE_INFO * info;

	info = (PRINTER_DEVICE_INFO *) irp->dev->info;
	LLOGLN(10, ("printe_write: %s id=%d len=%d off=%lld", info->printer_name,
		irp->fileID, irp->inputBufferLength, irp->offset));
	if (irp->fileID != info->printjob_id)
	{
		LLOGLN(0, ("printer_write: invalid file id"));
		return RD_STATUS_INVALID_HANDLE;
	}
	cupsWriteRequestData(info->printjob_http_t, irp->inputBuffer, irp->inputBufferLength);
	return RD_STATUS_SUCCESS;
}

void
printer_hw_free(void * info)
{
	PRINTER_DEVICE_INFO * pinfo;

	LLOGLN(10, ("printer_free"));
	pinfo = (PRINTER_DEVICE_INFO *) info;
	if (pinfo->printer_name)
	{
		free(pinfo->printer_name);
		pinfo->printer_name = NULL;
	}
	if (pinfo->printjob_http_t)
	{
		httpClose(pinfo->printjob_http_t);
		pinfo->printjob_http_t = NULL;
	}
	free(pinfo);
}

