## Process this file with automake to produce Makefile.in

# drdynvc
drdynvcdir = $(PLUGIN_PATH)

drdynvc_LTLIBRARIES = drdynvc.la

drdynvc_la_SOURCES = \
	dvcman.c dvcman.h \
	drdynvc_types.h \
	drdynvc_main.c drdynvc_main.h

drdynvc_la_CFLAGS = -I$(top_srcdir)/include \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

drdynvc_la_LDFLAGS = -avoid-version -module

drdynvc_la_LIBADD = ../../libfreerdputils/libfreerdputils.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 
