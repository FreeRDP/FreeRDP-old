/*
   FreeRDP: A Remote Desktop Protocol client.
   RemoteFX Codec Library - Decode

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
#include "rfx_rlgr.h"
#include "rfx_differential.h"
#include "rfx_quantization.h"
#include "rfx_dwt.h"

#include "rfx_decode.h"

#define MINMAX(_v,_l,_h) ((_v) < (_l) ? (_l) : ((_v) > (_h) ? (_h) : (_v)))

void
rfx_decode_YCbCr_to_RGB(sint16 * y_r_buf, sint16 * cb_g_buf, sint16 * cr_b_buf)
{
	sint16 y, cb, cr;
	sint16 r, g, b;

	int i;
	for (i = 0; i < 4096; i++)
	{
		y = y_r_buf[i] + 128;
		cb = cb_g_buf[i];
		cr = cr_b_buf[i];
		r = (y + cr + (cr >> 2) + (cr >> 3) + (cr >> 5));
		y_r_buf[i] = MINMAX(r, 0, 255);
		g = (y - ((cb >> 2) + (cb >> 4) + (cb >> 5)) - ((cr >> 1) + (cr >> 3) + (cr >> 4) + (cr >> 5)));
		cb_g_buf[i] = MINMAX(g, 0, 255);
		b = (y + cb + (cb >> 1) + (cb >> 2) + (cb >> 6));
		cr_b_buf[i] = MINMAX(b, 0, 255);
	}
}

static void
rfx_decode_component(RFX_CONTEXT * context, const uint32 * quantization_values,
	const uint8 * data, int size, sint16 * buffer)
{
	PROFILER_ENTER(context->prof_rfx_decode_component);

	PROFILER_ENTER(context->prof_rfx_rlgr_decode);
		rfx_rlgr_decode(context->mode, data, size, buffer, 4096);
	PROFILER_EXIT(context->prof_rfx_rlgr_decode);

	PROFILER_ENTER(context->prof_rfx_differential_decode);
		rfx_differential_decode(buffer + 4032, 64);
	PROFILER_EXIT(context->prof_rfx_differential_decode);

	PROFILER_ENTER(context->prof_rfx_quantization_decode);
		context->quantization_decode(buffer, quantization_values);
	PROFILER_EXIT(context->prof_rfx_quantization_decode);

	PROFILER_ENTER(context->prof_rfx_dwt_2d_decode);
		context->dwt_2d_decode(buffer, context->dwt_buffer_8, context->dwt_buffer_16, context->dwt_buffer_32);
	PROFILER_EXIT(context->prof_rfx_dwt_2d_decode);

	PROFILER_EXIT(context->prof_rfx_decode_component);
}

uint8*
rfx_decode_rgb(RFX_CONTEXT * context,
	const uint8 * y_data, int y_size, const uint32 * y_quants,
	const uint8 * cb_data, int cb_size, const uint32 * cb_quants,
	const uint8 * cr_data, int cr_size, const uint32 * cr_quants, uint8* rgb_buffer)
{
	int i;
	uint8 * dst;
	sint16 r, g, b;

	PROFILER_ENTER(context->prof_rfx_decode_rgb);

	dst = rgb_buffer;
	rfx_decode_component(context, y_quants, y_data, y_size, context->y_r_buffer);
	rfx_decode_component(context, cb_quants, cb_data, cb_size, context->cb_g_buffer);
	rfx_decode_component(context, cr_quants, cr_data, cr_size, context->cr_b_buffer);

	PROFILER_ENTER(context->prof_rfx_decode_YCbCr_to_RGB);
		context->decode_YCbCr_to_RGB(context->y_r_buffer, context->cb_g_buffer, context->cr_b_buffer);
	PROFILER_EXIT(context->prof_rfx_decode_YCbCr_to_RGB);

	for (i = 0; i < 4096; i++)
	{
		r = context->y_r_buffer[i];
		g = context->cb_g_buffer[i];
		b = context->cr_b_buffer[i];
		switch (context->pixel_format)
		{
			case RFX_PIXEL_FORMAT_BGRA:
				*dst++ = (uint8) (b);
				*dst++ = (uint8) (g);
				*dst++ = (uint8) (r);
				*dst++ = 0xFF;
				break;
			case RFX_PIXEL_FORMAT_RGBA:
				*dst++ = (uint8) (r);
				*dst++ = (uint8) (g);
				*dst++ = (uint8) (b);
				*dst++ = 0xFF;
				break;
			case RFX_PIXEL_FORMAT_BGR:
				*dst++ = (uint8) (b);
				*dst++ = (uint8) (g);
				*dst++ = (uint8) (r);
				break;
			case RFX_PIXEL_FORMAT_RGB:
				*dst++ = (uint8) (r);
				*dst++ = (uint8) (g);
				*dst++ = (uint8) (b);
				break;
			default:
				break;
		}
	}
	PROFILER_EXIT(context->prof_rfx_decode_rgb);
	return rgb_buffer;
}
