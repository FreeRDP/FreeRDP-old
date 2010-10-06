/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI Keyboard Handling

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libfreerdpkbd/keyboard.h"
#include <freerdp/freerdp.h>
#include <freerdp/kbd.h>
#include "dfb_event.h"

unsigned char keymap[256];
	
void
dfb_kb_init(void)
{
	memset(keymap, 0, sizeof(keymap));

	/* Map DirectFB keycodes to Virtual Key Codes */
	
	keymap[DIKI_A - DIKI_UNKNOWN] = VK_KEY_A;
	keymap[DIKI_B - DIKI_UNKNOWN] = VK_KEY_B;
	keymap[DIKI_C - DIKI_UNKNOWN] = VK_KEY_C;
	keymap[DIKI_D - DIKI_UNKNOWN] = VK_KEY_D;
	keymap[DIKI_E - DIKI_UNKNOWN] = VK_KEY_E;
	keymap[DIKI_F - DIKI_UNKNOWN] = VK_KEY_F;
	keymap[DIKI_G - DIKI_UNKNOWN] = VK_KEY_G;
	keymap[DIKI_H - DIKI_UNKNOWN] = VK_KEY_H;
	keymap[DIKI_I - DIKI_UNKNOWN] = VK_KEY_I;
	keymap[DIKI_J - DIKI_UNKNOWN] = VK_KEY_J;
	keymap[DIKI_K - DIKI_UNKNOWN] = VK_KEY_K;
	keymap[DIKI_L - DIKI_UNKNOWN] = VK_KEY_L;
	keymap[DIKI_M - DIKI_UNKNOWN] = VK_KEY_M;
	keymap[DIKI_N - DIKI_UNKNOWN] = VK_KEY_N;
	keymap[DIKI_O - DIKI_UNKNOWN] = VK_KEY_O;
	keymap[DIKI_P - DIKI_UNKNOWN] = VK_KEY_P;
	keymap[DIKI_Q - DIKI_UNKNOWN] = VK_KEY_Q;
	keymap[DIKI_R - DIKI_UNKNOWN] = VK_KEY_R;
	keymap[DIKI_S - DIKI_UNKNOWN] = VK_KEY_S;
	keymap[DIKI_T - DIKI_UNKNOWN] = VK_KEY_T;
	keymap[DIKI_U - DIKI_UNKNOWN] = VK_KEY_U;
	keymap[DIKI_V - DIKI_UNKNOWN] = VK_KEY_V;
	keymap[DIKI_W - DIKI_UNKNOWN] = VK_KEY_W;
	keymap[DIKI_X - DIKI_UNKNOWN] = VK_KEY_X;
	keymap[DIKI_Y - DIKI_UNKNOWN] = VK_KEY_Y;
	keymap[DIKI_Z - DIKI_UNKNOWN] = VK_KEY_Z;

	keymap[DIKI_0 - DIKI_UNKNOWN] = VK_KEY_0;
	keymap[DIKI_1 - DIKI_UNKNOWN] = VK_KEY_1;
	keymap[DIKI_2 - DIKI_UNKNOWN] = VK_KEY_2;
	keymap[DIKI_3 - DIKI_UNKNOWN] = VK_KEY_3;
	keymap[DIKI_4 - DIKI_UNKNOWN] = VK_KEY_4;
	keymap[DIKI_5 - DIKI_UNKNOWN] = VK_KEY_5;
	keymap[DIKI_6 - DIKI_UNKNOWN] = VK_KEY_6;
	keymap[DIKI_7 - DIKI_UNKNOWN] = VK_KEY_7;
	keymap[DIKI_8 - DIKI_UNKNOWN] = VK_KEY_8;
	keymap[DIKI_9 - DIKI_UNKNOWN] = VK_KEY_9;
	
	keymap[DIKI_F1 - DIKI_UNKNOWN] = VK_F1;
	keymap[DIKI_F2 - DIKI_UNKNOWN] = VK_F2;
	keymap[DIKI_F3 - DIKI_UNKNOWN] = VK_F3;
	keymap[DIKI_F4 - DIKI_UNKNOWN] = VK_F4;
	keymap[DIKI_F5 - DIKI_UNKNOWN] = VK_F5;
	keymap[DIKI_F6 - DIKI_UNKNOWN] = VK_F6;
	keymap[DIKI_F7 - DIKI_UNKNOWN] = VK_F7;
	keymap[DIKI_F8 - DIKI_UNKNOWN] = VK_F8;
	keymap[DIKI_F9 - DIKI_UNKNOWN] = VK_F9;
	keymap[DIKI_F10 - DIKI_UNKNOWN] = VK_F10;
	keymap[DIKI_F11 - DIKI_UNKNOWN] = VK_F11;
	keymap[DIKI_F12 - DIKI_UNKNOWN] = VK_F12;
}

void
dfb_kb_send_key(rdpInst * inst, int flags, uint8 keycode)
{
	int scancode = freerdp_kbd_get_scancode_by_virtualkey(keymap[keycode]);
	inst->rdp_send_input(inst, RDP_INPUT_SCANCODE, flags, scancode, 0);
}

int
dfb_kb_get_toggle_keys_state(rdpInst * inst)
{
	return 0;
}

void
dfb_kb_focus_in(rdpInst * inst)
{

}

