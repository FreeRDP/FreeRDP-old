/*
   FreeRDP: A Remote Desktop Protocol client.
   Extension

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

#ifndef __RDPEXT_H
#define __RDPEXT_H

#include <freerdp/types/ui.h>

#ifdef _WIN32
#define RDPEXT_CC __stdcall
#else
#define RDPEXT_CC
#endif

/* Extensions ought to check for it to ensure compatibility */
#define RDPEXT_API 1

#define RDPEXT_EXPORT_FUNC_NAME "FreeRDPExtensionEntry"

typedef struct rdp_ext_plugin rdpExtPlugin;

struct rdp_ext_plugin
{
	void * ext;
	int (*init) (rdpExtPlugin * plugin, rdpInst * inst);
	int (*uninit) (rdpExtPlugin * plugin, rdpInst * inst);
};

typedef uint32 (RDPEXT_CC * PFREERDP_EXTENSION_HOOK)(rdpExtPlugin * plugin, rdpInst * inst);

typedef uint32 (RDPEXT_CC * PREGISTEREXTENSION)(rdpExtPlugin * plugin);
typedef uint32 (RDPEXT_CC * PREGISTERPRECONNECTHOOK)(rdpExtPlugin * plugin, PFREERDP_EXTENSION_HOOK hook);
typedef uint32 (RDPEXT_CC * PREGISTERPOSTCONNECTHOOK)(rdpExtPlugin * plugin, PFREERDP_EXTENSION_HOOK hook);

struct _FREERDP_EXTENSION_ENTRY_POINTS
{
	void * ext; /* Reference to internal instance */
	PREGISTEREXTENSION pRegisterExtension;
	PREGISTERPRECONNECTHOOK pRegisterPreConnectHook;
	PREGISTERPOSTCONNECTHOOK pRegisterPostConnectHook;
	void * data;
};
typedef struct _FREERDP_EXTENSION_ENTRY_POINTS FREERDP_EXTENSION_ENTRY_POINTS;
typedef FREERDP_EXTENSION_ENTRY_POINTS * PFREERDP_EXTENSION_ENTRY_POINTS;

typedef int (RDPEXT_CC * PFREERDP_EXTENSION_ENTRY)(PFREERDP_EXTENSION_ENTRY_POINTS pEntryPoints);

#endif
