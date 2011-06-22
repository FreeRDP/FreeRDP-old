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
#include "rfx_dwt.h"

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

	context->dwt_buffer_8 = (sint16 *)(((uintptr_t)context->dwt_mem_8 + 16) & ~ 0x0F);
	context->dwt_buffer_16 = (sint16 *)(((uintptr_t)context->dwt_mem_16 + 16) & ~ 0x0F);
	context->dwt_buffer_32 = (sint16 *)(((uintptr_t)context->dwt_mem_32 + 16) & ~ 0x0F);

	/* create profilers for default decoding routines */
	rfx_profiler_create(context);
	
	/* set up default routines */
	context->decode_YCbCr_to_RGB = rfx_decode_YCbCr_to_RGB;
	context->encode_RGB_to_YCbCr = rfx_encode_RGB_to_YCbCr;
	context->quantization_decode = rfx_quantization_decode;	
	context->dwt_2d_decode = rfx_dwt_2d_decode;

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
rfx_process_message_sync(RFX_CONTEXT * context, uint8 * data, int size)
{
	uint32 magic;

	/* RFX_SYNC */
	magic = GET_UINT32(data, 0); /* magic (4 bytes), 0xCACCACCA */

	if (magic != WF_MAGIC)
	{
		DEBUG_RFX("invalid magic number 0x%X", magic);
		return;
	}

	context->version = GET_UINT16(data, 4); /* version (2 bytes), WF_VERSION_1_0 (0x0100) */

	if (context->version != WF_VERSION_1_0)
	{
		DEBUG_RFX("unknown version number 0x%X", context->version);
		return;
	}

	DEBUG_RFX("version 0x%X", context->version);
}

static void
rfx_process_message_codec_versions(RFX_CONTEXT * context, uint8 * data, int size)
{
	int numCodecs;

	numCodecs = GET_UINT8(data, 0); /* numCodecs (1 byte), must be set to 0x01 */

	if (numCodecs != 1)
	{
		DEBUG_RFX("numCodecs: %d, expected:1", numCodecs);
		return;
	}

	/* RFX_CODEC_VERSIONT */
	context->codec_id = GET_UINT8(data, 1); /* codecId (1 byte) */
	context->codec_version = GET_UINT16(data, 2); /* version (2 bytes) */

	DEBUG_RFX("id %d version 0x%X.", context->codec_id, context->codec_version);
}

static void
rfx_process_message_channels(RFX_CONTEXT * context, uint8 * data, int size)
{
	int channelId;
	uint8 numChannels;

	numChannels = GET_UINT8(data, 0); /* numChannels (1 byte), must bet set to 0x01 */

	if (numChannels != 1)
	{
		DEBUG_RFX("numChannels:%d, expected:1", numChannels);
		return;
	}

	/* RFX_CHANNELT */
	channelId = GET_UINT8(data, 1); /* channelId (1 byte) */
	context->width = GET_UINT16(data, 2); /* width (2 bytes) */
	context->height = GET_UINT16(data, 4); /* height (2 bytes) */

	DEBUG_RFX("numChannels %d id %d, %dx%d.",
		numChannels, channelId, context->width, context->height);
}

