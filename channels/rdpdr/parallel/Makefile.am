## Process this file with automake to produce Makefile.in

# parallel
paralleldir = $(PLUGIN_PATH)

parallel_LTLIBRARIES = parallel.la

parallel_la_SOURCES = \
	parallel_main.c

parallel_la_CFLAGS = -I../../../include -I../../common -I../ \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

parallel_la_LDFLAGS = -avoid-version -module

parallel_la_LIBADD = ../../common/libcommon.la

# extra
EXTRA_DIST =

DISTCLEANFILES =
