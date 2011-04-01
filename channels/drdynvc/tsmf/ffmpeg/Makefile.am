## Process this file with automake to produce Makefile.in

# tsmf_ffmpeg
tsmf_ffmpegdir = $(PLUGIN_PATH)

tsmf_ffmpeg_LTLIBRARIES = tsmf_ffmpeg.la

tsmf_ffmpeg_la_SOURCES = \
	tsmf_ffmpeg.c

tsmf_ffmpeg_la_CFLAGS = @FFMPEG_CFLAGS@ \
	-I$(top_srcdir)/include -I$(srcdir)/.. -I$(srcdir)/../.. -I$(srcdir)/../../../common \
	-DPLUGIN_PATH=\"$(PLUGIN_PATH)\"

tsmf_ffmpeg_la_LDFLAGS = -avoid-version -module

tsmf_ffmpeg_la_LIBADD = @FFMPEG_LIBS@ ../../../common/libcommon.la

# extra
EXTRA_DIST =

DISTCLEANFILES = 

