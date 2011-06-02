## Process this file with automake to produce Makefile.in

# librdpdr
rdpdrdir = $(PLUGIN_PATH)

rdpdr_LTLIBRARIES = rdpdr.la

rdpdr_la_SOURCES = \
	rdpdr_types.h \
	rdpdr_main.c rdpdr_main.h \
	rdpdr_capabilities.c rdpdr_capabilities.h \
	rdpdr_constants.h \
	devman.c devman.h \
	irp.c irp.h \
	irp_queue.c irp_queue.h \
	rdpdr_scard.h rdpdr_scard.c

rdpdr_la_CFLAGS = -I$(top_srcdir)/include \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

rdpdr_la_LDFLAGS = -avoid-version -module

rdpdr_la_LIBADD = \
	../../libutils/libutils.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 
