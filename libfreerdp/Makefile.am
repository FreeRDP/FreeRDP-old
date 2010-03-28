## Process this file with automake to produce Makefile.in

# libfreerdp
libfreerdpdir = $(libdir)

libfreerdp_LTLIBRARIES = libfreerdp.la

libfreerdp_la_SOURCES = \
	asn1.c asn1.h \
	bitmap.c bitmap.h \
	cache.c cache.h \
	capabilities.c capabilities.h \
	chan.c chan.h \
	freerdp.c \
	iso.c iso.h \
	licence.c licence.h \
	mcs.c mcs.h \
	mem.c mem.h \
	mppc.c \
	orders.c orders.h \
	orderstypes.h \
	stream.h \
	pstcache.c pstcache.h \
	rail.c rail.h \
	rdp.c rdp.h \
	rdp5.c \
	secure.c secure.h \
	credssp.c credssp.h \
	ssl.c ssl.h \
	tcp.c tcp.h \
	types.h debug.h frdp.h

libfreerdp_la_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/include/freerdp \
	-I$(top_srcdir)/asn1 -DFREERDP_EXPORTS

libfreerdp_la_LDFLAGS = ../asn1/libasn1.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

