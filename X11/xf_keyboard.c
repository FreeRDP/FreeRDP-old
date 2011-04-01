/*
   FreeRDP: A Remote Desktop Protocol client.
   Keyboard

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <freerdp/freerdp.h>
#include <freerdp/kbd.h>
#include "xf_types.h"
#include "xf_win.h"
#include "xf_keyboard.h"

static int xf_kb_keyboard_layout = 0;
static RD_BOOL pressed_keys[256] = { False };

void
xf_kb_set_keypress(uint8 keycode, KeySym keysym)
{
	if (keycode >= 8)
		pressed_keys[keycode] = keysym;
	else
		return;
}

void
xf_kb_unset_keypress(uint8 keycode)
{
	if (keycode >= 8)
		pressed_keys[keycode] = NoSymbol;
	else
		return;
}

static RD_BOOL
xf_kb_key_pressed(xfInfo * xfi, KeySym keysym)
{
	KeyCode keycode = XKeysymToKeycode(xfi->display, keysym);
	return pressed_keys[keycode] == keysym;
}

void
xf_kb_init(Display *dpy, unsigned int keyboard_layout_id)
{
	xf_kb_keyboard_layout = freerdp_kbd_init(dpy, keyboard_layout_id);
	DEBUG_KBD("freerdp_kbd_init: %X\n", xf_kb_keyboard_layout);
}

void
xf_kb_inst_init(xfInfo * xfi)
{
	xfi->inst->settings->keyboard_layout = xf_kb_keyboard_layout;
};

void
xf_kb_send_key(xfInfo * xfi, RD_BOOL up, uint8 keycode)
{
	uint8 scancode;
	RD_BOOL extended;

	scancode = freerdp_kbd_get_scancode_by_keycode(keycode, &extended);
	if (scancode == 0)
	{
		printf("xf_kb_send_key: unknown key %s keycode=%d (X keysym=0x%04X)\n",
			up ? "up" : "down", keycode, (unsigned int)XKeycodeToKeysym(xfi->display, keycode, 0));
	}
	else
	{
		DEBUG_KBD("%s keycode=%d (X keysym=0x%04X) as extended=%d scancode=%d\n",
			up ? "up" : "down", keycode, (unsigned int)XKeycodeToKeysym(xfi->display, keycode, 0),
			!!extended, scancode);

		xfi->inst->rdp_send_input_scancode(xfi->inst, up, extended, scancode);

		if ((scancode == 0x3A) && up) /* caps lock was released */
		{
			/* caps lock state haven't necessarily been toggled locally - better set remote state explicitly */
			DEBUG_KBD("sending extra caps lock synchronization\n");
			xfi->inst->rdp_sync_input(xfi->inst, xf_kb_get_toggle_keys_state(xfi));
		}
	}
}

static int
xf_kb_read_keyboard_state(xfInfo * xfi)
{
	uint32 state;
	Window wdummy;
	int dummy;

	XQueryPointer(xfi->display, xfi->wnd, &wdummy, &wdummy, &dummy, &dummy,
		&dummy, &dummy, &state);
	return state;
}

static RD_BOOL
xf_kb_get_key_state(xfInfo * xfi, int state, int keysym)
{
	int modifierpos, key, keysymMask = 0;
	int offset;
	KeyCode keycode = XKeysymToKeycode(xfi->display, keysym);

	if (keycode == NoSymbol)
	{
		return False;
	}
	for (modifierpos = 0; modifierpos < 8; modifierpos++)
	{
		offset = xfi->mod_map->max_keypermod * modifierpos;
		for (key = 0; key < xfi->mod_map->max_keypermod; key++)
		{
			if (xfi->mod_map->modifiermap[offset + key] == keycode)
			{
				keysymMask |= 1 << modifierpos;
			}
		}
	}
	return (state & keysymMask) ? True : False;
}

int
xf_kb_get_toggle_keys_state(xfInfo * xfi)
{
	int toggle_keys_state = 0;
	int state;

	state = xf_kb_read_keyboard_state(xfi);
	if (xf_kb_get_key_state(xfi, state, XK_Scroll_Lock))
	{
		toggle_keys_state |= KBD_SYNC_SCROLL_LOCK;
	}
	if (xf_kb_get_key_state(xfi, state, XK_Num_Lock))
	{
		toggle_keys_state |= KBD_SYNC_NUM_LOCK;
	}
	if (xf_kb_get_key_state(xfi, state, XK_Caps_Lock))
	{
		toggle_keys_state |= KBD_SYNC_CAPS_LOCK;
	}
	if (xf_kb_get_key_state(xfi, state, XK_Kana_Lock))
	{
		toggle_keys_state |= KBD_SYNC_KANA_LOCK;
	}
	return toggle_keys_state;
}

void
xf_kb_focus_in(xfInfo * xfi)
{
	int flags;

	/* on focus in send a tab up like mstsc.exe */
	xfi->inst->rdp_send_input_scancode(xfi->inst, True, False, 15);

	/* sync num, caps, scroll, kana lock */
	flags = xf_kb_get_toggle_keys_state(xfi);
	xfi->inst->rdp_sync_input(xfi->inst, flags);
}

RD_BOOL
xf_kb_handle_special_keys(xfInfo * xfi, KeySym keysym)
{
	if (keysym == XK_Return)
	{
		if ((xf_kb_key_pressed(xfi, XK_Alt_L) || xf_kb_key_pressed(xfi, XK_Alt_R))
		    && (xf_kb_key_pressed(xfi, XK_Control_L) || xf_kb_key_pressed(xfi, XK_Control_R)))
		{
			/* Ctrl-Alt-Enter: toggle full screen */
			xf_toggle_fullscreen(xfi);
			return True;
		}
	}

	return False;
}
