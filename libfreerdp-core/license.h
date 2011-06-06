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

#ifndef __LICENSE_H
#define __LICENSE_H

#include "stream.h"
#include <freerdp/types/ui.h>
#include <freerdp/utils/debug.h>

struct rdp_license
{
	struct rdp_sec * sec;
	uint8 license_key[16];
	uint8 license_sign_key[16];
	RD_BOOL license_issued;
};
typedef struct rdp_license rdpLicense;

void
license_process(rdpLicense * license, STREAM s);
rdpLicense *
license_new(struct rdp_sec * secure);
void
license_free(rdpLicense * license);

#ifdef WITH_DEBUG_LICENSE
#define DEBUG_LICENSE(fmt, ...) DEBUG_CLASS("LICENSE", fmt, ## __VA_ARGS__)
#else
#define DEBUG_LICENSE(fmt, ...) DEBUG_NULL(fmt, ## __VA_ARGS__)
#endif

#endif
