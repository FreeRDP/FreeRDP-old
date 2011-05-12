## Process this file with automake to produce Makefile.in

# parallel
paralleldir = $(PLUGIN_PATH)

parallel_LTLIBRARIES = parallel.la

parallel_la_SOURCES = \
	parallel_main.c

parallel_la_CFLAGS = -I$(top_srcdir)/include \
	-I$(srcdir)/.. -DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

parallel_la_LDFLAGS = -avoid-version -module

parallel_la_LIBADD = ../../../libfreerdputils/libfreerdputils.la

# extra
EXTRA_DIST =

DISTCLEANFILES =
