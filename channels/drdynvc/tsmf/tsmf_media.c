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
#include <freerdp/constants_ui.h>
#include "drdynvc_types.h"
#include "tsmf_constants.h"
#include "tsmf_types.h"
#include "tsmf_decoder.h"
#include "tsmf_audio.h"
#include "tsmf_main.h"
#include "tsmf_codec.h"
#include "tsmf_media.h"

struct _TSMF_PRESENTATION
{
	uint8 presentation_id[GUID_SIZE];

	const char * audio_name;
	const char * audio_device;
	int eos;

	uint32 last_x;
	uint32 last_y;
	uint32 last_width;
	uint32 last_height;
	uint16 last_num_rects;
	RD_RECT * last_rects;

	uint32 output_x;
	uint32 output_y;
	uint32 output_width;
	uint32 output_height;
	uint16 output_num_rects;
	RD_RECT * output_rects;

	IWTSVirtualChannelCallback * channel_callback;

	/* The stream list could be accessed by differnt threads and need to be protected. */
	pthread_mutex_t * mutex;

	TSMF_STREAM * stream_list_head;
	TSMF_STREAM * stream_list_tail;

	TSMF_PRESENTATION * next;
	TSMF_PRESENTATION * prev;
};

struct _TSMF_STREAM
{
	uint32 stream_id;

	TSMF_PRESENTATION * presentation;

	ITSMFDecoder * decoder;

	int major_type;
	int eos;
	uint32 width;
	uint32 height;

	ITSMFAudioDevice * audio;
	uint32 sample_rate;
	uint32 channels;
	uint32 bits_per_sample;

	/* The end_time of last played sample */
	uint64 last_end_time;

	/* The samples will be accessed by producer/consumer running in different
	   threads. So we use mutex to protect it at stream layer. */
	pthread_mutex_t * mutex;

	int thread_status;
	int thread_exit;

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
	uint32 extensions;
	uint32 data_size;
	uint8 * data;
	uint32 decoded_size;
	uint32 pixfmt;

	TSMF_STREAM * stream;
	IWTSVirtualChannelCallback * channel_callback;

	TSMF_SAMPLE * next;
};

static TSMF_PRESENTATION * presentation_list_head = NULL;
static TSMF_PRESENTATION * presentation_list_tail = NULL;

static TSMF_SAMPLE *
tsmf_stream_pop_sample(TSMF_STREAM * stream)
{
	TSMF_PRESENTATION * presentation = stream->presentation;
	TSMF_SAMPLE * sample;
	TSMF_STREAM * s;
	int pending = 0;

	/* Check if some other stream has earlier sample that needs to be played first */
	if (stream->last_end_time > 1000000) /* 100ms */
	{
		pthread_mutex_lock(presentation->mutex);
		for (s = presentation->stream_list_head; s; s = s->next)
		{
			if (s != stream && !s->eos && s->last_end_time &&
				s->last_end_time < stream->last_end_time - 1000000)
			{
					pending = 1;
					break;
			}
		}
		pthread_mutex_unlock(presentation->mutex);
		if (pending)
			return NULL;
	}

	pthread_mutex_lock(stream->mutex);

	sample = stream->sample_queue_head;
	if (sample)
	{
		stream->sample_queue_head = sample->next;
		sample->next = NULL;
		if (stream->sample_queue_head == NULL)
			stream->sample_queue_tail = NULL;
	}

	pthread_mutex_unlock(stream->mutex);

	if (sample)
		stream->last_end_time = sample->end_time;

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
	if (sample->data)
		free(sample->data);
	free(sample);
}

TSMF_PRESENTATION *
tsmf_presentation_new(const uint8 * guid, IWTSVirtualChannelCallback * pChannelCallback)
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
	presentation->channel_callback = pChannelCallback;

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

