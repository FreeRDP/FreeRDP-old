
#ifndef __DFB_KEYBOARD_H
#define __DFB_KEYBOARD_H

#include <freerdp/freerdp.h>

void
dfb_kb_init(void);
void
dfb_kb_inst_init(rdpInst * inst);
void
dfb_kb_send_key(rdpInst * inst, uint16 flags, uint8 keycode);
int
dfb_kb_get_toggle_keys_state(rdpInst * inst);
void
dfb_kb_focus_in(rdpInst * inst);

#endif /* __DFB_KEYBOARD_H */
