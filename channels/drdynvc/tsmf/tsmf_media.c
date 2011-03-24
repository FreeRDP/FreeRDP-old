/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel - Media Container

   Copyright 2010-2011 Vic Lee

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
#include "drdynvc_types.h"
#include "tsmf_main.h"
#include "tsmf_media.h"

#define GUID_SIZE 16

struct _TSMF_PRESENTATION
{
	uint8 presentation_id[GUID_SIZE];

	TSMF_STREAM * stream_list_head;
	TSMF_STREAM * stream_list_tail;

	TSMF_PRESENTATION * next;
	TSMF_PRESENTATION * prev;
};

struct _TSMF_STREAM
{
	uint32 stream_id;

	TSMF_PRESENTATION * presentation;

	TSMF_SAMPLE * sample_queue_head;
	TSMF_SAMPLE * sample_queue_tail;

	TSMF_STREAM * next;
	TSMF_STREAM * prev;
};

struct _TSMF_SAMPLE
{
	uint32 sample_id;
	uint64 end_time;
	uint64 duration;
	uint32 data_size;
	uint8 * data;

	TSMF_SAMPLE * next;
	TSMF_SAMPLE * prev;
};

static TSMF_PRESENTATION * presentation_list_head = NULL;
static TSMF_PRESENTATION * presentation_list_tail = NULL;

TSMF_PRESENTATION *
tsmf_presentation_new (const uint8 * guid)
{
	TSMF_PRESENTATION * presentation;

	presentation = tsmf_presentation_find_by_id(guid);
	if (presentation)
	{
		printf("tsmf_presentation_new: duplicated presentation id!\n");
		return NULL;
	}

	presentation = malloc(sizeof(TSMF_PRESENTATION));
	memset(presentation, 0, sizeof(TSMF_PRESENTATION));

	memcpy(presentation->presentation_id, guid, GUID_SIZE);

	if (presentation_list_tail == NULL)
	{
		presentation_list_head = presentation;
		presentation_list_tail = presentation;
	}
	else
	{
		presentation->prev = presentation_list_tail;
		presentation_list_tail->next = presentation;
		presentation_list_tail = presentation;
	}

	return presentation;
}

TSMF_PRESENTATION *
tsmf_presentation_find_by_id (const uint8 * guid)
{
	TSMF_PRESENTATION * presentation;

	for (presentation = presentation_list_head; presentation; presentation = presentation->next)
	{
		if (memcmp(presentation->presentation_id, guid, GUID_SIZE) == 0)
			return presentation;
	}
	return NULL;
}

void
tsmf_presentation_free (TSMF_PRESENTATION * presentation)
{
	if (presentation_list_head == presentation)
		presentation_list_head = presentation->next;
	else
		presentation->prev->next = presentation->next;

	if (presentation_list_tail == presentation)
		presentation_list_tail = presentation->prev;
	else
		presentation->next->prev = presentation->prev;

	while (presentation->stream_list_head)
		tsmf_stream_free(presentation->stream_list_head);

	free(presentation);
}

TSMF_STREAM *
tsmf_stream_new (TSMF_PRESENTATION * presentation, uint32 stream_id)
{
	TSMF_STREAM * stream;

	stream = tsmf_stream_find_by_id(presentation, stream_id);
	if (stream)
	{
		printf("tsmf_stream_new: duplicated stream id %d!\n", stream_id);
		return NULL;
	}

	stream = malloc(sizeof(TSMF_STREAM));
	memset(stream, 0, sizeof(TSMF_STREAM));

	stream->stream_id = stream_id;
	stream->presentation = presentation;

	if (presentation->stream_list_tail == NULL)
	{
		presentation->stream_list_head = stream;
		presentation->stream_list_tail = stream;
	}
	else
	{
		stream->prev = presentation->stream_list_tail;
		presentation->stream_list_tail->next = stream;
		presentation->stream_list_tail = stream;
	}

	return stream;
}

TSMF_STREAM *
tsmf_stream_find_by_id (TSMF_PRESENTATION * presentation, uint32 stream_id)
{
	TSMF_STREAM * stream;

	for (stream = presentation->stream_list_head; stream; stream = stream->next)
	{
		if (stream->stream_id == stream_id)
			return stream;
	}
	return NULL;
}

void
tsmf_stream_free (TSMF_STREAM * stream)
{
	TSMF_PRESENTATION * presentation = stream->presentation;

	if (presentation->stream_list_head == stream)
		presentation->stream_list_head = stream->next;
	else
		stream->prev->next = stream->next;

	if (presentation->stream_list_tail == stream)
		presentation->stream_list_tail = stream->prev;
	else
		stream->next->prev = stream->prev;

	free(stream);
}

void
tsmf_stream_push_sample(TSMF_STREAM * stream, IWTSVirtualChannelCallback * pChannelCallback,
	uint64 sample_id, uint64 end_time, uint64 duration, uint32 size, uint8 * data)
{
	tsmf_playback_ack(pChannelCallback, sample_id, duration, size);
}

