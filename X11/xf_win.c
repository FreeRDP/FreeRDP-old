/*
   FreeRDP: A Remote Desktop Protocol client.
   UI window

   Copyright (C) Jay Sorg 2009-2011

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

#define USE_XCURSOR

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef USE_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <freerdp/chanman.h>

#include "xf_types.h"
#include "xf_event.h"
#include "xf_keyboard.h"
#include "xf_win.h"
#include "xf_decode.h"
#include "color.h"
#include "gdi_palette.h"

#ifdef HAVE_LIBXINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#define MWM_HINTS_DECORATIONS   (1L << 1)
#define PROP_MOTIF_WM_HINTS_ELEMENTS    5

typedef struct
{
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long inputMode;
	unsigned long status;
}
PropMotifWmHints;

static uint8 g_hatch_patterns[] = {
	0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, /* 0 - bsHorizontal */
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, /* 1 - bsVertical */
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, /* 2 - bsFDiagonal */
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, /* 3 - bsBDiagonal */
	0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08, /* 4 - bsCross */
	0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81  /* 5 - bsDiagCross */
};

/* @msdn{cc241582} */
static const uint8 g_rop2_map[] = {
	GXclear,        /* 0 */
	GXnor,          /* DPon */
	GXandInverted,  /* DPna */
	GXcopyInverted, /* Pn */
	GXandReverse,   /* PDna */
	GXinvert,       /* Dn */
	GXxor,          /* DPx */
	GXnand,         /* DPan */
	GXand,          /* DPa */
	GXequiv,        /* DPxn */
	GXnoop,         /* D */
	GXorInverted,   /* DPno */
	GXcopy,         /* P */
	GXorReverse,    /* PDno */
	GXor,           /* DPo */
	GXset           /* 1 */
};

static int
xf_set_rop2(xfInfo * xfi, int rop2)
{
	if ((rop2 < 0x01) || (rop2 > 0x10))
	{
		printf("xf_set_rop2: unknown rop2 %x\n", rop2);
		return 0;
	}
	XSetFunction(xfi->display, xfi->gc, g_rop2_map[rop2 - 1]);
	return 0;
}

static int
xf_set_rop3(xfInfo * xfi, int rop3)
{
	switch (rop3)
	{
		case 0x00: /* 0 - 0 */
			XSetFunction(xfi->display, xfi->gc, GXclear);
			break;
		case 0x05: /* ~(P | D) - DPon */
			XSetFunction(xfi->display, xfi->gc, GXnor);
			break;
		case 0x0a: /* ~P & D - DPna */
			XSetFunction(xfi->display, xfi->gc, GXandInverted);
			break;
		case 0x0f: /* ~P - Pn */
			XSetFunction(xfi->display, xfi->gc, GXcopyInverted);
			break;
		case 0x11: /* ~(S | D) - DSon */
			XSetFunction(xfi->display, xfi->gc, GXnor);
			break;
		case 0x22: /* ~S & D - DSna */
			XSetFunction(xfi->display, xfi->gc, GXandInverted);
			break;
		case 0x33: /* ~S - Sn */
			XSetFunction(xfi->display, xfi->gc, GXcopyInverted);
			break;
		case 0x44: /* S & ~D - SDna */
			XSetFunction(xfi->display, xfi->gc, GXandReverse);
			break;
		case 0x50: /* P & ~D - PDna */
			XSetFunction(xfi->display, xfi->gc, GXandReverse);
			break;
		case 0x55: /* ~D - Dn */
			XSetFunction(xfi->display, xfi->gc, GXinvert);
			break;
		case 0x5a: /* D ^ P - DPx */
			XSetFunction(xfi->display, xfi->gc, GXxor);
			break;
		case 0x5f: /* ~(P & D) - DPan */
			XSetFunction(xfi->display, xfi->gc, GXnand);
			break;
		case 0x66: /* D ^ S - DSx */
			XSetFunction(xfi->display, xfi->gc, GXxor);
			break;
		case 0x77: /* ~(S & D) - DSan */
			XSetFunction(xfi->display, xfi->gc, GXnand);
			break;
		case 0x88: /* D & S - DSa */
			XSetFunction(xfi->display, xfi->gc, GXand);
			break;
		case 0x99: /* ~(S ^ D) - DSxn */
			XSetFunction(xfi->display, xfi->gc, GXequiv);
			break;
		case 0xa0: /* P & D - DPa */
			XSetFunction(xfi->display, xfi->gc, GXand);
			break;
		case 0xa5: /* ~(P ^ D) - PDxn */
			XSetFunction(xfi->display, xfi->gc, GXequiv);
			break;
		case 0xaa: /* D - D */
			XSetFunction(xfi->display, xfi->gc, GXnoop);
			break;
		case 0xaf: /* ~P | D - DPno */
			XSetFunction(xfi->display, xfi->gc, GXorInverted);
			break;
		case 0xbb: /* ~S | D - DSno */
			XSetFunction(xfi->display, xfi->gc, GXorInverted);
			break;
		case 0xcc: /* S - S */
			XSetFunction(xfi->display, xfi->gc, GXcopy);
			break;
		case 0xdd: /* S | ~D - SDno */
			XSetFunction(xfi->display, xfi->gc, GXorReverse);
			break;
		case 0xee: /* D | S - DSo */
			XSetFunction(xfi->display, xfi->gc, GXor);
			break;
		case 0xf0: /* P - P */
			XSetFunction(xfi->display, xfi->gc, GXcopy);
			break;
		case 0xf5: /* P | ~D - PDno */
			XSetFunction(xfi->display, xfi->gc, GXorReverse);
			break;
		case 0xfa: /* P | D - DPo */
			XSetFunction(xfi->display, xfi->gc, GXor);
			break;
		case 0xff: /* 1 - 1 */
			XSetFunction(xfi->display, xfi->gc, GXset);
			break;
		default:
			printf("xf_set_rop3: unknonw rop3 %x\n", rop3);
			break;
	}
	return 0;
}

static void
l_ui_error(struct rdp_inst * inst, const char * text)
{
	printf("ui_error: %s", text);
}

static void
l_ui_warning(struct rdp_inst * inst, const char * text)
{
	printf("ui_warning: %s\n", text);
}

static void
l_ui_unimpl(struct rdp_inst * inst, const char * text)
{
	printf("ui_unimpl: %s\n", text);
}

static void
l_ui_begin_update(struct rdp_inst * inst)
{
}

