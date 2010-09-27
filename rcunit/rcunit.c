#include <rcunit.h>
#include <stdlib.h>

#include <freerdp/freerdp.h>
#include "libfreerdpgdi.h"

/* GetDC() */
RCU_DEF_TEST_FUNC(test_GetDC, param)
{
	HDC hdc = GetDC();
	RCU_ASSERT(hdc->bytesPerPixel == 4);
	RCU_ASSERT(hdc->bitsPerPixel == 32);
	RCU_ASSERT(hdc->drawMode == R2_COPYPEN);
}

/* CreateCompatibleDC() */
RCU_DEF_TEST_FUNC(test_CreateCompatibleDC, param)
{
	HDC hdc;
	HDC chdc;
 
	hdc = GetDC();
	hdc->bytesPerPixel = 2;
	hdc->bitsPerPixel = 16;
	hdc->drawMode = R2_XORPEN;

	chdc = CreateCompatibleDC(hdc);

	RCU_ASSERT(chdc->bytesPerPixel == hdc->bytesPerPixel);
	RCU_ASSERT(chdc->bitsPerPixel == hdc->bitsPerPixel);
	RCU_ASSERT(chdc->drawMode == hdc->drawMode);
}

/* CreateBitmap() */
RCU_DEF_TEST_FUNC(test_CreateBitmap, param)
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

	RCU_ASSERT(hBitmap->objectType == GDIOBJ_BITMAP);
	RCU_ASSERT(hBitmap->bitsPerPixel == bpp);
	RCU_ASSERT(hBitmap->width == width);
	RCU_ASSERT(hBitmap->height == height);
	RCU_ASSERT(hBitmap->data == data);

	DeleteObject((HGDIOBJ) hBitmap);
}

/* CreateCompatibleBitmap() */
RCU_DEF_TEST_FUNC(test_CreateCompatibleBitmap, param)
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

	RCU_ASSERT(hBitmap->objectType == GDIOBJ_BITMAP);
	RCU_ASSERT(hBitmap->bytesPerPixel == hdc->bytesPerPixel);
	RCU_ASSERT(hBitmap->bitsPerPixel == hdc->bitsPerPixel);
	RCU_ASSERT(hBitmap->width == width);
	RCU_ASSERT(hBitmap->height == height);
	RCU_ASSERT(hBitmap->data != NULL);

	DeleteObject((HGDIOBJ) hBitmap);
}

/* CreatePen() */
RCU_DEF_TEST_FUNC(test_CreatePen, param)
{
	HPEN hPen = CreatePen(PS_SOLID, 8, 0xAABBCCDD);
	RCU_ASSERT(hPen->style == PS_SOLID);
	RCU_ASSERT(hPen->width == 8);
	RCU_ASSERT(hPen->color == 0xAABBCCDD);
	DeleteObject((HGDIOBJ) hPen);
}

/* CreatePalette() */
RCU_DEF_TEST_FUNC(test_CreatePalette, param)
{
	HPALETTE hPalette;
	LOGPALETTE *logicalPalette;

	logicalPalette = (LOGPALETTE*) malloc(sizeof(LOGPALETTE));
	logicalPalette->entries = (PALETTEENTRY*) malloc(sizeof(PALETTEENTRY) * 256);
	memset(logicalPalette->entries, 0, sizeof(PALETTEENTRY) * 256);
	logicalPalette->count = 5;

	hPalette = CreatePalette(logicalPalette);
	RCU_ASSERT(hPalette->objectType == GDIOBJ_PALETTE);
	RCU_ASSERT(hPalette->logicalPalette == logicalPalette);
	DeleteObject((HGDIOBJ) hPalette);
}

/* CreateSolidBrush() */
RCU_DEF_TEST_FUNC(test_CreateSolidBrush, param)
{
	HBRUSH hBrush = CreateSolidBrush(0xAABBCCDD);
	RCU_ASSERT(hBrush->objectType == GDIOBJ_BRUSH);
	RCU_ASSERT(hBrush->style == BS_SOLID);
	RCU_ASSERT(hBrush->color == 0xAABBCCDD);
	DeleteObject((HGDIOBJ) hBrush);
}

