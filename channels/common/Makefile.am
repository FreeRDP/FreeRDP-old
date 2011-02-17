## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	chan_plugin.c chan_plugin.h \
	chan_stream.c chan_stream.h \
	wait_obj.c wait_obj.h \
	types.h

libcommon_la_CFLAGS = \
	-I$(top_srcdir)/include \
	-pthread

libcommon_la_LDFLAGS =

libcommon_la_LIBADD = @LIBICONV@

# extra
EXTRA_DIST =

DISTCLEANFILES =
