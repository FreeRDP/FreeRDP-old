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
#include "xf_types.h"
#include <freerdp/rfx.h>
#include <freerdp/utils/stream.h>

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
xf_decode_frame(xfInfo * xfi, int x, int y, uint8 * bitmapData, uint32 bitmapDataLength)
{
	int i;
	int tx, ty;
	XImage * image;
	RFX_MESSAGE * message;

	switch (xfi->codec)
	{
		case XF_CODEC_REMOTEFX:

			message = rfx_process_message((RFX_CONTEXT *) xfi->rfx_context, bitmapData, bitmapDataLength);

			/* Clip the updated region based on the union of rects, so that pixels outside of the region will not be drawn. */
			XSetFunction(xfi->display, xfi->gc, GXcopy);
			XSetFillStyle(xfi->display, xfi->gc, FillSolid);
			XSetClipRectangles(xfi->display, xfi->gc, x, y, (XRectangle*)message->rects, message->num_rects, YXBanded);

			/* Draw the tiles to backstore, each is 64x64. */
			for (i = 0; i < message->num_tiles; i++)
			{
				image = XCreateImage(xfi->display, xfi->visual, 24, ZPixmap, 0,
					(char *) message->tiles[i]->data, 64, 64, 32, 0);
				tx = message->tiles[i]->x + x;
				ty = message->tiles[i]->y + y;
				XPutImage(xfi->display, xfi->backstore, xfi->gc, image, 0, 0, tx, ty, 64, 64);
				XFree(image);
			}

			/* Copy the updated region from backstore to the window. */
			for (i = 0; i < message->num_rects; i++)
			{
				tx = message->rects[i].x + x;
				ty = message->rects[i].y + y;
				XCopyArea(xfi->display, xfi->backstore, xfi->wnd, xfi->gc_default,
					tx, ty, message->rects[i].width, message->rects[i].height, tx, ty);
			}
			rfx_message_free(xfi->rfx_context, message);

			XSetClipMask(xfi->display, xfi->gc, None);

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
	int destLeft;
	int destTop;
	int size;

	while (data_size > 0)
	{
		cmdType = GET_UINT16(data, 0);
		switch (cmdType)
		{
			case CMDTYPE_SET_SURFACE_BITS:
			case CMDTYPE_STREAM_SURFACE_BITS:
				destLeft = GET_UINT16(data, 2);
				destTop = GET_UINT16(data, 4);
				bitmapDataLength = GET_UINT32(data, 18);
				xf_decode_frame(xfi, destLeft, destTop, data + 22, bitmapDataLength);
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

