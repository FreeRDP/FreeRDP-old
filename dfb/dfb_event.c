
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freerdp/freerdp.h>
#include "dfb_event.h"
#include "dfb_keyboard.h"

#if 0

static int
dfb_handle_event_Expose(rdpInst * inst)
{

	return 0;
}

static int
dfb_handle_event_VisibilityNotify(rdpInst * inst)
{

	return 0;
}

static int
dfb_handle_event_MotionNotify(rdpInst * inst)
{

	return 0;
}

static int
dfb_handle_event_ButtonPress(rdpInst * inst)
{

	return 0;
}

static int
dfb_handle_event_ButtonRelease(rdpInst * inst)
{

	return 0;
}

static int
dfb_handle_event_KeyPress(rdpInst * inst)
{
	//dfb_kb_send_key(inst, RDP_KEYPRESS, xevent->xkey.keycode);
	return 0;
}

static int
dfb_skip_key_release(rdpInst * inst)
{
	return 0;
}

static int
dfb_handle_event_KeyRelease(rdpInst * inst)
{
	//dfb_kb_send_key(inst, RDP_KEYRELEASE, xevent->xkey.keycode);
	return 0;
}

static int
dfb_handle_event_FocusIn(rdpInst * inst)
{
	//dfb_kb_focus_in(inst);
	return 0;
}

static int
dfb_handle_event_MappingNotify(rdpInst * inst)
{
	return 0;
}

#endif

int
dfb_handle_event(rdpInst * inst)
{
	return 0;
}
