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

void rfx_decode_YCbCr_to_RGB_SSE2(uint16 * y_r_buffer, uint16 * cb_g_buffer, uint16 * cr_b_buffer)
{	
	__m128i zero = _mm_setzero_si128();
	__m128i max = _mm_set1_epi16(255);

	__m128i * y_r_buf = (__m128i*) y_r_buffer;
	__m128i * cb_g_buf = (__m128i*) cb_g_buffer;
	__m128i * cr_b_buf = (__m128i*) cr_b_buffer;

	int i;
	for (i = 0; i < (4096 * sizeof(uint16) / sizeof(__m128i)); i+=(CACHE_LINE_BYTES / sizeof(__m128i)))
	{
		_mm_prefetch((char*)(&y_r_buf[i]), _MM_HINT_NTA);
		_mm_prefetch((char*)(&cb_g_buf[i]), _MM_HINT_NTA);
		_mm_prefetch((char*)(&cr_b_buf[i]), _MM_HINT_NTA);
	}
	for (i = 0; i < (4096 * sizeof(uint16) / sizeof(__m128i)); i++)
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
rfx_quantization_decode_block_SSE2(uint16 * buffer, const int buffer_size, const uint32 factor)
{
	if (factor <= 6)
		return;
	
	__m128i * buf = (__m128i*) buffer;
	
	int i;
	for (i = 0; i < (buffer_size * sizeof(uint16) / sizeof(__m128i)); i++)
	{
		// buf[i] <<= (factor - 6);
		__m128i a = _mm_load_si128(&buf[i]);
		a = _mm_slli_epi16(a, factor-6);
		_mm_store_si128(&buf[i], a);
	}
}

void rfx_quantization_decode_SSE2(uint16 * buffer, const uint32 * quantization_values)
{
	__m128i * buf = (__m128i*) buffer;
	
	int i;
	for (i = 0; i < (4096 * sizeof(uint16) / sizeof(__m128i)); i+=(CACHE_LINE_BYTES / sizeof(__m128i)))
	{
		_mm_prefetch((char*)(&buf[i]), _MM_HINT_NTA);
	}
	_mm_prefetch((char*)(&quantization_values[0]), _MM_HINT_NTA);
		
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
