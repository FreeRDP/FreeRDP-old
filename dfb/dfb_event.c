/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI Event Handling

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
#include <freerdp/freerdp.h>
#include "dfbfreerdp.h"
#include "dfb_event.h"
#include "dfb_keyboard.h"

int
dfb_process_event(rdpInst * inst, DFBEvent * event)
{
	int keycode;
	int cursor_x;
	int cursor_y;
	int device_flags;
	GDI *gdi = GET_GDI(inst);
	DFBInputEvent * input_event;

	cursor_x = gdi->cursor_x;
	cursor_y = gdi->cursor_y;
	
	if (event->clazz == DFEC_INPUT)
	{
		device_flags = 0;
		input_event = (DFBInputEvent *) event;

		switch (input_event->type)
		{
			case DIET_AXISMOTION:

				if (input_event->flags & DIEF_AXISABS)
				{
					if (input_event->axis == DIAI_X)
						cursor_x = input_event->axisabs;
					else if (input_event->axis == DIAI_Y)
						cursor_y = input_event->axisabs;
				}
				else if (input_event->flags & DIEF_AXISREL)
				{
					if (input_event->axis == DIAI_X)
						cursor_x += input_event->axisrel;
					else if (input_event->axis == DIAI_Y)
						cursor_y += input_event->axisrel;
				}

				if (cursor_x > (gdi->width - 1))
					cursor_x = gdi->width - 1;

				if (cursor_y > (gdi->height - 1))
					cursor_y = gdi->height - 1;
				
				inst->ui_move_pointer(inst, cursor_x, cursor_y);
				
				break;

			case DIET_BUTTONPRESS:

				if (input_event->button == DIBI_LEFT)
					device_flags = PTRFLAGS_DOWN | PTRFLAGS_BUTTON1;
				else if (input_event->button == DIBI_RIGHT)
					device_flags = PTRFLAGS_DOWN | PTRFLAGS_BUTTON2;
				else if (input_event->button == DIBI_MIDDLE)
					device_flags = PTRFLAGS_DOWN | PTRFLAGS_BUTTON3;

				if (device_flags != 0)
					inst->rdp_send_input(inst, RDP_INPUT_MOUSE, device_flags, cursor_x, cursor_y);
				
				break;

			case DIET_BUTTONRELEASE:

				if (input_event->button == DIBI_LEFT)
					device_flags = PTRFLAGS_BUTTON1;
				else if (input_event->button == DIBI_RIGHT)
					device_flags = PTRFLAGS_BUTTON2;
				else if (input_event->button == DIBI_MIDDLE)
					device_flags = PTRFLAGS_BUTTON3;

				if (device_flags != 0)
					inst->rdp_send_input(inst, RDP_INPUT_MOUSE, device_flags, cursor_x, cursor_y);
				
				break;
				
			case DIET_KEYPRESS:
				keycode = input_event->key_id - DIKI_UNKNOWN;
				dfb_kb_send_key(inst, RDP_KEYPRESS, keycode);
				break;

			case DIET_KEYRELEASE:
				keycode = input_event->key_id - DIKI_UNKNOWN;
				dfb_kb_send_key(inst, RDP_KEYPRESS, keycode);
				break;

			case DIET_UNKNOWN:
				break;
		}
	}

	return 1;
}
