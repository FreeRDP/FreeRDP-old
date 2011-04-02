/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel - FFmpeg Decoder

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
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include "tsmf_constants.h"
#include "tsmf_decoder.h"

/* Compatibility with older FFmpeg */
#if LIBAVUTIL_VERSION_MAJOR < 50
#define AVMEDIA_TYPE_VIDEO 0
#define AVMEDIA_TYPE_AUDIO 1
#endif

typedef struct _TSMFFFmpegDecoder
{
	ITSMFDecoder iface;

	int media_type;
	enum CodecID codec_id;
	AVCodecContext * codec_context;
	AVCodec * codec;
	AVFrame * frame;
	AVFrame * output_frame;
	uint8 * output_buffer;
	struct SwsContext * sws_context;
	int prepared;

	uint32 width;
	uint32 height;
} TSMFFFmpegDecoder;

static int
tsmf_ffmpeg_init_context(ITSMFDecoder * decoder)
{
	TSMFFFmpegDecoder * mdecoder = (TSMFFFmpegDecoder *) decoder;

	mdecoder->codec_context = avcodec_alloc_context();
	if (!mdecoder->codec_context)
	{
		LLOGLN(0, ("tsmf_ffmpeg_init_context: avcodec_alloc_context failed."));
		return 1;
	}

	return 0;
}

static int
tsmf_ffmpeg_init_video_frame_buffer(ITSMFDecoder * decoder)
{
	TSMFFFmpegDecoder * mdecoder = (TSMFFFmpegDecoder *) decoder;
	int size;

	if (mdecoder->output_frame)
		av_free(mdecoder->output_frame);
	if (mdecoder->output_buffer)
		free(mdecoder->output_buffer);
	mdecoder->output_frame = avcodec_alloc_frame();
	size = avpicture_get_size(PIX_FMT_RGB24, mdecoder->width, mdecoder->height);
	mdecoder->output_buffer = malloc(size);
	avpicture_fill((AVPicture *) mdecoder->output_frame, mdecoder->output_buffer,
		PIX_FMT_RGB24, mdecoder->width, mdecoder->height);

	return 0;
}

static int
tsmf_ffmpeg_set_size(ITSMFDecoder * decoder, uint32 width, uint32 height)
{
	TSMFFFmpegDecoder * mdecoder = (TSMFFFmpegDecoder *) decoder;

	if (width > 0 && height > 0 && (!mdecoder->output_frame || mdecoder->width != width || mdecoder->height != height))
	{
		mdecoder->width = width;
		mdecoder->height = height;
		return tsmf_ffmpeg_init_video_frame_buffer(decoder);
	}
	return 0;
}

static int
tsmf_ffmpeg_init_video_stream(ITSMFDecoder * decoder, const TS_AM_MEDIA_TYPE * media_type)
{
	TSMFFFmpegDecoder * mdecoder = (TSMFFFmpegDecoder *) decoder;

	mdecoder->codec_context->width = media_type->Width;
	mdecoder->codec_context->height = media_type->Height;
	mdecoder->codec_context->bit_rate = media_type->BitRate;
	mdecoder->codec_context->time_base.den = media_type->SamplesPerSecond.Numerator;
	mdecoder->codec_context->time_base.num = media_type->SamplesPerSecond.Denominator;

	mdecoder->codec_context->gop_size = 12;
	mdecoder->codec_context->pix_fmt = PIX_FMT_YUV420P;

	mdecoder->frame = avcodec_alloc_frame();

	return 0;
}

static int
tsmf_ffmpeg_init_audio_stream(ITSMFDecoder * decoder, const TS_AM_MEDIA_TYPE * media_type)
{
	TSMFFFmpegDecoder * mdecoder = (TSMFFFmpegDecoder *) decoder;

	mdecoder->codec_context->sample_rate = media_type->SamplesPerSecond.Numerator;
	mdecoder->codec_context->bit_rate = media_type->BitRate;
	mdecoder->codec_context->channels = media_type->Channels;
	mdecoder->codec_context->block_align = media_type->BlockAlign;

	switch (media_type->BitsPerSample)
	{
		case 8:
			mdecoder->codec_context->sample_fmt = SAMPLE_FMT_U8;
			break;
		case 16:
			mdecoder->codec_context->sample_fmt = SAMPLE_FMT_S16;
			break;
		case 32:
			mdecoder->codec_context->sample_fmt = SAMPLE_FMT_S32;
			break;
	}

	return 0;
}

