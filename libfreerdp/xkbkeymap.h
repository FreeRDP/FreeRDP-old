/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   User interface services - X keyboard mapping using XKB

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2009

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

#ifndef __XKBKEYMAP_H
#define	__XKBKEYMAP_H

int
xkbkeymap_init();
uint16
xkb_translate_button(uint32 button);
void
xkb_handle_special_keys(void * inst, uint32 time, uint16 flags, uint8 keycode);
void
xkbkeymap_send_key(uint32 time, uint16 flags, uint8 keycode);
void
xkb_handle_focus_in(void * inst, uint32 time);

#endif // __XKBKEYMAP_H

