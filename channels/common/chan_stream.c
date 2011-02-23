/*
   FreeRDP: A Remote Desktop Protocol client.
   Channel stream parsing utils

   Copyright 2010 Marc-Andre Moreau <marcandre.moreau@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
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

