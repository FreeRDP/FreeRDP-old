## Process this file with automake to produce Makefile.in

# libfreerdp-rfx
libfreerdp_rfxdir = $(libdir)

libfreerdp_rfx_LTLIBRARIES = libfreerdp-rfx.la

libfreerdp_rfx_la_SOURCES = \
	rfx_bitstream.c rfx_bitstream.h \
	rfx_rlgr.c rfx_rlgr.h \
	rfx_differential.c rfx_differential.h \
	rfx_quantization.c rfx_quantization.h \
	rfx_dwt.c rfx_dwt.h \
	rfx_decode.c rfx_decode.h \
	rfx_constants.h \
	rfx_private.h \
	rfx.c

libfreerdp_rfx_la_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include

libfreerdp_rfx_la_LDFLAGS =

libfreerdp_rfx_la_LIBDADD =

# libfreerdp-rfx headers
includedir = $(prefix)/include/rfx
include_HEADERS = rfx.h

# extra
EXTRA_DIST =

DISTCLEANFILES = 