static void
l_ui_end_update(struct rdp_inst * inst)
{
	xfInfo * xfi = GET_XFI(inst);
	XFlush(xfi->display);
}

static void
l_ui_gdi_begin_update(struct rdp_inst * inst)
{
	GDI *gdi = GET_GDI(inst);
	gdi->primary->hdc->hwnd->invalid->null = 1;
}

static void
l_ui_gdi_end_update(struct rdp_inst * inst)
{
	XImage * image;
	GDI *gdi = GET_GDI(inst);
	xfInfo * xfi = GET_XFI(inst);

	if (gdi->primary->hdc->hwnd->invalid->null)
		return;

	image = XCreateImage(xfi->display, xfi->visual, xfi->depth, ZPixmap, 0,
			(char *) gdi->primary_buffer, gdi->width, gdi->height, xfi->bitmap_pad, 0);

	XPutImage(xfi->display, xfi->backstore, xfi->gc_default, image, 0, 0, 0, 0, gdi->width, gdi->height);

	XCopyArea(xfi->display, xfi->backstore, xfi->wnd, xfi->gc_default,
			gdi->primary->hdc->hwnd->invalid->x,
			gdi->primary->hdc->hwnd->invalid->y,
			gdi->primary->hdc->hwnd->invalid->w,
			gdi->primary->hdc->hwnd->invalid->h,
			gdi->primary->hdc->hwnd->invalid->x,
			gdi->primary->hdc->hwnd->invalid->y);

	XFlush(xfi->display);

	XFree(image);
}


static void
l_ui_desktop_save(struct rdp_inst * inst, int offset, int x, int y,
	int cx, int cy)
{
	printf("ui_desktop_save:\n");
}

static void
l_ui_desktop_restore(struct rdp_inst * inst, int offset, int x, int y,
	int cx, int cy)
{
	printf("ui_desktop_restore:\n");
}

static RD_HGLYPH
l_ui_create_glyph(struct rdp_inst * inst, int width, int height, uint8 * data)
{
	int scanline;
	Pixmap bitmap;
	XImage * image;
	xfInfo * xfi = GET_XFI(inst);

	scanline = (width + 7) / 8;

	bitmap = XCreatePixmap(xfi->display, xfi->wnd, width, height, 1);

	image = XCreateImage(xfi->display, xfi->visual, 1,
			ZPixmap, 0, (char *) data, width, height, 8, scanline);

	image->byte_order = MSBFirst;
	image->bitmap_bit_order = MSBFirst;

	XInitImage(image);
	XPutImage(xfi->display, bitmap, xfi->gc_mono, image, 0, 0, 0, 0, width, height);
	XFree(image);

	return (RD_HGLYPH) bitmap;
}

static void
l_ui_destroy_glyph(struct rdp_inst * inst, RD_HGLYPH glyph)
{
	xfInfo * xfi = GET_XFI(inst);
	XFreePixmap(xfi->display, (Pixmap) glyph);
}

static RD_HBITMAP
l_ui_create_bitmap(struct rdp_inst * inst, int width, int height, uint8 * data)
{
	Pixmap bitmap;
	uint8 * cdata;
	XImage * image;
	xfInfo * xfi = GET_XFI(inst);

	bitmap = XCreatePixmap(xfi->display, xfi->wnd, width, height, xfi->depth);

	cdata = gdi_image_convert(data, NULL, width, height, inst->settings->server_depth, xfi->bpp, xfi->clrconv);

	image = XCreateImage(xfi->display, xfi->visual, xfi->depth,
			ZPixmap, 0, (char *) cdata, width, height, xfi->bitmap_pad, 0);

	XPutImage(xfi->display, bitmap, xfi->gc_default, image, 0, 0, 0, 0, width, height);
	XFree(image);

	if (cdata != data)
		free(cdata);

	return (RD_HBITMAP) bitmap;
}

static void
l_ui_paint_bitmap(struct rdp_inst * inst, int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
	uint8 * cdata;
	XImage * image;
	xfInfo * xfi = GET_XFI(inst);

	cdata = gdi_image_convert(data, NULL, width, height, inst->settings->server_depth, xfi->bpp, xfi->clrconv);

	image = XCreateImage(xfi->display, xfi->visual, xfi->depth,
			ZPixmap, 0, (char *) cdata, width, height, xfi->bitmap_pad, 0);

	XPutImage(xfi->display, xfi->backstore, xfi->gc_default, image, 0, 0, x, y, cx, cy);
	XCopyArea(xfi->display, xfi->backstore, xfi->wnd, xfi->gc_default, x, y, cx, cy, x, y);

	XFree(image);

	if (cdata != data)
		free(cdata);
}

static void
l_ui_destroy_bitmap(struct rdp_inst * inst, RD_HBITMAP bmp)
{
	xfInfo * xfi = GET_XFI(inst);
	XFreePixmap(xfi->display, (Pixmap) bmp);
}

static void
l_ui_line(struct rdp_inst * inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN * pen)
{
	uint32 color;
	xfInfo * xfi = GET_XFI(inst);

	xf_set_rop2(xfi, opcode);
	color = gdi_color_convert(pen->color, inst->settings->server_depth, xfi->bpp, xfi->clrconv);

	XSetFillStyle(xfi->display, xfi->gc, FillSolid);
	XSetForeground(xfi->display, xfi->gc, color);
	XDrawLine(xfi->display, xfi->drw, xfi->gc, startx, starty, endx, endy);

	if (xfi->drw == xfi->backstore)
	{
		XDrawLine(xfi->display, xfi->wnd, xfi->gc, startx, starty, endx, endy);
	}
}

static void
l_ui_rect(struct rdp_inst * inst, int x, int y, int cx, int cy, uint32 color)
{
	xfInfo * xfi = GET_XFI(inst);

	color = gdi_color_convert(color, inst->settings->server_depth, xfi->bpp, xfi->clrconv);

	XSetFunction(xfi->display, xfi->gc, GXcopy);
	XSetFillStyle(xfi->display, xfi->gc, FillSolid);
	XSetForeground(xfi->display, xfi->gc, color);
	XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);

	if (xfi->drw == xfi->backstore)
	{
		XFillRectangle(xfi->display, xfi->wnd, xfi->gc, x, y, cx, cy);
	}
}

