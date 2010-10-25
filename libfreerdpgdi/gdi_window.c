/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   GDI Window Routines

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <freerdp/freerdp.h>
#include "libfreerdpgdi.h"

#include "gdi_color.h"
#include "gdi_window.h"

/* Ternary Raster Operation Table */
const unsigned int rop3_code_table[] =
{
	0x00000042, // 0
	0x00010289, // DPSoon
	0x00020C89, // DPSona
	0x000300AA, // PSon
	0x00040C88, // SDPona
	0x000500A9, // DPon
	0x00060865, // PDSxnon
	0x000702C5, // PDSaon
	0x00080F08, // SDPnaa
	0x00090245, // PDSxon
	0x000A0329, // DPna
	0x000B0B2A, // PSDnaon
	0x000C0324, // SPna
	0x000D0B25, // PDSnaon
	0x000E08A5, // PDSonon
	0x000F0001, // Pn
	0x00100C85, // PDSona
	0x001100A6, // DSon
	0x00120868, // SDPxnon
	0x001302C8, // SDPaon
	0x00140869, // DPSxnon
	0x001502C9, // DPSaon
	0x00165CCA, // PSDPSanaxx
	0x00171D54, // SSPxDSxaxn
	0x00180D59, // SPxPDxa
	0x00191CC8, // SDPSanaxn
	0x001A06C5, // PDSPaox
	0x001B0768, // SDPSxaxn
	0x001C06CA, // PSDPaox
	0x001D0766, // DSPDxaxn
	0x001E01A5, // PDSox
	0x001F0385, // PDSoan
	0x00200F09, // DPSnaa
	0x00210248, // SDPxon
	0x00220326, // DSna
	0x00230B24, // SPDnaon
	0x00240D55, // SPxDSxa
	0x00251CC5, // PDSPanaxn
	0x002606C8, // SDPSaox
	0x00271868, // SDPSxnox
	0x00280369, // DPSxa
	0x002916CA, // PSDPSaoxxn
	0x002A0CC9, // DPSana
	0x002B1D58, // SSPxPDxaxn
	0x002C0784, // SPDSoax
	0x002D060A, // PSDnox
	0x002E064A, // PSDPxox
	0x002F0E2A, // PSDnoan
	0x0030032A, // PSna
	0x00310B28, // SDPnaon
	0x00320688, // SDPSoox
	0x00330008, // Sn
	0x003406C4, // SPDSaox
	0x00351864, // SPDSxnox
	0x003601A8, // SDPox
	0x00370388, // SDPoan
	0x0038078A, // PSDPoax
	0x00390604, // SPDnox
	0x003A0644, // SPDSxox
	0x003B0E24, // SPDnoan
	0x003C004A, // PSx
	0x003D18A4, // SPDSonox
	0x003E1B24, // SPDSnaox
	0x003F00EA, // PSan
	0x00400F0A, // PSDnaa
	0x00410249, // DPSxon
	0x00420D5D, // SDxPDxa
	0x00431CC4, // SPDSanaxn
	0x00440328, // SDna
	0x00450B29, // DPSnaon
	0x004606C6, // DSPDaox
	0x0047076A, // PSDPxaxn
	0x00480368, // SDPxa
	0x004916C5, // PDSPDaoxxn
	0x004A0789, // DPSDoax
	0x004B0605, // PDSnox
	0x004C0CC8, // SDPana
	0x004D1954, // SSPxDSxoxn
	0x004E0645, // PDSPxox
	0x004F0E25, // PDSnoan
	0x00500325, // PDna
	0x00510B26, // DSPnaon
	0x005206C9, // DPSDaox
	0x00530764, // SPDSxaxn
	0x005408A9, // DPSonon
	0x00550009, // Dn
	0x005601A9, // DPSox
	0x00570389, // DPSoan
	0x00580785, // PDSPoax
	0x00590609, // DPSnox
	0x005A0049, // DPx
	0x005B18A9, // DPSDonox
	0x005C0649, // DPSDxox
	0x005D0E29, // DPSnoan
	0x005E1B29, // DPSDnaox
	0x005F00E9, // DPan
	0x00600365, // PDSxa
	0x006116C6, // DSPDSaoxxn
	0x00620786, // DSPDoax
	0x00630608, // SDPnox
	0x00640788, // SDPSoax
	0x00650606, // DSPnox
	0x00660046, // DSx
	0x006718A8, // SDPSonox
	0x006858A6, // DSPDSonoxxn
	0x00690145, // PDSxxn
	0x006A01E9, // DPSax
	0x006B178A, // PSDPSoaxxn
	0x006C01E8, // SDPax
	0x006D1785, // PDSPDoaxxn
	0x006E1E28, // SDPSnoax
	0x006F0C65, // PDSxnan
	0x00700CC5, // PDSana
	0x00711D5C, // SSDxPDxaxn
	0x00720648, // SDPSxox
	0x00730E28, // SDPnoan
	0x00740646, // DSPDxox
	0x00750E26, // DSPnoan
	0x00761B28, // SDPSnaox
	0x007700E6, // DSan
	0x007801E5, // PDSax
	0x00791786, // DSPDSoaxxn
	0x007A1E29, // DPSDnoax
	0x007B0C68, // SDPxnan
	0x007C1E24, // SPDSnoax
	0x007D0C69, // DPSxnan
	0x007E0955, // SPxDSxo
	0x007F03C9, // DPSaan
	0x008003E9, // DPSaa
	0x00810975, // SPxDSxon
	0x00820C49, // DPSxna
	0x00831E04, // SPDSnoaxn
	0x00840C48, // SDPxna
	0x00851E05, // PDSPnoaxn
	0x008617A6, // DSPDSoaxx
	0x008701C5, // PDSaxn
	0x008800C6, // DSa
	0x00891B08, // SDPSnaoxn
	0x008A0E06, // DSPnoa
	0x008B0666, // DSPDxoxn
	0x008C0E08, // SDPnoa
	0x008D0668, // SDPSxoxn
	0x008E1D7C, // SSDxPDxax
	0x008F0CE5, // PDSanan
	0x00900C45, // PDSxna
	0x00911E08, // SDPSnoaxn
	0x009217A9, // DPSDPoaxx
	0x009301C4, // SPDaxn
	0x009417AA, // PSDPSoaxx
	0x009501C9, // DPSaxn
	0x00960169, // DPSxx
	0x0097588A, // PSDPSonoxx
	0x00981888, // SDPSonoxn
	0x00990066, // DSxn
	0x009A0709, // DPSnax
	0x009B07A8, // SDPSoaxn
	0x009C0704, // SPDnax
	0x009D07A6, // DSPDoaxn
	0x009E16E6, // DSPDSaoxx
	0x009F0345, // PDSxan
	0x00A000C9, // DPa
	0x00A11B05, // PDSPnaoxn
	0x00A20E09, // DPSnoa
	0x00A30669, // DPSDxoxn
	0x00A41885, // PDSPonoxn
	0x00A50065, // PDxn
	0x00A60706, // DSPnax
	0x00A707A5, // PDSPoaxn
	0x00A803A9, // DPSoa
	0x00A90189, // DPSoxn
	0x00AA0029, // D
	0x00AB0889, // DPSono
	0x00AC0744, // SPDSxax
	0x00AD06E9, // DPSDaoxn
	0x00AE0B06, // DSPnao
	0x00AF0229, // DPno
	0x00B00E05, // PDSnoa
	0x00B10665, // PDSPxoxn
	0x00B21974, // SSPxDSxox
	0x00B30CE8, // SDPanan
	0x00B4070A, // PSDnax
	0x00B507A9, // DPSDoaxn
	0x00B616E9, // DPSDPaoxx
	0x00B70348, // SDPxan
	0x00B8074A, // PSDPxax
	0x00B906E6, // DSPDaoxn
	0x00BA0B09, // DPSnao
	0x00BB0226, // DSno
	0x00BC1CE4, // SPDSanax
	0x00BD0D7D, // SDxPDxan
	0x00BE0269, // DPSxo
	0x00BF08C9, // DPSano
	0x00C000CA, // PSa
	0x00C11B04, // SPDSnaoxn
	0x00C21884, // SPDSonoxn
	0x00C3006A, // PSxn
	0x00C40E04, // SPDnoa
	0x00C50664, // SPDSxoxn
	0x00C60708, // SDPnax
	0x00C707AA, // PSDPoaxn
	0x00C803A8, // SDPoa
	0x00C90184, // SPDoxn
	0x00CA0749, // DPSDxax
	0x00CB06E4, // SPDSaoxn
	0x00CC0020, // S
	0x00CD0888, // SDPono
	0x00CE0B08, // SDPnao
	0x00CF0224, // SPno
	0x00D00E0A, // PSDnoa
	0x00D1066A, // PSDPxoxn
	0x00D20705, // PDSnax
	0x00D307A4, // SPDSoaxn
	0x00D41D78, // SSPxPDxax
	0x00D50CE9, // DPSanan
	0x00D616EA, // PSDPSaoxx
	0x00D70349, // DPSxan
	0x00D80745, // PDSPxax
	0x00D906E8, // SDPSaoxn
	0x00DA1CE9, // DPSDanax
	0x00DB0D75, // SPxDSxan
	0x00DC0B04, // SPDnao
	0x00DD0228, // SDno
	0x00DE0268, // SDPxo
	0x00DF08C8, // SDPano
	0x00E003A5, // PDSoa
	0x00E10185, // PDSoxn
	0x00E20746, // DSPDxax
	0x00E306EA, // PSDPaoxn
	0x00E40748, // SDPSxax
	0x00E506E5, // PDSPaoxn
	0x00E61CE8, // SDPSanax
	0x00E70D79, // SPxPDxan
	0x00E81D74, // SSPxDSxax
	0x00E95CE6, // DSPDSanaxxn
	0x00EA02E9, // DPSao
	0x00EB0849, // DPSxno
	0x00EC02E8, // SDPao
	0x00ED0848, // SDPxno
	0x00EE0086, // DSo
	0x00EF0A08, // SDPnoo
	0x00F00021, // P
	0x00F10885, // PDSono
	0x00F20B05, // PDSnao
	0x00F3022A, // PSno
	0x00F40B0A, // PSDnao
	0x00F50225, // PDno
	0x00F60265, // PDSxo
	0x00F708C5, // PDSano
	0x00F802E5, // PDSao
	0x00F90845, // PDSxno
	0x00FA0089, // DPo
	0x00FB0A09, // DPSnoo
	0x00FC008A, // PSo
	0x00FD0A0A, // PSDnoo
	0x00FE02A9, // DPSoo
	0x00FF0062  // 1
};

