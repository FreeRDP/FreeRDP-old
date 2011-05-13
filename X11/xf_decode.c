/*
   FreeRDP: A Remote Desktop Protocol client.
   UI decode

   Copyright (C) Vic Lee 2011

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
#include <freerdp/utils.h>
#include <librfx/rfx.h>
#include "xf_types.h"
#include "xf_decode.h"

void
xf_decode_init(xfInfo * xfi)
{
	switch (xfi->codec)
	{
		case XF_CODEC_REMOTEFX:
			xfi->rfx_context = rfx_context_new();
			break;

		default:
			break;
	}
}

void
xf_decode_uninit(xfInfo * xfi)
{
	if (xfi->rfx_context)
		rfx_context_free((RFX_CONTEXT *) xfi->rfx_context);
}

static void
xf_decode_frame(xfInfo * xfi, uint8 * bitmapData, uint32 bitmapDataLength)
{
	RFX_MESSAGE * message;

	switch (xfi->codec)
	{
		case XF_CODEC_REMOTEFX:
			//printf("xf_decode_frame: RemoteFX frame size %d.\n", bitmapDataLength);
			message = rfx_process_message((RFX_CONTEXT *) xfi->rfx_context, bitmapData, bitmapDataLength);
			rfx_message_free(message);
			break;

		default:
			printf("xf_decode_frame: no codec defined.\n");
			break;
	}
}

void
xf_decode_data(xfInfo * xfi, uint8 * data, int data_size)
{
	uint16 cmdType;
	uint32 bitmapDataLength;
	int size;

	///printf("xf_decode_data: %d\n", data_size);
	while (data_size > 0)
	{
		cmdType = GET_UINT16(data, 0);
		//printf("xf_decode_data: cmdType %d\n", cmdType);
		switch (cmdType)
		{
			case CMDTYPE_SET_SURFACE_BITS:
			case CMDTYPE_STREAM_SURFACE_BITS:
				bitmapDataLength = GET_UINT32(data, 18);
				xf_decode_frame(xfi, data + 22, bitmapDataLength);
				size = 22 + bitmapDataLength;
				break;

			case CMDTYPE_FRAME_MARKER:
				size = 8;
				break;

			default:
				printf("xf_decode_data: unknown cmdType %d\n", cmdType);
				size = 2;
				break;
		}
		data_size -= size;
		data += size;
	}
}

