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

#ifndef __RFX_SSE2_PRIVATE_H
#define __RFX_SSE2_PRIVATE_H

#include "xmmintrin.h"
#include "emmintrin.h"

/* TODO: move these SSE helpers to a higher-up utility include file */

static __inline __m128 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_between_ps (__m128 val, __m128 min, __m128 max)
{
	__m128 ret;
	ret = _mm_max_ps(val, min);
	return _mm_min_ps(ret, max);
}

static __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_cvtps_epi32_and_store (__m128i * loc, __m128 val)
{
	__m128i tmp;
	tmp = _mm_cvtps_epi32(val);
	_mm_stream_si128(loc, tmp);
}

#endif /* __RFX_SSE2_PRIVATE_H */
