/*
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Device Manager

   Copyright 2010-2011 Vic Lee

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freerdp/types_ui.h>
#include "rdpsnd_dsp.h"

uint8 *
rdpsnd_dsp_resample(uint8 * src, int bytes_per_frame,
	uint32 srate, int sframes,
	uint32 rrate, int * prframes)
{
	uint8 * dst;
	uint8 * p;
	int rframes;
	int rsize;
	int i, j;
	int n1, n2;

	rframes = sframes * rrate / srate;
	*prframes = rframes;
	rsize = bytes_per_frame * rframes;
	dst = (uint8 *) malloc(rsize);
	memset(dst, 0, rsize);

	p = dst;
	for (i = 0; i < rframes; i++)
	{
		n1 = i * srate / rrate;
		if (n1 >= sframes)
			n1 = sframes - 1;
		n2 = (n1 * rrate == i * srate || n1 == sframes - 1 ? n1 : n1 + 1);
		for (j = 0; j < bytes_per_frame; j++)
		{
			/* Nearest Interpolation, probably the easiest, but works */
			*p++ = (i * srate - n1 * rrate > n2 * rrate - i * srate ?
				src[n2 * bytes_per_frame + j] :
				src[n1 * bytes_per_frame + j]);
		}
	}

	return dst;
}

