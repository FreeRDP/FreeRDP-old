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
	STREAM rv;

	rv = 0;
	if (memcmp(codec_guid, g_rfx_guid, 16) == 0)
	{
		printf("got remotefx guid\n");
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
	return rv;
}

int
surface_cmd(rdpRdp * rdp, STREAM s)
{
	printf("surface_cmd:\n");
  return 0;
}
