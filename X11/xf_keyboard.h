/*
   Copyright (c) 2009-2010 Jay Sorg

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
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

