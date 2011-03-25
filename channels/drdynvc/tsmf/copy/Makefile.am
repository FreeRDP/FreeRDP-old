## Process this file with automake to produce Makefile.in

# tsmf_copy
tsmf_copydir = $(PLUGIN_PATH)

tsmf_copy_LTLIBRARIES = tsmf_copy.la

tsmf_copy_la_SOURCES = \
	tsmf_copy.c

tsmf_copy_la_CFLAGS = \
	-I$(top_srcdir)/include -I$(srcdir)/.. -I$(srcdir)/../.. -I$(srcdir)/../../../common \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

tsmf_copy_la_LDFLAGS = -avoid-version -module

tsmf_copy_la_LIBADD = ../../../common/libcommon.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

