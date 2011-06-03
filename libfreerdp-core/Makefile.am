## Process this file with automake to produce Makefile.in

# libfreerdp-core
libfreerdp_coredir = $(libdir)

libfreerdp_core_LTLIBRARIES = libfreerdp-core.la

libfreerdp_core_la_SOURCES = \
	asn1.c asn1.h \
	bitmap.c bitmap.h \
	cache.c cache.h \
	capabilities.c capabilities.h \
	chan.c chan.h \
	ext.c ext.h \
	freerdp.c \
	iso.c iso.h \
	licence.c licence.h \
	mcs.c mcs.h \
	mppc.c \
	orders.c orders.h \
	orderstypes.h \
	stream.c stream.h \
	pstcache.c pstcache.h \
	rail.c rail.h \
	rdp.c rdp.h \
	secure.c secure.h \
	crypto.h \
	tcp.c tcp.h \
	nego.c nego.h \
	types.h frdp.h \
	errinfo.c \
	surface.c surface.h

if CRYPTO_NSS
libfreerdp_core_la_SOURCES += crypto/nss.c
endif

if CRYPTO_GNUTLS
libfreerdp_core_la_SOURCES += crypto/gnutls.c
endif

if CRYPTO_OPENSSL
libfreerdp_core_la_SOURCES += crypto/openssl.c crypto/openssl.h
endif

if CRYPTO_POLARSSL
libfreerdp_core_la_SOURCES += crypto/polarssl.c
endif

if ENABLE_TLS
libfreerdp_core_la_SOURCES += tls.h credssp.c credssp.h ntlmssp.c ntlmssp.h
endif

libfreerdp_core_la_CFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/libfreerdp-gdi \
	-I$(top_srcdir)/libfreerdp-rfx \
	-I$(top_srcdir)/libfreerdp-asn1 \
	@CRYPTO_CFLAGS@ -DFREERDP_EXPORTS \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\" \
	-DEXT_PATH=\"$(EXT_PATH)\"

libfreerdp_core_la_LDFLAGS =

libfreerdp_core_la_LIBADD = \
	../libfreerdp-gdi/libfreerdp-gdi.la \
	../libfreerdp-rfx/libfreerdp-rfx.la \
	../libfreerdp-asn1/libfreerdp-asn1.la \
	../libfreerdp-utils/libfreerdp-utils.la \
	@CRYPTO_LIBS@ @LIBICONV@

# extra
EXTRA_DIST =

DISTCLEANFILES = 

