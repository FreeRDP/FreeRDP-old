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

#ifndef __RFX_DECODE_H
#define __RFX_DECODE_H

#include <freerdp/rfx.h>

unsigned char *
rfx_decode_rgb(RFX_CONTEXT * context,
	const unsigned char * y_data, int y_size, const int * y_quants,
	const unsigned char * cb_data, int cb_size, const int * cb_quants,
	const unsigned char * cr_data, int cr_size, const int * cr_quants);

#endif

