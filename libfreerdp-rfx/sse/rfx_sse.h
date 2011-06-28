/*
   FreeRDP: A Remote Desktop Protocol client.
   RemoteFX Codec Library - SSE Optimizations

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

#ifndef __RFX_SSE_H
#define __RFX_SSE_H

#include "librfx.h"
#include "xmmintrin.h"
#include "emmintrin.h"
#include <freerdp/rfx.h>

void rfx_init_sse(RFX_CONTEXT * context);

#ifndef RFX_INIT_SIMD
#define RFX_INIT_SIMD(_rfx_context) rfx_init_sse(_rfx_context)
#endif

static __inline __m128i __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_between_epi16 (__m128i val, __m128i min, __m128i max)
{
	__m128i ret;
	ret = _mm_max_epi16(val, min);
	return _mm_min_epi16(ret, max);
}

#endif /* __RFX_SSE_H */
