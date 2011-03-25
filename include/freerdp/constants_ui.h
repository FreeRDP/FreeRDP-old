/*
   FreeRDP: A Remote Desktop Protocol client.

   Copyright (C) Jay Sorg 2009-2011

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   these are the types needed for the ui interface
   self contained file, requires no other includes

*/

#ifndef __CONSTANTS_UI_H
#define __CONSTANTS_UI_H

/* TCP port for Remote Desktop Protocol */
#define TCP_PORT_RDP 3389

/* Toggle key synchronization */
#define KBD_SYNC_SCROLL_LOCK	0x0001
#define KBD_SYNC_NUM_LOCK	0x0002
#define KBD_SYNC_CAPS_LOCK	0x0004
#define KBD_SYNC_KANA_LOCK	0x0008

/* @msdn{cc240586} */
#define PTRFLAGS_WHEEL_NEGATIVE 0x0100
#define PTRFLAGS_WHEEL          0x0200
#define PTRFLAGS_WHEEL_ROTATION_MASK 0x01ff
#define PTRFLAGS_MOVE           0x0800
#define PTRFLAGS_BUTTON1        0x1000
#define PTRFLAGS_BUTTON2        0x2000
#define PTRFLAGS_BUTTON3        0x4000
#define PTRFLAGS_DOWN           0x8000

#define PERF_FLAG_NONE                  0x00000000
#define PERF_DISABLE_WALLPAPER          0x00000001
#define PERF_DISABLE_FULLWINDOWDRAG     0x00000002
#define PERF_DISABLE_MENUANIMATIONS     0x00000004
#define PERF_DISABLE_THEMING            0x00000008
#define PERF_RESERVED1                  0x00000010
#define PERF_DISABLE_CURSOR_SHADOW      0x00000020
#define PERF_DISABLE_CURSORSETTINGS     0x00000040
#define PERF_ENABLE_FONT_SMOOTHING      0x00000080
#define PERF_ENABLE_DESKTOP_COMPOSITION 0x00000100

#endif
