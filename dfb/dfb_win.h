/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI Main Window

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __DFB_WIN_H
#define __DFB_WIN_H

#include <freerdp/freerdp.h>

void
dfb_init(int *argc, char *(*argv[]));
int
dfb_pre_connect(rdpInst * inst);
int
dfb_post_connect(rdpInst * inst);
void
dfb_uninit(void * dfb_info);
int
dfb_get_fds(rdpInst * inst, void ** read_fds, int * read_count,
	void ** write_fds, int * write_count);
int
dfb_check_fds(rdpInst * inst);
int
dfb_err(rdpInst * inst);

#endif /* __DFB_WIN_H */
