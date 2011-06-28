/*
   FreeRDP: A Remote Desktop Protocol client.
   RemoteFX Codec Library

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

#ifndef __LIBRFX_H
#define __LIBRFX_H

#include <freerdp/utils/debug.h>

#ifdef WITH_DEBUG_RFX
#define DEBUG_RFX(fmt, ...) DEBUG_CLASS(RFX, fmt, ## __VA_ARGS__)
#else
#define DEBUG_RFX(fmt, ...) DEBUG_NULL(fmt, ## __VA_ARGS__)
#endif

#ifdef WITH_SSE
#include "rfx_sse.h"
#endif

#ifdef WITH_NEON
#include "rfx_neon.h"
#endif

#ifndef RFX_INIT_SIMD
#define RFX_INIT_SIMD(_rfx_context) do { } while (0)
#endif

#endif /* __LIBRFX_H */
