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

#ifndef __RFX_SSE2_H
#define __RFX_SSE2_H

#include <freerdp/rfx.h>

void rfx_decode_YCbCr_to_RGB_SSE2(sint16 * y_r_buffer, sint16 * cb_g_buffer, sint16 * cr_b_buffer);
void rfx_encode_RGB_to_YCbCr_SSE2(sint16 * y_r_buffer, sint16 * cb_g_buffer, sint16 * cr_b_buffer);
void rfx_quantization_decode_SSE2(sint16 * buffer, const uint32 * quantization_values);
void rfx_dwt_2d_decode_SSE2(sint16 * buffer, sint16 * dwt_buffer_8, sint16 * dwt_buffer_16, sint16 * dwt_buffer_32);

#endif /* __RFX_SSE2_H */
