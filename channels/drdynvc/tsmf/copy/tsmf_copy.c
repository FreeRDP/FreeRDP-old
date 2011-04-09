/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel - Copy Decoder and Consumer

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
#include <libavformat/avformat.h>
#include "tsmf_constants.h"
#include "tsmf_decoder.h"

/* Compatibility with older FFmpeg */
#if LIBAVFORMAT_VERSION_MAJOR < 52
#define av_guess_format guess_format
#endif

#if LIBAVUTIL_VERSION_MAJOR < 50
#define AVMEDIA_TYPE_VIDEO 0
#define AVMEDIA_TYPE_AUDIO 1
#endif

#if LIBAVCODEC_VERSION_MAJOR < 52
#define AV_PKT_FLAG_KEY PKT_FLAG_KEY
#endif

static int tsmf_copy_id_seq = 0;

typedef struct _TSMFCopyDecoder
{
	ITSMFDecoder iface;

	int media_type;
	enum CodecID codec_id;
	const char * format_name;
	AVOutputFormat * format;
	AVFormatContext * format_context;
	AVStream * stream;
	AVCodecContext * codec_context;
	int prepared;
} TSMFCopyDecoder;

static int
tsmf_copy_init_context(ITSMFDecoder * decoder)
{
	TSMFCopyDecoder * copy_decoder = (TSMFCopyDecoder *) decoder;

	copy_decoder->format = av_guess_format("matroska", NULL, NULL);
	if (copy_decoder->format)
	{
		copy_decoder->format_name = "mkv";
	}
	else
	{
		LLOGLN(0, ("tsmf_copy_init_context: Matroska is not supported. Try mp4."));
		copy_decoder->format = av_guess_format("mp4", NULL, NULL);
		if (copy_decoder->format)
		{
			copy_decoder->format_name = "mp4";
		}
		else
		{
			LLOGLN(0, ("tsmf_copy_init_context: av_guess_format failed."));
			return 1;
		}
	}

	copy_decoder->format_context = avformat_alloc_context();
	if (!copy_decoder->format_context)
	{
		LLOGLN(0, ("tsmf_copy_init_context: avformat_alloc_context failed."));
		return 1;
	}
	copy_decoder->format_context->oformat = copy_decoder->format;

	copy_decoder->format->video_codec = copy_decoder->codec_id;

	snprintf(copy_decoder->format_context->filename,
		sizeof(copy_decoder->format_context->filename),
		"/tmp/FreeRDP_%d.%s", tsmf_copy_id_seq++, copy_decoder->format_name);

	return 0;
}

static int
tsmf_copy_init_video_stream(ITSMFDecoder * decoder, const TS_AM_MEDIA_TYPE * media_type)
{
	TSMFCopyDecoder * copy_decoder = (TSMFCopyDecoder *) decoder;

	copy_decoder->codec_context->width = media_type->Width;
	copy_decoder->codec_context->height = media_type->Height;
	copy_decoder->codec_context->bit_rate = media_type->BitRate;
	copy_decoder->codec_context->time_base.den = media_type->SamplesPerSecond.Numerator;
	copy_decoder->codec_context->time_base.num = media_type->SamplesPerSecond.Denominator;

	copy_decoder->codec_context->gop_size = 12;
	copy_decoder->codec_context->pix_fmt = PIX_FMT_YUV420P;

	return 0;
}

static int
tsmf_copy_init_audio_stream(ITSMFDecoder * decoder, const TS_AM_MEDIA_TYPE * media_type)
{
	TSMFCopyDecoder * copy_decoder = (TSMFCopyDecoder *) decoder;

	copy_decoder->codec_context->sample_rate = media_type->SamplesPerSecond.Numerator;
	copy_decoder->codec_context->bit_rate = media_type->BitRate;
	copy_decoder->codec_context->channels = media_type->Channels;
	copy_decoder->codec_context->block_align = media_type->BlockAlign;

	switch (media_type->BitsPerSample)
	{
		case 8:
			copy_decoder->codec_context->sample_fmt = SAMPLE_FMT_U8;
			break;
		case 16:
			copy_decoder->codec_context->sample_fmt = SAMPLE_FMT_S16;
			break;
		case 32:
			copy_decoder->codec_context->sample_fmt = SAMPLE_FMT_S32;
			break;
	}

	return 0;
}

static int
tsmf_copy_init_stream(ITSMFDecoder * decoder, const TS_AM_MEDIA_TYPE * media_type)
{
	TSMFCopyDecoder * copy_decoder = (TSMFCopyDecoder *) decoder;

	copy_decoder->stream = av_new_stream(copy_decoder->format_context, 0);
	if (!copy_decoder->stream)
	{
		LLOGLN(0, ("tsmf_copy_init_stream: av_new_stream failed."));
		return 1;
	}

	copy_decoder->codec_context = copy_decoder->stream->codec;
	copy_decoder->codec_context->codec_id = copy_decoder->codec_id;
	copy_decoder->codec_context->codec_type = copy_decoder->media_type;

	if (copy_decoder->media_type == AVMEDIA_TYPE_VIDEO)
	{
		if (tsmf_copy_init_video_stream(decoder, media_type))
			return 1;
	}
	else if (copy_decoder->media_type == AVMEDIA_TYPE_AUDIO)
	{
		if (tsmf_copy_init_audio_stream(decoder, media_type))
			return 1;
	}

	if (media_type->ExtraData)
	{
		copy_decoder->codec_context->extradata_size = media_type->ExtraDataSize;
		copy_decoder->codec_context->extradata = malloc(copy_decoder->codec_context->extradata_size);
		memcpy(copy_decoder->codec_context->extradata, media_type->ExtraData, media_type->ExtraDataSize);
	}

	if (copy_decoder->format->flags & AVFMT_GLOBALHEADER)
		copy_decoder->codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return 0;
}