static void
l_ui_polygon(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point,
	int npoints, RD_BRUSH * brush, uint32 bgcolor, uint32 fgcolor)
{
	Pixmap fill;
	uint32 polygon_bgcolor;
	uint32 polygon_fgcolor;
	uint8 style, i, ipattern[8];
	xfInfo * xfi = GET_XFI(inst);

	xf_set_rop2(xfi, opcode);
	style = brush ? brush->style : 0;

	polygon_fgcolor = gdi_color_convert(fgcolor, inst->settings->server_depth, xfi->bpp, xfi->clrconv);
	polygon_bgcolor = gdi_color_convert(bgcolor, inst->settings->server_depth, xfi->bpp, xfi->clrconv);

	switch (fillmode)
	{
		case 1: /* alternate */
			XSetFillRule(xfi->display, xfi->gc, EvenOddRule);
			break;
		case 2: /* winding */
			XSetFillRule(xfi->display, xfi->gc, WindingRule);
			break;
		default:
			printf("fill mode %d\n", fillmode);
	}

	switch (style)
	{
		case 0:	/* Solid */
			XSetFillStyle(xfi->display, xfi->gc, FillSolid);
			XSetForeground(xfi->display, xfi->gc, polygon_fgcolor);
			XFillPolygon(xfi->display, xfi->drw, xfi->gc,
				(XPoint *) point, npoints, Complex, CoordModePrevious);
			if (xfi->drw == xfi->backstore)
			{
				XFillPolygon(xfi->display, xfi->wnd, xfi->gc,
					(XPoint *) point, npoints, Complex, CoordModePrevious);
			}
			break;

		case 2:	/* Hatch */
			fill = (Pixmap) l_ui_create_glyph(inst, 8, 8,
							g_hatch_patterns + brush->pattern[0] * 8);
			XSetForeground(xfi->display, xfi->gc, polygon_fgcolor);
			XSetBackground(xfi->display, xfi->gc, polygon_bgcolor);
			XSetFillStyle(xfi->display, xfi->gc, FillOpaqueStippled);
			XSetStipple(xfi->display, xfi->gc, fill);
			XSetTSOrigin(xfi->display, xfi->gc, brush->xorigin, brush->yorigin);
			XFillPolygon(xfi->display, xfi->drw, xfi->gc,
				(XPoint *) point, npoints, Complex, CoordModePrevious);
			if (xfi->drw == xfi->backstore)
			{
				XFillPolygon(xfi->display, xfi->wnd, xfi->gc,
					(XPoint *) point, npoints, Complex, CoordModePrevious);
			}
			XSetFillStyle(xfi->display, xfi->gc, FillSolid);
			XSetTSOrigin(xfi->display, xfi->gc, 0, 0);
			l_ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			break;

		case 3:	/* Pattern */
			if (brush->bd == 0)	/* rdp4 brush */
			{
				for (i = 0; i != 8; i++)
					ipattern[7 - i] = brush->pattern[i];
				fill = (Pixmap) l_ui_create_glyph(inst, 8, 8, ipattern);
				XSetForeground(xfi->display, xfi->gc, polygon_fgcolor);
				XSetBackground(xfi->display, xfi->gc, polygon_bgcolor);
				XSetFillStyle(xfi->display, xfi->gc, FillOpaqueStippled);
				XSetStipple(xfi->display, xfi->gc, fill);
				XSetTSOrigin(xfi->display, xfi->gc, brush->xorigin, brush->yorigin);
				XFillPolygon(xfi->display, xfi->drw, xfi->gc,
					(XPoint *) point, npoints, Complex, CoordModePrevious);
				if (xfi->drw == xfi->backstore)
				{
					XFillPolygon(xfi->display, xfi->wnd, xfi->gc,
						(XPoint *) point, npoints, Complex, CoordModePrevious);
				}
				XSetFillStyle(xfi->display, xfi->gc, FillSolid);
				XSetTSOrigin(xfi->display, xfi->gc, 0, 0);
				l_ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			}
			else if (brush->bd->color_code > 1)	/* > 1 bpp */
			{
				fill = (Pixmap) l_ui_create_bitmap(inst, 8, 8, brush->bd->data);
				XSetFillStyle(xfi->display, xfi->gc, FillTiled);
				XSetTile(xfi->display, xfi->gc, fill);
				XSetTSOrigin(xfi->display, xfi->gc, brush->xorigin, brush->yorigin);
				XFillPolygon(xfi->display, xfi->drw, xfi->gc,
					(XPoint *) point, npoints, Complex, CoordModePrevious);
				if (xfi->drw == xfi->backstore)
				{
					XFillPolygon(xfi->display, xfi->wnd, xfi->gc,
						(XPoint *) point, npoints, Complex, CoordModePrevious);
				}
				XSetFillStyle(xfi->display, xfi->gc, FillSolid);
				XSetTSOrigin(xfi->display, xfi->gc, 0, 0);
				l_ui_destroy_bitmap(inst, (RD_HBITMAP) fill);
			}
			else
			{
				fill = (Pixmap) l_ui_create_glyph(inst, 8, 8, brush->bd->data);
				XSetForeground(xfi->display, xfi->gc, polygon_fgcolor);
				XSetBackground(xfi->display, xfi->gc, polygon_bgcolor);
				XSetFillStyle(xfi->display, xfi->gc, FillOpaqueStippled);
				XSetStipple(xfi->display, xfi->gc, fill);
				XSetTSOrigin(xfi->display, xfi->gc, brush->xorigin, brush->yorigin);
				XFillPolygon(xfi->display, xfi->drw, xfi->gc,
					(XPoint *) point, npoints, Complex, CoordModePrevious);
				if (xfi->drw == xfi->backstore)
				{
					XFillPolygon(xfi->display, xfi->wnd, xfi->gc,
						(XPoint *) point, npoints, Complex, CoordModePrevious);
				}
				XSetFillStyle(xfi->display, xfi->gc, FillSolid);
				XSetTSOrigin(xfi->display, xfi->gc, 0, 0);
				l_ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			}
			break;

		default:
			printf("brush %d\n", brush->style);
	}

	XSetFunction(xfi->display, xfi->gc, GXcopy);
}

