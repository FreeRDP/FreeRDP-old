## Process this file with automake to produce Makefile.in

# cliprdr
cliprdrdir = $(PLUGIN_PATH)

cliprdr_LTLIBRARIES = cliprdr.la

cliprdr_la_SOURCES = \
	cliprdr_main.h cliprdr_main.c

cliprdr_la_CFLAGS = -I$(top_srcdir)/include

cliprdr_la_LDFLAGS = -avoid-version -module

cliprdr_la_LIBADD = \
	../../libfreerdp-utils/libfreerdp-utils.la

if WITH_X11
cliprdr_la_SOURCES += cliprdr_x11.c
cliprdr_la_CFLAGS += @X_CFLAGS@
cliprdr_la_LIBADD += @X_LIBS@ @X_EXTRA_LIBS@
endif

# extra
EXTRA_DIST =

DISTCLEANFILES = 

