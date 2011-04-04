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
#include "tsmf_constants.h"
#include "tsmf_types.h"
#include "tsmf_decoder.h"
#include "tsmf_audio.h"
#include "tsmf_main.h"
#include "tsmf_media.h"

#define GUID_SIZE 16

typedef struct _TSMFMediaTypeMap
{
	uint8 guid[16];
	const char * name;
	int type;
} TSMFMediaTypeMap;

static const TSMFMediaTypeMap tsmf_major_type_map[] =
{
	/* 73646976-0000-0010-8000-00AA00389B71 */
	{
		{ 0x76, 0x69, 0x64, 0x73, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"MEDIATYPE_Video",
		TSMF_MAJOR_TYPE_VIDEO
	},

	/* 73647561-0000-0010-8000-00AA00389B71 */
	{
		{ 0x61, 0x75, 0x64, 0x73, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"MEDIATYPE_Audio",
		TSMF_MAJOR_TYPE_AUDIO
	},

	{
		{ 0 },
		"Unknown",
		TSMF_MAJOR_TYPE_UNKNOWN
	}
};

static const TSMFMediaTypeMap tsmf_sub_type_map[] =
{
	/* 31435657-0000-0010-8000-00AA00389B71 */
	{
		{ 0x57, 0x56, 0x43, 0x31, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"MEDIASUBTYPE_WVC1",
		TSMF_SUB_TYPE_WVC1
	},

	/* 00000161-0000-0010-8000-00AA00389B71 */
	{
		{ 0x61, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"WMMEDIASUBTYPE_WMAudioV2", /* V7, V8 has the same GUID */
		TSMF_SUB_TYPE_WMA2
	},

	/* 00000162-0000-0010-8000-00AA00389B71 */
	{
		{ 0x62, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"WMMEDIASUBTYPE_WMAudioV9",
		TSMF_SUB_TYPE_WMA9
	},

	{
		{ 0 },
		"Unknown",
		TSMF_SUB_TYPE_UNKNOWN
	}

};

static const TSMFMediaTypeMap tsmf_format_type_map[] =
{
	/* AED4AB2D-7326-43CB-9464-C879CAB9C43D */
	{
		{ 0x2D, 0xAB, 0xD4, 0xAE, 0x26, 0x73, 0xCB, 0x43, 0x94, 0x64, 0xC8, 0x79, 0xCA, 0xB9, 0xC4, 0x3D },
		"FORMAT_MFVideoFormat",
		TSMF_FORMAT_TYPE_MFVIDEOFORMAT
	},

	/* 05589F81-C356-11CE-BF01-00AA0055595A */
	{
		{ 0x81, 0x9F, 0x58, 0x05, 0x56, 0xC3, 0xCE, 0x11, 0xBF, 0x01, 0x00, 0xAA, 0x00, 0x55, 0x59, 0x5A },
		"FORMAT_WaveFormatEx",
		TSMF_FORMAT_TYPE_WAVEFORMATEX
	},

	{
		{ 0 },
		"Default",
		TSMF_FORMAT_TYPE_DEFAULT
	}
};

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

	ITSMFAudioDevice * audio;
	const char * audio_name;
	const char * audio_device;
	uint32 sample_rate;
	uint32 channels;
	uint32 bits_per_sample;

	TSMF_STREAM * stream_list_head;
	TSMF_STREAM * stream_list_tail;

	TSMF_PRESENTATION * next;
	TSMF_PRESENTATION * prev;

	uint32 output_width;
	uint32 output_height;
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

static void
tsmf_print_guid(const uint8 * guid)
{
	int i;

	for (i = 3; i >= 0; i--)
		LLOG(0, ("%02X", guid[i]));
	LLOG(0, ("-"));
	for (i = 5; i >= 4; i--)
		LLOG(0, ("%02X", guid[i]));
	LLOG(0, ("-"));
	for (i = 7; i >= 6; i--)
		LLOG(0, ("%02X", guid[i]));
	LLOG(0, ("-"));
	for (i = 8; i < 16; i++)
	{
		LLOG(0, ("%02X", guid[i]));
		if (i == 9)
			LLOG(0, ("-"));
	}
}

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
	if (sample->data)
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

static void
tsmf_sample_playback_video(TSMF_SAMPLE * sample)
{
	LLOGLN(10, ("tsmf_presentation_playback_video_sample: MessageId %d EndTime %d data_size %d consumed.",
		sample->sample_id, (int)sample->end_time, sample->data_size));

#if 0
	if (sample->data)
	{
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
	}
#endif
}

static void
tsmf_sample_playback_audio(TSMF_SAMPLE * sample)
{
	LLOGLN(10, ("tsmf_presentation_playback_audio_sample: MessageId %d EndTime %d consumed.",
		sample->sample_id, (int)sample->end_time));

	if (sample->stream->presentation->audio && sample->data)
	{
		sample->stream->presentation->audio->Play(sample->stream->presentation->audio,
			sample->data, sample->data_size);
		sample->data = NULL;
		sample->data_size = 0;
	}
}

static void
tsmf_sample_playback(TSMF_SAMPLE * sample)
{
	switch (sample->stream->major_type)
	{
		case TSMF_MAJOR_TYPE_VIDEO:
			tsmf_sample_playback_video(sample);
			break;
		case TSMF_MAJOR_TYPE_AUDIO:
			tsmf_sample_playback_audio(sample);
			break;
	}
}

static void *
tsmf_presentation_playback_func(void * arg)
{
	TSMF_PRESENTATION * presentation = (TSMF_PRESENTATION *) arg;
	TSMF_SAMPLE * sample;

	LLOGLN(0, ("tsmf_presentation_playback_func: in"));
	if (presentation->sample_rate && presentation->channels && presentation->bits_per_sample)
	{
		presentation->audio = tsmf_load_audio_device(
			presentation->audio_name && presentation->audio_name[0] ? presentation->audio_name : NULL,
			presentation->audio_device && presentation->audio_device[0] ? presentation->audio_device : NULL);
		if (presentation->audio)
		{
			presentation->audio->SetFormat(presentation->audio,
				presentation->sample_rate, presentation->channels, presentation->bits_per_sample);
		}
	}
	while (!presentation->thread_exit)
	{
		sample = tsmf_presentation_pop_sample(presentation);
		if (sample)
		{
			tsmf_sample_playback(sample);
			tsmf_sample_free(sample);
		}
		else
		{
			usleep(10000);
		}
	}
	if (presentation->audio)
	{
		presentation->audio->Free(presentation->audio);
		presentation->audio = NULL;
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
tsmf_presentation_set_size(TSMF_PRESENTATION * presentation, uint32 width, uint32 height)
{
	presentation->output_width = width;
	presentation->output_height = height;
}

void
tsmf_presentation_set_audio_device(TSMF_PRESENTATION * presentation, const char * name, const char * device)
{
	presentation->audio_name = name;
	presentation->audio_device = device;
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
tsmf_stream_set_format(TSMF_STREAM * stream, const char * name, const uint8 * pMediaType)
{
	TS_AM_MEDIA_TYPE mediatype;
	uint32 cbFormat;
	int i;

	memset(&mediatype, 0, sizeof(TS_AM_MEDIA_TYPE));

	LLOG(0, ("MajorType:  "));
	tsmf_print_guid(pMediaType);
	for (i = 0; tsmf_major_type_map[i].type != TSMF_MAJOR_TYPE_UNKNOWN; i++)
	{
		if (memcmp(tsmf_major_type_map[i].guid, pMediaType, 16) == 0)
			break;
	}
	mediatype.MajorType = tsmf_major_type_map[i].type;
	LLOGLN(0, (" (%s)", tsmf_major_type_map[i].name));

	LLOG(0, ("SubType:    "));
	tsmf_print_guid(pMediaType + 16);
	for (i = 0; tsmf_sub_type_map[i].type != TSMF_SUB_TYPE_UNKNOWN; i++)
	{
		if (memcmp(tsmf_sub_type_map[i].guid, pMediaType + 16, 16) == 0)
			break;
	}
	mediatype.SubType = tsmf_sub_type_map[i].type;
	LLOGLN(0, (" (%s)", tsmf_sub_type_map[i].name));

	LLOG(0, ("FormatType: "));
	tsmf_print_guid(pMediaType + 44);
	for (i = 0; tsmf_format_type_map[i].type != TSMF_FORMAT_TYPE_DEFAULT; i++)
	{
		if (memcmp(tsmf_format_type_map[i].guid, pMediaType + 44, 16) == 0)
			break;
	}
	mediatype.FormatType = tsmf_format_type_map[i].type;
	LLOGLN(0, (" (%s)", tsmf_format_type_map[i].name));

	cbFormat = GET_UINT32(pMediaType, 60);
	LLOGLN(0, ("tsmf_stream_set_format: cbFormat %d", cbFormat));

	for (i = 0; i < cbFormat; i++)
	{
		LLOG(0, ("%02X ", pMediaType[64 + i]));
		if (i % 16 == 15)
			LLOG(0, ("\n"));
	}
	LLOG(0, ("\n"));

	switch (mediatype.FormatType)
	{
		case TSMF_FORMAT_TYPE_MFVIDEOFORMAT:
			/* http://msdn.microsoft.com/en-us/library/aa473808.aspx */

			/* MFVIDEOFORMAT.videoInfo.dwWidth */
			mediatype.Width = GET_UINT32(pMediaType, 64 + 8);
			/* MFVIDEOFORMAT.videoInfo.dwHeight */
			mediatype.Height = GET_UINT32(pMediaType, 64 + 12);
			/* MFVIDEOFORMAT.compressedInfo.AvgBitrate */
			mediatype.BitRate = GET_UINT32(pMediaType, 64 + 136);
			/* MFVIDEOFORMAT.videoInfo.FramesPerSecond */
			mediatype.SamplesPerSecond.Numerator = GET_UINT32(pMediaType, 64 + 48);
			mediatype.SamplesPerSecond.Denominator = GET_UINT32(pMediaType, 64 + 52);

			if (cbFormat > 176)
			{
				mediatype.ExtraDataSize = cbFormat - 176;
				mediatype.ExtraData = pMediaType + 64 + 176;
			}
			break;

		case TSMF_FORMAT_TYPE_WAVEFORMATEX:
			/* http://msdn.microsoft.com/en-us/library/dd757720.aspx */

			mediatype.Channels = GET_UINT16(pMediaType, 64 + 2);
			mediatype.SamplesPerSecond.Numerator = GET_UINT32(pMediaType, 64 + 4);
			mediatype.SamplesPerSecond.Denominator = 1;
			mediatype.BitRate = GET_UINT32(pMediaType, 64 + 8) * 8;
			mediatype.BlockAlign = GET_UINT16(pMediaType, 64 + 12);
			mediatype.BitsPerSample = GET_UINT16(pMediaType, 64 + 14);
			mediatype.ExtraDataSize = GET_UINT16(pMediaType, 64 + 16);
			if (mediatype.ExtraDataSize > 0)
				mediatype.ExtraData = pMediaType + 64 + 18;
			
			break;

		default:
			if (mediatype.MajorType == TSMF_MAJOR_TYPE_VIDEO)
			{
				/* VIDEOINFOHEADER.rcSource, RECT(LONG left, LONG top, LONG right, LONG bottom) */
				mediatype.Width = GET_UINT32(pMediaType, 64 + 8);
				mediatype.Height = GET_UINT32(pMediaType, 64 + 12);
				/* VIDEOINFOHEADER.dwBitRate */
				mediatype.BitRate = GET_UINT32(pMediaType, 64 + 32);
				/* VIDEOINFOHEADER.AvgTimePerFrame */
				mediatype.SamplesPerSecond.Numerator = (int)(10000000LL / GET_UINT64(pMediaType, 64 + 40));
				mediatype.SamplesPerSecond.Denominator = 1;
			}
			break;
	}

	if (mediatype.SamplesPerSecond.Numerator == 0)
		mediatype.SamplesPerSecond.Numerator = 1;
	if (mediatype.SamplesPerSecond.Denominator == 0)
		mediatype.SamplesPerSecond.Denominator = 1;

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
		stream->presentation->sample_rate = mediatype.SamplesPerSecond.Numerator;
		stream->presentation->channels = mediatype.Channels;
		stream->presentation->bits_per_sample = mediatype.BitsPerSample;
	}

	stream->major_type = mediatype.MajorType;
	stream->width = mediatype.Width;
	stream->height = mediatype.Height;
	stream->decoder = tsmf_load_decoder(name, &mediatype);
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

	stream->eos = 0;
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

	if (stream->decoder)
		stream->decoder->Free(stream->decoder);

	free(stream);
}

void
tsmf_stream_push_sample(TSMF_STREAM * stream, IWTSVirtualChannelCallback * pChannelCallback,
	uint32 sample_id, uint64 start_time, uint64 end_time, uint64 duration, uint32 extensions,
	uint32 data_size, uint8 * data)
{
	TSMF_PRESENTATION * presentation = stream->presentation;
	TSMF_SAMPLE * sample;
	int ret = 1;

	if (stream->decoder)
		ret = stream->decoder->Decode(stream->decoder, data, data_size, extensions);
	if (ret)
		return;

	sample = (TSMF_SAMPLE *) malloc(sizeof(TSMF_SAMPLE));
	memset(sample, 0, sizeof(TSMF_SAMPLE));

	sample->sample_id = sample_id;
	sample->start_time = start_time;
	sample->end_time = end_time;
	sample->duration = duration;
	sample->stream = stream;
	sample->channel_callback = pChannelCallback;

	if (stream->decoder->GetDecodedData)
	{
		sample->data = stream->decoder->GetDecodedData(stream->decoder, &sample->data_size);
	}

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

