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
#include "rfx_decode.h"
#include "rfx.h"

struct _RFX_CONTEXT
{
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
		printf("rfx_process_message: blockType 0x%X blockLen %d\n", blockType, blockLen);

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