/* GDI Helper Functions */

unsigned int
gdi_rop3_code(unsigned char code)
{
	return rop3_code_table[code];
}

void
gdi_copy_mem(char * d, char * s, int n)
{
	while (n & (~7))
	{
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		*(d++) = *(s++);
		n = n - 8;
	}
	while (n > 0)
	{
		*(d++) = *(s++);
		n--;
	}
}

void
gdi_copy_memb(char * d, char * s, int n)
{
	d = (d + n) - 1;
	s = (s + n) - 1;
	
	while (n & (~7))
	{
		*(d--) = *(s--);
		*(d--) = *(s--);
		*(d--) = *(s--);
		*(d--) = *(s--);
		*(d--) = *(s--);
		*(d--) = *(s--);
		*(d--) = *(s--);
		*(d--) = *(s--);
		n = n - 8;
	}

	while (n > 0)
	{
		*(d--) = *(s--);
		n--;
	}
}

char*
gdi_get_bitmap_pointer(HDC hdcBmp, int x, int y)
{
	char * p;
	HBITMAP hBmp = (HBITMAP) hdcBmp->selectedObject;
	
	if (x >= 0 && x < hBmp->width && y >= 0 && y < hBmp->height)
	{
		p = hBmp->data + (y * hBmp->width * hdcBmp->bytesPerPixel) + (x * hdcBmp->bytesPerPixel);
		return p;
	}
	else
	{
		printf("gdi_get_bitmap_pointer: requesting invalid pointer: (%d,%d) in %dx%d\n", x, y, hBmp->width, hBmp->height);
		return 0;
	}
}

