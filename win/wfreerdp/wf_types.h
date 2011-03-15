/*
   FreeRDP: A Remote Desktop Protocol client.
   UI types

   Copyright (c) 2009-2011 Jay Sorg
   Copyright (c) 2010-2011 Vic Lee

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

#ifndef __WF_TYPES_H
#define __WF_TYPES_H

#include <windows.h>
#include <freerdp/types_ui.h>

#define SET_WFI(_inst, _wfi) (_inst)->param1 = _wfi
#define GET_WFI(_inst) ((wfInfo *) ((_inst)->param1))

struct wf_bitmap
{
	HDC hdc;
	HBITMAP bitmap;
	HBITMAP org_bitmap;
};

struct wf_info
{
	HWND hwnd;
	rdpInst * inst;
	struct wf_bitmap * backstore;
	struct wf_bitmap * drw;
	uint8 * colormap;
	RECT update_rect;
	int update_pending;
	HCURSOR cursor;
	HBRUSH brush;
	HBRUSH org_brush;
};
typedef struct wf_info wfInfo;

#endif