static void
rfx_process_message_context(RFX_CONTEXT * context, uint8 * data, int size)
{
	uint8 ctxId;
	uint16 tileSize;
	uint16 properties;

	ctxId = GET_UINT8(data, 0); /* ctxId (1 byte), must be set to 0x00 */
	tileSize = GET_UINT16(data, 1); /* tileSize (2 bytes), must be set to CT_TILE_64x64 (0x0040) */
	properties = GET_UINT16(data, 3); /* properties (2 bytes) */

	DEBUG_RFX("ctxId %d tileSize %d properties 0x%X.", ctxId, tileSize, properties);

	context->flags = (properties & 0x0007);

	if (context->flags == CODEC_MODE)
		DEBUG_RFX("codec is in image mode.");
	else
		DEBUG_RFX("codec is in video mode.");

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
rfx_process_message_frame_begin(RFX_CONTEXT * context, RFX_MESSAGE * message, uint8 * data, int size)
{
	uint32 frameIdx;
	uint16 numRegions;

	frameIdx = GET_UINT32(data, 0); /* frameIdx (4 bytes), if codec is in video mode, must be ignored */
	numRegions = GET_UINT16(data, 4); /* numRegions (2 bytes) */

	DEBUG_RFX("RFX_FRAME_BEGIN: frameIdx:%d numRegions:%d", frameIdx, numRegions);
}

static void
rfx_process_message_frame_end(RFX_CONTEXT * context, RFX_MESSAGE * message, uint8 * data, int size)
{
	DEBUG_RFX("RFX_FRAME_END");
}

static void
rfx_process_message_region(RFX_CONTEXT * context, RFX_MESSAGE * message, uint8 * data, int size)
{
	int i;

	/* regionFlags (1 byte) */
	message->num_rects = GET_UINT16(data, 1); /* numRects (2 bytes) */

	if (message->num_rects < 1)
	{
		DEBUG_RFX("no rects.");
		return;
	}

	if (message->rects != NULL)
		message->rects = (RFX_RECT*) realloc((void*) message->rects, message->num_rects * sizeof(RFX_RECT));
	else
		message->rects = (RFX_RECT*) malloc(message->num_rects * sizeof(RFX_RECT));

	data += 3;
	size -= 3;

	/* rects */
	for (i = 0; i < message->num_rects && size > 0; i++)
	{
		/* RFX_RECT */
		message->rects[i].x = GET_UINT16(data, 0); /* x (2 bytes) */
		message->rects[i].y = GET_UINT16(data, 2); /* y (2 bytes) */
		message->rects[i].width = GET_UINT16(data, 4); /* width (2 bytes) */
		message->rects[i].height = GET_UINT16(data, 6); /* height (2 bytes) */

		DEBUG_RFX("rect %d (%d %d %d %d).",
			i, message->rects[i].x, message->rects[i].y, message->rects[i].width, message->rects[i].height);

		data += 8;
		size -= 8;
	}
}

static void
rfx_process_message_tile(RFX_CONTEXT * context, RFX_TILE * tile, uint8 * data, int size)
{
	uint8 quantIdxY;
	uint8 quantIdxCb;
	uint8 quantIdxCr;
	uint16 xIdx, yIdx;
	uint16 YLen, CbLen, CrLen;

	/* RFX_TILE */
	quantIdxY = GET_UINT8(data, 0); /* quantIdxY (1 byte) */
	quantIdxCb = GET_UINT8(data, 1); /* quantIdxCb (1 byte) */
	quantIdxCr = GET_UINT8(data, 2); /* quantIdxCr (1 byte) */
	xIdx = GET_UINT16(data, 3); /* xIdx (2 bytes) */
	yIdx = GET_UINT16(data, 5); /* yIdx (2 bytes) */
	YLen = GET_UINT16(data, 7); /* YLen (2 bytes) */
	CbLen = GET_UINT16(data, 9); /* CbLen (2 bytes) */
	CrLen = GET_UINT16(data, 11); /* CrLen (2 bytes) */

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
rfx_process_message_tileset(RFX_CONTEXT * context, RFX_MESSAGE * message, uint8 * data, int size)
{
	int i;
	uint16 subtype;
	uint32 blockLen;
	uint32 blockType;
	uint32 tilesDataSize;

	subtype = GET_UINT16(data, 0); /* subtype (2 bytes) must be set to CBT_TILESET (0xCAC2) */

	if (subtype != CBT_TILESET)
	{
		DEBUG_RFX("invalid subtype, expected CBT_TILESET.");
		return;
	}

	/* idx (2 bytes), must be set to 0x0000 */
	/* properties (2 bytes) */

	context->num_quants = GET_UINT8(data, 6); /* numQuant (1 byte) */
	/* tileSize (1 byte), must be set to 0x40 */

	if (context->num_quants < 1)
	{
		DEBUG_RFX("no quantization value.");
		return;
	}

	message->num_tiles = GET_UINT16(data, 8); /* numTiles (2 bytes) */

	if (message->num_tiles < 1)
	{
		DEBUG_RFX("no tiles.");
		return;
	}

	tilesDataSize = GET_UINT32(data, 10); /* tilesDataSize (4 bytes) */

	data += 14;
	size -= 14;

	if (context->quants != NULL)
		context->quants = (uint32*) realloc((void*) context->quants, context->num_quants * 10 * sizeof(uint32));
	else
		context->quants = (uint32*) malloc(context->num_quants * 10 * sizeof(uint32));

	/* quantVals */
	for (i = 0; i < context->num_quants && size > 0; i++)
	{
		/* RFX_CODEC_QUANT */
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
		size -= 5;
	}

	message->tiles = rfx_pool_get_tiles(context->pool, message->num_tiles);

	/* tiles */
	for (i = 0; i < message->num_tiles && size > 0; i++)
	{
		/* RFX_TILE */
		blockType = GET_UINT16(data, 0); /* blockType (2 bytes), must be set to CBT_TILE (0xCAC3) */
		blockLen = GET_UINT32(data, 2); /* blockLen (4 bytes) */

		if (blockType != CBT_TILE)
		{
			DEBUG_RFX("unknown block type 0x%X, expected CBT_TILE (0xCAC3).", blockType);
			break;
		}

		rfx_process_message_tile(context, message->tiles[i], data + 6, blockLen - 6);

		size -= blockLen;
		data += blockLen;
	}
}

RFX_MESSAGE *
rfx_process_message(RFX_CONTEXT * context, uint8 * data, int size)
{
	uint32 offset;
	uint32 blockLen;
	uint32 blockType;
	RFX_MESSAGE * message;

	message = (RFX_MESSAGE *) malloc(sizeof(RFX_MESSAGE));
	memset(message, 0, sizeof(RFX_MESSAGE));

	while (size > 0)
	{
		/* RFX_BLOCKT */
		blockType = GET_UINT16(data, 0); /* blockType (2 bytes) */
		blockLen = GET_UINT32(data, 2); /* blockLen (4 bytes) */
		offset = 6;

		DEBUG_RFX("blockType 0x%X blockLen %d", blockType, blockLen);

		if (blockType >= WBT_CONTEXT && blockType <= WBT_EXTENSION)
		{
			/* RFX_CODEC_CHANNELT */
			/* codecId (1 byte) must be set to 0x01 */
			/* channelId (1 byte) must be set to 0x00 */
			offset = 8;
		}

		switch (blockType)
		{
			case WBT_SYNC:
				rfx_process_message_sync(context, data + offset, blockLen - offset);
				break;

			case WBT_CODEC_VERSIONS:
				rfx_process_message_codec_versions(context, data + offset, blockLen - offset);
				break;

			case WBT_CHANNELS:
				rfx_process_message_channels(context, data + offset, blockLen - offset);
				break;

			case WBT_CONTEXT:
				rfx_process_message_context(context, data + offset, blockLen - offset);
				break;

			case WBT_FRAME_BEGIN:
				rfx_process_message_frame_begin(context, message, data + offset, blockLen - offset);
				break;

			case WBT_FRAME_END:
				rfx_process_message_frame_end(context, message, data + offset, blockLen - offset);
				break;

			case WBT_REGION:
				rfx_process_message_region(context, message, data + offset, blockLen - offset);
				break;

			case WBT_EXTENSION:
				rfx_process_message_tileset(context, message, data + offset, blockLen - offset);
				break;

			default:
				DEBUG_RFX("unknown blockType 0x%X", blockType);
				break;
		}

		size -= blockLen;
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
	SET_UINT16(buffer, 0, WBT_SYNC); /* BlockT.blockType */
	SET_UINT32(buffer, 2, 12); /* BlockT.blockLen */
	SET_UINT32(buffer, 6, WF_MAGIC); /* magic */
	SET_UINT16(buffer, 10, WF_VERSION_1_0); /* version */

	return 12;
}

static int
rfx_compose_message_codec_versions(RFX_CONTEXT * context, uint8 * buffer, int buffer_size)
{
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
	return 0;
}

static int
rfx_compose_message_region(RFX_CONTEXT * context, uint8 * buffer, int buffer_size,
	const RFX_RECT * rects, int num_rects)
{
	return 0;
}

static int
rfx_compose_message_tileset(RFX_CONTEXT * context, uint8 * buffer, int buffer_size,
	uint8 * image_buffer, int width, int height)
{
	return 0;
}

static int
rfx_compose_message_frame_end(RFX_CONTEXT * context, uint8 * buffer, int buffer_size)
{
	return 0;
}

int
rfx_compose_message_data(RFX_CONTEXT * context, uint8 * buffer, int buffer_size,
	const RFX_RECT * rects, int num_rects, uint8 * image_buffer, int width, int height)
{
	int composed_size;

	composed_size = rfx_compose_message_frame_begin(context, buffer, buffer_size);
	composed_size += rfx_compose_message_region(context, buffer + composed_size, buffer_size - composed_size,
		rects, num_rects);
	composed_size += rfx_compose_message_tileset(context, buffer + composed_size, buffer_size - composed_size,
		image_buffer, width, height);
	composed_size += rfx_compose_message_frame_end(context, buffer + composed_size, buffer_size - composed_size);

	return composed_size;
}
