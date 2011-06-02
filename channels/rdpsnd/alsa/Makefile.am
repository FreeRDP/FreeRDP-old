## Process this file with automake to produce Makefile.in

# rdpsnd_alsa
rdpsnd_alsadir = $(PLUGIN_PATH)

rdpsnd_alsa_LTLIBRARIES = rdpsnd_alsa.la

rdpsnd_alsa_la_SOURCES = \
	rdpsnd_alsa.c

rdpsnd_alsa_la_CFLAGS = @ALSA_CFLAGS@ \
	-I$(top_srcdir)/include -I$(srcdir)/.. \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

rdpsnd_alsa_la_LDFLAGS = -avoid-version -module

rdpsnd_alsa_la_LIBADD = \
	@ALSA_LIBS@ \
	../../../libfreerdp-utils/libfreerdp-utils.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

