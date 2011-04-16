## Process this file with automake to produce Makefile.in

# tsmf_alsa
tsmf_alsadir = $(PLUGIN_PATH)

tsmf_alsa_LTLIBRARIES = tsmf_alsa.la

tsmf_alsa_la_SOURCES = \
	tsmf_alsa.c

tsmf_alsa_la_CFLAGS = @ALSA_CFLAGS@ \
	-I$(top_srcdir)/include -I$(srcdir)/.. -I$(srcdir)/../.. -I$(srcdir)/../../../common \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

tsmf_alsa_la_LDFLAGS = -avoid-version -module

tsmf_alsa_la_LIBADD = @ALSA_LIBS@ ../../../common/libcommon.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

