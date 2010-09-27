## Process this file with automake to produce Makefile.in

# FreeRDP RCUNIT Tests
bin_PROGRAMS = rcunit

rcunit_SOURCES = \
	rcunit.c

rcunit_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/libfreerdpgdi

rcunit_LDADD = \
	../libfreerdp/libfreerdp.la \
	../libfreerdpkbd/libfreerdpkbd.la \
	../libfreerdpgdi/libfreerdpgdi.la \
	../libfreerdpchanman/libfreerdpchanman.la \
	-lfusion -ldirect -lz -lpthread -lrcunit


