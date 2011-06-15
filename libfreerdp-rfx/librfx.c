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
#include <inttypes.h>
#include <freerdp/rfx.h>
#include <freerdp/types/base.h>
#include <freerdp/utils/stream.h>

#include "rfx_pool.h"
#include "rfx_decode.h"
#include "rfx_encode.h"
#include "rfx_quantization.h"

#include "librfx.h"

void rfx_profiler_create(RFX_CONTEXT * context)
{
	PROFILER_CREATE(context->prof_rfx_decode_rgb, "rfx_decode_rgb");
	PROFILER_CREATE(context->prof_rfx_decode_component, "rfx_decode_component");
	PROFILER_CREATE(context->prof_rfx_rlgr_decode, "rfx_rlgr_decode");
	PROFILER_CREATE(context->prof_rfx_differential_decode, "rfx_differential_decode");
	PROFILER_CREATE(context->prof_rfx_quantization_decode, "rfx_quantization_decode");
	PROFILER_CREATE(context->prof_rfx_dwt_2d_decode, "rfx_dwt_2d_decode");
	PROFILER_CREATE(context->prof_rfx_decode_YCbCr_to_RGB, "rfx_decode_YCbCr_to_RGB");
}

void rfx_profiler_free(RFX_CONTEXT * context)
{
	PROFILER_FREE(context->prof_rfx_decode_rgb);
	PROFILER_FREE(context->prof_rfx_decode_component);
	PROFILER_FREE(context->prof_rfx_rlgr_decode);
	PROFILER_FREE(context->prof_rfx_differential_decode);
	PROFILER_FREE(context->prof_rfx_quantization_decode);
	PROFILER_FREE(context->prof_rfx_dwt_2d_decode);
	PROFILER_FREE(context->prof_rfx_decode_YCbCr_to_RGB);
}

void rfx_profiler_print(RFX_CONTEXT * context)
{
	PROFILER_PRINT_HEADER;
	PROFILER_PRINT(context->prof_rfx_decode_rgb);
	PROFILER_PRINT(context->prof_rfx_decode_component);
	PROFILER_PRINT(context->prof_rfx_rlgr_decode);
	PROFILER_PRINT(context->prof_rfx_differential_decode);
	PROFILER_PRINT(context->prof_rfx_quantization_decode);
	PROFILER_PRINT(context->prof_rfx_dwt_2d_decode);
	PROFILER_PRINT(context->prof_rfx_decode_YCbCr_to_RGB);
	PROFILER_PRINT_FOOTER;
}

RFX_CONTEXT *
rfx_context_new(void)
{
	RFX_CONTEXT * context;

	context = (RFX_CONTEXT *) malloc(sizeof(RFX_CONTEXT));
	memset(context, 0, sizeof(RFX_CONTEXT));

	context->pool = rfx_pool_new();

	/* align buffers to 16 byte boundary (needed for SSE/SSE2 instructions) */
	context->y_r_buffer = (sint16 *)(((uintptr_t)context->y_r_mem + 16) & ~ 0x0F);
	context->cb_g_buffer = (sint16 *)(((uintptr_t)context->cb_g_mem + 16) & ~ 0x0F);
	context->cr_b_buffer = (sint16 *)(((uintptr_t)context->cr_b_mem + 16) & ~ 0x0F);

	context->dwt_buffers[1] = (sint16*) context->dwt_buffer_8;
	context->dwt_buffers[2] = (sint16*) context->dwt_buffer_16;
	context->dwt_buffers[4] = (sint16*) context->dwt_buffer_32;

	/* create profilers for default decoding routines */
	rfx_profiler_create(context);
	
	/* set up default routines */
	context->decode_YCbCr_to_RGB = rfx_decode_YCbCr_to_RGB;
	context->encode_RGB_to_YCbCr = rfx_encode_RGB_to_YCbCr;
	context->quantization_decode = rfx_quantization_decode;	

	/* detect and enable SIMD CPU acceleration */
	RFX_INIT_SIMD(context);

	return context;
}