char*
gdi_get_brush_pointer(HDC hdcBrush, int x, int y)
{
	char * p;

	if (hdcBrush->brush != NULL)
	{
		if (hdcBrush->brush->style == BS_PATTERN)
		{
			HBITMAP hBmpBrush = hdcBrush->brush->pattern;
	
			if (x >= 0 && y >= 0)
			{
				x = x % hBmpBrush->width;
				y = y % hBmpBrush->height;
				p = hBmpBrush->data + (y * hBmpBrush->scanline) + (x * hBmpBrush->bytesPerPixel);
				return p;
			}
		}
	}

	p = (char*) &(hdcBrush->textColor);
	return p;
}

int
gdi_is_mono_pixel_set(char* data, int x, int y, int width)
{
	int byte;
	int shift;

	width = (width + 7) / 8;
	byte = (y * width) + (x / 8);
	shift = x % 8;

	return (data[byte] & (0x80 >> shift)) != 0;
}

HBITMAP
gdi_create_bitmap(GDI* gdi, int width, int height, int bpp, int reverse, char* data)
{
	char* bmpData;
	HBITMAP bitmap;
	
	bmpData = (char*) gdi_image_convert(data, width, height, gdi->srcBpp, bpp, gdi->palette);
	bitmap = CreateBitmap(width, height, gdi->dstBpp, bmpData);
	
	return bitmap;
}

