/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   User interface services - Virtual key code definitions, conversion maps,
   and also locales with their Microsoft Windows locale code for the keyboard layout

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

/* Microsoft Windows Virtual Key Codes
   Reference: http://msdn.microsoft.com/en-us/library/ms645540.aspx
*/

#ifndef __KEYBOARD_H
#define __KEYBOARD_H

/* Mouse buttons */

#define VK_LBUTTON	0x01 // Left mouse button
#define VK_RBUTTON	0x02 // Right mouse button
#define VK_CANCEL	0x03 // Control-break processing
#define VK_MBUTTON	0x04 // Middle mouse button (three-button mouse)
#define VK_XBUTTON1	0x05 // Windows 2000/XP: X1 mouse button
#define VK_XBUTTON2	0x06 // Windows 2000/XP: X2 mouse button

/* 0x07 is undefined */

#define VK_BACK		0x08 // BACKSPACE key
#define VK_TAB		0x09 // TAB key

/* 0x0A to 0x0B are reserved */

#define VK_CLEAR	0x0C // CLEAR key
#define VK_RETURN	0x0D // ENTER key

/* 0x0E to 0x0F are undefined */

#define VK_SHIFT	0x10 // SHIFT key
#define VK_CONTROL	0x11 // CTRL key
#define VK_MENU		0x12 // ALT key
#define VK_PAUSE	0x13 // PAUSE key
#define VK_CAPITAL	0x14 // CAPS LOCK key
#define VK_KANA		0x15 // Input Method Editor (IME) Kana mode
#define VK_HANGUEL	0x15 // IME Hanguel mode (maintained for compatibility; use #define VK_HANGUL)
#define VK_HANGUL	0x15 // IME Hangul mode

/* 0x16 is undefined */

#define VK_JUNJA	0x17 // IME Junja mode
#define VK_FINAL	0x18 // IME final mode
#define VK_HANJA	0x19 // IME Hanja mode
#define VK_KANJI	0x19 // IME Kanji mode

/* 0x1A is undefined */

#define VK_ESCAPE	0x1B // ESC key
#define VK_CONVERT	0x1C // IME convert
#define VK_NONCONVERT	0x1D // IME nonconvert
#define VK_ACCEPT	0x1E // IME accept
#define VK_MODECHANGE	0x1F // IME mode change request

#define VK_SPACE	0x20 // SPACEBAR
#define VK_PRIOR	0x21 // PAGE UP key
#define VK_NEXT		0x22 // PAGE DOWN key
#define VK_END		0x23 // END key
#define VK_HOME		0x24 // HOME key
#define VK_LEFT		0x25 // LEFT ARROW key
#define VK_UP		0x26 // UP ARROW key
#define VK_RIGHT	0x27 // RIGHT ARROW key
#define VK_DOWN		0x28 // DOWN ARROW key
#define VK_SELECT	0x29 // SELECT key
#define VK_PRINT	0x2A // PRINT key
#define VK_EXECUTE	0x2B // EXECUTE key
#define VK_SNAPSHOT	0x2C // PRINT SCREEN key
#define VK_INSERT	0x2D // INS key
#define VK_DELETE	0x2E // DEL key
#define VK_HELP		0x2F // HELP key

/* Digits, the last 4 bits of the code represent the corresponding digit */

#define VK_KEY_0	0x30 // '0' key
#define VK_KEY_1	0x31 // '1' key
#define VK_KEY_2	0x32 // '2' key
#define VK_KEY_3	0x33 // '3' key
#define VK_KEY_4	0x34 // '4' key
#define VK_KEY_5	0x35 // '5' key
#define VK_KEY_6	0x36 // '6' key
#define VK_KEY_7	0x37 // '7' key
#define VK_KEY_8	0x38 // '8' key
#define VK_KEY_9	0x39 // '9' key

/* 0x3A to 0x40 are undefined */

/* The alphabet, the code corresponds to the capitalized letter in the ASCII code */

#define VK_KEY_A	0x41 // 'A' key
#define VK_KEY_B	0x42 // 'B' key
#define VK_KEY_C	0x43 // 'C' key
#define VK_KEY_D	0x44 // 'D' key
#define VK_KEY_E	0x45 // 'E' key
#define VK_KEY_F	0x46 // 'F' key
#define VK_KEY_G	0x47 // 'G' key
#define VK_KEY_H	0x48 // 'H' key
#define VK_KEY_I	0x49 // 'I' key
#define VK_KEY_J	0x4A // 'J' key
#define VK_KEY_K	0x4B // 'K' key
#define VK_KEY_L	0x4C // 'L' key
#define VK_KEY_M	0x4D // 'M' key
#define VK_KEY_N	0x4E // 'N' key
#define VK_KEY_O	0x4F // 'O' key
#define VK_KEY_P	0x50 // 'P' key
#define VK_KEY_Q	0x51 // 'Q' key
#define VK_KEY_R	0x52 // 'R' key
#define VK_KEY_S	0x53 // 'S' key
#define VK_KEY_T	0x54 // 'T' key
#define VK_KEY_U	0x55 // 'U' key
#define VK_KEY_V	0x56 // 'V' key
#define VK_KEY_W	0x57 // 'W' key
#define VK_KEY_X	0x58 // 'X' key
#define VK_KEY_Y	0x59 // 'Y' key
#define VK_KEY_Z	0x5A // 'Z' key

#define VK_LWIN		0x5B // Left Windows key (Microsoft Natural keyboard) 
#define VK_RWIN		0x5C // Right Windows key (Natural keyboard)
#define VK_APPS		0x5D // Applications key (Natural keyboard)

/* 0x5E is reserved */

#define VK_SLEEP	0x5F // Computer Sleep key

/* Numeric keypad digits, the last four bits of the code represent the corresponding digit */

#define VK_NUMPAD0	0x60 // Numeric keypad '0' key
#define VK_NUMPAD1	0x61 // Numeric keypad '1' key
#define VK_NUMPAD2	0x62 // Numeric keypad '2' key
#define VK_NUMPAD3	0x63 // Numeric keypad '3' key
#define VK_NUMPAD4	0x64 // Numeric keypad '4' key
#define VK_NUMPAD5	0x65 // Numeric keypad '5' key
#define VK_NUMPAD6	0x66 // Numeric keypad '6' key
#define VK_NUMPAD7	0x67 // Numeric keypad '7' key
#define VK_NUMPAD8	0x68 // Numeric keypad '8' key
#define VK_NUMPAD9	0x69 // Numeric keypad '9' key

/* Numeric keypad operators and special keys */

#define VK_MULTIPLY	0x6A // Multiply key
#define VK_ADD		0x6B // Add key
#define VK_SEPARATOR	0x6C // Separator key
#define VK_SUBTRACT	0x6D // Subtract key
#define VK_DECIMAL	0x6E // Decimal key
#define VK_DIVIDE	0x6F // Divide key

/* Function keys, from F1 to F24 */

#define VK_F1		0x70 // F1 key
#define VK_F2		0x71 // F2 key
#define VK_F3		0x72 // F3 key
#define VK_F4		0x73 // F4 key
#define VK_F5		0x74 // F5 key
#define VK_F6		0x75 // F6 key
#define VK_F7		0x76 // F7 key
#define VK_F8		0x77 // F8 key
#define VK_F9		0x78 // F9 key
#define VK_F10		0x79 // F10 key
#define VK_F11		0x7A // F11 key
#define VK_F12		0x7B // F12 key
#define VK_F13		0x7C // F13 key
#define VK_F14		0x7D // F14 key
#define VK_F15		0x7E // F15 key
#define VK_F16		0x7F // F16 key
#define VK_F17		0x80 // F17 key
#define VK_F18		0x81 // F18 key
#define VK_F19		0x82 // F19 key
#define VK_F20		0x83 // F20 key
#define VK_F21		0x84 // F21 key
#define VK_F22		0x85 // F22 key
#define VK_F23		0x86 // F23 key
#define VK_F24		0x87 // F24 key

/* 0x88 to 0x8F are unassigned */

#define VK_NUMLOCK	0x90 // NUM LOCK key
#define VK_SCROLL	0x91 // SCROLL LOCK key

/* 0x92 to 0x96 are OEM specific */
/* 0x97 to 0x9F are unassigned */

/* Modifier keys */

#define VK_LSHIFT	0xA0 // Left SHIFT key
#define VK_RSHIFT	0xA1 // Right SHIFT key
#define VK_LCONTROL	0xA2 // Left CONTROL key
#define VK_RCONTROL	0xA3 // Right CONTROL key
#define VK_LMENU	0xA4 // Left MENU key
#define VK_RMENU	0xA5 // Right MENU key

/* Browser related keys */

#define VK_BROWSER_BACK		0xA6 // Windows 2000/XP: Browser Back key
#define VK_BROWSER_FORWARD	0xA7 // Windows 2000/XP: Browser Forward key
#define VK_BROWSER_REFRESH	0xA8 // Windows 2000/XP: Browser Refresh key
#define VK_BROWSER_STOP		0xA9 // Windows 2000/XP: Browser Stop key
#define VK_BROWSER_SEARCH	0xAA // Windows 2000/XP: Browser Search key 
#define VK_BROWSER_FAVORITES	0xAB // Windows 2000/XP: Browser Favorites key
#define VK_BROWSER_HOME		0xAC // Windows 2000/XP: Browser Start and Home key

/* Volume related keys */

#define VK_VOLUME_MUTE		0xAD // Windows 2000/XP: Volume Mute key
#define VK_VOLUME_DOWN		0xAE // Windows 2000/XP: Volume Down key
#define VK_VOLUME_UP		0xAF // Windows 2000/XP: Volume Up key

/* Media player related keys */

#define VK_MEDIA_NEXT_TRACK	0xB0 // Windows 2000/XP: Next Track key
#define VK_MEDIA_PREV_TRACK	0xB1 // Windows 2000/XP: Previous Track key
#define VK_MEDIA_STOP		0xB2 // Windows 2000/XP: Stop Media key
#define VK_MEDIA_PLAY_PAUSE	0xB3 // Windows 2000/XP: Play/Pause Media key

/* Application launcher keys */

#define VK_LAUNCH_MAIL		0xB4 // Windows 2000/XP: Start Mail key
#define VK_LAUNCH_MEDIA_SELECT	0xB5 // Windows 2000/XP: Select Media key
#define VK_LAUNCH_APP1		0xB6 // Windows 2000/XP: Start Application 1 key
#define VK_LAUNCH_APP2		0xB7 // Windows 2000/XP: Start Application 2 key

/* 0xB8 and 0xB9 are reserved */

/* OEM keys */

#define VK_OEM_1	0xBA // Used for miscellaneous characters; it can vary by keyboard.
			     // Windows 2000/XP: For the US standard keyboard, the ';:' key 

#define VK_OEM_PLUS	0xBB // Windows 2000/XP: For any country/region, the '+' key
#define VK_OEM_COMMA	0xBC // Windows 2000/XP: For any country/region, the ',' key
#define VK_OEM_MINUS	0xBD // Windows 2000/XP: For any country/region, the '-' key
#define VK_OEM_PERIOD	0xBE // Windows 2000/XP: For any country/region, the '.' key

#define VK_OEM_2	0xBF // Used for miscellaneous characters; it can vary by keyboard.
			     // Windows 2000/XP: For the US standard keyboard, the '/?' key

#define VK_OEM_3	0xC0 // Used for miscellaneous characters; it can vary by keyboard.
			     // Windows 2000/XP: For the US standard keyboard, the '`~' key 

/* 0xC1 to 0xD7 are reserved */
#define VK_ABNT_C1	0xC1 // Brazilian (ABNT) Keyboard
#define VK_ABNT_C2	0xC2 // Brazilian (ABNT) Keyboard

/* 0xD8 to 0xDA are unassigned */

#define VK_OEM_4	0xDB // Used for miscellaneous characters; it can vary by keyboard.
			     // Windows 2000/XP: For the US standard keyboard, the '[{' key

#define VK_OEM_5	0xDC // Used for miscellaneous characters; it can vary by keyboard.
			     // Windows 2000/XP: For the US standard keyboard, the '\|' key

#define VK_OEM_6	0xDD // Used for miscellaneous characters; it can vary by keyboard.
			     // Windows 2000/XP: For the US standard keyboard, the ']}' key

#define VK_OEM_7	0xDE // Used for miscellaneous characters; it can vary by keyboard.
			     // Windows 2000/XP: For the US standard keyboard, the 'single-quote/double-quote' key

#define VK_OEM_8	0xDF // Used for miscellaneous characters; it can vary by keyboard.

/* 0xE0 is reserved */
/* 0xE1 is OEM specific */

#define VK_OEM_102	0xE2 // Windows 2000/XP: Either the angle bracket key or
			     // the backslash key on the RT 102-key keyboard

/* 0xE3 and 0xE4 are OEM specific */

#define VK_PROCESSKEY	0xE5 // Windows 95/98/Me, Windows NT 4.0, Windows 2000/XP: IME PROCESS key

/* 0xE6 is OEM specific */

#define VK_PACKET	0xE7	// Windows 2000/XP: Used to pass Unicode characters as if they were keystrokes.
				// The #define VK_PACKET key is the low word of a 32-bit Virtual Key value used
				// for non-keyboard input methods. For more information,
				// see Remark in KEYBDINPUT, SendInput, WM_KEYDOWN, and WM_KEYUP

/* 0xE8 is unassigned */
/* 0xE9 to 0xF5 are OEM specific */

#define VK_ATTN		0xF6 // Attn key
#define VK_CRSEL	0xF7 // CrSel key
#define VK_EXSEL	0xF8 // ExSel key
#define VK_EREOF	0xF9 // Erase EOF key
#define VK_PLAY		0xFA // Play key
#define VK_ZOOM		0xFB // Zoom key
#define VK_NONAME	0xFC // Reserved 
#define VK_PA1		0xFD // PA1 key
#define VK_OEM_CLEAR	0xFE // Clear key

