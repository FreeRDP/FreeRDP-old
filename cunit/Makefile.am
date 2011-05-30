## Process this file with automake to produce Makefile.in

# FreeRDP cunit tests
bin_PROGRAMS = test_freerdp

test_freerdp_SOURCES = \
	test_credssp.c test_credssp.h \
	test_freerdp.c test_freerdp.h \
	test_gdi_color.c test_gdi_color.h \
	test_gdi_window.c test_gdi_window.h \
	test_libgdi.c test_libgdi.h \
	test_librfx.c test_librfx.h \
	test_ntlmssp.c test_ntlmssp.h

test_freerdp_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/libfreerdp \
	-I$(top_srcdir)/libgdi \
	-I$(top_srcdir)/librfx \
	-pthread

test_freerdp_LDADD = \
	../libfreerdp/libfreerdp.la \
	../libgdi/libgdi.la \
	../librfx/librfx.la \
	../libfreerdpkbd/libfreerdpkbd.la \
	../libfreerdpchanman/libfreerdpchanman.la \
	-lfusion -ldirect -lz -lcunit -lncurses


