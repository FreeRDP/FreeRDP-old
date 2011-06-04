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
#include "wfreerdp.h"
#include "wf_win.h"

#include "wf_event.h"

extern HCURSOR g_default_cursor;
static HWND g_focus_hWnd;	/* set to hWnd when wfreerdp has focus so the low level keyboard hook can use it */

#define X_POS(lParam) (lParam & 0xffff)
#define Y_POS(lParam) ((lParam >> 16) & 0xffff)

LRESULT CALLBACK
wf_ll_kbd_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
	wfInfo * wfi;
	uint8 scanCode;
	DWORD flags;

	DEBUG_KBD("Low-level keyboard hook, hWnd %X nCode %X wParam %X\n", g_focus_hWnd, nCode, wParam);
	if (g_focus_hWnd && (nCode == HC_ACTION)) {
		switch (wParam) {
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
			wfi = (wfInfo *) GetWindowLongPtr(g_focus_hWnd, GWLP_USERDATA);
			PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT) lParam;
			scanCode = (uint8)p->scanCode;
			flags = p->flags;
			DEBUG_KBD("keydown %d scanCode %04X flags %02X vkCode %02X\n",
					wParam == WM_KEYDOWN, scanCode, flags, p->vkCode);

			if (wfi->fs_toggle &&
					((p->vkCode == VK_RETURN) || (p->vkCode == VK_CANCEL)) &&
					(GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
					(GetAsyncKeyState(VK_MENU) & 0x8000)) // could also use flags & LLKHF_ALTDOWN
			{
				if (wParam == WM_KEYDOWN)
				{
					wf_toggle_fullscreen(wfi);
				}
				return 1;
			}

			if (scanCode == 0x45) /* NumLock-ish */
			{
				if (flags & LLKHF_EXTENDED)
				{
					/* Windows sends NumLock as extended - rdp doesn't */
					DEBUG_KBD("hack: NumLock (x45) should not be extended\n");
					flags &= ~LLKHF_EXTENDED;
				}
				else
				{
					/* Windows sends Pause as if it was a RDP NumLock (handled above).
					 * It must however be sent as a one-shot Ctrl+NumLock */
					if (wParam == WM_KEYDOWN)
					{
						DEBUG_KBD("Pause, sent as Ctrl+NumLock\n");
						wfi->inst->rdp_send_input_scancode(wfi->inst, 0, 0, 0x1D); /* Ctrl down */
						wfi->inst->rdp_send_input_scancode(wfi->inst, 0, 0, 0x45); /* NumLock down */
						wfi->inst->rdp_send_input_scancode(wfi->inst, 1, 0, 0x1D); /* Ctrl up */
						wfi->inst->rdp_send_input_scancode(wfi->inst, 1, 0, 0x45); /* NumLock up */
					}
					else
						DEBUG_KBD("Pause up\n");
					return 1;
				}
			}

			if ((scanCode == 0x36) && (flags & LLKHF_EXTENDED))
			{
				DEBUG_KBD("hack: right shift (x36) should not be extended\n");
				flags &= ~LLKHF_EXTENDED;
			}

			wfi->inst->rdp_send_input_scancode(wfi->inst, flags & LLKHF_UP, flags & LLKHF_EXTENDED, scanCode);
			if (p->vkCode == VK_CAPITAL)
				DEBUG_KBD("caps lock is processed on client side too to toggle caps lock indicator\n");
			else
				return 1;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

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
			wfi->inst->rdp_send_input_mouse(wfi->inst,
				PTRFLAGS_DOWN | PTRFLAGS_BUTTON1, X_POS(lParam), Y_POS(lParam));
		}
		break;

	case WM_LBUTTONUP:
		if (wfi != NULL)
		{
			wfi->inst->rdp_send_input_mouse(wfi->inst,
				PTRFLAGS_BUTTON1, X_POS(lParam), Y_POS(lParam));
		}
		break;

	case WM_RBUTTONDOWN:
		if (wfi != NULL)
		{
			wfi->inst->rdp_send_input_mouse(wfi->inst,
				PTRFLAGS_DOWN | PTRFLAGS_BUTTON2, X_POS(lParam), Y_POS(lParam));
		}
		break;

	case WM_RBUTTONUP:
		if (wfi != NULL)
		{
			wfi->inst->rdp_send_input_mouse(wfi->inst,
				PTRFLAGS_BUTTON2, X_POS(lParam), Y_POS(lParam));
		}
		break;

	case WM_MOUSEMOVE:
		if (wfi != NULL)
		{
			wfi->inst->rdp_send_input_mouse(wfi->inst,
				PTRFLAGS_MOVE, X_POS(lParam), Y_POS(lParam));
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

	case WM_SETFOCUS:
		DEBUG_KBD("getting focus %X\n", hWnd);
		g_focus_hWnd = hWnd;
		break;

	case WM_KILLFOCUS:
		DEBUG_KBD("loosing focus %X\n", hWnd);
		if (g_focus_hWnd == hWnd)
			g_focus_hWnd = NULL; // racy - but probably not in a fatal way
		break;

	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}