void
rfx_context_free(RFX_CONTEXT * context)
{
	if (context->quants != NULL)
		free(context->quants);

	rfx_pool_free(context->pool);

	rfx_profiler_print(context);
	rfx_profiler_free(context);

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

static int
rfx_compose_message_sync(RFX_CONTEXT * context, uint8 * buffer, int buffer_size)
{
	if (buffer_size < 12)
	{
		printf("rfx_compose_message_sync: buffer size too small.\n");
		return 0;
	}

	SET_UINT16(buffer, 0, WBT_SYNC); /* BlockT.blockType */
	SET_UINT32(buffer, 2, 12); /* BlockT.blockLen */
	SET_UINT32(buffer, 6, WF_MAGIC); /* magic */
	SET_UINT16(buffer, 10, WF_VERSION_1_0); /* version */

	return 12;
}

static int
rfx_compose_message_codec_versions(RFX_CONTEXT * context, uint8 * buffer, int buffer_size)
{
	if (buffer_size < 10)
	{
		printf("rfx_compose_message_codec_versions: buffer size too small.\n");
		return 0;
	}

	SET_UINT16(buffer, 0, WBT_CODEC_VERSIONS); /* BlockT.blockType */
	SET_UINT32(buffer, 2, 10); /* BlockT.blockLen */
	SET_UINT8(buffer, 6, 1); /* numCodecs */
	SET_UINT8(buffer, 7, 1); /* codecs.codecId */
	SET_UINT16(buffer, 8, WF_VERSION_1_0); /* codecs.version */

	return 10;
}

static int
rfx_compose_message_channels(RFX_CONTEXT * context, uint8 * buffer, int buffer_size)
{
	if (buffer_size < 12)
	{
		printf("rfx_compose_message_channels: buffer size too small.\n");
		return 0;
	}

	SET_UINT16(buffer, 0, WBT_CHANNELS); /* BlockT.blockType */
	SET_UINT32(buffer, 2, 12); /* BlockT.blockLen */
	SET_UINT8(buffer, 6, 1); /* numChannels */
	SET_UINT8(buffer, 7, 0); /* Channel.channelId */
	SET_UINT16(buffer, 8, context->width); /* Channel.width */
	SET_UINT16(buffer, 10, context->height); /* Channel.height */

	return 12;
}

static int
rfx_compose_message_context(RFX_CONTEXT * context, uint8 * buffer, int buffer_size)
{
	uint16 properties;

	if (buffer_size < 13)
	{
		printf("rfx_compose_message_context: buffer size too small.\n");
		return 0;
	}

	SET_UINT16(buffer, 0, WBT_CONTEXT); /* CodecChannelT.blockType */
	SET_UINT32(buffer, 2, 13); /* CodecChannelT.blockLen */
	SET_UINT8(buffer, 6, 1); /* CodecChannelT.codecId */
	SET_UINT8(buffer, 7, 0); /* CodecChannelT.channelId */
	SET_UINT8(buffer, 8, 0); /* ctxId */
	SET_UINT16(buffer, 9, CT_TILE_64x64); /* tileSize */

	/* properties */
	properties = context->flags; /* flags */
	properties |= (COL_CONV_ICT << 3); /* cct */
	properties |= (CLW_XFORM_DWT_53_A << 5); /* xft */
	properties |= ((context->mode == RLGR1 ? CLW_ENTROPY_RLGR1 : CLW_ENTROPY_RLGR3) << 9); /* et */
	properties |= (SCALAR_QUANTIZATION << 13); /* qt */
	SET_UINT16(buffer, 11, properties);

	return 13;
}

int
rfx_compose_message_header(RFX_CONTEXT * context, uint8 * buffer, int buffer_size)
{
	int composed_size;

	composed_size = rfx_compose_message_sync(context, buffer, buffer_size);
	composed_size += rfx_compose_message_codec_versions(context, buffer + composed_size, buffer_size - composed_size);
	composed_size += rfx_compose_message_channels(context, buffer + composed_size, buffer_size - composed_size);
	composed_size += rfx_compose_message_context(context, buffer + composed_size, buffer_size - composed_size);

	return composed_size;
}

static int
rfx_compose_message_frame_begin(RFX_CONTEXT * context, uint8 * buffer, int buffer_size)
{
	if (buffer_size < 14)
	{
		printf("rfx_compose_message_frame_begin: buffer size too small.\n");
		return 0;
	}

	SET_UINT16(buffer, 0, WBT_FRAME_BEGIN); /* CodecChannelT.blockType */
	SET_UINT32(buffer, 2, 14); /* CodecChannelT.blockLen */
	SET_UINT8(buffer, 6, 1); /* CodecChannelT.codecId */
	SET_UINT8(buffer, 7, 0); /* CodecChannelT.channelId */
	SET_UINT32(buffer, 8, context->frame_idx); /* frameIdx */
	SET_UINT16(buffer, 12, 1); /* numRegions */

	return 14;
}

static int
rfx_compose_message_region(RFX_CONTEXT * context, uint8 * buffer, int buffer_size,
	const RFX_RECT * rects, int num_rects)
{
	return 0;
}

static int
rfx_compose_message_tileset(RFX_CONTEXT * context, uint8 * buffer, int buffer_size,
	uint8 * image_data, int width, int height, int rowstride)
{
	return 0;
}

static int
rfx_compose_message_frame_end(RFX_CONTEXT * context, uint8 * buffer, int buffer_size)
{
	if (buffer_size < 8)
	{
		printf("rfx_compose_message_frame_end: buffer size too small.\n");
		return 0;
	}

	SET_UINT16(buffer, 0, WBT_FRAME_END); /* CodecChannelT.blockType */
	SET_UINT32(buffer, 2, 8); /* CodecChannelT.blockLen */
	SET_UINT8(buffer, 6, 1); /* CodecChannelT.codecId */
	SET_UINT8(buffer, 7, 0); /* CodecChannelT.channelId */

	return 8;
}

int
rfx_compose_message_data(RFX_CONTEXT * context, uint8 * buffer, int buffer_size,
	const RFX_RECT * rects, int num_rects, uint8 * image_data, int width, int height, int rowstride)
{
	int composed_size;

	composed_size = rfx_compose_message_frame_begin(context, buffer, buffer_size);
	composed_size += rfx_compose_message_region(context, buffer + composed_size, buffer_size - composed_size,
		rects, num_rects);
	composed_size += rfx_compose_message_tileset(context, buffer + composed_size, buffer_size - composed_size,
		image_data, width, height, rowstride);
	composed_size += rfx_compose_message_frame_end(context, buffer + composed_size, buffer_size - composed_size);

	return composed_size;
}
