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

#ifndef __EXT_H
#define __EXT_H

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

