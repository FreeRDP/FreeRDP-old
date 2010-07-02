
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freerdp/freerdp.h>
#include "dfb_event.h"
#include "dfb_keyboard.h"

int
dfb_process_event(rdpInst * inst, DFBEvent * event)
{
	int cursor_x;
	int cursor_y;
	dfbInfo * dfbi;
	DFBInputEvent * input_event;
	
	dfbi = GET_DFBI(inst);
	cursor_x = dfbi->cursor_x;
	cursor_y = dfbi->cursor_y;
	
	if (event->clazz == DFEC_INPUT)
	{
		input_event = (DFBInputEvent *) event;

		if (input_event->type == DIET_AXISMOTION)
		{
			//printf("DIET_AXISMOTION\n");
			if (input_event->flags & DIEF_AXISABS)
			{
				if (input_event->axis == DIAI_X)
					cursor_x = input_event->axisabs;
				else if (input_event->axis == DIAI_Y)
					cursor_y = input_event->axisabs;
			}
			if (input_event->flags & DIEF_AXISREL)
			{
				if (input_event->axis == DIAI_X)
					cursor_x += input_event->axisrel;
				else if (input_event->axis == DIAI_Y)
					cursor_y += input_event->axisrel;
			}
			
			inst->ui_move_pointer(inst, cursor_x, cursor_y);
		}
		else if (input_event->type == DIET_BUTTONPRESS)
		{
			printf("DIET_BUTTONPRESS\n");
		}
		else if (input_event->type == DIET_BUTTONRELEASE)
		{
			printf("DIET_BUTTONRELEASE\n");
		}
		else if (input_event->type == DIET_KEYPRESS)
		{
			printf("DIET_KEYPRESS\n");
		}
		else if (input_event->type == DIET_KEYRELEASE)
		{
			printf("DIET_KEYRELEASE\n");
		}
	}
	
	//dfbi->primary->SetColor(dfbi->primary, 0xFF, 0xFF, 0xFF, 0xFF);
	//dfbi->primary->FillRectangle(dfbi->primary, cursor_x, cursor_y, 16, 16);
	return 1;
}
