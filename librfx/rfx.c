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

struct _RFX_CONTEXT
{
	unsigned int version;
	unsigned int codec_id;
	unsigned int codec_version;
	int width;
	int height;
	int flags;
	RLGR_MODE mode;
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

RFX_MESSAGE *
rfx_process_message(RFX_CONTEXT * context, unsigned char * data, int data_size)
{
	RFX_MESSAGE * message;
	unsigned int blockType;
	unsigned int blockLen;

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

