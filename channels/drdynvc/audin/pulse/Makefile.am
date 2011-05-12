## Process this file with automake to produce Makefile.in

# audin_pulse
audin_pulsedir = $(PLUGIN_PATH)

audin_pulse_LTLIBRARIES = audin_pulse.la

audin_pulse_la_SOURCES = \
	audin_pulse.c

audin_pulse_la_CFLAGS = @PULSE_CFLAGS@ \
	-I$(top_srcdir)/include -I$(srcdir)/.. -I$(srcdir)/../.. \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

audin_pulse_la_LDFLAGS = -avoid-version -module

audin_pulse_la_LIBADD = @PULSE_LIBS@ ../../../../libfreerdputils/libfreerdputils.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

