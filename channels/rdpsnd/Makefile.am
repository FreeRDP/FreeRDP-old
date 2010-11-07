## Process this file with automake to produce Makefile.in

# rdpsnd
rdpsnddir = $(PLUGIN_PATH)

rdpsnd_LTLIBRARIES = rdpsnd.la

rdpsnd_la_SOURCES = \
	rdpsnd_dsp.h rdpsnd.h \
	rdpsnd_main.c

rdpsnd_la_CFLAGS = -I$(top_srcdir)/include -I$(srcdir)/../common \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

rdpsnd_la_LDFLAGS = -avoid-version -module

rdpsnd_la_LIBADD = ../common/libcommon.la -ldl -lpthread

# extra
EXTRA_DIST =

DISTCLEANFILES = 

