## Process this file with automake to produce Makefile.in

# libgdi
libgdidir = $(libdir)

libgdi_LTLIBRARIES = libgdi.la

libgdi_la_SOURCES = \
	gdi_bitmap.c gdi_bitmap.h \
	gdi_region.c gdi_region.h \
	gdi_drawing.c gdi_drawing.h \
	gdi_palette.c gdi_palette.h \
	gdi_clipping.c gdi_clipping.h \
	gdi_brush.c gdi_brush.h \
	gdi_shape.c gdi_shape.h \
	gdi_pen.c gdi_pen.h \
	gdi_dc.c gdi_dc.h \
	gdi_line.c gdi_line.h \
	gdi_32bpp.c gdi_32bpp.h \
	gdi_16bpp.c gdi_16bpp.h \
	gdi_8bpp.c gdi_8bpp.h \
	color.c color.h \
	decode.c decode.h \
	gdi.c gdi.h

libgdi_la_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/librfx

libgdi_la_LDFLAGS =

libgdi_la_LIBDADD = \
	../librfx/librfx.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

