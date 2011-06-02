## Process this file with automake to produce Makefile.in

# rdpsnd_pulse
rdpsnd_pulsedir = $(PLUGIN_PATH)

rdpsnd_pulse_LTLIBRARIES = rdpsnd_pulse.la

rdpsnd_pulse_la_SOURCES = \
	rdpsnd_pulse.c

rdpsnd_pulse_la_CFLAGS = @PULSE_CFLAGS@ \
	-I$(top_srcdir)/include -I$(srcdir)/.. \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

rdpsnd_pulse_la_LDFLAGS = -avoid-version -module

rdpsnd_pulse_la_LIBADD = \
	@PULSE_LIBS@ \
	../../../libutils/libutils.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

