## Process this file with automake to produce Makefile.in

# DirectFB FreeRDP UI
bin_PROGRAMS = dfbfreerdp

dfbfreerdp_SOURCES = \
	dfb_colour.c dfb_colour.h \
	dfb_event.c  dfb_event.h \
	dfb_keyboard.c dfb_keyboard.h \
	dfb_win.h dfb_win.c \
	dfbfreerdp.c

dfbfreerdp_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include

dfbfreerdp_LDADD = \
	../libfreerdp/libfreerdp.la \
	../libfreerdpkbd/libfreerdpkbd.la \
	../libfreerdpchanman/libfreerdpchanman.la \
	-ldirectfb -lfusion -ldirect -lz -lpthread


