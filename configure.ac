AC_INIT(freerdp, m4_esyscmd([build-aux/git-version-gen .tarball-version]),
        [freerdp-devel@lists.sourceforge.net])
AM_INIT_AUTOMAKE([1.11 dist-xz color-tests parallel-tests])
AM_SILENT_RULES([yes]) # make --enable-silent-rules the default.
AM_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([libfreerdp/freerdp.c])

AC_PROG_CC
AM_PROG_CC_C_O
if test "$GCC" = yes; then
    CFLAGS="$CFLAGS -Wall"
fi

AH_TEMPLATE(B_ENDIAN, [Big endian])
AH_TEMPLATE(L_ENDIAN, [Little endian])
AH_TEMPLATE(EGD_SOCKET, [EGD])
AH_TEMPLATE(HAVE_LIBSAMPLERATE, [Define if you have libsamplerate])
AH_TEMPLATE(IPv6, [IPv6])
AH_TEMPLATE(NEED_ALIGN, [Alignment])
AH_TEMPLATE(DISABLE_TLS, [Disable TLS encryption])
AH_TEMPLATE(WITH_SCARD, [Define if smartcard is enabled])
AH_TEMPLATE(WITH_DEBUG, [Turn on debugging messages])
AH_TEMPLATE(WITH_DEBUG_NLA, [Turn on debugging messages])
AH_TEMPLATE(WITH_DEBUG_CHANNEL, [Turn on debugging messages])
AH_TEMPLATE(WITH_DEBUG_KBD, [Turn on debugging messages])
AH_TEMPLATE(WITH_DEBUG_DRAW, [Turn on debugging messages])
AH_TEMPLATE(WITH_DEBUG_SCARD, [Turn on debugging messages])
AH_TEMPLATE(WITH_DEBUG_SERIAL, [Turn on debugging messages])
AH_TEMPLATE(WITH_DEBUG_SOUND, [Turn on debugging messages])
AH_TEMPLATE(WITH_DEBUG_ASSERT, [Turn on debugging messages])
AH_TEMPLATE(WITH_DEBUG_STREAM_ASSERT, [Turn on debugging messages])

AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_LANG_C
AC_HEADER_STDC
AC_C_BIGENDIAN([AC_DEFINE(B_ENDIAN)], [AC_DEFINE(L_ENDIAN)])

AC_PATH_PROG(PKG_CONFIG, pkg-config)

AC_SEARCH_LIBS(socket, socket)
AC_SEARCH_LIBS(inet_aton, resolv)

AC_CHECK_HEADERS(sys/select.h sys/modem.h sys/filio.h sys/strtio.h)
AC_CHECK_HEADERS(locale.h langinfo.h)

AC_CHECK_TOOL(STRIP, strip, :)

dnl Don't depend on pkg-config
m4_ifdef([PKG_CHECK_MODULES], [], [
		m4_errprint([warning: pkg-config checks are not available])
		m4_defun([PKG_CHECK_MODULES], [
			AC_MSG_WARN([pkg-config not available, cannot check for $2])
			$4
		])
])

rpath=""

dnl Define keymap path
KEYMAP_PATH="${datadir}/freerdp/keymaps"
AC_SUBST(KEYMAP_PATH)

dnl Define plugin path
PLUGIN_PATH='${libdir}/freerdp'
AC_SUBST(PLUGIN_PATH)

dnl Define extensions path
EXT_PATH='${PLUGIN_PATH}/extensions'
AC_SUBST(EXT_PATH)

#
# Check Crypto libraries
#
AC_ARG_WITH([crypto],
	[AS_HELP_STRING([--with-crypto=ARG],
		[Choose the crypto library. Valid arguments are nss, gnutls, openssl (the default), polarssl or simple])],
	[with_crypto=$withval],
	[with_crypto=openssl])

