## Process this file with automake to produce Makefile.in

# FreeRDP cunit tests
bin_PROGRAMS = test_freerdp

test_freerdp_SOURCES = \
	test_freerdp.c \
	test_libfreerdpgdi.c \
	test_gdi_window.c

test_freerdp_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/libfreerdpgdi

test_freerdp_LDADD = \
	../libfreerdp/libfreerdp.la \
	../libfreerdpkbd/libfreerdpkbd.la \
	../libfreerdpgdi/libfreerdpgdi.la \
	../libfreerdpchanman/libfreerdpchanman.la \
	-lfusion -ldirect -lz -lpthread -lcunit



