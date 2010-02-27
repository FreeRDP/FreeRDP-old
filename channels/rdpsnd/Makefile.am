## Process this file with automake to produce Makefile.in

if WITH_RDPSND

# librdpsnd
librdpsnddir = $(libdir)

librdpsnd_LTLIBRARIES = librdpsnd.la

librdpsnd_la_SOURCES = \
	../common/wait_obj.c \
	../common/chan_plugin.c \
	rdpsnd_main.c

librdpsnd_la_CFLAGS = -I../../include/freerdp -I../common

librdpsnd_la_LDFLAGS = -avoid-version -module

librdpsnd_la_LIBADD =

if RDPSND_ALSA
librdpsnd_la_SOURCES += rdpsnd_alsa.c
librdpsnd_la_CFLAGS += @ALSA_CFLAGS@
librdpsnd_la_LIBADD += @ALSA_LIBS@
endif

#	rdpsnd_pulse.c

# extra
EXTRA_DIST =

DISTCLEANFILES = 

endif

