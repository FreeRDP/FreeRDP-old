/*
   FreeRDP: A Remote Desktop Protocol client.

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

#include <stdio.h>
#include <freerdp/freerdp.h>
#include "wf_event.h"

extern HCURSOR g_default_cursor;

#define X_POS (lParam & 0xffff)
#define Y_POS ((lParam >> 16) & 0xffff)
#define SCANCODE ((lParam >> 16) & 0xff)

LRESULT CALLBACK
wf_event_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	wfInfo * wfi;
	LONG ptr;
	HDC hdc;
	PAINTSTRUCT ps;

	ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
	wfi = (wfInfo *) ptr;
	switch(Msg)
	{
	case WM_DESTROY:
		/* TODO: Should be multi-session aware */
		PostQuitMessage(WM_QUIT);
		break;

	case WM_PAINT:
		if (wfi != NULL)
		{
			hdc = BeginPaint(hWnd, &ps);
			BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right - ps.rcPaint.left,
				ps.rcPaint.bottom - ps.rcPaint.top,
				wfi->backstore->hdc, ps.rcPaint.left, ps.rcPaint.top,
				SRCCOPY);
			EndPaint(hWnd, &ps);
		}
		break;

	case WM_LBUTTONDOWN:
		if (wfi != NULL)
		{
			wfi->inst->rdp_send_input(wfi->inst, RDP_INPUT_MOUSE,
				PTRFLAGS_DOWN | PTRFLAGS_BUTTON1, X_POS, Y_POS);
		}
		break;

	case WM_LBUTTONUP:
		if (wfi != NULL)
		{
			wfi->inst->rdp_send_input(wfi->inst, RDP_INPUT_MOUSE,
				PTRFLAGS_BUTTON1, X_POS, Y_POS);
		}
		break;

	case WM_RBUTTONDOWN:
		if (wfi != NULL)
		{
			wfi->inst->rdp_send_input(wfi->inst, RDP_INPUT_MOUSE,
				PTRFLAGS_DOWN | PTRFLAGS_BUTTON2, X_POS, Y_POS);
		}
		break;

	case WM_RBUTTONUP:
		if (wfi != NULL)
		{
			wfi->inst->rdp_send_input(wfi->inst, RDP_INPUT_MOUSE,
				PTRFLAGS_BUTTON2, X_POS, Y_POS);
		}
		break;

	case WM_MOUSEMOVE:
		if (wfi != NULL)
		{
			wfi->inst->rdp_send_input(wfi->inst, RDP_INPUT_MOUSE,
				PTRFLAGS_MOVE, X_POS, Y_POS);
		}
		break;

	case WM_KEYDOWN:
		if (wfi != NULL)
		{
			wfi->inst->rdp_send_input(wfi->inst, RDP_INPUT_SCANCODE,
				RDP_KEYPRESS, SCANCODE, 0);
		}
		break;

	case WM_KEYUP:
		if (wfi != NULL)
		{
			wfi->inst->rdp_send_input(wfi->inst, RDP_INPUT_SCANCODE,
				RDP_KEYRELEASE, SCANCODE, 0);
		}
		break;

	case WM_SETCURSOR:
		if (wfi != NULL)
		{
			SetCursor(wfi->cursor);
		}
		else
		{
			SetCursor(g_default_cursor);
		}
		break;

	default:
		// Process the left-over messages
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	// If something was not done, let it go
	return 0;
}
