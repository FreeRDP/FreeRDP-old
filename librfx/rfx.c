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
#include <freerdp/utils.h>
#include "rfx_constants.h"
#include "rfx_decode.h"
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

	/* temporary data within a frame */
	int num_rects;
	RFX_RECT * rects;
	int num_quants;
	int * quants;
};

RFX_CONTEXT *
rfx_context_new(void)
{
	RFX_CONTEXT * context;

	context = (RFX_CONTEXT *) malloc(sizeof(RFX_CONTEXT));
	memset(context, 0, sizeof(RFX_CONTEXT));
	return context;
}

void
rfx_context_free(RFX_CONTEXT * context)
{
	if (context->rects)
		free(context->rects);
	if (context->quants)
		free(context->quants);
	free(context);
}

static void
rfx_process_message_sync(RFX_CONTEXT * context, unsigned char * data, int data_size)
{
	unsigned int magic;

	magic = GET_UINT32(data, 0);
	if (magic != WF_MAGIC)
	{
		printf("rfx_process_message_sync: invalid magic number 0x%X\n", magic);
		return;
	}
	context->version = GET_UINT16(data, 4);
	if (context->version != WF_VERSION_1_0)
	{
		printf("rfx_process_message_sync: unknown version number 0x%X\n", context->version);
		return;
	}
	printf("rfx_process_message_sync: version 0x%X\n", context->version);
}

static void
rfx_process_message_codec_versions(RFX_CONTEXT * context, unsigned char * data, int data_size)
{
	int numCodecs;

	numCodecs = GET_UINT8(data, 0);
	if (numCodecs < 1)
	{
		printf("rfx_process_message_codec_versions: no version.\n");
		return;
	}
	context->codec_id = GET_UINT8(data, 1);
	context->codec_version = GET_UINT16(data, 2);
	printf("rfx_process_message_codec_versions: id %d version 0x%X.\n",
		context->codec_id, context->codec_version);
}

static void
rfx_process_message_channels(RFX_CONTEXT * context, unsigned char * data, int data_size)
{
	int numChannels;
	int channelId;

	numChannels = GET_UINT8(data, 0);
	if (numChannels < 1)
	{
		printf("rfx_process_message_channels: no channel.\n");
		return;
	}
	channelId = GET_UINT8(data, 1);
	context->width = GET_UINT16(data, 2);
	context->height = GET_UINT16(data, 4);
	printf("rfx_process_message_channels: numChannels %d id %d, %dx%d.\n",
		numChannels, channelId, context->width, context->height);
}

static void
rfx_process_message_context(RFX_CONTEXT * context, unsigned char * data, int data_size)
{
	int codecId;
	int channelId;
	int ctxId;
	int tileSize;
	unsigned int properties;

	codecId = GET_UINT8(data, 0);
	channelId = GET_UINT8(data, 1);
	ctxId = GET_UINT8(data, 2);
	tileSize = GET_UINT16(data, 3);
	properties = GET_UINT16(data, 5);
	printf("rfx_process_message_context: codec %d channel %d ctx %d tileSize %d properties 0x%X.\n",
		codecId, channelId, ctxId, tileSize, properties);

	context->flags = (properties & 0x0007);
	if (context->flags == CODEC_MODE)
		printf("rfx_process_message_context: codec in image mode.\n");
	else
		printf("rfx_process_message_context: codec in video mode.\n");

	switch ((properties & 0x1e00) >> 9)
	{
		case CLW_ENTROPY_RLGR1:
			context->mode = RLGR1;
			printf("rfx_process_message_context: RLGR1.\n");
			break;
		case CLW_ENTROPY_RLGR3:
			context->mode = RLGR3;
			printf("rfx_process_message_context: RLGR3.\n");
			break;
		default:
			printf("rfx_process_message_context: unknown RLGR algorithm.\n");
			break;
	}
}

static void
rfx_process_message_region(RFX_CONTEXT * context, unsigned char * data, int data_size)
{
	int i;

	context->num_rects = GET_UINT16(data, 3);
	if (context->num_rects < 1)
	{
		printf("rfx_process_message_region: no rects.\n");
		return;
	}
	if (context->rects)
		free(context->rects);
	context->rects = (RFX_RECT *) malloc(context->num_rects * sizeof(RFX_RECT));
	data += 5;
	data_size -= 5;
	for (i = 0; i < context->num_rects && data_size > 0; i++)
	{
		context->rects[i].x = GET_UINT16(data, 0);
		context->rects[i].y = GET_UINT16(data, 2);
		context->rects[i].width = GET_UINT16(data, 4);
		context->rects[i].height = GET_UINT16(data, 6);
		printf("rfx_process_message_region: rect %d (%d %d %d %d).\n",
			i, context->rects[i].x, context->rects[i].y, context->rects[i].width, context->rects[i].height);

		data += 8;
		data_size -= 8;
	}
}

