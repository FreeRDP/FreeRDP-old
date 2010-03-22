## Process this file with automake to produce Makefile.in
SUBDIRS = \
	libfreerdp \
	include \
	libfreerdpkbd \
	keymaps \
	libfreerdpchanman \
	X11 \
	channels/common \
	channels/rdpsnd \
	channels/rdpdr \
	channels/rdpdr/disk

EXTRA_DIST = autogen.sh indent-all.sh freerdp.spec \
	AUTHORS ChangeLog COPYING INSTALL NEWS README doc

## Support version extraction from GIT tree
BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