static int
tsmf_copy_prepare(ITSMFDecoder * decoder)
{
	TSMFCopyDecoder * copy_decoder = (TSMFCopyDecoder *) decoder;
	URLContext * h;

	if (av_set_parameters(copy_decoder->format_context, NULL) < 0)
	{
		LLOGLN(0, ("tsmf_copy_prepare: av_set_parameters failed."));
		return 1;
	}

	if (url_open(&h, copy_decoder->format_context->filename, URL_WRONLY) < 0)
	{
		LLOGLN(0, ("tsmf_copy_prepare: url_open failed to open file %s.",
			copy_decoder->format_context->filename));
		return 1;
	}
	if (url_fdopen(&copy_decoder->format_context->pb, h) < 0)
	{
		url_close(h);
		LLOGLN(0, ("tsmf_copy_prepare: url_fdopen failed to open file %s.",
			copy_decoder->format_context->filename));
		return 1;
	}

	av_write_header(copy_decoder->format_context);

	copy_decoder->prepared = 1;

	return 0;
}

static int
tsmf_copy_set_format(ITSMFDecoder * decoder, const TS_AM_MEDIA_TYPE * media_type)
{
	TSMFCopyDecoder * copy_decoder = (TSMFCopyDecoder *) decoder;

	switch (media_type->MajorType)
	{
		case TSMF_MAJOR_TYPE_VIDEO:
			copy_decoder->media_type = AVMEDIA_TYPE_VIDEO;
			break;
		case TSMF_MAJOR_TYPE_AUDIO:
			copy_decoder->media_type = AVMEDIA_TYPE_AUDIO;
			break;
		default:
			return 1;
	}
	switch (media_type->SubType)
	{
		case TSMF_SUB_TYPE_WVC1:
			copy_decoder->codec_id = CODEC_ID_VC1;
			break;
		case TSMF_SUB_TYPE_WMA2:
			copy_decoder->codec_id = CODEC_ID_WMAV2;
			break;
		case TSMF_SUB_TYPE_WMA9:
			copy_decoder->codec_id = CODEC_ID_WMAPRO;
			break;
		case TSMF_SUB_TYPE_MP3:
			copy_decoder->codec_id = CODEC_ID_MP3;
			break;
		case TSMF_SUB_TYPE_MP2A:
			copy_decoder->codec_id = CODEC_ID_MP2;
			break;
		case TSMF_SUB_TYPE_MP2V:
			copy_decoder->codec_id = CODEC_ID_MPEG2VIDEO;
			break;
		case TSMF_SUB_TYPE_WMV3:
			copy_decoder->codec_id = CODEC_ID_WMV3;
			break;
		default:
			return 1;
	}

	if (tsmf_copy_init_context(decoder))
		return 1;
	if (tsmf_copy_init_stream(decoder, media_type))
		return 1;
	if (tsmf_copy_prepare(decoder))
		return 1;

	return 0;
}

static int
tsmf_copy_decode(ITSMFDecoder * decoder, const uint8 * data, uint32 data_size, uint32 extensions)
{
	TSMFCopyDecoder * copy_decoder = (TSMFCopyDecoder *) decoder;
	AVPacket pkt;

	av_init_packet(&pkt);
	if (extensions & TSMM_SAMPLE_EXT_CLEANPOINT)
		pkt.flags |= AV_PKT_FLAG_KEY;
	pkt.stream_index = 0;
	pkt.data = (uint8_t *) data;
	pkt.size = data_size;

	return av_interleaved_write_frame(copy_decoder->format_context, &pkt);
}

static void
tsmf_copy_free(ITSMFDecoder * decoder)
{
	TSMFCopyDecoder * copy_decoder = (TSMFCopyDecoder *) decoder;

	if (copy_decoder->prepared)
	{
		av_write_trailer(copy_decoder->format_context);
	}
	if (copy_decoder->codec_context)
	{
		if (copy_decoder->codec_context->extradata)
			free(copy_decoder->codec_context->extradata);
	}
	if (copy_decoder->stream)
	{
		av_free(copy_decoder->stream);
	}
	if (copy_decoder->format_context)
	{
		if (copy_decoder->format_context->pb)
		{
			url_close(copy_decoder->format_context->pb->opaque);
			av_free(copy_decoder->format_context->pb->buffer);
			av_free(copy_decoder->format_context->pb);
		}	
		av_free(copy_decoder->format_context);
	}
	free(decoder);
}

ITSMFDecoder *
TSMFDecoderEntry(void)
{
	TSMFCopyDecoder * decoder;

	av_register_all();

	decoder = malloc(sizeof(TSMFCopyDecoder));
	memset(decoder, 0, sizeof(TSMFCopyDecoder));

	decoder->iface.SetFormat = tsmf_copy_set_format;
	decoder->iface.Decode = tsmf_copy_decode;
	decoder->iface.Free = tsmf_copy_free;

	return (ITSMFDecoder *) decoder;
}