static void
tsmf_presentation_restore_last_video_frame(TSMF_PRESENTATION * presentation)
{
	RD_REDRAW_EVENT * revent;

	if (presentation->last_width && presentation->last_height)
	{
		revent = (RD_REDRAW_EVENT *) malloc(sizeof(RD_REDRAW_EVENT));
		memset(revent, 0, sizeof(RD_REDRAW_EVENT));
		revent->event.event_type = RD_EVENT_TYPE_REDRAW;
		revent->event.event_callback = (RD_EVENT_CALLBACK) free;
		revent->x = presentation->last_x;
		revent->y = presentation->last_y;
		revent->width = presentation->last_width;
		revent->height = presentation->last_height;
		if (tsmf_push_event(presentation->channel_callback, (RD_EVENT *) revent) != 0)
		{
			free(revent);
		}
		presentation->last_x = 0;
		presentation->last_y = 0;
		presentation->last_width = 0;
		presentation->last_height = 0;
	}
}

static void
tsmf_free_video_frame_event(RD_EVENT * event)
{
	RD_VIDEO_FRAME_EVENT * vevent = (RD_VIDEO_FRAME_EVENT *) event;
	LLOGLN(10, ("tsmf_free_video_frame_event:"));
	if (vevent->frame_data)
		free(vevent->frame_data);
	if (vevent->visible_rects)
		free(vevent->visible_rects);
	free(vevent);
}

static void
tsmf_sample_playback_video(TSMF_SAMPLE * sample)
{
	TSMF_PRESENTATION * presentation = sample->stream->presentation;
	RD_VIDEO_FRAME_EVENT * vevent;

	LLOGLN(10, ("tsmf_sample_playback_video: MessageId %d EndTime %d data_size %d consumed.",
		sample->sample_id, (int)sample->end_time, sample->data_size));

	if (sample->data)
	{
		if (presentation->last_x != presentation->output_x ||
			presentation->last_y != presentation->output_y ||
			presentation->last_width != presentation->output_width ||
			presentation->last_height != presentation->output_height ||
			presentation->last_num_rects != presentation->output_num_rects ||
			(presentation->last_rects && presentation->output_rects &&
			memcmp(presentation->last_rects, presentation->output_rects,
			presentation->last_num_rects * sizeof(RD_RECT)) != 0))
		{
			tsmf_presentation_restore_last_video_frame(presentation);

			presentation->last_x = presentation->output_x;
			presentation->last_y = presentation->output_y;
			presentation->last_width = presentation->output_width;
			presentation->last_height = presentation->output_height;

			if (presentation->last_rects)
			{
				free(presentation->last_rects);
				presentation->last_rects = NULL;
			}
			presentation->last_num_rects = presentation->output_num_rects;
			if (presentation->last_num_rects > 0)
			{
				presentation->last_rects = malloc(presentation->last_num_rects * sizeof(RD_RECT));
				memcpy(presentation->last_rects, presentation->output_rects,
					presentation->last_num_rects * sizeof(RD_RECT));
			}
		}

		vevent = (RD_VIDEO_FRAME_EVENT *) malloc(sizeof(RD_VIDEO_FRAME_EVENT));
		memset(vevent, 0, sizeof(RD_VIDEO_FRAME_EVENT));
		vevent->event.event_type = RD_EVENT_TYPE_VIDEO_FRAME;
		vevent->event.event_callback = tsmf_free_video_frame_event;
		vevent->frame_data = sample->data;
		vevent->frame_size = sample->decoded_size;
		vevent->frame_pixfmt = sample->pixfmt;
		vevent->frame_width = sample->stream->width;
		vevent->frame_height = sample->stream->height;
		vevent->x = presentation->output_x;
		vevent->y = presentation->output_y;
		vevent->width = presentation->output_width;
		vevent->height = presentation->output_height;
		if (presentation->output_num_rects > 0)
		{
			vevent->num_visible_rects = presentation->output_num_rects;
			vevent->visible_rects = (RD_RECT *) malloc(presentation->output_num_rects * sizeof(RD_RECT));
			memcpy(vevent->visible_rects, presentation->output_rects,
				presentation->output_num_rects * sizeof(RD_RECT));
		}

		/* The frame data ownership is passed to the event object, and is freed after the event is processed. */
		sample->data = NULL;
		sample->decoded_size = 0;

		if (tsmf_push_event(sample->channel_callback, (RD_EVENT *) vevent) != 0)
		{
			tsmf_free_video_frame_event((RD_EVENT *) vevent);
		}

#if 0
		/* Dump a .ppm image for every 30 frames. Assuming the frame is in YUV format, we
		   extract the Y values to create a grayscale image. */
		static int frame_id = 0;
		char buf[100];
		FILE * fp;
		if ((frame_id % 30) == 0)
		{
			snprintf(buf, sizeof(buf), "/tmp/FreeRDP_Frame_%d.ppm", frame_id);
			fp = fopen(buf, "wb");
			fwrite("P5\n", 1, 3, fp);
			snprintf(buf, sizeof(buf), "%d %d\n", sample->stream->width, sample->stream->height);
			fwrite(buf, 1, strlen(buf), fp);
			fwrite("255\n", 1, 4, fp);
			fwrite(sample->data, 1, sample->stream->width * sample->stream->height, fp);
			fflush(fp);
			fclose(fp);
		}
		frame_id++;
#endif
	}
}