/* CreatePatternBrush() */
RCU_DEF_TEST_FUNC(test_CreatePatternBrush, param)
{
	HBRUSH hBrush;
	HBITMAP hBitmap;

	hBitmap = CreateBitmap(64, 64, 32, NULL);
	hBrush = CreatePatternBrush(hBitmap);

	RCU_ASSERT(hBrush->objectType == GDIOBJ_BRUSH);
	RCU_ASSERT(hBrush->style == BS_PATTERN);
	RCU_ASSERT(hBrush->pattern == hBitmap);

	DeleteObject((HGDIOBJ) hBrush);
	DeleteObject((HGDIOBJ) hBitmap);
}

/* CreateRectRgn() */
RCU_DEF_TEST_FUNC(test_CreateRectRgn, param)
{
	int x1 = 32;
	int y1 = 64;
	int x2 = 128;
	int y2 = 256;

	HRGN hRegion = CreateRectRgn(x1, y1, x2, y2);

	RCU_ASSERT(hRegion->objectType == GDIOBJ_REGION);
	RCU_ASSERT(hRegion->x == x1);
	RCU_ASSERT(hRegion->y == y1);
	RCU_ASSERT(hRegion->w == x2 - x1);
	RCU_ASSERT(hRegion->h == y2 - y1);
	RCU_ASSERT(hRegion->null == 0);

	DeleteObject((HGDIOBJ) hRegion);
}

/* CreateRect() */
RCU_DEF_TEST_FUNC(test_CreateRect, param)
{
	int x1 = 32;
	int y1 = 64;
	int x2 = 128;
	int y2 = 256;

	HRECT hRect = CreateRect(x1, y1, x2, y2);

	RCU_ASSERT(hRect->objectType == GDIOBJ_RECT);
	RCU_ASSERT(hRect->left == x1);
	RCU_ASSERT(hRect->top == y1);
	RCU_ASSERT(hRect->right == x2);
	RCU_ASSERT(hRect->bottom == y2);

	DeleteObject((HGDIOBJ) hRect);
}

/* GetPixel() */
RCU_DEF_TEST_FUNC(test_GetPixel, param)
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

	printf("GetPixel(32, 64) = 0x%04X\n", GetPixel(hdc, 32, 64));
	RCU_ASSERT(GetPixel(hdc, 32, 64) == 0xAABBCCDD);

	DeleteObject((HGDIOBJ) hBitmap);
}

/* SetPixel() */
RCU_DEF_TEST_FUNC(test_SetPixel, param)
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
	printf("SetPixel(hdc, 32, 64, 0xAABBCCDD)\n");
	printf("GetPixel(32, 64) = 0x%04X\n", GetPixel(hdc, 32, 64));
	RCU_ASSERT(GetPixel(hdc, 32, 64) == 0xAABBCCDD);

	printf("SetPixel(hdc, width - 1, height - 1, 0xAABBCCDD)\n");
	SetPixel(hdc, width - 1, height - 1, 0xAABBCCDD);
	RCU_ASSERT(GetPixel(hdc, width - 1, height - 1) == 0xAABBCCDD);

	DeleteObject((HGDIOBJ) hBitmap);
}

/* SetROP2() */
RCU_DEF_TEST_FUNC(test_SetROP2, param)
{
	HDC hdc = GetDC();
	SetROP2(hdc, R2_BLACK);
	RCU_ASSERT(hdc->drawMode == R2_BLACK);
}

/* MoveTo() */
RCU_DEF_TEST_FUNC(test_MoveTo, param)
{
	HDC hdc;
	HPEN hPen;

	hdc = GetDC();
	hPen = CreatePen(PS_SOLID, 8, 0xAABBCCDD);
	SelectObject(hdc, (HGDIOBJ) hPen);
	MoveTo(hdc, 128, 256);

	RCU_ASSERT(hdc->pen->posX == 128);
	RCU_ASSERT(hdc->pen->posY == 256);
}

