/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI NEON Optimizations

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

#ifndef __GDI_NEON_H
#define __GDI_NEON_H

#include "gdi.h"

void gdi_init_neon(GDI* gdi);

#ifndef GDI_INIT_SIMD
#define GDI_INIT_SIMD(_gdi) gdi_init_neon(_gdi)
#endif

#endif /* __GDI_NEON_H */
