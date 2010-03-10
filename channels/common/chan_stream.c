
#include "chan_stream.h"

int
set_wstr(char* dst, int dstlen, char* src, int srclen)
{
	int i;
	int len = (srclen < dstlen) ? srclen : dstlen;

	for (i = 0; i < len; i++)
	{
		dst[2*i] = src[i];
	}
}