gdi_bitmap*
gdi_bitmap_new(GDI *gdi, int width, int height, int bpp, int reverse, char* data)
{
	gdi_bitmap *gdi_bmp;
	
	gdi_bmp = (gdi_bitmap*) malloc(sizeof(gdi_bitmap));
	gdi_bmp->hdc = CreateCompatibleDC(gdi->hdc);
	
	if (data == NULL)
	{
		gdi_bmp->bitmap = CreateCompatibleBitmap(gdi->hdc, width, height);
	}
	else
	{
		gdi_bmp->bitmap = gdi_create_bitmap(gdi, width, height, bpp, reverse, data);
	}
	
	SelectObject(gdi_bmp->hdc, (HGDIOBJ) gdi_bmp->bitmap);
	gdi_bmp->org_bitmap = NULL;
	
	return gdi_bmp;
}

void
gdi_bitmap_free(gdi_bitmap *gdi_bmp)
{
	if (gdi_bmp != 0)
	{
		SelectObject(gdi_bmp->hdc, (HGDIOBJ) gdi_bmp->org_bitmap);
		DeleteObject((HGDIOBJ) gdi_bmp->bitmap);
		DeleteDC(gdi_bmp->hdc);
		free(gdi_bmp);
	}
}

/* GDI callbacks registered in libfreerdp */

static void
gdi_ui_desktop_save(struct rdp_inst * inst, int offset, int x, int y, int cx, int cy)
{
	DEBUG_GDI("gdi_ui_desktop_save\n");
}

static void
gdi_ui_desktop_restore(struct rdp_inst * inst, int offset, int x, int y, int cx, int cy)
{
	DEBUG_GDI("gdi_ui_desktop_restore\n");
}

