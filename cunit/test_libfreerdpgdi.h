
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
void test_BitBlt(void);
void test_ClipCoords(void);
void test_InvalidateRegion(void);
