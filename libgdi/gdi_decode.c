/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI RemoteFX Decoder

   Copyright 2010-2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#include <freerdp/utils.h>
#include <freerdp/freerdp.h>
#include "rfx.h"
#include "gdi.h"
#include <stdio.h>
#include <stdlib.h>

#include "gdi_decode.h"

void gdi_decode_frame(GDI *gdi, int x, int y, uint8 * data, uint32 length)
{
	int i, tx, ty;
	RFX_MESSAGE * message;

	message = rfx_process_message((RFX_CONTEXT *) gdi->rfx_context, data, length);

	for (i = 0; i < message->num_rects; i++)
	{
		tx = message->rects[i].x + x;
		ty = message->rects[i].y + y;
		SetClipRgn(gdi->primary->hdc, tx, ty, message->rects[i].width, message->rects[i].height);
	}

	for (i = 0; i < message->num_tiles; i++)
	{
		tx = message->tiles[i].x + x;
		ty = message->tiles[i].y + y;
		data = message->tiles[i].data;

		gdi_image_convert(data, gdi->tile->bitmap->data, 64, 64, 32, 32, gdi->clrconv);
		BitBlt(gdi->primary->hdc, tx, ty, 64, 64, gdi->tile->hdc, 0, 0, SRCCOPY);

		InvalidateRegion(gdi->primary->hdc, tx, ty, 64, 64);
	}

	rfx_message_free(message);
}

void gdi_decode_data(GDI *gdi, uint8 * data, int data_size)
{
	int size;
	int destLeft;
	int destTop;
	uint16 cmdType;
	uint32 length;

	while (data_size > 0)
	{
		cmdType = GET_UINT16(data, 0);

		switch (cmdType)
		{
			case CMDTYPE_SET_SURFACE_BITS:
			case CMDTYPE_STREAM_SURFACE_BITS:
				destLeft = GET_UINT16(data, 2);
				destTop = GET_UINT16(data, 4);
				length = GET_UINT32(data, 18);
				gdi_decode_frame(gdi, destLeft, destTop, data + 22, length);
				size = 22 + length;
				break;

			case CMDTYPE_FRAME_MARKER:
				size = 8;
				break;

			default:
				size = 2;
				break;
		}
		data_size -= size;
		data += size;
	}
}
