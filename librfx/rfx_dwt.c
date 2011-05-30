/*
   FreeRDP: A Remote Desktop Protocol client.
   RemoteFX Codec Library - DWT

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
#include "rfx_dwt.h"

void
rfx_dwt_2d_decode(int * buffer, int subband_width)
{
	int * idwt;
	int * dst;
	int * l;
	int * h;
	int total_width;
	int x, y;
	int n;

	/* The 4 sub-bands are stored in HL(0), LH(1), HH(2), LL(3) order. */

	idwt = (int *) malloc(subband_width * subband_width * 4 * sizeof(int));
	total_width =  subband_width << 1;

	/* Inverse DWT in horizontal direction, results in 2 sub-bands in L, H order in tmp buffer idwt. */

	/* The lower part L uses LL(3) and HL(0). */

	l = buffer + subband_width * subband_width * 3;
	h = buffer;
	dst = idwt;

	for (y = 0; y < subband_width; y++)
	{
		/* Even coefficients */
		for (n = 0; n < subband_width; n++)
		{
			x = n << 1;
			dst[x] = l[n] - ((h[n > 0 ? n - 1 : n] + h[n] + 1) >> 1);
		}

		/* Odd coefficients */
		for (n = 0; n < subband_width; n++)
		{
			x = n << 1;
			dst[x + 1] = (h[n] << 1) + ((dst[x] + dst[n < subband_width - 1 ? x + 2 : x]) >> 1);
		}

		l += subband_width;
		h += subband_width;
		dst += total_width;
	}

	/* The higher part H uses LH(1) and HH(2). */

	l = buffer + subband_width * subband_width;
	h = buffer + subband_width * subband_width * 2;

	for (y = 0; y < subband_width; y++)
	{
		/* Even coefficients */
		for (n = 0; n < subband_width; n++)
		{
			x = n << 1;
			dst[x] = l[n] - ((h[n > 0 ? n - 1 : n] + h[n] + 1) >> 1);
		}

		/* Odd coefficients */
		for (n = 0; n < subband_width; n++)
		{
			x = n << 1;
			dst[x + 1] = (h[n] << 1) + ((dst[x] + dst[n < subband_width - 1 ? x + 2 : x]) >> 1);
		}

		l += subband_width;
		h += subband_width;
		dst += total_width;
	}

	/* Inverse DWT in vertical direction, results are stored in original buffer. */
	for (x = 0; x < total_width; x++)
	{
		/* Even coefficients */
		for (n = 0; n < subband_width; n++)
		{
			y = n << 1;
			dst = buffer + y * total_width + x;
			l = idwt + n * total_width + x;
			h = l + subband_width * total_width;
			dst[0] = *l - (((n > 0 ? *(h - total_width) : *h) + (*h) + 1) >> 1);
		}

		/* Odd coefficients */
		for (n = 0; n < subband_width; n++)
		{
			y = n << 1;
			dst = buffer + y * total_width + x;
			l = idwt + n * total_width + x;
			h = l + subband_width * total_width;
			dst[total_width] = (*h << 1) + ((dst[0] + dst[n < subband_width - 1 ? 2 * total_width : 0]) >> 1);
		}
	}

	free(idwt);
}

