## Process this file with automake to produce Makefile.in

# libfreerdp-chanman
libfreerdp_chanmandir = $(libdir)

libfreerdp_chanman_LTLIBRARIES = libfreerdp-chanman.la

libfreerdp_chanman_la_SOURCES = \
	libchanman.c

libfreerdp_chanman_la_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

libfreerdp_chanman_la_LDFLAGS = \
	-pthread

libfreerdp_chanman_la_LIBDADD = \
	-ldl \
	../libfreerdp-core/libfreerdp-core.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