static void
tsmf_sample_playback_audio(TSMF_SAMPLE * sample)
{
	LLOGLN(10, ("tsmf_presentation_playback_audio_sample: MessageId %d EndTime %d consumed.",
		sample->sample_id, (int)sample->end_time));

	if (sample->stream->audio && sample->data)
	{
		sample->stream->audio->Play(sample->stream->audio,
			sample->data, sample->decoded_size);
		sample->data = NULL;
		sample->decoded_size = 0;
	}
}

static void
tsmf_sample_playback(TSMF_SAMPLE * sample)
{
	TSMF_STREAM * stream = sample->stream;
	int ret = 1;
	uint32 width;
	uint32 height;
	uint32 pixfmt = 0;

	if (stream->decoder)
		ret = stream->decoder->Decode(stream->decoder, sample->data, sample->data_size, sample->extensions);
	if (ret)
		return;

	free(sample->data);
	sample->data = NULL;

	if (stream->major_type == TSMF_MAJOR_TYPE_VIDEO)
	{
		if (stream->decoder->GetDecodedFormat)
		{
			pixfmt = stream->decoder->GetDecodedFormat(stream->decoder);
			if (pixfmt == ((uint32) -1))
				return;
			sample->pixfmt = pixfmt;
		}

		if (stream->decoder->GetDecodedDimension)
			ret = stream->decoder->GetDecodedDimension(stream->decoder, &width, &height);
		if (ret == 0 && (width != stream->width || height != stream->height))
		{
			LLOGLN(0, ("tsmf_sample_playback: video dimension changed to %d x %d", width, height));
			stream->width = width;
			stream->height = height;
		}
	}

	if (stream->decoder->GetDecodedData)
	{
		sample->data = stream->decoder->GetDecodedData(stream->decoder, &sample->decoded_size);
	}

	switch (sample->stream->major_type)
	{
		case TSMF_MAJOR_TYPE_VIDEO:
			tsmf_sample_playback_video(sample);
			break;
		case TSMF_MAJOR_TYPE_AUDIO:
			tsmf_sample_playback_audio(sample);
			break;
	}

	tsmf_sample_ack(sample);
	tsmf_sample_free(sample);
}

