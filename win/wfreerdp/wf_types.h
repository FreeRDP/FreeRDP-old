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
#include <freerdp/chanman.h>
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
	/* RDP stuff */
	rdpSet * settings;
	rdpChanMan * chan_man;

	/* UI settings */
	int fs_toggle;
	int fullscreen;
	int percentscreen;

	/* Windows stuff */
	HWND hwnd;
	rdpInst * inst;
	struct wf_bitmap * backstore; /* paint here - InvalidateRect will cause a WM_PAINT event that will BitBlt to hWnd */
	/* state: */
	struct wf_bitmap * drw; /* the current drawing surface - either backstore or something else */
	uint8 * palette;
	HCURSOR cursor;
	HBRUSH brush;
	HBRUSH org_brush;
};
typedef struct wf_info wfInfo;

#ifdef WITH_DEBUG
#define DEBUG(fmt, ...)	printf("DBG (win) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_KBD
#define DEBUG_KBD(fmt, ...) printf("DBG (win-KBD) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_KBD(fmt, ...) do { } while (0)
#endif

#endif
