/*
   FreeRDP: A Remote Desktop Protocol client.
   Rdpdr stuff for redirected Smart Card Device Service

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


#ifndef __RDPDR_SCARD_H
#define __RDPDR_SCARD_H

#include <pthread.h>

#include "rdpdr_types.h"
#include "rdpdr_main.h"

extern pthread_t scard_thread;

void *
rdpdr_scard_finished_scanner(void *arg);

#endif
