/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel - Codec

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
#include "tsmf_constants.h"
#include "tsmf_types.h"
#include "tsmf_codec.h"

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
		"MEDIASUBTYPE_WMAudioV2", /* V7, V8 has the same GUID */
		TSMF_SUB_TYPE_WMA2
	},

	/* 00000162-0000-0010-8000-00AA00389B71 */
	{
		{ 0x62, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"MEDIASUBTYPE_WMAudioV9",
		TSMF_SUB_TYPE_WMA9
	},

	/* 00000055-0000-0010-8000-00AA00389B71 */
	{
		{ 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"MEDIASUBTYPE_MP3",
		TSMF_SUB_TYPE_MP3
	},

	/* E06D802B-DB46-11CF-B4D1-00805F6CBBEA */
	{
		{ 0x2B, 0x80, 0x6D, 0xE0, 0x46, 0xDB, 0xCF, 0x11, 0xB4, 0xD1, 0x00, 0x80, 0x5F, 0x6C, 0xBB, 0xEA },
		"MEDIASUBTYPE_MPEG2_AUDIO",
		TSMF_SUB_TYPE_MP2A
	},

	/* E06D8026-DB46-11CF-B4D1-00805F6CBBEA */
	{
		{ 0x26, 0x80, 0x6D, 0xE0, 0x46, 0xDB, 0xCF, 0x11, 0xB4, 0xD1, 0x00, 0x80, 0x5F, 0x6C, 0xBB, 0xEA },
		"MEDIASUBTYPE_MPEG2_VIDEO",
		TSMF_SUB_TYPE_MP2V
	},

	/* 33564D57-0000-0010-8000-00AA00389B71 */
	{
		{ 0x57, 0x4D, 0x56, 0x33, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"MEDIASUBTYPE_WMV3",
		TSMF_SUB_TYPE_WMV3
	},

	/* 00001610-0000-0010-8000-00AA00389B71 */
	{
		{ 0x10, 0x16, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"MEDIASUBTYPE_MPEG_HEAAC",
		TSMF_SUB_TYPE_AAC
	},

	/* 34363248-0000-0010-8000-00AA00389B71 */
	{
		{ 0x48, 0x32, 0x36, 0x34, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"MEDIASUBTYPE_H264",
		TSMF_SUB_TYPE_H264
	},

	/* 31435641-0000-0010-8000-00AA00389B71 */
	{
		{ 0x41, 0x56, 0x43, 0x31, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 },
		"MEDIASUBTYPE_AVC1",
		TSMF_SUB_TYPE_AVC1
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

	/* E06D80E3-DB46-11CF-B4D1-00805F6CBBEA */
	{
		{ 0xE3, 0x80, 0x6D, 0xE0, 0x46, 0xDB, 0xCF, 0x11, 0xB4, 0xD1, 0x00, 0x80, 0x5F, 0x6C, 0xBB, 0xEA },
		"FORMAT_MPEG2_VIDEO",
		TSMF_FORMAT_TYPE_MPEG2VIDEOINFO
	},

	/* F72A76A0-EB0A-11D0-ACE4-0000C0CC16BA */
	{
		{ 0xA0, 0x76, 0x2A, 0xF7, 0x0A, 0xEB, 0xD0, 0x11, 0xAC, 0xE4, 0x00, 0x00, 0xC0, 0xCC, 0x16, 0xBA },
		"FORMAT_VideoInfo2",
		TSMF_FORMAT_TYPE_VIDEOINFO2
	},

	{
		{ 0 },
		"Unknown",
		TSMF_FORMAT_TYPE_UNKNOWN
	}
};

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

/* http://msdn.microsoft.com/en-us/library/dd318229.aspx */
static int
tsmf_codec_parse_BITMAPINFOHEADER(TS_AM_MEDIA_TYPE * mediatype, const uint8 * pBitmap)
{
	uint32 biSize;

	biSize = GET_UINT32(pBitmap, 0);
	if (mediatype->Width == 0)
		mediatype->Width = GET_UINT32(pBitmap, 4);
	if (mediatype->Height == 0)
		mediatype->Height = GET_UINT32(pBitmap, 8);
	/* Assume there will be no color table for video? */
	return biSize;
}

/* http://msdn.microsoft.com/en-us/library/dd407326.aspx */
static int
tsmf_codec_parse_VIDEOINFOHEADER2(TS_AM_MEDIA_TYPE * mediatype, const uint8 * pFormat)
{
	/* VIDEOINFOHEADER2.rcSource, RECT(LONG left, LONG top, LONG right, LONG bottom) */
	mediatype->Width = GET_UINT32(pFormat, 8);
	mediatype->Height = GET_UINT32(pFormat, 12);
	/* VIDEOINFOHEADER2.dwBitRate */
	mediatype->BitRate = GET_UINT32(pFormat, 32);
	/* VIDEOINFOHEADER2.AvgTimePerFrame */
	mediatype->SamplesPerSecond.Numerator = 1000000;
	mediatype->SamplesPerSecond.Denominator = (int)(GET_UINT64(pFormat, 40) / 10LL);

	return 72;
}

int
tsmf_codec_parse_media_type(TS_AM_MEDIA_TYPE * mediatype, const uint8 * pMediaType)
{
	uint32 cbFormat;
	const uint8 * pFormat;
	int i;
	int ret = 0;

	memset(mediatype, 0, sizeof(TS_AM_MEDIA_TYPE));

	LLOG(0, ("MajorType:  "));
	tsmf_print_guid(pMediaType);
	for (i = 0; tsmf_major_type_map[i].type != TSMF_MAJOR_TYPE_UNKNOWN; i++)
	{
		if (memcmp(tsmf_major_type_map[i].guid, pMediaType, 16) == 0)
			break;
	}
	mediatype->MajorType = tsmf_major_type_map[i].type;
	if (mediatype->MajorType == TSMF_MAJOR_TYPE_UNKNOWN)
		ret = 1;
	LLOGLN(0, (" (%s)", tsmf_major_type_map[i].name));

	LLOG(0, ("SubType:    "));
	tsmf_print_guid(pMediaType + 16);
	for (i = 0; tsmf_sub_type_map[i].type != TSMF_SUB_TYPE_UNKNOWN; i++)
	{
		if (memcmp(tsmf_sub_type_map[i].guid, pMediaType + 16, 16) == 0)
			break;
	}
	mediatype->SubType = tsmf_sub_type_map[i].type;
	if (mediatype->SubType == TSMF_SUB_TYPE_UNKNOWN)
		ret = 1;
	LLOGLN(0, (" (%s)", tsmf_sub_type_map[i].name));

	LLOG(0, ("FormatType: "));
	tsmf_print_guid(pMediaType + 44);
	for (i = 0; tsmf_format_type_map[i].type != TSMF_FORMAT_TYPE_UNKNOWN; i++)
	{
		if (memcmp(tsmf_format_type_map[i].guid, pMediaType + 44, 16) == 0)
			break;
	}
	mediatype->FormatType = tsmf_format_type_map[i].type;
	if (mediatype->FormatType == TSMF_FORMAT_TYPE_UNKNOWN)
		ret = 1;
	LLOGLN(0, (" (%s)", tsmf_format_type_map[i].name));

	cbFormat = GET_UINT32(pMediaType, 60);
	LLOGLN(0, ("tsmf_stream_set_format: cbFormat %d", cbFormat));

	pFormat = pMediaType + 64;

	for (i = 0; i < cbFormat; i++)
	{
		LLOG(0, ("%02X ", pFormat[i]));
		if (i % 16 == 15)
			LLOG(0, ("\n"));
	}
	LLOG(0, ("\n"));

	switch (mediatype->FormatType)
	{
		case TSMF_FORMAT_TYPE_MFVIDEOFORMAT:
			/* http://msdn.microsoft.com/en-us/library/aa473808.aspx */

			/* MFVIDEOFORMAT.videoInfo.dwWidth */
			mediatype->Width = GET_UINT32(pFormat, 8);
			/* MFVIDEOFORMAT.videoInfo.dwHeight */
			mediatype->Height = GET_UINT32(pFormat, 12);
			/* MFVIDEOFORMAT.compressedInfo.AvgBitrate */
			mediatype->BitRate = GET_UINT32(pFormat, 136);
			/* MFVIDEOFORMAT.videoInfo.FramesPerSecond */
			mediatype->SamplesPerSecond.Numerator = GET_UINT32(pFormat, 48);
			mediatype->SamplesPerSecond.Denominator = GET_UINT32(pFormat, 52);

			if (cbFormat > 176)
			{
				mediatype->ExtraDataSize = cbFormat - 176;
				mediatype->ExtraData = pFormat + 176;
			}
			break;

		case TSMF_FORMAT_TYPE_WAVEFORMATEX:
			/* http://msdn.microsoft.com/en-us/library/dd757720.aspx */

			mediatype->Channels = GET_UINT16(pFormat, 2);
			mediatype->SamplesPerSecond.Numerator = GET_UINT32(pFormat, 4);
			mediatype->SamplesPerSecond.Denominator = 1;
			mediatype->BitRate = GET_UINT32(pFormat, 8) * 8;
			mediatype->BlockAlign = GET_UINT16(pFormat, 12);
			mediatype->BitsPerSample = GET_UINT16(pFormat, 14);
			mediatype->ExtraDataSize = GET_UINT16(pFormat, 16);
			if (mediatype->ExtraDataSize > 0)
				mediatype->ExtraData = pFormat + 18;
			
			break;

		case TSMF_FORMAT_TYPE_MPEG2VIDEOINFO:
			/* http://msdn.microsoft.com/en-us/library/dd390707.aspx */
			i = tsmf_codec_parse_VIDEOINFOHEADER2(mediatype, pFormat);
			i += tsmf_codec_parse_BITMAPINFOHEADER(mediatype, pFormat + i);
			if (cbFormat > i)
			{
				mediatype->ExtraDataSize = cbFormat - i;
				mediatype->ExtraData = pFormat + i;
			}
			break;

		case TSMF_FORMAT_TYPE_VIDEOINFO2:
			i = tsmf_codec_parse_VIDEOINFOHEADER2(mediatype, pFormat);
			tsmf_codec_parse_BITMAPINFOHEADER(mediatype, pFormat + i);
			i += 40; /* Skip only BITMAPINFOHEADER */
			if (cbFormat > i)
			{
				mediatype->ExtraDataSize = cbFormat - i;
				mediatype->ExtraData = pFormat + i;
			}
			break;

		default:
			break;
	}

	if (mediatype->SamplesPerSecond.Numerator == 0)
		mediatype->SamplesPerSecond.Numerator = 1;
	if (mediatype->SamplesPerSecond.Denominator == 0)
		mediatype->SamplesPerSecond.Denominator = 1;

	return ret;
}

int
tsmf_codec_check_media_type(const uint8 * pMediaType)
{
	TS_AM_MEDIA_TYPE mediatype;

	return tsmf_codec_parse_media_type(&mediatype, pMediaType);
}

