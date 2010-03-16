## Process this file with automake to produce Makefile.in

# librdpdr
librdpdrdir = $(libdir)

librdpdr_LTLIBRARIES = librdpdr.la

librdpdr_la_SOURCES = \
	../common/wait_obj.c \
	../common/chan_plugin.c \
	../common/chan_stream.c \
	rdpdr.h \
	rdpdr_main.c \
	rdpdr_capabilities.c rdpdr_capabilities.h \
	devman.c devman.h \
	irp.c irp.h

librdpdr_la_CFLAGS = -I../../include/freerdp -I../common -I.

librdpdr_la_LDFLAGS = -avoid-version -module

# extra
EXTRA_DIST =

DISTCLEANFILES = 
