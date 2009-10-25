
#ifndef __XF_KEYBOARD_H
#define __XF_KEYBOARD_H

#include "freerdp.h"

void
xf_kb_init(rdpInst * inst);
void
xf_kb_send_key(rdpInst * inst, uint16 flags, uint8 keycode);
int
xf_kb_get_toggle_keys_state(xfInfo * xfi);
void
xf_kb_focus_in(rdpInst * inst);

#endif
