## Process this file with automake to produce Makefile.in

# libfreerdpchanman
libfreerdpchanmandir = $(libdir)

libfreerdpchanman_LTLIBRARIES = libfreerdpchanman.la

libfreerdpchanman_la_SOURCES = \
	libfreerdpchanman.c

libfreerdpchanman_la_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

libfreerdpchanman_la_LDFLAGS = -ldl -lpthread

libfreerdpchanman_la_LIBDADD = ../libfreerdp/libfreerdp.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

