/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP licensing negotiation

   Copyright (C) Jay Sorg 2009

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

#ifndef __LICENCE_H
#define __LICENCE_H

#include <freerdp/debug.h>
#include <freerdp/types/ui.h>
#include "stream.h"

struct rdp_licence
{
	struct rdp_sec * sec;
	uint8 licence_key[16];
	uint8 licence_sign_key[16];
	RD_BOOL licence_issued;
};
typedef struct rdp_licence rdpLicence;

void
licence_process(rdpLicence * licence, STREAM s);
rdpLicence *
licence_new(struct rdp_sec * secure);
void
licence_free(rdpLicence * licence);

#ifdef WITH_DEBUG_LICENSE
#define DEBUG_LICENSE(fmt, ...) DEBUG_CLASS(LICENSE, fmt, ...)
#else
#define DEBUG_LICENSE(fmt, ...) DEBUG_NULL(fmt, ...)
#endif

#endif
