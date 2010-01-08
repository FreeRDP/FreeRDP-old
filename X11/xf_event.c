
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freerdp.h"
#include "xf_event.h"
#include "xf_keyboard.h"

static int
xf_handle_event_Expose(rdpInst * inst, xfInfo * xfi, XEvent * xevent)
{
	int x;
	int y;
	int cx;
	int cy;

	if (xevent->xexpose.window == xfi->wnd)
	{
		x = xevent->xexpose.x;
		y = xevent->xexpose.y;
		cx = xevent->xexpose.width;
		cy = xevent->xexpose.height;
		XCopyArea(xfi->display, xfi->backstore, xfi->wnd, xfi->gc_default,
			x, y, cx, cy, x, y);
	}
	return 0;
}

static int
xf_handle_event_VisibilityNotify(rdpInst * inst, xfInfo * xfi, XEvent * xevent)
{
	if (xevent->xvisibility.window == xfi->wnd)
	{
		xfi->unobscured = xevent->xvisibility.state == VisibilityUnobscured;
	}
	return 0;
}

static int
xf_handle_event_MotionNotify(rdpInst * inst, xfInfo * xfi, XEvent * xevent)
{
	int x;
	int y;

	if (xevent->xmotion.window == xfi->wnd)
	{
		x = xevent->xmotion.x;
		y = xevent->xmotion.y;
		inst->rdp_send_input(inst, RDP_INPUT_MOUSE, PTRFLAGS_MOVE, x, y);
	}
	return 0;
}

static int
xf_handle_event_ButtonPress(rdpInst * inst, xfInfo * xfi, XEvent * xevent)
{
	int device_flags;
	int param1;
	int param2;

	if (xevent->xbutton.window == xfi->wnd)
	{
		device_flags = 0;
		param1 = 0;
		param2 = 0;
		switch (xevent->xbutton.button)
		{
			case 1:
				device_flags = PTRFLAGS_DOWN | PTRFLAGS_BUTTON1;
				param1 = xevent->xbutton.x;
				param2 = xevent->xbutton.y;
				break;
			case 2:
				device_flags = PTRFLAGS_DOWN | PTRFLAGS_BUTTON3;
				param1 = xevent->xbutton.x;
				param2 = xevent->xbutton.y;
				break;
			case 3:
				device_flags = PTRFLAGS_DOWN | PTRFLAGS_BUTTON2;
				param1 = xevent->xbutton.x;
				param2 = xevent->xbutton.y;
				break;
			case 4:
				device_flags = PTRFLAGS_WHEEL | 0x0078;
				break;
			case 5:
				device_flags = PTRFLAGS_WHEEL | PTRFLAGS_WHEEL_NEGATIVE | 0x0088;
				break;
		}
		if (device_flags != 0)
		{
			inst->rdp_send_input(inst, RDP_INPUT_MOUSE, device_flags,
				param1, param2);
		}
	}
	return 0;
}

static int
xf_handle_event_ButtonRelease(rdpInst * inst, xfInfo * xfi, XEvent * xevent)
{
	int device_flags;
	int param1;
	int param2;

	if (xevent->xbutton.window == xfi->wnd)
	{
		device_flags = 0;
		param1 = 0;
		param2 = 0;
		switch (xevent->xbutton.button)
		{
			case 1:
				device_flags = PTRFLAGS_BUTTON1;
				param1 = xevent->xbutton.x;
				param2 = xevent->xbutton.y;
				break;
			case 2:
				device_flags = PTRFLAGS_BUTTON3;
				param1 = xevent->xbutton.x;
				param2 = xevent->xbutton.y;
				break;
			case 3:
				device_flags = PTRFLAGS_BUTTON2;
				param1 = xevent->xbutton.x;
				param2 = xevent->xbutton.y;
				break;
		}
		if (device_flags != 0)
		{
			inst->rdp_send_input(inst, RDP_INPUT_MOUSE, device_flags,
				param1, param2);
		}
	}
	return 0;
}

static int
xf_handle_event_KeyPress(rdpInst * inst, xfInfo * xfi, XEvent * xevent)
{
	xf_kb_send_key(inst, RDP_KEYPRESS, xevent->xkey.keycode);
	return 0;
}

static int
xf_handle_event_KeyRelease(rdpInst * inst, xfInfo * xfi, XEvent * xevent)
{
	xf_kb_send_key(inst, RDP_KEYRELEASE, xevent->xkey.keycode);
	return 0;
}

static int
xf_handle_event_FocusIn(rdpInst * inst, xfInfo * xfi, XEvent * xevent)
{
	xf_kb_focus_in(inst);
	return 0;
}

static int
xf_handle_event_MappingNotify(rdpInst * inst, xfInfo * xfi, XEvent * xevent)
{
	if (xevent->xmapping.request == MappingModifier)
	{
		XFreeModifiermap(xfi->mod_map);
		xfi->mod_map = XGetModifierMapping(xfi->display);
	}
	return 0;
}

int
xf_handle_event(rdpInst * inst, xfInfo * xfi, XEvent * xevent)
{
	int rv;

	rv = 0;
	switch (xevent->type)
	{
		case Expose:
			rv = xf_handle_event_Expose(inst, xfi, xevent);
			break;
		case VisibilityNotify:
			rv = xf_handle_event_VisibilityNotify(inst, xfi, xevent);
			break;
		case MotionNotify:
			rv = xf_handle_event_MotionNotify(inst, xfi, xevent);
			break;
		case ButtonPress:
			rv = xf_handle_event_ButtonPress(inst, xfi, xevent);
			break;
		case ButtonRelease:
			rv = xf_handle_event_ButtonRelease(inst, xfi, xevent);
			break;
		case KeyPress:
			rv = xf_handle_event_KeyPress(inst, xfi, xevent);
			break;
		case KeyRelease:
			rv = xf_handle_event_KeyRelease(inst, xfi, xevent);
			break;
		case FocusIn:
			rv = xf_handle_event_FocusIn(inst, xfi, xevent);
			break;
		case FocusOut:
			break;
		case EnterNotify:
			/*printf("xf_handle_event: EnterNotify\n");*/
			break;
		case LeaveNotify:
			/*printf("xf_handle_event: LeaveNotify\n");*/
			break;
		case NoExpose:
			printf("xf_handle_event: NoExpose\n");
			break;
		case GraphicsExpose:
			printf("xf_handle_event: GraphicsExpose\n");
			break;
		case ConfigureNotify:
			printf("xf_handle_event: ConfigureNotify\n");
			break;
		case MapNotify:
			printf("xf_handle_event: MapNotify\n");
			break;
		case ReparentNotify:
			printf("xf_handle_event: ReparentNotify\n");
			break;
		case MappingNotify:
			rv = xf_handle_event_MappingNotify(inst, xfi, xevent);
			break;
		default:
			printf("xf_handle_event unknown event %d\n", xevent->type);
			break;
	}
	return rv;
}
