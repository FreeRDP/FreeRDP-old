/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Drawing Functions

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

#ifndef __GDI_DRAWING_H
#define __GDI_DRAWING_H

#include "gdi.h"

int GetROP2(HDC hdc);
int SetROP2(HDC hdc, int fnDrawMode);
COLORREF GetBkColor(HDC hdc);
COLORREF SetBkColor(HDC hdc, COLORREF crColor);
int GetBkMode(HDC hdc);
int SetBkMode(HDC hdc, int iBkMode);
COLORREF SetTextColor(HDC hdc, COLORREF crColor);

#endif /* __GDI_DRAWING_H */