disable_tls=yes
AM_CONDITIONAL(CRYPTO_NSS, false)
AM_CONDITIONAL(CRYPTO_GNUTLS, false)
AM_CONDITIONAL(CRYPTO_OPENSSL, false)
AM_CONDITIONAL(CRYPTO_POLARSSL, false)
AM_CONDITIONAL(CRYPTO_SIMPLE, false)
case $with_crypto in
	nss)
		PKG_CHECK_MODULES(CRYPTO, [nss])
		AM_CONDITIONAL(CRYPTO_NSS, true)
		;;

	gnutls)
		PKG_CHECK_MODULES(CRYPTO, [gnutls >= 2.10.1])
		AM_CONDITIONAL(CRYPTO_GNUTLS, true)
		;;

	openssl)
		PKG_CHECK_MODULES(CRYPTO, [openssl >= 0.9.8a], [], [
			# For systems like FreeBSD, openssl.pc is not installed. Detect it directly in a more common way
			AC_CHECK_LIB(crypto, [CRYPTO_new_ex_data], [], [AC_MSG_ERROR([library 'crypto' is required for OpenSSL])])
			AC_CHECK_LIB(ssl, [SSL_library_init], [], [AC_MSG_ERROR([library 'ssl' is required for OpenSSL])])
			AC_CHECK_HEADER(openssl/ssl.h, [], [AC_MSG_ERROR(OpenSSL development header not found)])
			CRYPTO_LIBS="-lssl -lcrypto"
			CRYPTO_CFLAGS=
			AC_SUBST(CRYPTO_LIBS)
			AC_SUBST(CRYPTO_CFLAGS)
		])
		AM_CONDITIONAL(CRYPTO_OPENSSL, true)
		disable_tls=no
		;;

	polarssl)
		AC_CHECK_LIB(polarssl, ssl_init,
			[CRYPTO_LIBS=-lpolarssl],
			[AC_MSG_ERROR(PolarSSL library not found)])
		AC_CHECK_HEADER(polarssl/ssl.h,
			[CRYPTO_CFLAGS=],
			[AC_MSG_ERROR(PolarSSL development header not found)])
		AC_SUBST(CRYPTO_LIBS)
		AC_SUBST(CRYPTO_CFLAGS)
		AM_CONDITIONAL(CRYPTO_POLARSSL, true)
		;;

	simple)
		AC_MSG_WARN([Using simple built-in cryptographic library.])
		AM_CONDITIONAL(CRYPTO_SIMPLE, true)
		;;

	*)
		AC_MSG_ERROR([Invalid --with-crypto argument $with_crypto.])
		;;
esac

#
# Disable TLS encryption
#

AC_ARG_ENABLE(tls,
	[AS_HELP_STRING([--disable-tls], [disable TLS encryption])],
	[
		if test $enableval = "no";
		then
			disable_tls=yes
		fi
    ])

AM_CONDITIONAL(ENABLE_TLS, true)

if test "$disable_tls" = "yes"; then
	AC_DEFINE(DISABLE_TLS,1)
	AM_CONDITIONAL(ENABLE_TLS, false)
fi

#
# Smartcard
#
AC_ARG_ENABLE(smartcard,
             [  --enable-smartcard	  Enables smart-card support.
	     ],
             [
			  if test "$enableval" = no; then
				  WITH_SCARD=0
			  else
	     	case "$OSTYPE" in
			darwin*)
				AC_CHECK_HEADER(PCSC/pcsclite.h, [WITH_SCARD=1], [WITH_SCARD=0])
				PCSCLITE_CFLAGS=""
				PCSCLITE_LIBS="-framework PCSC"
				;;
			*)
				if test -n "$PKG_CONFIG"; then
					PKG_CHECK_MODULES(PCSCLITE, libpcsclite, [WITH_SCARD=1], [WITH_SCARD=0])
				fi
				;;
		esac

		if test x"$WITH_SCARD" = "x1"; then
			SCARDOBJ="scard.o"
			CFLAGS="$CFLAGS $PCSCLITE_CFLAGS"
			LIBS="$LIBS $PCSCLITE_LIBS"
			AC_DEFINE(WITH_SCARD)
		fi

		AC_MSG_CHECKING([for old version of PCSC])
		AC_TRY_LINK([
			#include <stdlib.h>
			#ifdef __APPLE__
			#include <PCSC/wintypes.h>
			#include <PCSC/winscard.h>
			#else
			#include <winscard.h>
			#endif
			],
			[SCardControl(NULL, NULL, 0, NULL, NULL);],
			[AC_MSG_RESULT(yes) AC_DEFINE(WITH_PCSC120, 1, [old version of PCSC])],
			[AC_MSG_RESULT(no)]
		)
		fi
             ])

