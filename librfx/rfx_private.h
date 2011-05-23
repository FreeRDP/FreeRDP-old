/*
   FreeRDP: A Remote Desktop Protocol client.
   RemoteFX Codec Library - Private Structures

   Copyright 2011 Vic Lee

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

#ifndef __RFX_PRIVATE_H
#define __RFX_PRIVATE_H

#include "rfx_rlgr.h"
#include "rfx.h"

typedef struct _RFX_RECT
{
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
} RFX_RECT;

struct _RFX_CONTEXT
{
	unsigned int version;
	unsigned int codec_id;
	unsigned int codec_version;
	int width;
	int height;
	int flags;
	RLGR_MODE mode;
	RFX_PIXEL_FORMAT pixel_format;

	/* temporary data within a frame */
	int num_rects;
	RFX_RECT * rects;
	int num_quants;
	int * quants;
};

#endif

