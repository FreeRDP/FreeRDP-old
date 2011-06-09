## Process this file with automake to produce Makefile.in

# libfreerdp-rfx-sse
noinst_LTLIBRARIES = libfreerdp-rfx-sse.la

libfreerdp_rfx_sse_la_SOURCES =

if WITH_SSE
libfreerdp_rfx_sse_la_SOURCES += \
	rfx_sse.c rfx_sse.h \
	rfx_sse2.c rfx_sse2.h
endif

libfreerdp_rfx_sse_la_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	-I..

libfreerdp_rfx_sse_la_LDFLAGS =

libfreerdp_rfx_sse_la_LIBDADD =

# extra
EXTRA_DIST =

DISTCLEANFILES = 
