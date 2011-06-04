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

#ifndef __CONSTANTS_VCHAN_H
#define __CONSTANTS_VCHAN_H

#define CHANNEL_FLAG_FIRST 0x01
#define CHANNEL_FLAG_LAST 0x02
#define CHANNEL_FLAG_ONLY (CHANNEL_FLAG_FIRST | CHANNEL_FLAG_LAST)
#define CHANNEL_FLAG_MIDDLE 0
#define CHANNEL_FLAG_FAIL 0x100
#define CHANNEL_FLAG_SHOW_PROTOCOL 0x10
#define CHANNEL_FLAG_SUSPEND 0x20
#define CHANNEL_FLAG_RESUME 0x40

#define CHANNEL_OPTION_INITIALIZED   0x80000000
#define CHANNEL_OPTION_ENCRYPT_RDP   0x40000000
#define CHANNEL_OPTION_ENCRYPT_SC    0x20000000
#define CHANNEL_OPTION_ENCRYPT_CS    0x10000000
#define CHANNEL_OPTION_PRI_HIGH      0x08000000
#define CHANNEL_OPTION_PRI_MED       0x04000000
#define CHANNEL_OPTION_PRI_LOW       0x02000000
#define CHANNEL_OPTION_COMPRESS_RDP  0x00800000
#define CHANNEL_OPTION_COMPRESS      0x00400000
#define CHANNEL_OPTION_SHOW_PROTOCOL 0x00200000

#define CHANNEL_NAME_LEN 7

#define CHANNEL_CHUNK_LENGTH 1600

#endif