static void
l_ui_polyline(struct rdp_inst * inst, uint8 opcode, RD_POINT * points, int npoints, RD_PEN * pen)
{
	XPoint * pts;
	uint32 color;
	xfInfo * xfi = GET_XFI(inst);

	color = gdi_color_convert(pen->color, inst->settings->server_depth, xfi->bpp, xfi->clrconv);

	xf_set_rop2(xfi, opcode);
	XSetFillStyle(xfi->display, xfi->gc, FillSolid);
	XSetForeground(xfi->display, xfi->gc, color);

	pts = (XPoint *) points;
	XDrawLines(xfi->display, xfi->drw, xfi->gc, pts, npoints, CoordModePrevious);

	if (xfi->drw == xfi->backstore)
	{
		XDrawLines(xfi->display, xfi->wnd, xfi->gc, pts, npoints, CoordModePrevious);
	}
}

static void
l_ui_ellipse(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, int x, int y,
	int cx, int cy, RD_BRUSH * brush, uint32 bgcolor, uint32 fgcolor)
{
	printf("ui_ellipse:\n");
}

static void
l_ui_start_draw_glyphs(struct rdp_inst * inst, uint32 bgcolor, uint32 fgcolor)
{
	uint32 glyph_bgcolor;
	uint32 glyph_fgcolor;
	xfInfo * xfi = GET_XFI(inst);

	glyph_fgcolor = gdi_color_convert(fgcolor, inst->settings->server_depth, xfi->bpp, xfi->clrconv);
	glyph_bgcolor = gdi_color_convert(bgcolor, inst->settings->server_depth, xfi->bpp, xfi->clrconv);

	XSetFunction(xfi->display, xfi->gc, GXcopy);
	XSetForeground(xfi->display, xfi->gc, glyph_fgcolor);
	XSetBackground(xfi->display, xfi->gc, glyph_bgcolor);
	XSetFillStyle(xfi->display, xfi->gc, FillStippled);
}

static void
l_ui_draw_glyph(struct rdp_inst * inst, int x, int y, int cx, int cy, RD_HGLYPH glyph)
{
	xfInfo * xfi = GET_XFI(inst);

	XSetStipple(xfi->display, xfi->gc, (Pixmap) glyph);
	XSetTSOrigin(xfi->display, xfi->gc, x, y);
	XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
	XSetStipple(xfi->display, xfi->gc, xfi->bitmap_mono);
}

static void
l_ui_end_draw_glyphs(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
	xfInfo * xfi = GET_XFI(inst);

	if (xfi->drw == xfi->backstore)
	{
		XCopyArea(xfi->display, xfi->backstore, xfi->wnd, xfi->gc, x, y, cx, cy, x, y);
	}
}

static uint32
l_ui_get_toggle_keys_state(struct rdp_inst * inst)
{
	xfInfo * xfi = GET_XFI(inst);
	return xf_kb_get_toggle_keys_state(xfi);
}

static void
l_ui_bell(struct rdp_inst * inst)
{

}

static void
l_ui_destblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy)
{
	xfInfo * xfi = GET_XFI(inst);

	xf_set_rop3(xfi, opcode);
	XSetFillStyle(xfi->display, xfi->gc, FillSolid);
	XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);

	if (xfi->drw == xfi->backstore)
	{
		XFillRectangle(xfi->display, xfi->wnd, xfi->gc, x, y, cx, cy);
	}
}

static void
l_ui_patblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_BRUSH * brush, uint32 bgcolor, uint32 fgcolor)
{
	Pixmap fill;
	uint8 * pat;
	uint8 i, ipattern[8];
	uint32 patblt_bgcolor;
	uint32 patblt_fgcolor;
	xfInfo * xfi = GET_XFI(inst);

	patblt_fgcolor = gdi_color_convert(fgcolor, inst->settings->server_depth, xfi->bpp, xfi->clrconv);
	patblt_bgcolor = gdi_color_convert(bgcolor, inst->settings->server_depth, xfi->bpp, xfi->clrconv);
	xf_set_rop3(xfi, opcode);

	switch (brush->style)
	{
		case 0:	/* Solid */
			XSetFillStyle(xfi->display, xfi->gc, FillSolid);
			XSetForeground(xfi->display, xfi->gc, patblt_fgcolor);
			XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
			if (xfi->drw == xfi->backstore)
			{
				XFillRectangle(xfi->display, xfi->wnd, xfi->gc, x, y, cx, cy);
			}
			return;
		case 2:	/* Hatch */
			pat = g_hatch_patterns + brush->pattern[0] * 8;
			fill = (Pixmap) l_ui_create_glyph(inst, 8, 8, pat);
			XSetForeground(xfi->display, xfi->gc, patblt_fgcolor);
			XSetBackground(xfi->display, xfi->gc, patblt_bgcolor);
			XSetFillStyle(xfi->display, xfi->gc, FillOpaqueStippled);
			XSetStipple(xfi->display, xfi->gc, fill);
			XSetTSOrigin(xfi->display, xfi->gc, brush->xorigin, brush->yorigin);
			XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
			XSetStipple(xfi->display, xfi->gc, xfi->bitmap_mono);
			l_ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			break;
		case 3:	/* Pattern */
			if (brush->bd == 0)	/* rdp4 brush */
			{
				for (i = 0; i != 8; i++)
					ipattern[7 - i] = brush->pattern[i];
				fill = (Pixmap) l_ui_create_glyph(inst, 8, 8, ipattern);
				XSetForeground(xfi->display, xfi->gc, patblt_bgcolor);
				XSetBackground(xfi->display, xfi->gc, patblt_fgcolor);
				XSetFillStyle(xfi->display, xfi->gc, FillOpaqueStippled);
				XSetStipple(xfi->display, xfi->gc, fill);
				XSetTSOrigin(xfi->display, xfi->gc, brush->xorigin, brush->yorigin);
				XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
				XSetStipple(xfi->display, xfi->gc, xfi->bitmap_mono);
				l_ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			}
			else if (brush->bd->color_code > 1)	/* > 1 bpp */
			{
				fill = (Pixmap) l_ui_create_bitmap(inst, 8, 8, brush->bd->data);
				XSetFillStyle(xfi->display, xfi->gc, FillTiled);
				XSetTile(xfi->display, xfi->gc, fill);
				XSetTSOrigin(xfi->display, xfi->gc, brush->xorigin, brush->yorigin);
				XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
				XSetTile(xfi->display, xfi->gc, xfi->backstore);
				l_ui_destroy_bitmap(inst, (RD_HBITMAP) fill);
			}
			else
			{
				fill = (Pixmap) l_ui_create_glyph(inst, 8, 8, brush->bd->data);
				XSetForeground(xfi->display, xfi->gc, patblt_bgcolor);
				XSetBackground(xfi->display, xfi->gc, patblt_fgcolor);
				XSetFillStyle(xfi->display, xfi->gc, FillOpaqueStippled);
				XSetStipple(xfi->display, xfi->gc, fill);
				XSetTSOrigin(xfi->display, xfi->gc, brush->xorigin, brush->yorigin);
				XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
				XSetStipple(xfi->display, xfi->gc, xfi->bitmap_mono);
				l_ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			}
			break;
		default:
			printf("brush %d\n", brush->style);
			break;
	}

	if (xfi->drw == xfi->backstore)
	{
		XSetFunction(xfi->display, xfi->gc, GXcopy);
		XCopyArea(xfi->display, xfi->backstore, xfi->wnd, xfi->gc, x, y, cx, cy, x, y);
	}
}

