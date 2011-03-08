## Process this file with automake to produce Makefile.in
REQUIRED_SUBDIRS = \
	asn1 \
	libfreerdp \
	doc \
	contrib \
	include \
	libfreerdpkbd \
	keymaps \
	libfreerdpgdi \
	libfreerdputils \
	libfreerdpchanman \
	channels/common \
	channels/rdpsnd \
	channels/drdynvc \
	channels/drdynvc/pnpdr \
	channels/drdynvc/audin \
	channels/cliprdr \
	channels/rdpdr \
	channels/rdpdr/disk \
	channels/rdpdr/printer \
	channels/rdpdr/serial \
	channels/rdpdr/parallel

OPTIONAL_SUBDIRS = \
	channels/rdpsnd/alsa \
	tests \
	cunit \
	X11 \
	win \
	dfb

SUBDIRS = $(REQUIRED_SUBDIRS) $(EXTRA_SUBDIRS)

DIST_SUBDIRS = $(REQUIRED_SUBDIRS) $(OPTIONAL_SUBDIRS)

EXTRA_DIST = autogen.sh build-aux indent-all.sh freerdp.spec \
	AUTHORS ChangeLog COPYING INSTALL NEWS README

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = freerdp.pc

## Support version extraction from GIT tree
BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