AC_SUBST(SCARDOBJ)

#
# Alignment
#
AC_MSG_CHECKING([if architecture needs alignment])
AC_TRY_RUN([
#include <stdlib.h>
#include <signal.h>
int main(int argc, char **argv)
{
	unsigned char test[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
	signal(SIGBUS, exit);
	signal(SIGABRT, exit);
	signal(SIGSEGV, exit);
	if (*((unsigned int *)(test + 1)) != 0x55443322 && *((unsigned int *)(test + 1)) != 0x22334455) {
		return 1;
	}
	return 0;
}],
 [AC_MSG_RESULT(no)],
 [AC_MSG_RESULT(yes)
  AC_DEFINE(NEED_ALIGN)],
 [AC_MSG_RESULT(assuming yes)
  AC_DEFINE(NEED_ALIGN)])


#
# sound
#
PKG_CHECK_MODULES(ALSA, alsa, [
	HAVE_ALSA=1
	alsa="yes"
], [
	HAVE_ALSA=0
	alsa="no"
])

PKG_CHECK_MODULES(PULSE, libpulse, [
	HAVE_PULSE=1
	pulse="yes"
], [
	HAVE_PULSE=0
	pulse="no"
])

#
# printer
#

AC_ARG_WITH([printer],
	[AS_HELP_STRING([--with-printer=ARG],
		[Choose the printer module. Valid arguments are cups, no (default is cups)])],
	[printer=$withval],
	[printer=cups])

AM_CONDITIONAL(RDPDR_PRINTER_CUPS, false)

case $printer in
	cups)
		AC_CHECK_PROG(HAVE_CUPS, cups-config, 1, 0)
		if test x"$HAVE_CUPS" = "x1"; then
			CUPS_CFLAGS=`cups-config --cflags`
			AC_SUBST(CUPS_CFLAGS)
			CUPS_LIBS=`cups-config --libs`
			AC_SUBST(CUPS_LIBS)
			AM_CONDITIONAL(RDPDR_PRINTER_CUPS, true)
		else
			AC_MSG_WARN([Printer support disabled. CUPS development package not found.])
			printer=no
		fi
		;;

	no)
		AC_MSG_WARN([Printer support disabled.])
		;;

	*)
		AC_MSG_ERROR([Invalid --with-printer argument $with_printer.])
		;;
esac

# Checks for time zones
AC_STRUCT_TIMEZONE

#
# dirfd
#
dnl Find out how to get the file descriptor associated with an open DIR*.
dnl From Jim Meyering

