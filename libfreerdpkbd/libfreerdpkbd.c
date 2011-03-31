/*
   FreeRDP: A Remote Desktop Protocol client.
   XKB-based Keyboard Mapping to Microsoft Keyboard System

   Copyright 2009 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#include <freerdp/freerdp.h>
#include <freerdp/kbd.h>
#include "config.h"
#include "debug.h"
#include "locales.h"
#include "layout_ids.h"
#include "layouts_xkb.h"
#include "keyboard.h"

/* The actual mapping from X keycodes to RDP keycodes,
   initialized from xkb keycodes or similar.
   Used directly by freerdp_kbd_get_scancode_by_keycode.
   The mapping is a global variable, but it only depends on which keycodes
   the X servers keyboard driver uses and is thus very static. */

RdpKeycodes x_keycode_to_rdp_keycode;

static unsigned int
detect_keyboard(void *dpy, unsigned int keyboardLayoutID, char *xkbfile, size_t xkbfilelength)
{
	xkbfile[0] = '\0';

	if (keyboardLayoutID != 0)
		DEBUG_KBD("keyboard layout configuration: %X\n", keyboardLayoutID);

#if defined(sun)
	if(keyboardLayoutID == 0)
	{
		keyboardLayoutID = detect_keyboard_type_and_layout_sunos(xkbfile, xkbfilelength);
		DEBUG_KBD("detect_keyboard_type_and_layout_sunos: %X %s\n", keyboardLayoutID, xkbfile);
	}
#endif

#ifdef WITH_XKBFILE
	if(keyboardLayoutID == 0)
	{
		keyboardLayoutID = detect_keyboard_layout_from_xkb(dpy);
		DEBUG_KBD("detect_keyboard_layout_from_xkb: %X\n", keyboardLayoutID);
	}
#endif

	if(keyboardLayoutID == 0)
	{
		keyboardLayoutID = detect_keyboard_layout_from_locale();
		DEBUG_KBD("detect_keyboard_layout_from_locale: %X\n", keyboardLayoutID);
	}

	if (keyboardLayoutID == 0)
	{
		keyboardLayoutID = 0x0409;
		DEBUG_KBD("using default keyboard layout: %X\n", keyboardLayoutID);
	}

	if (xkbfile[0] == '\0')
	{
		strncpy(xkbfile, "base", xkbfilelength);
#ifdef WITH_XKBFILE
		detect_keyboard_type_from_xkb(dpy, xkbfile, xkbfilelength);
#endif
		DEBUG_KBD("detect_keyboard_type_from_xkb: %s\n", xkbfile);
	}

	return keyboardLayoutID;
}

/* Initialize global keyboard mapping and return the suggested server side layout.
   dpy must be a X Display* or NULL. */
unsigned int
freerdp_kbd_init(void *dpy, unsigned int keyboard_layout_id)
{
#ifdef WITH_XKBFILE
	if (!init_xkb(dpy))
	{
		printf("Error initializing xkb\n");
		return 0;
	}
#endif
	char xkbfile[256];
	KeycodeToVkcode keycodeToVkcode;
	int keycode;

	keyboard_layout_id = detect_keyboard(dpy, keyboard_layout_id, xkbfile, sizeof(xkbfile));
	printf("Using keyboard layout 0x%X with xkb name %s and xkbfile %s\n",
			keyboard_layout_id, get_layout_name(keyboard_layout_id), xkbfile);

	load_keyboard_map(keycodeToVkcode, xkbfile);

	for (keycode=0; keycode<256; keycode++)
	{
		int vkcode;
		vkcode = keycodeToVkcode[keycode];
		x_keycode_to_rdp_keycode[keycode].keycode = virtualKeyboard[vkcode].scancode;
		x_keycode_to_rdp_keycode[keycode].extended = virtualKeyboard[vkcode].flags == KBD_EXT;
	}

	return keyboard_layout_id;
}

rdpKeyboardLayout *
freerdp_kbd_get_layouts(int types)
{
	return get_keyboard_layouts(types);
}

uint8
freerdp_kbd_get_scancode_by_keycode(uint8 keycode, RD_BOOL * extended)
{
	*extended = x_keycode_to_rdp_keycode[keycode].extended;
	return x_keycode_to_rdp_keycode[keycode].keycode;
}

uint8
freerdp_kbd_get_scancode_by_virtualkey(int vkcode, RD_BOOL * extended)
{
	*extended = virtualKeyboard[vkcode].flags == KBD_EXT;
	return virtualKeyboard[vkcode].scancode;
}