static void
l_ui_screenblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	int srcx, int srcy)
{
	xfInfo * xfi = GET_XFI(inst);

	xf_set_rop3(xfi, opcode);
	XCopyArea(xfi->display, xfi->backstore, xfi->drw, xfi->gc, srcx, srcy, cx, cy, x, y);

	if (xfi->drw == xfi->backstore)
	{
		if (xfi->unobscured)
		{
			XCopyArea(xfi->display, xfi->wnd, xfi->wnd, xfi->gc, srcx, srcy, cx, cy, x, y);
		}
		else
		{
			XSetFunction(xfi->display, xfi->gc, GXcopy);
			XCopyArea(xfi->display, xfi->backstore, xfi->wnd, xfi->gc, x, y, cx, cy, x, y);
		}
	}
	else
	{

	}
}

static void
l_ui_memblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy)
{
	xfInfo * xfi = GET_XFI(inst);

	xf_set_rop3(xfi, opcode);
	XCopyArea(xfi->display, (Pixmap) src, xfi->drw, xfi->gc, srcx, srcy, cx, cy, x, y);

	if (xfi->drw == xfi->backstore)
	{
		XCopyArea(xfi->display, (Pixmap) src, xfi->wnd, xfi->gc, srcx, srcy, cx, cy, x, y);
	}
}

static void
l_ui_triblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, uint32 bgcolor, uint32 fgcolor)
{
	xfInfo * xfi = GET_XFI(inst);
	xf_set_rop3(xfi, opcode);
}

static int
l_ui_select(struct rdp_inst * inst, int rdp_socket)
{
	return 1;
}

static void
l_ui_set_clip(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
	xfInfo * xfi;
	XRectangle clip_rect;

	xfi = GET_XFI(inst);
	clip_rect.x = x;
	clip_rect.y = y;
	clip_rect.width = cx;
	clip_rect.height = cy;
	XSetClipRectangles(xfi->display, xfi->gc, 0, 0, &clip_rect, 1, YXBanded);
}

static void
l_ui_reset_clip(struct rdp_inst * inst)
{
	xfInfo * xfi = GET_XFI(inst);
	XSetClipMask(xfi->display, xfi->gc, None);
}

static void
l_ui_resize_window(struct rdp_inst * inst)
{
	xfInfo * xfi = GET_XFI(inst);

	if (xfi->settings->software_gdi == 1)
	{
		gdi_free(inst);
		gdi_init(inst, CLRCONV_ALPHA | CLRBUF_32BPP);		
	}
	
	printf("ui_resize_window:\n");
}

static void
l_ui_set_cursor(struct rdp_inst * inst, RD_HCURSOR cursor)
{
	xfInfo * xfi = GET_XFI(inst);
	XDefineCursor(xfi->display, xfi->wnd, (Cursor) cursor);
}

static void
l_ui_destroy_cursor(struct rdp_inst * inst, RD_HCURSOR cursor)
{
	xfInfo * xfi = GET_XFI(inst);
	XFreeCursor(xfi->display, (Cursor) cursor);
}

#ifdef USE_XCURSOR

static int
convert_a(int owidth, int oheight, void * odata, int iwidth, int iheight, void * idata)
{
	int i1;
	int j1;
	int imax;
	int jmax;
	int pixel;
	int * d32;
	int * s32;
	int a1, r1, g1, b1;

	imax = oheight > iheight ? iheight : oheight;
	jmax = owidth > iwidth ? iwidth : owidth;
	s32 = (int *) idata;
	d32 = (int *) odata;
	for (i1 = 0; i1 < imax; i1++)
	{
		for (j1 = 0; j1 < jmax; j1++)
		{
			pixel = s32[i1 * iwidth + j1];
			a1 = (pixel >> 24) & 0xff;
			r1 = (pixel >> 16) & 0xff;
			g1 = (pixel >> 8) & 0xff;
			b1 = (pixel >> 0) & 0xff;
			pixel = (a1 << 24) | (b1 << 16) | (g1 << 8) | r1;
			d32[i1 * owidth + j1] = pixel;
		}
	}
	return 0;
}

static RD_HCURSOR
l_ui_create_cursor(struct rdp_inst * inst, uint32 x, uint32 y,
	int width, int height, uint8 * andmask, uint8 * xormask, int bpp)
{
	xfInfo * xfi;
	Cursor cur;
	XcursorImage ci;

	xfi = GET_XFI(inst);
	memset(&ci, 0, sizeof(ci));
	ci.version = XCURSOR_IMAGE_VERSION;
	ci.size = sizeof(ci);
	ci.width = width;
	ci.height = height;
	ci.xhot = x;
	ci.yhot = y;
	ci.pixels = (XcursorPixel *) malloc(width * height * 4);
	memset(ci.pixels, 0, width * height * 4);

	if ((andmask != 0) && (xormask != 0))
	{
		gdi_alpha_cursor_convert((uint8 *) (ci.pixels), xormask, andmask, width, height, bpp, xfi->clrconv);
	}
	if (bpp > 24)
	{
		convert_a(width, height, ci.pixels, width, height, ci.pixels);
	}

	cur = XcursorImageLoadCursor(xfi->display, &ci);
	free(ci.pixels);

	return (RD_HCURSOR) cur;
}

#else