AC_DEFUN([UTILS_FUNC_DIRFD],
[

  AC_HEADER_DIRENT
  dirfd_headers='
#if HAVE_DIRENT_H
# include <dirent.h>
#else /* not HAVE_DIRENT_H */
# define dirent direct
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# if HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */
'
  AC_CHECK_FUNCS(dirfd)
  AC_CHECK_DECLS([dirfd], , , $dirfd_headers)

  AC_CACHE_CHECK([whether dirfd is a macro],
    jm_cv_func_dirfd_macro,
    [AC_EGREP_CPP([dirent_header_defines_dirfd], [$dirfd_headers
#ifdef dirfd
 dirent_header_defines_dirfd
#endif],
       jm_cv_func_dirfd_macro=yes,
       jm_cv_func_dirfd_macro=no)])

  # Use the replacement only if we have no function, macro,
  # or declaration with that name.
  if test $ac_cv_func_dirfd,$ac_cv_have_decl_dirfd,$jm_cv_func_dirfd_macro \
      = no,no,no; then
    AC_REPLACE_FUNCS([dirfd])
    AC_CACHE_CHECK(
	      [how to get the file descriptor associated with an open DIR*],
		   gl_cv_sys_dir_fd_member_name,
      [
        dirfd_save_CFLAGS=$CFLAGS
	for ac_expr in d_fd dd_fd; do

	  CFLAGS="$CFLAGS -DDIR_FD_MEMBER_NAME=$ac_expr"
	  AC_TRY_COMPILE(
	    [$dirfd_headers
	    ],
	    [DIR *dir_p = opendir("."); (void) dir_p->DIR_FD_MEMBER_NAME;],
	    dir_fd_found=yes
	  )
	  CFLAGS=$dirfd_save_CFLAGS
	  test "$dir_fd_found" = yes && break
	done
	test "$dir_fd_found" = yes || ac_expr=no_such_member

	gl_cv_sys_dir_fd_member_name=$ac_expr
      ]
    )
    if test $gl_cv_sys_dir_fd_member_name != no_such_member; then
      AC_DEFINE_UNQUOTED(DIR_FD_MEMBER_NAME,
	$gl_cv_sys_dir_fd_member_name,
	[the name of the file descriptor member of DIR])
    fi
    AH_VERBATIM(DIR_TO_FD,
		[#ifdef DIR_FD_MEMBER_NAME
# define DIR_TO_FD(Dir_p) ((Dir_p)->DIR_FD_MEMBER_NAME)
#else
# define DIR_TO_FD(Dir_p) -1
#endif
]
    )
  fi
])

UTILS_FUNC_DIRFD

#
# iconv
#

dnl This macros shamelessly stolen from
dnl http://gcc.gnu.org/ml/gcc-bugs/2001-06/msg01398.html.
dnl Written by Bruno Haible.

AC_DEFUN([UTILS_FUNC_ICONV],
[
  dnl Some systems have iconv in libc, some have it in libiconv (OSF/1 and
  dnl those with the standalone portable GNU libiconv installed).

  AC_ARG_WITH([libiconv-prefix],
[  --with-libiconv-prefix=DIR  search for libiconv in DIR/include and DIR/lib], [
    for dir in `echo "$withval" | tr : ' '`; do
      if test -d $dir/include; then CPPFLAGS="$CPPFLAGS -I$dir/include"; fi
      if test -d $dir/lib; then LDFLAGS="$LDFLAGS -L$dir/lib"; fi
    done
   ])
  AC_CHECK_HEADERS(iconv.h)

  AC_CACHE_CHECK(for iconv, am_cv_func_iconv, [
    am_cv_func_iconv="no, consider installing GNU libiconv"
    am_cv_lib_iconv=no
    AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
      [iconv_t cd = iconv_open("","");
       iconv(cd,NULL,NULL,NULL,NULL);
       iconv_close(cd);],
      am_cv_func_iconv=yes)
    if test "$am_cv_func_iconv" != yes; then
      am_save_LIBS="$LIBS"
      LIBS="$LIBS -liconv"
      AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
        [iconv_t cd = iconv_open("","");
         iconv(cd,NULL,NULL,NULL,NULL);
         iconv_close(cd);],
        am_cv_lib_iconv=yes
        am_cv_func_iconv=yes)
      LIBS="$am_save_LIBS"
    fi
  ])
  if test "$am_cv_func_iconv" = yes; then
    AC_DEFINE(HAVE_ICONV, 1, [Define if you have the iconv() function.])
    AC_MSG_CHECKING([for iconv declaration])
    AC_CACHE_VAL(am_cv_proto_iconv, [
      AC_TRY_COMPILE([
#include <stdlib.h>
#include <iconv.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
#else
size_t iconv();
#endif
], [], am_cv_proto_iconv_arg1="", am_cv_proto_iconv_arg1="const")
      am_cv_proto_iconv="extern size_t iconv (iconv_t cd, $am_cv_proto_iconv_arg1 char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);"])
    am_cv_proto_iconv=`echo "[$]am_cv_proto_iconv" | tr -s ' ' | sed -e 's/( /(/'`
    AC_MSG_RESULT([$]{ac_t:-
         }[$]am_cv_proto_iconv)
    AC_DEFINE_UNQUOTED(ICONV_CONST, $am_cv_proto_iconv_arg1,
      [Define as const if the declaration of iconv() needs const.])
  fi
  LIBICONV=
  if test "$am_cv_lib_iconv" = yes; then
    LIBICONV="-liconv"
  fi
  AC_SUBST(LIBICONV)
])

AC_ARG_ENABLE(iconv,
	[AS_HELP_STRING([--disable-iconv], [disable iconv support])],
	[
		if test $enableval != "no";
		then
			UTILS_FUNC_ICONV
		fi
    ],
	[
		UTILS_FUNC_ICONV
	])
LIBS="$LIBS $LIBICONV"

#
# socklen_t
# from curl

