## Process this file with automake to produce Makefile.in

# libkbd
libkbddir = $(libdir)

libkbd_LTLIBRARIES = libkbd.la

libkbd_la_SOURCES = \
	debug.h \
	keyboard.h \
	layout_ids.c layout_ids.h \
	locales.c locales.h \
	x_layout_id_table.c x_layout_id_table.h \
	layouts_xkb.c layouts_xkb.h \
	libkbd.c

libkbd_la_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include \
	-DKEYMAP_PATH=\"$(KEYMAP_PATH)\"

libkbd_la_LDFLAGS = @XKBFILE_LIBS@

# extra
EXTRA_DIST =

DISTCLEANFILES = 

