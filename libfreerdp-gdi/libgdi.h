/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Library

   Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __LIBGDI_H
#define __LIBGDI_H

#ifdef WITH_SSE
#include "gdi_sse.h"
#endif

#ifdef WITH_NEON
#include "gdi_neon.h"
#endif

#ifndef GDI_INIT_SIMD
#define GDI_INIT_SIMD(_gdi) do { } while (0)
#endif

#endif /* __LIBGDI_H */