static RD_HGLYPH
gdi_ui_create_glyph(struct rdp_inst * inst, int width, int height, uint8 * data)
{
	char* glyph;
	gdi_bitmap *gdi_bmp;

	//DEBUG_GDI("gdi_ui_create_glyph: width:%d height:%d\n", width, height);

	gdi_bmp = (gdi_bitmap*) malloc(sizeof(gdi_bitmap));
	
	gdi_bmp->hdc = GetDC();
	gdi_bmp->hdc->bytesPerPixel = 1;
	gdi_bmp->hdc->bitsPerPixel = 1;
	glyph = gdi_glyph_convert(width, height, (char*) data);
	gdi_bmp->bitmap = CreateBitmap(width, height, 1, glyph);
	gdi_bmp->bitmap->bytesPerPixel = 1;
	gdi_bmp->bitmap->bitsPerPixel = 1;
	SelectObject(gdi_bmp->hdc, (HGDIOBJ) gdi_bmp->bitmap);
	gdi_bmp->org_bitmap = NULL;

	return (RD_HGLYPH) gdi_bmp;
}

static void
gdi_ui_destroy_glyph(struct rdp_inst * inst, RD_HGLYPH glyph)
{
	gdi_bitmap_free((gdi_bitmap*) glyph);
}

static RD_HBITMAP
gdi_ui_create_bitmap(struct rdp_inst * inst, int width, int height, uint8* data)
{
	gdi_bitmap *gdi_bmp;
	GDI *gdi = GET_GDI(inst);

	//DEBUG_GDI("gdi_ui_create_bitmap: width:%d height:%d\n", width, height);

	gdi_bmp = gdi_bitmap_new(gdi, width, height, gdi->dstBpp, 1, (char*) data);
	
	return (RD_HBITMAP) gdi_bmp;
}

static void
gdi_ui_paint_bitmap(struct rdp_inst * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
	gdi_bitmap *gdi_bmp;
	GDI *gdi = GET_GDI(inst);

	DEBUG_GDI("ui_paint_bitmap: x:%d y:%d cx:%d cy:%d\n", x, y, cx, cy);

	gdi_bmp = (gdi_bitmap*) inst->ui_create_bitmap(inst, width, height, data);
	BitBlt(gdi->primary->hdc, x, y, cx, cy, gdi_bmp->hdc, 0, 0, SRCCOPY);
	inst->ui_destroy_bitmap(inst, (RD_HBITMAP) gdi_bmp);
}

static void
gdi_ui_destroy_bitmap(struct rdp_inst * inst, RD_HBITMAP bmp)
{
	gdi_bitmap_free((gdi_bitmap*) bmp);
}

static void
gdi_ui_line(struct rdp_inst * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen)
{
	DEBUG_GDI("ui_line opcode:%d startx:%d starty:%d endx:%d endy:%d\n", opcode, startx, starty, endx, endy);

	int cx;
	int cy;
	HPEN hPen;
	GDI *gdi = GET_GDI(inst);

	cx = endx - startx + 1;
	cy = endy - starty + 1;
	
	gdi_color_convert(&(gdi->pixel), pen->color, gdi->srcBpp, gdi->palette);
	
	hPen = CreatePen(pen->style, pen->width, (COLORREF) PixelRGB(gdi->pixel));
	SelectObject(gdi->drawing->hdc, (HGDIOBJ) hPen);
	SetROP2(gdi->drawing->hdc, opcode + 1);

	MoveTo(gdi->drawing->hdc, startx, starty);
	LineTo(gdi->drawing->hdc, endx, endy);
	
	DeleteObject((HGDIOBJ) hPen);
}

static void
gdi_ui_rect(struct rdp_inst * inst, int x, int y, int cx, int cy, int color)
{
	RECT rect;
	HBRUSH hBrush;
	GDI *gdi = GET_GDI(inst);

	//DEBUG_GDI("ui_rect: x:%d y:%d cx:%d cy:%d\n", x, y, cx, cy);

	CRgnToRect(x, y, cx, cy, &rect);
	gdi_color_convert(&(gdi->pixel), color, gdi->srcBpp, gdi->palette);
	hBrush = CreateSolidBrush(PixelRGB(gdi->pixel));
	FillRect(gdi->drawing->hdc, &rect, hBrush);
}

