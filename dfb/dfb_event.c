/*
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI Event Handling

   Copyright 2010 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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
	dfbInfo *dfbi = GET_DFBI(inst);

	DFBInputEvent * input_event;

	dfbi->layer->GetCursorPosition(dfbi->layer, &cursor_x, &cursor_y);

	if (event->clazz == DFEC_INPUT)
	{
		device_flags = 0;
		input_event = (DFBInputEvent *) event;

		switch (input_event->type)
		{
			case DIET_AXISMOTION:

				if (cursor_x > (gdi->width - 1))
					cursor_x = gdi->width - 1;

				if (cursor_y > (gdi->height - 1))
					cursor_y = gdi->height - 1;

				inst->ui_move_pointer(inst, cursor_x, cursor_y);

				break;

			case DIET_BUTTONPRESS:
				device_flags = PTRFLAGS_DOWN;
				/* fall */

			case DIET_BUTTONRELEASE:
				if (input_event->button == DIBI_LEFT)
					device_flags |= PTRFLAGS_BUTTON1;
				else if (input_event->button == DIBI_RIGHT)
					device_flags |= PTRFLAGS_BUTTON2;
				else if (input_event->button == DIBI_MIDDLE)
					device_flags |= PTRFLAGS_BUTTON3;
				
				/* printf("Button=> @(%d, %d) %x\n", cursor_x, cursor_y, device_flags); */
				if (device_flags != 0)
					inst->rdp_send_input_mouse(inst, device_flags, cursor_x, cursor_y);

				break;

			case DIET_KEYPRESS:
				keycode = input_event->key_id - DIKI_UNKNOWN;
				dfb_kb_send_key(inst, 0, keycode);
				break;

			case DIET_KEYRELEASE:
				keycode = input_event->key_id - DIKI_UNKNOWN;
				dfb_kb_send_key(inst, 1, keycode);
				break;

			case DIET_UNKNOWN:
				break;
		}
	}

	return 1;
}
