/*
   FreeRDP: A Remote Desktop Protocol client.
   RemoteFX Codec Library - SSE2 Optimizations

   Copyright 2011 Stephen Erisman

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

#include "rfx_sse.h"

#include "rfx_sse2.h"

#define CACHE_LINE_BYTES	64

static __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_prefetch_buffer(char * buffer, int num_bytes)
{
	__m128i * buf = (__m128i*) buffer;
	int i;
	for (i = 0; i < (num_bytes / sizeof(__m128i)); i+=(CACHE_LINE_BYTES / sizeof(__m128i)))
	{
		_mm_prefetch((char*)(&buf[i]), _MM_HINT_NTA);
	}
}

void rfx_decode_YCbCr_to_RGB_SSE2(sint16 * y_r_buffer, sint16 * cb_g_buffer, sint16 * cr_b_buffer)
{	
	__m128i zero = _mm_setzero_si128();
	__m128i max = _mm_set1_epi16(255);

	__m128i * y_r_buf = (__m128i*) y_r_buffer;
	__m128i * cb_g_buf = (__m128i*) cb_g_buffer;
	__m128i * cr_b_buf = (__m128i*) cr_b_buffer;

	int i;
	for (i = 0; i < (4096 * sizeof(sint16) / sizeof(__m128i)); i+=(CACHE_LINE_BYTES / sizeof(__m128i)))
	{
		_mm_prefetch((char*)(&y_r_buf[i]), _MM_HINT_NTA);
		_mm_prefetch((char*)(&cb_g_buf[i]), _MM_HINT_NTA);
		_mm_prefetch((char*)(&cr_b_buf[i]), _MM_HINT_NTA);
	}
	for (i = 0; i < (4096 * sizeof(sint16) / sizeof(__m128i)); i++)
	{
		// y = y_r_buf[i] + 128;
		__m128i y = _mm_load_si128(&y_r_buf[i]);
		y = _mm_add_epi16(y, _mm_set1_epi16(128));
		
		// cr = cr_b_buf[i];
		__m128i cr = _mm_load_si128(&cr_b_buf[i]);
			
		// r = between(y + cr + (cr >> 2) + (cr >> 3) + (cr >> 5), 0, 255);
		__m128i r = _mm_add_epi16(y, cr);
		r = _mm_add_epi16(r, _mm_srai_epi16(cr, 2));
		r = _mm_add_epi16(r, _mm_srai_epi16(cr, 3));
		r = _mm_add_epi16(r, _mm_srai_epi16(cr, 5));
		r = _mm_between_epi16(r, zero, max);
		_mm_store_si128(&y_r_buf[i], r);

		// cb = cb_g_buf[i];
		__m128i cb = _mm_load_si128(&cb_g_buf[i]);

		// g = between(y - (cb >> 2) - (cb >> 4) - (cb >> 5) - (cr >> 1) - (cr >> 3) - (cr >> 4) - (cr >> 5), 0, 255);
		__m128i g = _mm_sub_epi16(y, _mm_srai_epi16(cb, 2));
		g = _mm_sub_epi16(g, _mm_srai_epi16(cb, 4));
		g = _mm_sub_epi16(g, _mm_srai_epi16(cb, 5));
		g = _mm_sub_epi16(g, _mm_srai_epi16(cr, 1));
		g = _mm_sub_epi16(g, _mm_srai_epi16(cr, 3));
		g = _mm_sub_epi16(g, _mm_srai_epi16(cr, 4));
		g = _mm_sub_epi16(g, _mm_srai_epi16(cr, 5));
		g = _mm_between_epi16(g, zero, max);
		_mm_store_si128(&cb_g_buf[i], g);		
		
		// b = between(y + cb + (cb >> 1) + (cb >> 2) + (cb >> 6), 0, 255);
		__m128i b = _mm_add_epi16(y, cb);
		b = _mm_add_epi16(b, _mm_srai_epi16(cb, 1));
		b = _mm_add_epi16(b, _mm_srai_epi16(cb, 2));
		b = _mm_add_epi16(b, _mm_srai_epi16(cb, 6));
		b = _mm_between_epi16(b, zero, max);
		_mm_store_si128(&cr_b_buf[i], b);
	}
}

static __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))
rfx_quantization_decode_block_SSE2(sint16 * buffer, const int buffer_size, const uint32 factor)
{
	int shift = factor-6;
	if (shift <= 0)
		return;
	
	__m128i a;
	__m128i * ptr = (__m128i*) buffer;
	__m128i * buf_end = (__m128i*) (buffer + buffer_size);
	do
	{
		a = _mm_load_si128(ptr);
		a = _mm_slli_epi16(a, shift);
		_mm_store_si128(ptr, a);

		ptr++;
	} while(ptr < buf_end);
}

void rfx_quantization_decode_SSE2(sint16 * buffer, const uint32 * quantization_values)
{
	_mm_prefetch_buffer((char *) buffer, 4096 * sizeof(sint16));

	rfx_quantization_decode_block_SSE2(buffer, 1024, quantization_values[8]); // HL1
	rfx_quantization_decode_block_SSE2(buffer + 1024, 1024, quantization_values[7]); // LH1
	rfx_quantization_decode_block_SSE2(buffer + 2048, 1024, quantization_values[9]); // HH1
	rfx_quantization_decode_block_SSE2(buffer + 3072, 256, quantization_values[5]); // HL2
	rfx_quantization_decode_block_SSE2(buffer + 3328, 256, quantization_values[4]); // LH2
	rfx_quantization_decode_block_SSE2(buffer + 3584, 256, quantization_values[6]); // HH2
	rfx_quantization_decode_block_SSE2(buffer + 3840, 64, quantization_values[2]); // HL3
	rfx_quantization_decode_block_SSE2(buffer + 3904, 64, quantization_values[1]); // LH3
	rfx_quantization_decode_block_SSE2(buffer + 3868, 64, quantization_values[3]); // HH3
	rfx_quantization_decode_block_SSE2(buffer + 4032, 64, quantization_values[0]); // LL3	
}

static __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))
rfx_dwt_2d_decode_block_horiz_SSE2(sint16 * l, sint16 * h, sint16 * dst, int subband_width)
{
	int y, n;		
	sint16 * l_ptr = l;
	sint16 * h_ptr = h;
	sint16 * dst_ptr = dst;

	for (y = 0; y < subband_width; y++)
	{
		/* Even coefficients */
		for (n = 0; n < subband_width; n+=8)
		{
			// dst[2n] = l[n] - ((h[n-1] + h[n] + 1) >> 1);
			
			__m128i l_n = _mm_load_si128((__m128i*) l_ptr);

			__m128i h_n = _mm_load_si128((__m128i*) h_ptr);
			__m128i h_n_m = _mm_loadu_si128((__m128i*) (h_ptr - 1));
			if (n == 0)
			{
				int first = _mm_extract_epi16(h_n_m, 1);
				h_n_m = _mm_insert_epi16(h_n_m, first, 0);
			}
			
			__m128i tmp_n = _mm_add_epi16(h_n, h_n_m);
			tmp_n = _mm_add_epi16(tmp_n, _mm_set1_epi16(1));
			tmp_n = _mm_srai_epi16(tmp_n, 1);
			
			__m128i dst_n = _mm_sub_epi16(l_n, tmp_n);
			
			_mm_store_si128((__m128i*) l_ptr, dst_n);
			
			l_ptr+=8;
			h_ptr+=8;
		}		
		l_ptr -= subband_width;
		h_ptr -= subband_width;
		
		/* Odd coefficients */
		for (n = 0; n < subband_width; n+=8)
		{
			// dst[2n + 1] = (h[n] << 1) + ((dst[2n] + dst[2n + 2]) >> 1);
			
			__m128i h_n = _mm_load_si128((__m128i*) h_ptr);
			
			h_n = _mm_slli_epi16(h_n, 1);
			
			__m128i dst_n = _mm_load_si128((__m128i*) (l_ptr));
			__m128i dst_n_p = _mm_loadu_si128((__m128i*) (l_ptr + 1));
			if (n == subband_width - 8)
			{
				int last = _mm_extract_epi16(dst_n_p, 6);
				dst_n_p = _mm_insert_epi16(dst_n_p, last, 7);
			}
			
			__m128i tmp_n = _mm_add_epi16(dst_n_p, dst_n);
			tmp_n = _mm_srai_epi16(tmp_n, 1);
			
			tmp_n = _mm_add_epi16(tmp_n, h_n);
			
			__m128i dst1 = _mm_unpacklo_epi16(dst_n, tmp_n);
			__m128i dst2 = _mm_unpackhi_epi16(dst_n, tmp_n);
			
			_mm_store_si128((__m128i*) dst_ptr, dst1);
			_mm_store_si128((__m128i*) (dst_ptr + 8), dst2);
			
			l_ptr+=8;
			h_ptr+=8;
			dst_ptr+=16;
		}
	}
}

