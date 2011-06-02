## Process this file with automake to produce Makefile.in

# xfreerdp
bin_PROGRAMS = xfreerdp

xfreerdp_SOURCES = \
	xf_event.c  xf_event.h \
	xf_keyboard.c xf_keyboard.h \
	xf_types.h \
	xf_win.h xf_win.c \
	xf_video.h xf_video.c \
	xf_decode.h xf_decode.c \
	xfreerdp.c

xfreerdp_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/libgdi \
	-pthread \
	@XCURSOR_CFLAGS@ \
	@XV_CFLAGS@ \
	@X_CFLAGS@

xfreerdp_LDADD = \
	../libgdi/libgdi.la \
	../librfx/librfx.la \
	../libkbd/libkbd.la \
	../libutils/libutils.la \
	../libchanman/libchanman.la \
	../libfreerdp/libfreerdp.la \
	@XCURSOR_LIBS@ @XV_LIBS@ @X_LIBS@ @X_EXTRA_LIBS@

