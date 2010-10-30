/*
   rdesktop: A Remote Desktop Protocol client.
   Common data types
   Copyright (C) Matthew Chapman 1999-2008

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

#ifndef __TYPES_H
#define __TYPES_H

#include <freerdp/types_ui.h>
#include "constants.h"
#include "stream.h"

#ifndef True
#define True  (1)
#define False (0)
#endif

typedef struct _BOUNDS
{
	sint16 left;
	sint16 top;
	sint16 right;
	sint16 bottom;
} BOUNDS;

typedef struct _FONTGLYPH
{
	sint16 offset;
	sint16 baseline;
	uint16 width;
	uint16 height;
	RD_HBITMAP pixmap;
} FONTGLYPH;

typedef struct _DATABLOB
{
	void *data;
	int size;
} DATABLOB;

typedef struct _systemTime
{
	uint16 wYear;
	uint16 wMonth;
	uint16 wDayOfWeek;
	uint16 wDay;
	uint16 wHour;
	uint16 wMinute;
	uint16 wSecond;
	uint16 wMilliseconds;

} systemTime;

/* PSTCACHE */
typedef uint8 HASH_KEY[8];

/* Header for an entry in the persistent bitmap cache file */
typedef struct _PSTCACHE_CELLHEADER
{
	HASH_KEY key;
	uint8 width, height;
	uint16 length;
	uint32 stamp;
} CELLHEADER;

typedef struct _RDPCOMP
{
	uint32 roff;
	uint8 hist[RDP_MPPC_DICT_SIZE];
	struct stream ns;
} RDPCOMP;

#endif // __TYPES_H