static __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))
rfx_dwt_2d_decode_block_SSE2(sint16 * buffer, sint16 * idwt, int subband_width)
{
	sint16 * dst, * l, * h;
	sint16 * l_dst, * h_dst;
	sint16 * hl, * lh, * hh, * ll;
	int total_width;
	int x, y;
	int n;

	_mm_prefetch_buffer((char *) idwt, subband_width * 4 * sizeof(sint16));

	total_width = subband_width + subband_width;

	/* Inverse DWT in horizontal direction, results in 2 sub-bands in L, H order in tmp buffer idwt. */
	/* The 4 sub-bands are stored in HL(0), LH(1), HH(2), LL(3) order. */
	/* The lower part L uses LL(3) and HL(0). */
	/* The higher part H uses LH(1) and HH(2). */

	ll = buffer + subband_width * subband_width * 3;
	hl = buffer;
	l_dst = idwt;

	rfx_dwt_2d_decode_block_horiz_SSE2(ll, hl, l_dst, subband_width);

	lh = buffer + subband_width * subband_width;
	hh = buffer + subband_width * subband_width * 2;
	h_dst = idwt + subband_width * subband_width * 2;
	
	rfx_dwt_2d_decode_block_horiz_SSE2(lh, hh, h_dst, subband_width);

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
}

void rfx_dwt_2d_decode_SSE2(sint16 * buffer, sint16 * dwt_buffer_8, sint16 * dwt_buffer_16, sint16 * dwt_buffer_32)
{
	_mm_prefetch_buffer((char *) buffer, 4096 * sizeof(sint16));
	
	rfx_dwt_2d_decode_block_SSE2(buffer + 3840, dwt_buffer_8, 8);
	rfx_dwt_2d_decode_block_SSE2(buffer + 3072, dwt_buffer_16, 16);
	rfx_dwt_2d_decode_block_SSE2(buffer, dwt_buffer_32, 32);
}
