## Process this file with automake to produce Makefile.in

# disk
diskdir = $(PLUGIN_PATH)

disk_LTLIBRARIES = disk.la

disk_la_SOURCES = \
	../../common/chan_stream.c \
	disk_main.c

disk_la_CFLAGS = -I../../../include -I../../common -I../ \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

disk_la_LDFLAGS = -avoid-version -module

disk_la_LIBADD = 

# extra
EXTRA_DIST =

DISTCLEANFILES = 

