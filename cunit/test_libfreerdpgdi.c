
#include <stdio.h>
#include <stdlib.h>
#include <freerdp/freerdp.h>
#include "libfreerdpgdi.h"
#include "test_libfreerdpgdi.h"

int init_libfreerdpgdi_suite(void)
{
	return 0;
}

int clean_libfreerdpgdi_suite(void)
{
	return 0;
}

int add_libfreerdpgdi_suite(void)
{
	add_test_suite(libfreerdpgdi);

	add_test_function(GetDC);
	add_test_function(CreateCompatibleDC);
	add_test_function(CreateBitmap);
	add_test_function(CreateCompatibleBitmap);
	add_test_function(CreatePen);
	add_test_function(CreatePalette);
	add_test_function(CreateSolidBrush);
	add_test_function(CreatePatternBrush);
	add_test_function(CreateRectRgn);
	add_test_function(CreateRect);
	add_test_function(GetPixel);
	add_test_function(SetPixel);
	add_test_function(SetROP2);
	add_test_function(MoveTo);
	add_test_function(PtInRect);
	add_test_function(FillRect);

	return 0;
}

void test_GetDC(void)
{
	HDC hdc = GetDC();
	CU_ASSERT(hdc->bytesPerPixel == 4);
	CU_ASSERT(hdc->bitsPerPixel == 32);
	CU_ASSERT(hdc->drawMode == R2_COPYPEN);
}

void test_CreateCompatibleDC(void)
{
	HDC hdc;
	HDC chdc;
 
	hdc = GetDC();
	hdc->bytesPerPixel = 2;
	hdc->bitsPerPixel = 16;
	hdc->drawMode = R2_XORPEN;

	chdc = CreateCompatibleDC(hdc);

	CU_ASSERT(chdc->bytesPerPixel == hdc->bytesPerPixel);
	CU_ASSERT(chdc->bitsPerPixel == hdc->bitsPerPixel);
	CU_ASSERT(chdc->drawMode == hdc->drawMode);
}

void test_CreateBitmap(void)
{
	int bpp;
	int width;
	int height;
	char* data;
	HBITMAP hBitmap;
	
	bpp = 32;
	width = 32;
	height = 16;
	data = (char*) malloc(width * height * 4);
	hBitmap = CreateBitmap(width, height, bpp, data);

	CU_ASSERT(hBitmap->objectType == GDIOBJ_BITMAP);
	CU_ASSERT(hBitmap->bitsPerPixel == bpp);
	CU_ASSERT(hBitmap->width == width);
	CU_ASSERT(hBitmap->height == height);
	CU_ASSERT(hBitmap->data == data);

	DeleteObject((HGDIOBJ) hBitmap);
}

void test_CreateCompatibleBitmap(void)
{
	HDC hdc;
	int width;
	int height;
	HBITMAP hBitmap;
	
	hdc = GetDC();
	hdc->bytesPerPixel = 4;
	hdc->bitsPerPixel = 32;

	width = 32;
	height = 16;
	hBitmap = CreateCompatibleBitmap(hdc, width, height);

	CU_ASSERT(hBitmap->objectType == GDIOBJ_BITMAP);
	CU_ASSERT(hBitmap->bytesPerPixel == hdc->bytesPerPixel);
	CU_ASSERT(hBitmap->bitsPerPixel == hdc->bitsPerPixel);
	CU_ASSERT(hBitmap->width == width);
	CU_ASSERT(hBitmap->height == height);
	CU_ASSERT(hBitmap->data != NULL);

	DeleteObject((HGDIOBJ) hBitmap);
}

void test_CreatePen(void)
{
	HPEN hPen = CreatePen(PS_SOLID, 8, 0xAABBCCDD);
	CU_ASSERT(hPen->style == PS_SOLID);
	CU_ASSERT(hPen->width == 8);
	CU_ASSERT(hPen->color == 0xAABBCCDD);
	DeleteObject((HGDIOBJ) hPen);
}