static int
tsmf_ffmpeg_init_stream(ITSMFDecoder * decoder, const TS_AM_MEDIA_TYPE * media_type)
{
	TSMFFFmpegDecoder * mdecoder = (TSMFFFmpegDecoder *) decoder;

	mdecoder->codec = avcodec_find_decoder(mdecoder->codec_id);
	if (!mdecoder->codec)
	{
		LLOGLN(0, ("tsmf_ffmpeg_init_stream: avcodec_find_decoder failed."));
		return 1;
	}

	mdecoder->codec_context->codec_id = mdecoder->codec_id;
	mdecoder->codec_context->codec_type = mdecoder->media_type;

	if (mdecoder->media_type == AVMEDIA_TYPE_VIDEO)
	{
		if (tsmf_ffmpeg_init_video_stream(decoder, media_type))
			return 1;
	}
	else if (mdecoder->media_type == AVMEDIA_TYPE_AUDIO)
	{
		if (tsmf_ffmpeg_init_audio_stream(decoder, media_type))
			return 1;
	}

	if (media_type->ExtraData)
	{
		mdecoder->codec_context->extradata_size = media_type->ExtraDataSize;
		mdecoder->codec_context->extradata = malloc(mdecoder->codec_context->extradata_size);
		memcpy(mdecoder->codec_context->extradata, media_type->ExtraData, media_type->ExtraDataSize);
	}

	if (mdecoder->codec->capabilities & CODEC_CAP_TRUNCATED)
		mdecoder->codec_context->flags |= CODEC_FLAG_TRUNCATED;

	return 0;
}

static int
tsmf_ffmpeg_prepare(ITSMFDecoder * decoder)
{
	TSMFFFmpegDecoder * mdecoder = (TSMFFFmpegDecoder *) decoder;

	if (avcodec_open(mdecoder->codec_context, mdecoder->codec) < 0)
	{
		LLOGLN(0, ("tsmf_ffmpeg_prepare: avcodec_open failed."));
		return 1;
	}

	mdecoder->prepared = 1;

	return 0;
}

static int
tsmf_ffmpeg_set_format(ITSMFDecoder * decoder, const TS_AM_MEDIA_TYPE * media_type)
{
	TSMFFFmpegDecoder * mdecoder = (TSMFFFmpegDecoder *) decoder;

	switch (media_type->MajorType)
	{
		case TSMF_MAJOR_TYPE_VIDEO:
			mdecoder->media_type = AVMEDIA_TYPE_VIDEO;
			break;
		case TSMF_MAJOR_TYPE_AUDIO:
			mdecoder->media_type = AVMEDIA_TYPE_AUDIO;
			break;
		default:
			return 1;
	}
	switch (media_type->SubType)
	{
		case TSMF_SUB_TYPE_WVC1:
			mdecoder->codec_id = CODEC_ID_VC1;
			break;
		case TSMF_SUB_TYPE_WMA2:
			mdecoder->codec_id = CODEC_ID_WMAV2;
			break;
		case TSMF_SUB_TYPE_WMA9:
			mdecoder->codec_id = CODEC_ID_WMAPRO;
			break;
		default:
			return 1;
	}

	if (tsmf_ffmpeg_init_context(decoder))
		return 1;
	if (tsmf_ffmpeg_init_stream(decoder, media_type))
		return 1;
	if (tsmf_ffmpeg_prepare(decoder))
		return 1;

	return 0;
}

