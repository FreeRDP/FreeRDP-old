/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   RDP licensing negotiation
   Copyright (C) Jay Sorg 2009

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

#ifndef __LICENCE_H
#define __LICENCE_H

#include <freerdp/types_ui.h>
#include "stream.h"

struct rdp_licence
{
	struct rdp_sec * sec;
	uint8 licence_key[16];
	uint8 licence_sign_key[16];
	RD_BOOL licence_issued;
};
typedef struct rdp_licence rdpLicence;

void
licence_process(rdpLicence * licence, STREAM s);
rdpLicence *
licence_new(struct rdp_sec * secure);
void
licence_free(rdpLicence * licence);

#endif