static void
gdi_ui_polygon(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point, int npoints, RD_BRUSH * brush, int bgcolor, int fgcolor)
{
	DEBUG_GDI("ui_polygon\n");
}

static void
gdi_ui_polyline(struct rdp_inst * inst, uint8 opcode, RD_POINT * points, int npoints, RD_PEN * pen)
{
	DEBUG_GDI("ui_polyline\n");
}

static void
gdi_ui_ellipse(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy, RD_BRUSH * brush, int bgcolor, int fgcolor)
{
	DEBUG_GDI("ui_ellipse\n");
}

static void
gdi_ui_start_draw_glyphs(struct rdp_inst * inst, int bgcolor, int fgcolor)
{
	GDI *gdi = GET_GDI(inst);
	gdi_color_convert(&(gdi->pixel), fgcolor, gdi->srcBpp, gdi->palette);
	gdi->textColor = SetTextColor(gdi->drawing->hdc, PixelRGB(gdi->pixel));
}

static void
gdi_ui_draw_glyph(struct rdp_inst * inst, int x, int y, int cx, int cy, RD_HGLYPH glyph)
{
	gdi_bitmap* gdi_bmp;
	GDI *gdi = GET_GDI(inst);

	gdi_bmp = (gdi_bitmap*) glyph;
	BitBlt(gdi->drawing->hdc, x, y, cx, cy, gdi_bmp->hdc, 0, 0, DSPDxax);
}

static void
gdi_ui_end_draw_glyphs(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
	GDI *gdi = GET_GDI(inst);
	SetTextColor(gdi->drawing->hdc, gdi->textColor);
}

static void
gdi_ui_destblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy)
{
	GDI *gdi = GET_GDI(inst);

	DEBUG_GDI("ui_destblt: x: %d y: %d cx: %d cy: %d rop: 0x%X\n", x, y, cx, cy, rop3_code_table[opcode]);
	BitBlt(gdi->drawing->hdc, x, y, cx, cy, NULL, 0, 0, gdi_rop3_code(opcode));
}

static void
gdi_ui_patblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH * brush, int bgcolor, int fgcolor)
{
	GDI *gdi = GET_GDI(inst);
	
	DEBUG_GDI("ui_patblt: x: %d y: %d cx: %d cy: %d rop: 0x%X\n", x, y, cx, cy, gdi_rop3_code(opcode));
	
	if (brush->style == BS_PATTERN)
	{
		HBITMAP hBmp;
		HBRUSH originalBrush;
		
		hBmp = CreateBitmap(8, 8, gdi->drawing->hdc->bitsPerPixel,
			(char*) gdi_image_convert((char*) brush->bd->data, 8, 8, gdi->srcBpp, gdi->dstBpp, gdi->palette));

		originalBrush = gdi->drawing->hdc->brush;
		gdi->drawing->hdc->brush = CreatePatternBrush(hBmp);

		PatBlt(gdi->drawing->hdc, x, y, cx, cy, gdi_rop3_code(opcode));

		DeleteObject((HGDIOBJ) gdi->drawing->hdc->brush);
		gdi->drawing->hdc->brush = originalBrush;
	}
	else if (brush->style == BS_SOLID)
	{
		gdi_color_convert(&(gdi->pixel), fgcolor, gdi->dstBpp, gdi->palette);
		gdi->textColor = SetTextColor(gdi->drawing->hdc, PixelRGB(gdi->pixel));
		
		PatBlt(gdi->drawing->hdc, x, y, cx, cy, gdi_rop3_code(opcode));

		SetTextColor(gdi->drawing->hdc, gdi->textColor);
	}
	else
	{
		printf("ui_patblt: unknown brush style: %d\n", brush->style);
	}
}

