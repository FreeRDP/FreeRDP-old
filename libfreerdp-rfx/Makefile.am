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
	rfx_encode.c rfx_encode.h \
	rfx_pool.c rfx_pool.h \
	librfx.c librfx.h

libfreerdp_rfx_la_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include

libfreerdp_rfx_la_LDFLAGS =

libfreerdp_rfx_la_LIBADD =

if WITH_SSE
SUBDIRS = sse
libfreerdp_rfx_la_CFLAGS += -I$(top_srcdir)/libfreerdp-rfx/sse
libfreerdp_rfx_la_LIBADD += sse/libfreerdp-rfx-sse.la
endif

if WITH_NEON
SUBDIRS = neon
libfreerdp_rfx_la_CFLAGS += -I$(top_srcdir)/libfreerdp-rfx/neon
libfreerdp_rfx_la_LIBADD += neon/libfreerdp-rfx-neon.la
endif

# extra
EXTRA_DIST =

DISTCLEANFILES = 

