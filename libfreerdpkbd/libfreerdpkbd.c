/*
   FreeRDP: A Remote Desktop Protocol client.
   XKB-based Keyboard Mapping to Microsoft Keyboard System

   Copyright 2009 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <freerdp/freerdp.h>
#include <freerdp/kbd.h>
#include "locales.h"
#include "keyboard.h"

unsigned int
find_keyboard_layout_in_xorg_rules(char* layout, char* variant)
{
	int i;
	int j;

	if(layout == NULL)
		return 0;

	printf("xkbLayout: %s\txkbVariant: %s\n", layout, variant);

	for (i = 0; i < sizeof(xkbLayouts) / sizeof(xkbLayout); i++)
	{
		if (strcmp(xkbLayouts[i].layout, layout) == 0)
		{
			for (j = 0; xkbLayouts[i].variants[j].variant != NULL && strlen(xkbLayouts[i].variants[j].variant) > 0; j++)
			{
				if (strcmp(xkbLayouts[i].variants[j].variant, variant) == 0)
				{
					return xkbLayouts[i].variants[j].keyboardLayoutID;
				}
			}

			return xkbLayouts[i].keyboardLayoutID;
		}
	}

	return 0;
}

static rdpKeyboardLayout *
get_keyboard_layouts(int types)
{
	rdpKeyboardLayout * layouts;
	int num;
	int len;
	int i;

	num = 0;
	layouts = (rdpKeyboardLayout *) malloc((num + 1) * sizeof(rdpKeyboardLayout));

	if ((types & RDP_KEYBOARD_LAYOUT_TYPE_STANDARD) != 0)
	{
		len = sizeof(keyboardLayouts) / sizeof(keyboardLayout);
		layouts = (rdpKeyboardLayout *) realloc(layouts, (num + len + 1) * sizeof(rdpKeyboardLayout));
		for (i = 0; i < len; i++, num++)
		{
			layouts[num].code = keyboardLayouts[i].code;
			strcpy(layouts[num].name, keyboardLayouts[i].name);
		}
	}
	if ((types & RDP_KEYBOARD_LAYOUT_TYPE_VARIANT) != 0)
	{
		len = sizeof(keyboardLayoutVariants) / sizeof(keyboardLayoutVariant);
		layouts = (rdpKeyboardLayout *) realloc(layouts, (num + len + 1) * sizeof(rdpKeyboardLayout));
		for (i = 0; i < len; i++, num++)
		{
			layouts[num].code = keyboardLayoutVariants[i].code;
			strcpy(layouts[num].name, keyboardLayoutVariants[i].name);
		}
	}
	if ((types & RDP_KEYBOARD_LAYOUT_TYPE_IME) != 0)
	{
		len = sizeof(keyboardIMEs) / sizeof(keyboardIME);
		layouts = (rdpKeyboardLayout *) realloc(layouts, (num + len + 1) * sizeof(rdpKeyboardLayout));
		for (i = 0; i < len; i++, num++)
		{
			layouts[num].code = keyboardIMEs[i].code;
			strcpy(layouts[num].name, keyboardIMEs[i].name);
		}
	}

	memset(&layouts[num], 0, sizeof(rdpKeyboardLayout));

	return layouts;
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
	unsigned int keyboard_layout = 0;

	/* We start by looking for _XKB_RULES_NAMES_BACKUP which appears to be used by libxklavier */

	xprop = popen("xprop -root _XKB_RULES_NAMES_BACKUP", "r");

	/* Sample output for "Canadian Multilingual Standard"
	 *
	 * _XKB_RULES_NAMES_BACKUP(STRING) = "xorg", "pc105", "ca", "multix", ""
	 * Where "xorg" is the set of rules
	 * "pc105" the keyboard type
	 * "ca" the keyboard layout
	 * "multi" the keyboard layout variant
	 */

	while(fgets(buffer, sizeof(buffer), xprop) != NULL)
	{
		if((pch = strstr(buffer, "_XKB_RULES_NAMES_BACKUP(STRING) = ")) != NULL)
		{
			/* "rules" */
			pch = strchr(&buffer[34], ','); // We assume it is xorg
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
	pclose(xprop);

	keyboard_layout = find_keyboard_layout_in_xorg_rules(layout, variant);

	if(keyboard_layout > 0)
		return keyboard_layout;

	/* Check _XKB_RULES_NAMES if _XKB_RULES_NAMES_BACKUP fails */

	xprop = popen("xprop -root _XKB_RULES_NAMES", "r");

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
	pclose(xprop);

	keyboard_layout = find_keyboard_layout_in_xorg_rules(layout, variant);
	return keyboard_layout;
}

unsigned int
detect_keyboard_type_from_xkb(char* xkbfile, int length)
{
	char* pch;
	char* beg;
	char* end;
	char buffer[1024];
	unsigned int rv = 0;

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

				rv = 1;
				break;
			}
		}
	}

	if (xkbfile[0] == '\0')
		strcpy(xkbfile, "base");

	pclose(setxkbmap);
	return rv;
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

	printf("Found locale : %s_%s\n", locales[i].language, locales[i].country);

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

			if(k >= 1)
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
	pclose(kbd);

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
		/* Look first in path given at compile time (install path) */
		snprintf(xkbfilepath, sizeof(xkbfilepath), "%s/%s", KEYMAP_PATH, xkbfile);

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
	char* kbd;
	char xkbfile[256];
	char* xkbfileEnd;
	unsigned int keyboard_layout = 0;
	unsigned int keyboardLayoutID = 0;

	int keymapLoaded = 0;
	memset(xkbfile, '\0', sizeof(xkbfile));

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

	kbd = xkbfile;
	xkbfileEnd = xkbfile + strlen(xkbfile);

#ifdef __APPLE__
	/* Apple X11 breaks XKB detection */
	keymapLoaded += load_keyboard("macosx(macosx)");
#else
	do
	{
		// Multiple maps are separated by '+'
		int kbdlen = strcspn(kbd + 1, "+") + 1;
		kbd[kbdlen] = '\0';

		// Load keyboard map
		keymapLoaded += load_keyboard(kbd);

		kbd += kbdlen + 1;
	}
	while(kbd < xkbfileEnd);
#endif

	if(keymapLoaded <= 0)
	{
		// No keymap was loaded, load default hard-coded keymap
		memcpy(keycodeToVkcode, defaultKeycodeToVkcode, sizeof(keycodeToVkcode));
	}

	return keyboardLayoutID;
}

unsigned int
freerdp_kbd_init(unsigned int keyboard_layout_id)
{
	unsigned int rv;

	rv = detect_and_load_keyboard();

	if (keyboard_layout_id == 0)
		keyboard_layout_id = rv;

	printf("kbd_init: detect_and_load_keyboard returned %d\n", keyboard_layout_id);

	if (keyboard_layout_id == 0)
		keyboard_layout_id = 0x0409;

	return keyboard_layout_id;
}

rdpKeyboardLayout *
freerdp_kbd_get_layouts(int types)
{
	return get_keyboard_layouts(types);
}

int
freerdp_kbd_get_scancode_by_keycode(uint8 keycode, int * flags)
{
	int vkcode;
	int scancode;

	vkcode = keycodeToVkcode[keycode];
	scancode = virtualKeyboard[vkcode].scancode;
	*flags |= virtualKeyboard[vkcode].flags;
	return scancode;
}

int
freerdp_kbd_get_scancode_by_virtualkey(int vkcode)
{
	return virtualKeyboard[vkcode].scancode;
}

