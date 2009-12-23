/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   User interface services - X keyboard mapping using XKB

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freerdp.h"
#include "libkbd.h"

unsigned int
find_keyboard_layout_in_xorg_rules(char* layout, char* variant)
{
	int i;
	int j;

	for(i = 0; i < sizeof(xkbLayouts) / sizeof(xkbLayout); i++)
	{
		if(strcmp(xkbLayouts[i].layout, layout) == 0)
		{
			for(j = 0; xkbLayouts[i].variants[j].variant != NULL; j++)
			{
				if(strcmp(xkbLayouts[i].variants[j].variant, variant) == 0)
				{
					return xkbLayouts[i].variants[j].keyboardLayoutID;
				}
			}

			return xkbLayouts[i].keyboardLayoutID;
		}
	}

	return 0;
}

unsigned int
detect_keyboard_layout_from_xkb()
{
        FILE* xprop;

	char* pch;
	char* beg;
	char* end;

	char* layout = NULL;
	char* variant = NULL;
	
        char buffer[1024];

        xprop = popen("xprop -root _XKB_RULES_NAMES", "r");

	/* Sample output for "Canadian Multilingual Standard"
	 *
	 * _XKB_RULES_NAMES(STRING) = "xorg", "pc105", "ca", "multix", ""
	 * Where "xorg" is the set of rules
	 * "pc105" the keyboard type
	 * "ca" the keyboard layout
	 * "multi" the keyboard layout variant
	 */

        while(fgets(buffer, sizeof(buffer), xprop) != NULL)
        {
		if((pch = strstr(buffer, "_XKB_RULES_NAMES(STRING) = ")) != NULL)
		{
			/* "rules" */
			pch = strchr(&buffer[27], ','); // We assume it is xorg
			pch += 1;

			/* "type" */
			pch = strchr(pch, ',');

			/* "layout" */
			beg = strchr(pch + 1, '"');
			beg += 1;

			end = strchr(beg, '"');
			*end = '\0';

			layout = beg;

			/* "variant" */
			beg = strchr(end + 1, '"');
			beg += 1;

			end = strchr(beg, '"');
			*end = '\0';

			variant = beg;
		}
        }

	return find_keyboard_layout_in_xorg_rules(layout, variant);
}

unsigned int
detect_keyboard_type_from_xkb(char* xkbfile, int length)
{
	char* pch;
	char* beg;
	char* end;
	char buffer[1024];

	FILE* setxkbmap;

	// This tells us about the current XKB configuration, if XKB is available
	setxkbmap = popen("setxkbmap -print", "r");

	while(fgets(buffer, sizeof(buffer), setxkbmap) != NULL)
	{
		// The line with xkb_keycodes is what interests us
		pch = strstr(buffer, "xkb_keycodes");

		if(pch != NULL)
		{
			pch = strstr(pch, "include");
			if(pch != NULL)
			{
				// Check " " delimiter presence
				if((beg = strchr(pch, '"')) == NULL)
					break;
				else
					beg++;

				if((pch = strchr(beg + 1, '"')) == NULL)
					break;

				end = strcspn(beg + 1, "\"") + beg + 1;
				*end = '\0';

				strncpy(xkbfile, beg, length);

				return 1;
			}
		}
	}

	return 0;
}

