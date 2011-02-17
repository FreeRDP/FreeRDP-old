/*
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI Main Window

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

#ifndef __DFB_WIN_H
#define __DFB_WIN_H

#include <freerdp/freerdp.h>

void
dfb_init(int *argc, char *(*argv[]));
int
dfb_pre_connect(rdpInst * inst);
int
dfb_post_connect(rdpInst * inst);
void
dfb_uninit(void * dfb_info);
int
dfb_get_fds(rdpInst * inst, void ** read_fds, int * read_count,
	void ** write_fds, int * write_count);
int
dfb_check_fds(rdpInst * inst, fd_set *set);
int
dfb_err(rdpInst * inst);

#endif /* __DFB_WIN_H */