void test_CreatePalette(void)
{
	HPALETTE hPalette;
	LOGPALETTE *logicalPalette;

	logicalPalette = (LOGPALETTE*) malloc(sizeof(LOGPALETTE));
	logicalPalette->entries = (PALETTEENTRY*) malloc(sizeof(PALETTEENTRY) * 256);
	memset(logicalPalette->entries, 0, sizeof(PALETTEENTRY) * 256);
	logicalPalette->count = 5;

	hPalette = CreatePalette(logicalPalette);
	CU_ASSERT(hPalette->objectType == GDIOBJ_PALETTE);
	CU_ASSERT(hPalette->logicalPalette == logicalPalette);
	DeleteObject((HGDIOBJ) hPalette);
}

void test_CreateSolidBrush(void)
{
	HBRUSH hBrush = CreateSolidBrush(0xAABBCCDD);
	CU_ASSERT(hBrush->objectType == GDIOBJ_BRUSH);
	CU_ASSERT(hBrush->style == BS_SOLID);
	CU_ASSERT(hBrush->color == 0xAABBCCDD);
	DeleteObject((HGDIOBJ) hBrush);
}

void test_CreatePatternBrush(void)
{
	HBRUSH hBrush;
	HBITMAP hBitmap;

	hBitmap = CreateBitmap(64, 64, 32, NULL);
	hBrush = CreatePatternBrush(hBitmap);

	CU_ASSERT(hBrush->objectType == GDIOBJ_BRUSH);
	CU_ASSERT(hBrush->style == BS_PATTERN);
	CU_ASSERT(hBrush->pattern == hBitmap);

	DeleteObject((HGDIOBJ) hBrush);
	DeleteObject((HGDIOBJ) hBitmap);
}

void test_CreateRectRgn(void)
{
	int x1 = 32;
	int y1 = 64;
	int x2 = 128;
	int y2 = 256;

	HRGN hRegion = CreateRectRgn(x1, y1, x2, y2);

	CU_ASSERT(hRegion->objectType == GDIOBJ_REGION);
	CU_ASSERT(hRegion->x == x1);
	CU_ASSERT(hRegion->y == y1);
	CU_ASSERT(hRegion->w == x2 - x1);
	CU_ASSERT(hRegion->h == y2 - y1);
	CU_ASSERT(hRegion->null == 0);

	DeleteObject((HGDIOBJ) hRegion);
}

void test_CreateRect(void)
{
	int x1 = 32;
	int y1 = 64;
	int x2 = 128;
	int y2 = 256;

	HRECT hRect = CreateRect(x1, y1, x2, y2);

	CU_ASSERT(hRect->objectType == GDIOBJ_RECT);
	CU_ASSERT(hRect->left == x1);
	CU_ASSERT(hRect->top == y1);
	CU_ASSERT(hRect->right == x2);
	CU_ASSERT(hRect->bottom == y2);

	DeleteObject((HGDIOBJ) hRect);
}

void test_GetPixel(void)
{
	HDC hdc;
	int width = 128;
	int height = 64;
	HBITMAP hBitmap;

	hdc = GetDC();
	hdc->bytesPerPixel = 4;
	hdc->bitsPerPixel = 32;
	
	hBitmap = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(hdc, (HGDIOBJ) hBitmap);

	hBitmap->data[(64 * width * 4) + 32 * 4 + 0] = 0xDD;
	hBitmap->data[(64 * width * 4) + 32 * 4 + 1] = 0xCC;
	hBitmap->data[(64 * width * 4) + 32 * 4 + 2] = 0xBB;
	hBitmap->data[(64 * width * 4) + 32 * 4 + 3] = 0xAA;

	CU_ASSERT(GetPixel(hdc, 32, 64) == 0xAABBCCDD);

	DeleteObject((HGDIOBJ) hBitmap);
}

