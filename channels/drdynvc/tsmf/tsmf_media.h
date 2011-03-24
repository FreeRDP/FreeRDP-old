/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel - Media Container

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

/*
   The media container maintains a global list of presentations, and a list of
   streams in each presentation.
*/

#ifndef __TSMF_MEDIA_H
#define __TSMF_MEDIA_H

#include <freerdp/types_ui.h>

typedef struct _TSMF_PRESENTATION TSMF_PRESENTATION;

typedef struct _TSMF_STREAM TSMF_STREAM;

TSMF_PRESENTATION *
tsmf_presentation_new (const uint8 * guid);
TSMF_PRESENTATION *
tsmf_presentation_find_by_id (const uint8 * guid);
void
tsmf_presentation_free (TSMF_PRESENTATION * presentation);

TSMF_STREAM *
tsmf_stream_new (TSMF_PRESENTATION * presentation, uint32 stream_id);
TSMF_STREAM *
tsmf_stream_find_by_id (TSMF_PRESENTATION * presentation, uint32 stream_id);
void
tsmf_stream_free (TSMF_STREAM * stream);

#endif

