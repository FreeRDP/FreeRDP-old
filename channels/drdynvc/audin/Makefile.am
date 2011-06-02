## Process this file with automake to produce Makefile.in

# audin
audindir = $(PLUGIN_PATH)

audin_LTLIBRARIES = audin.la

audin_la_SOURCES = \
	audin_types.h \
	audin_dsp.c audin_dsp.h \
	audin_main.c

audin_la_CFLAGS = -I$(top_srcdir)/include \
	-I$(srcdir)/.. -DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

audin_la_LDFLAGS = -avoid-version -module

audin_la_LIBADD = \
	../../../libutils/libutils.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