void test_SetPixel(void)
{
	HDC hdc;
	int width = 128;
	int height = 64;
	HBITMAP hBitmap;

	hdc = GetDC();
	hdc->bytesPerPixel = 4;
	hdc->bitsPerPixel = 32;
	
	hBitmap = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(hdc, (HGDIOBJ) hBitmap);

	SetPixel(hdc, 32, 64, 0xAABBCCDD);
	CU_ASSERT(GetPixel(hdc, 32, 64) == 0xAABBCCDD);

	SetPixel(hdc, width - 1, height - 1, 0xAABBCCDD);
	CU_ASSERT(GetPixel(hdc, width - 1, height - 1) == 0xAABBCCDD);

	DeleteObject((HGDIOBJ) hBitmap);
}

void test_SetROP2(void)
{
	HDC hdc = GetDC();
	SetROP2(hdc, R2_BLACK);
	CU_ASSERT(hdc->drawMode == R2_BLACK);
}

void test_MoveTo(void)
{
	HDC hdc;
	HPEN hPen;

	hdc = GetDC();
	hPen = CreatePen(PS_SOLID, 8, 0xAABBCCDD);
	SelectObject(hdc, (HGDIOBJ) hPen);
	MoveTo(hdc, 128, 256);

	CU_ASSERT(hdc->pen->posX == 128);
	CU_ASSERT(hdc->pen->posY == 256);
}

void test_PtInRect(void)
{
	HRECT hRect;
	int x1 = 20;
	int y1 = 40;
	int x2 = 60;
	int y2 = 80;

	hRect = CreateRect(x1, y1, x2, y2);

	CU_ASSERT(PtInRect(hRect, 0, 0) == 0);
	CU_ASSERT(PtInRect(hRect, 500, 500) == 0);
	CU_ASSERT(PtInRect(hRect, 40, 100) == 0);
	CU_ASSERT(PtInRect(hRect, 10, 40) == 0);
	CU_ASSERT(PtInRect(hRect, 30, 50) == 1);
	CU_ASSERT(PtInRect(hRect, x1, y1) == 1);
	CU_ASSERT(PtInRect(hRect, x2, y2) == 0);
	CU_ASSERT(PtInRect(hRect, x2, 60) == 0);
	CU_ASSERT(PtInRect(hRect, 40, y2) == 0);
}

void test_FillRect(void)
{
	HDC hdc;
	HRECT hRect;
	HBRUSH hBrush;
	HBITMAP hBitmap;

	int x, y;
	int badPixels;
	int goodPixels;
	int width = 200;
	int height = 300;

	int x1 = 20;
	int y1 = 40;
	int x2 = 60;
	int y2 = 80;

	hdc = GetDC();
	hdc->bytesPerPixel = 4;
	hdc->bitsPerPixel = 32;

	hRect = CreateRect(x1, y1, x2, y2);

	hBitmap = CreateCompatibleBitmap(hdc, width, height);
	memset(hBitmap->data, 0, width * height * hdc->bytesPerPixel);
	SelectObject(hdc, (HGDIOBJ) hBitmap);

	hBrush = CreateSolidBrush(0xAABBCCDD);

	FillRect(hdc, hRect, hBrush);

	badPixels = 0;
	goodPixels = 0;

	for (x = 0; x < width; x++)
	{
		for (y = 0; y < height; y++)
		{
			if (PtInRect(hRect, x, y))
			{
				if (GetPixel(hdc, x, y) == 0xAABBCCDD) {
					goodPixels++;
				}
				else {
					badPixels++;
				}
			}
			else
			{
				if (GetPixel(hdc, x, y) == 0xAABBCCDD) {
					badPixels++;
				}
				else {
					goodPixels++;
				}
			}
		}
	}

	CU_ASSERT(goodPixels == width * height);
	CU_ASSERT(badPixels == 0);

	DeleteObject((HGDIOBJ) hBrush);
	DeleteObject((HGDIOBJ) hBitmap);
}


