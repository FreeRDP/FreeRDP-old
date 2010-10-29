/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   RDP licensing constants

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

#ifndef __CONSTANTS_LICENSE_H
#define __CONSTANTS_LICENSE_H

/* RDP licensing constants */
#define LICENCE_TOKEN_SIZE     10
#define LICENCE_HWID_SIZE      20
#define LICENCE_SIGNATURE_SIZE 16

#define LICENSE_REQUEST             0x01
#define LICENSE_PLATFORM_CHALLENGE  0x02
#define NEW_LICENSE                 0x03
#define UPGRADE_LICENSE             0x04
#define LICENSE_INFO                0x12
#define NEW_LICENSE_REQUEST         0x13
#define PLATFORM_CHALLENGE_RESPONSE 0x15
#define LICENCE_ERROR_ALERT         0xff

#define LICENCE_TAG_USER    0x000f
#define LICENCE_TAG_HOST    0x0010

#endif /* __CONSTANTS_LICENSE_H */
