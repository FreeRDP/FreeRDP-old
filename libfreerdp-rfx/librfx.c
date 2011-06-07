/*
   FreeRDP: A Remote Desktop Protocol client.
   RemoteFX Codec Library

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freerdp/rfx.h>
#include <freerdp/types/base.h>
#include <freerdp/utils/stream.h>

#include "rfx_pool.h"
#include "rfx_decode.h"

#include "librfx.h"

RFX_CONTEXT *
rfx_context_new(void)
{
	RFX_CONTEXT * context;

	context = (RFX_CONTEXT *) malloc(sizeof(RFX_CONTEXT));
	memset(context, 0, sizeof(RFX_CONTEXT));

	context->pool = rfx_pool_new();

	context->idwt_buffers[1] = (uint32*) context->idwt_buffer_8;
	context->idwt_buffers[2] = (uint32*) context->idwt_buffer_16;
	context->idwt_buffers[4] = (uint32*) context->idwt_buffer_32;

	return context;
}

void
rfx_context_free(RFX_CONTEXT * context)
{
	if (context->quants != NULL)
		free(context->quants);

	rfx_pool_free(context->pool);

	if (context != NULL)
		free(context);
}

void
rfx_context_set_pixel_format(RFX_CONTEXT * context, RFX_PIXEL_FORMAT pixel_format)
{
	context->pixel_format = pixel_format;
}

static void
rfx_process_message_sync(RFX_CONTEXT * context, uint8 * data, int data_size)
{
	uint32 magic;

	magic = GET_UINT32(data, 0);

	if (magic != WF_MAGIC)
	{
		DEBUG_RFX("invalid magic number 0x%X", magic);
		return;
	}

	context->version = GET_UINT16(data, 4);

	if (context->version != WF_VERSION_1_0)
	{
		DEBUG_RFX("unknown version number 0x%X", context->version);
		return;
	}

	DEBUG_RFX("version 0x%X", context->version);
}

static void
rfx_process_message_codec_versions(RFX_CONTEXT * context, uint8 * data, int data_size)
{
	int numCodecs;

	numCodecs = GET_UINT8(data, 0);

	if (numCodecs < 1)
	{
		DEBUG_RFX("no version.");
		return;
	}

	context->codec_id = GET_UINT8(data, 1);
	context->codec_version = GET_UINT16(data, 2);

	DEBUG_RFX("id %d version 0x%X.", context->codec_id, context->codec_version);
}

static void
rfx_process_message_channels(RFX_CONTEXT * context, uint8 * data, int data_size)
{
	int channelId;
	int numChannels;

	numChannels = GET_UINT8(data, 0);

	if (numChannels < 1)
	{
		DEBUG_RFX("no channel.");
		return;
	}

	channelId = GET_UINT8(data, 1);
	context->width = GET_UINT16(data, 2);
	context->height = GET_UINT16(data, 4);

	DEBUG_RFX("numChannels %d id %d, %dx%d.",
		numChannels, channelId, context->width, context->height);
}

static void
rfx_process_message_context(RFX_CONTEXT * context, uint8 * data, int data_size)
{
	uint8 ctxId;
	uint8 codecId;
	uint8 channelId;
	uint16 tileSize;
	uint16 properties;

	codecId = GET_UINT8(data, 0);
	channelId = GET_UINT8(data, 1);
	ctxId = GET_UINT8(data, 2);
	tileSize = GET_UINT16(data, 3);
	properties = GET_UINT16(data, 5);

	DEBUG_RFX("codec %d channel %d ctx %d tileSize %d properties 0x%X.",
		codecId, channelId, ctxId, tileSize, properties);

	context->flags = (properties & 0x0007);

	if (context->flags == CODEC_MODE)
		DEBUG_RFX("codec in image mode.");
	else
		DEBUG_RFX("codec in video mode.");

	switch ((properties & 0x1E00) >> 9)
	{
		case CLW_ENTROPY_RLGR1:
			context->mode = RLGR1;
			DEBUG_RFX("RLGR1.");
			break;

		case CLW_ENTROPY_RLGR3:
			context->mode = RLGR3;
			DEBUG_RFX("RLGR3.");
			break;

		default:
			DEBUG_RFX("unknown RLGR algorithm.");
			break;
	}
}

static void
rfx_process_message_region(RFX_CONTEXT * context, RFX_MESSAGE * message, uint8 * data, int data_size)
{
	int i;

	message->num_rects = GET_UINT16(data, 3);

	if (message->num_rects < 1)
	{
		DEBUG_RFX("no rects.");
		return;
	}

	if (message->rects != NULL)
		message->rects = (RFX_RECT*) realloc((void*) message->rects, message->num_rects * sizeof(RFX_RECT));
	else
		message->rects = (RFX_RECT*) malloc(message->num_rects * sizeof(RFX_RECT));

	data += 5;
	data_size -= 5;

	for (i = 0; i < message->num_rects && data_size > 0; i++)
	{
		message->rects[i].x = GET_UINT16(data, 0);
		message->rects[i].y = GET_UINT16(data, 2);
		message->rects[i].width = GET_UINT16(data, 4);
		message->rects[i].height = GET_UINT16(data, 6);

		DEBUG_RFX("rect %d (%d %d %d %d).",
			i, message->rects[i].x, message->rects[i].y, message->rects[i].width, message->rects[i].height);

		data += 8;
		data_size -= 8;
	}
}

static void
rfx_process_message_tile(RFX_CONTEXT * context, RFX_TILE * tile, uint8 * data, int data_size)
{
	uint8 quantIdxY;
	uint8 quantIdxCb;
	uint8 quantIdxCr;
	uint16 xIdx, yIdx;
	uint16 YLen, CbLen, CrLen;

	quantIdxY = GET_UINT8(data, 0);
	quantIdxCb = GET_UINT8(data, 1);
	quantIdxCr = GET_UINT8(data, 2);
	xIdx = GET_UINT16(data, 3);
	yIdx = GET_UINT16(data, 5);
	YLen = GET_UINT16(data, 7);
	CbLen = GET_UINT16(data, 9);
	CrLen = GET_UINT16(data, 11);

	DEBUG_RFX("quantIdxY:%d quantIdxCb:%d quantIdxCr:%d xIdx:%d yIdx:%d YLen:%d CbLen:%d CrLen:%d",
		quantIdxY, quantIdxCb, quantIdxCr, xIdx, yIdx, YLen, CbLen, CrLen);

	data += 13;

	tile->x = xIdx * 64;
	tile->y = yIdx * 64;

	rfx_decode_rgb(context,
		data, YLen, context->quants + (quantIdxY * 10),
		data + YLen, CbLen, context->quants + (quantIdxCb * 10),
		data + YLen + CbLen, CrLen, context->quants + (quantIdxCr * 10), tile->data);
}

static void
rfx_process_message_tileset(RFX_CONTEXT * context, RFX_MESSAGE * message, uint8 * data, int data_size)
{
	int i;
	uint32 blockLen;
	uint32 blockType;
	uint32 tileDataSize;

	context->num_quants = GET_UINT8(data, 4);

	if (context->num_quants < 1)
	{
		DEBUG_RFX("no quantization value.");
		return;
	}

	message->num_tiles = GET_UINT16(data, 6);

	if (message->num_tiles < 1)
	{
		DEBUG_RFX("no tiles.");
		return;
	}

	tileDataSize = GET_UINT32(data, 8);

	data += 12;
	data_size -= 12;

	if (context->quants != NULL)
		context->quants = (uint32*) realloc((void*) context->quants, context->num_quants * 10 * sizeof(uint32));
	else
		context->quants = (uint32*) malloc(context->num_quants * 10 * sizeof(uint32));

	for (i = 0; i < context->num_quants && data_size > 0; i++)
	{
		context->quants[i * 10] = (data[0] & 0x0F);
		context->quants[i * 10 + 1] = (data[0] >> 4);
		context->quants[i * 10 + 2] = (data[1] & 0x0F);
		context->quants[i * 10 + 3] = (data[1] >> 4);
		context->quants[i * 10 + 4] = (data[2] & 0x0F);
		context->quants[i * 10 + 5] = (data[2] >> 4);
		context->quants[i * 10 + 6] = (data[3] & 0x0F);
		context->quants[i * 10 + 7] = (data[3] >> 4);
		context->quants[i * 10 + 8] = (data[4] & 0x0F);
		context->quants[i * 10 + 9] = (data[4] >> 4);

		DEBUG_RFX("quant %d (%d %d %d %d %d %d %d %d %d %d).",
			i, context->quants[i * 10], context->quants[i * 10 + 1],
			context->quants[i * 10 + 2], context->quants[i * 10 + 3],
			context->quants[i * 10 + 4], context->quants[i * 10 + 5],
			context->quants[i * 10 + 6], context->quants[i * 10 + 7],
			context->quants[i * 10 + 8], context->quants[i * 10 + 9]);

		data += 5;
		data_size -= 5;
	}

	message->tiles = rfx_pool_get_tiles(context->pool, message->num_tiles);

	for (i = 0; i < message->num_tiles && data_size > 0; i++)
	{
		blockType = GET_UINT16(data, 0);
		blockLen = GET_UINT32(data, 2);

		switch (blockType)
		{
			case CBT_TILE:
				rfx_process_message_tile(context, message->tiles[i], data + 6, blockLen - 6);
				break;

			default:
				DEBUG_RFX("unknown block type 0x%X", blockType);
				break;
		}

		data_size -= blockLen;
		data += blockLen;
	}
}

RFX_MESSAGE *
rfx_process_message(RFX_CONTEXT * context, uint8 * data, int data_size)
{
	uint32 subtype;
	uint32 blockLen;
	uint32 blockType;
	RFX_MESSAGE * message;

	message = (RFX_MESSAGE *) malloc(sizeof(RFX_MESSAGE));
	memset(message, 0, sizeof(RFX_MESSAGE));

	while (data_size > 0)
	{
		blockType = GET_UINT16(data, 0);
		blockLen = GET_UINT32(data, 2);
		DEBUG_RFX("blockType 0x%X blockLen %d", blockType, blockLen);

		switch (blockType)
		{
			case WBT_SYNC:
				rfx_process_message_sync(context, data + 6, blockLen - 6);
				break;

			case WBT_CODEC_VERSIONS:
				rfx_process_message_codec_versions(context, data + 6, blockLen - 6);
				break;

			case WBT_CHANNELS:
				rfx_process_message_channels(context, data + 6, blockLen - 6);
				break;

			case WBT_CONTEXT:
				rfx_process_message_context(context, data + 6, blockLen - 6);
				break;

			case WBT_FRAME_BEGIN:
			case WBT_FRAME_END:
				/* Can be ignored. */
				break;

			case WBT_REGION:
				rfx_process_message_region(context, message, data + 6, blockLen - 6);
				break;

			case WBT_EXTENSION:
				subtype = GET_UINT16(data, 8);
				switch (subtype)
				{
					case CBT_TILESET:
						rfx_process_message_tileset(context, message, data + 10, blockLen - 10);
						break;
					default:
						DEBUG_RFX("unknown subtype 0x%X", subtype);
						break;
				}
				break;

			default:
				DEBUG_RFX("unknown blockType 0x%X", blockType);
				break;
		}

		data_size -= blockLen;
		data += blockLen;
	}

	return message;
}

void
rfx_message_free(RFX_CONTEXT * context, RFX_MESSAGE * message)
{
	if (message != NULL)
	{
		if (message->rects != NULL)
			free(message->rects);

		if (message->tiles != NULL)
		{
			rfx_pool_put_tiles(context->pool, message->tiles, message->num_tiles);
			free(message->tiles);
		}

		free(message);
	}
}
