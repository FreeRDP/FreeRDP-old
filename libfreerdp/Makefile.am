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
	constants.h \
	constants_capabilities.h \
	constants_core.h \
	constants_crypto.h \
	constants_license.h \
	constants_pdu.h \
	constants_rail.h \
	constants_window.h \
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
	crypto.h \
	tcp.c tcp.h \
	types.h debug.h frdp.h

if CRYPTO_NSS
libfreerdp_la_SOURCES += crypto_nss.c
endif

if CRYPTO_GNUTLS
libfreerdp_la_SOURCES += crypto_gnutls.c
endif

if CRYPTO_OPENSSL
libfreerdp_la_SOURCES += crypto_openssl.c
endif

if CRYPTO_SIMPLE
libfreerdp_la_SOURCES += crypto_simple.c ssl.c ssl.h
endif

if ENABLE_TLS
libfreerdp_la_SOURCES += tls.c tls.h credssp.c credssp.h
endif

libfreerdp_la_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/include/freerdp \
	-I$(top_srcdir)/asn1 @CRYPTO_CFLAGS@ -DFREERDP_EXPORTS

libfreerdp_la_LDFLAGS =

libfreerdp_la_LIBADD = ../asn1/libasn1.la @CRYPTO_LIBS@ @LIBICONV@

# extra
EXTRA_DIST =

DISTCLEANFILES = 