/* PtInRect() */
RCU_DEF_TEST_FUNC(test_PtInRect, param)
{
	HRECT hRect;
	int x1 = 20;
	int y1 = 40;
	int x2 = 60;
	int y2 = 80;

	hRect = CreateRect(x1, y1, x2, y2);

	RCU_ASSERT(PtInRect(hRect, 0, 0) == 0);
	RCU_ASSERT(PtInRect(hRect, 500, 500) == 0);
	RCU_ASSERT(PtInRect(hRect, 40, 100) == 0);
	RCU_ASSERT(PtInRect(hRect, 10, 40) == 0);
	RCU_ASSERT(PtInRect(hRect, 30, 50) == 1);
	RCU_ASSERT(PtInRect(hRect, x1, y1) == 1);
	RCU_ASSERT(PtInRect(hRect, x2, y2) == 0);
	RCU_ASSERT(PtInRect(hRect, x2, 60) == 0);
	RCU_ASSERT(PtInRect(hRect, 40, y2) == 0);
}

/* FillRect() */
RCU_DEF_TEST_FUNC(test_FillRect, param)
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

	printf("FillRect: x1:%d y1:%d x2:%d y2:%d width:%d height:%d\n", x1, y1, x2, y2, (x2 - x1), (y2 - y1));
	FillRect(hdc, hRect, hBrush);

	badPixels = 0;
	goodPixels = 0;

	for (x = 0; x < width; x++)
	{
		for (y = 0; y < height; y++)
		{
			if (PtInRect(hRect, x, y))
			{
				if (GetPixel(hdc, x, y) == 0xAABBCCDD)
				{
					goodPixels++;
				}
				else
				{
					printf("Bad Pixel (in rect): %d,%d, 0x%04X\n", x, y, GetPixel(hdc, x, y));
					badPixels++;
				}
			}
			else
			{
				if (GetPixel(hdc, x, y) == 0xAABBCCDD)
				{
					printf("Bad Pixel (out of rect): %d,%d, 0x%04X\n", x, y, GetPixel(hdc, x, y));
					badPixels++;
				}
				else
				{
					goodPixels++;
				}
			}
		}
	}

	printf("good pixels: %d\n", goodPixels);
	printf("bad pixels: %d\n", badPixels);

	RCU_ASSERT(goodPixels == width * height);
	RCU_ASSERT(badPixels == 0);

	DeleteObject((HGDIOBJ) hBrush);
	DeleteObject((HGDIOBJ) hBitmap);
}

int libfreerdpgdi_tests()
{
	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_GetDC,
		RCU_NULL, RCU_NULL, "test_GetDC", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_CreateCompatibleDC,
		RCU_NULL, RCU_NULL, "test_CreateCompatibleDC", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_CreateBitmap,
		RCU_NULL, RCU_NULL, "test_CreateBitmap", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_CreateCompatibleBitmap,
		RCU_NULL, RCU_NULL, "test_CreateCompatibleBitmap", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_CreatePen,
		RCU_NULL, RCU_NULL, "test_CreatePen", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_CreatePalette,
		RCU_NULL, RCU_NULL, "test_CreatePalette", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_CreateSolidBrush,
		RCU_NULL, RCU_NULL, "test_CreateSolidBrush", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_CreatePatternBrush,
		RCU_NULL, RCU_NULL, "test_CreatePatternBrush", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_CreateRectRgn,
		RCU_NULL, RCU_NULL, "test_CreateRectRgn", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_CreateRect,
		RCU_NULL, RCU_NULL, "test_CreateRect", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_GetPixel,
		RCU_NULL, RCU_NULL, "test_GetPixel", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_SetPixel,
		RCU_NULL, RCU_NULL, "test_SetPixel", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_SetROP2,
		RCU_NULL, RCU_NULL, "test_SetROP2", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_MoveTo,
		RCU_NULL, RCU_NULL, "test_MoveTo", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_PtInRect,
		RCU_NULL, RCU_NULL, "test_PtInRect", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	if ((rcu_add_test_func(RCU_DEFAULT_MODULE, test_FillRect,
		RCU_NULL, RCU_NULL, "test_FillRect", RCU_TRUE)) == RCU_E_NG) {
		rcu_destroy();
		return -1;
	}

	return 0;
}

int main(int argc,char **argv)
{
	if (rcu_init() == RCU_E_NG)
		return -1;

	printf("Running libfreerdpgdi tests...\n");
	libfreerdpgdi_tests();

	rcu_dump_test_dbase();
	rcu_run_test_mach();
	rcu_destroy();

	return 0;
}


