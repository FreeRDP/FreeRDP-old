## Process this file with automake to produce Makefile.in

# librdpsnd
librdpsnddir = $(libdir)

librdpsnd_LTLIBRARIES = librdpsnd.la

librdpsnd_la_SOURCES = \
	../common/wait_obj.c \
	../common/chan_plugin.c \
	rdpsnd_main.c

if FOUND_ALSA
librdpsnd_la_SOURCES += rdpsnd_alsa.c
endif

#	rdpsnd_pulse.c

librdpsnd_la_CFLAGS = -I../../include/freerdp -I../common

librdpsnd_la_LDFLAGS = -avoid-version -module

# extra
EXTRA_DIST =

DISTCLEANFILES = 

