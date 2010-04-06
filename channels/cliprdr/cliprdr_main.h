/*
   Copyright (c) 2009-2010 Jay Sorg
   Copyright (c) 2010 Vic Lee

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

*/

#ifndef __CLIPRDR_MAIN_H
#define __CLIPRDR_MAIN_H

/* CLIPRDR_HEADER.msgType */
#define CB_MONITOR_READY           1
#define CB_FORMAT_LIST             2
#define CB_FORMAT_LIST_RESPONSE    3
#define CB_FORMAT_DATA_REQUEST     4
#define CB_FORMAT_DATA_RESPONSE    5
#define CB_TEMP_DIRECTORY          6
#define CB_CLIP_CAPS               7
#define CB_FILECONTENTS_REQUEST    8
#define CB_FILECONTENTS_RESPONSE   9
#define CB_LOCK_CLIPDATA          10
#define CB_UNLOCK_CLIPDATA        11

/* CLIPRDR_HEADER.msgFlags */
#define CB_RESPONSE_OK             1
#define CB_RESPONSE_FAIL           2
#define CB_ASCII_NAMES             4

/* CLIPRDR_CAPS_SET.capabilitySetType */
#define CB_CAPSTYPE_GENERAL        1

/* CLIPRDR_GENERAL_CAPABILITY.lengthCapability */
#define CB_CAPSTYPE_GENERAL_LEN     12

/* CLIPRDR_GENERAL_CAPABILITY.version */
#define CB_CAPS_VERSION_1            1
#define CB_CAPS_VERSION_2            2

/* CLIPRDR_GENERAL_CAPABILITY.generalFlags */
#define CB_USE_LONG_FORMAT_NAMES     2
#define CB_STREAM_FILECLIP_ENABLED   4
#define CB_FILECLIP_NO_FILE_PATHS    8
#define CB_CAN_LOCK_CLIPDATA        16

/* Clipboard constants, "borrowed" from GCC system headers in 
   the w32 cross compiler
   this is the CF_ set when WINVER is 0x0400 */

#define CF_RAW          0

#ifndef WIN32

#ifndef CF_TEXT
#define CF_TEXT         1
#define CF_BITMAP       2
#define CF_METAFILEPICT 3
#define CF_SYLK         4
#define CF_DIF          5
#define CF_TIFF         6
#define CF_OEMTEXT      7
#define CF_DIB          8
#define CF_PALETTE      9
#define CF_PENDATA      10
#define CF_RIFF         11
#define CF_WAVE         12
#define CF_UNICODETEXT  13
#define CF_ENHMETAFILE  14
#define CF_HDROP        15
#define CF_LOCALE       16
#define CF_MAX          17
#define CF_OWNERDISPLAY 128
#define CF_DSPTEXT      129
#define CF_DSPBITMAP    130
#define CF_DSPMETAFILEPICT      131
#define CF_DSPENHMETAFILE       142
#define CF_PRIVATEFIRST 512
#define CF_PRIVATELAST  767
#define CF_GDIOBJFIRST  768
#define CF_GDIOBJLAST   1023
#endif

#endif

typedef struct cliprdr_plugin cliprdrPlugin;

int
cliprdr_send_packet(cliprdrPlugin * plugin, int type, int flag,
	char * data, int length);

/* implementations are in the hardware file */
void *
clipboard_new(cliprdrPlugin * plugin);
int
clipboard_sync(void * device_data);
int
clipboard_format_list(void * device_data, int flag,
	char * data, int length);
int
clipboard_format_list_response(void * device_data, int flag);
int
clipboard_request_data(void * device_data, uint32 format);
int
clipboard_handle_data(void * device_data, int flag,
	char * data, int length);
int
clipboard_handle_caps(void * device_data, int flag,
	char * data, int length);
void
clipboard_free(void * device_data);

#endif

