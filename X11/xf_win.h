/*
   FreeRDP: A Remote Desktop Protocol client.
   UI window

   Copyright (C) Jay Sorg 2009-2011

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

#ifndef __XF_WIN_H
#define __XF_WIN_H

#include "xf_types.h"

int
xf_pre_connect(xfInfo * xfi);
int
xf_post_connect(xfInfo * xfi);
void
xf_uninit(xfInfo * xfi);
int
xf_get_fds(xfInfo * xfi, void ** read_fds, int * read_count,
	void ** write_fds, int * write_count);
int
xf_check_fds(xfInfo * xfi);
void
xf_toggle_fullscreen(xfInfo * xfi);

#endif
