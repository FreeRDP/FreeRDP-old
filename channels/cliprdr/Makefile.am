## Process this file with automake to produce Makefile.in

# cliprdr
cliprdrdir = $(PLUGIN_PATH)

cliprdr_LTLIBRARIES = cliprdr.la

cliprdr_la_SOURCES = \
	../common/chan_plugin.c \
	../common/wait_obj.c \
	cliprdr_main.h cliprdr_main.c \
	cliprdr_x11.c

cliprdr_la_CFLAGS = -I../../include -I../common

cliprdr_la_LDFLAGS = -avoid-version -module

cliprdr_la_LIBADD = -lX11

# extra
EXTRA_DIST =

DISTCLEANFILES = 

