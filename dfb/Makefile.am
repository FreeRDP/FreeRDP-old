## Process this file with automake to produce Makefile.in

# DirectFB FreeRDP UI
bin_PROGRAMS = dfbfreerdp

dfbfreerdp_SOURCES = \
	dfb_event.c  dfb_event.h \
	dfb_keyboard.c dfb_keyboard.h \
	dfb_win.h dfb_win.c \
	dfbfreerdp.c dfbfreerdp.h

dfbfreerdp_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/libfreerdp-gdi \
	-I$(top_srcdir)/libfreerdp-kbd \
	-pthread \
	@DFB_CFLAGS@

dfbfreerdp_LDADD = \
	../libfreerdp-gdi/libfreerdp-gdi.la \
	../libfreerdp-rfx/libfreerdp-rfx.la \
	../libfreerdp-kbd/libfreerdp-kbd.la \
	../libfreerdp-chanman/libfreerdp-chanman.la \
	../libfreerdp-core/libfreerdp-core.la \
	../libfreerdp-utils/libfreerdp-utils.la \
	@DFB_LIBS@ -lfusion -ldirect -lz


