/*
   rdesktop: A Remote Desktop Protocol client.
   Master include file
   Copyright (C) Matthew Chapman 1999-2008

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __RDESKTOP_H
#define __RDESKTOP_H

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#define WINVER 0x0400
#include <windows.h>
#include <winsock.h>
#include <time.h>
#define DIR int
#else
#include <dirent.h>
#include <sys/time.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif
#endif
#include <limits.h>		/* PATH_MAX */

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

/* X11 includes */
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef WITH_DEBUG
#define DEBUG(fmt, args...)	fprintf(stderr, "DBG %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_KBD
#define DEBUG_KBD(fmt, args...) fprintf(stderr, "DBG (KBD) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_KBD(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_RDP5
#define DEBUG_RDP5(fmt, args...) fprintf(stderr, "DBG (RDP5) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_RDP5(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_CLIPBOARD
#define DEBUG_CLIPBOARD(fmt, args...) fprintf(stderr, "DBG (CLIBBOARD) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_CLIPBOARD(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_SOUND
#define DEBUG_SOUND(fmt, args...) fprintf(stderr, "DBG (SOUND) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_SOUND(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_SERIAL
#define DEBUG_SERIAL(fmt, args...) fprintf(stderr, "DBG (SERIAL) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_SERIAL(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_CHANNEL
#define DEBUG_CHANNEL(fmt, args...) fprintf(stderr, "DBG (CHANNEL) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_CHANNEL(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_SCARD
#define DEBUG_SCARD(fmt, args...) fprintf(stderr, "DBG (SCARD) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_SCARD(fmt, args...) do { } while (0)
#endif

#define STRNCPY(dst,src,n)	{ strncpy(dst,src,n-1); dst[n-1] = 0; }

#ifndef MIN
#define MIN(x,y)		(((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y)		(((x) > (y)) ? (x) : (y))
#endif

/* timeval macros */
#ifndef timerisset
#define timerisset(tvp)\
         ((tvp)->tv_sec || (tvp)->tv_usec)
#endif
#ifndef timercmp
#define timercmp(tvp, uvp, cmp)\
        ((tvp)->tv_sec cmp (uvp)->tv_sec ||\
        (tvp)->tv_sec == (uvp)->tv_sec &&\
        (tvp)->tv_usec cmp (uvp)->tv_usec)
#endif
#ifndef timerclear
#define timerclear(tvp)\
        ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#endif

/* If configure does not define the endianess, try
   to find it out */
#if !defined(L_ENDIAN) && !defined(B_ENDIAN)
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define L_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define B_ENDIAN
#else
#error Unknown endianness. Edit rdesktop.h.
#endif
#endif /* B_ENDIAN, L_ENDIAN from configure */

/* No need for alignment on x86 and amd64 */
#if !defined(NEED_ALIGN)
#if !(defined(__x86__) || defined(__x86_64__) || \
      defined(__AMD64__) || defined(_M_IX86) || \
      defined(__i386__))
#define NEED_ALIGN
#endif
#endif

#include "parse.h"
#include "constants.h"
#include "types.h"

#ifndef MAKE_PROTO
#include "proto.h"
#include "xproto.h"
#endif

#endif // __RDESKTOP_H


