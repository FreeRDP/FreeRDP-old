/*
   FreeRDP: A Remote Desktop Protocol client.
   Keyboard

   Copyright (C) Jay Sorg 2009-2011

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

#ifndef __XF_KEYBOARD_H
#define __XF_KEYBOARD_H

#include <freerdp/freerdp.h>
#include "xf_types.h"

void
xf_kb_init(unsigned int keyboard_layout_id);
void
xf_kb_inst_init(xfInfo * xfi);
void
xf_kb_send_key(xfInfo * xfi, int flags, uint8 keycode);
int
xf_kb_get_toggle_keys_state(xfInfo * xfi);
void
xf_kb_focus_in(xfInfo * xfi);
void
xf_kb_set_keypress(uint8 keycode, KeySym keysym);
void
xf_kb_unset_keypress(uint8 keycode);
RD_BOOL
xf_kb_handle_special_keys(xfInfo * xfi, KeySym keysym);

#endif
