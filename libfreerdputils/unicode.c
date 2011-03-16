/*
   FreeRDP: A Remote Desktop Protocol client.
   Unicode Utils

   Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#include <freerdp/utils.h>

/* Convert pin/in_len from WINDOWS_CODEPAGE - return like xstrdup, 0-terminated */

char* freerdp_uniconv_in(UNICONV *uniconv, unsigned char* pin, size_t in_len)
{
	unsigned char *conv_pin = pin;
	size_t conv_in_len = in_len;
	char *pout = xmalloc(in_len + 1), *conv_pout = pout;
	size_t conv_out_len = in_len;

#ifdef HAVE_ICONV
	if (iconv(uniconv->in_iconv_h, (ICONV_CONST char **) &conv_pin, &conv_in_len, &conv_pout, &conv_out_len) == (size_t) - 1)
	{
		/* TODO: xrealloc if conv_out_len == 0 - it shouldn't be needed, but would allow a smaller initial alloc ... */
		//printf("freerdp_in_unicode: iconv failure, errno %d\n", errno);
		return 0;
	}
#else
	while (conv_in_len >= 2)
	{
		if ((signed char)(*conv_pin) < 0)
		{
			//printf("xstrdup_in_unistr: wrong input conversion of char %d\n", *conv_pin);
		}
		*conv_pout++ = *conv_pin++;
		if ((*conv_pin) != 0)
		{
			//printf("xstrdup_in_unistr: wrong input conversion skipping non-zero char %d\n", *conv_pin);
		}
		conv_pin++;
		conv_in_len -= 2;
		conv_out_len--;
	}
#endif

	if (conv_in_len > 0)
	{
		//printf("xstrdup_in_unistr: conversion failure - %d chars left\n", conv_in_len);
	}

	*conv_pout = 0;
	return pout;
}

/* Convert str from DEFAULT_CODEPAGE to WINDOWS_CODEPAGE and return buffer like xstrdup.
 * Buffer is 0-terminated but that is not included in the returned length. */

char* freerdp_uniconv_out(UNICONV *uniconv, char *str, size_t *pout_len)
{
	size_t ibl = strlen(str), obl = 2 * ibl; /* FIXME: worst case */
	char *pin = str, *pout0 = xmalloc(obl + 2), *pout = pout0;

#ifdef HAVE_ICONV
	if (iconv(uniconv->out_iconv_h, (ICONV_CONST char **) &pin, &ibl, &pout, &obl) == (size_t) - 1)
	{
		return NULL;
	}
#else
	while ((ibl > 0) && (obl > 0))
	{
		if ((signed char)(*pin) < 0)
		{
			return NULL;
		}
		*pout++ = *pin++;
		*pout++ = 0;
		ibl--;
		obl -= 2;
	}
#endif

	if (ibl > 0)
	{
		//printf("xmalloc_out_unistr: string not fully converted - %d chars left\n", ibl);
	}

	*pout_len = pout - pout0;
	*pout++ = 0;	/* Add extra double zero termination */
	*pout = 0;
	return pout0;
}

UNICONV* freerdp_uniconv_new()
{
	UNICONV *uniconv = xmalloc(sizeof(UNICONV));
	memset(uniconv, '\0', sizeof(UNICONV));

#ifdef HAVE_ICONV
	uniconv->iconv = 1;
	uniconv->in_iconv_h = iconv_open(DEFAULT_CODEPAGE, WINDOWS_CODEPAGE);
	if (errno == EINVAL)
	{
		//DEBUG("Error opening iconv converter to %s from %s\n", DEFAULT_CODEPAGE, WINDOWS_CODEPAGE);
	}
	uniconv->out_iconv_h = iconv_open(WINDOWS_CODEPAGE, DEFAULT_CODEPAGE);
	if (errno == EINVAL)
	{
		//DEBUG("Error opening iconv converter to %s from %s\n", WINDOWS_CODEPAGE, DEFAULT_CODEPAGE);
	}
#endif

	return uniconv;
}

void freerdp_uniconv_free(UNICONV *uniconv)
{
	if (uniconv != NULL)
	{
#ifdef HAVE_ICONV
		iconv_close(rdp->in_iconv_h);
		iconv_close(rdp->out_iconv_h);
#endif
		xfree(uniconv);
	}
}
