## Process this file with automake to produce Makefile.in

# rdesktop
bin_PROGRAMS = xfreerdp

xfreerdp_SOURCES = \
	xf_colour.c xf_colour.h \
	xf_event.c  xf_event.h \
	xf_keyboard.c xf_keyboard.h \
	xf_types.h \
	xf_win.h xf_win.c \
	xfreerdp.c

xfreerdp_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include @XCURSOR_CFLAGS@ @X_CFLAGS@

xfreerdp_LDADD = \
	../libfreerdp/libfreerdp.la \
	../libfreerdpkbd/libfreerdpkbd.la \
	../libfreerdpchanman/libfreerdpchanman.la \
	@XCURSOR_LIBS@ @X_LIBS@ @X_EXTRA_LIBS@ \
	-lpthread




