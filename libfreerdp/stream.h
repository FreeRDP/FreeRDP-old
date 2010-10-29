/*
   rdesktop: A Remote Desktop Protocol client.
   Stream parsing primitives
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

#ifndef __STREAM_H
#define __STREAM_H

#if !(defined(L_ENDIAN) || defined(B_ENDIAN))
#warning no endian defined
#endif

#include "debug.h"

/* Parser state */
struct stream
{
	unsigned char *p;	/* current position */
	unsigned char *end;	/* end of stream, < data+size, no read or write beyond this */
	unsigned char *data;	/* pointer to stream-related mem.h-managed data */
	unsigned int size;	/* size of allocated data */

	/* Saved positions for various layers */
	unsigned char *iso_hdr;
	unsigned char *mcs_hdr;
	unsigned char *sec_hdr;
	unsigned char *rdp_hdr;
	unsigned char *channel_hdr;
};
typedef struct stream *STREAM;

/* Save current pos for one encapsulation layer and skip forward */
#define s_push_layer(s,h,n)	do { (s)->h = (s)->p; (s)->p += n; } while (0)
/* Restore pos for an encapsulation layer */
#define s_pop_layer(s,h)	(s)->p = (s)->h
/* Mark that end of stream has been reached */
#define s_mark_end(s)		(s)->end = (s)->p

/* True if end not reached
 * FIXME: should be used more! */
#define s_check(s)		((s)->p <= (s)->end)
/* True if n more in stream */
#define s_check_rem(s,n)	((s)->p + n <= (s)->end)
/* True if exactly at end */
#define s_check_end(s)		((s)->p == (s)->end)

#ifndef DISABLE_STREAM_CHECK
/* Check all stream access to prevent buffer overruns. */
#define ASSERT_AVAILABLE(s,n) ASSERT(s_check_rem(s,n))
#else
#define ASSERT_AVAILABLE(s,n) do { } while (0)
#endif

#if defined(L_ENDIAN) && !defined(NEED_ALIGN)
/* Direct LE parsing */
/* Read uint16 from stream and assign to v */
#define in_uint16_le(s,v)	do { ASSERT_AVAILABLE(s,2); v = *(uint16 *)((s)->p); (s)->p += 2; } while (0)
/* Read uint32 from stream and assign to v */
#define in_uint32_le(s,v)	do { ASSERT_AVAILABLE(s,4); v = *(uint32 *)((s)->p); (s)->p += 4; } while (0)
/* Write uint16 in v to stream */
#define out_uint16_le(s,v)	do { ASSERT_AVAILABLE(s,2); *(uint16 *)((s)->p) = v; (s)->p += 2; } while (0)
/* Write uint32 in v to stream */
#define out_uint32_le(s,v)	do { ASSERT_AVAILABLE(s,4); *(uint32 *)((s)->p) = v; (s)->p += 4; } while (0)

#else
/* Byte-oriented LE parsing */
#define in_uint16_le(s,v)	do { ASSERT_AVAILABLE(s,2); v = *((s)->p++); v += *((s)->p++) << 8; } while (0)
#define in_uint32_le(s,v)	do { ASSERT_AVAILABLE(s,4); in_uint16_le(s,v); \
				v += *((s)->p++) << 16; v += *((s)->p++) << 24; } while (0)
#define out_uint16_le(s,v)	do { ASSERT_AVAILABLE(s,2); *((s)->p++) = (v) & 0xff; *((s)->p++) = ((v) >> 8) & 0xff; } while (0)
#define out_uint32_le(s,v)	do { ASSERT_AVAILABLE(s,4); out_uint16_le(s, (v) & 0xffff); out_uint16_le(s, ((v) >> 16) & 0xffff); } while (0)
#endif

#if defined(B_ENDIAN) && !defined(NEED_ALIGN)
/* Direct BE parsing */
#define in_uint16_be(s,v)	do { ASSERT_AVAILABLE(s,2); v = *(uint16 *)((s)->p); (s)->p += 2; } while (0)
#define in_uint32_be(s,v)	do { ASSERT_AVAILABLE(s,4); v = *(uint32 *)((s)->p); (s)->p += 4; } while (0)
#define out_uint16_be(s,v)	do { ASSERT_AVAILABLE(s,2); *(uint16 *)((s)->p) = v; (s)->p += 2; } while (0)
#define out_uint32_be(s,v)	do { ASSERT_AVAILABLE(s,4); *(uint32 *)((s)->p) = v; (s)->p += 4; } while (0)

#else
/* Byte-oriented BE parsing */
#define in_uint16_be(s,v)	do { ASSERT_AVAILABLE(s,2); v = *((s)->p++); next_be(s,v); } while (0)
#define in_uint32_be(s,v)	do { ASSERT_AVAILABLE(s,4); in_uint16_be(s,v); next_be(s,v); next_be(s,v); } while (0)
#define out_uint16_be(s,v)	do { ASSERT_AVAILABLE(s,2); *((s)->p++) = ((v) >> 8) & 0xff; *((s)->p++) = (v) & 0xff; } while (0)
#define out_uint32_be(s,v)	do { ASSERT_AVAILABLE(s,4); out_uint16_be(s, ((v) >> 16) & 0xffff); out_uint16_be(s, (v) & 0xffff); } while (0)
#endif

/* Read uint8 from stream and assign to v */
#define in_uint8(s,v)		do { ASSERT_AVAILABLE(s,1); v = *((s)->p++); } while (0)
/* Let v point to data at current pos and skip n */
#define in_uint8p(s,v,n)	do { ASSERT_AVAILABLE(s,n); v = (s)->p; (s)->p += n; } while (0)
/* Copy n bytes from current pos to *v and skip n */
#define in_uint8a(s,v,n)	do { ASSERT_AVAILABLE(s,n); memcpy(v,(s)->p,n); (s)->p += n; } while (0)
/* Skip n bytes */
#define in_uint8s(s,n)		do { ASSERT_AVAILABLE(s,n); (s)->p += n; } while (0)
/* Write uint8 in v to stream */
#define out_uint8(s,v)		do { ASSERT_AVAILABLE(s,1); *((s)->p++) = v; } while (0)
/* Copy n bytes from *v to stream */
#define out_uint8p(s,v,n)	do { ASSERT_AVAILABLE(s,n); memcpy((s)->p,v,n); (s)->p += n; } while (0)
/* Copy n bytes from *v to stream */
#define out_uint8a(s,v,n)	do { ASSERT_AVAILABLE(s,n); out_uint8p(s,v,n); } while (0)
/* Output n bytes zero to stream */
#define out_uint8s(s,n)		do { ASSERT_AVAILABLE(s,n); memset((s)->p,0,n); (s)->p += n; } while (0)

/* Shift old v value and read new LSByte */
#define next_be(s,v)		do { ASSERT_AVAILABLE(s,1); v = ((v) << 8) + *((s)->p++); } while (0)

#endif /* __STREAM_H */
