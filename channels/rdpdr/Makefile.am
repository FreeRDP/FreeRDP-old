## Process this file with automake to produce Makefile.in

# librdpdr
rdpdrdir = $(PLUGIN_PATH)

rdpdr_LTLIBRARIES = rdpdr.la

rdpdr_la_SOURCES = \
	../common/wait_obj.c \
	../common/chan_plugin.c \
	../common/chan_stream.c \
	rdpdr_types.h \
	rdpdr_main.c rdpdr_main.h \
	rdpdr_capabilities.c rdpdr_capabilities.h \
	rdpdr_constants.h \
	devman.c devman.h \
	irp.c irp.h

rdpdr_la_CFLAGS = -I../../include -I../common -I. \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

rdpdr_la_LDFLAGS = -avoid-version -module

rdpdr_la_LIBADD = 

# extra
EXTRA_DIST =

DISTCLEANFILES = 
