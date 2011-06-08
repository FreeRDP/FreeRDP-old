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

void
rfx_sse2_init(RFX_CONTEXT * context);

void
rfx_sse2_decode_YCbCr_to_RGB(uint32 * y_r_buf, uint32 * cb_g_buf, uint32 * cr_b_buf);

#endif /* __RFX_SSE2_H */
