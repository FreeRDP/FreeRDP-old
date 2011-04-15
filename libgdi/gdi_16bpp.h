/*
   FreeRDP: A Remote Desktop Protocol client.
   16bpp Internal Buffer Routines

   Copyright 2010 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#include <freerdp/freerdp.h>
#include "gdi.h"
#include "gdi_color.h"

int FillRect_16bpp(HDC hdc, HRECT rect, HBRUSH hbr);
int BitBlt_16bpp(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, int rop);
int PatBlt_16bpp(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight, int rop);
int LineTo_16bpp(HDC hdc, int nXEnd, int nYEnd);
