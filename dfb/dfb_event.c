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
	int mouse_x;
	int mouse_y;
	GDI *gdi = GET_GDI(inst);
	DFBInputEvent * input_event;

	mouse_x = gdi->cursor_x;
	mouse_y = gdi->cursor_y;
	
	if (event->clazz == DFEC_INPUT)
	{
		input_event = (DFBInputEvent *) event;

		switch (input_event->type)
		{
			case DIET_AXISMOTION:

				if (input_event->flags & DIEF_AXISABS)
				{
					if (input_event->axis == DIAI_X)
						mouse_x = input_event->axisabs;
					else if (input_event->axis == DIAI_Y)
						mouse_y = input_event->axisabs;
				}
				else if (input_event->flags & DIEF_AXISREL)
				{
					if (input_event->axis == DIAI_X)
						mouse_x += input_event->axisrel;
					else if (input_event->axis == DIAI_Y)
						mouse_y += input_event->axisrel;
				}

				inst->ui_move_pointer(inst, mouse_x, mouse_y);
				break;

			case DIET_BUTTONPRESS:
				break;

			case DIET_BUTTONRELEASE:
				break;

			case DIET_KEYPRESS:
				break;

			case DIET_KEYRELEASE:
				break;

			case DIET_UNKNOWN:
				break;
		}
	}

	return 1;
}
