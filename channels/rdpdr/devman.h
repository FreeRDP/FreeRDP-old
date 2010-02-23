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

#ifndef __DEVMAN_H
#define __DEVMAN_H

#include "types_ui.h"

typedef int (*PDEVICE_SERVICE_INIT)(void);

typedef struct service
{
	uint32 type;
	int(*create) (void);
	int(*close) (void);
	int(*read) (void);
	int(*write) (void);
	int(*control) (void);
}
SERVICE;

typedef struct device
{
	uint32 id;
	void* info;
	void* prev;
	void* next;
	SERVICE* service;
}
DEVICE;

typedef struct devman
{
	int count; /* device count */
	DEVICE* idev; /* iterator device */
	DEVICE* head; /* head device in linked list */
	DEVICE* tail; /* tail device in linked list */
}
DEVMAN;

DEVMAN*
devman_new();
int
devman_free(DEVMAN* devman);
SERVICE*
devman_register_service(DEVMAN* devman, uint32 type);
int
devman_unregister_service(DEVMAN* devman, SERVICE* srv);
DEVICE*
devman_register_device(DEVMAN* devman, SERVICE* srv);
int
devman_unregister_device(DEVMAN* devman, DEVICE* dev);
void
devman_rewind(DEVMAN* devman);
int
devman_has_next(DEVMAN* devman);
DEVICE*
devman_get_next(DEVMAN* devman);
DEVICE*
devman_get_device_by_id(DEVMAN* devman, uint32 id);
int
devman_load_device_service(DEVMAN* devman, char* filename);

#endif // __DEVMAN_H