static RD_HCURSOR
l_ui_create_cursor(struct rdp_inst * inst, uint32 x, uint32 y,
	int width, int height, uint8 * andmask, uint8 * xormask, int bpp)
{
	XColor fg;
	XColor bg;
	Cursor cur;
	Pixmap src_glyph;
	Pixmap msk_glyph;
	uint8 * src_data;
	uint8 * msk_data;
	xfInfo * xfi = GET_XFI(inst);

	src_data = (uint8 *) malloc(width * height);
	memset(src_data, 0, width * height);
	msk_data = (uint8 *) malloc(width * height);
	memset(msk_data, 0, width * height);
	memset(&fg, 0xff, sizeof(fg));
	memset(&bg, 0, sizeof(bg));

	if ((andmask != 0) && (xormask != 0))
	{
		gdi_mono_cursor_convert(src_data, msk_data, xormask, andmask, width, height, bpp, xfi->clrconv);
	}

	src_glyph = (Pixmap) l_ui_create_glyph(inst, width, height, src_data);
	msk_glyph = (Pixmap) l_ui_create_glyph(inst, width, height, msk_data);
	cur = XCreatePixmapCursor(xfi->display, src_glyph, msk_glyph, &fg, &bg, x, y);

	l_ui_destroy_glyph(inst, (RD_HGLYPH) src_glyph);
	l_ui_destroy_glyph(inst, (RD_HGLYPH) msk_glyph);

	free(src_data);
	free(msk_data);

	return (RD_HCURSOR) cur;
}

#endif

static void
l_ui_set_null_cursor(struct rdp_inst * inst)
{
	xfInfo * xfi = GET_XFI(inst);
	XDefineCursor(xfi->display, xfi->wnd, xfi->null_cursor);
}

static void
l_ui_set_default_cursor(struct rdp_inst * inst)
{
	xfInfo * xfi = GET_XFI(inst);
	XUndefineCursor(xfi->display, xfi->wnd);
}

static RD_HPALETTE
l_ui_create_palette(struct rdp_inst * inst, RD_PALETTE * palette)
{
	return (RD_HPALETTE) gdi_CreatePalette((HGDI_PALETTE) palette);
}

static void
l_ui_set_palette(struct rdp_inst * inst, RD_HPALETTE palette)
{
	xfInfo * xfi = GET_XFI(inst);
	xfi->clrconv->palette = (RD_PALETTE*) palette;
}

static void
l_ui_move_pointer(struct rdp_inst * inst, int x, int y)
{
	xfInfo * xfi = GET_XFI(inst);
	XWarpPointer(xfi->display, xfi->wnd, xfi->wnd, 0, 0, 0, 0, x, y);
}

static RD_HBITMAP
l_ui_create_surface(struct rdp_inst * inst, int width, int height, RD_HBITMAP old_surface)
{
	Pixmap new;
	Pixmap old;
	xfInfo * xfi = GET_XFI(inst);

	new = XCreatePixmap(xfi->display, xfi->wnd, width, height, xfi->depth);
	old = (Pixmap) old_surface;

	if (old != 0)
	{
		XCopyArea(xfi->display, old, new, xfi->gc_default, 0, 0,
			width, height, 0, 0);
		XFreePixmap(xfi->display, old);
	}

	if (xfi->drw == old)
	{
		xfi->drw = new;
	}

	return (RD_HBITMAP) new;
}

static void
l_ui_set_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	xfInfo * xfi = GET_XFI(inst);

	if (surface != 0)
	{
		xfi->drw = (Drawable) surface;
	}
	else
	{
		xfi->drw = xfi->backstore;
	}
}

static void
l_ui_destroy_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	xfInfo * xfi = GET_XFI(inst);

	if (xfi->drw == (Drawable) surface)
	{
		l_ui_warning(inst, "ui_destroy_surface: freeing active surface!\n");
		xfi->drw = xfi->backstore;
	}
	if (surface != 0)
	{
		XFreePixmap(xfi->display, (Pixmap) surface);
	}
}

static void
l_ui_channel_data(struct rdp_inst * inst, int chan_id, char * data, int data_size,
	int flags, int total_size)
{
	freerdp_chanman_data(inst, chan_id, data, data_size, flags, total_size);
}

static RD_BOOL
l_ui_authenticate(struct rdp_inst * inst)
{
	int l;
	char * pass;

	printf("Please enter NLA login credential.\n");

	printf("User name:");
	if (inst->settings->username[0] == 0)
	{
		if (fgets(inst->settings->username, sizeof(inst->settings->username), stdin) == NULL)
		{
		}
		l = strlen(inst->settings->username);
		if (l > 0 && inst->settings->username[l - 1] == '\n')
			inst->settings->username[l - 1] = 0;
	}
	else
		printf("%s\n", inst->settings->username);

	printf("Domain:");
	if (inst->settings->domain[0] == 0)
	{
		if (fgets(inst->settings->domain, sizeof(inst->settings->domain), stdin) == NULL)
		{
		}
		l = strlen(inst->settings->domain);
		if (l > 0 && inst->settings->domain[l - 1] == '\n')
			inst->settings->domain[l - 1] = 0;
	}
	else
		printf("%s\n", inst->settings->domain);

	if (!inst->settings->password[0])
	{
		pass = getpass("Password:");
		strncpy(inst->settings->password, pass, sizeof(inst->settings->password) - 1);
	}
	return True;
}

static int
l_ui_decode(struct rdp_inst * inst, uint8 * data, int data_size)
{
	xfInfo * xfi = GET_XFI(inst);
	xf_decode_data(xfi, data, data_size);
	return 0;
}

RD_BOOL
l_ui_check_certificate(rdpInst * inst, const char * fingerprint,
	const char * subject, const char * issuer, RD_BOOL verified)
{
	//char answer;

	printf("certificate details:\n");
	printf("  Subject:\n    %s\n", subject);
	printf("  Issued by:\n    %s\n", issuer);
	printf("  Fingerprint:\n    %s\n",  fingerprint);

	if (!verified)
		printf("The server could not be authenticated. Connection security may be compromised!\n");

#if 0
	printf("Accept this certificate? (Y/N): ");
	answer = fgetc(stdin);

	if (answer == 'y' || answer == 'Y')
		return True;
	else
		return False;
#else
	return True;
#endif
}

static int
xf_register_callbacks(rdpInst * inst)
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
	inst->ui_create_palette = l_ui_create_palette;
	inst->ui_set_palette = l_ui_set_palette;
	inst->ui_move_pointer = l_ui_move_pointer;
	inst->ui_create_surface = l_ui_create_surface;
	inst->ui_set_surface = l_ui_set_surface;
	inst->ui_destroy_surface = l_ui_destroy_surface;
	inst->ui_channel_data = l_ui_channel_data;
	inst->ui_authenticate = l_ui_authenticate;
	inst->ui_decode = l_ui_decode;
	inst->ui_check_certificate = l_ui_check_certificate;
	return 0;
}

