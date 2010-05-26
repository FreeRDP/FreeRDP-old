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

	void * printjob_object;
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

#ifndef _CUPS_API_1_4
	LLOGLN(0, ("printer_hw_new: use CUPS API 1.2"));
#endif

	return info;
}

static void
printer_hw_get_printjob_name(char * buf, int size)
{
	time_t tt;
	struct tm * t;

	tt = time(NULL);
	t = localtime(&tt);
	snprintf(buf, size - 1, "FreeRDP Print Job %d%02d%02d%02d%02d%02d",
		t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec);
}

uint32
printer_hw_create(IRP * irp, const char * path)
{
	PRINTER_DEVICE_INFO * info;

	info = (PRINTER_DEVICE_INFO *) irp->dev->info;

	/* Server's print queue will ensure no two print jobs will be sent to the same printer.
	   However, we still want to do a simple locking just to ensure we are safe. */
	if (info->printjob_object)
	{
		return RD_STATUS_DEVICE_BUSY;
	}

#ifndef _CUPS_API_1_4

	info->printjob_id++;
	info->printjob_object = strdup(tmpnam(NULL));

#else
	{
		char buf[100];

		info->printjob_object = httpConnectEncrypt(cupsServer(), ippPort(), HTTP_ENCRYPT_IF_REQUESTED);
		if (info->printjob_object == NULL)
		{
			LLOGLN(0, ("printer_hw_create: httpConnectEncrypt: %s", cupsLastErrorString()));
			return RD_STATUS_DEVICE_BUSY;
		}

		printer_hw_get_printjob_name(buf, sizeof(buf));
		info->printjob_id = cupsCreateJob((http_t *) info->printjob_object,
			info->printer_name, buf,
			0, NULL);

		if (info->printjob_id == 0)
		{
			LLOGLN(0, ("printer_hw_create: cupsCreateJob: %s", cupsLastErrorString()));
			httpClose((http_t *) info->printjob_object);
			info->printjob_object = NULL;
			/* Should get the right return code based on printer status */
			return RD_STATUS_DEVICE_BUSY;
		}
		cupsStartDocument((http_t *) info->printjob_object,
			info->printer_name, info->printjob_id, buf,
			CUPS_FORMAT_POSTSCRIPT, 1);
	}

#endif

	LLOGLN(10, ("printe_hw_create: %s id=%d", info->printer_name, info->printjob_id));
	irp->fileID = info->printjob_id;

	return RD_STATUS_SUCCESS;
}

uint32
printer_hw_close(IRP * irp)
{
	PRINTER_DEVICE_INFO * info;

	info = (PRINTER_DEVICE_INFO *) irp->dev->info;
	LLOGLN(10, ("printe_hw_close: %s id=%d", info->printer_name, irp->fileID));

	if (irp->fileID != info->printjob_id)
	{
		LLOGLN(0, ("printer_hw_close: invalid file id"));
		return RD_STATUS_INVALID_HANDLE;
	}

#ifndef _CUPS_API_1_4

	{
		char buf[100];

		printer_hw_get_printjob_name(buf, sizeof(buf));
		if (cupsPrintFile(info->printer_name, (const char *) info->printjob_object, buf, 0, NULL) == 0)
		{
			LLOGLN(0, ("printer_hw_close: cupsPrintFile: %s", cupsLastErrorString()));
		}
		unlink(info->printjob_object);
		free(info->printjob_object);
	}

#else

	cupsFinishDocument((http_t *) info->printjob_object, info->printer_name);
	info->printjob_id = 0;
	httpClose((http_t *) info->printjob_object);

#endif

	info->printjob_object = NULL;

	return RD_STATUS_SUCCESS;
}

uint32
printer_hw_write(IRP * irp)
{
	PRINTER_DEVICE_INFO * info;

	info = (PRINTER_DEVICE_INFO *) irp->dev->info;
	LLOGLN(10, ("printe_hw_write: %s id=%d len=%d off=%lld", info->printer_name,
		irp->fileID, irp->inputBufferLength, irp->offset));
	if (irp->fileID != info->printjob_id)
	{
		LLOGLN(0, ("printer_hw_write: invalid file id"));
		return RD_STATUS_INVALID_HANDLE;
	}

#ifndef _CUPS_API_1_4

	{
		FILE * fp;

		fp = fopen((const char *) info->printjob_object, "a+b");
		if (fp == NULL)
		{
			LLOGLN(0, ("printer_hw_write: failed to open file %s", (char *) info->printjob_object));
			return RD_STATUS_DEVICE_BUSY;
		}
		if (fwrite(irp->inputBuffer, 1, irp->inputBufferLength, fp) < irp->inputBufferLength)
		{
			fclose(fp);
			LLOGLN(0, ("printer_hw_write: failed to write file %s", (char *) info->printjob_object));
			return RD_STATUS_DEVICE_BUSY;
		}
		fclose(fp);
	}

#else

	cupsWriteRequestData((http_t *) info->printjob_object, irp->inputBuffer, irp->inputBufferLength);

#endif

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
	if (pinfo->printjob_object)
	{
#ifndef _CUPS_API_1_4
		free(pinfo->printjob_object);
#else
		httpClose((http_t *) pinfo->printjob_object);
#endif
		pinfo->printjob_object = NULL;
	}
	free(pinfo);
}