static void *
tsmf_stream_playback_func(void * arg)
{
	TSMF_STREAM * stream = (TSMF_STREAM *) arg;
	TSMF_PRESENTATION * presentation = stream->presentation;
	TSMF_SAMPLE * sample;

	LLOGLN(10, ("tsmf_stream_playback_func: in %d", stream->stream_id));
	if (stream->major_type == TSMF_MAJOR_TYPE_AUDIO &&
		stream->sample_rate && stream->channels && stream->bits_per_sample)
	{
		stream->audio = tsmf_load_audio_device(
			presentation->audio_name && presentation->audio_name[0] ? presentation->audio_name : NULL,
			presentation->audio_device && presentation->audio_device[0] ? presentation->audio_device : NULL);
		if (stream->audio)
		{
			stream->audio->SetFormat(stream->audio,
				stream->sample_rate, stream->channels, stream->bits_per_sample);
		}
	}
	while (!stream->thread_exit)
	{
		sample = tsmf_stream_pop_sample(stream);
		if (sample)
			tsmf_sample_playback(sample);
		else
			usleep(10000);
	}
	if (stream->eos || presentation->eos)
	{
		while ((sample = tsmf_stream_pop_sample(stream)) != NULL)
			tsmf_sample_playback(sample);
	}
	if (stream->audio)
	{
		stream->audio->Free(stream->audio);
		stream->audio = NULL;
	}
	LLOGLN(10, ("tsmf_stream_playback_func: out %d", stream->stream_id));
	stream->thread_status = 0;
	return NULL;
}

static void
tsmf_stream_start(TSMF_STREAM * stream)
{
	pthread_t thread;

	if (stream->thread_status == 0)
	{
		stream->thread_status = 1;
		stream->thread_exit = 0;
		pthread_create(&thread, 0, tsmf_stream_playback_func, stream);
		pthread_detach(thread);
	}
}

static void
tsmf_stream_stop(TSMF_STREAM * stream)
{
	stream->thread_exit = 1;
	while (stream->thread_status > 0)
	{
		usleep(250 * 1000);
	}
}

void
tsmf_presentation_start(TSMF_PRESENTATION * presentation)
{
	TSMF_STREAM * stream;

	for (stream = presentation->stream_list_head; stream; stream = stream->next)
		tsmf_stream_start(stream);
}

void
tsmf_presentation_stop(TSMF_PRESENTATION * presentation)
{
	TSMF_STREAM * stream;

	for (stream = presentation->stream_list_head; stream; stream = stream->next)
		tsmf_stream_stop(stream);

	tsmf_presentation_restore_last_video_frame(presentation);
	if (presentation->last_rects)
	{
		free(presentation->last_rects);
		presentation->last_rects = NULL;
	}
	presentation->last_num_rects = 0;
	if (presentation->output_rects)
	{
		free(presentation->output_rects);
		presentation->output_rects = NULL;
	}
	presentation->output_num_rects = 0;
}

void
tsmf_presentation_set_geometry_info(TSMF_PRESENTATION * presentation,
	uint32 x, uint32 y, uint32 width, uint32 height,
	int num_rects, RD_RECT * rects)
{
	presentation->output_x = x;
	presentation->output_y = y;
	presentation->output_width = width;
	presentation->output_height = height;
	if (presentation->output_rects)
		free(presentation->output_rects);
	presentation->output_rects = rects;
	presentation->output_num_rects = num_rects;
}

void
tsmf_presentation_set_audio_device(TSMF_PRESENTATION * presentation, const char * name, const char * device)
{
	presentation->audio_name = name;
	presentation->audio_device = device;
}

static void
tsmf_stream_flush(TSMF_STREAM * stream)
{
	TSMF_SAMPLE * sample;

	while ((sample = tsmf_stream_pop_sample(stream)) != NULL)
		tsmf_sample_free(sample);

	if (stream->audio)
		stream->audio->Flush(stream->audio);

	stream->eos = 0;
	stream->last_end_time = 0;
}