// Default built-in keymap
unsigned char defaultKeycodeToVkcode[256] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
	0x37, 0x38, 0x39, 0x30, 0xBD, 0xBB, 0x08, 0x09, 0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49,
	0x4F, 0x50, 0xDB, 0xDD, 0x0D, 0xA2, 0x41, 0x53, 0x44, 0x46, 0x47, 0x48, 0x4A, 0x4B, 0x4C, 0xBA,
	0xDE, 0xC0, 0xA0, 0x00, 0x5A, 0x58, 0x43, 0x56, 0x42, 0x4E, 0x4D, 0xBC, 0xBE, 0xBF, 0xA1, 0x6A,
	0x12, 0x20, 0x14, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x90, 0x91, 0x67,
	0x68, 0x69, 0x6D, 0x64, 0x65, 0x66, 0x6B, 0x61, 0x62, 0x63, 0x60, 0x6E, 0x00, 0x00, 0x00, 0x7A,
	0x7B, 0x24, 0x26, 0x21, 0x25, 0x00, 0x27, 0x23, 0x28, 0x22, 0x2D, 0x2E, 0x0D, 0xA3, 0x13, 0x2C,
	0x6F, 0x12, 0x00, 0x5B, 0x5C, 0x5D, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

unsigned char keycodeToVkcode[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Use the virtual key code as an index in this array in order to get its associated scan code		

typedef struct _virtualKey
{
	// Scan code
	int scancode;

	// Flags	
	int flags;

	// Name of virtual key
	char name[32];

} virtualKey;

virtualKey virtualKeyboard[256] =
{
	{ 0x00, 0, "" },
	{ 0x00, 0, "VK_LBUTTON" },
	{ 0x00, 0, "VK_RBUTTON" },
	{ 0x00, 0, "VK_CANCEL" },
	{ 0x00, 0, "VK_MBUTTON" },
	{ 0x00, 0, "VK_XBUTTON1" },
	{ 0x00, 0, "VK_XBUTTON2" },
	{ 0x00, 0, "" },
	{ 0x0E, 0, "VK_BACK" },
	{ 0x0F, 0, "VK_TAB" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "VK_CLEAR" },
	{ 0x1C, 0, "VK_RETURN" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x2A, 0, "VK_SHIFT" },
	{ 0x00, 0, "VK_CONTROL" },
	{ 0x38, 0, "VK_MENU" },
	{ 0x45, 0, "VK_PAUSE" },
	{ 0x3A, 0, "VK_CAPITAL" },
	{ 0x00, 0, "VK_KANA / VK_HANGUL" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "VK_JUNJA" },
	{ 0x00, 0, "VK_FINAL" },
	{ 0x00, 0, "VK_HANJA / VK_KANJI" },
	{ 0x00, 0, "" },
	{ 0x01, 0, "VK_ESCAPE" },
	{ 0x00, 0, "VK_CONVERT" },
	{ 0x00, 0, "VK_NONCONVERT" },
	{ 0x00, 0, "VK_ACCEPT" },
	{ 0x00, 0, "VK_MODECHANGE" },
	{ 0x39, 0, "VK_SPACE" },
	{ 0x49, KBD_FLAG_EXT, "VK_PRIOR" },
	{ 0x51, KBD_FLAG_EXT, "VK_NEXT" },
	{ 0x4F, KBD_FLAG_EXT, "VK_END" },
	{ 0x47, KBD_FLAG_EXT, "VK_HOME" },
	{ 0x4B, KBD_FLAG_EXT, "VK_LEFT" },
	{ 0x48, KBD_FLAG_EXT, "VK_UP" },
	{ 0x4D, KBD_FLAG_EXT, "VK_RIGHT" },
	{ 0x50, KBD_FLAG_EXT, "VK_DOWN" },
	{ 0x00, 0, "VK_SELECT" },
	{ 0x37, KBD_FLAG_EXT, "VK_PRINT" },
	{ 0x37, KBD_FLAG_EXT, "VK_EXECUTE" },
	{ 0x37, KBD_FLAG_EXT, "VK_SNAPSHOT" },
	{ 0x52, KBD_FLAG_EXT, "VK_INSERT" },
	{ 0x53, KBD_FLAG_EXT, "VK_DELETE" },
	{ 0x63, 0, "VK_HELP" },
	{ 0x0B, 0, "VK_KEY_0" },
	{ 0x02, 0, "VK_KEY_1" },
	{ 0x03, 0, "VK_KEY_2" },
	{ 0x04, 0, "VK_KEY_3" },
	{ 0x05, 0, "VK_KEY_4" },
	{ 0x06, 0, "VK_KEY_5" },
	{ 0x07, 0, "VK_KEY_6" },
	{ 0x08, 0, "VK_KEY_7" },
	{ 0x09, 0, "VK_KEY_8" },
	{ 0x0A, 0, "VK_KEY_9" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x1E, 0, "VK_KEY_A" }, // A
	{ 0x30, 0, "VK_KEY_B" }, // B
	{ 0x2E, 0, "VK_KEY_C" }, // C
	{ 0x20, 0, "VK_KEY_D" }, // D
	{ 0x12, 0, "VK_KEY_E" }, // E
	{ 0x21, 0, "VK_KEY_F" }, // F
	{ 0x22, 0, "VK_KEY_G" }, // G
	{ 0x23, 0, "VK_KEY_H" }, // H
	{ 0x17, 0, "VK_KEY_I" }, // I
	{ 0x24, 0, "VK_KEY_J" }, // J
	{ 0x25, 0, "VK_KEY_K" }, // K
	{ 0x26, 0, "VK_KEY_L" }, // L
	{ 0x32, 0, "VK_KEY_M" }, // M
	{ 0x31, 0, "VK_KEY_N" }, // N
	{ 0x18, 0, "VK_KEY_O" }, // O
	{ 0x19, 0, "VK_KEY_P" }, // P
	{ 0x10, 0, "VK_KEY_Q" }, // Q
	{ 0x13, 0, "VK_KEY_R" }, // R
	{ 0x1F, 0, "VK_KEY_S" }, // S
	{ 0x14, 0, "VK_KEY_T" }, // T
	{ 0x16, 0, "VK_KEY_U" }, // U
	{ 0x2F, 0, "VK_KEY_V" }, // V
	{ 0x11, 0, "VK_KEY_W" }, // W
	{ 0x2D, 0, "VK_KEY_X" }, // X
	{ 0x15, 0, "VK_KEY_Y" }, // Y
	{ 0x2C, 0, "VK_KEY_Z" }, // Z
	{ 0x5B, KBD_FLAG_EXT, "VK_LWIN" },
	{ 0x5C, KBD_FLAG_EXT, "VK_RWIN" },
	{ 0x5D, KBD_FLAG_EXT, "VK_APPS" },
	{ 0x00, 0, "" },
	{ 0x5F, 0, "VK_SLEEP" },
	{ 0x52, 0, "VK_NUMPAD0" },
	{ 0x4F, 0, "VK_NUMPAD1" },
	{ 0x50, 0, "VK_NUMPAD2" },
	{ 0x51, 0, "VK_NUMPAD3" },
	{ 0x4B, 0, "VK_NUMPAD4" },
	{ 0x4C, 0, "VK_NUMPAD5" },
	{ 0x4D, 0, "VK_NUMPAD6" },
	{ 0x47, 0, "VK_NUMPAD7" },
	{ 0x48, 0, "VK_NUMPAD8" },
	{ 0x49, 0, "VK_NUMPAD9" },
	{ 0x37, 0, "VK_MULTIPLY" },
	{ 0x4E, 0, "VK_ADD" },
	{ 0x00, 0, "VK_SEPARATOR" },
	{ 0x4A, 0, "VK_SUBTRACT" },
	{ 0x53, 0, "VK_DECIMAL" },
	{ 0x35, KBD_FLAG_EXT, "VK_DIVIDE" },
	{ 0x3B, 0, "VK_F1" },
	{ 0x3C, 0, "VK_F2" },
	{ 0x3D, 0, "VK_F3" },
	{ 0x3E, 0, "VK_F4" },
	{ 0x3F, 0, "VK_F5" },
	{ 0x40, 0, "VK_F6" },
	{ 0x41, 0, "VK_F7" },
	{ 0x42, 0, "VK_F8" },
	{ 0x43, 0, "VK_F9" },
	{ 0x44, 0, "VK_F10" },
	{ 0x57, 0, "VK_F11" },
	{ 0x58, 0, "VK_F12" },
	{ 0x64, 0, "VK_F13" },
	{ 0x65, 0, "VK_F14" },
	{ 0x66, 0, "VK_F15" },
	{ 0x67, 0, "VK_F16" },
	{ 0x68, 0, "VK_F17" },
	{ 0x69, 0, "VK_F18" },
	{ 0x6A, 0, "VK_F19" },
	{ 0x6B, 0, "VK_F20" },
	{ 0x6C, 0, "VK_F21" },
	{ 0x6D, 0, "VK_F22" },
	{ 0x6E, 0, "VK_F23" },
	{ 0x6F, 0, "VK_F24" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x45, 0, "VK_NUMLOCK" },
	{ 0x46, 0, "VK_SCROLL" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x2A, 0, "VK_LSHIFT" },
	{ 0x36, 0, "VK_RSHIFT" },
	{ 0x1D, 0, "VK_LCONTROL" },
	{ 0x1D, KBD_FLAG_EXT, "VK_RCONTROL" },
	{ 0x38, 0, "VK_LMENU" },
	{ 0x38, KBD_FLAG_EXT, "VK_RMENU" },
	{ 0x00, 0, "VK_BROWSER_BACK" },
	{ 0x00, 0, "VK_BROWSER_FORWARD" },
	{ 0x00, 0, "VK_BROWSER_REFRESH" },
	{ 0x00, 0, "VK_BROWSER_STOP" },
	{ 0x00, 0, "VK_BROWSER_SEARCH" },
	{ 0x00, 0, "VK_BROWSER_FAVORITES" },
	{ 0x00, 0, "VK_BROWSER_HOME" },
	{ 0x00, 0, "VK_VOLUME_MUTE" },
	{ 0x00, 0, "VK_VOLUME_DOWN" },
	{ 0x00, 0, "VK_VOLUME_UP" },
	{ 0x00, 0, "VK_MEDIA_NEXT_TRACK" },
	{ 0x00, 0, "VK_MEDIA_PREV_TRACK" },
	{ 0x00, 0, "VK_MEDIA_STOP" },
	{ 0x00, 0, "VK_MEDIA_PLAY_PAUSE" },
	{ 0x00, 0, "VK_LAUNCH_MAIL" },
	{ 0x00, 0, "VK_MEDIA_SELECT" },
	{ 0x00, 0, "VK_LAUNCH_APP1" },
	{ 0x00, 0, "VK_LAUNCH_APP2" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x27, 0, "VK_OEM_1" },
	{ 0x0D, 0, "VK_OEM_PLUS" },
	{ 0x33, 0, "VK_OEM_COMMA" },
	{ 0x0C, 0, "VK_OEM_MINUS" },
	{ 0x34, 0, "VK_OEM_PERIOD" },
	{ 0x35, 0, "VK_OEM_2" },
	{ 0x29, 0, "VK_OEM_3" },
	{ 0x73, 0, "VK_ABNT_C1" },
	{ 0x7E, 0, "VK_ABNT_C2" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x1A, 0, "VK_OEM_4" },
	{ 0x2B, 0, "VK_OEM_5" },
	{ 0x1B, 0, "VK_OEM_6" },
	{ 0x28, 0, "VK_OEM_7" },
	{ 0x1D, 0, "VK_OEM_8" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x56, 0, "VK_OEM_102" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "VK_PROCESSKEY" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "VK_PACKET" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "" },
	{ 0x00, 0, "VK_ATTN" },
	{ 0x00, 0, "VK_CRSEL" },
	{ 0x00, 0, "VK_EXSEL" },
	{ 0x00, 0, "VK_EREOF" },
	{ 0x00, 0, "VK_PLAY" },
	{ 0x62, 0, "VK_ZOOM" },
	{ 0x00, 0, "VK_NONAME" },
	{ 0x00, 0, "VK_PA1" },
	{ 0x00, 0, "VK_OEM_CLEAR" },
	{ 0x00, 0, "" }
};

// Keyboard layout IDs

#define KBD_ARABIC_101				0x00000401
#define KBD_BULGARIAN				0x00000402
#define KBD_CHINESE_TRADITIONAL_US		0x00000404
#define KBD_CZECH				0x00000405
#define KBD_DANISH				0x00000406
#define KBD_GERMAN				0x00000407
#define KBD_GREEK				0x00000408
#define KBD_US					0x00000409
#define KBD_SPANISH				0x0000040A
#define KBD_FINNISH				0x0000040B
#define KBD_FRENCH				0x0000040C
#define KBD_HEBREW				0x0000040D
#define KBD_HUNGARIAN				0x0000040E
#define KBD_ICELANDIC				0x0000040F
#define KBD_ITALIAN				0x00000410
#define KBD_JAPANESE				0x00000411
#define KBD_KOREAN				0x00000412
#define KBD_DUTCH				0x00000413
#define KBD_NORWEGIAN				0x00000414
#define KBD_POLISH_PROGRAMMERS			0x00000415
#define KBD_PORTUGUESE_BRAZILIAN_ABNT		0x00000416
#define KBD_ROMANIAN				0x00000418
#define KBD_RUSSIAN				0x00000419
#define KBD_CROATIAN				0x0000041A
#define KBD_SLOVAK				0x0000041B
#define KBD_ALBANIAN				0x0000041C
#define KBD_SWEDISH				0x0000041D
#define KBD_THAI_KEDMANEE			0x0000041E
#define KBD_TURKISH_Q				0x0000041F
#define KBD_URDU				0x00000420
#define KBD_UKRAINIAN				0x00000422
#define KBD_BELARUSIAN				0x00000423
#define KBD_SLOVENIAN				0x00000424
#define KBD_ESTONIAN				0x00000425
#define KBD_LATVIAN				0x00000426
#define KBD_LITHUANIAN_IBM			0x00000427
#define KBD_FARSI				0x00000429
#define KBD_VIETNAMESE				0x0000042A
#define KBD_ARMENIAN_EASTERN			0x0000042B
#define KBD_AZERI_LATIN				0x0000042C
#define KBD_FYRO_MACEDONIAN			0x0000042F
#define KBD_GEORGIAN				0x00000437
#define KBD_FAEROESE				0x00000438
#define KBD_DEVANAGARI_INSCRIPT			0x00000439
#define KBD_MALTESE_47_KEY			0x0000043A
#define KBD_NORWEGIAN_WITH_SAMI			0x0000043B
#define KBD_KAZAKH				0x0000043F
#define KBD_KYRGYZ_CYRILLIC			0x00000440
#define KBD_TATAR				0x00000444
#define KBD_BENGALI				0x00000445
#define KBD_PUNJABI				0x00000446
#define KBD_GUJARATI				0x00000447
#define KBD_TAMIL				0x00000449
#define KBD_TELUGU				0x0000044A
#define KBD_KANNADA				0x0000044B
#define KBD_MALAYALAM				0x0000044C
#define KBD_MARATHI				0x0000044E
#define KBD_MONGOLIAN_CYRILLIC			0x00000450
#define KBD_UNITED_KINGDOM_EXTENDED		0x00000452
#define KBD_SYRIAC				0x0000045A
#define KBD_NEPALI				0x00000461
#define KBD_PASHTO				0x00000463
#define KBD_DIVEHI_PHONETIC			0x00000465
#define KBD_LUXEMBOURGISH			0x0000046E
#define KBD_MAORI				0x00000481
#define KBD_CHINESE_SIMPLIFIED_US		0x00000804
#define KBD_SWISS_GERMAN			0x00000807
#define KBD_UNITED_KINGDOM			0x00000809
#define KBD_LATIN_AMERICAN			0x0000080A
#define KBD_BELGIAN_FRENCH			0x0000080C
#define KBD_BELGIAN_PERIOD			0x00000813
#define KBD_PORTUGUESE				0x00000816
#define KBD_SERBIAN_LATIN			0x0000081A
#define KBD_AZERI_CYRILLIC			0x0000082C
#define KBD_SWEDISH_WITH_SAMI			0x0000083B
#define KBD_UZBEK_CYRILLIC			0x00000843
#define KBD_INUKTITUT_LATIN			0x0000085D
#define KBD_CANADIAN_FRENCH_LEGACY		0x00000C0C
#define KBD_SERBIAN_CYRILLIC			0x00000C1A
#define KBD_CANADIAN_FRENCH			0x00001009
#define KBD_SWISS_FRENCH			0x0000100C
#define KBD_BOSNIAN				0x0000141A
#define KBD_IRISH				0x00001809
#define KBD_BOSNIAN_CYRILLIC			0x0000201A

// Keyboard layout variant IDs

#define KBD_ARABIC_102					0x00010401
#define KBD_BULGARIAN_LATIN				0x00010402
#define KBD_CZECH_QWERTY				0x00010405
#define KBD_GERMAN_IBM					0x00010407
#define KBD_GREEK_220					0x00010408
#define KBD_UNITED_STATES_DVORAK			0x00010409
#define KBD_SPANISH_VARIATION				0x0001040A
#define KBD_HUNGARIAN_101_KEY				0x0001040E
#define KBD_ITALIAN_142					0x00010410
#define KBD_POLISH_214					0x00010415
#define KBD_PORTUGUESE_BRAZILIAN_ABNT2			0x00010416
#define KBD_RUSSIAN_TYPEWRITER				0x00010419
#define KBD_SLOVAK_QWERTY				0x0001041B
#define KBD_THAI_PATTACHOTE				0x0001041E
#define KBD_TURKISH_F					0x0001041F
#define KBD_LATVIAN_QWERTY				0x00010426
#define KBD_LITHUANIAN					0x00010427
#define KBD_ARMENIAN_WESTERN				0x0001042B
#define KBD_HINDI_TRADITIONAL				0x00010439
#define KBD_MALTESE_48_KEY				0x0001043A
#define KBD_SAMI_EXTENDED_NORWAY			0x0001043B
#define KBD_BENGALI_INSCRIPT				0x00010445
#define KBD_SYRIAC_PHONETIC				0x0001045A
#define KBD_DIVEHI_TYPEWRITER				0x00010465
#define KBD_BELGIAN_COMMA				0x0001080C
#define KBD_FINNISH_WITH_SAMI				0x0001083B
#define KBD_CANADIAN_MULTILINGUAL_STANDARD		0x00011009
#define KBD_GAELIC					0x00011809
#define KBD_ARABIC_102_AZERTY				0x00020401
#define KBD_CZECH_PROGRAMMERS				0x00020405
#define KBD_GREEK_319					0x00020408
#define KBD_UNITED_STATES_INTERNATIONAL			0x00020409
#define KBD_THAI_KEDMANEE_NON_SHIFTLOCK			0x0002041E
#define KBD_SAMI_EXTENDED_FINLAND_SWEDEN		0x0002083B
#define KBD_GREEK_220_LATIN				0x00030408
#define KBD_UNITED_STATES_DVORAK_FOR_LEFT_HAND		0x00030409
#define KBD_THAI_PATTACHOTE_NON_SHIFTLOCK		0x0003041E
#define KBD_GREEK_319_LATIN				0x00040408
#define KBD_UNITED_STATES_DVORAK_FOR_RIGHT_HAND		0x00040409
#define KBD_GREEK_LATIN					0x00050408
#define KBD_US_ENGLISH_TABLE_FOR_IBM_ARABIC_238_L	0x00050409
#define KBD_GREEK_POLYTONIC				0x00060408
#define KBD_GERMAN_NEO					0xB0000407

// Global Input Method Editor (IME) IDs

#define KBD_CHINESE_TRADITIONAL_PHONETIC			0xE0010404
#define KBD_JAPANESE_INPUT_SYSTEM_MS_IME2002			0xE0010411
#define KBD_KOREAN_INPUT_SYSTEM_IME_2000			0xE0010412
#define KBD_CHINESE_SIMPLIFIED_QUANPIN				0xE0010804
#define KBD_CHINESE_TRADITIONAL_CHANGJIE			0xE0020404
#define KBD_CHINESE_SIMPLIFIED_SHUANGPIN			0xE0020804
#define KBD_CHINESE_TRADITIONAL_QUICK				0xE0030404
#define KBD_CHINESE_SIMPLIFIED_ZHENGMA				0xE0030804
#define KBD_CHINESE_TRADITIONAL_BIG5_CODE			0xE0040404
#define KBD_CHINESE_TRADITIONAL_ARRAY				0xE0050404
#define KBD_CHINESE_SIMPLIFIED_NEIMA				0xE0050804
#define KBD_CHINESE_TRADITIONAL_DAYI				0xE0060404
#define KBD_CHINESE_TRADITIONAL_UNICODE				0xE0070404
#define KBD_CHINESE_TRADITIONAL_NEW_PHONETIC			0xE0080404
#define KBD_CHINESE_TRADITIONAL_NEW_CHANGJIE			0xE0090404
#define KBD_CHINESE_TRADITIONAL_MICROSOFT_PINYIN_IME_3		0xE00E0804
#define KBD_CHINESE_TRADITIONAL_ALPHANUMERIC			0xE00F0404

typedef struct
{
	// Keyboard layout code
	unsigned int code;

	// Keyboard layout name
	char name[50];

} keyboardLayout;

// In Windows XP, this information is available in the system registry at
// HKEY_LOCAL_MACHINE/SYSTEM/CurrentControlSet001/Control/Keyboard Layouts/

keyboardLayout keyboardLayouts[] =
{
	{ KBD_ARABIC_101,		"Arabic (101)" },
	{ KBD_BULGARIAN,		"Bulgarian" },
	{ KBD_CHINESE_TRADITIONAL_US,	"Chinese (Traditional) - US Keyboard" },
	{ KBD_CZECH,			"Czech" },
	{ KBD_DANISH,			"Danish" },
	{ KBD_GERMAN,			"German" },
	{ KBD_GREEK,			"Greek" },
	{ KBD_US,			"US" },
	{ KBD_SPANISH,			"Spanish" },
	{ KBD_FINNISH,			"Finnish" },
	{ KBD_FRENCH,			"French" },
	{ KBD_HEBREW,			"Hebrew" },
	{ KBD_HUNGARIAN,		"Hungarian" },
	{ KBD_ICELANDIC,		"Icelandic" },
	{ KBD_ITALIAN,			"Italian" },
	{ KBD_JAPANESE,			"Japanese" },
	{ KBD_KOREAN,			"Korean" },
	{ KBD_DUTCH,			"Dutch" },
	{ KBD_NORWEGIAN,		"Norwegian" },
	{ KBD_POLISH_PROGRAMMERS,	"Polish (Programmers)" },
	{ KBD_PORTUGUESE_BRAZILIAN_ABNT,"Portuguese (Brazilian ABNT)" },
	{ KBD_ROMANIAN,			"Romanian" },
	{ KBD_RUSSIAN,			"Russian" },
	{ KBD_CROATIAN,			"Croatian" },
	{ KBD_SLOVAK,			"Slovak" },
	{ KBD_ALBANIAN,			"Albanian" },
	{ KBD_SWEDISH,			"Swedish" },
	{ KBD_THAI_KEDMANEE,		"Thai Kedmanee" },
	{ KBD_TURKISH_Q,		"Turkish Q" },
	{ KBD_URDU,			"Urdu" },
	{ KBD_UKRAINIAN,		"Ukrainian" },
	{ KBD_BELARUSIAN,		"Belarusian" },
	{ KBD_SLOVENIAN,		"Slovenian" },
	{ KBD_ESTONIAN,			"Estonian" },
	{ KBD_LATVIAN,			"Latvian" },
	{ KBD_LITHUANIAN_IBM,		"Lithuanian IBM" },
	{ KBD_FARSI,			"Farsi" },
	{ KBD_VIETNAMESE,		"Vietnamese" },
	{ KBD_ARMENIAN_EASTERN,		"Armenian Eastern" },
	{ KBD_AZERI_LATIN,		"Azeri Latin" },
	{ KBD_FYRO_MACEDONIAN,		"FYRO Macedonian" },
	{ KBD_GEORGIAN,			"Georgian" },
	{ KBD_FAEROESE,			"Faeroese" },
	{ KBD_DEVANAGARI_INSCRIPT,	"Devanagari - INSCRIPT" },
	{ KBD_MALTESE_47_KEY,		"Maltese 47-key" },
	{ KBD_NORWEGIAN_WITH_SAMI,	"Norwegian with Sami" },
	{ KBD_KAZAKH,			"Kazakh" },
	{ KBD_KYRGYZ_CYRILLIC,		"Kyrgyz Cyrillic" },
	{ KBD_TATAR,			"Tatar" },
	{ KBD_BENGALI,			"Bengali" },
	{ KBD_PUNJABI,			"Punjabi" },
	{ KBD_GUJARATI,			"Gujarati" },
	{ KBD_TAMIL,			"Tamil" },
	{ KBD_TELUGU,			"Telugu" },
	{ KBD_KANNADA,			"Kannada" },
	{ KBD_MALAYALAM,		"Malayalam" },
	{ KBD_MARATHI,			"Marathi" },
	{ KBD_MONGOLIAN_CYRILLIC,	"Mongolian Cyrillic" },
	{ KBD_UNITED_KINGDOM_EXTENDED,	"United Kingdom Extended" },
	{ KBD_SYRIAC,			"Syriac" },
	{ KBD_NEPALI,			"Nepali" },
	{ KBD_PASHTO,			"Pashto" },
	{ KBD_DIVEHI_PHONETIC,		"Divehi Phonetic" },
	{ KBD_LUXEMBOURGISH,		"Luxembourgish" },
	{ KBD_MAORI,			"Maori" },
	{ KBD_CHINESE_SIMPLIFIED_US,	"Chinese (Simplified) - US Keyboard" },
	{ KBD_SWISS_GERMAN,		"Swiss German" },
	{ KBD_UNITED_KINGDOM,		"United Kingdom" },
	{ KBD_LATIN_AMERICAN,		"Latin American" },
	{ KBD_BELGIAN_FRENCH,		"Belgian French" },
	{ KBD_BELGIAN_PERIOD,		"Belgian (Period)" },
	{ KBD_PORTUGUESE,		"Portuguese" },
	{ KBD_SERBIAN_LATIN,		"Serbian (Latin)" },
	{ KBD_AZERI_CYRILLIC,		"Azeri Cyrillic" },
	{ KBD_SWEDISH_WITH_SAMI,	"Swedish with Sami" },
	{ KBD_UZBEK_CYRILLIC,		"Uzbek Cyrillic" },
	{ KBD_INUKTITUT_LATIN,		"Inuktitut Latin" },
	{ KBD_CANADIAN_FRENCH_LEGACY,	"Canadian French (legacy)" },
	{ KBD_SERBIAN_CYRILLIC,		"Serbian (Cyrillic)" },
	{ KBD_CANADIAN_FRENCH,		"Canadian French" },
	{ KBD_SWISS_FRENCH,		"Swiss French" },
	{ KBD_BOSNIAN,			"Bosnian" },
	{ KBD_IRISH,			"Irish" },
	{ KBD_BOSNIAN_CYRILLIC,		"Bosnian Cyrillic" }
};


typedef struct
{
	// Keyboard layout code
	unsigned int code;
	
	// Keyboard variant ID
	unsigned short id;

	// Keyboard layout variant name
	char name[50];

} keyboardLayoutVariant;


keyboardLayoutVariant keyboardLayoutVariants[] =
{
	{ KBD_ARABIC_102,				0x0028, "Arabic (102)" },
	{ KBD_BULGARIAN_LATIN,				0x0004, "Bulgarian (Latin)" },
	{ KBD_CZECH_QWERTY,				0x0005, "Czech (QWERTY)" },
	{ KBD_GERMAN_IBM,				0x0012, "German (IBM)" },
	{ KBD_GREEK_220,				0x0016, "Greek (220)" },
	{ KBD_UNITED_STATES_DVORAK,			0x0002, "United States-Dvorak" },
	{ KBD_SPANISH_VARIATION,			0x0086, "Spanish Variation" },
	{ KBD_HUNGARIAN_101_KEY,			0x0006, "Hungarian 101-key" },
	{ KBD_ITALIAN_142,				0x0003, "Italian (142)" },
	{ KBD_POLISH_214,				0x0007, "Polish (214)" },
	{ KBD_PORTUGUESE_BRAZILIAN_ABNT2,		0x001D, "Portuguese (Brazilian ABNT2)" },
	{ KBD_RUSSIAN_TYPEWRITER,			0x0008, "Russian (Typewriter)" },
	{ KBD_SLOVAK_QWERTY,				0x0013, "Slovak (QWERTY)" },
	{ KBD_THAI_PATTACHOTE,				0x0021, "Thai Pattachote" },
	{ KBD_TURKISH_F,				0x0014, "Turkish F" },
	{ KBD_LATVIAN_QWERTY,				0x0015, "Latvian (QWERTY)" },
	{ KBD_LITHUANIAN,				0x0027, "Lithuanian" },
	{ KBD_ARMENIAN_WESTERN,				0x0025, "Armenian Western" },
	{ KBD_HINDI_TRADITIONAL,			0x000C, "Hindi Traditional" },
	{ KBD_MALTESE_48_KEY,				0x002B, "Maltese 48-key" },
	{ KBD_SAMI_EXTENDED_NORWAY,			0x002C, "Sami Extended Norway" },
	{ KBD_BENGALI_INSCRIPT,				0x002A, "Bengali (Inscript)" },
	{ KBD_SYRIAC_PHONETIC,				0x000E, "Syriac Phonetic" },
	{ KBD_DIVEHI_TYPEWRITER,			0x000D, "Divehi Typewriter" },
	{ KBD_BELGIAN_COMMA,				0x001E, "Belgian (Comma)" },
	{ KBD_FINNISH_WITH_SAMI,			0x002D, "Finnish with Sami" },
	{ KBD_CANADIAN_MULTILINGUAL_STANDARD,		0x0020, "Canadian Multilingual Standard" },
	{ KBD_GAELIC,					0x0026, "Gaelic" },
	{ KBD_ARABIC_102_AZERTY,			0x0029, "Arabic (102) AZERTY" },
	{ KBD_CZECH_PROGRAMMERS,			0x000A, "Czech Programmers" },
	{ KBD_GREEK_319,				0x0018, "Greek (319)" },
	{ KBD_UNITED_STATES_INTERNATIONAL,		0x0001, "United States-International" },
	{ KBD_THAI_KEDMANEE_NON_SHIFTLOCK,		0x0022, "Thai Kedmanee (non-ShiftLock)" },
	{ KBD_SAMI_EXTENDED_FINLAND_SWEDEN,		0x002E, "Sami Extended Finland-Sweden" },
	{ KBD_GREEK_220_LATIN,				0x0017, "Greek (220) Latin" },
	{ KBD_UNITED_STATES_DVORAK_FOR_LEFT_HAND,	0x001A, "United States-Dvorak for left hand" },
	{ KBD_THAI_PATTACHOTE_NON_SHIFTLOCK,		0x0023, "Thai Pattachote (non-ShiftLock)" },
	{ KBD_GREEK_319_LATIN,				0x0011, "Greek (319) Latin" },
	{ KBD_UNITED_STATES_DVORAK_FOR_RIGHT_HAND,	0x001B, "United States-Dvorak for right hand" },
	{ KBD_GREEK_LATIN,				0x0019, "Greek Latin" },
	{ KBD_US_ENGLISH_TABLE_FOR_IBM_ARABIC_238_L,	0x000B, "US English Table for IBM Arabic 238_L" },
	{ KBD_GREEK_POLYTONIC,				0x001F, "Greek Polytonic" },
	{ KBD_GERMAN_NEO,				0x00C0, "German Neo" }
};


// Input Method Editor (IME)

typedef struct
{
	// Keyboard layout code
	unsigned int code;

	// IME file name
	char fileName[32];

	// Keyboard layout name
	char name[50];

} keyboardIME;


// Global Input Method Editors (IME)

keyboardIME keyboardIMEs[] =
{
	{ KBD_CHINESE_TRADITIONAL_PHONETIC,			"phon.ime", "Chinese (Traditional) - Phonetic" },
	{ KBD_JAPANESE_INPUT_SYSTEM_MS_IME2002,			"imjp81.ime", "Japanese Input System (MS-IME2002)" },
	{ KBD_KOREAN_INPUT_SYSTEM_IME_2000,			"imekr61.ime", "Korean Input System (IME 2000)" },
	{ KBD_CHINESE_SIMPLIFIED_QUANPIN,			"winpy.ime", "Chinese (Simplified) - QuanPin" },
	{ KBD_CHINESE_TRADITIONAL_CHANGJIE,			"chajei.ime", "Chinese (Traditional) - ChangJie" },
	{ KBD_CHINESE_SIMPLIFIED_SHUANGPIN,			"winsp.ime", "Chinese (Simplified) - ShuangPin" },
	{ KBD_CHINESE_TRADITIONAL_QUICK,			"quick.ime", "Chinese (Traditional) - Quick" },
	{ KBD_CHINESE_SIMPLIFIED_ZHENGMA,			"winzm.ime", "Chinese (Simplified) - ZhengMa" },
	{ KBD_CHINESE_TRADITIONAL_BIG5_CODE,			"winime.ime", "Chinese (Traditional) - Big5 Code" },
	{ KBD_CHINESE_TRADITIONAL_ARRAY,			"winar30.ime", "Chinese (Traditional) - Array" },
	{ KBD_CHINESE_SIMPLIFIED_NEIMA,				"wingb.ime", "Chinese (Simplified) - NeiMa" },
	{ KBD_CHINESE_TRADITIONAL_DAYI,				"dayi.ime", "Chinese (Traditional) - DaYi" },
	{ KBD_CHINESE_TRADITIONAL_UNICODE,			"unicdime.ime", "Chinese (Traditional) - Unicode" },
	{ KBD_CHINESE_TRADITIONAL_NEW_PHONETIC,			"TINTLGNT.IME", "Chinese (Traditional) - New Phonetic" },
	{ KBD_CHINESE_TRADITIONAL_NEW_CHANGJIE,			"CINTLGNT.IME", "Chinese (Traditional) - New ChangJie" },
	{ KBD_CHINESE_TRADITIONAL_MICROSOFT_PINYIN_IME_3,	"pintlgnt.ime", "Chinese (Traditional) - Microsoft Pinyin IME 3.0" },
	{ KBD_CHINESE_TRADITIONAL_ALPHANUMERIC,			"romanime.ime", "Chinese (Traditional) - Alphanumeric" }
};

typedef struct
{
	// XKB Keyboard layout variant
	char* variant;
	
	// Keyboard Layout ID
	unsigned int keyboardLayoutID;

} xkbVariant;

typedef struct
{
	// XKB Keyboard layout
	char* layout;
	
	// Keyboard Layout ID
	unsigned int keyboardLayoutID;

	xkbVariant* variants;

} xkbLayout;

/* Those have been generated automatically and are waiting to be filled by hand */

/* USA */
xkbVariant us_variants[] = 
{
	{ "chr",		0 }, // Cherokee
	{ "euro",		0 }, // With EuroSign on 5
	{ "intl",		KBD_UNITED_STATES_INTERNATIONAL }, // International (with dead keys)
	{ "alt-intl",		KBD_UNITED_STATES_INTERNATIONAL }, // Alternative international (former us_intl)
	{ "colemak",		0 }, // Colemak
	{ "dvorak",		KBD_UNITED_STATES_DVORAK }, // Dvorak
	{ "dvorak-intl",	KBD_UNITED_STATES_DVORAK }, // Dvorak international
	{ "dvorak-l",		KBD_UNITED_STATES_DVORAK_FOR_LEFT_HAND }, // Left handed Dvorak
	{ "dvorak-r",		KBD_UNITED_STATES_DVORAK_FOR_RIGHT_HAND }, // Right handed Dvorak
	{ "dvorak-classic",	KBD_UNITED_STATES_DVORAK }, // Classic Dvorak
	{ "dvp",		KBD_UNITED_STATES_DVORAK }, // Programmer Dvorak
	{ "rus",		0 }, // Russian phonetic
	{ "mac",		KBD_US }, // Macintosh
	{ "altgr-intl",		KBD_UNITED_STATES_INTERNATIONAL }, // International (AltGr dead keys)
	{ "olpc2",		KBD_US }, // Group toggle on multiply/divide key
	{ "",			0 },
};

/* Afghanistan */
xkbVariant af_variants[] = 
{
	{ "ps",			KBD_PASHTO }, // Pashto
	{ "uz",			KBD_UZBEK_CYRILLIC }, // Southern Uzbek
	{ "olpc-ps",		KBD_PASHTO }, // OLPC Pashto
	{ "olpc-fa",		0 }, // OLPC Dari
	{ "olpc-uz",		KBD_UZBEK_CYRILLIC }, // OLPC Southern Uzbek
	{ "",			0 },
};

/* Arabic */
xkbVariant ara_variants[] = 
{
	{ "azerty",		KBD_ARABIC_102_AZERTY }, // azerty
	{ "azerty_digits",	KBD_ARABIC_102_AZERTY }, // azerty/digits
	{ "digits",		KBD_ARABIC_102_AZERTY }, // digits
	{ "qwerty",		KBD_ARABIC_101 }, // qwerty
	{ "qwerty_digits",	KBD_ARABIC_101 }, // qwerty/digits
	{ "buckwalter",		KBD_US_ENGLISH_TABLE_FOR_IBM_ARABIC_238_L }, // Buckwalter
	{ "",			0 },
};

/* Armenia */
xkbVariant am_variants[] = 
{
	{ "phonetic",		0 }, // Phonetic
	{ "phonetic-alt",	0 }, // Alternative Phonetic
	{ "eastern",		KBD_ARMENIAN_EASTERN }, // Eastern
	{ "western",		KBD_ARMENIAN_WESTERN }, // Western
	{ "eastern-alt",	KBD_ARMENIAN_EASTERN }, // Alternative Eastern
	{ "",			0 },
};

/* Azerbaijan */
xkbVariant az_variants[] = 
{
	{ "cyrillic",		KBD_AZERI_CYRILLIC }, // Cyrillic
	{ "",			0 },
};

/* Belarus */
xkbVariant by_variants[] = 
{
	{ "winkeys",		KBD_BELARUSIAN }, // Winkeys
	{ "latin",		KBD_BELARUSIAN }, // Latin
	{ "",			0 },
};

/* Belgium */
xkbVariant be_variants[] = 
{
	{ "oss",		KBD_BELGIAN_FRENCH }, // Alternative
	{ "oss_latin9",		KBD_BELGIAN_FRENCH }, // Alternative, latin-9 only
	{ "oss_sundeadkeys",	KBD_BELGIAN_PERIOD }, // Alternative, Sun dead keys
	{ "iso-alternate",	KBD_BELGIAN_COMMA }, // ISO Alternate
	{ "nodeadkeys",		KBD_BELGIAN_COMMA }, // Eliminate dead keys
	{ "sundeadkeys",	KBD_BELGIAN_PERIOD }, // Sun dead keys
	{ "wang",		KBD_BELGIAN_FRENCH }, // Wang model 724 azerty
	{ "",			0 },
};

/* Bangladesh */
xkbVariant bd_variants[] = 
{
	{ "probhat",		KBD_BENGALI_INSCRIPT }, // Probhat
	{ "",			0 },
};

/* India */
xkbVariant in_variants[] = 
{
	{ "ben",		KBD_BENGALI }, // Bengali
	{ "ben_probhat",	KBD_BENGALI_INSCRIPT }, // Bengali Probhat
	{ "guj",		KBD_GUJARATI }, // Gujarati
	{ "guru",		0 }, // Gurmukhi
	{ "jhelum",		0 }, // Gurmukhi Jhelum
	{ "kan",		KBD_KANNADA }, // Kannada
	{ "mal",		KBD_MALAYALAM }, // Malayalam
	{ "mal_lalitha",	KBD_MALAYALAM }, // Malayalam Lalitha
	{ "ori",		0 }, // Oriya
	{ "tam_unicode",	KBD_TAMIL }, // Tamil Unicode
	{ "tam_TAB",		KBD_TAMIL }, // Tamil TAB Typewriter
	{ "tam_TSCII",		KBD_TAMIL }, // Tamil TSCII Typewriter
	{ "tam",		KBD_TAMIL }, // Tamil
	{ "tel",		KBD_TELUGU }, // Telugu
	{ "urd-phonetic",	KBD_URDU }, // Urdu, Phonetic
	{ "urd-phonetic3",	KBD_URDU }, // Urdu, Alternative phonetic
	{ "urd-winkeys",	KBD_URDU }, // Urdu, Winkeys
	{ "bolnagri",		KBD_HINDI_TRADITIONAL }, // Hindi Bolnagri
	{ "hin-wx",		KBD_HINDI_TRADITIONAL }, // Hindi Wx
	{ "",			0 },
};

/* Bosnia and Herzegovina */
xkbVariant ba_variants[] = 
{
	{ "alternatequotes",	KBD_BOSNIAN }, // Use guillemets for quotes
	{ "unicode",		KBD_BOSNIAN }, // Use Bosnian digraphs
	{ "unicodeus",		KBD_BOSNIAN }, // US keyboard with Bosnian digraphs
	{ "us",			KBD_BOSNIAN_CYRILLIC }, // US keyboard with Bosnian letters
	{ "",			0 },
};

/* Brazil */
xkbVariant br_variants[] = 
{
	{ "nodeadkeys",		KBD_PORTUGUESE_BRAZILIAN_ABNT2 }, // Eliminate dead keys
	{ "dvorak",		KBD_UNITED_STATES_DVORAK }, // Dvorak
	{ "nativo",		KBD_PORTUGUESE_BRAZILIAN_ABNT2 }, // Nativo
	{ "nativo-us",		KBD_PORTUGUESE_BRAZILIAN_ABNT2 }, // Nativo for USA keyboards
	{ "nativo-epo",		KBD_PORTUGUESE_BRAZILIAN_ABNT2 }, // Nativo for Esperanto
	{ "",			0 },
};

/* Bulgaria */
xkbVariant bg_variants[] =
{
	{ "phonetic",		KBD_BULGARIAN_LATIN }, // Traditional Phonetic
	{ "bas_phonetic",	KBD_BULGARIAN_LATIN }, // Standard Phonetic
	{ "",			0 },
};

/* Morocco */
xkbVariant ma_variants[] =
{
	{ "french",			KBD_FRENCH }, // French
	{ "tifinagh",			0 }, // Tifinagh
	{ "tifinagh-alt",		0 }, // Tifinagh Alternative
	{ "tifinagh-alt-phonetic",	0 }, // Tifinagh Alternative Phonetic
	{ "tifinagh-extended",		0 }, // Tifinagh Extended
	{ "tifinagh-phonetic",		0 }, // Tifinagh Phonetic
	{ "tifinagh-extended-phonetic",	0 }, // Tifinagh Extended Phonetic
	{ "",				0 },
};

/* Canada */
xkbVariant ca_variants[] =
{
	{ "fr-dvorak",		KBD_UNITED_STATES_DVORAK }, // French Dvorak
	{ "fr-legacy",		KBD_CANADIAN_FRENCH }, // French (legacy)
	{ "multix",		KBD_CANADIAN_MULTILINGUAL_STANDARD }, // Multilingual
	{ "multi",		KBD_CANADIAN_MULTILINGUAL_STANDARD }, // Multilingual, first part
	{ "multi-2gr",		KBD_CANADIAN_MULTILINGUAL_STANDARD }, // Multilingual, second part
	{ "ike",		KBD_INUKTITUT_LATIN }, // Inuktitut
	{ "shs",		0 }, // Secwepemctsin
	{ "kut",		0 }, // Ktunaxa
	{ "eng",		KBD_US }, // English
	{ "",			0 },
};

/* China */
xkbVariant cn_variants[] =
{
	{ "tib",		0 }, // Tibetan
	{ "tib_asciinum",	0 }, // Tibetan (with ASCII numerals)
	{ "",			0 },
};

/* Croatia */
xkbVariant hr_variants[] =
{
	{ "alternatequotes",	KBD_CROATIAN }, // Use guillemets for quotes
	{ "unicode",		KBD_CROATIAN }, // Use Croatian digraphs
	{ "unicodeus",		KBD_CROATIAN }, // US keyboard with Croatian digraphs
	{ "us",			KBD_CROATIAN }, // US keyboard with Croatian letters
	{ "",			0 },
};

/* Czechia */
xkbVariant cz_variants[] =
{
	{ "bksl",		KBD_CZECH_PROGRAMMERS }, // With &lt;\|&gt; key
	{ "qwerty",		KBD_CZECH_QWERTY }, // qwerty
	{ "qwerty_bksl",	KBD_CZECH_QWERTY }, // qwerty, extended Backslash
	{ "ucw",		KBD_CZECH }, // UCW layout (accented letters only)
	{ "",			0 },
};

/* Denmark */
xkbVariant dk_variants[] =
{
	{ "nodeadkeys",		KBD_DANISH }, // Eliminate dead keys
	{ "mac",		KBD_DANISH }, // Macintosh
	{ "mac_nodeadkeys",	KBD_DANISH }, // Macintosh, eliminate dead keys
	{ "dvorak",		KBD_UNITED_STATES_DVORAK }, // Dvorak
	{ "",			0 },
};

/* Netherlands */
xkbVariant nl_variants[] =
{
	{ "sundeadkeys",	KBD_SWISS_FRENCH }, // Sun dead keys
	{ "mac",		KBD_SWISS_FRENCH }, // Macintosh
	{ "std",		KBD_SWISS_FRENCH }, // Standard
	{ "",			0 },
};

/* Estonia */
xkbVariant ee_variants[] =
{
	{ "nodeadkeys",		KBD_US }, // Eliminate dead keys
	{ "dvorak",		KBD_UNITED_STATES_DVORAK }, // Dvorak
	{ "us",			KBD_UNITED_STATES_INTERNATIONAL }, // US keyboard with Estonian letters
	{ "",			0 },
};

/* Iran */
xkbVariant ir_variants[] =
{
	{ "pro",		0 }, // Pro
	{ "keypad",		0 }, // Keypad
	{ "pro_keypad",		0 }, // Pro Keypad
	{ "ku",			0 }, // Kurdish, Latin Q
	{ "ku_f",		0 }, // Kurdish, (F)
	{ "ku_alt",		0 }, // Kurdish, Latin Alt-Q
	{ "ku_ara",		0 }, // Kurdish, Arabic-Latin
	{ "",			0 },
};

/* Iraq */
xkbVariant iq_variants[] =
{
	{ "ku",			0 }, // Kurdish, Latin Q
	{ "ku_f",		0 }, // Kurdish, (F)
	{ "ku_alt",		0 }, // Kurdish, Latin Alt-Q
	{ "ku_ara",		0 }, // Kurdish, Arabic-Latin
	{ "",			0 },
};

/* Faroe Islands */
xkbVariant fo_variants[] =
{
	{ "nodeadkeys",		0 }, // Eliminate dead keys
	{ "",			0 },
};

/* Finland */
xkbVariant fi_variants[] =
{
	{ "nodeadkeys",		0 }, // Eliminate dead keys
	{ "smi",		0 }, // Northern Saami
	{ "classic",		0 }, // Classic
	{ "mac",		0 }, // Macintosh
	{ "",			0 },
};

/* France */
xkbVariant fr_variants[] =
{
	{ "nodeadkeys",		0 }, // Eliminate dead keys
	{ "sundeadkeys",	0 }, // Sun dead keys
	{ "oss",		0 }, // Alternative
	{ "oss_latin9",		0 }, // Alternative, latin-9 only
	{ "oss_nodeadkeys",	0 }, // Alternative, eliminate dead keys
	{ "oss_sundeadkeys",	0 }, // Alternative, Sun dead keys
	{ "latin9",		0 }, // (Legacy) Alternative
	{ "latin9_nodeadkeys",	0 }, // (Legacy) Alternative, eliminate dead keys
	{ "latin9_sundeadkeys",	0 }, // (Legacy) Alternative, Sun dead keys
	{ "bepo",		0 }, // Bepo, ergonomic, Dvorak way
	{ "bepo_latin9",	0 }, // Bepo, ergonomic, Dvorak way, latin-9 only
	{ "dvorak",		0 }, // Dvorak
	{ "mac",		0 }, // Macintosh
	{ "bre",		0 }, // Breton
	{ "oci",		0 }, // Occitan
	{ "geo",		0 }, // Georgian AZERTY Tskapo
	{ "",			0 },
};

/* Ghana */
xkbVariant gh_variants[] =
{
	{ "generic",		0 }, // Multilingual
	{ "akan",		0 }, // Akan
	{ "ewe",		0 }, // Ewe
	{ "fula",		0 }, // Fula
	{ "ga",			0 }, // Ga
	{ "hausa",		0 }, // Hausa
	{ "",			0 },
};

/* Georgia */
xkbVariant ge_variants[] =
{
	{ "ergonomic",		0 }, // Ergonomic
	{ "mess",		0 }, // MESS
	{ "ru",			0 }, // Russian
	{ "os",			0 }, // Ossetian
	{ "",			0 },
};

/* Germany */
xkbVariant de_variants[] =
{
	{ "deadacute",		KBD_GERMAN }, // Dead acute
	{ "deadgraveacute",	KBD_GERMAN }, // Dead grave acute
	{ "nodeadkeys",		KBD_GERMAN }, // Eliminate dead keys
	{ "ro",			KBD_GERMAN }, // Romanian keyboard with German letters
	{ "ro_nodeadkeys",	KBD_GERMAN }, // Romanian keyboard with German letters, eliminate dead keys
	{ "dvorak",		KBD_UNITED_STATES_DVORAK }, // Dvorak
	{ "sundeadkeys",	KBD_GERMAN }, // Sun dead keys
	{ "neo",		KBD_GERMAN_NEO }, // Neo 2
	{ "mac",		KBD_GERMAN }, // Macintosh
	{ "mac_nodeadkeys",	KBD_GERMAN }, // Macintosh, eliminate dead keys
	{ "dsb",		KBD_GERMAN }, // Lower Sorbian
	{ "dsb_qwertz",		KBD_GERMAN }, // Lower Sorbian (qwertz)
	{ "qwerty",		KBD_GERMAN_IBM }, // qwerty
	{ "",			0 },
};

/* Greece */
xkbVariant gr_variants[] =
{
	{ "simple",		KBD_GREEK_220 }, // Simple
	{ "extended",		KBD_GREEK_319 }, // Extended
	{ "nodeadkeys",		KBD_GREEK_319}, // Eliminate dead keys
	{ "polytonic",		KBD_GREEK_POLYTONIC }, // Polytonic
	{ "",			0 },
};

/* Hungary */
xkbVariant hu_variants[] =
{
	{ "standard",				KBD_HUNGARIAN_101_KEY }, // Standard
	{ "nodeadkeys",				KBD_HUNGARIAN_101_KEY }, // Eliminate dead keys
	{ "qwerty",				KBD_HUNGARIAN_101_KEY }, // qwerty
	{ "101_qwertz_comma_dead",		KBD_HUNGARIAN_101_KEY }, // 101/qwertz/comma/Dead keys
	{ "101_qwertz_comma_nodead",		KBD_HUNGARIAN_101_KEY }, // 101/qwertz/comma/Eliminate dead keys
	{ "101_qwertz_dot_dead",		KBD_HUNGARIAN_101_KEY }, // 101/qwertz/dot/Dead keys
	{ "101_qwertz_dot_nodead",		KBD_HUNGARIAN_101_KEY }, // 101/qwertz/dot/Eliminate dead keys
	{ "101_qwerty_comma_dead",		KBD_HUNGARIAN_101_KEY }, // 101/qwerty/comma/Dead keys
	{ "101_qwerty_comma_nodead",		KBD_HUNGARIAN_101_KEY }, // 101/qwerty/comma/Eliminate dead keys
	{ "101_qwerty_dot_dead",		KBD_HUNGARIAN_101_KEY }, // 101/qwerty/dot/Dead keys
	{ "101_qwerty_dot_nodead",		KBD_HUNGARIAN_101_KEY }, // 101/qwerty/dot/Eliminate dead keys
	{ "102_qwertz_comma_dead",		KBD_HUNGARIAN_101_KEY }, // 102/qwertz/comma/Dead keys
	{ "102_qwertz_comma_nodead",		KBD_HUNGARIAN_101_KEY }, // 102/qwertz/comma/Eliminate dead keys
	{ "102_qwertz_dot_dead",		KBD_HUNGARIAN_101_KEY }, // 102/qwertz/dot/Dead keys
	{ "102_qwertz_dot_nodead",		KBD_HUNGARIAN_101_KEY }, // 102/qwertz/dot/Eliminate dead keys
	{ "102_qwerty_comma_dead",		KBD_HUNGARIAN_101_KEY }, // 102/qwerty/comma/Dead keys
	{ "102_qwerty_comma_nodead",		KBD_HUNGARIAN_101_KEY }, // 102/qwerty/comma/Eliminate dead keys
	{ "102_qwerty_dot_dead",		KBD_HUNGARIAN_101_KEY }, // 102/qwerty/dot/Dead keys
	{ "102_qwerty_dot_nodead",		KBD_HUNGARIAN_101_KEY }, // 102/qwerty/dot/Eliminate dead keys
	{ "",					0 },
};

/* Iceland */
xkbVariant is_variants[] =
{
	{ "Sundeadkeys",	KBD_ICELANDIC }, // Sun dead keys
	{ "nodeadkeys",		KBD_ICELANDIC }, // Eliminate dead keys
	{ "mac",		KBD_ICELANDIC }, // Macintosh
	{ "dvorak",		KBD_UNITED_STATES_DVORAK }, // Dvorak
	{ "",			0 },
};

/* Israel */
xkbVariant il_variants[] =
{
	{ "lyx",		KBD_HEBREW }, // lyx
	{ "phonetic",		KBD_HEBREW }, // Phonetic
	{ "biblical",		KBD_HEBREW }, // Biblical Hebrew (Tiro)
	{ "",			0 },
};

/* Italy */
xkbVariant it_variants[] =
{
	{ "nodeadkeys",		KBD_ITALIAN_142 }, // Eliminate dead keys
	{ "mac",		KBD_ITALIAN }, // Macintosh
	{ "geo",		KBD_GEORGIAN }, // Georgian
	{ "",			0 },
};

/* Japan */
xkbVariant jp_variants[] =
{
	{ "kana",		KBD_JAPANESE_INPUT_SYSTEM_MS_IME2002 }, // Kana
	{ "OADG109A",		KBD_JAPANESE_INPUT_SYSTEM_MS_IME2002 }, // OADG 109A
	{ "",			0 },
};

/* Kyrgyzstan */
xkbVariant kg_variants[] =
{
	{ "phonetic",		KBD_KYRGYZ_CYRILLIC }, // Phonetic
	{ "",			0 },
};

/* Kazakhstan */
xkbVariant kz_variants[] =
{
	{ "ruskaz",		KBD_KAZAKH }, // Russian with Kazakh
	{ "kazrus",		KBD_KAZAKH }, // Kazakh with Russian
	{ "",			0 },
};

/* Latin America */
xkbVariant latam_variants[] =
{
	{ "nodeadkeys",		KBD_LATIN_AMERICAN }, // Eliminate dead keys
	{ "deadtilde",		KBD_LATIN_AMERICAN }, // Include dead tilde
	{ "sundeadkeys",	KBD_LATIN_AMERICAN }, // Sun dead keys
	{ "",			0 },
};

/* Lithuania */
xkbVariant lt_variants[] =
{
	{ "std",		KBD_LITHUANIAN }, // Standard
	{ "us",			KBD_LITHUANIAN_IBM }, // US keyboard with Lithuanian letters
	{ "ibm",		KBD_LITHUANIAN_IBM }, // IBM (LST 1205-92)
	{ "lekp",		KBD_LITHUANIAN }, // LEKP
	{ "lekpa",		KBD_LITHUANIAN }, // LEKPa
	{ "balticplus",		KBD_LITHUANIAN }, // Baltic+
	{ "",			0 },
};

/* Latvia */
xkbVariant lv_variants[] =
{
	{ "apostrophe",		KBD_LATVIAN }, // Apostrophe (') variant
	{ "tilde",		KBD_LATVIAN }, // Tilde (~) variant
	{ "fkey",		KBD_LATVIAN }, // F-letter (F) variant
	{ "",			0 },
};

/* Montenegro */
xkbVariant me_variants[] =
{
	{ "cyrillic",			0 }, // Cyrillic
	{ "cyrillicyz",			0 }, // Cyrillic, Z and ZHE swapped
	{ "latinunicode",		0 }, // Latin unicode
	{ "latinyz",			0 }, // Latin qwerty
	{ "latinunicodeyz",		0 }, // Latin unicode qwerty
	{ "cyrillicalternatequotes",	0 }, // Cyrillic with guillemets
	{ "latinalternatequotes",	0 }, // Latin with guillemets
	{ "",			0 },
};

/* Macedonia */
xkbVariant mk_variants[] =
{
	{ "nodeadkeys",		KBD_FYRO_MACEDONIAN }, // Eliminate dead keys
	{ "",			0 },
};

/* Malta */
xkbVariant mt_variants[] =
{
	{ "us",			KBD_MALTESE_48_KEY }, // Maltese keyboard with US layout
	{ "",			0 },
};

/* Norway */
xkbVariant no_variants[] =
{
	{ "nodeadkeys",		KBD_NORWEGIAN }, // Eliminate dead keys
	{ "dvorak",		KBD_UNITED_STATES_DVORAK }, // Dvorak
	{ "smi",		KBD_NORWEGIAN_WITH_SAMI }, // Northern Saami
	{ "smi_nodeadkeys",	KBD_SAMI_EXTENDED_NORWAY }, // Northern Saami, eliminate dead keys
	{ "mac",		KBD_NORWEGIAN }, // Macintosh
	{ "mac_nodeadkeys",	KBD_SAMI_EXTENDED_NORWAY }, // Macintosh, eliminate dead keys
	{ "",			0 },
};

/* Poland */
xkbVariant pl_variants[] =
{
	{ "qwertz",		KBD_POLISH_214 }, // qwertz
	{ "dvorak",		KBD_UNITED_STATES_DVORAK }, // Dvorak
	{ "dvorak_quotes",	KBD_UNITED_STATES_DVORAK }, // Dvorak, Polish quotes on quotemark key
	{ "dvorak_altquotes",	KBD_UNITED_STATES_DVORAK }, // Dvorak, Polish quotes on key 1
	{ "csb",		0 }, // Kashubian
	{ "ru_phonetic_dvorak",	KBD_UNITED_STATES_DVORAK }, // Russian phonetic Dvorak
	{ "",			0 },
};

/* Portugal */
xkbVariant pt_variants[] =
{
	{ "nodeadkeys",		KBD_PORTUGUESE }, // Eliminate dead keys
	{ "sundeadkeys",	KBD_PORTUGUESE }, // Sun dead keys
	{ "mac",		KBD_PORTUGUESE }, // Macintosh
	{ "mac_nodeadkeys",	KBD_PORTUGUESE }, // Macintosh, eliminate dead keys
	{ "mac_sundeadkeys",	KBD_PORTUGUESE }, // Macintosh, Sun dead keys
	{ "nativo",		KBD_PORTUGUESE }, // Nativo
	{ "nativo-us",		KBD_PORTUGUESE }, // Nativo for USA keyboards
	{ "nativo-epo",		KBD_PORTUGUESE }, // Nativo for Esperanto
	{ "",			0 },
};

/* Romania */
xkbVariant ro_variants[] =
{
	{ "cedilla",		KBD_ROMANIAN }, // Cedilla
	{ "std",		KBD_ROMANIAN }, // Standard
	{ "std_cedilla",	KBD_ROMANIAN }, // Standard (Cedilla)
	{ "winkeys",		KBD_ROMANIAN }, // Winkeys
	{ "crh_f",		KBD_TURKISH_F }, // Crimean Tatar (Turkish F)
	{ "crh_alt",		KBD_TURKISH_Q }, // Crimean Tatar (Turkish Alt-Q)
	{ "crh_dobruca1",	KBD_TATAR }, // Crimean Tatar (Dobruca-1 Q)
	{ "crh_dobruca2",	KBD_TATAR }, // Crimean Tatar (Dobruca-2 Q)
	{ "",			0 },
};

/* Russia */
xkbVariant ru_variants[] =
{
	{ "phonetic",		KBD_RUSSIAN }, // Phonetic
	{ "phonetic_winkeys",	KBD_RUSSIAN }, // Phonetic Winkeys
	{ "typewriter",		KBD_RUSSIAN_TYPEWRITER }, // Typewriter
	{ "legacy",		KBD_RUSSIAN }, // Legacy
	{ "tt",			KBD_TATAR }, // Tatar
	{ "os_legacy",		0 }, // Ossetian, legacy
	{ "os_winkeys",		0 }, // Ossetian, Winkeys
	{ "cv",			0 }, // Chuvash
	{ "cv_latin",		0 }, // Chuvash Latin
	{ "udm",		0 }, // Udmurt
	{ "kom",		0 }, // Komi
	{ "sah",		0 }, // Yakut
	{ "xal",		0 }, // Kalmyk
	{ "dos",		0 }, // DOS
	{ "",			0 },
};

/* Serbia */
xkbVariant rs_variants[] =
{
	{ "yz",				KBD_SERBIAN_CYRILLIC }, // Z and ZHE swapped
	{ "latin",			KBD_SERBIAN_LATIN }, // Latin
	{ "latinunicode",		KBD_SERBIAN_LATIN }, // Latin Unicode
	{ "latinyz",			KBD_SERBIAN_LATIN }, // Latin qwerty
	{ "latinunicodeyz",		KBD_SERBIAN_LATIN }, // Latin Unicode qwerty
	{ "alternatequotes",		KBD_SERBIAN_CYRILLIC }, // With guillemets
	{ "latinalternatequotes",	KBD_SERBIAN_LATIN }, // Latin with guillemets
	{ "",				0 },
};

/* Slovenia */
xkbVariant si_variants[] =
{
	{ "alternatequotes",	KBD_SLOVENIAN }, // Use guillemets for quotes
	{ "us",			KBD_UNITED_STATES_INTERNATIONAL }, // US keyboard with Slovenian letters
	{ "",			0 },
};

/* Slovakia */
xkbVariant sk_variants[] =
{
	{ "bksl",		KBD_SLOVAK }, // Extended Backslash
	{ "qwerty",		KBD_SLOVAK_QWERTY }, // qwerty
	{ "qwerty_bksl",	KBD_SLOVAK_QWERTY }, // qwerty, extended Backslash
	{ "",			0 },
};

/* Spain */
xkbVariant es_variants[] =
{
	{ "nodeadkeys",		KBD_SPANISH_VARIATION }, // Eliminate dead keys
	{ "deadtilde",		KBD_SPANISH_VARIATION }, // Include dead tilde
	{ "sundeadkeys",	KBD_SPANISH }, // Sun dead keys
	{ "dvorak",		KBD_UNITED_STATES_DVORAK }, // Dvorak
	{ "ast",		KBD_SPANISH_VARIATION }, // Asturian variant with bottom-dot H and bottom-dot L
	{ "cat",		KBD_SPANISH_VARIATION }, // Catalan variant with middle-dot L
	{ "mac",		KBD_SPANISH }, // Macintosh
	{ "",			0 },
};

/* Sweden */
xkbVariant se_variants[] =
{
	{ "nodeadkeys",		KBD_SWEDISH }, // Eliminate dead keys
	{ "dvorak",		KBD_UNITED_STATES_DVORAK }, // Dvorak
	{ "rus",		KBD_RUSSIAN }, // Russian phonetic
	{ "rus_nodeadkeys",	KBD_RUSSIAN }, // Russian phonetic, eliminate dead keys
	{ "smi",		KBD_SWEDISH_WITH_SAMI }, // Northern Saami
	{ "mac",		KBD_SWEDISH }, // Macintosh
	{ "svdvorak",		KBD_UNITED_STATES_DVORAK }, // Svdvorak
	{ "",			0 },
};

/* Switzerland */
xkbVariant ch_variants[] =
{
	{ "de_nodeadkeys",	KBD_SWISS_GERMAN }, // German, eliminate dead keys
	{ "de_sundeadkeys",	KBD_SWISS_GERMAN }, // German, Sun dead keys
	{ "fr",			KBD_SWISS_FRENCH }, // French
	{ "fr_nodeadkeys",	KBD_SWISS_FRENCH }, // French, eliminate dead keys
	{ "fr_sundeadkeys",	KBD_SWISS_FRENCH }, // French, Sun dead keys
	{ "fr_mac",		KBD_SWISS_FRENCH }, // French (Macintosh)
	{ "de_mac",		KBD_SWISS_GERMAN }, // German (Macintosh)
	{ "",			0 },
};

/* Syria */
xkbVariant sy_variants[] =
{
	{ "syc",		KBD_SYRIAC }, // Syriac
	{ "syc_phonetic",	KBD_SYRIAC_PHONETIC }, // Syriac phonetic
	{ "ku",			0 }, // Kurdish, Latin Q
	{ "ku_f",		0 }, // Kurdish, (F)
	{ "ku_alt",		0 }, // Kurdish, Latin Alt-Q
	{ "",			0 },
};

/* Tajikistan */
xkbVariant tj_variants[] =
{
	{ "legacy",		0 }, // Legacy
	{ "",			0 },
};

/* Sri Lanka */
xkbVariant lk_variants[] =
{
	{ "tam_unicode",	KBD_TAMIL }, // Tamil Unicode
	{ "tam_TAB",		KBD_TAMIL }, // Tamil TAB Typewriter
	{ "",			0 },
};

/* Thailand */
xkbVariant th_variants[] =
{
	{ "tis",		KBD_THAI_KEDMANEE_NON_SHIFTLOCK }, // TIS-820.2538
	{ "pat",		KBD_THAI_PATTACHOTE }, // Pattachote
	{ "",			0 },
};

/* Turkey */
xkbVariant tr_variants[] =
{
	{ "f",			KBD_TURKISH_F }, // (F)
	{ "alt",		KBD_TURKISH_Q }, // Alt-Q
	{ "sundeadkeys",	KBD_TURKISH_F }, // Sun dead keys
	{ "ku",			0 }, // Kurdish, Latin Q
	{ "ku_f",		0 }, // Kurdish, (F)
	{ "ku_alt",		0 }, // Kurdish, Latin Alt-Q
	{ "intl",		KBD_TURKISH_F }, // International (with dead keys)
	{ "crh",		KBD_TATAR }, // Crimean Tatar (Turkish Q)
	{ "crh_f",		KBD_TURKISH_F }, // Crimean Tatar (Turkish F)
	{ "crh_alt",		KBD_TURKISH_Q }, // Crimean Tatar (Turkish Alt-Q)
	{ "",			0 },
};

/* Ukraine */
xkbVariant ua_variants[] =
{
	{ "phonetic",		KBD_UKRAINIAN }, // Phonetic
	{ "typewriter",		KBD_UKRAINIAN }, // Typewriter
	{ "winkeys",		KBD_UKRAINIAN }, // Winkeys
	{ "legacy",		KBD_UKRAINIAN }, // Legacy
	{ "rstu",		KBD_UKRAINIAN }, // Standard RSTU
	{ "rstu_ru",		KBD_UKRAINIAN }, // Standard RSTU on Russian layout
	{ "homophonic",		KBD_UKRAINIAN }, // Homophonic
	{ "crh",		KBD_TATAR }, // Crimean Tatar (Turkish Q)
	{ "crh_f",		KBD_TURKISH_F }, // Crimean Tatar (Turkish F)
	{ "crh_alt",		KBD_TURKISH_Q }, // Crimean Tatar (Turkish Alt-Q)
	{ "",			0 },
};

/* United Kingdom */
xkbVariant gb_variants[] =
{
	{ "extd",		KBD_UNITED_KINGDOM_EXTENDED }, // Extended - Winkeys
	{ "intl",		KBD_UNITED_KINGDOM_EXTENDED }, // International (with dead keys)
	{ "dvorak",		KBD_UNITED_STATES_DVORAK }, // Dvorak
	{ "dvorakukp",		KBD_UNITED_STATES_DVORAK }, // Dvorak (UK Punctuation)
	{ "mac",		KBD_UNITED_KINGDOM }, // Macintosh
	{ "colemak",		0 }, // Colemak
	{ "",			0 },
};

/* Uzbekistan */
xkbVariant uz_variants[] =
{
	{ "latin",		0 }, // Latin
	{ "crh",		KBD_TATAR }, // Crimean Tatar (Turkish Q)
	{ "crh_f",		KBD_TURKISH_F }, // Crimean Tatar (Turkish F)
	{ "crh_alt",		KBD_TURKISH_Q }, // Crimean Tatar (Turkish Alt-Q)
	{ "",			0 },
};

/* Korea, Republic of */
xkbVariant kr_variants[] =
{
	{ "kr104",		KBD_KOREAN_INPUT_SYSTEM_IME_2000 }, // 101/104 key Compatible
	{ "",			0 },
};

/* Ireland */
xkbVariant ie_variants[] =
{
	{ "CloGaelach",		KBD_GAELIC }, // CloGaelach
	{ "UnicodeExpert",	KBD_GAELIC }, // UnicodeExpert
	{ "ogam",		KBD_GAELIC }, // Ogham
	{ "ogam_is434",		KBD_GAELIC }, // Ogham IS434
	{ "",			0 },
};

/* Pakistan */
xkbVariant pk_variants[] =
{
	{ "urd-crulp",		0 }, // CRULP
	{ "urd-nla",		0 }, // NLA
	{ "ara",		KBD_ARABIC_101 }, // Arabic
	{ "",			0 },
};

/* Esperanto */
xkbVariant epo_variants[] =
{
	{ "legacy",		0 }, // displaced semicolon and quote (obsolete)
	{ "",			0 },
};

/* Nigeria */
xkbVariant ng_variants[] =
{
	{ "igbo",		0 }, // Igbo
	{ "yoruba",		0 }, // Yoruba
	{ "hausa",		0 }, // Hausa
	{ "",			0 },
};

/* Braille */
xkbVariant brai_variants[] =
{
	{ "left_hand",		0 }, // Left hand
	{ "right_hand",		0 }, // Right hand
	{ "",			0 },
};

/* Turkmenistan */
xkbVariant tm_variants[] =
{
	{ "alt",		KBD_TURKISH_Q }, // Alt-Q
	{ "",			0 },
};

xkbLayout xkbLayouts[] =
{
	{ "us",		 KBD_US, us_variants }, // USA
	{ "ad",		 0, NULL }, // Andorra
	{ "af",		 KBD_FARSI, af_variants }, // Afghanistan
	{ "ara",	 KBD_ARABIC_101, ara_variants }, // Arabic
	{ "al",		 0, NULL }, // Albania
	{ "am",		 KBD_ARMENIAN_EASTERN, am_variants }, // Armenia
	{ "az",		 KBD_AZERI_CYRILLIC, az_variants }, // Azerbaijan
	{ "by",		 KBD_BELARUSIAN, by_variants }, // Belarus
	{ "be",		 KBD_BELGIAN_FRENCH, be_variants }, // Belgium
	{ "bd",		 KBD_BENGALI, bd_variants }, // Bangladesh
	{ "in",		 KBD_HINDI_TRADITIONAL, in_variants }, // India
	{ "ba",		 KBD_CROATIAN, ba_variants }, // Bosnia and Herzegovina
	{ "br",		 KBD_PORTUGUESE_BRAZILIAN_ABNT, br_variants }, // Brazil
	{ "bg",		 KBD_BULGARIAN_LATIN, bg_variants }, // Bulgaria
	{ "ma",		 KBD_FRENCH, ma_variants }, // Morocco
	{ "mm",		 0, NULL }, // Myanmar
	{ "ca",		 KBD_US, ca_variants }, // Canada
	{ "cd",		 0, NULL }, // Congo, Democratic Republic of the
	{ "cn",		 KBD_CHINESE_TRADITIONAL_PHONETIC, cn_variants }, // China
	{ "hr",		 KBD_CROATIAN, hr_variants }, // Croatia
	{ "cz",		 KBD_CZECH, cz_variants }, // Czechia
	{ "dk",		 KBD_DANISH, dk_variants }, // Denmark
	{ "nl",		 KBD_DUTCH, nl_variants }, // Netherlands
	{ "bt",		 0, NULL }, // Bhutan
	{ "ee",		 KBD_ESTONIAN, ee_variants }, // Estonia
	{ "ir",		 0, ir_variants }, // Iran
	{ "iq",		 0, iq_variants }, // Iraq
	{ "fo",		 0, fo_variants }, // Faroe Islands
	{ "fi",		 KBD_FINNISH, fi_variants }, // Finland
	{ "fr",		 KBD_FRENCH, fr_variants }, // France
	{ "gh",		 0, gh_variants }, // Ghana
	{ "gn",		 0, NULL }, // Guinea
	{ "ge",		 KBD_GEORGIAN, ge_variants }, // Georgia
	{ "de",		 KBD_GERMAN, de_variants }, // Germany
	{ "gr",		 KBD_GREEK, gr_variants }, // Greece
	{ "hu",		 KBD_HUNGARIAN, hu_variants }, // Hungary
	{ "is",		 KBD_ICELANDIC, is_variants }, // Iceland
	{ "il",		 KBD_HEBREW, il_variants }, // Israel
	{ "it",		 KBD_ITALIAN, it_variants }, // Italy
	{ "jp",		 KBD_JAPANESE_INPUT_SYSTEM_MS_IME2002, jp_variants }, // Japan
	{ "kg",		 0, kg_variants }, // Kyrgyzstan
	{ "kh",		 0, NULL }, // Cambodia
	{ "kz",		 KBD_KAZAKH, kz_variants }, // Kazakhstan
	{ "la",		 0, NULL }, // Laos
	{ "latam",	 KBD_LATIN_AMERICAN, latam_variants }, // Latin America
	{ "lt",		 KBD_LITHUANIAN, lt_variants }, // Lithuania
	{ "lv",		 KBD_LATVIAN, lv_variants }, // Latvia
	{ "mao",	 KBD_MAORI, NULL }, // Maori
	{ "me",		 KBD_SERBIAN_LATIN, me_variants }, // Montenegro
	{ "mk",		 KBD_FYRO_MACEDONIAN, mk_variants }, // Macedonia
	{ "mt",		 KBD_MALTESE_48_KEY, mt_variants }, // Malta
	{ "mn",		 KBD_MONGOLIAN_CYRILLIC, NULL }, // Mongolia
	{ "no",		 KBD_NORWEGIAN, no_variants }, // Norway
	{ "pl",		 KBD_POLISH_214, pl_variants }, // Poland
	{ "pt",		 KBD_PORTUGUESE, pt_variants }, // Portugal
	{ "ro",		 KBD_ROMANIAN, ro_variants }, // Romania
	{ "ru",		 KBD_RUSSIAN, ru_variants }, // Russia
	{ "rs",		 KBD_SERBIAN_LATIN, rs_variants }, // Serbia
	{ "si",		 KBD_SLOVENIAN, si_variants }, // Slovenia
	{ "sk",		 KBD_SLOVAK, sk_variants }, // Slovakia
	{ "es",		 KBD_SPANISH, es_variants }, // Spain
	{ "se",		 KBD_SWEDISH, se_variants }, // Sweden
	{ "ch",		 KBD_SWISS_FRENCH, ch_variants }, // Switzerland
	{ "sy",		 KBD_SYRIAC, sy_variants }, // Syria
	{ "tj",		 0, tj_variants }, // Tajikistan
	{ "lk",		 0, lk_variants }, // Sri Lanka
	{ "th",		 KBD_THAI_KEDMANEE, th_variants }, // Thailand
	{ "tr",		 KBD_TURKISH_Q, tr_variants }, // Turkey
	{ "ua",		 KBD_UKRAINIAN, ua_variants }, // Ukraine
	{ "gb",		 KBD_UNITED_KINGDOM, gb_variants }, // United Kingdom
	{ "uz",		 KBD_UZBEK_CYRILLIC, uz_variants }, // Uzbekistan
	{ "vn",		 KBD_VIETNAMESE, NULL }, // Vietnam
	{ "kr",		 KBD_KOREAN_INPUT_SYSTEM_IME_2000, kr_variants }, // Korea, Republic of
	{ "ie",		 KBD_UNITED_KINGDOM, ie_variants }, // Ireland
	{ "pk",		 0, pk_variants }, // Pakistan
	{ "mv",		 0, NULL }, // Maldives
	{ "za",		 0, NULL }, // South Africa
	{ "epo",	 0, epo_variants }, // Esperanto
	{ "np",		 KBD_NEPALI, NULL }, // Nepal
	{ "ng",		 0, ng_variants }, // Nigeria
	{ "et",		 0, NULL }, // Ethiopia
	{ "sn",		 0, NULL }, // Senegal
	{ "brai",	 0, brai_variants }, // Braille
	{ "tm",		 KBD_TURKISH_Q, tm_variants }, // Turkmenistan
};

/* OpenSolaris 2008.11 and 2009.06 keyboard layouts
 *
 * While OpenSolaris comes with Xorg and XKB, it maintains a set of keyboard layout
 * names that map directly to a particular keyboard layout in XKB. Fortunately for us,
 * this way of doing things comes from Solaris, which is XKB unaware. The same keyboard
 * layout naming system is used in Solaris, so we can use the same XKB configuration as
 * we would on OpenSolaris and get an accurate keyboard layout detection :)
 *
 * We can check for the current keyboard layout using the "kbd -l" command:
 *
 * type=6
 * layout=33 (0x21)
 * delay(ms)=500
 * rate(ms)=40
 *
 * We can check at runtime if the kbd utility is present, parse the output, and use the
 * keyboard layout indicated by the index given (in this case, 33, or US-English).
 */


typedef struct _SunOSKeyboard
{
        // Sun keyboard type
	int type;

	// Layout
	int layout;

        // XKB keyboard
        char* xkbType;

        // XKB keyboard layout
        unsigned int keyboardLayoutID;

} SunOSKeyboard;


SunOSKeyboard SunOSKeyboards[] =
{
	{ 4,   0,    "sun(type4)",               KBD_US					}, //  US4
	{ 4,   1,    "sun(type4)",               KBD_US					}, //  US4
	{ 4,   2,    "sun(type4tuv)",            KBD_FRENCH				}, //  FranceBelg4
	{ 4,   3,    "sun(type4_ca)",            KBD_US					}, //  Canada4
	{ 4,   4,    "sun(type4tuv)",            KBD_DANISH				}, //  Denmark4
	{ 4,   5,    "sun(type4tuv)",            KBD_GERMAN				}, //  Germany4
	{ 4,   6,    "sun(type4tuv)",            KBD_ITALIAN				}, //  Italy4
	{ 4,   7,    "sun(type4tuv)",            KBD_DUTCH				}, //  Netherland4
	{ 4,   8,    "sun(type4tuv)",            KBD_NORWEGIAN				}, //  Norway4
	{ 4,   9,    "sun(type4tuv)",            KBD_PORTUGUESE				}, //  Portugal4
	{ 4,   10,   "sun(type4tuv)",            KBD_SPANISH				}, //  SpainLatAm4
	{ 4,   11,   "sun(type4tuv)",            KBD_SWEDISH				}, //  SwedenFin4
	{ 4,   12,   "sun(type4tuv)",            KBD_SWISS_FRENCH			}, //  Switzer_Fr4
	{ 4,   13,   "sun(type4tuv)",            KBD_SWISS_GERMAN			}, //  Switzer_Ge4
	{ 4,   14,   "sun(type4tuv)",            KBD_UNITED_KINGDOM			}, //  UK4
	{ 4,   16,   "sun(type4)",               KBD_KOREAN_INPUT_SYSTEM_IME_2000	}, //  Korea4
	{ 4,   17,   "sun(type4)",               KBD_CHINESE_TRADITIONAL_PHONETIC	}, //  Taiwan4
	{ 4,   32,   "sun(type4jp)",             KBD_JAPANESE_INPUT_SYSTEM_MS_IME2002	}, //  Japan4
	{ 4,   19,   "sun(type5)",               KBD_US					}, //  US101A_PC
	{ 4,   33,   "sun(type5)",               KBD_US					}, //  US5
	{ 4,   34,   "sun(type5unix)",           KBD_US					}, //  US_UNIX5
	{ 4,   35,   "sun(type5tuv)",            KBD_FRENCH				}, //  France5
	{ 4,   36,   "sun(type5tuv)",            KBD_DANISH				}, //  Denmark5
	{ 4,   37,   "sun(type5tuv)",            KBD_GERMAN				}, //  Germany5
	{ 4,   38,   "sun(type5tuv)",            KBD_ITALIAN				}, //  Italy5
	{ 4,   39,   "sun(type5tuv)",            KBD_DUTCH				}, //  Netherland5
	{ 4,   40,   "sun(type5tuv)",            KBD_NORWEGIAN				}, //  Norway5
	{ 4,   41,   "sun(type5tuv)",            KBD_PORTUGUESE				}, //  Portugal5
	{ 4,   42,   "sun(type5tuv)",            KBD_SPANISH				}, //  Spain5
	{ 4,   43,   "sun(type5tuv)",            KBD_SWEDISH				}, //  Sweden5
	{ 4,   44,   "sun(type5tuv)",            KBD_SWISS_FRENCH			}, //  Switzer_Fr5
	{ 4,   45,   "sun(type5tuv)",            KBD_SWISS_GERMAN			}, //  Switzer_Ge5
	{ 4,   46,   "sun(type5tuv)",            KBD_UNITED_KINGDOM			}, //  UK5
	{ 4,   47,   "sun(type5)",               KBD_KOREAN_INPUT_SYSTEM_IME_2000	}, //  Korea5
	{ 4,   48,   "sun(type5)",               KBD_CHINESE_TRADITIONAL_PHONETIC	}, //  Taiwan5
	{ 4,   49,   "sun(type5jp)",             KBD_JAPANESE_INPUT_SYSTEM_MS_IME2002	}, //  Japan5
	{ 4,   50,   "sun(type5tuv)",            KBD_CANADIAN_FRENCH			}, //  Canada_Fr5
	{ 4,   51,   "sun(type5tuv)",            KBD_HUNGARIAN 				}, //  Hungary5
	{ 4,   52,   "sun(type5tuv)",            KBD_POLISH_214				}, //  Poland5
	{ 4,   53,   "sun(type5tuv)",            KBD_CZECH				}, //  Czech5
	{ 4,   54,   "sun(type5tuv)",            KBD_RUSSIAN				}, //  Russia5
	{ 4,   55,   "sun(type5tuv)",            KBD_LATVIAN				}, //  Latvia5
	{ 4,   57,   "sun(type5tuv)",            KBD_GREEK				}, //  Greece5
	{ 4,   59,   "sun(type5tuv)",            KBD_LITHUANIAN				}, //  Lithuania5
	{ 4,   63,   "sun(type5tuv)",            KBD_CANADIAN_FRENCH			}, //  Canada_Fr5_TBITS5
	{ 4,   56,   "sun(type5tuv)",            KBD_TURKISH_Q				}, //  TurkeyQ5
	{ 4,   58,   "sun(type5tuv)",            KBD_ARABIC_101				}, //  Arabic5
	{ 4,   60,   "sun(type5tuv)",            KBD_BELGIAN_FRENCH			}, //  Belgian5
	{ 4,   62,   "sun(type5tuv)",            KBD_TURKISH_F				}, //  TurkeyF5
	{ 4,   80,   "sun(type5hobo)",           KBD_US					}, //  US5_Hobo
	{ 4,   81,   "sun(type5hobo)",           KBD_US					}, //  US_UNIX5_Hobo
	{ 4,   82,   "sun(type5tuvhobo)",        KBD_FRENCH				}, //  France5_Hobo
	{ 4,   83,   "sun(type5tuvhobo)",        KBD_DANISH				}, //  Denmark5_Hobo
	{ 4,   84,   "sun(type5tuvhobo)",        KBD_GERMAN				}, //  Germany5_Hobo
	{ 4,   85,   "sun(type5tuvhobo)",        KBD_ITALIAN				}, //  Italy5_Hobo
	{ 4,   86,   "sun(type5tuvhobo)",        KBD_DUTCH				}, //  Netherland5_Hobo
	{ 4,   87,   "sun(type5tuvhobo)",        KBD_NORWEGIAN				}, //  Norway5_Hobo
	{ 4,   88,   "sun(type5tuvhobo)",        KBD_PORTUGUESE				}, //  Portugal5_Hobo
	{ 4,   89,   "sun(type5tuvhobo)",        KBD_SPANISH				}, //  Spain5_Hobo
	{ 4,   90,   "sun(type5tuvhobo)",        KBD_SWEDISH				}, //  Sweden5_Hobo
	{ 4,   91,   "sun(type5tuvhobo)",        KBD_SWISS_FRENCH			}, //  Switzer_Fr5_Hobo
	{ 4,   92,   "sun(type5tuvhobo)",        KBD_SWISS_GERMAN			}, //  Switzer_Ge5_Hobo
	{ 4,   93,   "sun(type5tuvhobo)",        KBD_UNITED_KINGDOM			}, //  UK5_Hobo
	{ 4,   94,   "sun(type5hobo)",           KBD_KOREAN_INPUT_SYSTEM_IME_2000	}, //  Korea5_Hobo
	{ 4,   95,   "sun(type5hobo)",           KBD_CHINESE_TRADITIONAL_PHONETIC	}, //  Taiwan5_Hobo
	{ 4,   96,   "sun(type5jphobo)",         KBD_JAPANESE_INPUT_SYSTEM_MS_IME2002	}, //  Japan5_Hobo
	{ 4,   97,   "sun(type5tuvhobo)",        KBD_CANADIAN_FRENCH			}, //  Canada_Fr5_Hobo
	{ 101, 1,    "digital_vndr/pc(pc104)",   KBD_US					}, //  US101A_x86
	{ 101, 34,   "digital_vndr/pc(pc104)",   KBD_US					}, //  J3100_x86
	{ 101, 35,   "digital_vndr/pc(pc104)",   KBD_FRENCH				}, //  France_x86
	{ 101, 36,   "digital_vndr/pc(pc104)",   KBD_DANISH				}, //  Denmark_x86
	{ 101, 37,   "digital_vndr/pc(pc104)",   KBD_GERMAN				}, //  Germany_x86
	{ 101, 38,   "digital_vndr/pc(pc104)",   KBD_ITALIAN				}, //  Italy_x86
	{ 101, 39,   "digital_vndr/pc(pc104)",   KBD_DUTCH				}, //  Netherland_x86
	{ 101, 40,   "digital_vndr/pc(pc104)",   KBD_NORWEGIAN				}, //  Norway_x86
	{ 101, 41,   "digital_vndr/pc(pc104)",   KBD_PORTUGUESE				}, //  Portugal_x86
	{ 101, 42,   "digital_vndr/pc(pc104)",   KBD_SPANISH				}, //  Spain_x86
	{ 101, 43,   "digital_vndr/pc(pc104)",   KBD_SWEDISH				}, //  Sweden_x86
	{ 101, 44,   "digital_vndr/pc(pc104)",   KBD_SWISS_FRENCH			}, //  Switzer_Fr_x86
	{ 101, 45,   "digital_vndr/pc(pc104)",   KBD_SWISS_GERMAN			}, //  Switzer_Ge_x86
	{ 101, 46,   "digital_vndr/pc(pc104)",   KBD_UNITED_KINGDOM			}, //  UK_x86
	{ 101, 47,   "digital_vndr/pc(pc104)",   KBD_KOREAN_INPUT_SYSTEM_IME_2000	}, //  Korea_x86
	{ 101, 48,   "digital_vndr/pc(pc104)",   KBD_CHINESE_TRADITIONAL_PHONETIC	}, //  Taiwan_x86
	{ 101, 49,   "digital_vndr/pc(lk411jj)", KBD_JAPANESE_INPUT_SYSTEM_MS_IME2002	}, //  Japan_x86
	{ 101, 50,   "digital_vndr/pc(pc104)",   KBD_CANADIAN_FRENCH			}, //  Canada_Fr2_x86
	{ 101, 51,   "digital_vndr/pc(pc104)",   KBD_HUNGARIAN				}, //  Hungary_x86
	{ 101, 52,   "digital_vndr/pc(pc104)",   KBD_POLISH_214				}, //  Poland_x86
	{ 101, 53,   "digital_vndr/pc(pc104)",   KBD_CZECH				}, //  Czech_x86
	{ 101, 54,   "digital_vndr/pc(pc104)",   KBD_RUSSIAN				}, //  Russia_x86
	{ 101, 55,   "digital_vndr/pc(pc104)",   KBD_LATVIAN				}, //  Latvia_x86
	{ 101, 56,   "digital_vndr/pc(pc104)",   KBD_TURKISH_Q				}, //  Turkey_x86
	{ 101, 57,   "digital_vndr/pc(pc104)",   KBD_GREEK				}, //  Greece_x86
	{ 101, 59,   "digital_vndr/pc(pc104)",   KBD_LITHUANIAN				}, //  Lithuania_x86
	{ 101, 1001, "digital_vndr/pc(pc104)",   KBD_US					}, //  MS_US101A_x86
	{ 6,   6,    "sun(type6tuv)",            KBD_DANISH				}, //  Denmark6_usb
	{ 6,   7,    "sun(type6tuv)",            KBD_FINNISH				}, //  Finnish6_usb
	{ 6,   8,    "sun(type6tuv)",            KBD_FRENCH				}, //  France6_usb
	{ 6,   9,    "sun(type6tuv)",            KBD_GERMAN				}, //  Germany6_usb
	{ 6,   14,   "sun(type6tuv)",            KBD_ITALIAN				}, //  Italy6_usb
	{ 6,   15,   "sun(type6jp)",             KBD_JAPANESE_INPUT_SYSTEM_MS_IME2002	}, //  Japan7_usb
	{ 6,   16,   "sun(type6)",               KBD_KOREAN_INPUT_SYSTEM_IME_2000	}, //  Korea6_usb
	{ 6,   18,   "sun(type6tuv)",            KBD_DUTCH				}, //  Netherland6_usb
	{ 6,   19,   "sun(type6tuv)",            KBD_NORWEGIAN				}, //  Norway6_usb
	{ 6,   22,   "sun(type6tuv)",            KBD_PORTUGUESE				}, //  Portugal6_usb
	{ 6,   23,   "sun(type6tuv)",            KBD_RUSSIAN				}, //  Russia6_usb
	{ 6,   25,   "sun(type6tuv)",            KBD_SPANISH				}, //  Spain6_usb
	{ 6,   26,   "sun(type6tuv)",            KBD_SWEDISH				}, //  Sweden6_usb
	{ 6,   27,   "sun(type6tuv)",            KBD_SWISS_FRENCH			}, //  Switzer_Fr6_usb
	{ 6,   28,   "sun(type6tuv)",            KBD_SWISS_GERMAN			}, //  Switzer_Ge6_usb
	{ 6,   30,   "sun(type6)",               KBD_CHINESE_TRADITIONAL_PHONETIC	}, //  Taiwan6_usb
	{ 6,   32,   "sun(type6tuv)",            KBD_UNITED_KINGDOM			}, //  UK6_usb
	{ 6,   33,   "sun(type6)",               KBD_US					}, //  US6_usb
	{ 6,   1,    "sun(type6tuv)",            KBD_ARABIC_101				}, //  Arabic6_usb
	{ 6,   2,    "sun(type6tuv)",            KBD_BELGIAN_FRENCH			}, //  Belgian6_usb
	{ 6,   31,   "sun(type6tuv)",            KBD_TURKISH_Q				}, //  TurkeyQ6_usb
	{ 6,   35,   "sun(type6tuv)",            KBD_TURKISH_F				}, //  TurkeyF6_usb
	{ 6,   271,  "sun(type6jp)",             KBD_JAPANESE_INPUT_SYSTEM_MS_IME2002	}, //  Japan6_usb
	{ 6,   264,  "sun(type6tuv)",            KBD_ALBANIAN				}, //  Albanian6_usb
	{ 6,   261,  "sun(type6tuv)",            KBD_BELARUSIAN				}, //  Belarusian6_usb
	{ 6,   260,  "sun(type6tuv)",            KBD_BULGARIAN				}, //  Bulgarian6_usb
	{ 6,   259,  "sun(type6tuv)",            KBD_CROATIAN				}, //  Croatian6_usb
	{ 6,   5,    "sun(type6tuv)",            KBD_CZECH				}, //  Czech6_usb
	{ 6,   4,    "sun(type6tuv)",            KBD_CANADIAN_FRENCH			}, //  French-Canadian6_usb
	{ 6,   12,   "sun(type6tuv)",            KBD_HUNGARIAN				}, //  Hungarian6_usb
	{ 6,   10,   "sun(type6tuv)",            KBD_GREEK				}, //  Greek6_usb
	{ 6,   17,   "sun(type6)",               KBD_LATIN_AMERICAN			}, //  Latin-American6_usb
	{ 6,   265,  "sun(type6tuv)",            KBD_LITHUANIAN				}, //  Lithuanian6_usb
	{ 6,   266,  "sun(type6tuv)",            KBD_LATVIAN				}, //  Latvian6_usb
	{ 6,   267,  "sun(type6tuv)",            KBD_FYRO_MACEDONIAN			}, //  Macedonian6_usb
	{ 6,   263,  "sun(type6tuv)",            KBD_MALTESE_47_KEY			}, //  Malta_UK6_usb
	{ 6,   262,  "sun(type6tuv)",            KBD_MALTESE_48_KEY			}, //  Malta_US6_usb
	{ 6,   21,   "sun(type6tuv)",            KBD_POLISH_214				}, //  Polish6_usb
	{ 6,   257,  "sun(type6tuv)",            KBD_SERBIAN_LATIN			}, //  Serbia-And-Montenegro6_usb
	{ 6,   256,  "sun(type6tuv)",            KBD_SLOVENIAN				}, //  Slovenian6_usb
	{ 6,   24,   "sun(type6tuv)",            KBD_SLOVAK				}, //  Slovakian6_usb
	{ 6,   3,    "sun(type6)",               KBD_CANADIAN_MULTILINGUAL_STANDARD	}, //  Canada_Bi6_usb
	{ 6,   272,  "sun(type6)",               KBD_PORTUGUESE_BRAZILIAN_ABNT		}  //  Brazil6_usb
};

#endif // __KEYBOARD_H


