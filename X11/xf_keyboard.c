
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "freerdp.h"
#include "libkbd.h"
#include "xf_event.h"

void
xf_kb_init(rdpInst * inst)
{
	inst->settings->keyboard_layout = kbd_init();
	printf("kbd_init: %X\n", inst->settings->keyboard_layout);
};

void
xf_kb_send_key(rdpInst * inst, uint16 aflags, uint8 keycode)
{
	xfInfo * xfi;
	int vkcode;
	int scancode;
	int flags;

	xfi = GET_XFI(inst);
	if (keycode == xfi->pause_key)
	{
		/* This is a special key the actually sends two scancodes to the
		   server.  It looks like Control - NumLock but with special flags. */
		//printf("special VK_PAUSE\n");
		if (aflags & KBD_FLAG_UP)
		{
			inst->rdp_send_input(inst, RDP_INPUT_SCANCODE, 0x8200, 0x1d, 0);
			inst->rdp_send_input(inst, RDP_INPUT_SCANCODE, 0x8000, 0x45, 0);
		}
		else
		{
			inst->rdp_send_input(inst, RDP_INPUT_SCANCODE, 0x0200, 0x1d, 0);
			inst->rdp_send_input(inst, RDP_INPUT_SCANCODE, 0x0000, 0x45, 0);
		}
	}
	else
	{
		vkcode = keycodeToVkcode[keycode];
		scancode = virtualKeyboard[vkcode].scancode;
		flags = virtualKeyboard[vkcode].flags | aflags;
		inst->rdp_send_input(inst, RDP_INPUT_SCANCODE, flags, scancode, 0);
	}
}

static int
xf_kb_read_keyboard_state(xfInfo * xfi)
{
	uint32 state;
	Window wdummy;
	int dummy;

	XQueryPointer(xfi->display, xfi->wnd, &wdummy, &wdummy, &dummy, &dummy,
		&dummy, &dummy, &state);
	return state;
}

static RD_BOOL
xf_kb_get_key_state(xfInfo * xfi, int state, int keysym)
{
	int modifierpos, key, keysymMask = 0;
	int offset;
	KeyCode keycode = XKeysymToKeycode(xfi->display, keysym);

	if (keycode == NoSymbol)
	{
		return False;
	}
	for (modifierpos = 0; modifierpos < 8; modifierpos++)
	{
		offset = xfi->mod_map->max_keypermod * modifierpos;
		for (key = 0; key < xfi->mod_map->max_keypermod; key++)
		{
			if (xfi->mod_map->modifiermap[offset + key] == keycode)
			{
				keysymMask |= 1 << modifierpos;
			}
		}
	}
	return (state & keysymMask) ? True : False;
}

int
xf_kb_get_toggle_keys_state(xfInfo * xfi)
{
	int toggle_keys_state = 0;
	int state;

	state = xf_kb_read_keyboard_state(xfi);
	if (xf_kb_get_key_state(xfi, state, XK_Scroll_Lock))
	{
		toggle_keys_state |= KBD_SYNC_SCROLL_LOCK;
	}
	if (xf_kb_get_key_state(xfi, state, XK_Num_Lock))
	{
		toggle_keys_state |= KBD_SYNC_NUM_LOCK;
	}
	if (xf_kb_get_key_state(xfi, state, XK_Caps_Lock))
	{
		toggle_keys_state |= KBD_SYNC_CAPS_LOCK;
	}
	if (xf_kb_get_key_state(xfi, state, XK_Kana_Lock))
	{
		toggle_keys_state |= KBD_SYNC_KANA_LOCK;
	}
	return toggle_keys_state;
}

void
xf_kb_focus_in(rdpInst * inst)
{
	xfInfo * xfi;
	int flags;
	int scancode;

	xfi = GET_XFI(inst);
	/* on focus in send a tab up like mstsc.exe */
	scancode = virtualKeyboard[xfi->tab_key].scancode;
	inst->rdp_send_input(inst, RDP_INPUT_SCANCODE, KBD_FLAG_UP, scancode, 0);
	/* sync num, caps, scroll, kana lock */
	flags = xf_kb_get_toggle_keys_state(xfi);
	inst->rdp_sync_input(inst, flags);
}

