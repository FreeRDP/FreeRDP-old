## Process this file with automake to produce Makefile.in

# rdpdbg
rdpdbgdir = $(PLUGIN_PATH)

rdpdbg_LTLIBRARIES = rdpdbg.la

rdpdbg_la_SOURCES = \
	rdpdbg_main.c

rdpdbg_la_CFLAGS = -I$(top_srcdir)/include -I$(srcdir)/../common

rdpdbg_la_LDFLAGS = -avoid-version -module

rdpdbg_la_LIBADD = ../common/libcommon.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

