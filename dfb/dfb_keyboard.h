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

#ifndef __DFB_KEYBOARD_H
#define __DFB_KEYBOARD_H

#include <freerdp/freerdp.h>

void
dfb_kb_init(void);
void
dfb_kb_send_key(rdpInst * inst, uint16 flags, uint8 keycode);
int
dfb_kb_get_toggle_keys_state(rdpInst * inst);
void
dfb_kb_focus_in(rdpInst * inst);

#endif /* __DFB_KEYBOARD_H */
