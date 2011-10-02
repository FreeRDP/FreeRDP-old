/*
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI Keyboard Handling

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

#ifndef __DFB_KEYBOARD_H
#define __DFB_KEYBOARD_H

#include <freerdp/freerdp.h>

void
dfb_kb_init(void);
void
dfb_kb_send_key(rdpInst * inst, RD_BOOL up, uint8 keycode, uint8 hardwarecode);
int
dfb_kb_get_toggle_keys_state(rdpInst * inst);
void
dfb_kb_focus_in(rdpInst * inst);

#endif /* __DFB_KEYBOARD_H */
