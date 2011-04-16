/*
   FreeRDP: A Remote Desktop Protocol client.
   UI XVideo

   Copyright (C) Vic Lee 2011

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

#ifndef __XF_VIDEO_H
#define __XF_VIDEO_H

#include "xf_types.h"

int
xf_video_init(xfInfo * xfi);
int
xf_video_process_frame(xfInfo * xfi, RD_VIDEO_FRAME_EVENT * vevent);
void
xf_video_uninit(xfInfo * xfi);

#endif

