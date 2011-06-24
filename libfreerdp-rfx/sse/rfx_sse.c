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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rfx_sse2.h"
#include "rfx_sse.h"

void rfx_init_sse(RFX_CONTEXT * context)
{
		DEBUG_RFX("Using SSE2 optimizations");

		IF_PROFILER(context->prof_rfx_decode_YCbCr_to_RGB->name = "rfx_decode_YCbCr_to_RGB_SSE2");
		IF_PROFILER(context->prof_rfx_encode_RGB_to_YCbCr->name = "rfx_encode_RGB_to_YCbCr_SSE2");
		IF_PROFILER(context->prof_rfx_quantization_decode->name = "rfx_quantization_decode_SSE2");
		IF_PROFILER(context->prof_rfx_quantization_encode->name = "rfx_quantization_encode_SSE2");
		IF_PROFILER(context->prof_rfx_dwt_2d_decode->name = "rfx_dwt_2d_decode_SSE2");
		IF_PROFILER(context->prof_rfx_dwt_2d_encode->name = "rfx_dwt_2d_encode_SSE2");
		
		context->decode_YCbCr_to_RGB = rfx_decode_YCbCr_to_RGB_SSE2;
		context->encode_RGB_to_YCbCr = rfx_encode_RGB_to_YCbCr_SSE2;
		context->quantization_decode = rfx_quantization_decode_SSE2;
		context->quantization_encode = rfx_quantization_encode_SSE2;
		context->dwt_2d_decode = rfx_dwt_2d_decode_SSE2;
		context->dwt_2d_encode = rfx_dwt_2d_encode_SSE2;
}
