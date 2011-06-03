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

#ifndef __SCARD_MAIN_H
#define __SCARD_MAIN_H

#include <inttypes.h>

#include "devman.h"
#include "rdpdr_types.h"
#include "scard_queue.h"
#include <freerdp/debug.h>

struct _SCARD_DEVICE_INFO
{
	PDEVMAN devman;

	PDEVMAN_REGISTER_SERVICE DevmanRegisterService;
	PDEVMAN_UNREGISTER_SERVICE DevmanUnregisterService;
	PDEVMAN_REGISTER_DEVICE DevmanRegisterDevice;
	PDEVMAN_UNREGISTER_DEVICE DevmanUnregisterDevice;

	char * path;
	char * name;
	char * alias;
};
typedef struct _SCARD_DEVICE_INFO SCARD_DEVICE_INFO;


uint32_t
sc_create();

IRP *
sc_enqueue_pending(IRP * peding);

IRP *
sc_next_finished();

void
sc_wait_finished_ready();

#ifdef WITH_DEBUG_SCARD
#define DEBUG_SCARD(fmt, ...) DEBUG_CLASS(SCARD, fmt, ...)
#else
#define DEBUG_SCARD(fmt, ...) DEBUG_NULL(fmt, ...)
#endif

#endif
