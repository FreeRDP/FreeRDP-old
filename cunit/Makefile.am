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
	-I$(top_srcdir)/libgdi \
	-I$(top_srcdir)/librfx \
	-I$(top_srcdir)/libfreerdp \
	-pthread

test_freerdp_LDADD = \
	../libgdi/libgdi.la \
	../librfx/librfx.la \
	../libfreerdp/libfreerdp.la \
	../libfreerdpkbd/libfreerdpkbd.la \
	../libfreerdpchanman/libfreerdpchanman.la \
	-lfusion -ldirect -lz -lcunit -lncurses


