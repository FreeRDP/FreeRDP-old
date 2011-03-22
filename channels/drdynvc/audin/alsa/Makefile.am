## Process this file with automake to produce Makefile.in

# audin_alsa
audin_alsadir = $(PLUGIN_PATH)

audin_alsa_LTLIBRARIES = audin_alsa.la

audin_alsa_la_SOURCES = \
	audin_alsa.c

audin_alsa_la_CFLAGS = @ALSA_CFLAGS@ \
	-I$(top_srcdir)/include -I$(srcdir)/.. -I$(srcdir)/../.. -I$(srcdir)/../../../common \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

audin_alsa_la_LDFLAGS = -avoid-version -module

audin_alsa_la_LIBADD = @ALSA_LIBS@ ../../../common/libcommon.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

