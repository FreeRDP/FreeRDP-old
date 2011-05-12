## Process this file with automake to produce Makefile.in

# tsmf_pulse
tsmf_pulsedir = $(PLUGIN_PATH)

tsmf_pulse_LTLIBRARIES = tsmf_pulse.la

tsmf_pulse_la_SOURCES = \
	tsmf_pulse.c

tsmf_pulse_la_CFLAGS = @PULSE_CFLAGS@ \
	-I$(top_srcdir)/include -I$(srcdir)/.. -I$(srcdir)/../.. \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

tsmf_pulse_la_LDFLAGS = -avoid-version -module

tsmf_pulse_la_LIBADD = @PULSE_LIBS@ ../../../../libfreerdputils/libfreerdputils.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

