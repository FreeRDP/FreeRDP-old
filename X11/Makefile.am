## Process this file with automake to produce Makefile.in

# rdesktop
bin_PROGRAMS = xfreerdp

xfreerdp_SOURCES = \
	xf_colour.c xf_colour.h \
	xf_event.c  xf_event.h \
	xf_keyboard.c xf_keyboard.h \
	xf_win.h xf_win.c \
	xfreerdp.c

xfreerdp_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/include/freerdp -I$(top_srcdir)/libxkb

xfreerdp_LDADD = ../libfreerdp/libfreerdp.la ../libxkb/libxkb.la -lX11 -lXcursor


