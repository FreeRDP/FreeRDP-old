/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Device Manager

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

#include "devman.h"
#include "types_ui.h"

int dev_count; /* device count */
DEVICE* pdev; /* pointer to a device */
DEVICE* idev; /* iterator device */
DEVICE* head_dev; /* head device in linked list */
DEVICE* tail_dev; /* tail device in linked list */

void
devman_init()
{
	pdev = NULL;
	idev = NULL;
	head_dev = NULL;
	tail_dev = NULL;
	dev_count = 0;
}

SERVICE*
devman_register_service(uint32 type)
{
	SERVICE* service = (SERVICE*)malloc(sizeof(SERVICE));	

	switch (type)
	{
		case DEVICE_TYPE_SERIAL:
			service->type = DEVICE_TYPE_SERIAL;
			break;

		case DEVICE_TYPE_PARALLEL:
			service->type = DEVICE_TYPE_PARALLEL;
			break;

		case DEVICE_TYPE_PRINTER:
			service->type = DEVICE_TYPE_PRINTER;
			break;

		case DEVICE_TYPE_DISK:
			service->type = DEVICE_TYPE_DISK;
			break;

		case DEVICE_TYPE_SMARTCARD:
			service->type = DEVICE_TYPE_SMARTCARD;
			break;

		default:
			/* unknown device service type */
			free(service);
			return NULL;
			break;
	}

	service->create = NULL;
	service->close = NULL;
	service->read = NULL;
	service->write = NULL;
	service->control = NULL;

	return service;
}

int
devman_unregister_service(SERVICE* service)
{
	/* unregister all devices depending on the service */

	devman_rewind();

	while (devman_has_next() != 0)
	{
		pdev = devman_get_next();

		if (pdev->service == service)
		{
			devman_unregister_device(pdev);
			devman_rewind();
		}
	}

	/* unregister service */
	free(service);

	return 1;
}

DEVICE*
devman_register_device(SERVICE* service)
{
	pdev = (DEVICE*)malloc(sizeof(DEVICE));
	pdev->prev = NULL;
	pdev->next = NULL;
	pdev->service = service;

	if (head_dev == tail_dev)
	{
		/* linked list is empty */
		head_dev = pdev;
		tail_dev = pdev;
	}
	else
	{
		/* append device to the end of the linked list */
		tail_dev->next = (void*)pdev;
		pdev->prev = (void*)tail_dev;
		tail_dev = pdev;
	}

	dev_count++;
	return pdev;
}

int
devman_unregister_device(DEVICE* dev)
{
	devman_rewind();

	while (devman_has_next() != 0)
	{
		pdev = devman_get_next();
		
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
				/* unregistered device is the head, update head_dev */
				head_dev = (DEVICE*)dev->next;
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
				/* unregistered device is the tail, update tail_dev */
				tail_dev = (DEVICE*)dev->prev;
			}

			dev_count--;

			free(dev); /* free memory for unregistered device */
			return 1; /* unregistration successful */
		}
	}

	/* if we reach this point, the device wasn't found */
	return 0;
}

void
devman_rewind()
{
	idev = head_dev;
}

int
devman_has_next()
{
	if (idev == NULL)
		return 0;
	else if (idev == head_dev)
		return 1;
	else if (idev->next != NULL)
		return 1;
	else
		return 0;
}

DEVICE*
devman_get_next()
{
	pdev = idev;
	idev = (DEVICE*)idev->next;
	return pdev;
}

int
devman_get_count()
{
	return dev_count;
}

