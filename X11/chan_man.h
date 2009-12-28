/*
   Copyright (c) 2009 Jay Sorg

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

#ifndef __CHAN_MAN_H
#define __CHAN_MAN_H

#include "freerdp.h"

int
chan_man_init(void);
int
chan_man_deinit(void);
int
chan_man_load_plugin(rdpSet * settings, const char * filename);
int
chan_man_pre_connect(struct rdp_inst * inst);
int
chan_man_post_connect(struct rdp_inst * inst);
int
chan_man_data(struct rdp_inst * inst, int chan_id, char * data,
	int data_size, int flags, int total_size);
int
chan_man_get_fds(rdpInst * inst, void ** read_fds, int * read_count,
	void ** write_fds, int * write_count);
int
chan_man_check_fds(rdpInst * inst);

#endif
