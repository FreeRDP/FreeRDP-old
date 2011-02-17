/*
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Device Manager

   Copyright 2010 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "rdpdr_types.h"
#include "rdpdr_constants.h"
#include "devman.h"

DEVMAN*
devman_new(void* data)
{
	DEVMAN* devman;
	PDEVMAN_ENTRY_POINTS pDevmanEntryPoints;

	devman = (PDEVMAN)malloc(sizeof(DEVMAN));
	pDevmanEntryPoints = (PDEVMAN_ENTRY_POINTS)malloc(sizeof(DEVMAN_ENTRY_POINTS));

	devman->idev = NULL;
	devman->head = NULL;
	devman->tail = NULL;
	devman->count = 0;
	devman->id_sequence = 1;

	pDevmanEntryPoints->pDevmanRegisterService = devman_register_service;
	pDevmanEntryPoints->pDevmanUnregisterService = devman_unregister_service;
	pDevmanEntryPoints->pDevmanRegisterDevice = devman_register_device;
	pDevmanEntryPoints->pDevmanUnregisterDevice = devman_unregister_device;
	pDevmanEntryPoints->pExtendedData = data;
	devman->pDevmanEntryPoints = (void*)pDevmanEntryPoints;

	return devman;
}

int
devman_free(DEVMAN* devman)
{
	DEVICE* pdev;

	/* unregister all services, which will in turn unregister all devices */

	devman_rewind(devman);

	while (devman_has_next(devman) != 0)
	{
		pdev = devman_get_next(devman);
		devman_unregister_service(devman, pdev->service);
		devman_rewind(devman);
	}

	free(devman->pDevmanEntryPoints);

	/* free devman */
	free(devman);

	return 1;
}

SERVICE*
devman_register_service(DEVMAN* devman)
{
	SERVICE* srv;

	srv = (SERVICE*)malloc(sizeof(SERVICE));
	memset(srv, 0, sizeof(SERVICE));

	return srv;
}

int
devman_unregister_service(DEVMAN* devman, SERVICE* srv)
{
	DEVICE* pdev;

	/* unregister all devices depending on the service */

	devman_rewind(devman);

	while (devman_has_next(devman) != 0)
	{
		pdev = devman_get_next(devman);

		if (pdev->service == srv)
		{
			devman_unregister_device(devman, pdev);
			devman_rewind(devman);
		}
	}

	/* unregister service */
	free(srv);

	return 1;
}

DEVICE*
devman_register_device(DEVMAN* devman, SERVICE* srv, char* name)
{
	DEVICE* pdev;

	pdev = (DEVICE*)malloc(sizeof(DEVICE));
	pdev->id = devman->id_sequence++;
	pdev->prev = NULL;
	pdev->next = NULL;
	pdev->service = srv;
	pdev->data_len = 0;
	pdev->data = NULL;

	pdev->name = malloc(strlen(name) + 1);
	strcpy(pdev->name, name);

	if (devman->head == NULL)
	{
		/* linked list is empty */
		devman->head = pdev;
		devman->tail = pdev;
	}
	else
	{
		/* append device to the end of the linked list */
		devman->tail->next = (void*)pdev;
		pdev->prev = (void*)devman->tail;
		devman->tail = pdev;
	}

	devman->count++;
	return pdev;
}

int
devman_unregister_device(DEVMAN* devman, DEVICE* dev)
{
	DEVICE* pdev;

	devman_rewind(devman);

	while (devman_has_next(devman) != 0)
	{
		pdev = devman_get_next(devman);

		if (pdev == dev) /* device exists */
		{
			/* set previous device to point to next device */

			if (dev->prev != NULL)
			{
				/* unregistered device is not the head */
				pdev = (DEVICE*)dev->prev;
				pdev->next = dev->next;
			}
			else
			{
				/* unregistered device is the head, update head */
				devman->head = (DEVICE*)dev->next;
			}

			/* set next device to point to previous device */

			if (dev->next != NULL)
			{
				/* unregistered device is not the tail */
				pdev = (DEVICE*)dev->next;
				pdev->prev = dev->prev;
			}
			else
			{
				/* unregistered device is the tail, update tail */
				devman->tail = (DEVICE*)dev->prev;
			}

			devman->count--;

			if (dev->service->free)
				dev->service->free(dev);
			free(dev->name);
			free(dev); /* free memory for unregistered device */
			return 1; /* unregistration successful */
		}
	}

	/* if we reach this point, the device wasn't found */
	return 0;
}

void
devman_rewind(DEVMAN* devman)
{
	devman->idev = devman->head;
}

int
devman_has_next(DEVMAN* devman)
{
	if (devman->idev == NULL)
		return 0;
	else
		return 1;
}

DEVICE*
devman_get_next(DEVMAN* devman)
{
	DEVICE* pdev;

	pdev = devman->idev;
	devman->idev = (DEVICE*)devman->idev->next;

	return pdev;
}

DEVICE*
devman_get_device_by_id(DEVMAN* devman, uint32 id)
{
	DEVICE* pdev;

	devman_rewind(devman);

	while (devman_has_next(devman) != 0)
	{
		pdev = devman_get_next(devman);

		/* device with given ID was found */
		if (pdev->id == id)
			return pdev;
	}

	/* no device with the given ID was found */
	return NULL;
}

SERVICE*
devman_get_service_by_type(DEVMAN* devman, int type)
{
	DEVICE* pdev;

	devman_rewind(devman);

	while (devman_has_next(devman) != 0)
	{
		pdev = devman_get_next(devman);

		/* service with given type was found */
		if (pdev->service->type == type)
			return pdev->service;
	}

	/* no service with the given type was found */
	return NULL;
}

int
devman_load_device_service(DEVMAN* devman, char* filename)
{
	void* dl;
	char* fn;
	PDEVICE_SERVICE_ENTRY pDeviceServiceEntry = NULL;

	if (strchr(filename, '/'))
	{
		fn = strdup(filename);
	}
	else
	{
		fn = malloc(strlen(PLUGIN_PATH) + strlen(filename) + 10);
		sprintf(fn, PLUGIN_PATH "/%s.so", filename);
	}
	dl = dlopen(fn, RTLD_LOCAL | RTLD_LAZY);

	pDeviceServiceEntry = (PDEVICE_SERVICE_ENTRY)dlsym(dl, "DeviceServiceEntry");

	if(pDeviceServiceEntry != NULL)
	{
		pDeviceServiceEntry(devman, devman->pDevmanEntryPoints);
		LLOGLN(0, ("loaded device service: %s", fn));
	}
	free(fn);

	return 0;
}
