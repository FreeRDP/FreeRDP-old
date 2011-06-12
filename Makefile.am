## Process this file with automake to produce Makefile.in
REQUIRED_SUBDIRS = \
	libfreerdp-asn1 \
	libfreerdp-gdi \
	libfreerdp-rfx/sse \
	libfreerdp-rfx \
	libfreerdp-utils \
	libfreerdp-core \
	docs \
	contrib \
	include \
	keymaps \
	libfreerdp-kbd \
	libfreerdp-chanman \
	channels/rdpsnd \
	channels/drdynvc \
	channels/drdynvc/pnpdr \
	channels/drdynvc/tsmf \
	channels/drdynvc/audin \
	channels/cliprdr \
	channels/rdpdbg \
	channels/rdpdr \
	channels/rdpdr/disk \
	channels/rdpdr/printer \
	channels/rdpdr/serial \
	channels/rdpdr/parallel

OPTIONAL_SUBDIRS = \
	channels/rdpdr/smartcard \
	channels/rdpsnd/alsa \
	channels/rdpsnd/pulse \
	channels/drdynvc/audin/alsa \
	channels/drdynvc/audin/pulse \
	channels/drdynvc/tsmf/ffmpeg \
	channels/drdynvc/tsmf/alsa \
	channels/drdynvc/tsmf/pulse \
	cunit \
	X11 \
	win \
	dfb

SUBDIRS = $(REQUIRED_SUBDIRS) $(EXTRA_SUBDIRS)

DIST_SUBDIRS = $(REQUIRED_SUBDIRS) $(OPTIONAL_SUBDIRS)

EXTRA_DIST = autogen.sh build-aux indent-all.sh freerdp.spec \
	AUTHORS ChangeLog INSTALL NEWS README LICENSE .version

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = freerdp.pc

## Support version extraction from GIT tree
BUILT_SOURCES = .version
.version:
	echo $(VERSION) > $@-t && mv $@-t $@

# Arrange so that .tarball-version appears only in the distribution
# tarball, and never in a checked-out repository.
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
