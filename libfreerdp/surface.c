/*
   FreeRDP: A Remote Desktop Protocol client.
   surface routines

   Copyright (C) Jay Sorg 2011

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

#include <freerdp/freerdp.h>
#include "frdp.h"
#include "rdp.h"
#include "stream.h"
#include "mem.h"

/* CODEC_GUID_REMOTEFX
   0x76772F12BD724463AFB3B73C9C6F7886 */
static uint8 g_rfx_guid[] =
{ 0x12, 0x2f, 0x77, 0x76, 0x72, 0xbd, 0x63, 0x44,
  0xaf, 0xb3, 0xb7, 0x3c, 0x9c, 0x6f, 0x78, 0x86 };

/* CODEC_GUID_NSCODEC
   0xCA8D1BB9000F154F589FAE2D1A87E2D6 */
static uint8 g_nsc_guid[] =
{ 0xb9, 0x1b, 0x8d, 0xca, 0x0f, 0x00, 0x4f, 0x15,
  0x58, 0x9f, 0xae, 0x2d, 0x1a, 0x87, 0xe2, 0xd6 };

STREAM
surface_codec_cap(rdpRdp * rdp, uint8 * codec_guid, int codec_id,
	uint8 * codec_property, int codec_properties_size)
{
	STREAM s;

	s = 0;
	if (memcmp(codec_guid, g_rfx_guid, 16) == 0)
	{
		printf("got remotefx guid\n");
		if (rdp->settings->rfx_flags)
		{
			s = stream_new(1024);
			out_uint8a(s, g_rfx_guid, 16);
			out_uint8(s, codec_id);
			out_uint16_le(s, 29 + 12);
			out_uint32_le(s, 29 + 12); /* total size */
			out_uint32_le(s, 0x00000000); /* Capture Flags */
			out_uint32_le(s, 29); /* size after this */
			/* struct CbyCaps */
			out_uint16_le(s, 0xcbc0); /* CBY_CAPS */
			out_uint32_le(s, 8); /* size of this struct */
			out_uint16_le(s, 1); /* numCapsets */
			/* struct ClyCapset */
			out_uint16_le(s, 0xcbc1); /* CBY_CAPSET */
			out_uint32_le(s, 21); /* size of this struct */
			out_uint8(s, 1); /* codec id */
			out_uint16_le(s, 0xcfc0); /* CLY_CAPSET */
			out_uint16_le(s, 1); /* numIcaps */
			out_uint16_le(s, 8); /* icapLen */
			/* 64x64 tiles */
			out_uint16_le(s, 0x100); /* version */
			out_uint16_le(s, 64); /* tile size */
			out_uint8(s, 0); /* flags */
			out_uint8(s, 1); /* colConvBits */
			out_uint8(s, 1); /* transformBits */
			out_uint8(s, 1); /* entropyBits */
			s_mark_end(s);
		}
	}
	else if (memcmp(codec_guid, g_nsc_guid, 16) == 0)
	{
		printf("got nscodec guid\n");
	}
	else
	{
		printf("unknown guid\n");
		hexdump(codec_guid, 16);
	}
	return s;
}

int
surface_cmd(rdpRdp * rdp, STREAM s)
{
	int cmdType;
	int frameAction;
	int frameId;
	int destLeft;
	int destTop;
	int destRight;
	int destBottom;
	int bpp;
	int codecID;
	int width;
	int height;
	int bitmapDataLength;

	printf("surface_cmd: size %d\n", s->end - s->p);
	//hexdump(s->p, 1024);
	frameId = 0;
	while (s->p < s->end)
	{
		in_uint16_le(s, cmdType);
		printf("  surface_cmd: %d\n", cmdType);
		switch (cmdType)
		{
			case 4: /* CMDTYPE_FRAME_MARKER */
				in_uint16_le(s, frameAction);
				in_uint32_le(s, frameId);
				printf("    surface_cmd: CMDTYPE_FRAME_MARKER %d %d\n",
					frameAction, frameId);
				break;
			case 6: /* CMDTYPE_STREAM_SURFACE_BITS */
				in_uint16_le(s, destLeft);
				in_uint16_le(s, destTop);
				in_uint16_le(s, destRight);
				in_uint16_le(s, destBottom);
				in_uint8(s, bpp);
				in_uint8s(s, 2);
				in_uint8(s, codecID);
				in_uint16_le(s, width);
				in_uint16_le(s, height);
				in_uint32_le(s, bitmapDataLength);
				in_uint8s(s, bitmapDataLength);
				printf("    surface_cmd: CMDTYPE_STREAM_SURFACE_BITS "
					"id %d width %d height %d bpp %d size %d\n",
					codecID, width, height, bpp, bitmapDataLength);
				break;
		}
	}
	if (rdp->got_frame_ack_caps)
	{
		rdp_send_frame_ack(rdp, frameId);
	}
	return 0;
}
