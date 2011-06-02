## Process this file with automake to produce Makefile.in

# FreeRDP cunit tests
bin_PROGRAMS = test_freerdp

test_freerdp_SOURCES = \
	test_color.c test_color.h \
	test_libgdi.c test_libgdi.h \
	test_librfx.c test_librfx.h \
	test_ntlmssp.c test_ntlmssp.h \
	test_freerdp.c test_freerdp.h

test_freerdp_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/libfreerdp-gdi \
	-I$(top_srcdir)/libfreerdp-rfx \
	-I$(top_srcdir)/libfreerdp-core \
	-pthread

test_freerdp_LDADD = \
	../libfreerdp-gdi/libfreerdp-gdi.la \
	../libfreerdp-rfx/libfreerdp-rfx.la \
	../libfreerdp-kbd/libfreerdp-kbd.la \
	../libfreerdp-chanman/libfreerdp-chanman.la \
	../libfreerdp-core/libfreerdp-core.la \
	-lfusion -ldirect -lz -lcunit -lncurses


