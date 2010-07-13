## Process this file with automake to produce Makefile.in

# printer
printerdir = $(PLUGIN_PATH)

printer_LTLIBRARIES = printer.la

printer_la_SOURCES = \
	printer_main.c printer_main.h

printer_la_CFLAGS = -I$(top_srcdir)/include -I$(srcdir)/../../common \
	-I$(srcdir)/.. -DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

printer_la_LDFLAGS = -avoid-version -module

printer_la_LIBADD = ../../common/libcommon.la

if RDPDR_PRINTER_CUPS
printer_la_SOURCES += printer_cups.c
printer_la_CFLAGS += @CUPS_CFLAGS@
printer_la_LIBADD += @CUPS_LIBS@
endif

# extra
EXTRA_DIST =

DISTCLEANFILES = 

