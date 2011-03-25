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
#include <pthread.h>
#include <unistd.h>
#include "drdynvc_types.h"
#include "tsmf_main.h"
#include "tsmf_media.h"

#define GUID_SIZE 16

struct _TSMF_PRESENTATION
{
	uint8 presentation_id[GUID_SIZE];

	/* The streams and samples will be accessed by producer/consumer running
	   in different threads. So we use mutex to protect it at presentation
	   layer. */
	pthread_mutex_t * mutex;

	int thread_status;
	int thread_exit;

	uint64 playback_time;

	TSMF_STREAM * stream_list_head;
	TSMF_STREAM * stream_list_tail;

	TSMF_PRESENTATION * next;
	TSMF_PRESENTATION * prev;
};

struct _TSMF_STREAM
{
	uint32 stream_id;

	TSMF_PRESENTATION * presentation;

	int eos;

	TSMF_SAMPLE * sample_queue_head;
	TSMF_SAMPLE * sample_queue_tail;

	TSMF_STREAM * next;
	TSMF_STREAM * prev;
};

struct _TSMF_SAMPLE
{
	uint32 sample_id;
	uint64 start_time;
	uint64 end_time;
	uint64 duration;
	uint32 data_size;
	uint8 * data;

	TSMF_STREAM * stream;
	IWTSVirtualChannelCallback * channel_callback;

	TSMF_SAMPLE * next;
};

static TSMF_PRESENTATION * presentation_list_head = NULL;
static TSMF_PRESENTATION * presentation_list_tail = NULL;

static TSMF_SAMPLE *
tsmf_stream_pop_sample(TSMF_STREAM * stream)
{
	TSMF_SAMPLE * sample;

	sample = stream->sample_queue_head;
	if (sample)
	{
		stream->sample_queue_head = sample->next;
		sample->next = NULL;
		if (stream->sample_queue_head == NULL)
			stream->sample_queue_tail = NULL;
	}

	return sample;
}

static void
tsmf_sample_ack(TSMF_SAMPLE * sample)
{
	tsmf_playback_ack(sample->channel_callback, sample->sample_id, sample->duration, sample->data_size);
}

static void
tsmf_sample_free(TSMF_SAMPLE * sample)
{
	free(sample->data);
	free(sample);
}

/* Pop a sample from the stream with smallest end_time */
static TSMF_SAMPLE *
tsmf_presentation_pop_sample(TSMF_PRESENTATION * presentation)
{
	TSMF_STREAM * stream;
	TSMF_STREAM * earliest_stream = NULL;
	TSMF_SAMPLE * sample = NULL;
	int has_pending_stream = 0;

	pthread_mutex_lock(presentation->mutex);

	for (stream = presentation->stream_list_head; stream; stream = stream->next)
	{
		if (!stream->sample_queue_head && !stream->eos)
			has_pending_stream = 1;
		if (stream->sample_queue_head && (!earliest_stream ||
			earliest_stream->sample_queue_head->end_time > stream->sample_queue_head->end_time))
		{
			earliest_stream = stream;
		}
	}
	/* Ensure multiple streams are interleaved.
	   1. If all streams has samples available, we just consume the earliest one
	   2. If the earliest sample with end_time <= current playback time, we consume it
	   3. If the earliest sample with end_time > current playback time, we check if
	      there's a stream pending for receiving sample. If so, we bypasss it and wait.
	*/
	if (earliest_stream && (!has_pending_stream || presentation->playback_time == 0 ||
		presentation->playback_time >= earliest_stream->sample_queue_head->start_time))
	{
		sample = tsmf_stream_pop_sample(earliest_stream);
		presentation->playback_time = sample->end_time;
	}
	pthread_mutex_unlock(presentation->mutex);

	if (sample)
	{
		tsmf_sample_ack(sample);
	}

	return sample;
}

TSMF_PRESENTATION *
tsmf_presentation_new(const uint8 * guid)
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
	presentation->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(presentation->mutex, 0);

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
tsmf_presentation_find_by_id(const uint8 * guid)
{
	TSMF_PRESENTATION * presentation;

	for (presentation = presentation_list_head; presentation; presentation = presentation->next)
	{
		if (memcmp(presentation->presentation_id, guid, GUID_SIZE) == 0)
			return presentation;
	}
	return NULL;
}

