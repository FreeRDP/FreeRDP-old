/*
   FreeRDP: A Remote Desktop Protocol client.
   Common Data Types

   Copyright (C) Matthew Chapman 1999-2008

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

typedef struct _RECTANGLE
{
	sint16 l, t, w, h;
} RECTANGLE;

#endif // __TYPES_H