static void
gdi_ui_screenblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy)
{
	GDI *gdi = GET_GDI(inst);
	
	DEBUG_GDI("gdi_ui_screenblt x:%d y:%d cx:%d cy:%d srcx:%d srcy:%d rop:0x%X\n",
	          x, y, cx, cy, srcx, srcy, rop3_code_table[opcode]);
	
	BitBlt(gdi->drawing->hdc, x, y, cx, cy, gdi->primary->hdc, srcx, srcy, gdi_rop3_code(opcode));
}

static void
gdi_ui_memblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy)
{
	gdi_bitmap *gdi_bmp;
	GDI *gdi = GET_GDI(inst);

	//DEBUG_GDI("gdi_ui_memblt: x:%d y:%d cx:%d cy:%d srcx:%d, srcy:%d rop:0x%X\n",
	//          x, y, cx, cy, srcx, srcy, gdi_rop3_code(opcode));

	gdi_bmp = (gdi_bitmap*) src;
	BitBlt(gdi->drawing->hdc, x, y, cx, cy, gdi_bmp->hdc, srcx, srcy, gdi_rop3_code(opcode));
}

static void
gdi_ui_mem3blt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, int bgcolor, int fgcolor)
{
	DEBUG_GDI("gdi_ui_mem3blt opcode: 0x%X\n", rop3_code_table[opcode]);
}

static RD_HPALETTE
gdi_ui_create_palette(struct rdp_inst * inst, RD_PALETTE * colors)
{
	int i;
	HPALETTE palette;
	LOGPALETTE *logicalPalette;

	DEBUG_GDI("gdi_ui_create_palette\n");
	
	logicalPalette = (LOGPALETTE*) malloc(sizeof(LOGPALETTE));

	logicalPalette->entries = (PALETTEENTRY*) malloc(sizeof(PALETTEENTRY) * 256);
	memset(logicalPalette->entries, 0, sizeof(PALETTEENTRY) * 256);
	logicalPalette->count = colors->ncolors;

	if (logicalPalette->count > 256)
		logicalPalette->count = 256;

	for (i = logicalPalette->count - 1; i >= 0; i--)
	{
		logicalPalette->entries[i].red = colors->colors[i].red;
		logicalPalette->entries[i].green = colors->colors[i].green;
		logicalPalette->entries[i].blue = colors->colors[i].blue;
	}

	palette = CreatePalette(logicalPalette);

	return (RD_HPALETTE) palette;
}

static void
gdi_ui_set_palette(struct rdp_inst * inst, RD_HPALETTE palette)
{
	GDI *gdi = GET_GDI(inst);

	DEBUG_GDI("gdi_ui_set_palette\n");
	gdi->palette = (HPALETTE) palette;
}

static void
gdi_ui_set_clipping_region(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
	GDI *gdi = GET_GDI(inst);
	SetClipRgn(gdi->drawing->hdc, x, y, cx, cy);
}

static void
gdi_ui_reset_clipping_region(struct rdp_inst * inst)
{
	GDI *gdi = GET_GDI(inst);
	SetNullClipRgn(gdi->drawing->hdc);
}

static RD_HBITMAP
gdi_ui_create_surface(struct rdp_inst * inst, int width, int height, RD_HBITMAP old_surface)
{
	gdi_bitmap *gdi_bmp;
	gdi_bitmap *old_gdi_bmp;
	GDI *gdi = GET_GDI(inst);

	gdi_bmp = gdi_bitmap_new(gdi, width, height, 0, 0, NULL);
	old_gdi_bmp = (gdi_bitmap*) old_surface;
	
	if (old_gdi_bmp != 0)
	{
		gdi_bitmap_free(old_gdi_bmp);
	}

	if (gdi->drawing == old_gdi_bmp)
	{
		gdi->drawing = gdi_bmp;
	}
	
	DEBUG_GDI("ui_create_surface\n");
	
	return (RD_HBITMAP) gdi_bmp;
}

