 /*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Palette Functions

   Copyright 2010-2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

/* GDI Palette Functions: http://msdn.microsoft.com/en-us/library/dd183454/ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <freerdp/freerdp.h>
#include "gdi.h"

#include "gdi_palette.h"

static HPALETTE hSystemPalette = NULL;

static const PALETTEENTRY default_system_palette[20] =
{
	/* First 10 entries */
	{ 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00 },
	{ 0x00, 0x80, 0x00 },
	{ 0x80, 0x80, 0x00 },
	{ 0x00, 0x00, 0x80 },
	{ 0x80, 0x00, 0x80 },
	{ 0x00, 0x80, 0x80 },
	{ 0xC0, 0xC0, 0xC0 },
	{ 0xC0, 0xDC, 0xC0 },
	{ 0xA6, 0xCA, 0xF0 },

	/* Last 10 entries */
	{ 0xFF, 0xFB, 0xF0 },
	{ 0xA0, 0xA0, 0xA4 },
	{ 0x80, 0x80, 0x80 },
	{ 0xFF, 0x00, 0x00 },
	{ 0x00, 0xFF, 0x00 },
	{ 0xFF, 0xFF, 0x00 },
	{ 0x00, 0x00, 0xFF },
	{ 0xFF, 0x00, 0xFF },
	{ 0x00, 0xFF, 0xFF },
	{ 0xFF, 0xFF, 0xFF }
};

/**
 * Create a new palette.\n
 * @msdn{dd183507}
 * @param lplgpl logical palette
 * @return new palette
 */

HPALETTE CreatePalette(LOGPALETTE *lplgpl)
{
	HPALETTE hPalette = (HPALETTE) malloc(sizeof(PALETTE));
	hPalette->objectType = GDIOBJ_PALETTE;
	hPalette->logicalPalette = lplgpl;
	return hPalette;
}

/**
 * Create system palette\n
 * @return system palette
 */

HPALETTE CreateSystemPalette()
{
	HPALETTE hPalette;
	LOGPALETTE *logicalPalette;

	logicalPalette = (LOGPALETTE*) malloc(sizeof(LOGPALETTE));
	logicalPalette->count = 256;
	logicalPalette->entries = (PALETTEENTRY*) malloc(sizeof(PALETTEENTRY) * 256);
	memset(logicalPalette->entries, 0, sizeof(PALETTEENTRY) * 256);

	memcpy(&logicalPalette->entries[0], &default_system_palette[0], 10 * sizeof(PALETTEENTRY));
	memcpy(&logicalPalette->entries[256 - 10], &default_system_palette[10], 10 * sizeof(PALETTEENTRY));

	hPalette = CreatePalette(logicalPalette);

	return hPalette;
}

/**
 * Get system palette\n
 * @return system palette
 */

HPALETTE GetSystemPalette()
{
	if (hSystemPalette == NULL)
		hSystemPalette = CreateSystemPalette();

	return hSystemPalette;
}