dnl Check for socklen_t: historically on BSD it is an int, and in
dnl POSIX 1g it is a type of its own, but some platforms use different
dnl types for the argument to getsockopt, getpeername, etc.  So we
dnl have to test to find something that will work.
AC_DEFUN([TYPE_SOCKLEN_T],
[
   AC_CHECK_TYPE([socklen_t], ,[
      AC_MSG_CHECKING([for socklen_t equivalent])
      AC_CACHE_VAL([socklen_t_cv_equiv],
      [
         # Systems have either "struct sockaddr *" or
         # "void *" as the second argument to getpeername
         socklen_t_cv_equiv=
         for arg2 in "struct sockaddr" void; do
            for t in int size_t unsigned long "unsigned long"; do
               AC_TRY_COMPILE([
                  #include <sys/types.h>
                  #include <sys/socket.h>

                  int getpeername (int, $arg2 *, $t *);
               ],[
                  $t len;
                  getpeername(0,0,&len);
               ],[
                  socklen_t_cv_equiv="$t"
                  break
               ])
            done
         done

         if test "x$socklen_t_cv_equiv" = x; then
            AC_MSG_ERROR([Cannot find a type to use in place of socklen_t])
         fi
      ])
      AC_MSG_RESULT($socklen_t_cv_equiv)
      AC_DEFINE_UNQUOTED(socklen_t, $socklen_t_cv_equiv,
			[type to use in place of socklen_t if not defined])],
      [#include <sys/types.h>
#include <sys/socket.h>])
])

TYPE_SOCKLEN_T

#
# statfs stuff
#
AC_CHECK_HEADERS(sys/vfs.h)
AC_CHECK_HEADERS(sys/statvfs.h)
AC_CHECK_HEADERS(sys/statfs.h)
AC_CHECK_HEADERS(sys/param.h)

mount_includes="\
  $ac_includes_default
  #if HAVE_SYS_PARAM_H
  # include <sys/param.h>
  #endif
  "

AC_CHECK_HEADERS(sys/mount.h,,,[$mount_includes])

#################################################
# these tests are taken from the GNU fileutils package
AC_CHECKING(how to get filesystem space usage)
space=no

# Test for statvfs64.
if test $space = no; then
  # SVR4
  AC_CACHE_CHECK([statvfs64 function (SVR4)], fu_cv_sys_stat_statvfs64,
  [AC_TRY_RUN([
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/statvfs.h>
  main ()
  {
    struct statvfs64 fsd;
    exit (statvfs64 (".", &fsd));
  }],
  fu_cv_sys_stat_statvfs64=yes,
  fu_cv_sys_stat_statvfs64=no,
  fu_cv_sys_stat_statvfs64=cross)])
  if test $fu_cv_sys_stat_statvfs64 = yes; then
    space=yes
    AC_DEFINE(STAT_STATVFS64,1,[Whether statvfs64() is available])
  fi
fi

# Perform only the link test since it seems there are no variants of the
# statvfs function.  This check is more than just AC_CHECK_FUNCS(statvfs)
# because that got a false positive on SCO OSR5.  Adding the declaration
# of a `struct statvfs' causes this test to fail (as it should) on such
# systems.  That system is reported to work fine with STAT_STATFS4 which
# is what it gets when this test fails.
if test $space = no; then
  # SVR4
  AC_CACHE_CHECK([statvfs function (SVR4)], fu_cv_sys_stat_statvfs,
                 [AC_TRY_LINK([#include <sys/types.h>
#include <sys/statvfs.h>],
                              [struct statvfs fsd; statvfs (0, &fsd);],
                              fu_cv_sys_stat_statvfs=yes,
                              fu_cv_sys_stat_statvfs=no)])
  if test $fu_cv_sys_stat_statvfs = yes; then
    space=yes
    AC_DEFINE(STAT_STATVFS,1,[Whether statvfs() is available])
  fi
fi

if test $space = no; then
  # DEC Alpha running OSF/1
  AC_MSG_CHECKING([for 3-argument statfs function (DEC OSF/1)])
  AC_CACHE_VAL(fu_cv_sys_stat_statfs3_osf1,
  [AC_TRY_RUN([
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
  main ()
  {
    struct statfs fsd;
    fsd.f_fsize = 0;
    exit (statfs (".", &fsd, sizeof (struct statfs)));
  }],
  fu_cv_sys_stat_statfs3_osf1=yes,
  fu_cv_sys_stat_statfs3_osf1=no,
  fu_cv_sys_stat_statfs3_osf1=no)])


#C_MSG_RESULT($fu_cv_sys_stat_statfs3_osf1)
  if test $fu_cv_sys_stat_statfs3_osf1 = yes; then
    space=yes
    AC_DEFINE(STAT_STATFS3_OSF1,1,[Whether statfs requires 3 arguments])
  fi
fi

if test $space = no; then
# AIX
  AC_MSG_CHECKING([for two-argument statfs with statfs.bsize dnl
member (AIX, 4.3BSD)])
  AC_CACHE_VAL(fu_cv_sys_stat_statfs2_bsize,
  [AC_TRY_RUN([
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
  main ()
  {
  struct statfs fsd;
  fsd.f_bsize = 0;
  exit (statfs (".", &fsd));
  }],
  fu_cv_sys_stat_statfs2_bsize=yes,
  fu_cv_sys_stat_statfs2_bsize=no,
  fu_cv_sys_stat_statfs2_bsize=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_statfs2_bsize)
  if test $fu_cv_sys_stat_statfs2_bsize = yes; then
    space=yes
    AC_DEFINE(STAT_STATFS2_BSIZE,1,[Whether statfs requires two arguments and struct statfs has bsize property])
  fi
fi

if test $space = no; then
# SVR3
  AC_MSG_CHECKING([for four-argument statfs (AIX-3.2.5, SVR3)])
  AC_CACHE_VAL(fu_cv_sys_stat_statfs4,
  [AC_TRY_RUN([#include <sys/types.h>
#include <sys/statfs.h>
  main ()
  {
  struct statfs fsd;
  exit (statfs (".", &fsd, sizeof fsd, 0));
  }],
    fu_cv_sys_stat_statfs4=yes,
    fu_cv_sys_stat_statfs4=no,
    fu_cv_sys_stat_statfs4=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_statfs4)
  if test $fu_cv_sys_stat_statfs4 = yes; then
    space=yes
    AC_DEFINE(STAT_STATFS4,1,[Whether statfs requires 4 arguments])
  fi
fi

if test $space = no; then
# 4.4BSD and NetBSD
  AC_MSG_CHECKING([for two-argument statfs with statfs.fsize dnl
member (4.4BSD and NetBSD)])
  AC_CACHE_VAL(fu_cv_sys_stat_statfs2_fsize,
  [AC_TRY_RUN([#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
  main ()
  {
  struct statfs fsd;
  fsd.f_fsize = 0;
  exit (statfs (".", &fsd));
  }],
  fu_cv_sys_stat_statfs2_fsize=yes,
  fu_cv_sys_stat_statfs2_fsize=no,
  fu_cv_sys_stat_statfs2_fsize=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_statfs2_fsize)
  if test $fu_cv_sys_stat_statfs2_fsize = yes; then
    space=yes
        AC_DEFINE(STAT_STATFS2_FSIZE,1,[Whether statfs requires 2 arguments and struct statfs has fsize])
  fi
fi

if test $space = no; then
  # Ultrix
  AC_MSG_CHECKING([for two-argument statfs with struct fs_data (Ultrix)])
  AC_CACHE_VAL(fu_cv_sys_stat_fs_data,
  [AC_TRY_RUN([#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_FS_TYPES_H
#include <sys/fs_types.h>
#endif
  main ()
  {
  struct fs_data fsd;
  /* Ultrix's statfs returns 1 for success,
     0 for not mounted, -1 for failure.  */
  exit (statfs (".", &fsd) != 1);
  }],
  fu_cv_sys_stat_fs_data=yes,
  fu_cv_sys_stat_fs_data=no,
  fu_cv_sys_stat_fs_data=no)])
  AC_MSG_RESULT($fu_cv_sys_stat_fs_data)
  if test $fu_cv_sys_stat_fs_data = yes; then
    space=yes
    AC_DEFINE(STAT_STATFS2_FS_DATA,1,[Whether statfs requires 2 arguments and struct fs_data is available])
  fi
fi

  statxfs_includes="\
$ac_includes_default
#if HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif
#if HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif
#if !HAVE_SYS_STATVFS_H && !HAVE_SYS_VFS_H
# if HAVE_SYS_MOUNT_H && HAVE_SYS_PARAM_H
/* NetBSD 1.5.2 needs these, for the declaration of struct statfs. */
#  include <sys/param.h>
#  include <sys/mount.h>
# elif HAVE_NETINET_IN_H && HAVE_NFS_NFS_CLNT_H && HAVE_NFS_VFS_H
/* Ultrix 4.4 needs these for the declaration of struct statfs.  */
#  include <netinet/in.h>
#  include <nfs/nfs_clnt.h>
#  include <nfs/vfs.h>
# endif
#endif
"

AC_CHECK_MEMBERS([struct statfs.f_namemax],,,[$statxfs_includes])
AC_CHECK_MEMBERS([struct statvfs.f_namemax],,,[$statxfs_includes])
AC_CHECK_MEMBERS([struct statfs.f_namelen],,,[$statxfs_includes])
AC_CHECK_MEMBERS([struct statvfs.f_namelen],,,[$statxfs_includes])

#
# Large file support
#
AC_SYS_LARGEFILE

#
# mntent
#
AC_CHECK_HEADERS(mntent.h)
AC_CHECK_FUNCS(setmntent)

#
# IPv6
#
AC_ARG_ENABLE(ipv6,
	[AS_HELP_STRING([--disable-ipv6], [disable IPv6 support])],
	[
		if test $enableval != "no";
		then
			AC_DEFINE(IPv6,1)
		fi
    ],
	[
		AC_DEFINE(IPv6,1)
	])

#
# debugging
#
AC_ARG_WITH(debug,
    [  --with-debug            enable protocol debugging output],
    [
        if test $withval != "no";
        then
            AC_DEFINE(WITH_DEBUG,1)
            debug_support=yes
        fi
    ])

AC_ARG_WITH(debug-nla,
    [  --with-debug-nla        enable debugging of network level authentication],
    [
        if test $withval != "no";
        then
            AC_DEFINE(WITH_DEBUG_NLA,1)
	fi
    ])

AC_ARG_WITH(debug-kbd,
    [  --with-debug-kbd        enable debugging of keyboard handling],
    [
        if test $withval != "no";
        then
            AC_DEFINE(WITH_DEBUG_KBD,1)
            debug_support=yes
        fi
    ])

AC_ARG_WITH(debug-rdp5,
    [  --with-debug-rdp5       enable debugging of drawing code],
    [
        if test $withval != "no";
        then
            AC_DEFINE(WITH_DEBUG_DRAW,1)
            debug_support=yes
        fi
    ])

AC_ARG_WITH(debug-sound,
    [  --with-debug-sound      enable debugging of sound code],
    [
        if test $withval != "no";
        then
            AC_DEFINE(WITH_DEBUG_SOUND,1)
            debug_support=yes
        fi
    ])

AC_ARG_WITH(debug-channel,
    [  --with-debug-channel    enable debugging of virtual channel code],
    [
        if test $withval != "no";
        then
            AC_DEFINE(WITH_DEBUG_CHANNEL,1)
            debug_support=yes
        fi
    ])

AC_ARG_WITH(debug-serial,
    [  --with-debug-serial     enable debugging of Serial code],
    [
        if test $withval != "no";
        then
            AC_DEFINE(WITH_DEBUG_SERIAL,1)
            debug_support=yes
        fi
    ])

AC_ARG_WITH(debug-smartcard,
    [  --with-debug-smartcard  enable debugging of smart-card code],
    [
        if test $withval != "no";
        then
          if test x"$WITH_SCARD" = "x1"; then
            AC_DEFINE(WITH_DEBUG_SCARD,1)
            debug_support=yes
          fi
        fi
    ])

AC_ARG_WITH(debug-assert,
    [  --with-debug-assert     enable assert debugging],
    [
        if test $withval != "no";
        then
            AC_DEFINE(WITH_DEBUG_ASSERT,1)
            debug_support=yes
        fi
    ])

AC_ARG_WITH(debug-stream-assert,
    [  --with-debug-stream-assert     enable stream assert debugging],
    [
        if test $withval != "no";
        then
            AC_DEFINE(WITH_DEBUG_STREAM_ASSERT,1)
            debug_support=yes
        fi
    ])

if test "$debug_support" != "yes"; then
    CFLAGS="$CFLAGS -DNDEBUG"
fi

#
# target-specific stuff
#
# strip leading colon from rpath
rpath=`echo $rpath |sed 's/^://'`
AC_CANONICAL_HOST
case "$host" in
*-*-solaris*)
    LDFLAGS="$LDFLAGS -R$rpath"
    ;;
*-dec-osf*)
    LDFLAGS="$LDFLAGS -Wl,-rpath,$rpath"
    ;;
*-*-hpux*)
    CFLAGS="$CFLAGS -D_XOPEN_SOURCE_EXTENDED"
    ;;
*-*-irix6.5*)
    CFLAGS="$CFLAGS -D__SGI_IRIX__"
    ;;
esac

#
# Optional build targets
#
EXTRA_SUBDIRS=""

#
# ALSA
#
if test x"$HAVE_ALSA" = "x1"; then
	EXTRA_SUBDIRS="$EXTRA_SUBDIRS channels/rdpsnd/alsa channels/drdynvc/audin/alsa"
fi

#
# PulseAudio
#
if test x"$HAVE_PULSE" = "x1"; then
	EXTRA_SUBDIRS="$EXTRA_SUBDIRS channels/rdpsnd/pulse"
fi

#
# CUnit
#
cunit="no"
AC_ARG_WITH([cunit],
	[AS_HELP_STRING([--with-cunit], [Build CUnit])],
	[
		AC_CHECK_HEADER(CUnit/Basic.h, [], [AC_MSG_ERROR(CUnit development header not found)])
		cunit="yes"
		EXTRA_SUBDIRS="$EXTRA_SUBDIRS cunit"
	]
)

#
# X11
#
AM_CONDITIONAL(WITH_X11, true)
AC_PATH_XTRA
if test "$no_x" = "yes"; then
	x11="no"
	AM_CONDITIONAL(WITH_X11, false)
else
	AC_CHECK_LIB(X11, XOpenDisplay, [X_LIBS="$X_LIBS -lX11"])
	PKG_CHECK_MODULES(XCURSOR, [xcursor])
	x11="yes"
	EXTRA_SUBDIRS="$EXTRA_SUBDIRS X11"
fi

#
# DirectFB (optionally built when --with-dfb specified)
#
dfb="no"
AC_ARG_WITH([dfb],
	[AS_HELP_STRING([--with-dfb], [Build DirectFB UI])],
	[
		# Put dependency check here
		EXTRA_SUBDIRS="$EXTRA_SUBDIRS dfb"
		
		# DirectFB
		PKG_CHECK_MODULES(DFB, [directfb >= 1.0.0])
		dfb="yes"
	])

AC_SUBST(EXTRA_SUBDIRS)

AC_CONFIG_FILES([
Makefile
freerdp.pc
asn1/Makefile
libfreerdputils/Makefile
libfreerdp/Makefile
docs/Makefile
include/Makefile
libfreerdpkbd/Makefile
keymaps/Makefile
cunit/Makefile
libfreerdpgdi/Makefile
libfreerdpchanman/Makefile
X11/Makefile
dfb/Makefile
win/Makefile
channels/common/Makefile
channels/rdpdbg/Makefile
channels/rdpsnd/Makefile
channels/rdpsnd/alsa/Makefile
channels/rdpsnd/pulse/Makefile
channels/drdynvc/Makefile
channels/drdynvc/pnpdr/Makefile
channels/drdynvc/audin/Makefile
channels/drdynvc/audin/alsa/Makefile
channels/cliprdr/Makefile
channels/rdpdr/Makefile
channels/rdpdr/disk/Makefile
channels/rdpdr/printer/Makefile
channels/rdpdr/serial/Makefile
channels/rdpdr/parallel/Makefile
contrib/Makefile
])

AC_OUTPUT

echo
echo "FreeRDP configure result:"
echo
echo "Cryptography : $with_crypto"
if test "x$disable_tls" = "xyes"; then
	echo "TLS          : disabled"
else
	echo "TLS          : enabled"
fi
echo "ALSA         : $alsa"
echo "PulseAudio   : $pulse"
echo "Printer      : $printer"
echo "CUnit        : $cunit"
echo "X11          : $x11"
echo "DirectFB     : $dfb"
