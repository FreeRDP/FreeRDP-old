## Process this file with automake to produce Makefile.in

# audin
audindir = $(PLUGIN_PATH)

audin_LTLIBRARIES = audin.la

audin_la_SOURCES = \
	audin_main.c audin_main.h

audin_la_CFLAGS = -I$(top_srcdir)/include -I$(srcdir)/../../common \
	-I$(srcdir)/.. -DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

audin_la_LDFLAGS = -avoid-version -module

audin_la_LIBADD = ../../common/libcommon.la

if RDPSND_ALSA
audin_la_SOURCES += audin_alsa.c
audin_la_CFLAGS += @ALSA_CFLAGS@
audin_la_LIBADD += @ALSA_LIBS@
endif

# extra
EXTRA_DIST =

DISTCLEANFILES = 

