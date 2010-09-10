/*
   Copyright (c) 2010 Vic Lee

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drdynvc_types.h"
#include "audin_main.h"

void *
wave_in_new(void)
{
	return NULL;
}

void
wave_in_free(void * device_data)
{
}

int
wave_in_format_supported(void * device_data, char * snd_format, int size)
{
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;
	int cbSize;
	int wFormatTag;

	wFormatTag = GET_UINT16(snd_format, 0);
	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	wBitsPerSample = GET_UINT16(snd_format, 14);
	cbSize = GET_UINT16(snd_format, 16);
	LLOGLN(10, ("wave_in_format_supported: size=%d wFormatTag=%d nChannels=%d nSamplesPerSec=%d wBitsPerSample=%d cbSize=%d",
		size, wFormatTag, nChannels, nSamplesPerSec, wBitsPerSample, cbSize));
	if (cbSize == 0 &&
		(nSamplesPerSec == 22050 || nSamplesPerSec == 44100) &&
		(wBitsPerSample == 8 || wBitsPerSample == 16) &&
		(nChannels == 1 || nChannels == 2) &&
		wFormatTag == 1) /* WAVE_FORMAT_PCM */
	{
		LLOGLN(0, ("wave_in_format_supported: ok."));
		return 1;
	}
	return 0;
}

