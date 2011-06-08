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

void rfx_decode_YCbCr_to_RGB_SSE2(uint32 * y_r_buffer, uint32 * cb_g_buffer, uint32 * cr_b_buffer)
{
	__m128 y_add = _mm_set_ps1(128.0f);
	__m128 r_cr_t = _mm_set_ps1(1.403f);
	__m128 g_cb_t = _mm_set_ps1(-0.344f);
	__m128 g_cr_t = _mm_set_ps1(-0.714f);
	__m128 b_cb_t = _mm_set_ps1(1.77f);

	__m128 min = _mm_set_ps1(0.0f);
	__m128 max = _mm_set_ps1(255.0f);

	__m128 y, cb, cr;
	__m128 r, g, b, tmp;	

	__m128i * y_r_buf = (__m128i*) y_r_buffer;
	__m128i * cb_g_buf = (__m128i*) cb_g_buffer;
	__m128i * cr_b_buf = (__m128i*) cr_b_buffer;

	int i;
	for (i = 0; i < (4096 / 4); i++)
	{
		y = _mm_cvtepi32_ps(*y_r_buf);
		cb = _mm_cvtepi32_ps(*cb_g_buf);
		cr = _mm_cvtepi32_ps(*cr_b_buf);

		/* y = y + 128 */
		y = _mm_add_ps(y, y_add);

		/* r = between(y + (cr * 1.403), 0, 255) */
		r = _mm_mul_ps(cr, r_cr_t);
		r = _mm_add_ps(r, y);
		r = _mm_between_ps(r, min, max);
		_mm_cvtps_epi32_and_store(y_r_buf, r);

		/* g = between(y + (cb * -0.344) + (cr * -0.714), 0, 255) */
		g = _mm_mul_ps(cb, g_cb_t);
		tmp = _mm_mul_ps(cr, g_cr_t);
		g = _mm_add_ps(g, tmp);
		g = _mm_add_ps(g, y);
		g = _mm_between_ps(g, min, max);
		_mm_cvtps_epi32_and_store(cb_g_buf, g);

		/* b = between(y + (cb * 1.77), 0, 255) */
		b = _mm_mul_ps(cb, b_cb_t);
		b = _mm_add_ps(b, y);
		b = _mm_between_ps(b, min, max);
		_mm_cvtps_epi32_and_store(cr_b_buf, b);

		y_r_buf++;
		cb_g_buf++;
		cr_b_buf++;
	}
}