unsigned int
detect_keyboard_layout_from_locale()
{
	int i;
	int j;
	int k;
	int dot;
	int underscore;

	char language[4];
	char country[10];

	// LANG = <language>_<country>.<encoding>
	char* envLang = getenv("LANG"); // Get locale from environment variable LANG

	if(envLang == NULL)
		return 0; // LANG environment variable was not set

	underscore = strcspn(envLang, "_");

	if(underscore > 3)
		return 0; // The language name should not be more than 3 letters long
	else
	{
		// Get language code
		strncpy(language, envLang, underscore);
		language[underscore] = '\0';
	}

	// There is always the special case of "C" or "POSIX" as locale name
	// In this case, use a U.S. keyboard and a U.S. keyboard layout

	if((strcmp(language, "C") == 0) || (strcmp(language, "POSIX") == 0))
		return ENGLISH_UNITED_STATES; // U.S. Keyboard Layout

	dot = strcspn(envLang, ".");
	
	// Get country code
	if(dot > underscore)
	{
		strncpy(country, &envLang[underscore + 1], dot - underscore - 1);
		country[dot - underscore - 1] = '\0';
	}
	else
		return 0; // Invalid locale

	for(i = 0; i < sizeof(locales) / sizeof(locale); i++)
	{
		if((strcmp(language, locales[i].language) == 0) && (strcmp(country, locales[i].country) == 0))
			break;
	}

	// printf("Found locale : %s_%s\n", locales[i].language, locales[i].country);

	for(j = 0; j < sizeof(defaultKeyboardLayouts) / sizeof(localeAndKeyboardLayout); j++)
	{
		if(defaultKeyboardLayouts[j].locale == locales[i].code)
		{
			// Locale found in list of default keyboard layouts
			for(k = 0; k < 5; k++)
			{
				if(defaultKeyboardLayouts[j].keyboardLayouts[k] == ENGLISH_UNITED_STATES)
				{
					continue; // Skip, try to get a more localized keyboard layout
				}
				else if(defaultKeyboardLayouts[j].keyboardLayouts[k] == 0)
				{
					break; // No more keyboard layouts
				}
				else
				{
					return defaultKeyboardLayouts[j].keyboardLayouts[k];
				}
			}
			
			// If we skip the ENGLISH_UNITED_STATES keyboard layout but there are no
			// other possible keyboard layout for the locale, we end up here with k > 1

			if(k > 1)
				return ENGLISH_UNITED_STATES;
			else
				return 0;
		}
	}

	return 0; // Could not detect the current keyboard layout from locale
}

unsigned int
detect_keyboard_type_and_layout_sunos(char* xkbfile, int length)
{
        FILE* kbd;

	int i;
	int type = 0;
	int layout = 0;

	char* pch;
	char* beg;
	char* end;

        char buffer[1024];
        
	/*
		Sample output for "kbd -t -l" :

		USB keyboard
		type=6
		layout=3 (0x03)
		delay(ms)=500
		rate(ms)=40
	*/

        kbd = popen("kbd -t -l", "r");

        while(fgets(buffer, sizeof(buffer), kbd) != NULL)
        {
		if((pch = strstr(buffer, "type=")) != NULL)
		{
			beg = pch + sizeof("type=") - 1;
			end = strchr(beg, '\n');
			end[0] = '\0';
			type = atoi(beg);
		}
		else if((pch = strstr(buffer, "layout=")) != NULL)
		{
			beg = pch + sizeof("layout=") - 1;
			end = strchr(beg, ' ');
			end[0] = '\0';
			layout = atoi(beg);
		}
        }

	for(i = 0; i < sizeof(SunOSKeyboards) / sizeof(SunOSKeyboard); i++)
	{
		if(SunOSKeyboards[i].type == type)
		{
			if(SunOSKeyboards[i].layout == layout)
			{
				strncpy(xkbfile, SunOSKeyboards[i].xkbType, length);
				return SunOSKeyboards[i].keyboardLayoutID;
			}
		}
	}
	
	return 0;
}

