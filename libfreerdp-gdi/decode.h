/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI RemoteFX Decoder

   Copyright 2010-2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __DECODE_H
#define __DECODE_H

#include <freerdp/freerdp.h>
#include <freerdp/rfx.h>

#include "gdi.h"

void gdi_decode_bitmap_data(GDI *gdi, int x, int y, uint8 * data, uint32 length);
void gdi_decode_data(GDI *gdi, uint8 * data, int size);

#endif /* __DECODE_H */
