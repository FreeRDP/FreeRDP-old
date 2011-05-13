/*
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Smart Card Device Service

   Copyright 2011 O.S. Systems Software Ltda.
   Copyright 2011 Eduardo Fiss Beloni <beloni@ossystems.com.br>

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

#include <stdlib.h>

#include "rdpdr_types.h"
#include "rdpdr_constants.h"

#include "scard_main.h"

static uint32
scard_close(IRP * irp)
{
	return RD_STATUS_SUCCESS;
}

static uint32
scard_read(IRP * irp)
{
	return RD_STATUS_SUCCESS;
}

static uint32
scard_write(IRP * irp)
{
	return RD_STATUS_SUCCESS;
}

static uint32
scard_create(IRP * irp, const char * path)
{
	if (!irp && !path)
		return sc_create();

	return RD_STATUS_SUCCESS;
}

static uint32
scard_control(IRP * irp)
{
	if (sc_enqueue_pending(irp))
		return RD_STATUS_PENDING | 0xC0000000;

	return RD_STATUS_NO_SUCH_FILE;
}

static uint32
scard_free(DEVICE * dev)
{
	free(dev->info);
	dev->info = NULL;
	if (dev->data)
	{
		free(dev->data);
		dev->data = NULL;
	}

	return 0;
}

static void *
scard_message(void * data)
{
	int wait = (int) data;

	if (wait)
	{
		sc_wait_finished_ready();
		return NULL;
	}

	return sc_next_finished();
}

static SERVICE *
scard_register_service(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints)
{
	SERVICE * srv;

	srv = pEntryPoints->pDevmanRegisterService(pDevman);

	srv->create = scard_create;
	srv->close = scard_close;
	srv->read = scard_read;
	srv->write = scard_write;
	srv->control = scard_control;
	srv->query_volume_info = NULL;
	srv->query_info = NULL;
	srv->set_info = NULL;
	srv->query_directory = NULL;
	srv->notify_change_directory = NULL;
	srv->lock_control = NULL;
	srv->free = scard_free;
	srv->type = RDPDR_DTYP_SMARTCARD;
	srv->get_event = NULL;
	srv->file_descriptor = NULL;
	srv->get_timeouts = NULL;
	srv->message = scard_message;

	return srv;
}

int
DeviceServiceEntry(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints)
{
	SERVICE * srv = NULL;
	DEVICE * dev;
	SCARD_DEVICE_INFO * info;
	RD_PLUGIN_DATA * data;
	int i;

	data = (RD_PLUGIN_DATA *) pEntryPoints->pExtendedData;
	while (data && data->size > 0)
	{
		if (strcmp((char*)data->data[0], "scard") == 0)
		{
			if (srv == NULL)
				srv = scard_register_service(pDevman, pEntryPoints);

			info = (SCARD_DEVICE_INFO *) malloc(sizeof(SCARD_DEVICE_INFO));
			memset(info, 0, sizeof(SCARD_DEVICE_INFO));
			info->devman = pDevman;
			info->DevmanRegisterService = pEntryPoints->pDevmanRegisterService;
			info->DevmanUnregisterService = pEntryPoints->pDevmanUnregisterService;
			info->DevmanRegisterDevice = pEntryPoints->pDevmanRegisterDevice;
			info->DevmanUnregisterDevice = pEntryPoints->pDevmanUnregisterDevice;

			info->name = (char *) data->data[1];
			info->alias = (char *) data->data[2];

			dev = info->DevmanRegisterDevice(pDevman, srv, "SCARD");
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
