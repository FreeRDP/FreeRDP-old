/*
   Copyright (c) 2010 Vic Lee

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

*/

#ifndef __RDPEXT_H
#define __RDPEXT_H

#include <freerdp/types_ui.h>

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
