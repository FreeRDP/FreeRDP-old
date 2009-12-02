## Process this file with automake to produce Makefile.in

# libfreerdp
libfreerdpdir = $(libdir)

libfreerdp_LTLIBRARIES = libfreerdp.la

libfreerdp_la_SOURCES = \
	asn1.c asn1.h \
	bitmap.c bitmap.h \
	cache.c cache.h \
	capabilities.c capabilities.h \
	channels.c channels.h \
	constants.h \
	freerdp.c freerdp.h \
	iso.c iso.h \
	keyboard.h \
	licence.c licence.h \
	locales.h \
	mcs.c mcs.h \
	mem.c mem.h \
	mppc.c \
	orders.c orders.h \
	orderstypes.h \
	parse.h \
	proto.h \
	pstcache.c pstcache.h \
	rail.c rail.h \
	rdp.c rdp.h \
	rdp5.c \
	secure.c secure.h \
	spnego.c  spnego.h \
	ssl.c ssl.h \
	tcp.c tcp.h \
	types.h

libfreerdp_la_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/include/freerdp

libfreerdp_la_LDFLAGS =

# rdesktop (some files should move to libfreerdp above when it's ready
bin_PROGRAMS = rdesktop

rdesktop_SOURCES = \
	cliprdr.c \
	disk.c disk.h \
	irp.c irp.h \
	lspci.c lspci.h \
	rdpdr.c rdpdr.h \
	rdpsnd.c rdpsnd.h \
	rdpsnd_alsa.c \
	rdpsnd_dsp.c rdpsnd_dsp.h \
	rdpsnd_libao.c \
	rdpsnd_oss.c \
	parallel.c \
	printer.c \
	printercache.c \
	serial.c \
	rdesktop.c rdesktop.h \
	xclip.c \
	xkbkeymap.c xkbkeymap.h \
	xproto.h \
	xwin.c

rdesktop_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/include -I$(top_srcdir)/include/freerdp \
	-DKEYMAP_PATH=\"$(KEYMAP_PATH)\"

rdesktop_LDADD = libfreerdp.la -lX11

# extra
EXTRA_DIST =

DISTCLEANFILES = 

