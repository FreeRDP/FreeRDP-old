/*
   FreeRDP: A Remote Desktop Protocol client.
   RemoteFX Codec Library - Bit Stream

   Copyright 2011 Vic Lee

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

#ifndef __RFX_BITSTREAM_H
#define __RFX_BITSTREAM_H

#include <freerdp/rfx.h>

struct _RFX_BITSTREAM
{
	unsigned char * bytes;
	int nbytes;
	int byte_pos;
	int bits_left;
};
typedef struct _RFX_BITSTREAM RFX_BITSTREAM;

RFX_BITSTREAM *
rfx_bitstream_new(void);
void
rfx_bitstream_put_bytes(RFX_BITSTREAM * bs, const unsigned char * bytes, int nbytes);
unsigned int
rfx_bitstream_get_bits(RFX_BITSTREAM * bs, int nbits);
int
rfx_bitstream_eos(RFX_BITSTREAM * bs);
int
rfx_bitstream_left(RFX_BITSTREAM * bs);
void
rfx_bitstream_free(RFX_BITSTREAM * bs);

#endif

