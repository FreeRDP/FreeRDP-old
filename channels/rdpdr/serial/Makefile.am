## Process this file with automake to produce Makefile.in

# serial
serialdir = $(PLUGIN_PATH)

serial_LTLIBRARIES = serial.la

serial_la_SOURCES = \
	serial_main.c

serial_la_CFLAGS = -I$(top_srcdir)/include \
	-I$(srcdir)/.. -DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

serial_la_LDFLAGS = -avoid-version -module

serial_la_LIBADD = \
	../../../libfreerdp-utils/libfreerdp-utils.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

