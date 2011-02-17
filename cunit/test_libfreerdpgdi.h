/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP GDI Adaption Layer Unit Tests

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

#include "test_freerdp.h"

int init_libfreerdpgdi_suite(void);
int clean_libfreerdpgdi_suite(void);
int add_libfreerdpgdi_suite(void);

void test_GetDC(void);
void test_CreateCompatibleDC(void);
void test_CreateBitmap(void);
void test_CreateCompatibleBitmap(void);
void test_CreatePen(void);
void test_CreatePalette(void);
void test_CreateSolidBrush(void);
void test_CreatePatternBrush(void);
void test_CreateRectRgn(void);
void test_CreateRect(void);
void test_GetPixel(void);
void test_SetPixel(void);
void test_SetROP2(void);
void test_MoveTo(void);
void test_PtInRect(void);
void test_FillRect(void);
void test_BitBlt_32bpp(void);
void test_BitBlt_16bpp(void);
void test_ClipCoords(void);
void test_InvalidateRegion(void);
