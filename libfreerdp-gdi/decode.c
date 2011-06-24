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

#include <stdio.h>
#include <stdlib.h>
#include <freerdp/freerdp.h>
#include <freerdp/utils/stream.h>
#include <freerdp/rfx.h>

#include "gdi.h"
#include "gdi_bitmap.h"
#include "gdi_region.h"
#include "gdi_clipping.h"

#include "decode.h"

int gdi_decode_bitmap_data_ex(GDI *gdi, uint16 x, uint16 y, uint8 * data, int size)
{
	int i, j;
	int tx, ty;
	uint8* bitmapData;
	uint32 bitmapDataLength;
	RFX_MESSAGE * message;

	/* BITMAP_DATA_EX */
	/* bpp (1 byte) */
	/* reserved1 (1 byte) */
	/* reserved2 (1 byte) */
	/* codecID (1 byte) */
	/* width (2 bytes) */
	/* height (2 bytes) */
	bitmapDataLength = GET_UINT32(data, 8); /* bitmapDataLength (4 bytes) */
	bitmapData = data + 12; /* bitmapData */

	/* decode bitmap data */
	message = rfx_process_message((RFX_CONTEXT *) gdi->rfx_context, bitmapData, bitmapDataLength);

	/* blit each tile */
	for (i = 0; i < message->num_tiles; i++)
	{
		tx = message->tiles[i]->x + x;
		ty = message->tiles[i]->y + y;
		data = message->tiles[i]->data;

		gdi_image_convert(data, gdi->tile->bitmap->data, 64, 64, 32, 32, gdi->clrconv);

		for (j = 0; j < message->num_rects; j++)
		{
			gdi_SetClipRgn(gdi->primary->hdc,
					message->rects[j].x, message->rects[j].y,
					message->rects[j].width, message->rects[j].height);

			gdi_BitBlt(gdi->primary->hdc, tx, ty, 64, 64, gdi->tile->hdc, 0, 0, GDI_SRCCOPY);
		}
	}

	for (i = 0; i < message->num_rects; i++)
	{
		gdi_InvalidateRegion(gdi->primary->hdc,
				message->rects[i].x, message->rects[i].y,
				message->rects[i].width, message->rects[i].height);
	}

	rfx_message_free(gdi->rfx_context, message);

	return bitmapDataLength + 12;
}

int gdi_decode_surface_bits(GDI *gdi, uint8 * data, int size)
{
	int length;
	uint16 destLeft;
	uint16 destTop;
	uint16 destRight;
	uint16 destBottom;

	/* SURFCMD_STREAM_SURF_BITS */
	/* cmdType (2 bytes) */
	destLeft = GET_UINT16(data, 2); /* destLeft (2 bytes) */
	destTop = GET_UINT16(data, 4); /* destTop (2 bytes) */
	destRight = GET_UINT16(data, 6); /* destRight (2 bytes) */
	destBottom = GET_UINT16(data, 8); /* destBottom (2 bytes) */

	/* set clipping region */
	gdi_SetClipRgn(gdi->primary->hdc, destLeft, destTop, destRight - destLeft, destBottom - destTop);

	/* decode extended bitmap data */
	length = gdi_decode_bitmap_data_ex(gdi, destLeft, destTop, data + 10, size - 10) + 10;

	return length;
}

int gdi_decode_frame_marker(GDI *gdi, uint8 * data, int size)
{
	uint16 frameAction;
	uint32 frameId;

	frameAction = GET_UINT16(data, 0); /* frameAction */
	frameId = GET_UINT32(data, 2); /* frameId */

	switch (frameAction)
	{
		case SURFACECMD_FRAMEACTION_BEGIN:
			break;

		case SURFACECMD_FRAMEACTION_END:
			break;

		default:
			break;
	}

	return 8;
}

void gdi_decode_data(GDI *gdi, uint8 * data, int size)
{
	int cmdLength;
	uint16 cmdType;

	while (size > 0)
	{
		cmdType = GET_UINT16(data, 0); /* cmdType */

		switch (cmdType)
		{
			case CMDTYPE_SET_SURFACE_BITS:
			case CMDTYPE_STREAM_SURFACE_BITS:
				cmdLength = gdi_decode_surface_bits(gdi, data, size);
				break;

			case CMDTYPE_FRAME_MARKER:
				cmdLength = gdi_decode_frame_marker(gdi, data, size);
				break;

			default:
				cmdLength = 2;
				break;
		}

		size -= cmdLength;
		data += cmdLength;
	}
}
