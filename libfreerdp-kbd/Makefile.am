## Process this file with automake to produce Makefile.in

# libfreerdp-kbd
libfreerdp_kbddir = $(libdir)

libfreerdp_kbd_LTLIBRARIES = libfreerdp-kbd.la

libfreerdp_kbd_la_SOURCES = \
	debug.h \
	keyboard.h \
	layout_ids.c layout_ids.h \
	locales.c locales.h \
	x_layout_id_table.c x_layout_id_table.h \
	layouts_xkb.c layouts_xkb.h \
	libkbd.c

libfreerdp_kbd_la_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	-DKEYMAP_PATH=\"$(KEYMAP_PATH)\"

libfreerdp_kbd_la_LDFLAGS = @XKBFILE_LIBS@

# extra
EXTRA_DIST =

DISTCLEANFILES = 