static void
rfx_process_message_tile(RFX_CONTEXT * context, RFX_TILE * tile, unsigned char * data, int data_size)
{
	int quantIdxY, quantIdxCb, quantIdxCr;
	int xIdx, yIdx;
	int YLen, CbLen, CrLen;

	quantIdxY = GET_UINT8(data, 0);
	quantIdxCb = GET_UINT8(data, 1);
	quantIdxCr = GET_UINT8(data, 2);
	xIdx = GET_UINT16(data, 3);
	yIdx = GET_UINT16(data, 5);
	YLen = GET_UINT16(data, 7);
	CbLen = GET_UINT16(data, 9);
	CrLen = GET_UINT16(data, 11);
	printf("rfx_process_message_tile: %d %d %d %d %d %d %d %d\n",
		quantIdxY, quantIdxCb, quantIdxCr, xIdx, yIdx, YLen, CbLen, CrLen);

	data += 13;

	tile->x = xIdx * 64;
	tile->y = yIdx * 64;
	tile->data = rfx_decode_rgb(context->mode,
		data, YLen, context->quants + (quantIdxY * 10),
		data + YLen, CbLen, context->quants + (quantIdxCb * 10),
		data + YLen + CbLen, CrLen, context->quants + (quantIdxCr * 10));
}

static void
rfx_process_message_tileset(RFX_CONTEXT * context, RFX_MESSAGE * message, unsigned char * data, int data_size)
{
	unsigned int tileDataSize;
	int i;
	unsigned int blockType;
	unsigned int blockLen;

	context->num_quants = GET_UINT8(data, 4);
	if (context->num_quants < 1)
	{
		printf("rfx_process_message_tileset: no quantization value.\n");
		return;
	}
	message->num_tiles = GET_UINT16(data, 6);
	if (message->num_tiles < 1)
	{
		printf("rfx_process_message_tileset: no tiles.\n");
		return;
	}
	tileDataSize = GET_UINT32(data, 8);

	data += 12;
	data_size -= 12;

	if (context->quants)
		free(context->quants);
	context->quants = (int *) malloc(context->num_quants * 10 * sizeof(int));
	for (i = 0; i < context->num_quants && data_size > 0; i++)
	{
		context->quants[i * 10] = (data[0] & 0x0f);
		context->quants[i * 10 + 1] = (data[0] >> 4);
		context->quants[i * 10 + 2] = (data[1] & 0x0f);
		context->quants[i * 10 + 3] = (data[1] >> 4);
		context->quants[i * 10 + 4] = (data[2] & 0x0f);
		context->quants[i * 10 + 5] = (data[2] >> 4);
		context->quants[i * 10 + 6] = (data[3] & 0x0f);
		context->quants[i * 10 + 7] = (data[3] >> 4);
		context->quants[i * 10 + 8] = (data[4] & 0x0f);
		context->quants[i * 10 + 9] = (data[4] >> 4);
		printf("rfx_process_message_tileset: quant %d (%d %d %d %d %d %d %d %d %d %d).\n",
			i, context->quants[i * 10], context->quants[i * 10 + 1],
			context->quants[i * 10 + 2], context->quants[i * 10 + 3],
			context->quants[i * 10 + 4], context->quants[i * 10 + 5],
			context->quants[i * 10 + 6], context->quants[i * 10 + 7],
			context->quants[i * 10 + 8], context->quants[i * 10 + 9]);

		data += 5;
		data_size -= 5;
	}

	message->tiles = (RFX_TILE *) malloc(message->num_tiles * sizeof(RFX_TILE));

	for (i = 0; i < message->num_tiles && data_size > 0; i++)
	{
		blockType = GET_UINT16(data, 0);
		blockLen = GET_UINT32(data, 2);

		switch (blockType)
		{
			case CBT_TILE:
				rfx_process_message_tile(context, &message->tiles[i], data + 6, blockLen - 6);
				break;

			default:
				printf("rfx_process_message_tileset: unknown block type 0x%X\n", blockType);
				break;
		}

		data_size -= blockLen;
		data += blockLen;
	}
}

RFX_MESSAGE *
rfx_process_message(RFX_CONTEXT * context, unsigned char * data, int data_size)
{
	RFX_MESSAGE * message;
	unsigned int blockType;
	unsigned int blockLen;
	unsigned int subtype;

	message = (RFX_MESSAGE *) malloc(sizeof(RFX_MESSAGE));
	memset(message, 0, sizeof(RFX_MESSAGE));

	while (data_size > 0)
	{
		blockType = GET_UINT16(data, 0);
		blockLen = GET_UINT32(data, 2);
		//printf("rfx_process_message: blockType 0x%X blockLen %d\n", blockType, blockLen);

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
				rfx_process_message_region(context, data + 6, blockLen - 6);
				break;

			case WBT_EXTENSION:
				subtype = GET_UINT16(data, 8);
				switch (subtype)
				{
					case CBT_TILESET:
						rfx_process_message_tileset(context, message, data + 10, blockLen - 10);
						break;
					default:
						printf("rfx_process_message: unknown subtype 0x%X\n", subtype);
						break;
				}
				break;

			default:
				printf("rfx_process_message: unknown blockType 0x%X\n", blockType);
				break;
		}

		data_size -= blockLen;
		data += blockLen;
	}

	return message;
}

void
rfx_message_free(RFX_MESSAGE * message)
{
	int i;

	if (message->tiles)
	{
		for (i = 0; i < message->num_tiles; i++)
		{
			if (message->tiles[i].data)
				free(message->tiles[i].data);
		}
		free(message->tiles);
	}
	free(message);
}

