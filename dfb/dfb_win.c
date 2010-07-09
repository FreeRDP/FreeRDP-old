/*
   Copyright (c) 2009 Jay Sorg

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>
#include <freerdp/chanman.h>
#include "dfb_win.h"
#include "dfb_gdi.h"
#include "dfb_event.h"
#include "dfb_colour.h"
#include "dfb_keyboard.h"

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

static void
l_ui_error(struct rdp_inst * inst, char * text)
{
	//printf("ui_error: %s", text);
}

static void
l_ui_warning(struct rdp_inst * inst, char * text)
{
	//printf("ui_warning: %s\n", text);
}

static void
l_ui_unimpl(struct rdp_inst * inst, char * text)
{
	//printf("ui_unimpl: %s\n", text);
}

static void
dfb_update_screen(dfbInfo * dfbi)
{
	if (dfbi->update_rect.w > 0 && dfbi->update_rect.h > 0)
	{
		dfbi->primary->Blit(dfbi->primary, dfbi->screen_surface,
	                    &(dfbi->update_rect), dfbi->update_rect.x, dfbi->update_rect.y);
		
		dfbi->update_rect.x = 0;
		dfbi->update_rect.y = 0;
		dfbi->update_rect.w = 0;
		dfbi->update_rect.h = 0;
	}
}

static void
dfb_invalidate_rect(dfbInfo * dfbi, int x1, int y1, int x2, int y2)
{
	if (dfbi->update_rect.w == 0 && dfbi->update_rect.h == 0)
	{
		dfbi->update_rect.x = x1;
		dfbi->update_rect.y = y1;
		dfbi->update_rect.w = x2 - x1;
		dfbi->update_rect.h = y2 - y1;
	}
	else
	{
		int right = dfbi->update_rect.x + dfbi->update_rect.w;
		int bottom = dfbi->update_rect.y + dfbi->update_rect.h;
		
		if (x1 < dfbi->update_rect.x)
			dfbi->update_rect.x = x1;

		if (y1 < dfbi->update_rect.y)
			dfbi->update_rect.y = y1;

		if (x2 > right)
			right = x2;

		if (y2 > bottom)
			bottom = y2;

		dfbi->update_rect.w = right - dfbi->update_rect.x;
		dfbi->update_rect.h = bottom - dfbi->update_rect.y;
	}
}

static void
l_ui_begin_update(struct rdp_inst * inst)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	dfb_update_screen(dfbi);
}

static void
l_ui_end_update(struct rdp_inst * inst)
{
}

static void
l_ui_desktop_save(struct rdp_inst * inst, int offset, int x, int y, int cx, int cy)
{
}

static void
l_ui_desktop_restore(struct rdp_inst * inst, int offset, int x, int y, int cx, int cy)
{
}

static RD_HGLYPH
l_ui_create_glyph(struct rdp_inst * inst, int width, int height, uint8 * data)
{	
	return (RD_HGLYPH) NULL;
}

static void
l_ui_destroy_glyph(struct rdp_inst * inst, RD_HGLYPH glyph)
{
}

static RD_HBITMAP
l_ui_create_bitmap(struct rdp_inst * inst, int width, int height, uint8 * data)
{
	HBITMAP bitmap;
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	bitmap = CreateBitmap(width, height, dfbi->bpp, (char*) dfb_image_convert(dfbi, inst->settings, width, height, (uint8*) data));
	return (RD_HBITMAP) bitmap;
}

static void
l_ui_paint_bitmap(struct rdp_inst * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
	HBITMAP bitmap;
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	bitmap = (HBITMAP) inst->ui_create_bitmap(inst, width, height, data);
	SelectObject(dfbi->hdcBmp, (HGDIOBJ) bitmap);	
	BitBlt(dfbi->hdc, x, y, cx, cy, dfbi->hdcBmp, 0, 0, SRCCOPY);
	
	if (dfbi->surface == dfbi->backingstore)
	{
		dfb_invalidate_rect(dfbi, x, y, x + cx, y + cy);
		dfb_update_screen(dfbi);
	}
}

static void
l_ui_destroy_bitmap(struct rdp_inst * inst, RD_HBITMAP bmp)
{
	DeleteObject((HGDIOBJ) bmp);
}

static void
l_ui_line(struct rdp_inst * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen)
{
	printf("ui_line\n");
}

static void
l_ui_rect(struct rdp_inst * inst, int x, int y, int cx, int cy, int colour)
{
	RECT rect;
	HBRUSH hBrush;
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	
	printf("ui_rect: x: %d y: %d cx: %d cy: %d\n", x, y, cx, cy);
	
	rect.left = x;
	rect.top = y;
	rect.right = x + cx;
	rect.bottom = y + cy;
	
	dfb_colour_convert(dfbi, colour, &(dfbi->pixel), inst->settings->server_depth, dfbi->bpp);
	hBrush = CreateSolidBrush((COLORREF) dfb_make_colorref(&(dfbi->pixel), dfbi->bpp));
	FillRect(dfbi->hdc, &rect, hBrush);

	if (dfbi->surface == dfbi->backingstore)
	{
		dfb_invalidate_rect(dfbi, x, y, x + cx, y + cy);
		dfb_update_screen(dfbi);
	}
}

static void
l_ui_polygon(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point,
	int npoints, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_polygon\n");
}

static void
l_ui_polyline(struct rdp_inst * inst, uint8 opcode, RD_POINT * points, int npoints, RD_PEN * pen)
{
	printf("ui_polyline\n");
}

static void
l_ui_ellipse(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, int x, int y,
	int cx, int cy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_ellipse\n");
}

static void
l_ui_start_draw_glyphs(struct rdp_inst * inst, int bgcolour, int fgcolour)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	dfb_colour_convert(dfbi, bgcolour, &(dfbi->bgcolour), inst->settings->server_depth, dfbi->bpp);
	dfb_colour_convert(dfbi, fgcolour, &(dfbi->fgcolour), inst->settings->server_depth, dfbi->bpp);
}

static void
l_ui_draw_glyph(struct rdp_inst * inst, int x, int y, int cx, int cy, RD_HGLYPH glyph)
{
}

static void
l_ui_end_draw_glyphs(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
}

static uint32
l_ui_get_toggle_keys_state(struct rdp_inst * inst)
{
	return 0;
}

static void
l_ui_bell(struct rdp_inst * inst)
{
}

static void
l_ui_destblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	printf("ui_destblt: x: %d y: %d cx: %d cy: %d rop: 0x%X\n", x, y, cx, cy, rop3_code_table[opcode]);
	
	BitBlt(dfbi->hdc, x, y, cx, cy, NULL, 0, 0, rop3_code_table[opcode]);

	if (dfbi->surface == dfbi->backingstore)
	{
		dfb_invalidate_rect(dfbi, x, y, x + cx, y + cy);
		dfb_update_screen(dfbi);
	}
}

static void
l_ui_patblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	dfb_colour_convert(dfbi, bgcolour, &(dfbi->bgcolour), inst->settings->server_depth, dfbi->bpp);
	dfb_colour_convert(dfbi, fgcolour, &(dfbi->fgcolour), inst->settings->server_depth, dfbi->bpp);

	if (brush->style == BS_PATTERN)
	{
		HBITMAP hBmp;
		HBRUSH hBrush;
		int originalBkMode;
		COLORREF originalBkColor;
		COLORREF originalTextColor;
		HGDIOBJ originalSelectedObject;

		printf("BS_PATTERN\n");
		
		hBmp = CreateBitmap(8, 8, dfbi->hdc->bitsPerPixel,
		                            (char*) dfb_image_convert(dfbi, inst->settings, 8, 8, (uint8*) brush->bd->data));
		
		hBrush = CreatePatternBrush(hBmp);

		originalBkMode = SetBkMode(dfbi->hdc, OPAQUE);
		originalBkColor = SetBkColor(dfbi->hdc, (COLORREF) dfb_make_colorref(&(dfbi->bgcolour), dfbi->bpp));
		originalTextColor = SetTextColor(dfbi->hdc, (COLORREF) dfb_make_colorref(&(dfbi->fgcolour), dfbi->bpp));
		originalSelectedObject = SelectObject(dfbi->hdc, (HGDIOBJ) hBrush);
		PatBlt(dfbi->hdc, x, y, cx, cy, rop3_code_table[opcode]);
		SetBkMode(dfbi->hdc, originalBkMode);
		SetBkColor(dfbi->hdc, originalBkColor);
		SetTextColor(dfbi->hdc, originalTextColor);
		SelectObject(dfbi->hdc, originalSelectedObject);

		if (dfbi->surface == dfbi->backingstore)
		{
			dfb_invalidate_rect(dfbi, x, y, x + cx, y + cy);
			dfb_update_screen(dfbi);
		}
	}
	else if (brush->style == BS_SOLID)
	{
		printf("BS_SOLID\n");
	}
	else if (brush->style == BS_HATCHED)
	{
		printf("BS_HATCHED\n");
	}
	else if (brush->style == BS_NULL)
	{
		printf("BS_NULL\n");
	}
	else
	{
		printf("unknown brush style: %d\n", brush->style);
	}
}

static void
l_ui_screenblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy)
{
	printf("ui_screenblt rop: 0x%X\n", rop3_code_table[opcode]);
}

static void
l_ui_memblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	SelectObject(dfbi->hdcBmp, (HGDIOBJ) src);
	BitBlt(dfbi->hdc, x, y, cx, cy, dfbi->hdcBmp, srcx, srcy, rop3_code_table[opcode]);
	//printf("memblt x: %d y: %d x + cx: %d y + cy: %d\n", x, y, x + cx, y + cy);

	if (dfbi->surface == dfbi->backingstore)
	{
		dfb_invalidate_rect(dfbi, x, y, x + cx, y + cy);
		dfb_update_screen(dfbi);
	}
}

static void
l_ui_triblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_triblt opcode: 0x%X\n", rop3_code_table[opcode]);
}

static int
l_ui_select(struct rdp_inst * inst, int rdp_socket)
{
	return 1;
}

static void
l_ui_set_clip(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
	dfbInfo * dfbi;
	HRGN clippingRegion;
	dfbi = GET_DFBI(inst);
	printf("ui_set_clip: x: %d y: %d cx: %d cy: %d\n", x, y, cx, cy);
	clippingRegion = CreateRectRgn(x, y, x + cx, y + cy);
	SelectClipRgn(dfbi->hdc, clippingRegion);
}

static void
l_ui_reset_clip(struct rdp_inst * inst)
{
	dfbInfo * dfbi;	
	dfbi = GET_DFBI(inst);
	SelectClipRgn(dfbi->hdc, NULL);
	printf("ui_reset_clip\n");
}

static void
l_ui_resize_window(struct rdp_inst * inst)
{
}

static void
l_ui_set_cursor(struct rdp_inst * inst, RD_HCURSOR cursor)
{
}

static void
l_ui_destroy_cursor(struct rdp_inst * inst, RD_HCURSOR cursor)
{
}

static RD_HCURSOR
l_ui_create_cursor(struct rdp_inst * inst, uint32 x, uint32 y,
                   int width, int height, uint8 * andmask, uint8 * xormask, int bpp)
{
	return (RD_HCURSOR) NULL;
}

static void
l_ui_set_null_cursor(struct rdp_inst * inst)
{
}

static void
l_ui_set_default_cursor(struct rdp_inst * inst)
{
}

static void
l_ui_move_pointer(struct rdp_inst * inst, int x, int y)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	dfbi->cursor_x = x;
	dfbi->cursor_y = y;
}

static RD_HCOLOURMAP
l_ui_create_colourmap(struct rdp_inst * inst, RD_COLOURMAP * colours)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	return dfb_create_colourmap(dfbi, inst->settings, colours);
}

static void
l_ui_set_colourmap(struct rdp_inst * inst, RD_HCOLOURMAP map)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	dfb_set_colourmap(dfbi, inst->settings, map);
}

static RD_HBITMAP
l_ui_create_surface(struct rdp_inst * inst, int width, int height, RD_HBITMAP old_surface)
{
	dfbInfo * dfbi;
	HBITMAP new_bitmap;
	HBITMAP old_bitmap;
	dfbi = GET_DFBI(inst);
	
	//printf("ui_create_surface\n");
	new_bitmap = CreateCompatibleBitmap(dfbi->hdc, width, height);
	old_bitmap = (HBITMAP) old_surface;

	if (old_bitmap != 0)
	{
		SelectObject(dfbi->hdcBmp, (HGDIOBJ) old_bitmap);
		SelectObject(dfbi->hdc, (HGDIOBJ) new_bitmap);
		//BitBlt(dfbi->hdc, 0, 0, width, height, dfbi->hdcBmp, 0, 0, SRCCOPY);
		DeleteObject((HGDIOBJ) old_bitmap);
	}
	else
	{
		dfbi->surface = new_bitmap;
	}
	
	return (RD_HBITMAP) new_bitmap;
}

static void
l_ui_set_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	//printf("ui_set_surface\n");
	
	if (surface != 0)
	{
		dfbi->surface = surface;
		SelectObject(dfbi->hdc, (HGDIOBJ) dfbi->surface);
	}
	else
	{
		dfbi->surface = dfbi->backingstore;
		SelectObject(dfbi->hdc, (HGDIOBJ) dfbi->surface);
	}
}

static void
l_ui_destroy_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	DeleteObject((HGDIOBJ) surface);
}

static void
l_ui_channel_data(struct rdp_inst * inst, int chan_id, char * data, int data_size, int flags, int total_size)
{
}

static int
dfb_assign_callbacks(rdpInst * inst)
{
	inst->ui_error = l_ui_error;
	inst->ui_warning = l_ui_warning;
	inst->ui_unimpl = l_ui_unimpl;
	inst->ui_begin_update = l_ui_begin_update;
	inst->ui_end_update = l_ui_end_update;
	inst->ui_desktop_save = l_ui_desktop_save;
	inst->ui_desktop_restore = l_ui_desktop_restore;
	inst->ui_create_bitmap = l_ui_create_bitmap;
	inst->ui_paint_bitmap = l_ui_paint_bitmap;
	inst->ui_destroy_bitmap = l_ui_destroy_bitmap;
	inst->ui_line = l_ui_line;
	inst->ui_rect = l_ui_rect;
	inst->ui_polygon = l_ui_polygon;
	inst->ui_polyline = l_ui_polyline;
	inst->ui_ellipse = l_ui_ellipse;
	inst->ui_start_draw_glyphs = l_ui_start_draw_glyphs;
	inst->ui_draw_glyph = l_ui_draw_glyph;
	inst->ui_end_draw_glyphs = l_ui_end_draw_glyphs;
	inst->ui_get_toggle_keys_state = l_ui_get_toggle_keys_state;
	inst->ui_bell = l_ui_bell;
	inst->ui_destblt = l_ui_destblt;
	inst->ui_patblt = l_ui_patblt;
	inst->ui_screenblt = l_ui_screenblt;
	inst->ui_memblt = l_ui_memblt;
	inst->ui_triblt = l_ui_triblt;
	inst->ui_create_glyph = l_ui_create_glyph;
	inst->ui_destroy_glyph = l_ui_destroy_glyph;
	inst->ui_select = l_ui_select;
	inst->ui_set_clip = l_ui_set_clip;
	inst->ui_reset_clip = l_ui_reset_clip;
	inst->ui_resize_window = l_ui_resize_window;
	inst->ui_set_cursor = l_ui_set_cursor;
	inst->ui_destroy_cursor = l_ui_destroy_cursor;
	inst->ui_create_cursor = l_ui_create_cursor;
	inst->ui_set_null_cursor = l_ui_set_null_cursor;
	inst->ui_set_default_cursor = l_ui_set_default_cursor;
	inst->ui_create_colourmap = l_ui_create_colourmap;
	inst->ui_move_pointer = l_ui_move_pointer;
	inst->ui_set_colourmap = l_ui_set_colourmap;
	inst->ui_create_surface = l_ui_create_surface;
	inst->ui_set_surface = l_ui_set_surface;
	inst->ui_destroy_surface = l_ui_destroy_surface;
	inst->ui_channel_data = l_ui_channel_data;
	return 0;
}

void
dfb_init(int *argc, char *(*argv[]))
{
	DFBResult err;
	err = DirectFBInit(argc, argv);
}

int
dfb_pre_connect(rdpInst * inst)
{
	dfbInfo * dfbi;
	dfb_assign_callbacks(inst);
	dfbi = (dfbInfo *) malloc(sizeof(dfbInfo));
	SET_DFBI(inst, dfbi);
	memset(dfbi, 0, sizeof(dfbInfo));
	dfbi->bytes_per_pixel = 4;
	dfbi->bpp = 32;
	return 0;
}

int
dfb_post_connect(rdpInst * inst)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);

	dfbi->err = DirectFBCreate(&(dfbi->dfb));

	dfbi->dsc.flags = DSDESC_CAPS;
	dfbi->dsc.caps = DSCAPS_PRIMARY;
	dfbi->err = dfbi->dfb->CreateSurface(dfbi->dfb, &(dfbi->dsc), &(dfbi->primary));
	dfbi->err = dfbi->primary->GetSize(dfbi->primary, &(dfbi->width), &(dfbi->height));
	dfbi->dfb->SetVideoMode(dfbi->dfb, dfbi->width, dfbi->height, dfbi->bpp);
	dfbi->dfb->CreateInputEventBuffer(dfbi->dfb, DICAPS_ALL, 0, &(dfbi->event));

	dfbi->dfb->GetDisplayLayer(dfbi->dfb, 0, &(dfbi->layer));
	dfbi->layer->EnableCursor(dfbi->layer, 1);
	
	dfbi->width = inst->settings->width;
	dfbi->height = inst->settings->height;
	dfbi->screen = malloc(dfbi->width * dfbi->height * dfbi->bytes_per_pixel);
	dfbi->dsc.flags = DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PREALLOCATED | DSDESC_PIXELFORMAT;
	dfbi->dsc.caps = DSCAPS_SYSTEMONLY;
	dfbi->dsc.width = dfbi->width;
	dfbi->dsc.height = dfbi->height;
	dfbi->dsc.pixelformat = DSPF_AiRGB;
	dfbi->dsc.preallocated[0].data = dfbi->screen;
	dfbi->dsc.preallocated[0].pitch = dfbi->width * dfbi->bytes_per_pixel;
	dfbi->dfb->CreateSurface(dfbi->dfb, &(dfbi->dsc), &(dfbi->screen_surface));

	dfbi->hdc = GetDC();
	dfbi->hdc->bitsPerPixel = dfbi->bpp;
	dfbi->hdc->bytesPerPixel = 4;

	dfbi->backingstore = CreateBitmap(inst->settings->width, inst->settings->height, dfbi->bpp, (char*) dfbi->screen);
	SelectObject(dfbi->hdc, (HGDIOBJ) dfbi->backingstore);
	dfbi->hdcBmp = CreateCompatibleDC(dfbi->hdc);
	
	return 0;
}

void
dfb_uninit(void * dfb_info)
{
	dfbInfo * dfbi;
	dfbi = (dfbInfo *) dfb_info;
	dfbi->primary->Release(dfbi->primary);
	dfbi->dfb->Release(dfbi->dfb);
}

int
dfb_get_fds(rdpInst * inst, void ** read_fds, int * read_count, void ** write_fds, int * write_count)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	dfbi->event->CreateFileDescriptor(dfbi->event, read_fds[*read_count]);
	(*read_count)++;
	return 0;
}

int
dfb_check_fds(rdpInst * inst)
{
	dfbInfo * dfbi;
	dfbi = GET_DFBI(inst);
	
	while (dfbi->event->HasEvent(dfbi->event) == DFB_OK)
	{
		if (dfbi->event->GetEvent(dfbi->event, &(dfbi->events[0])) == 0)
		{
			dfb_process_event(inst, &(dfbi->events[0]));
		}
	}

	return 0;
}