static int
xf_get_pixmap_info(xfInfo * xfi)
{
	int vi_count;
	int pf_count;
	int index;
	XVisualInfo * vis;
	XVisualInfo * vi;
	XVisualInfo template;
	XPixmapFormatValues * pfs;
	XPixmapFormatValues * pf;

	pfs = XListPixmapFormats(xfi->display, &pf_count);
	if (pfs == NULL)
	{
		printf("xf_get_pixmap_info: XListPixmapFormats failed\n");
		return 1;
	}
	for (index = 0; index < pf_count; index++)
	{
		pf = pfs + index;
		if (pf->depth == xfi->depth)
		{
			xfi->bitmap_pad = pf->scanline_pad;
			xfi->bpp = pf->bits_per_pixel;
			break;
		}
	}
	XFree(pfs);
	memset(&template, 0, sizeof(template));
	template.class = TrueColor;
	template.screen = xfi->screen_num;
	vis = XGetVisualInfo(xfi->display, VisualClassMask | VisualScreenMask, &template,
		&vi_count);
	if (vis == NULL)
	{
		printf("xf_get_pixmap_info: XGetVisualInfo failed\n");
		return 1;
	}
	for (index = 0; index < vi_count; index++)
	{
		vi = vis + index;
		if (vi->depth == xfi->depth)
		{
			xfi->visual = vi->visual;
			break;
		}
	}
	XFree(vis);
	if ((xfi->visual == NULL) || (xfi->bitmap_pad == 0))
	{
		return 1;
	}
	return 0;
}

int
xf_pre_connect(xfInfo * xfi)
{
#ifdef HAVE_LIBXINERAMA
	XineramaScreenInfo * screen_info = NULL;
	int ignored, ignored2;
	int n;
#endif

	int i1;

	xf_register_callbacks(xfi->inst);

	xfi->display = XOpenDisplay(NULL);

	if (xfi->display == NULL)
	{
		printf("xf_pre_connect: failed to open display: %s\n", XDisplayName(NULL));
		return 1;
	}

	xfi->x_socket = ConnectionNumber(xfi->display);
	xfi->screen_num = DefaultScreen(xfi->display);
	xfi->screen = ScreenOfDisplay(xfi->display, xfi->screen_num);
	xfi->depth = DefaultDepthOfScreen(xfi->screen);
	xfi->xserver_be = (ImageByteOrder(xfi->display) == MSBFirst);

	if (xfi->percentscreen > 0)
	{
		i1 = (WidthOfScreen(xfi->screen) * xfi->percentscreen) / 100;
		xfi->settings->width = i1;
		i1 = (HeightOfScreen(xfi->screen) * xfi->percentscreen) / 100;
		xfi->settings->height = i1;
	}

	if (xfi->fs_toggle)
	{
        xfi->settings->num_monitors = 0;
 		xfi->settings->width = WidthOfScreen(xfi->screen);
 		xfi->settings->height = HeightOfScreen(xfi->screen);

#ifdef HAVE_LIBXINERAMA
		if (XineramaQueryExtension(xfi->display, &ignored, &ignored2))
		{
			if (XineramaIsActive(xfi->display))
			{
				screen_info = XineramaQueryScreens(xfi->display, &xfi->settings->num_monitors);
				if (xfi->settings->num_monitors > 16)
					xfi->settings->num_monitors = 0;

				if (xfi->settings->num_monitors)
				{
					for (n = 0; n < xfi->settings->num_monitors; n++)
					{
						xfi->settings->monitors[n].x = screen_info[n].x_org;
						xfi->settings->monitors[n].y = screen_info[n].y_org;
						xfi->settings->monitors[n].width = screen_info[n].width;
						xfi->settings->monitors[n].height = screen_info[n].height;
						xfi->settings->monitors[n].is_primary = screen_info[n].x_org == 0 && screen_info[n].y_org == 0;
					}
				}
				XFree(screen_info);
			}
		}
#endif
	}

	i1 = xfi->settings->width;
	i1 = (i1 + 3) & (~3);
	xfi->settings->width = i1;

	if ((xfi->settings->width < 64) || (xfi->settings->height < 64) ||
		(xfi->settings->width > 4096) || (xfi->settings->height > 4096))
	{
		printf("xf_pre_connect: invalid dimensions %d %d\n", xfi->settings->width, xfi->settings->height);
		return 1;
	}

	return 0;
}

static void
xf_hide_decorations(xfInfo * xfi)
{
	Atom atom;
	PropMotifWmHints hints;

	hints.decorations = 0;
	hints.flags = MWM_HINTS_DECORATIONS;

	atom = XInternAtom(xfi->display, "_MOTIF_WM_HINTS", False);

	if (!atom)
	{
		printf("xf_hide_decorations: failed to obtain atom _MOTIF_WM_HINTS\n");
	}
	else
	{
		XChangeProperty(xfi->display, xfi->wnd, atom, atom, 32, PropModeReplace,
			(unsigned char *) &hints, PROP_MOTIF_WM_HINTS_ELEMENTS);
	}
}

