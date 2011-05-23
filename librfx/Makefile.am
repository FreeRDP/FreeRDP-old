## Process this file with automake to produce Makefile.in

# librfx
librfxdir = $(libdir)

librfx_LTLIBRARIES = librfx.la

librfx_la_SOURCES = \
	rfx_bitstream.c rfx_bitstream.h \
	rfx_rlgr.c rfx_rlgr.h \
	rfx_differential.c rfx_differential.h \
	rfx_quantization.c rfx_quantization.h \
	rfx_dwt.c rfx_dwt.h \
	rfx_decode.c rfx_decode.h \
	rfx_constants.h \
	rfx.c

librfx_la_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include

librfx_la_LDFLAGS =

librfx_la_LIBDADD =

# librfx headers
includedir = $(prefix)/include/librfx
include_HEADERS = rfx.h

# extra
EXTRA_DIST =

DISTCLEANFILES = 

