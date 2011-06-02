## Process this file with automake to produce Makefile.in

# pnpdr
pnpdrdir = $(PLUGIN_PATH)

pnpdr_LTLIBRARIES = pnpdr.la

pnpdr_la_SOURCES = \
	pnpdr_main.c

pnpdr_la_CFLAGS = -I$(top_srcdir)/include \
	-I$(srcdir)/.. -DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

pnpdr_la_LDFLAGS = -avoid-version -module

pnpdr_la_LIBADD = \
	../../../libfreerdp-utils/libfreerdp-utils.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