int
xf_post_connect(xfInfo * xfi)
{
	int width;
	int height;
	int input_mask;
	int fullscreen;
	XEvent xevent;
	XGCValues gcv;
	Atom kill_atom;
	Atom protocol_atom;
	XSizeHints *sizehints;
	XClassHint *classhints;
	XSetWindowAttributes attribs;

	if (xf_get_pixmap_info(xfi) != 0)
	{
		return 1;
	}

	fullscreen = xfi->fullscreen;
	width = fullscreen ? WidthOfScreen(xfi->screen) : xfi->settings->width;
	height = fullscreen ? HeightOfScreen(xfi->screen) : xfi->settings->height;

	attribs.background_pixel = BlackPixelOfScreen(xfi->screen);
	attribs.border_pixel = WhitePixelOfScreen(xfi->screen);
	attribs.backing_store = xfi->backstore ? NotUseful : Always;
	attribs.override_redirect = fullscreen;
	attribs.colormap = xfi->xcolmap;

	xfi->wnd = XCreateWindow(xfi->display, RootWindowOfScreen(xfi->screen),
		0, 0, width, height, 0, xfi->depth, InputOutput, xfi->visual,
		CWBackPixel | CWBackingStore | CWOverrideRedirect | CWColormap |
		CWBorderPixel, &attribs);

	classhints = XAllocClassHint();

	if (classhints != NULL)
	{
		classhints->res_name = "xfreerdp";
		classhints->res_class = "freerdp";
		XSetClassHint(xfi->display, xfi->wnd, classhints);
		XFree(classhints);
	}

	sizehints = XAllocSizeHints();

	if (sizehints)
	{
		sizehints->flags = PMinSize | PMaxSize;
		sizehints->min_width = sizehints->max_width = width;
		sizehints->min_height = sizehints->max_height = height;
		XSetWMNormalHints(xfi->display, xfi->wnd, sizehints);
		XFree(sizehints);
	}

	char win_title[64];
	if (strlen(xfi->window_title) > 0)
		snprintf(win_title, sizeof(win_title), "%s", xfi->window_title);
	else if (xfi->settings->tcp_port_rdp == 3389)
		snprintf(win_title, sizeof(win_title), "%s - freerdp", xfi->settings->server);
	else
		snprintf(win_title, sizeof(win_title), "%s:%d - freerdp", xfi->settings->server, xfi->settings->tcp_port_rdp);
	XStoreName(xfi->display, xfi->wnd, win_title);

	if (xfi->embed)
		XReparentWindow(xfi->display, xfi->wnd, xfi->embed, 0, 0);

	input_mask =
		KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
		VisibilityChangeMask | FocusChangeMask | StructureNotifyMask |
		PointerMotionMask | ExposureMask;

	if (xfi->grab_keyboard)
		input_mask |= EnterWindowMask | LeaveWindowMask;

	XSelectInput(xfi->display, xfi->wnd, input_mask);
	XMapWindow(xfi->display, xfi->wnd);

	if (fullscreen)
	{
		xf_hide_decorations(xfi);
		XSetInputFocus(xfi->display, xfi->wnd, RevertToParent, CurrentTime);
	}
	else if (xfi->decoration == 0)
	{
		xf_hide_decorations(xfi);
	}

	/* wait for VisibilityNotify */
	do
	{
		XMaskEvent(xfi->display, VisibilityChangeMask, &xevent);
	}
	while (xevent.type != VisibilityNotify);

	xfi->unobscured = xevent.xvisibility.state == VisibilityUnobscured;
	memset(&gcv, 0, sizeof(gcv));

	protocol_atom = XInternAtom(xfi->display, "WM_PROTOCOLS", True);
	kill_atom = XInternAtom(xfi->display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(xfi->display, xfi->wnd, &kill_atom, 1);

	if (!xfi->gc)
		xfi->gc = XCreateGC(xfi->display, xfi->wnd, GCGraphicsExposures, &gcv);

	if (!xfi->backstore)
		xfi->backstore = XCreatePixmap(xfi->display, xfi->wnd, width, height, xfi->depth);

	xfi->drw = xfi->backstore;

	xfi->bitmap_mono = XCreatePixmap(xfi->display, xfi->wnd, 8, 8, 1);
	xfi->gc_mono = XCreateGC(xfi->display, xfi->bitmap_mono, GCGraphicsExposures, &gcv);
	xfi->gc_default = XCreateGC(xfi->display, xfi->wnd, GCGraphicsExposures, &gcv);
	XSetForeground(xfi->display, xfi->gc, BlackPixelOfScreen(xfi->screen));
	XFillRectangle(xfi->display, xfi->backstore, xfi->gc, 0, 0, width, height);
	xfi->null_cursor = (Cursor) l_ui_create_cursor(xfi->inst, 0, 0, 32, 32, 0, 0, 0);
	xfi->mod_map = XGetModifierMapping(xfi->display);

	if (xfi->settings->software_gdi == 1)
	{
		GDI *gdi;
		gdi_init(xfi->inst, CLRCONV_ALPHA | CLRBUF_32BPP);
		gdi = GET_GDI(xfi->inst);

		xfi->inst->ui_begin_update = l_ui_gdi_begin_update;
		xfi->inst->ui_end_update = l_ui_gdi_end_update;
	}

	return 0;
}

static void
xf_destroy_window(xfInfo * xfi)
{
	/* xf_post_connect */
	XFreeModifiermap(xfi->mod_map);
	xfi->mod_map = 0;
	XFreeGC(xfi->display, xfi->gc_default);
	xfi->gc_default = 0;
	XFreeGC(xfi->display, xfi->gc_mono);
	xfi->gc_mono = 0;
	XFreePixmap(xfi->display, xfi->bitmap_mono);	/* Note: valgrind reports this at lost no matter what */
	xfi->bitmap_mono = 0;
	XFreeGC(xfi->display, xfi->gc);
	xfi->gc = 0;
	XDestroyWindow(xfi->display, xfi->wnd);
	xfi->wnd = 0;

	if (xfi->backstore)
	{
		XFreePixmap(xfi->display, xfi->backstore);
		xfi->backstore = 0;
	}
}

void
xf_uninit(xfInfo * xfi)
{
	xf_destroy_window(xfi);
	XCloseDisplay(xfi->display);
}

int
xf_get_fds(xfInfo * xfi, void ** read_fds, int * read_count,
	void ** write_fds, int * write_count)
{
	read_fds[*read_count] = (void *)(long) (xfi->x_socket);
	(*read_count)++;
	return 0;
}

int
xf_check_fds(xfInfo * xfi)
{
	XEvent xevent;

	while (XPending(xfi->display))
	{
		memset(&xevent, 0, sizeof(xevent));
		XNextEvent(xfi->display, &xevent);

		if (xf_handle_event(xfi, &xevent) != 0)
		{
			return 1;
		}
	}
	return 0;
}

void
xf_toggle_fullscreen(xfInfo * xfi)
{
	Pixmap contents = 0;
	int width = xfi->settings->width;
	int height = xfi->settings->height;

	contents = XCreatePixmap(xfi->display, xfi->wnd, width, height, xfi->depth);
	XCopyArea(xfi->display, xfi->backstore, contents, xfi->gc, 0, 0, width, height, 0, 0);

	xf_destroy_window(xfi);
	xfi->fullscreen = !xfi->fullscreen;
	xf_post_connect(xfi);

	XCopyArea(xfi->display, contents, xfi->backstore, xfi->gc, 0, 0, width, height, 0, 0);
	XFreePixmap(xfi->display, contents);
}
