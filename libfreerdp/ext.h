/*
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Device Manager

   Copyright 2010-2011 Vic Lee

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

#ifndef __EXT_H
#define __EXT_H

#include <freerdp/rdpext.h>
#include "rdp.h"

#define RDPEXT_MAX_COUNT 15

struct rdp_ext
{
	rdpInst * inst;
	rdpExtPlugin * plugins[RDPEXT_MAX_COUNT];
	int num_plugins;
	PFREERDP_EXTENSION_HOOK pre_connect_hooks[RDPEXT_MAX_COUNT];
	rdpExtPlugin * pre_connect_hooks_instances[RDPEXT_MAX_COUNT];
	int num_pre_connect_hooks;
	PFREERDP_EXTENSION_HOOK post_connect_hooks[RDPEXT_MAX_COUNT];
	rdpExtPlugin * post_connect_hooks_instances[RDPEXT_MAX_COUNT];
	int num_post_connect_hooks;
};
typedef struct rdp_ext rdpExt;

rdpExt *
ext_new(rdpRdp * rdp);
void
ext_free(rdpExt * ext);
int
ext_pre_connect(rdpExt * ext);
int
ext_post_connect(rdpExt * ext);

#endif

