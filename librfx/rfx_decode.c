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

static void
rfx_decode_component(RLGR_MODE mode, const int * quantization_values, int half,
	const unsigned char * data, int size, int * buffer)
{
	rfx_rlgr_decode(mode, data, size, buffer, 4096);

	rfx_differential_decode(buffer + 4032, 64);

	rfx_quantization_decode(buffer, 1024, quantization_values[8]); /* HL1 */
	rfx_quantization_decode(buffer + 1024, 1024, quantization_values[7]); /* LH1 */
	rfx_quantization_decode(buffer + 2048, 1024, quantization_values[9]); /* HH1 */
	rfx_quantization_decode(buffer + 3072, 256, quantization_values[5]); /* HL2 */
	rfx_quantization_decode(buffer + 3328, 256, quantization_values[4]); /* LH2 */
	rfx_quantization_decode(buffer + 3584, 256, quantization_values[6]); /* HH2 */
	rfx_quantization_decode(buffer + 3840, 64, quantization_values[2]); /* HL3 */
	rfx_quantization_decode(buffer + 3904, 64, quantization_values[1]); /* LH3 */
	rfx_quantization_decode(buffer + 3868, 64, quantization_values[3]); /* HH3 */
	rfx_quantization_decode(buffer + 4032, 64, quantization_values[0]); /* LL3 */

	rfx_dwt_2d_decode(buffer + 3840, 8);
	rfx_dwt_2d_decode(buffer + 3072, 16);
	if (!half)
		rfx_dwt_2d_decode(buffer, 32);
}

unsigned char *
rfx_decode_yv12(RLGR_MODE mode,
	const unsigned char * y_data, int y_size, const int * y_quants,
	const unsigned char * cb_data, int cb_size, const int * cb_quants,
	const unsigned char * cr_data, int cr_size, const int * cr_quants)
{
	unsigned char * output;
	unsigned char * dst;
	int buffer[4096];
	int i;

	/* 64x64 YV12 image has Y=64x64=4096, U,V=32x32=1024 */
	output = (unsigned char *) malloc(4096 + 1024 + 1024);
	dst = output;

	rfx_decode_component(mode, y_quants, 0, y_data, y_size, buffer);
	for (i = 0; i < 4096; i++)
		*dst++ = (unsigned char) (buffer[i] + 128);

	/* For Cb and Cr, we only need LL1 (which is half-size) to construct YV12 pixel format */
	rfx_decode_component(mode, cb_quants, 1, cb_data, cb_size, buffer);
	for (i = 3072; i < 4096; i++)
		*dst++ = (unsigned char) (buffer[i]);

	rfx_decode_component(mode, cr_quants, 1, cr_data, cr_size, buffer);
	for (i = 3072; i < 4096; i++)
		*dst++ = (unsigned char) (buffer[i]);

	return output;
}