void
tsmf_presentation_flush(TSMF_PRESENTATION * presentation)
{
	TSMF_STREAM * stream;

	for (stream = presentation->stream_list_head; stream; stream = stream->next)
		tsmf_stream_flush(stream);

	presentation->eos = 0;
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
	stream->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(stream->mutex, 0);

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
tsmf_stream_set_format(TSMF_STREAM * stream, const char * name, const uint8 * pMediaType)
{
	TS_AM_MEDIA_TYPE mediatype;

	if (stream->decoder)
	{
		LLOGLN(0, ("tsmf_stream_set_format: duplicated call"));
		return;
	}

	tsmf_codec_parse_media_type(&mediatype, pMediaType);

	if (mediatype.MajorType == TSMF_MAJOR_TYPE_VIDEO)
	{
		LLOGLN(0, ("tsmf_stream_set_format: video width %d height %d bit_rate %d frame_rate %f codec_data %d",
			mediatype.Width, mediatype.Height, mediatype.BitRate,
			(double)mediatype.SamplesPerSecond.Numerator / (double)mediatype.SamplesPerSecond.Denominator,
			mediatype.ExtraDataSize));
	}
	else if (mediatype.MajorType == TSMF_MAJOR_TYPE_AUDIO)
	{
		LLOGLN(0, ("tsmf_stream_set_format: audio channel %d sample_rate %d bits_per_sample %d codec_data %d",
			mediatype.Channels, mediatype.SamplesPerSecond.Numerator, mediatype.BitsPerSample,
			mediatype.ExtraDataSize));
		stream->sample_rate = mediatype.SamplesPerSecond.Numerator;
		stream->channels = mediatype.Channels;
		stream->bits_per_sample = mediatype.BitsPerSample;
		if (stream->bits_per_sample == 0)
			stream->bits_per_sample = 16;
	}

	stream->major_type = mediatype.MajorType;
	stream->width = mediatype.Width;
	stream->height = mediatype.Height;
	stream->decoder = tsmf_load_decoder(name, &mediatype);
}

void
tsmf_stream_end(TSMF_STREAM * stream)
{
	stream->eos = 1;
	stream->presentation->eos = 1;
}

void
tsmf_stream_free(TSMF_STREAM * stream)
{
	TSMF_PRESENTATION * presentation = stream->presentation;

	tsmf_stream_stop(stream);
	tsmf_stream_flush(stream);

	pthread_mutex_lock(presentation->mutex);

	if (presentation->stream_list_head == stream)
		presentation->stream_list_head = stream->next;
	else
		stream->prev->next = stream->next;

	if (presentation->stream_list_tail == stream)
		presentation->stream_list_tail = stream->prev;
	else
		stream->next->prev = stream->prev;

	pthread_mutex_unlock(presentation->mutex);

	if (stream->decoder)
		stream->decoder->Free(stream->decoder);

	pthread_mutex_destroy(stream->mutex);
	free(stream->mutex);

	free(stream);
}

void
tsmf_stream_push_sample(TSMF_STREAM * stream, IWTSVirtualChannelCallback * pChannelCallback,
	uint32 sample_id, uint64 start_time, uint64 end_time, uint64 duration, uint32 extensions,
	uint32 data_size, uint8 * data)
{
	TSMF_SAMPLE * sample;

	sample = (TSMF_SAMPLE *) malloc(sizeof(TSMF_SAMPLE));
	memset(sample, 0, sizeof(TSMF_SAMPLE));

	sample->sample_id = sample_id;
	sample->start_time = start_time;
	sample->end_time = end_time;
	sample->duration = duration;
	sample->extensions = extensions;
	sample->stream = stream;
	sample->channel_callback = pChannelCallback;
	sample->data_size = data_size;
	sample->data = malloc(data_size + TSMF_BUFFER_PADDING_SIZE);
	memcpy(sample->data, data, data_size);
	memset(sample->data + data_size, 0, TSMF_BUFFER_PADDING_SIZE);

	pthread_mutex_lock(stream->mutex);

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

	pthread_mutex_unlock(stream->mutex);
}