int
load_keyboard(char* kbd)
{
	char* pch;
	char *beg, *end;
	char* home;
	char buffer[1024] = "";
	char xkbfile[256] = "";
	char xkbfilepath[512] = "";
	char xkbmap[256] = "";
	char xkbinc[256] = "";

	FILE* fp;
	int kbdFound = 0;

	int i = 0;
	int keycode = 0;
	char keycodeString[32] = "";
	char vkcodeName[128] = "";

	beg = kbd;

	printf("Loading keymap %s\n", kbd);

	// Extract file name and keymap name
	if((end = strrchr(kbd, '(')) != NULL)
	{
		strncpy(xkbfile, &kbd[beg - kbd], end - beg);

		beg = end + 1;
		if((end = strrchr(kbd, ')')) != NULL)
		{
			strncpy(xkbmap, &kbd[beg - kbd], end - beg);
			xkbmap[end - beg] = '\0';
		}
	}
	else	
	{
		// The keyboard name is the same as the file name
		strcpy(xkbfile, kbd);
		strcpy(xkbmap, kbd);
	}

	// Get path to file relative to freerdp's directory
	snprintf(xkbfilepath, sizeof(xkbfilepath), "keymaps/%s", xkbfile);

	// Open the file for reading only
	// It can happen that the same file is opened twice at the same time
	// in order to load multiple keyboard maps from the same file, but
	// it does not matter: files can be opened as many times as we want
	// when it is for reading only.
	if((fp = fopen(xkbfilepath, "r")) == NULL)
	{
		/* If ran from the source tree, the keymaps will be in the parent directory */
		snprintf(xkbfilepath, sizeof(xkbfilepath), "../keymaps/%s", xkbfile);

		if((fp = fopen(xkbfilepath, "r")) == NULL)
		{
			// File wasn't found in the source tree, try ~/.freerdp/ folder
			if((home = getenv("HOME")) == NULL)
				return 0;

			// Get path to file in ~/.freerdp/ folder
			snprintf(xkbfilepath, sizeof(xkbfilepath), "%s/.freerdp/keymaps/%s", home, xkbfile);

			if((fp = fopen(xkbfilepath, "r")) == NULL)
			{
				// Try /usr/share/freerdp folder
				snprintf(xkbfilepath, sizeof(xkbfilepath), "/usr/share/freerdp/keymaps/%s", xkbfile);
			
				if((fp = fopen(xkbfilepath, "r")) == NULL)
				{
					// Try /usr/local/share/freerdp folder
					snprintf(xkbfilepath, sizeof(xkbfilepath), "/usr/local/share/freerdp/keymaps/%s", xkbfile);

					if((fp = fopen(xkbfilepath, "r")) == NULL)
					{
						// Error: Could not find keymap
						return 0;
					}
				}
			}
		}
	}

	printf("xkbfilepath: %s\n", xkbfilepath);

	while(fgets(buffer, sizeof(buffer), fp) != NULL)
	{
		if(buffer[0] == '#')
		{
			continue; // Skip comments
		}

		if(kbdFound)
		{
			// Closing curly bracket and semicolon
			if((pch = strstr(buffer, "};")) != NULL)
			{
				break;
			}
			else if((pch = strstr(buffer, "VK_")) != NULL)
			{
				// The end is delimited by the first white space
				end = strcspn(pch, " \t\n\0") + pch;

				// We copy the virtual key code name in a string
				beg = pch;
				strncpy(vkcodeName, beg, end - beg);
				vkcodeName[end - beg] = '\0';

				// Now we want to extract the virtual key code itself
				// which is in between '<' and '>'
				if((beg = strchr(pch + 3, '<')) == NULL)
					break;
				else
					beg++;
					
				if((end = strchr(beg, '>')) == NULL)
					break;
				
				// We copy the string representing the number in a string
				strncpy(keycodeString, beg, end - beg);
				keycodeString[end - beg] = '\0';

				// Convert the string representing the code to an integer
				keycode = atoi(keycodeString);

				// Make sure it is a valid keycode
				if(keycode < 0 || keycode > 255)
					break;

				// Load this key mapping in the keyboard mapping
				for(i = 0; i < sizeof(virtualKeyboard) / sizeof(virtualKey); i++)
				{
					if(strcmp(vkcodeName, virtualKeyboard[i].name) == 0)
					{
						keycodeToVkcode[keycode] = i;
						// printf("%s %d\n", virtualKeyboard[i].name, keycode);
					}
				}
			}
			else if((pch = strstr(buffer, ": extends")) != NULL)
			{
				// This map extends another keymap
				// We extract its name and we recursively
				// load the keymap we need to include.

				if((beg = strchr(pch + sizeof(": extends"), '"')) == NULL)
					break;
				beg++;

				if((end = strchr(beg, '"')) == NULL)
					break;

				strncpy(xkbinc, beg, end - beg);
				xkbinc[end - beg] = '\0';

				load_keyboard(xkbinc); // Load included keymap
			}
		}
		else if((pch = strstr(buffer, "keyboard")) != NULL)
		{
			// Keyboard map identifier
			if((beg = strchr(pch + sizeof("keyboard"), '"')) == NULL)
				break;
			beg++;

			if((end = strchr(beg, '"')) == NULL)
				break;

			pch = beg;
			buffer[end - beg] = '\0';

			// Does it match our keymap name?
			if(strncmp(xkbmap, pch, strlen(xkbmap)) == 0)
				kbdFound = 1;
		}
	}

	fclose(fp); // Don't forget to close file

	return 1;
}

