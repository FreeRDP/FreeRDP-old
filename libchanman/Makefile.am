## Process this file with automake to produce Makefile.in

# libchanman
libchanmandir = $(libdir)

libchanman_LTLIBRARIES = libchanman.la

libchanman_la_SOURCES = \
	libchanman.c

libchanman_la_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

libchanman_la_LDFLAGS = \
	-pthread

libchanman_la_LIBDADD = \
	-ldl \
	../libfreerdp/libfreerdp.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

