
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "freerdp.h"
#include "xf_event.h"

static int
xf_kb_set_km_item(xfInfo * xfi, KeySym key, int rdp_scancode, int rdp_flags)
{
	int x11_scancode;

	x11_scancode = XKeysymToKeycode(xfi->display, key);
	if ((x11_scancode > 0) && (x11_scancode < 256))
	{
		xfi->km[x11_scancode].scancode = rdp_scancode;
		xfi->km[x11_scancode].flags = rdp_flags;
	}
	return x11_scancode;
}

void
xf_kb_init(rdpInst * inst)
{
	xfInfo * xfi;

	xfi = GET_XFI(inst);
	xf_kb_set_km_item(xfi, XK_Escape, 1, 0);
	xf_kb_set_km_item(xfi, XK_Return, 28, 0);
	xf_kb_set_km_item(xfi, XK_Up, 72, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_Left, 75, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_Down, 80, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_Right, 77, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_BackSpace, 14, 0);
	xf_kb_set_km_item(xfi, XK_Shift_L, 42, 0);
	xf_kb_set_km_item(xfi, XK_Shift_R, 54, 0);
	xf_kb_set_km_item(xfi, XK_Alt_L, 56, 0);
	if (xf_kb_set_km_item(xfi, XK_Alt_R, 56, KBD_FLAG_EXT) == 0)
	{
		xf_kb_set_km_item(xfi, XK_Mode_switch, 56, KBD_FLAG_EXT);
	}
	xf_kb_set_km_item(xfi, XK_Control_L, 29, 0);
	xf_kb_set_km_item(xfi, XK_Control_R, 29, KBD_FLAG_EXT);
	xfi->tab_key = xf_kb_set_km_item(xfi, XK_Tab, 15, 0);
	xf_kb_set_km_item(xfi, XK_Caps_Lock, 58, 0);
	xf_kb_set_km_item(xfi, XK_space, 57, 0);
	xf_kb_set_km_item(xfi, XK_Super_L, 91, KBD_FLAG_EXT);
	if (xf_kb_set_km_item(xfi, XK_Super_R, 92, KBD_FLAG_EXT) == 0)
	{
		xf_kb_set_km_item(xfi, XK_Multi_key, 92, KBD_FLAG_EXT);
	}
	xf_kb_set_km_item(xfi, XK_Menu, 93, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_F1, 59, 0);
	xf_kb_set_km_item(xfi, XK_F2, 60, 0);
	xf_kb_set_km_item(xfi, XK_F3, 61, 0);
	xf_kb_set_km_item(xfi, XK_F4, 62, 0);
	xf_kb_set_km_item(xfi, XK_F5, 63, 0);
	xf_kb_set_km_item(xfi, XK_F6, 64, 0);
	xf_kb_set_km_item(xfi, XK_F7, 65, 0);
	xf_kb_set_km_item(xfi, XK_F8, 66, 0);
	xf_kb_set_km_item(xfi, XK_F9, 67, 0);
	xf_kb_set_km_item(xfi, XK_F10, 68, 0);
	xf_kb_set_km_item(xfi, XK_F11, 87, 0);
	xf_kb_set_km_item(xfi, XK_F12, 88, 0);
	xf_kb_set_km_item(xfi, XK_Print, 55, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_Scroll_Lock, 70, 0);
	xfi->pause_key = xf_kb_set_km_item(xfi, XK_Pause, 69, 0);
	xf_kb_set_km_item(xfi, XK_Insert, 82, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_Home, 71, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_Prior, 73, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_Delete, 83, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_End, 79, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_Next, 81, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_Num_Lock, 69, 0);
	xf_kb_set_km_item(xfi, XK_KP_Divide, 53, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_KP_Multiply, 55, 0);
	xf_kb_set_km_item(xfi, XK_KP_Subtract, 74, 0);
	xf_kb_set_km_item(xfi, XK_KP_Insert, 82, 0);
	xf_kb_set_km_item(xfi, XK_KP_End, 79, 0);
	xf_kb_set_km_item(xfi, XK_KP_Down, 80, 0);
	xf_kb_set_km_item(xfi, XK_KP_Next, 81, 0);
	xf_kb_set_km_item(xfi, XK_KP_Left, 75, 0);
	xf_kb_set_km_item(xfi, XK_KP_Begin, 76, 0);
	xf_kb_set_km_item(xfi, XK_KP_Right, 77, 0);
	xf_kb_set_km_item(xfi, XK_KP_Home, 71, 0);
	xf_kb_set_km_item(xfi, XK_KP_Up, 72, 0);
	xf_kb_set_km_item(xfi, XK_KP_Prior, 73, 0);
	xf_kb_set_km_item(xfi, XK_KP_Delete, 83, 0);
	xf_kb_set_km_item(xfi, XK_KP_Enter, 28, KBD_FLAG_EXT);
	xf_kb_set_km_item(xfi, XK_KP_Add, 78, 0);

	xfi->km[10].scancode = 2; /* 1 */
	xfi->km[11].scancode = 3; /* 2 */
	xfi->km[12].scancode = 4; /* 3 */
	xfi->km[13].scancode = 5; /* 4 */
	xfi->km[14].scancode = 6; /* 5 */
	xfi->km[15].scancode = 7; /* 6 */
	xfi->km[16].scancode = 8; /* 7 */
	xfi->km[17].scancode = 9; /* 8 */
	xfi->km[18].scancode = 10; /* 9 */
	xfi->km[19].scancode = 11; /* 0 */
	xfi->km[20].scancode = 12; /* - */
	xfi->km[21].scancode = 13; /* = */

	xfi->km[24].scancode = 16; /* Q */
	xfi->km[25].scancode = 17; /* W */
	xfi->km[26].scancode = 18; /* E */
	xfi->km[27].scancode = 19; /* R */
	xfi->km[28].scancode = 20; /* T */
	xfi->km[29].scancode = 21; /* Y */
	xfi->km[30].scancode = 22; /* U */
	xfi->km[31].scancode = 23; /* I */
	xfi->km[32].scancode = 24; /* O */
	xfi->km[33].scancode = 25; /* P */
	xfi->km[34].scancode = 26; /* { */
	xfi->km[35].scancode = 27; /* [ */
	xfi->km[36].scancode = 28; /* ] */

	xfi->km[38].scancode = 30; /* A */
	xfi->km[39].scancode = 31; /* S */
	xfi->km[40].scancode = 32; /* D */
	xfi->km[41].scancode = 33; /* F */
	xfi->km[42].scancode = 34; /* G */
	xfi->km[43].scancode = 35; /* H */
	xfi->km[44].scancode = 36; /* J */
	xfi->km[45].scancode = 37; /* K */
	xfi->km[46].scancode = 38; /* L */
	xfi->km[47].scancode = 39; /* ; */
	xfi->km[48].scancode = 40; /* ' */
	xfi->km[49].scancode = 41; /* ` */

	xfi->km[51].scancode = 43; /* \ */
	xfi->km[52].scancode = 44; /* Z */
	xfi->km[53].scancode = 45; /* X */
	xfi->km[54].scancode = 46; /* C */
	xfi->km[55].scancode = 47; /* V */
	xfi->km[56].scancode = 48; /* B */
	xfi->km[57].scancode = 49; /* N */
	xfi->km[58].scancode = 50; /* M */
	xfi->km[59].scancode = 51; /* , */
	xfi->km[60].scancode = 52; /* . */
	xfi->km[61].scancode = 53; /* / */
};

void
xf_kb_send_key(rdpInst * inst, uint16 aflags, uint8 keycode)
{
	xfInfo * xfi;
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
		scancode = xfi->km[keycode].scancode;
		flags = xfi->km[keycode].flags | aflags;
		inst->rdp_send_input(inst, RDP_INPUT_SCANCODE, flags, scancode, 0);
		//printf("X11 keycode %2.2X RDP scancode %2.2X RDP flags %4.4X\n",
		//	keycode, scancode, flags);
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
	scancode = xfi->km[xfi->tab_key].scancode;
	inst->rdp_send_input(inst, RDP_INPUT_SCANCODE, KBD_FLAG_UP, scancode, 0);
	/* sync num, caps, scroll, kana lock */
	flags = xf_kb_get_toggle_keys_state(xfi);
	inst->rdp_sync_input(inst, flags);
}
