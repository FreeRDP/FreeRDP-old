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

#ifndef __DEVMAN_H
#define __DEVMAN_H

#include "rdpdr_types.h"

typedef PSERVICE (*PDEVMAN_REGISTER_SERVICE)(PDEVMAN devman);
typedef int (*PDEVMAN_UNREGISTER_SERVICE)(PDEVMAN devman, PSERVICE srv);
typedef PDEVICE (*PDEVMAN_REGISTER_DEVICE)(PDEVMAN devman, PSERVICE srv, char* name);
typedef int (*PDEVMAN_UNREGISTER_DEVICE)(PDEVMAN devman, PDEVICE dev);

struct _DEVMAN_ENTRY_POINTS
{
	PDEVMAN_REGISTER_SERVICE pDevmanRegisterService;
	PDEVMAN_UNREGISTER_SERVICE pDevmanUnregisterService;
	PDEVMAN_REGISTER_DEVICE pDevmanRegisterDevice;
	PDEVMAN_UNREGISTER_DEVICE pDevmanUnregisterDevice;
	void* pExtendedData; /* extended data field to pass initial parameters */
};
typedef struct _DEVMAN_ENTRY_POINTS DEVMAN_ENTRY_POINTS;
typedef DEVMAN_ENTRY_POINTS * PDEVMAN_ENTRY_POINTS;

typedef int (*PDEVICE_SERVICE_ENTRY)(PDEVMAN, PDEVMAN_ENTRY_POINTS);

DEVMAN*
devman_new(void* data);
int
devman_free(DEVMAN* devman);
SERVICE*
devman_register_service(DEVMAN* devman);
int
devman_unregister_service(DEVMAN* devman, SERVICE* srv);
DEVICE*
devman_register_device(DEVMAN* devman, SERVICE* srv, char* name);
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
SERVICE*
devman_get_service_by_type(DEVMAN* devman, int type);
int
devman_load_device_service(DEVMAN* devman, char* filename);

#endif // __DEVMAN_H