static void *
tsmf_presentation_playback_func(void * arg)
{
	TSMF_PRESENTATION * presentation = (TSMF_PRESENTATION *) arg;
	TSMF_SAMPLE * sample;

	LLOGLN(0, ("tsmf_presentation_playback_func: in"));
	while (!presentation->thread_exit)
	{
		sample = tsmf_presentation_pop_sample(presentation);
		if (sample)
		{
			LLOGLN(0, ("tsmf_presentation_playback_func: MessageId %d EndTime %d consumed.",
				sample->sample_id, (int)sample->end_time));
			tsmf_sample_free(sample);
		}
		else
		{
			usleep(10000);
		}
	}
	LLOGLN(0, ("tsmf_presentation_playback_func: out"));
	presentation->thread_status = 0;
	return NULL;
}

void
tsmf_presentation_start(TSMF_PRESENTATION * presentation)
{
	pthread_t thread;

	if (presentation->thread_status == 0)
	{
		presentation->thread_status = 1;
		presentation->thread_exit = 0;
		presentation->playback_time = 0;
		pthread_create(&thread, 0, tsmf_presentation_playback_func, presentation);
		pthread_detach(thread);
	}
}

void
tsmf_presentation_stop(TSMF_PRESENTATION * presentation)
{
	presentation->thread_exit = 1;
	while (presentation->thread_status > 0)
	{
		usleep(250 * 1000);
	}
}

void
tsmf_presentation_free(TSMF_PRESENTATION * presentation)
{
	tsmf_presentation_stop(presentation);

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

	pthread_mutex_destroy(presentation->mutex);
	free(presentation->mutex);

	free(presentation);
}

TSMF_STREAM *
tsmf_stream_new(TSMF_PRESENTATION * presentation, uint32 stream_id)
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

	pthread_mutex_lock(presentation->mutex);

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

	pthread_mutex_unlock(presentation->mutex);

	return stream;
}

TSMF_STREAM *
tsmf_stream_find_by_id(TSMF_PRESENTATION * presentation, uint32 stream_id)
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
tsmf_stream_flush(TSMF_STREAM * stream)
{
	TSMF_PRESENTATION * presentation = stream->presentation;
	TSMF_SAMPLE * sample;

	pthread_mutex_lock(presentation->mutex);
	while (stream->sample_queue_head)
	{
		sample = tsmf_stream_pop_sample(stream);
		tsmf_sample_free(sample);
	}
	pthread_mutex_unlock(presentation->mutex);
}

void
tsmf_stream_end(TSMF_STREAM * stream)
{
	stream->eos = 1;
}

void
tsmf_stream_free(TSMF_STREAM * stream)
{
	TSMF_PRESENTATION * presentation = stream->presentation;
	TSMF_SAMPLE * sample;

	pthread_mutex_lock(presentation->mutex);

	while (stream->sample_queue_head)
	{
		sample = tsmf_stream_pop_sample(stream);
		tsmf_sample_free(sample);
	}

	if (presentation->stream_list_head == stream)
		presentation->stream_list_head = stream->next;
	else
		stream->prev->next = stream->next;

	if (presentation->stream_list_tail == stream)
		presentation->stream_list_tail = stream->prev;
	else
		stream->next->prev = stream->prev;

	pthread_mutex_unlock(presentation->mutex);

	free(stream);
}

void
tsmf_stream_push_sample(TSMF_STREAM * stream, IWTSVirtualChannelCallback * pChannelCallback,
	uint32 sample_id, uint64 start_time, uint64 end_time, uint64 duration, uint32 data_size, uint8 * data)
{
	TSMF_PRESENTATION * presentation = stream->presentation;
	TSMF_SAMPLE * sample;

	sample = (TSMF_SAMPLE *) malloc(sizeof(TSMF_SAMPLE));
	memset(sample, 0, sizeof(TSMF_SAMPLE));

	sample->sample_id = sample_id;
	sample->start_time = start_time;
	sample->end_time = end_time;
	sample->duration = duration;
	sample->data_size = data_size;
	sample->data = malloc(data_size);
	memcpy(sample->data, data, data_size);

	sample->stream = stream;
	sample->channel_callback = pChannelCallback;

	pthread_mutex_lock(presentation->mutex);

	if (stream->sample_queue_tail == NULL)
	{
		stream->sample_queue_head = sample;
		stream->sample_queue_tail = sample;
	}
	else
	{
		stream->sample_queue_tail->next = sample;
		stream->sample_queue_tail = sample;
	}	

	pthread_mutex_unlock(presentation->mutex);
}

