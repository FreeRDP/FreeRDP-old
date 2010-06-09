
#ifndef __XF_KEYBOARD_H
#define __XF_KEYBOARD_H

#include <freerdp/freerdp.h>
#include "xf_event.h"

void
xf_kb_init(void);
void
xf_kb_inst_init(rdpInst * inst);
void
xf_kb_send_key(rdpInst * inst, uint16 flags, uint8 keycode);
int
xf_kb_get_toggle_keys_state(rdpInst * inst);
void
xf_kb_focus_in(rdpInst * inst);
void
xf_kb_set_keypress(uint8 keycode, unsigned keysym);
void
xf_kb_unset_keypress(uint8 keycode);
RD_BOOL
xf_kb_handle_special_keys(rdpInst * inst, unsigned keysym);

#endif
