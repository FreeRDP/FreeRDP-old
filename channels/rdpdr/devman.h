/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Device Manager

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2009

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

#ifndef __DEVMAN_H
#define __DEVMAN_H

#include "types_ui.h"

typedef struct device
{
	uint32 deviceType;
	void* prev;
	void* next;
}
DEVICE;

void
devman_init();
DEVICE*
devman_register_device(uint32 deviceType);
int
devman_unregister_device(DEVICE* dev);
void
devman_rewind();
int
devman_has_next();
DEVICE*
devman_get_next();
int
devman_get_count();

#endif // __DEVMAN_H

