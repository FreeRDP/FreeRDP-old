## Process this file with automake to produce Makefile.in

# tsmf
tsmfdir = $(PLUGIN_PATH)

tsmf_LTLIBRARIES = tsmf.la

tsmf_la_SOURCES = \
	tsmf_constants.h \
	tsmf_types.h \
	tsmf_main.c

tsmf_la_CFLAGS = -I$(top_srcdir)/include -I$(srcdir)/../../common \
	-I$(srcdir)/.. -DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

tsmf_la_LDFLAGS = -avoid-version -module

tsmf_la_LIBADD = ../../common/libcommon.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

