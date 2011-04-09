/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP GDI Adaption Layer

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <freerdp/freerdp.h>

#include "gdi_color.h"
#include "gdi_window.h"

#ifndef __LIBFREERDPGDI_H
#define __LIBFREERDPGDI_H

/* For more information, see [MS-RDPEGDI].pdf */

/* Binary Raster Operations (ROP2) */
#define R2_BLACK		0x01  /* 0    */
#define R2_NOTMERGEPEN		0x02  /* DPon */
#define R2_MASKNOTPEN		0x03  /* DPna */
#define R2_NOTCOPYPEN		0x04  /* PN   */
#define R2_MASKPENNOT		0x05  /* PDna */
#define R2_NOT			0x06  /* Dn   */
#define R2_XORPEN		0x07  /* DPx  */
#define R2_NOTMASKPEN		0x08  /* DPan */
#define R2_MASKPEN		0x09  /* DPa  */
#define R2_NOTXORPEN		0x0A  /* DPxn */
#define R2_NOP			0x0B  /* D    */
#define R2_MERGENOTPEN		0x0C  /* DPno */
#define R2_COPYPEN		0x0D  /* P    */
#define R2_MERGEPENNOT		0x0E  /* PDno */
#define R2_MERGEPEN		0x0F  /* DPo  */
#define R2_WHITE		0x10  /* 1    */

/* Ternary Raster Operations (ROP3) */
#define SRCCOPY			0x00CC0020 /* D = S	*/
#define SRCPAINT		0x00EE0086 /* D = S | D	*/
#define SRCAND			0x008800C6 /* D = S & D	*/
#define SRCINVERT		0x00660046 /* D = S ^ D	*/
#define SRCERASE		0x00440328 /* D = S & ~D */
#define NOTSRCCOPY		0x00330008 /* D = ~S */
#define NOTSRCERASE		0x001100A6 /* D = ~S & ~D */
#define MERGECOPY		0x00C000CA /* D = S & P	*/
#define MERGEPAINT		0x00BB0226 /* D = ~S | D */
#define PATCOPY			0x00F00021 /* D = P	*/
#define PATPAINT		0x00FB0A09 /* D = D | (P | ~S) */
#define PATINVERT		0x005A0049 /* D = P ^ D	*/
#define DSTINVERT		0x00550009 /* D = ~D */
#define BLACKNESS		0x00000042 /* D = 0 */
#define WHITENESS		0x00FF0062 /* D = 1 */
#define DSPDxax			0x00E20746 /* D = (S & P) | (~S & D) */
#define SPna			0x000C0324 /* D = S & ~P */
#define DSna			0x00220326 /* D = D & ~S */

/* Brush Styles */
#define BS_SOLID		0x00
#define BS_NULL			0x01
#define BS_HATCHED		0x02
#define BS_PATTERN		0x03

/* Hatch Patterns */
#define HS_HORIZONTAL		0x00
#define HS_VERTICAL		0x01
#define HS_FDIAGONAL		0x02
#define HS_BDIAGONAL		0x03
#define HS_CROSS		0x04
#define HS_DIAGCROSS		0x05

/* Pen Styles */
#define PS_SOLID		0x00
#define PS_DASH			0x01
#define PS_NULL			0x05

/* GDI Object Types */
#define GDIOBJ_BITMAP		0x00
#define GDIOBJ_PEN		0x01
#define GDIOBJ_PALETTE		0x02
#define GDIOBJ_BRUSH		0x03
#define GDIOBJ_RECT		0x04
#define GDIOBJ_REGION		0x04

/* Background Modes */
#define OPAQUE			0x00000001
#define TRANSPARENT		0x00000002

struct _PIXEL
{
	int red;
	int green;
	int blue;
	int alpha;
};
typedef struct _PIXEL PIXEL;

struct _GDIOBJ
{
	unsigned char objectType;
};
typedef struct _GDIOBJ GDIOBJ;
typedef GDIOBJ* HGDIOBJ;

/* RGB encoded as 0x00BBGGRR */
typedef unsigned int COLORREF;
typedef COLORREF* LPCOLORREF;

struct _RECT
{
	unsigned char objectType;
	unsigned int left;
	unsigned int top;
	unsigned int right;
	unsigned int bottom;
};
typedef struct _RECT RECT;
typedef RECT* HRECT;

struct _RGN
{
	unsigned char objectType;
	int x; /* left */
	int y; /* top */
	int w; /* width */
	int h; /* height */
	int null; /* null region */
};
typedef struct _RGN RGN;
typedef RGN* HRGN;

struct _BITMAP
{
	unsigned char objectType;
	unsigned int bytesPerPixel;
	unsigned int bitsPerPixel;
	unsigned int width;
	unsigned int height;
	unsigned int scanline;
	uint8* data;
};
typedef struct _BITMAP BITMAP;
typedef BITMAP* HBITMAP;

struct _PEN
{
	unsigned char objectType;
	unsigned int style;
	unsigned int width;
	unsigned int posX;
	unsigned int posY;
	COLORREF color;
};
typedef struct _PEN PEN;
typedef PEN* HPEN;