unsigned int
detect_and_load_keyboard()
{
	int i;
	char* pch;
	char* beg;
	char* end;

	char xkbfile[256];
	unsigned int keyboard_layout = 0;
	unsigned int keyboardLayoutID = 0;

	int keymapLoaded = 0;

#if defined(sun)
	keyboardLayoutID = detect_keyboard_type_and_layout_sunos(xkbfile, sizeof(xkbfile));
#endif

	if(keyboardLayoutID == 0)
	{
		detect_keyboard_type_from_xkb(xkbfile, sizeof(xkbfile));
		keyboardLayoutID = detect_keyboard_layout_from_xkb();
	}
	
	keyboard_layout = detect_keyboard_layout_from_xkb();
		
	printf("find_keyboard_layout_in_xorg_rules: %X\n", keyboard_layout);

	if(keyboard_layout != 0)
		keyboardLayoutID = keyboard_layout;

	if(keyboardLayoutID == 0)
		keyboardLayoutID = detect_keyboard_layout_from_locale();

	printf("detect_keyboard_layout_from_locale: %X\n", keyboardLayoutID);

	keyboard_layout = keyboardLayoutID;

	for(i = 0; i < sizeof(keyboardLayouts) / sizeof(keyboardLayout); i++)
		if(keyboardLayouts[i].code == keyboard_layout)
		{
			printf("Using %s (0x%08X)\n", keyboardLayouts[i].name, keyboardLayouts[i].code);
			break;
		}

	for(i = 0; i < sizeof(keyboardLayoutVariants) / sizeof(keyboardLayoutVariant); i++)
		if(keyboardLayoutVariants[i].code == keyboard_layout)
		{
			printf("Using %s (0x%08X)\n", keyboardLayoutVariants[i].name, keyboardLayoutVariants[i].code);
			break;
		}

	for(i = 0; i < sizeof(keyboardIMEs) / sizeof(keyboardIME); i++)
		if(keyboardIMEs[i].code == keyboard_layout)
		{
			printf("Using %s (0x%08X)\n", keyboardIMEs[i].name, keyboardIMEs[i].code);
			break;
		}

	beg = xkbfile;
	pch = beg + strlen(xkbfile);

	do
	{
		// Multiple maps are separated by '+'
		end = strcspn(beg + 1, "+") + beg + 1;

		strncpy(xkbfile, beg, end - beg);
		xkbfile[end - beg] = '\0';

		// Load keyboard map
		keymapLoaded += load_keyboard(xkbfile);

		beg = end + 1;
	}
	while(pch > end);

	if(keymapLoaded <= 0)
	{
		// No keymap was loaded, load default hard-coded keymap
		memcpy(keycodeToVkcode, defaultKeycodeToVkcode, sizeof(keycodeToVkcode));
	}

	return keyboardLayoutID;
}

unsigned int
kbd_init()
{
	unsigned int rv;

	rv = detect_and_load_keyboard();
	printf("kbd_init: detect_and_load_keyboard returned %d\n", rv);
	if (rv == 0)
	{
		rv = 0x0409;
	}
	return rv;
}


