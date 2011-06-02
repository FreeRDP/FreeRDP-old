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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rfx_bitstream.h"

struct _RFX_BITSTREAM
{
	unsigned char * bytes;
	int nbytes;
	int byte_pos;
	int bits_left;
};

RFX_BITSTREAM *
rfx_bitstream_new(void)
{
	RFX_BITSTREAM * bs;

	bs = (RFX_BITSTREAM *) malloc(sizeof(RFX_BITSTREAM));
	memset(bs, 0, sizeof(RFX_BITSTREAM));
	return bs;
}

void
rfx_bitstream_put_bytes(RFX_BITSTREAM * bs, const unsigned char * bytes, int nbytes)
{
	bs->bytes = (unsigned char *) malloc(nbytes);
	memcpy(bs->bytes, bytes, nbytes);
	bs->nbytes = nbytes;
	bs->byte_pos = 0;
	bs->bits_left = 8;
}

unsigned int
rfx_bitstream_get_bits(RFX_BITSTREAM * bs, int nbits)
{
	unsigned int n = 0;
	int b;

	while (bs->byte_pos < bs->nbytes && nbits > 0)
	{
		b = nbits;
		if (b > bs->bits_left)
			b = bs->bits_left;
		if (n)
			n <<= b;
		n |= (bs->bytes[bs->byte_pos] >> (bs->bits_left - b)) & ((1 << b) - 1);
		bs->bits_left -= b;
		nbits -= b;
		if (bs->bits_left == 0)
		{
			bs->bits_left = 8;
			bs->byte_pos++;
		}
	}
	return n;
}

int
rfx_bitstream_eos(RFX_BITSTREAM * bs)
{
	return bs->byte_pos >= bs->nbytes;
}

int
rfx_bitstream_left(RFX_BITSTREAM * bs)
{
	if (bs->byte_pos >= bs->nbytes)
		return 0;
	return (bs->nbytes - bs->byte_pos - 1) * 8 + bs->bits_left;
}

void
rfx_bitstream_free(RFX_BITSTREAM * bs)
{
	if (bs->bytes)
		free(bs->bytes);
	free(bs);
}

