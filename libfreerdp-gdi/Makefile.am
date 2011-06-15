## Process this file with automake to produce Makefile.in

# libfreerdp-gdi
libfreerdp_gdidir = $(libdir)

libfreerdp_gdi_LTLIBRARIES = libfreerdp-gdi.la

libfreerdp_gdi_la_SOURCES = \
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
	libgdi.c libgdi.h \
	gdi.c gdi.h

libfreerdp_gdi_la_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/libfreerdp-rfx

libfreerdp_gdi_la_LDFLAGS =

libfreerdp_gdi_la_LIBADD = \
	../libfreerdp-rfx/libfreerdp-rfx.la

if WITH_SSE
SUBDIRS = sse
libfreerdp_gdi_la_CFLAGS += -I./sse
libfreerdp_gdi_la_LIBADD += ./sse/libfreerdp-gdi-sse.la
endif

if WITH_NEON
SUBDIRS = neon
libfreerdp_gdi_la_CFLAGS += -I./neon
libfreerdp_gdi_la_LIBADD += ./neon/libfreerdp-gdi-neon.la
endif

# extra
EXTRA_DIST =

DISTCLEANFILES = 