struct _PALETTEENTRY
{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};
typedef struct _PALETTEENTRY PALETTEENTRY;

struct _LOGPALETTE
{
	unsigned int count;
	PALETTEENTRY *entries;
};
typedef struct _LOGPALETTE LOGPALETTE;

struct _PALETTE
{
	unsigned char objectType;
	LOGPALETTE *logicalPalette;
};
typedef struct _PALETTE PALETTE;
typedef PALETTE* HPALETTE;

struct _POINT
{
	unsigned int x;
	unsigned int y;
};
typedef struct _POINT POINT;
typedef POINT* HPOINT;

struct _BRUSH
{
	unsigned char objectType;
	unsigned int style;
	HBITMAP pattern;
	COLORREF color;
};
typedef struct _BRUSH BRUSH;
typedef BRUSH* HBRUSH;

struct _WND
{
	HRGN invalid;
};
typedef struct _WND WND;
typedef WND* HWND;

struct _DC
{
	HGDIOBJ selectedObject;
	unsigned int bytesPerPixel;
	unsigned int bitsPerPixel;
	COLORREF bkColor;
	COLORREF textColor;
	HBRUSH brush;
	HRGN clip;
	HPEN pen;
	HWND hwnd;
	int drawMode;
	int bkMode;
};
typedef struct _DC DC;
typedef DC* HDC;

HDC GetDC();
HDC CreateCompatibleDC(HDC hdc);
HBITMAP CreateBitmap(int nWidth, int nHeight, int cBitsPerPixel, uint8* data);
HBITMAP CreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight);
int CompareBitmaps(HBITMAP hBmp1, HBITMAP hBmp2);
HPEN CreatePen(int fnPenStyle, int nWidth, int crColor);
HPALETTE CreatePalette(LOGPALETTE *lplgpl);
HBRUSH CreateSolidBrush(COLORREF crColor);
HBRUSH CreatePatternBrush(HBITMAP hbmp);
HRGN CreateRectRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
HRECT CreateRect(int xLeft, int yTop, int xRight, int yBottom);
void RectToRgn(HRECT rect, HRGN rgn);
void RgnToRect(HRGN rgn, HRECT rect);
void CRectToRgn(int left, int top, int right, int bottom, HRGN rgn);
void RectToCRgn(HRECT rect, int *x, int *y, int *w, int *h);
void CRgnToRect(int x, int y, int w, int h, HRECT rect);
void RgnToCRect(HRGN rgn, int *left, int *top, int *right, int *bottom);
void CRectToCRgn(int left, int top, int right, int bottom, int *x, int *y, int *w, int *h);
void CRgnToCRect(int x, int y, int w, int h, int *left, int *top, int *right, int *bottom);
int CopyOverlap(int x, int y, int width, int height, int srcx, int srcy);
int SetROP2(HDC hdc, int fnDrawMode);
int LineTo(HDC hdc, int nXEnd, int nYEnd);
int MoveToEx(HDC hdc, int X, int Y, HPOINT lpPoint);
HPALETTE CreateSystemPalette();
HPALETTE GetSystemPalette();
int SetRect(HRECT rc, int xLeft, int yTop, int xRight, int yBottom);
int SetRgn(HRGN hRgn, int nXLeft, int nYLeft, int nWidth, int nHeight);
int SetRectRgn(HRGN hRgn, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
int SetClipRgn(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight);
HRGN GetClipRgn(HDC hdc);
int SetNullClipRgn(HDC hdc);
int ClipCoords(HDC hdc, int *x, int *y, int *w, int *h, int *srcx, int *srcy);
int InvalidateRegion(HDC hdc, int x, int y, int w, int h);
int EqualRgn(HRGN hSrcRgn1, HRGN hSrcRgn2);
int CopyRect(HRECT dst, HRECT src);
int PtInRect(HRECT rc, int x, int y);
int SelectClipRgn(HDC hdc, HRGN hrgn);
COLORREF GetBkColor(HDC hdc);
COLORREF SetBkColor(HDC hdc, COLORREF crColor);
COLORREF SetTextColor(HDC hdc, COLORREF crColor);
int SetBkMode(HDC hdc, int iBkMode);
COLORREF GetPixel(HDC hdc, int nXPos, int nYPos);
COLORREF SetPixel(HDC hdc, int X, int Y, COLORREF crColor);
int FillRect(HDC hdc, HRECT rect, HBRUSH hbr);
int PatBlt(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight, int rop);
int BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, int rop);
HGDIOBJ SelectObject(HDC hdc, HGDIOBJ hgdiobj);
int DeleteObject(HGDIOBJ hgdiobj);
int DeleteDC(HDC hdc);

#define SET_GDI(_inst, _gdi) (_inst)->param2 = _gdi
#define GET_GDI(_inst) ((GDI*) ((_inst)->param2))

#ifdef WITH_DEBUG_GDI
#define DEBUG_GDI(fmt, ...) printf("DBG (GDI) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_GDI(fmt, ...) do { } while (0)
#endif

#include "gdi_color.h"
#include "gdi_window.h"

#endif /* __LIBFREERDPGDI_H */
