## Process this file with automake to produce Makefile.in

exttestdir = $(PLUGIN_PATH)

exttest_LTLIBRARIES = exttest.la

exttest_la_SOURCES = \
	exttest_main.c

exttest_la_CFLAGS = -I$(top_srcdir)/include -I$(srcdir)/../common \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

exttest_la_LDFLAGS = -avoid-version -module

exttest_la_LIBADD =

# extra
EXTRA_DIST =

DISTCLEANFILES = 

