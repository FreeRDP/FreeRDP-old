/*
   FreeRDP: A Remote Desktop Protocol client.
   Clipboard Virtual Channel

   Copyright 2009-2010 Jay Sorg
   Copyright 2010-2011 Vic Lee

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

#ifndef __CLIPRDR_MAIN_H
#define __CLIPRDR_MAIN_H

#include <freerdp/types/ui.h>

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
#define CF_FREERDP_HTML 0xd010
#define CF_FREERDP_PNG  0xd011
#define CF_FREERDP_JPEG 0xd012
#define CF_FREERDP_GIF  0xd013
#define CFSTR_HTML      "HTML Format"
#define CFSTR_PNG       "PNG"
#define CFSTR_JPEG      "JFIF"
#define CFSTR_GIF       "GIF"

#ifndef _WIN32

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