static void
gdi_ui_switch_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	GDI *gdi = GET_GDI(inst);

	DEBUG_GDI("ui_switch_surface\n");
	
	if (surface != 0)
	{
		gdi->drawing = (gdi_bitmap*) surface;
	}
	else
	{
		gdi->drawing = (gdi_bitmap*) gdi->primary;
	}
}

static void
gdi_ui_destroy_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	GDI *gdi = GET_GDI(inst);

	DEBUG_GDI("ui_destroy_surface\n");
	
	if (gdi->drawing == surface)
	{
		gdi->drawing = gdi->primary;
	}
	
	if (surface != 0)
	{
		gdi_bitmap_free((gdi_bitmap*) surface);
	}
}

static int
gdi_register_callbacks(rdpInst * inst)
{
	inst->ui_desktop_save = gdi_ui_desktop_save;
	inst->ui_desktop_restore = gdi_ui_desktop_restore;
	inst->ui_create_bitmap = gdi_ui_create_bitmap;
	inst->ui_paint_bitmap = gdi_ui_paint_bitmap;
	inst->ui_destroy_bitmap = gdi_ui_destroy_bitmap;
	inst->ui_line = gdi_ui_line;
	inst->ui_rect = gdi_ui_rect;
	inst->ui_polygon = gdi_ui_polygon;
	inst->ui_polyline = gdi_ui_polyline;
	inst->ui_ellipse = gdi_ui_ellipse;
	inst->ui_start_draw_glyphs = gdi_ui_start_draw_glyphs;
	inst->ui_draw_glyph = gdi_ui_draw_glyph;
	inst->ui_end_draw_glyphs = gdi_ui_end_draw_glyphs;
	inst->ui_destblt = gdi_ui_destblt;
	inst->ui_patblt = gdi_ui_patblt;
	inst->ui_screenblt = gdi_ui_screenblt;
	inst->ui_memblt = gdi_ui_memblt;
	inst->ui_triblt = gdi_ui_mem3blt;
	inst->ui_create_colormap = gdi_ui_create_palette;
	inst->ui_set_colormap = gdi_ui_set_palette;
	inst->ui_create_glyph = gdi_ui_create_glyph;
	inst->ui_destroy_glyph = gdi_ui_destroy_glyph;
	inst->ui_set_clip = gdi_ui_set_clipping_region;
	inst->ui_reset_clip = gdi_ui_reset_clipping_region;
	inst->ui_create_surface = gdi_ui_create_surface;
	inst->ui_set_surface = gdi_ui_switch_surface;
	inst->ui_destroy_surface = gdi_ui_destroy_surface;
	return 0;
}

int
gdi_init(rdpInst * inst)
{
	GDI *gdi = (GDI*) malloc(sizeof(GDI));
	memset(gdi, 0, sizeof(GDI));
	SET_GDI(inst, gdi);

	gdi->width = inst->settings->width;
	gdi->height = inst->settings->height;

	gdi->dstBpp = 32;
	gdi->srcBpp = inst->settings->server_depth;
	
	gdi->hdc = GetDC();
	gdi->hdc->bytesPerPixel = 4;
	gdi->hdc->bitsPerPixel = gdi->dstBpp;

	gdi->primary = gdi_bitmap_new(gdi, gdi->width, gdi->height, gdi->dstBpp, 0, NULL);
	gdi->primary_buffer = gdi->primary->bitmap->data;
	gdi->drawing = gdi->primary;

	gdi->primary->hdc->hwnd = (HWND) malloc(sizeof(WND));
	gdi->primary->hdc->hwnd->invalid = CreateRectRgn(0, 0, 0, 0);
	gdi->primary->hdc->hwnd->invalid->null = 1;

	gdi_register_callbacks(inst);

	return 0;
}