static int
tsmf_ffmpeg_decode_video(ITSMFDecoder * decoder, const uint8 * data, uint32 data_size, uint32 extensions)
{
	TSMFFFmpegDecoder * mdecoder = (TSMFFFmpegDecoder *) decoder;
	int decoded;
	int len;
	int ret = 0;

	len = avcodec_decode_video(mdecoder->codec_context, mdecoder->frame, &decoded, data, data_size);
	if (len < 0)
	{
		LLOGLN(0, ("tsmf_ffmpeg_decode_video: avcodec_decode_video failed (%d)", len));
		ret = 1;
	}
	else if (!decoded)
	{
		LLOGLN(0, ("tsmf_ffmpeg_decode_video: data_size %d, no frame is decoded.", data_size));
		ret = 1;
	}
	else
	{
		LLOGLN(0, ("tsmf_ffmpeg_decode_video: linesize[0] %d linesize[1] %d linesize[2] %d linesize[3] %d",
			mdecoder->frame->linesize[0], mdecoder->frame->linesize[1],
			mdecoder->frame->linesize[2], mdecoder->frame->linesize[3]));

		if (!mdecoder->output_frame)
			tsmf_ffmpeg_init_video_frame_buffer(decoder);
		mdecoder->sws_context = sws_getCachedContext(
			mdecoder->sws_context,
			mdecoder->codec_context->width,
			mdecoder->codec_context->height,
			mdecoder->codec_context->pix_fmt,
			mdecoder->width,
			mdecoder->height,
			PIX_FMT_RGB24,
			SWS_BILINEAR,
			NULL, NULL, NULL);
		sws_scale(mdecoder->sws_context,
			(const uint8_t**) mdecoder->frame->data, mdecoder->frame->linesize,
			0, mdecoder->codec_context->height,
			mdecoder->output_frame->data, mdecoder->output_frame->linesize);

		LLOGLN(0, ("tsmf_ffmpeg_decode_video: output linesize[0] %d linesize[1] %d linesize[2] %d linesize[3] %d",
			mdecoder->output_frame->linesize[0], mdecoder->output_frame->linesize[1],
			mdecoder->output_frame->linesize[2], mdecoder->output_frame->linesize[3]));
	}

	return ret;
}

static int
tsmf_ffmpeg_decode_audio(ITSMFDecoder * decoder, const uint8 * data, uint32 data_size, uint32 extensions)
{
	LLOGLN(0, ("tsmf_ffmpeg_decode_audio: data_size %d", data_size));

	return 0;
}

static int
tsmf_ffmpeg_decode(ITSMFDecoder * decoder, const uint8 * data, uint32 data_size, uint32 extensions)
{
	TSMFFFmpegDecoder * mdecoder = (TSMFFFmpegDecoder *) decoder;

	switch (mdecoder->media_type)
	{
		case AVMEDIA_TYPE_VIDEO:
			return tsmf_ffmpeg_decode_video(decoder, data, data_size, extensions);
		case AVMEDIA_TYPE_AUDIO:
			return tsmf_ffmpeg_decode_audio(decoder, data, data_size, extensions);
		default:
			LLOGLN(0, ("tsmf_ffmpeg_decode: unknown media type."));
			return 1;
	}
}

static void
tsmf_ffmpeg_free(ITSMFDecoder * decoder)
{
	TSMFFFmpegDecoder * mdecoder = (TSMFFFmpegDecoder *) decoder;

	if (mdecoder->frame)
		av_free(mdecoder->frame);
	if (mdecoder->output_frame)
		av_free(mdecoder->output_frame);
	if (mdecoder->output_buffer)
		free(mdecoder->output_buffer);
	if (mdecoder->sws_context)
		sws_freeContext(mdecoder->sws_context);
	if (mdecoder->codec_context)
	{
		if (mdecoder->prepared)
			avcodec_close(mdecoder->codec_context);
		if (mdecoder->codec_context->extradata)
			free(mdecoder->codec_context->extradata);
		av_free(mdecoder->codec_context);
	}
	free(decoder);
}

ITSMFDecoder *
TSMFDecoderEntry(void)
{
	TSMFFFmpegDecoder * decoder;

	avcodec_init();
	avcodec_register_all();

	decoder = malloc(sizeof(TSMFFFmpegDecoder));
	memset(decoder, 0, sizeof(TSMFFFmpegDecoder));

	decoder->iface.SetFormat = tsmf_ffmpeg_set_format;
	decoder->iface.SetSize = tsmf_ffmpeg_set_size;
	decoder->iface.Decode = tsmf_ffmpeg_decode;
	decoder->iface.Free = tsmf_ffmpeg_free;

	return (ITSMFDecoder *) decoder;
}

