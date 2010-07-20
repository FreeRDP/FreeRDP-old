/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_ICONV
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif
#endif
#include "chan_stream.h"

int
freerdp_set_wstr(char* dst, int dstlen, char* src, int srclen)
{
#ifdef HAVE_ICONV
	iconv_t cd;
	size_t avail;
	size_t in_size;

	cd = iconv_open("UTF-16LE", "UTF-8");
	if (cd == (iconv_t) - 1)
	{
		printf("set_wstr: iconv_open failed.\n");
		return 0;
	}
	in_size = (size_t)srclen;
	avail = (size_t)dstlen;
	iconv(cd, &src, &in_size, &dst, &avail);
	iconv_close(cd);
	return dstlen - (int)avail;
#else
	int avail;

	avail = dstlen;
	while (srclen > 0 && avail > 0)
	{
		*dst++ = *src++;
		*dst++ = '\0';
		srclen--;
		avail -= 2;
	}
	return dstlen - avail;
#endif
}

int
freerdp_get_wstr(char* dst, int dstlen, char* src, int srclen)
{
#ifdef HAVE_ICONV
	iconv_t cd;
	size_t avail;
	size_t in_size;

	cd = iconv_open("UTF-8", "UTF-16LE");
	if (cd == (iconv_t) - 1)
	{
		printf("set_wstr: iconv_open failed.\n");
		return 0;
	}
	in_size = (size_t)srclen;
	avail = (size_t)dstlen;
	iconv(cd, &src, &in_size, &dst, &avail);
	iconv_close(cd);
	return dstlen - (int)avail;
#else
	int avail;

	avail = dstlen;
	while (srclen > 0 && avail > 0)
	{
		*dst++ = *src++;
		src++;
		avail--;
		srclen -= 2;
	}
	return dstlen - avail;
#endif
}

