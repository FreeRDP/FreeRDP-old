## Process this file with automake to produce Makefile.in

# librdpdr
librdpdrdir = $(libdir)

librdpdr_LTLIBRARIES = librdpdr.la

librdpdr_la_SOURCES = \
	../common/wait_obj.c \
	../common/chan_plugin.c \
	../common/chan_stream.c \
	rdpdr_main.c \
	rdpdr_capabilities.c \
	devman.c \
	irp.c

librdpdr_la_CFLAGS = -I../../include/freerdp -I../common -I.

librdpdr_la_LDFLAGS = -avoid-version -module

# extra
EXTRA_DIST =

DISTCLEANFILES = 

