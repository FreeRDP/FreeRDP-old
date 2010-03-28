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

/* Parser state */
struct stream
{
	unsigned char *p;	/* current position */
	unsigned char *end;	/* end of stream */
	unsigned char *data;	/* pointer to stream-related mem.h-managed data */
	unsigned int size;	/* size of allocated data */

	/* Saved positions for various layeres */
	unsigned char *iso_hdr;
	unsigned char *mcs_hdr;
	unsigned char *sec_hdr;
	unsigned char *rdp_hdr;
	unsigned char *channel_hdr;
};
typedef struct stream *STREAM;

/* Save current pos for one encapsulation layer and skip forward */
#define s_push_layer(s,h,n)	{ (s)->h = (s)->p; (s)->p += n; }
/* Restore pos for an encapsulation layer */
#define s_pop_layer(s,h)	(s)->p = (s)->h;
/* Mark that end of stream has been reached */
#define s_mark_end(s)		(s)->end = (s)->p;

/* True if end not reached
 * FIXME: should be used more! */
#define s_check(s)		((s)->p <= (s)->end)
/* True if n more in stream
 * FIXME: should be used! */
#define s_check_rem(s,n)	((s)->p + n <= (s)->end)
/* True if exactly at end */
#define s_check_end(s)		((s)->p == (s)->end)

#if defined(L_ENDIAN) && !defined(NEED_ALIGN)
/* Direct LE parsing */
/* Read uint16 from stream and assign to v */
#define in_uint16_le(s,v)	{ v = *(uint16 *)((s)->p); (s)->p += 2; }
/* Read uint32 from stream and assign to v */
#define in_uint32_le(s,v)	{ v = *(uint32 *)((s)->p); (s)->p += 4; }
/* Write uint16 in v to stream */
#define out_uint16_le(s,v)	{ *(uint16 *)((s)->p) = v; (s)->p += 2; }
/* Write uint32 in v to stream */
#define out_uint32_le(s,v)	{ *(uint32 *)((s)->p) = v; (s)->p += 4; }

#else
/* Byte-oriented LE parsing */
#define in_uint16_le(s,v)	{ v = *((s)->p++); v += *((s)->p++) << 8; }
#define in_uint32_le(s,v)	{ in_uint16_le(s,v) \
				v += *((s)->p++) << 16; v += *((s)->p++) << 24; }
#define out_uint16_le(s,v)	{ *((s)->p++) = (v) & 0xff; *((s)->p++) = ((v) >> 8) & 0xff; }
#define out_uint32_le(s,v)	{ out_uint16_le(s, (v) & 0xffff); out_uint16_le(s, ((v) >> 16) & 0xffff); }
#endif

#if defined(B_ENDIAN) && !defined(NEED_ALIGN)
/* Direct BE parsing */
#define in_uint16_be(s,v)	{ v = *(uint16 *)((s)->p); (s)->p += 2; }
#define in_uint32_be(s,v)	{ v = *(uint32 *)((s)->p); (s)->p += 4; }
#define out_uint16_be(s,v)	{ *(uint16 *)((s)->p) = v; (s)->p += 2; }
#define out_uint32_be(s,v)	{ *(uint32 *)((s)->p) = v; (s)->p += 4; }

#else
/* Byte-oriented BE parsing */
#define in_uint16_be(s,v)	{ v = *((s)->p++); next_be(s,v); }
#define in_uint32_be(s,v)	{ in_uint16_be(s,v); next_be(s,v); next_be(s,v); }
#define out_uint16_be(s,v)	{ *((s)->p++) = ((v) >> 8) & 0xff; *((s)->p++) = (v) & 0xff; }
#define out_uint32_be(s,v)	{ out_uint16_be(s, ((v) >> 16) & 0xffff); out_uint16_be(s, (v) & 0xffff); }
#endif

/* Read uint8 from stream and assign to v */
#define in_uint8(s,v)		v = *((s)->p++);
/* Let v point to data at current pos and skip n */
#define in_uint8p(s,v,n)	{ v = (s)->p; (s)->p += n; }
/* Copy n bytes from current pos to *v and skip n */
#define in_uint8a(s,v,n)	{ memcpy(v,(s)->p,n); (s)->p += n; }
/* Skip n bytes */
#define in_uint8s(s,n)		(s)->p += n;
/* Write uint8 in v to stream */
#define out_uint8(s,v)		*((s)->p++) = v;
/* Copy n bytes from *v to stream */
#define out_uint8p(s,v,n)	{ memcpy((s)->p,v,n); (s)->p += n; }
/* Copy n bytes from *v to stream */
#define out_uint8a(s,v,n)	out_uint8p(s,v,n);
/* Copy n bytes from *v to stream */
#define out_uint8s(s,n)		{ memset((s)->p,0,n); (s)->p += n; }

/* Read least LSByte to v, shifting old value */
#define next_be(s,v)		v = ((v) << 8) + *((s)->p++);

#endif // __STREAM_H
