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

void gdi_decode_bitmap_data(GDI *gdi, int x, int y, uint8 * data, uint32 length)
{
	int i, tx, ty;
	RFX_MESSAGE * message;

	message = rfx_process_message((RFX_CONTEXT *) gdi->rfx_context, data, length);

	if (message->num_rects > 1)
	{
		printf("warning: more than one clipping region\n");
	}

	for (i = 0; i < message->num_rects; i++)
	{
		tx = message->rects[i].x + x;
		ty = message->rects[i].y + y;
		gdi_SetClipRgn(gdi->primary->hdc, tx, ty, message->rects[i].width, message->rects[i].height);
	}

	for (i = 0; i < message->num_tiles; i++)
	{
		tx = message->tiles[i]->x + x;
		ty = message->tiles[i]->y + y;
		data = message->tiles[i]->data;

		gdi_image_convert(data, gdi->tile->bitmap->data, 64, 64, 32, 32, gdi->clrconv);
		gdi_BitBlt(gdi->primary->hdc, tx, ty, 64, 64, gdi->tile->hdc, 0, 0, GDI_SRCCOPY);

		gdi_InvalidateRegion(gdi->primary->hdc, tx, ty, 64, 64);
	}

	rfx_message_free(gdi->rfx_context, message);
}

int gdi_decode_surface_bits(GDI *gdi, uint8 * data, int size)
{
	int destLeft;
	int destTop;
	uint32 bitmapDataLength;

	/* SURFCMD_STREAM_SURF_BITS */
	/* cmdType (2 bytes) */
	destLeft = GET_UINT16(data, 2); /* destLeft (2 bytes) */
	destTop = GET_UINT16(data, 4); /* destTop (2 bytes) */
	/* destRight (2 bytes) */
	/* destBottom (2 bytes) */

	/* BITMAP_DATA_EX */
	/* bpp (1 byte) */
	/* reserved1 (1 byte) */
	/* reserved2 (1 byte) */
	/* codecID (1 byte) */
	/* width (2 bytes) */
	/* height (2 bytes) */
	bitmapDataLength = GET_UINT32(data, 18); /* bitmapDataLength (4 bytes) */
	gdi_decode_bitmap_data(gdi, destLeft, destTop, data + 22, bitmapDataLength); /* bitmapData*/

	return 22 + bitmapDataLength;
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
