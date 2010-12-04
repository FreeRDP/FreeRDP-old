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
#include <sys/stat.h>
#include "rdpdr_constants.h"
#include "rdpdr_types.h"
#include "devman.h"
#include "printer_main.h"

static char *
printer_get_filename(const char * name)
{
	char * home;
	char * filename;
	struct stat st;

	home = getenv("HOME");
	if (home == NULL)
		return NULL;

	filename = (char *) malloc(strlen(home) + strlen("/.freerdp/printer_") + strlen(name) + 1);
	sprintf(filename, "%s/.freerdp", home);
	if (stat(filename, &st) != 0)
	{
		mkdir(filename, 0700);
		LLOGLN(0, ("printer_get_filename: created %s", filename));
	}
	strcat(filename, "/printer_");
	strcat(filename, name);
	return filename;
}

static void
printer_save_data(const char * name, const char * data, int data_len)
{
	char * filename;
	FILE * fp;
	int len;

	filename = printer_get_filename(name);
	fp = fopen(filename, "w+b");
	if (fp == NULL)
	{
		LLOGLN(0, ("printer_save_data: create %s failed.", filename));
	}
	else
	{
		len = (int) fwrite(data, 1, data_len, fp);
		if (len < data_len)
		{
			LLOGLN(0, ("printer_save_data: error writing %d bytes.", len));
		}
		fclose(fp);
	}
	free(filename);
}

static char *
printer_get_data(const char * name, int * len)
{
	char * buf;
	char * filename;
	FILE * fp;
	int i;

	filename = printer_get_filename(name);
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		*len = 0;
		buf = NULL;
	}
	else
	{
		fseek(fp, 0, SEEK_END);
		*len = (int) ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buf = (char *) malloc(*len);
		memset(buf, 0, *len);
		i = fread(buf, 1, *len, fp);
	}
	free(filename);
	return buf;
}

static uint32
printer_process_update_printer_event(SERVICE * srv, const char * data, int data_len)
{
	uint32 printerNameLen;
	char * printerName;
	uint32 configDataLen;
	uint32 size;

	printerNameLen = GET_UINT32(data, 0);
	configDataLen = GET_UINT32(data, 4);

	if (printerNameLen + configDataLen + 8 > data_len)
	{
		LLOGLN(0, ("printer_process_update_printer_event: expect %d+%d+8 got %d",
			printerNameLen, configDataLen, data_len));
		return 1;
	}
	size = printerNameLen * 3 / 2 + 2;
	printerName = (char *) malloc(size);
	memset(printerName, 0, size);
	freerdp_get_wstr(printerName, size, (char *)(data + 8), printerNameLen);
	LLOGLN(10, ("printer_process_update_printer_event: %s %d", printerName, configDataLen));
	printer_save_data(printerName, data + 8 + printerNameLen, configDataLen);
	free(printerName);

	return 0;
}

static uint32
printer_process_delete_printer_event(SERVICE * srv, const char * data, int data_len)
{
	uint32 printerNameLen;
	char * printerName;
	uint32 size;
	char * filename;

	printerNameLen = GET_UINT32(data, 0);

	size = printerNameLen * 3 / 2 + 2;
	printerName = (char *) malloc(size);
	memset(printerName, 0, size);
	freerdp_get_wstr(printerName, size, (char *)(data + 4), printerNameLen);

	filename = printer_get_filename(printerName);
	remove(filename);
	LLOGLN(0, ("printer_process_delete_printer_event: %s deleted", filename));
	free(filename);
	free(printerName);

	return 0;
}

static uint32
printer_process_cache_data(SERVICE * srv, const char * data, int data_len)
{
	uint32 eventID;

	eventID = GET_UINT32(data, 0);
	switch (eventID)
	{
		case RDPDR_ADD_PRINTER_EVENT:
			LLOGLN(0, ("RDPDR_ADD_PRINTER_EVENT"));
			break;

		case RDPDR_UPDATE_PRINTER_EVENT:
			LLOGLN(10, ("RDPDR_UPDATE_PRINTER_EVENT"));
			printer_process_update_printer_event(srv, &data[4], data_len - 4);
			break;

		case RDPDR_DELETE_PRINTER_EVENT:
			LLOGLN(10, ("RDPDR_DELETE_PRINTER_EVENT"));
			printer_process_delete_printer_event(srv, &data[4], data_len - 4);
			break;

		case RDPDR_RENAME_PRINTER_EVENT:
			LLOGLN(0, ("RDPDR_RENAME_PRINTER_EVENT"));
			break;

		default:
			LLOGLN(0, ("RDPDR printer unsupported eventID %i", eventID));
			break;
	}
	return 0;
}

static uint32
printer_process_data(SERVICE * srv, int type, const char * data, int data_len)
{
	switch (type)
	{
		case PAKID_PRN_CACHE_DATA:
			LLOGLN(10, ("PAKID_PRN_CACHE_DATA"));
			printer_process_cache_data(srv, data, data_len);
			break;

		default:
			LLOGLN(0, ("RDPDR printer component, packetID: 0x%02X", type));
			break;
	}
	return 0;
}

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
	srv->process_data = printer_process_data;
	srv->type = RDPDR_DTYP_PRINT;
	srv->get_event = NULL;
	srv->file_descriptor = NULL;
	srv->get_timeouts = NULL;

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
	char * cache_data;
	int cache_data_len;

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

	cache_data = printer_get_data(name, &cache_data_len);

	size = 24 + 4 + (strlen(name) + 1) * 2 + (strlen(driver) + 1) * 2 + cache_data_len;
	dev->data = malloc(size);
	memset(dev->data, 0, size);

	/*flags = RDPDR_PRINTER_ANNOUNCE_FLAG_XPSFORMAT;*/
	flags = 0;
	if (is_default)
		flags |= RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER;

	SET_UINT32 (dev->data, 0, flags); /* Flags */
	SET_UINT32 (dev->data, 4, 0); /* CodePage, reserved */
	SET_UINT32 (dev->data, 8, 0); /* PnPNameLen */
	SET_UINT32 (dev->data, 20, cache_data_len); /* CachedFieldsLen */
	offset = 24;
	len = freerdp_set_wstr(&dev->data[offset], size - offset, (char *) driver, strlen(driver) + 1);
	SET_UINT32 (dev->data, 12, len); /* DriverNameLen */
	offset += len;
	len = freerdp_set_wstr(&dev->data[offset], size - offset, (char *) name, strlen(name) + 1);
	SET_UINT32 (dev->data, 16, len); /* PrintNameLen */
	offset += len;
	if (cache_data)
	{
		memcpy(&dev->data[offset], cache_data, cache_data_len);
		offset += cache_data_len;
		free(cache_data);
	}

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
			{
				printer_hw_register_auto(pDevman, pEntryPoints, srv, &port);
				break;
			}
			else
			{
				printer_register(pDevman, pEntryPoints, srv, data->data[1], data->data[2], (port == 1), &port);
			}
		}
		data = (RD_PLUGIN_DATA *) (((void *) data) + data->size);
	}

	return 1;
}
