/*
   Copyright (c) 2009 Jay Sorg

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

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

#define CHANNEL_MAX_COUNT 30

#define CHANNEL_NAME_LEN 7

#define CHANNEL_CHUNK_LENGTH 1600

#endif
