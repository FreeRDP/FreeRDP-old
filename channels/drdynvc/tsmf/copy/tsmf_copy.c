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
#include "tsmf_decoder.h"

typedef struct _FFmpegSubTypeMap
{
	uint8 guid[16];
	const char * name;
	enum CodecID codec_id;
} FFmpegSubTypeMap;

typedef struct _FFmpegMajorTypeMap
{
	uint8 guid[16];
	const char * name;
	int media_type;
	const FFmpegSubTypeMap * sub_type_map;
} FFmpegMajorTypeMap;

static const FFmpegSubTypeMap ffmpeg_video_sub_type_map[] =
{
	/* 31435657-0000-0010-8000-00AA00389B71 */
	{
		{ 0x57, 0x56, 0x43, 0x31, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"MEDIASUBTYPE_WVC1",
		CODEC_ID_VC1
	},

	{
		{ 0 },
		NULL,
		0
	}

};

static const FFmpegMajorTypeMap ffmpeg_majortype_map[] =
{
	/* 73646976-0000-0010-8000-00AA00389B71 */
	{
		{ 0x76, 0x69, 0x64, 0x73, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"MEDIATYPE_Video",
		0, /* AVMEDIA_TYPE_VIDEO */
		ffmpeg_video_sub_type_map
	},

	{
		{ 0 },
		NULL,
		0,
		NULL
	}
};

typedef struct _TSMFCopyDecoder
{
	ITSMFDecoder iface;

	int media_type;
	enum CodecID codec_id;
} TSMFCopyDecoder;

static int
tsmf_copy_set_format(ITSMFDecoder * decoder, const uint8 * pMediaType)
{
	TSMFCopyDecoder * copy_decoder = (TSMFCopyDecoder *) decoder;
	const FFmpegMajorTypeMap * majortype = NULL;
	const FFmpegSubTypeMap * subtype = NULL;
	int i;

	for (i = 0; ffmpeg_majortype_map[i].name; i++)
	{
		if (memcmp(ffmpeg_majortype_map[i].guid, pMediaType, 16) == 0)
		{
			majortype = &ffmpeg_majortype_map[i];
			break;
		}
	}
	if (majortype == NULL)
	{
		LLOGLN(0, ("tsmf_copy_set_format: unsupported major type"));
		return 1;
	}

	for (i = 0; majortype->sub_type_map[i].name; i++)
	{
		if (memcmp(majortype->sub_type_map[i].guid, pMediaType + 16, 16) == 0)
		{
			subtype = &majortype->sub_type_map[i];
			break;
		}
	}
	if (subtype == NULL)
	{
		LLOGLN(0, ("tsmf_copy_set_format: unsupported major type"));
		return 1;
	}

	LLOGLN(0, ("tsmf_copy_set_format: %s / %s", majortype->name, subtype->name));
	copy_decoder->media_type = majortype->media_type;
	copy_decoder->codec_id = subtype->codec_id;

	return 0;
}

static int
tsmf_copy_decode(ITSMFDecoder * decoder, const uint8 * data, uint32 data_size, uint8 ** decoded_data, uint32 * decoded_size)
{
	*decoded_data = malloc(data_size);
	memcpy(*decoded_data, data, data_size);
	*decoded_size = data_size;
	return 0;
}

static void
tsmf_copy_free(ITSMFDecoder * decoder)
{
	free(decoder);
}

ITSMFDecoder *
TSMFDecoderEntry(void)
{
	TSMFCopyDecoder * decoder;

	decoder = malloc(sizeof(TSMFCopyDecoder));
	memset(decoder, 0, sizeof(TSMFCopyDecoder));
	decoder->iface.SetFormat = tsmf_copy_set_format;
	decoder->iface.Decode = tsmf_copy_decode;
	decoder->iface.Free = tsmf_copy_free;

	return (ITSMFDecoder *) decoder;
}

