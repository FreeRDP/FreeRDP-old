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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drdynvc_types.h"

static int
MyPluginInitialize(IWTSPlugin * pPlugin, IWTSVirtualChannelManager * pChannelMgr)
{
	LLOGLN(10, ("MyPluginInitialize: audin"));
	return 0;
}

static int
MyPluginTerminated(IWTSPlugin * pPlugin)
{
	LLOGLN(10, ("MyPluginTerminated: audin"));
	return 0;
}

int
DVCPluginEntry(IDRDYNVC_ENTRY_POINTS * pEntryPoints)
{
	IWTSPlugin * plugin;

	plugin = pEntryPoints->CreatePlugin(pEntryPoints);
	plugin->Initialize = MyPluginInitialize;
	plugin->Connected = NULL;
	plugin->Disconnected = NULL;
	plugin->Terminated = MyPluginTerminated;

	return 0;
}

