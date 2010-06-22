/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Secure sockets abstraction layer
   Copyright (C) Matthew Chapman 1999-2008
   Copyright (C) Jay Sorg 2006-2010

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

#include "frdp.h"
#include "mem.h"
#include "ssl.h"

/*****************************************************************************/
static void *
l_malloc(int size, RD_BOOL zero)
{
	void * p;

	p = xmalloc(size);
	if (zero)
	{
		if (p != NULL)
		{
			memset(p, 0, size);
		}
	}
	return p;
}

/*****************************************************************************/
static void
l_free(void * in)
{
	if (in != NULL)
	{
		xfree(in);
	}
}

/*****************************************************************************/

/*****************************************************************************/
/* sha1 stuff */
/* FIPS-180-1 compliant SHA-1 implementation
 *
 * Copyright (C) 2001-2003  Christophe Devine
 */

/*****************************************************************************/
void
ssl_sha1_init(SSL_SHA1 * sha1)
{
	memset(sha1, 0, sizeof(SSL_SHA1));
	sha1->state[0] = 0x67452301;
	sha1->state[1] = 0xEFCDAB89;
	sha1->state[2] = 0x98BADCFE;
	sha1->state[3] = 0x10325476;
	sha1->state[4] = 0xC3D2E1F0;
}

#undef GET_UINT32
#define GET_UINT32(n, b, i)          \
{                                    \
  (n) = ((b)[(i) + 0] << 24) |       \
        ((b)[(i) + 1] << 16) |       \
        ((b)[(i) + 2] << 8) |        \
        ((b)[(i) + 3] << 0);         \
}

#undef PUT_UINT32
#define PUT_UINT32(n, b, i)         \
{                                   \
  (b)[(i) + 0] = ((n) >> 24);       \
  (b)[(i) + 1] = ((n) >> 16);       \
  (b)[(i) + 2] = ((n) >> 8);        \
  (b)[(i) + 3] = ((n) >> 0);        \
}

/*****************************************************************************/
static void
sha1_process(struct sha1_context * ctx, char * in_data)
{
	int temp;
	int W[16];
	int A;
	int B;
	int C;
	int D;
	int E;
	unsigned char * data;

	data = (unsigned char *) in_data;

	GET_UINT32(W[0], data, 0);
	GET_UINT32(W[1], data, 4);
	GET_UINT32(W[2], data, 8);
	GET_UINT32(W[3], data, 12);
	GET_UINT32(W[4], data, 16);
	GET_UINT32(W[5], data, 20);
	GET_UINT32(W[6], data, 24);
	GET_UINT32(W[7], data, 28);
	GET_UINT32(W[8], data, 32);
	GET_UINT32(W[9], data, 36);
	GET_UINT32(W[10], data, 40);
	GET_UINT32(W[11], data, 44);
	GET_UINT32(W[12], data, 48);
	GET_UINT32(W[13], data, 52);
	GET_UINT32(W[14], data, 56);
	GET_UINT32(W[15], data, 60);

#define S(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define R(t)                        \
(                                   \
  temp = W[(t - 3) & 0x0F] ^        \
         W[(t - 8) & 0x0F] ^        \
         W[(t - 14) & 0x0F] ^       \
         W[(t - 0) & 0x0F],         \
         (W[t & 0x0F] = S(temp, 1)) \
)

#undef P
#define P(a, b, c, d, e, x)          \
{                                    \
  e += S(a, 5) + F(b, c, d) + K + x; \
  b = S(b, 30);                      \
}

	A = ctx->state[0];
	B = ctx->state[1];
	C = ctx->state[2];
	D = ctx->state[3];
	E = ctx->state[4];

#define F(x, y, z) (z ^ (x & (y ^ z)))
#define K 0x5A827999

	P(A, B, C, D, E, W[0]);
	P(E, A, B, C, D, W[1]);
	P(D, E, A, B, C, W[2]);
	P(C, D, E, A, B, W[3]);
	P(B, C, D, E, A, W[4]);
	P(A, B, C, D, E, W[5]);
	P(E, A, B, C, D, W[6]);
	P(D, E, A, B, C, W[7]);
	P(C, D, E, A, B, W[8]);
	P(B, C, D, E, A, W[9]);
	P(A, B, C, D, E, W[10]);
	P(E, A, B, C, D, W[11]);
	P(D, E, A, B, C, W[12]);
	P(C, D, E, A, B, W[13]);
	P(B, C, D, E, A, W[14]);
	P(A, B, C, D, E, W[15]);
	P(E, A, B, C, D, R(16));
	P(D, E, A, B, C, R(17));
	P(C, D, E, A, B, R(18));
	P(B, C, D, E, A, R(19));

#undef K
#undef F

#define F(x, y, z) (x ^ y ^ z)
#define K 0x6ED9EBA1

	P(A, B, C, D, E, R(20));
	P(E, A, B, C, D, R(21));
	P(D, E, A, B, C, R(22));
	P(C, D, E, A, B, R(23));
	P(B, C, D, E, A, R(24));
	P(A, B, C, D, E, R(25));
	P(E, A, B, C, D, R(26));
	P(D, E, A, B, C, R(27));
	P(C, D, E, A, B, R(28));
	P(B, C, D, E, A, R(29));
	P(A, B, C, D, E, R(30));
	P(E, A, B, C, D, R(31));
	P(D, E, A, B, C, R(32));
	P(C, D, E, A, B, R(33));
	P(B, C, D, E, A, R(34));
	P(A, B, C, D, E, R(35));
	P(E, A, B, C, D, R(36));
	P(D, E, A, B, C, R(37));
	P(C, D, E, A, B, R(38));
	P(B, C, D, E, A, R(39));

#undef K
#undef F

#define F(x, y, z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC

	P(A, B, C, D, E, R(40));
	P(E, A, B, C, D, R(41));
	P(D, E, A, B, C, R(42));
	P(C, D, E, A, B, R(43));
	P(B, C, D, E, A, R(44));
	P(A, B, C, D, E, R(45));
	P(E, A, B, C, D, R(46));
	P(D, E, A, B, C, R(47));
	P(C, D, E, A, B, R(48));
	P(B, C, D, E, A, R(49));
	P(A, B, C, D, E, R(50));
	P(E, A, B, C, D, R(51));
	P(D, E, A, B, C, R(52));
	P(C, D, E, A, B, R(53));
	P(B, C, D, E, A, R(54));
	P(A, B, C, D, E, R(55));
	P(E, A, B, C, D, R(56));
	P(D, E, A, B, C, R(57));
	P(C, D, E, A, B, R(58));
	P(B, C, D, E, A, R(59));

#undef K
#undef F

#define F(x, y, z) (x ^ y ^ z)
#define K 0xCA62C1D6

	P(A, B, C, D, E, R(60));
	P(E, A, B, C, D, R(61));
	P(D, E, A, B, C, R(62));
	P(C, D, E, A, B, R(63));
	P(B, C, D, E, A, R(64));
	P(A, B, C, D, E, R(65));
	P(E, A, B, C, D, R(66));
	P(D, E, A, B, C, R(67));
	P(C, D, E, A, B, R(68));
	P(B, C, D, E, A, R(69));
	P(A, B, C, D, E, R(70));
	P(E, A, B, C, D, R(71));
	P(D, E, A, B, C, R(72));
	P(C, D, E, A, B, R(73));
	P(B, C, D, E, A, R(74));
	P(A, B, C, D, E, R(75));
	P(E, A, B, C, D, R(76));
	P(D, E, A, B, C, R(77));
	P(C, D, E, A, B, R(78));
	P(B, C, D, E, A, R(79));

#undef K
#undef F

	ctx->state[0] += A;
	ctx->state[1] += B;
	ctx->state[2] += C;
	ctx->state[3] += D;
	ctx->state[4] += E;
}

/*****************************************************************************/
void
ssl_sha1_update(SSL_SHA1 * sha1, uint8 * data, uint32 len)
{
	int left;
	int fill;

	if (len == 0)
	{
		return;
	}
	left = sha1->total[0] & 0x3F;
	fill = 64 - left;
	sha1->total[0] += len;
	sha1->total[0] &= 0xFFFFFFFF;
	if (sha1->total[0] < (int) len)
	{
		sha1->total[1]++;
	}
	if (left && ((int) len >= fill))
	{
		memcpy(sha1->buffer + left, data, fill);
		sha1_process(sha1, sha1->buffer);
		len -= fill;
		data += fill;
		left = 0;
	}
	while (len >= 64)
	{
		sha1_process(sha1, (char *) data);
		len -= 64;
		data += 64;
	}
	if (len != 0)
	{
		memcpy(sha1->buffer + left, data, len);
	}
}

static unsigned char sha1_padding[64] =
{
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*****************************************************************************/
void
ssl_sha1_final(SSL_SHA1 * sha1, uint8 * out_data)
{
	int last;
	int padn;
	int high;
	int low;
	char msglen[8];

	high = (sha1->total[0] >> 29) | (sha1->total[1] << 3);
	low = (sha1->total[0] << 3);
	PUT_UINT32(high, msglen, 0);
	PUT_UINT32(low, msglen, 4);
	last = sha1->total[0] & 0x3F;
	padn = (last < 56) ? (56 - last) : (120 - last);
	ssl_sha1_update(sha1, sha1_padding, padn);
	ssl_sha1_update(sha1, (uint8*)msglen, 8);
	PUT_UINT32(sha1->state[0], out_data, 0);
	PUT_UINT32(sha1->state[1], out_data, 4);
	PUT_UINT32(sha1->state[2], out_data, 8);
	PUT_UINT32(sha1->state[3], out_data, 12);
	PUT_UINT32(sha1->state[4], out_data, 16);
}

/*****************************************************************************/

/*****************************************************************************/
/* md5 stuff */
/* RFC 1321 compliant MD5 implementation
 *
 * Copyright (C) 2001-2003  Christophe Devine
 */

/*****************************************************************************/
void
ssl_md5_init(SSL_MD5 * md5)
{
	memset(md5, 0, sizeof(SSL_MD5));
	md5->state[0] = 0x67452301;
	md5->state[1] = 0xEFCDAB89;
	md5->state[2] = 0x98BADCFE;
	md5->state[3] = 0x10325476;
}

#undef GET_UINT32
#define GET_UINT32(n, b, i)          \
{                                    \
  (n) = ((b)[(i) + 0] << 0) |        \
        ((b)[(i) + 1] << 8) |        \
        ((b)[(i) + 2] << 16) |       \
        ((b)[(i) + 3] << 24);        \
}

#undef PUT_UINT32
#define PUT_UINT32(n, b, i)          \
{                                    \
  (b)[(i) + 0] = ((n) >> 0);         \
  (b)[(i) + 1] = ((n) >> 8);         \
  (b)[(i) + 2] = ((n) >> 16);        \
  (b)[(i) + 3] = ((n) >> 24);        \
}

/*****************************************************************************/
static void
md5_process(struct md5_context* ctx, char* in_data)
{
	int X[16];
	int A;
	int B;
	int C;
	int D;
	unsigned char* data;

	data = (unsigned char *) in_data;
	GET_UINT32(X[0], data, 0);
	GET_UINT32(X[1], data, 4);
	GET_UINT32(X[2], data, 8);
	GET_UINT32(X[3], data, 12);
	GET_UINT32(X[4], data, 16);
	GET_UINT32(X[5], data, 20);
	GET_UINT32(X[6], data, 24);
	GET_UINT32(X[7], data, 28);
	GET_UINT32(X[8], data, 32);
	GET_UINT32(X[9], data, 36);
	GET_UINT32(X[10], data, 40);
	GET_UINT32(X[11], data, 44);
	GET_UINT32(X[12], data, 48);
	GET_UINT32(X[13], data, 52);
	GET_UINT32(X[14], data, 56);
	GET_UINT32(X[15], data, 60);

#define S(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#undef P
#define P(a, b, c, d, k, s, t) \
{                              \
  a += F(b, c, d) + X[k] + t;  \
  a = S(a, s) + b;             \
}

	A = ctx->state[0];
	B = ctx->state[1];
	C = ctx->state[2];
	D = ctx->state[3];

#define F(x, y, z) (z ^ (x & (y ^ z)))

	P(A, B, C, D,  0,  7, 0xD76AA478);
	P(D, A, B, C,  1, 12, 0xE8C7B756);
	P(C, D, A, B,  2, 17, 0x242070DB);
	P(B, C, D, A,  3, 22, 0xC1BDCEEE);
	P(A, B, C, D,  4,  7, 0xF57C0FAF);
	P(D, A, B, C,  5, 12, 0x4787C62A);
	P(C, D, A, B,  6, 17, 0xA8304613);
	P(B, C, D, A,  7, 22, 0xFD469501);
	P(A, B, C, D,  8,  7, 0x698098D8);
	P(D, A, B, C,  9, 12, 0x8B44F7AF);
	P(C, D, A, B, 10, 17, 0xFFFF5BB1);
	P(B, C, D, A, 11, 22, 0x895CD7BE);
	P(A, B, C, D, 12,  7, 0x6B901122);
	P(D, A, B, C, 13, 12, 0xFD987193);
	P(C, D, A, B, 14, 17, 0xA679438E);
	P(B, C, D, A, 15, 22, 0x49B40821);

#undef F

#define F(x, y, z) (y ^ (z & (x ^ y)))

	P(A, B, C, D,  1,  5, 0xF61E2562);
	P(D, A, B, C,  6,  9, 0xC040B340);
	P(C, D, A, B, 11, 14, 0x265E5A51);
	P(B, C, D, A,  0, 20, 0xE9B6C7AA);
	P(A, B, C, D,  5,  5, 0xD62F105D);
	P(D, A, B, C, 10,  9, 0x02441453);
	P(C, D, A, B, 15, 14, 0xD8A1E681);
	P(B, C, D, A,  4, 20, 0xE7D3FBC8);
	P(A, B, C, D,  9,  5, 0x21E1CDE6);
	P(D, A, B, C, 14,  9, 0xC33707D6);
	P(C, D, A, B,  3, 14, 0xF4D50D87);
	P(B, C, D, A,  8, 20, 0x455A14ED);
	P(A, B, C, D, 13,  5, 0xA9E3E905);
	P(D, A, B, C,  2,  9, 0xFCEFA3F8);
	P(C, D, A, B,  7, 14, 0x676F02D9);
	P(B, C, D, A, 12, 20, 0x8D2A4C8A);

#undef F

#define F(x, y, z) (x ^ y ^ z)

	P(A, B, C, D,  5,  4, 0xFFFA3942);
	P(D, A, B, C,  8, 11, 0x8771F681);
	P(C, D, A, B, 11, 16, 0x6D9D6122);
	P(B, C, D, A, 14, 23, 0xFDE5380C);
	P(A, B, C, D,  1,  4, 0xA4BEEA44);
	P(D, A, B, C,  4, 11, 0x4BDECFA9);
	P(C, D, A, B,  7, 16, 0xF6BB4B60);
	P(B, C, D, A, 10, 23, 0xBEBFBC70);
	P(A, B, C, D, 13,  4, 0x289B7EC6);
	P(D, A, B, C,  0, 11, 0xEAA127FA);
	P(C, D, A, B,  3, 16, 0xD4EF3085);
	P(B, C, D, A,  6, 23, 0x04881D05);
	P(A, B, C, D,  9,  4, 0xD9D4D039);
	P(D, A, B, C, 12, 11, 0xE6DB99E5);
	P(C, D, A, B, 15, 16, 0x1FA27CF8);
	P(B, C, D, A,  2, 23, 0xC4AC5665);

#undef F

#define F(x, y, z) (y ^ (x | ~z))

	P(A, B, C, D,  0,  6, 0xF4292244);
	P(D, A, B, C,  7, 10, 0x432AFF97);
	P(C, D, A, B, 14, 15, 0xAB9423A7);
	P(B, C, D, A,  5, 21, 0xFC93A039);
	P(A, B, C, D, 12,  6, 0x655B59C3);
	P(D, A, B, C,  3, 10, 0x8F0CCC92);
	P(C, D, A, B, 10, 15, 0xFFEFF47D);
	P(B, C, D, A,  1, 21, 0x85845DD1);
	P(A, B, C, D,  8,  6, 0x6FA87E4F);
	P(D, A, B, C, 15, 10, 0xFE2CE6E0);
	P(C, D, A, B,  6, 15, 0xA3014314);
	P(B, C, D, A, 13, 21, 0x4E0811A1);
	P(A, B, C, D,  4,  6, 0xF7537E82);
	P(D, A, B, C, 11, 10, 0xBD3AF235);
	P(C, D, A, B,  2, 15, 0x2AD7D2BB);
	P(B, C, D, A,  9, 21, 0xEB86D391);

#undef F

	ctx->state[0] += A;
	ctx->state[1] += B;
	ctx->state[2] += C;
	ctx->state[3] += D;
}

/*****************************************************************************/
void
ssl_md5_update(SSL_MD5 * md5, uint8 * data, uint32 len)
{
	int left;
	int fill;

	if (len == 0)
	{
		return;
	}
	left = md5->total[0] & 0x3F;
	fill = 64 - left;
	md5->total[0] += len;
	md5->total[0] &= 0xFFFFFFFF;
	if (md5->total[0] < (int) len)
	{
		md5->total[1]++;
	}
	if (left && ((int) len >= fill))
	{
		memcpy(md5->buffer + left, data, fill);
		md5_process(md5, md5->buffer);
		len -= fill;
		data += fill;
		left = 0;
	}
	while (len >= 64)
	{
		md5_process(md5, (char *) data);
		len -= 64;
		data += 64;
	}
	if (len != 0)
	{
		memcpy(md5->buffer + left, data, len);
	}
}

static unsigned char md5_padding[64] =
{
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*****************************************************************************/
void
ssl_md5_final(SSL_MD5 * md5, uint8 * out_data)
{
	int last;
	int padn;
	int high;
	int low;
	char msglen[8];

	high = (md5->total[0] >> 29) | (md5->total[1] << 3);
	low = (md5->total[0] << 3);
	PUT_UINT32(low, msglen, 0);
	PUT_UINT32(high, msglen, 4);
	last = md5->total[0] & 0x3F;
	padn = (last < 56) ? (56 - last) : (120 - last);
	ssl_md5_update(md5, md5_padding, padn);
	ssl_md5_update(md5, (uint8*)msglen, 8);
	PUT_UINT32(md5->state[0], out_data, 0);
	PUT_UINT32(md5->state[1], out_data, 4);
	PUT_UINT32(md5->state[2], out_data, 8);
	PUT_UINT32(md5->state[3], out_data, 12);
}

/*****************************************************************************/

/*****************************************************************************/
/* rc4 stuff */
/* An implementation of the ARC4 algorithm
 *
 * Copyright (C) 2001-2003  Christophe Devine
 */

/*****************************************************************************/
void
ssl_rc4_set_key(SSL_RC4 * rc4, uint8 * key, uint32 len)
{
	int i;
	int j;
	int k;
	int a;
	int * m;
	struct rc4_state * s;

	s = rc4;
	s->x = 0;
	s->y = 0;
	m = s->m;
	for (i = 0; i < 256; i++)
	{
		m[i] = i;
	}
	j = 0;
	k = 0;
	for (i = 0; i < 256; i++)
	{
		a = m[i];
		j = (unsigned char) (j + a + key[k]);
		m[i] = m[j];
		m[j] = a;
		k++;
		if (k >= (int) len)
		{
			k = 0;
		}
	}
}

/*****************************************************************************/
void
ssl_rc4_crypt(SSL_RC4 * rc4, uint8 * in_data, uint8 * out_data, uint32 len)
{
	int i;
	int x;
	int y;
	int a;
	int b;
	int * m;
	struct rc4_state * s;

	s = rc4;
	x = s->x;
	y = s->y;
	m = s->m;
	for (i = 0; i < (int)len; i++)
	{
		x = (unsigned char) (x + 1);
		a = m[x];
		y = (unsigned char) (y + a);
		b = m[y];
		m[x] = b;
		m[y] = a;
		out_data[i] = in_data[i] ^ (m[(unsigned char) (a + b)]);
	}
	s->x = x;
	s->y = y;
}

/*****************************************************************************/

/*
  http://spinning-yarns.org/michael/mpi/
  only enough of library to do modexp
  pulled from mpi.c, mpi.h, mpi-conifg.h, and mpi-types.h
  jay.sorg@gmail.com

  This software was written by Michael J. Fromberger,
    http://www.dartmouth.edu/~sting/

  See the MPI home page at
    http://www.dartmouth.edu/~sting/mpi/

  This software is in the public domain.  It is entirely free, and you
  may use it and/or redistribute it for whatever purpose you choose;
  however, as free software, it is provided without warranty of any
  kind, not even the implied warranty of merchantability or fitness for
  a particular purpose.

  Arbitrary precision integer arithmetic library
*/

#include <limits.h>

typedef char               mp_sign;
typedef unsigned short     mp_digit;  /* 2 byte type */
typedef unsigned int       mp_word;   /* 4 byte type */
typedef unsigned int       mp_size;
typedef int                mp_err;

#define MP_DIGIT_BIT       (CHAR_BIT * sizeof(mp_digit))
#define MP_DIGIT_MAX       USHRT_MAX
#define MP_WORD_BIT        (CHAR_BIT * sizeof(mp_word))
#define MP_WORD_MAX        UINT_MAX

#define RADIX              (MP_DIGIT_MAX+1)

#define MP_DIGIT_SIZE      2
#define DIGIT_FMT          "%04X"

/*
  For boolean options,
  0 = no
  1 = yes

  Other options are documented individually.

 */

#ifndef MP_IOFUNC
#define MP_IOFUNC     0  /* include mp_print() ?                */
#endif

#ifndef MP_MODARITH
#define MP_MODARITH   1  /* include modular arithmetic ?        */
#endif

#ifndef MP_NUMTH
#define MP_NUMTH      1  /* include number theoretic functions? */
#endif

#ifndef MP_LOGTAB
#define MP_LOGTAB     1  /* use table of logs instead of log()? */
#endif

#ifndef MP_MEMSET
#define MP_MEMSET     1  /* use memset() to zero buffers?       */
#endif

#ifndef MP_MEMCPY
#define MP_MEMCPY     1  /* use memcpy() to copy buffers?       */
#endif

#ifndef MP_CRYPTO
#define MP_CRYPTO     1  /* erase memory on free?               */
#endif

#ifndef MP_ARGCHK
/*
  0 = no parameter checks
  1 = runtime checks, continue execution and return an error to caller
  2 = assertions; dump core on parameter errors
 */
#define MP_ARGCHK     1  /* how to check input arguments        */
#endif

#ifndef MP_DEBUG
#define MP_DEBUG      0  /* print diagnostic output?            */
#endif

#ifndef MP_DEFPREC
#define MP_DEFPREC    16 /* default precision, in digits        */
#endif

#ifndef MP_MACRO
#define MP_MACRO      1  /* use macros for frequent calls?      */
#endif

#ifndef MP_SQUARE
#define MP_SQUARE     1  /* use separate squaring code?         */
#endif

#ifndef MP_PTAB_SIZE
/*
  When building mpprime.c, we build in a table of small prime
  values to use for primality testing.  The more you include,
  the more space they take up.  See primes.c for the possible
  values (currently 16, 32, 64, 128, 256, and 6542)
 */
#define MP_PTAB_SIZE  128  /* how many built-in primes?         */
#endif

#ifndef MP_COMPAT_MACROS
#define MP_COMPAT_MACROS 0   /* define compatibility macros?    */
#endif

#define  MP_NEG  1
#define  MP_ZPOS 0

/* Included for compatibility... */
#define  NEG     MP_NEG
#define  ZPOS    MP_ZPOS

#define  MP_OKAY          0 /* no error, all is well */
#define  MP_YES           0 /* yes (boolean result)  */
#define  MP_NO           -1 /* no (boolean result)   */
#define  MP_MEM          -2 /* out of memory         */
#define  MP_RANGE        -3 /* argument out of range */
#define  MP_BADARG       -4 /* invalid parameter     */
#define  MP_UNDEF        -5 /* answer is undefined   */
#define  MP_LAST_CODE    MP_UNDEF

/* Included for compatibility... */
#define DIGIT_BIT         MP_DIGIT_BIT
#define DIGIT_MAX         MP_DIGIT_MAX

/* Macros for accessing the mp_int internals           */
#define  SIGN(MP)     ((MP)->sign)
#define  USED(MP)     ((MP)->used)
#define  ALLOC(MP)    ((MP)->alloc)
#define  DIGITS(MP)   ((MP)->dp)
#define  DIGIT(MP,N)  (MP)->dp[(N)]

#if MP_ARGCHK == 1
#define  ARGCHK(X,Y)  {if(!(X)){return (Y);}}
#elif MP_ARGCHK == 2
#include <assert.h>
#define  ARGCHK(X,Y)  assert(X)
#else
#define  ARGCHK(X,Y)  /*  */
#endif

/* This defines the maximum I/O base (minimum is 2)   */
#define MAX_RADIX         64

typedef struct _mp_int
{
  mp_sign       sign;    /* sign of this quantity      */
  mp_size       alloc;   /* how many digits allocated  */
  mp_size       used;    /* how many digits used       */
  mp_digit     *dp;      /* the digits themselves      */
} mp_int;

#define  s_mp_alloc(nb, ni)  l_malloc((nb) * (ni), 1)
#define  s_mp_free(ptr) {if(ptr) l_free(ptr);}
#define  s_mp_setz(dp, count) memset(dp, 0, (count) * sizeof(mp_digit))
#define  s_mp_copy(sp, dp, count) memcpy(dp, sp, (count) * sizeof(mp_digit))

/* Default precision for newly created mp_int's      */
static unsigned int s_mp_defprec = MP_DEFPREC;

#define  CARRYOUT(W)  ((W)>>DIGIT_BIT)
#define  ACCUM(W)     ((W)&MP_DIGIT_MAX)

#define  MP_LT       -1
#define  MP_EQ        0
#define  MP_GT        1

/*****************************************************************************/
/*
  mp_init_size(mp, prec)

  Initialize a new zero-valued mp_int with at least the given
  precision; returns MP_OKAY if successful, or MP_MEM if memory could
  not be allocated for the structure.
*/
static mp_err
mp_init_size(mp_int * mp, mp_size prec)
{
	ARGCHK(mp != NULL && prec > 0, MP_BADARG);
	if ((DIGITS(mp) = s_mp_alloc(prec, sizeof(mp_digit))) == NULL)
		return MP_MEM;
	SIGN(mp) = MP_ZPOS;
	USED(mp) = 1;
	ALLOC(mp) = prec;
	return MP_OKAY;
} /* end mp_init_size() */

/*****************************************************************************/
/*
  mp_init(mp)

  Initialize a new zero-valued mp_int.  Returns MP_OKAY if successful,
  MP_MEM if memory could not be allocated for the structure.
*/
static mp_err
mp_init(mp_int * mp)
{
	return mp_init_size(mp, s_mp_defprec);
} /* end mp_init() */

/*****************************************************************************/
/*
  mp_zero(mp)

  Set mp to zero.  Does not change the allocated size of the structure,
  and therefore cannot fail (except on a bad argument, which we ignore)
*/
static void
mp_zero(mp_int * mp)
{
	if (mp == NULL)
		return;
	s_mp_setz(DIGITS(mp), ALLOC(mp));
	USED(mp) = 1;
	SIGN(mp) = MP_ZPOS;
} /* end mp_zero() */

/*****************************************************************************/
/* Make sure there are at least 'min' digits allocated to mp              */
static mp_err
s_mp_grow(mp_int * mp, mp_size min)
{
	if (min > ALLOC(mp))
	{
		mp_digit * tmp;
		/* Set min to next nearest default precision block size */
		min = ((min + (s_mp_defprec - 1)) / s_mp_defprec) * s_mp_defprec;
		if ((tmp = s_mp_alloc(min, sizeof(mp_digit))) == NULL)
			return MP_MEM;
		s_mp_copy(DIGITS(mp), tmp, USED(mp));
#if MP_CRYPTO
		s_mp_setz(DIGITS(mp), ALLOC(mp));
#endif
		s_mp_free(DIGITS(mp));
		DIGITS(mp) = tmp;
		ALLOC(mp) = min;
	}
	return MP_OKAY;
} /* end s_mp_grow() */

/*****************************************************************************/
/* Make sure the used size of mp is at least 'min', growing if needed     */
static mp_err
s_mp_pad(mp_int * mp, mp_size min)
{
	if (min > USED(mp))
	{
		mp_err  res;
		/* Make sure there is room to increase precision  */
		if (min > ALLOC(mp) && (res = s_mp_grow(mp, min)) != MP_OKAY)
			return res;
		/* Increase precision; should already be 0-filled */
		USED(mp) = min;
	}
	return MP_OKAY;
} /* end s_mp_pad() */

/*****************************************************************************/
/*
   Shift mp leftward by p digits, growing if needed, and zero-filling
   the in-shifted digits at the right end.  This is a convenient
   alternative to multiplication by powers of the radix
*/
static mp_err
s_mp_lshd(mp_int * mp, mp_size p)
{
	mp_err res;
	mp_size pos;
	mp_digit * dp;
	int ix;

	if (p == 0)
		return MP_OKAY;
	if ((res = s_mp_pad(mp, USED(mp) + p)) != MP_OKAY)
		return res;
	pos = USED(mp) - 1;
	dp = DIGITS(mp);
	/* Shift all the significant figures over as needed */
	for (ix = pos - p; ix >= 0; ix--)
		dp[ix + p] = dp[ix];
	/* Fill the bottom digits with zeroes */
	for (ix = 0; ix < p; ix++)
		dp[ix] = 0;
	return MP_OKAY;
} /* end s_mp_lshd() */

/*****************************************************************************/
/* Remove leading zeroes from the given value                             */
static void
s_mp_clamp(mp_int * mp)
{
	mp_size du = USED(mp);
	mp_digit * zp = DIGITS(mp) + du - 1;

	while (du > 1 && !*zp--)
		--du;
	if (du == 1 && *zp == 0)
		SIGN(mp) = MP_ZPOS;
	USED(mp) = du;
} /* end s_mp_clamp() */

/*****************************************************************************/
/*
  Multiply by the integer 2^d, where d is a number of bits.  This
  amounts to a bitwise shift of the value, and does not require the
  full multiplication code.
*/
static mp_err
s_mp_mul_2d(mp_int * mp, mp_digit d)
{
	mp_err res;
	mp_digit save, next, mask, *dp;
	mp_size  used;
	int ix;

	if ((res = s_mp_lshd(mp, d / DIGIT_BIT)) != MP_OKAY)
		return res;
	dp = DIGITS(mp); used = USED(mp);
	d %= DIGIT_BIT;
	mask = (1 << d) - 1;
	/* If the shift requires another digit, make sure we've got one to
		work with */
	if ((dp[used - 1] >> (DIGIT_BIT - d)) & mask)
	{
		if ((res = s_mp_grow(mp, used + 1)) != MP_OKAY)
			return res;
		dp = DIGITS(mp);
	}
	/* Do the shifting... */
	save = 0;
	for (ix = 0; ix < used; ix++)
	{
		next = (dp[ix] >> (DIGIT_BIT - d)) & mask;
		dp[ix] = (dp[ix] << d) | save;
		save = next;
	}
	/* If, at this point, we have a nonzero carryout into the next
		digit, we'll increase the size by one digit, and store it...
	*/
	if (save)
	{
		dp[used] = save;
		USED(mp) += 1;
	}
	s_mp_clamp(mp);
	return MP_OKAY;
} /* end s_mp_mul_2d() */

/*****************************************************************************/
/*
  mp_copy(from, to)

  Copies the mp_int 'from' to the mp_int 'to'.  It is presumed that
  'to' has already been initialized (if not, use mp_init_copy()
  instead). If 'from' and 'to' are identical, nothing happens.
*/
static mp_err
mp_copy(mp_int * from, mp_int * to)
{
	ARGCHK(from != NULL && to != NULL, MP_BADARG);
	if (from == to)
		return MP_OKAY;
	do
	{ /* copy */
		mp_digit   *tmp;

		/*
			If the allocated buffer in 'to' already has enough space to hold
			all the used digits of 'from', we'll re-use it to avoid hitting
			the memory allocater more than necessary; otherwise, we'd have
			to grow anyway, so we just allocate a hunk and make the copy as
			usual
		*/
		if (ALLOC(to) >= USED(from))
		{
			s_mp_setz(DIGITS(to) + USED(from), ALLOC(to) - USED(from));
			s_mp_copy(DIGITS(from), DIGITS(to), USED(from));
		}
		else
		{
			if ((tmp = s_mp_alloc(USED(from), sizeof(mp_digit))) == NULL)
				return MP_MEM;
			s_mp_copy(DIGITS(from), tmp, USED(from));
			if (DIGITS(to) != NULL)
			{
#if MP_CRYPTO
				s_mp_setz(DIGITS(to), ALLOC(to));
#endif
				s_mp_free(DIGITS(to));
			}
			DIGITS(to) = tmp;
			ALLOC(to) = USED(from);
		}
		/* Copy the precision and sign from the original */
		USED(to) = USED(from);
		SIGN(to) = SIGN(from);
	} while (0); /* end copy */
	return MP_OKAY;
} /* end mp_copy() */

/*****************************************************************************/
/* Add d to |mp| in place                                                 */
static mp_err
s_mp_add_d(mp_int * mp, mp_digit d)    /* unsigned digit addition */
{
	mp_word w;
	mp_word k = 0;
	mp_size ix = 1;
	mp_size used = USED(mp);
	mp_digit * dp = DIGITS(mp);

	w = dp[0] + d;
	dp[0] = ACCUM(w);
	k = CARRYOUT(w);
	while (ix < used && k)
	{
		w = dp[ix] + k;
		dp[ix] = ACCUM(w);
		k = CARRYOUT(w);
		++ix;
	}
	if (k != 0)
	{
		mp_err res;
		if ((res = s_mp_pad(mp, USED(mp) + 1)) != MP_OKAY)
			return res;
		DIGIT(mp, ix) = k;
	}
	return MP_OKAY;
} /* end s_mp_add_d() */

/*****************************************************************************/
/* Compare |a| <=> d, return 0 if equal, <0 if a<d, >0 if a>d             */
static int
s_mp_cmp_d(mp_int * a, mp_digit d)
{
	mp_size ua = USED(a);
	mp_digit * ap = DIGITS(a);

	if (ua > 1)
		return MP_GT;
	if (*ap < d)
		return MP_LT;
	else if (*ap > d)
		return MP_GT;
	else
		return MP_EQ;
} /* end s_mp_cmp_d() */

/*****************************************************************************/
/* Subtract d from |mp| in place, assumes |mp| > d                        */
static mp_err
s_mp_sub_d(mp_int * mp, mp_digit d)    /* unsigned digit subtract */
{
	mp_word w;
	mp_word b = 0;
	mp_size ix = 1;
	mp_size used = USED(mp);
	mp_digit * dp = DIGITS(mp);

	/* Compute initial subtraction    */
	w = (RADIX + dp[0]) - d;
	b = CARRYOUT(w) ? 0 : 1;
	dp[0] = ACCUM(w);
	/* Propagate borrows leftward     */
	while (b && ix < used)
	{
		w = (RADIX + dp[ix]) - b;
		b = CARRYOUT(w) ? 0 : 1;
		dp[ix] = ACCUM(w);
		++ix;
	}
	/* Remove leading zeroes          */
	s_mp_clamp(mp);
	/* If we have a borrow out, it's a violation of the input invariant */
	if (b)
		return MP_RANGE;
	else
		return MP_OKAY;
} /* end s_mp_sub_d() */

/*****************************************************************************/
/*
  mp_add_d(a, d, b)

  Compute the sum b = a + d, for a single digit d.  Respects the sign of
  its primary addend (single digits are unsigned anyway).
*/
static mp_err
mp_add_d(mp_int * a, mp_digit d, mp_int * b)
{
	mp_err res = MP_OKAY;

	ARGCHK(a != NULL && b != NULL, MP_BADARG);
	if ((res = mp_copy(a, b)) != MP_OKAY)
		return res;
	if (SIGN(b) == MP_ZPOS)
	{
		res = s_mp_add_d(b, d);
	}
	else if (s_mp_cmp_d(b, d) >= 0)
	{
		res = s_mp_sub_d(b, d);
	}
	else
	{
		SIGN(b) = MP_ZPOS;
		DIGIT(b, 0) = d - DIGIT(b, 0);
	}
	return res;
} /* end mp_add_d() */

/*****************************************************************************/
/*
  mp_read_unsigned_bin(mp, str, len)

  Read in an unsigned value (base 256) into the given mp_int
*/
static mp_err
mp_read_unsigned_bin(mp_int * mp, unsigned char * str, int len)
{
	int ix;
	mp_err res;

	ARGCHK(mp != NULL && str != NULL && len > 0, MP_BADARG);
	mp_zero(mp);
	for (ix = 0; ix < len; ix++)
	{
		if ((res = s_mp_mul_2d(mp, CHAR_BIT)) != MP_OKAY)
			return res;
		if ((res = mp_add_d(mp, str[ix], mp)) != MP_OKAY)
			return res;
	}
	return MP_OKAY;
} /* end mp_read_unsigned_bin() */

/*****************************************************************************/
/*
  mp_cmp_z(a)

  Compare a <=> 0.  Returns <0 if a<0, 0 if a=0, >0 if a>0.
*/
static int
mp_cmp_z(mp_int * a)
{
	if (SIGN(a) == MP_NEG)
		return MP_LT;
	else if (USED(a) == 1 && DIGIT(a, 0) == 0)
		return MP_EQ;
	else
		return MP_GT;
} /* end mp_cmp_z() */

/*****************************************************************************/
/*
  mp_init_copy(mp, from)

  Initialize mp as an exact copy of from.  Returns MP_OKAY if
  successful, MP_MEM if memory could not be allocated for the new
  structure.
*/
static mp_err
mp_init_copy(mp_int * mp, mp_int * from)
{
	ARGCHK(mp != NULL && from != NULL, MP_BADARG);
	if (mp == from)
		return MP_OKAY;
	if ((DIGITS(mp) = s_mp_alloc(USED(from), sizeof(mp_digit))) == NULL)
		return MP_MEM;
	s_mp_copy(DIGITS(from), DIGITS(mp), USED(from));
	USED(mp) = USED(from);
	ALLOC(mp) = USED(from);
	SIGN(mp) = SIGN(from);
	return MP_OKAY;
} /* end mp_init_copy() */

/*****************************************************************************/
/* Compare |a| <=> |b|, return 0 if equal, <0 if a<b, >0 if a>b           */
static int
s_mp_cmp(mp_int * a, mp_int * b)
{
	mp_size ua = USED(a);
	mp_size ub = USED(b);

	if (ua > ub)
		return MP_GT;
	else if (ua < ub)
		return MP_LT;
	else
	{
		int ix = ua - 1;
		mp_digit *ap = DIGITS(a) + ix, *bp = DIGITS(b) + ix;
		while (ix >= 0)
		{
			if (*ap > *bp)
				return MP_GT;
			else if (*ap < *bp)
				return MP_LT;
			--ap; --bp; --ix;
		}
		return MP_EQ;
	}
} /* end s_mp_cmp() */

/*****************************************************************************/
static void
mp_set(mp_int * mp, mp_digit d)
{
	if (mp == NULL)
		return;
	mp_zero(mp);
	DIGIT(mp, 0) = d;
} /* end mp_set() */

/*****************************************************************************/
/*
  Returns -1 if the value is not a power of two; otherwise, it returns
  k such that v = 2^k, i.e. lg(v).
*/
static int
s_mp_ispow2(mp_int * v)
{
	mp_digit d;
	mp_digit * dp;
	mp_size uv = USED(v);
	int extra = 0;
	int ix;

	d = DIGIT(v, uv - 1); /* most significant digit of v */
	while (d && ((d & 1) == 0))
	{
		d >>= 1;
		++extra;
	}
	if (d == 1)
	{
		ix = uv - 2;
		dp = DIGITS(v) + ix;
		while (ix >= 0)
		{
			if (*dp)
				return -1; /* not a power of two */
			--dp; --ix;
		}
		return ((uv - 1) * DIGIT_BIT) + extra;
	}
	return -1;
} /* end s_mp_ispow2() */

/*****************************************************************************/
/*
   Shift mp rightward by p digits.  Maintains the invariant that
   digits above the precision are all zero.  Digits shifted off the
   end are lost.  Cannot fail.
*/
static void
s_mp_rshd(mp_int * mp, mp_size p)
{
	mp_size ix;
	mp_digit * dp;

	if (p == 0)
		return;
	/* Shortcut when all digits are to be shifted off */
	if (p >= USED(mp))
	{
		s_mp_setz(DIGITS(mp), ALLOC(mp));
		USED(mp) = 1;
		SIGN(mp) = MP_ZPOS;
		return;
	}
	/* Shift all the significant figures over as needed */
	dp = DIGITS(mp);
	for (ix = p; ix < USED(mp); ix++)
		dp[ix - p] = dp[ix];
	/* Fill the top digits with zeroes */
	ix -= p;
	while (ix < USED(mp))
		dp[ix++] = 0;
	/* Strip off any leading zeroes    */
	s_mp_clamp(mp);
} /* end s_mp_rshd() */

/*****************************************************************************/
/*
  Divide the integer by 2^d, where d is a number of bits.  This
  amounts to a bitwise shift of the value, and does not require the
  full division code (used in Barrett reduction, see below)
*/
static void
s_mp_div_2d(mp_int * mp, mp_digit d)
{
	int ix;
	mp_digit save;
	mp_digit next;
	mp_digit mask;
	mp_digit * dp = DIGITS(mp);

	s_mp_rshd(mp, d / DIGIT_BIT);
	d %= DIGIT_BIT;
	mask = (1 << d) - 1;
	save = 0;
	for (ix = USED(mp) - 1; ix >= 0; ix--)
	{
		next = dp[ix] & mask;
		dp[ix] = (dp[ix] >> d) | (save << (DIGIT_BIT - d));
		save = next;
	}
	s_mp_clamp(mp);
} /* end s_mp_div_2d() */

/*****************************************************************************/
/*
  Remainder the integer by 2^d, where d is a number of bits.  This
  amounts to a bitwise AND of the value, and does not require the full
  division code
*/
static void
s_mp_mod_2d(mp_int * mp, mp_digit d)
{
	unsigned int ndig = (d / DIGIT_BIT);
	unsigned int nbit = (d % DIGIT_BIT);
	unsigned int ix;
	mp_digit dmask;
	mp_digit * dp = DIGITS(mp);

	if (ndig >= USED(mp))
		return;
	/* Flush all the bits above 2^d in its digit */
	dmask = (1 << nbit) - 1;
	dp[ndig] &= dmask;
	/* Flush all digits above the one with 2^d in it */
	for (ix = ndig + 1; ix < USED(mp); ix++)
		dp[ix] = 0;
	s_mp_clamp(mp);
} /* end s_mp_mod_2d() */

/*****************************************************************************/
/*
  s_mp_norm(a, b)

  Normalize a and b for division, where b is the divisor.  In order
  that we might make good guesses for quotient digits, we want the
  leading digit of b to be at least half the radix, which we
  accomplish by multiplying a and b by a constant.  This constant is
  returned (so that it can be divided back out of the remainder at the
  end of the division process).

  We multiply by the smallest power of 2 that gives us a leading digit
  at least half the radix.  By choosing a power of 2, we simplify the
  multiplication and division steps to simple shifts.
*/
static mp_digit
s_mp_norm(mp_int * a, mp_int * b)
{
	mp_digit t;
	mp_digit d = 0;

	t = DIGIT(b, USED(b) - 1);
	while (t < (RADIX / 2))
	{
		t <<= 1;
		++d;
	}
	if (d != 0)
	{
		s_mp_mul_2d(a, d);
		s_mp_mul_2d(b, d);
	}
	return d;
} /* end s_mp_norm() */

/*****************************************************************************/
/* Compute a = a * d, single digit multiplication                         */
static mp_err
s_mp_mul_d(mp_int * a, mp_digit d)
{
	mp_word w;
	mp_word k = 0;
	mp_size ix;
	mp_size max;
	mp_err res;
	mp_digit * dp = DIGITS(a);

	/*
		Single-digit multiplication will increase the precision of the
		output by at most one digit.  However, we can detect when this
		will happen -- if the high-order digit of a, times d, gives a
		two-digit result, then the precision of the result will increase;
		otherwise it won't.  We use this fact to avoid calling s_mp_pad()
		unless absolutely necessary.
	*/
	max = USED(a);
	w = dp[max - 1] * d;
	if (CARRYOUT(w) != 0)
	{
		if ((res = s_mp_pad(a, max + 1)) != MP_OKAY)
			return res;
		dp = DIGITS(a);
	}
	for (ix = 0; ix < max; ix++)
	{
		w = (dp[ix] * d) + k;
		dp[ix] = ACCUM(w);
		k = CARRYOUT(w);
	}
	/* If there is a precision increase, take care of it here; the above
		test guarantees we have enough storage to do this safely.
	*/
	if (k)
	{
		dp[max] = k;
		USED(a) = max + 1;
	}
	s_mp_clamp(a);
	return MP_OKAY;
} /* end s_mp_mul_d() */

/*****************************************************************************/
/* Compute a = |a| - |b|, assumes |a| >= |b|                              */
static mp_err
s_mp_sub(mp_int * a, mp_int * b) /* magnitude subtract      */
{
	mp_word w = 0;
	mp_digit * pa;
	mp_digit * pb;
	mp_size ix;
	mp_size used = USED(b);

	/*
		Subtract and propagate borrow.  Up to the precision of b, this
		accounts for the digits of b; after that, we just make sure the
		carries get to the right place.  This saves having to pad b out to
		the precision of a just to make the loops work right...
	*/
	pa = DIGITS(a);
	pb = DIGITS(b);
	for (ix = 0; ix < used; ++ix)
	{
		w = (RADIX + *pa) - w - *pb++;
		*pa++ = ACCUM(w);
		w = CARRYOUT(w) ? 0 : 1;
	}
	used = USED(a);
	while (ix < used)
	{
		w = RADIX + *pa - w;
		*pa++ = ACCUM(w);
		w = CARRYOUT(w) ? 0 : 1;
		++ix;
	}
	/* Clobber any leading zeroes we created    */
	s_mp_clamp(a);
	/*
		If there was a borrow out, then |b| > |a| in violation
		of our input invariant.  We've already done the work,
		but we'll at least complain about it...
	*/
	if (w)
		return MP_RANGE;
	else
		return MP_OKAY;
} /* end s_mp_sub() */

/*****************************************************************************/
/* Exchange the data for a and b; (b, a) = (a, b)                         */
void s_mp_exch(mp_int * a, mp_int * b)
{
	mp_int tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
} /* end s_mp_exch() */

/*****************************************************************************/
/*
  mp_clear(mp)

  Release the storage used by an mp_int, and void its fields so that
  if someone calls mp_clear() again for the same int later, we won't
  get tollchocked.
*/
static void
mp_clear(mp_int * mp)
{
	if (mp == NULL)
		return;
	if (DIGITS(mp) != NULL)
	{
#if MP_CRYPTO
		s_mp_setz(DIGITS(mp), ALLOC(mp));
#endif
		s_mp_free(DIGITS(mp));
		DIGITS(mp) = NULL;
	}
	USED(mp) = 0;
	ALLOC(mp) = 0;
} /* end mp_clear() */

/*****************************************************************************/
/*
  s_mp_div(a, b)

  Compute a = a / b and b = a mod b.  Assumes b > a.
*/
static mp_err
s_mp_div(mp_int * a, mp_int * b)
{
	mp_int quot;
	mp_int rem;
	mp_int t;
	mp_word q;
	mp_err res;
	mp_digit d;
	int ix;

	if (mp_cmp_z(b) == 0)
		return MP_RANGE;
	/* Shortcut if b is power of two */
	if ((ix = s_mp_ispow2(b)) >= 0)
	{
		mp_copy(a, b);  /* need this for remainder */
		s_mp_div_2d(a, (mp_digit)ix);
		s_mp_mod_2d(b, (mp_digit)ix);
		return MP_OKAY;
	}
	/* Allocate space to store the quotient */
	if ((res = mp_init_size(&quot, USED(a))) != MP_OKAY)
		return res;
	/* A working temporary for division     */
	if ((res = mp_init_size(&t, USED(a))) != MP_OKAY)
		goto T;
	/* Allocate space for the remainder     */
	if ((res = mp_init_size(&rem, USED(a))) != MP_OKAY)
		goto REM;
	/* Normalize to optimize guessing       */
	d = s_mp_norm(a, b);
	/* Perform the division itself...woo!   */
	ix = USED(a) - 1;
	while (ix >= 0)
	{
		/* Find a partial substring of a which is at least b */
		while (s_mp_cmp(&rem, b) < 0 && ix >= 0)
		{
			if ((res = s_mp_lshd(&rem, 1)) != MP_OKAY)
				goto CLEANUP;
			if ((res = s_mp_lshd(&quot, 1)) != MP_OKAY)
				goto CLEANUP;
			DIGIT(&rem, 0) = DIGIT(a, ix);
			s_mp_clamp(&rem);
			--ix;
		}
		/* If we didn't find one, we're finished dividing    */
		if (s_mp_cmp(&rem, b) < 0)
			break;
		/* Compute a guess for the next quotient digit       */
		q = DIGIT(&rem, USED(&rem) - 1);
		if (q <= DIGIT(b, USED(b) - 1) && USED(&rem) > 1)
			q = (q << DIGIT_BIT) | DIGIT(&rem, USED(&rem) - 2);
		q /= DIGIT(b, USED(b) - 1);
		/* The guess can be as much as RADIX + 1 */
		if (q >= RADIX)
			q = RADIX - 1;
		/* See what that multiplies out to                   */
		mp_copy(b, &t);
		if ((res = s_mp_mul_d(&t, q)) != MP_OKAY)
			goto CLEANUP;
		/*
			If it's too big, back it off.  We should not have to do this
			more than once, or, in rare cases, twice.  Knuth describes a
			method by which this could be reduced to a maximum of once, but
			I didn't implement that here.
		*/
		while (s_mp_cmp(&t, &rem) > 0)
		{
			--q;
			s_mp_sub(&t, b);
		}
		/* At this point, q should be the right next digit   */
		if ((res = s_mp_sub(&rem, &t)) != MP_OKAY)
			goto CLEANUP;
		/*
			Include the digit in the quotient.  We allocated enough memory
			for any quotient we could ever possibly get, so we should not
			have to check for failures here
		*/
		DIGIT(&quot, 0) = q;
	}
	/* Denormalize remainder                */
	if (d != 0)
		s_mp_div_2d(&rem, d);
	s_mp_clamp(&quot);
	s_mp_clamp(&rem);
	/* Copy quotient back to output         */
	s_mp_exch(&quot, a);
	/* Copy remainder back to output        */
	s_mp_exch(&rem, b);
CLEANUP:
	mp_clear(&rem);
REM:
	mp_clear(&t);
T:
	mp_clear(&quot);
	return res;
} /* end s_mp_div() */

/*****************************************************************************/
/*
  mp_div(a, b, q, r)

  Compute q = a / b and r = a mod b.  Input parameters may be re-used
  as output parameters.  If q or r is NULL, that portion of the
  computation will be discarded (although it will still be computed)

  Pay no attention to the hacker behind the curtain.
*/
static mp_err
mp_div(mp_int * a, mp_int * b, mp_int * q, mp_int * r)
{
	mp_err res;
	mp_int qtmp;
	mp_int rtmp;
	int cmp;

	ARGCHK(a != NULL && b != NULL, MP_BADARG);
	if (mp_cmp_z(b) == MP_EQ)
		return MP_RANGE;
	/* If a <= b, we can compute the solution without division, and
		avoid any memory allocation
	*/
	if ((cmp = s_mp_cmp(a, b)) < 0)
	{
		if (r)
		{
			if ((res = mp_copy(a, r)) != MP_OKAY)
				return res;
		}
		if (q)
			mp_zero(q);
		return MP_OKAY;
	}
	else if (cmp == 0)
	{
		/* Set quotient to 1, with appropriate sign */
		if (q)
		{
			int qneg = (SIGN(a) != SIGN(b));
			mp_set(q, 1);
			if (qneg)
				SIGN(q) = MP_NEG;
		}
		if (r)
			mp_zero(r);
		return MP_OKAY;
	}

	/* If we get here, it means we actually have to do some division */

	/* Set up some temporaries... */
	if ((res = mp_init_copy(&qtmp, a)) != MP_OKAY)
		return res;
	if ((res = mp_init_copy(&rtmp, b)) != MP_OKAY)
		goto CLEANUP;

	if ((res = s_mp_div(&qtmp, &rtmp)) != MP_OKAY)
		goto CLEANUP;
	/* Compute the signs for the output  */
	SIGN(&rtmp) = SIGN(a); /* Sr = Sa              */
	if (SIGN(a) == SIGN(b))
		SIGN(&qtmp) = MP_ZPOS;  /* Sq = MP_ZPOS if Sa = Sb */
	else
		SIGN(&qtmp) = MP_NEG;   /* Sq = MP_NEG if Sa != Sb */
	if (s_mp_cmp_d(&qtmp, 0) == MP_EQ)
		SIGN(&qtmp) = MP_ZPOS;
	if (s_mp_cmp_d(&rtmp, 0) == MP_EQ)
		SIGN(&rtmp) = MP_ZPOS;

	/* Copy output, if it is needed      */
	if (q)
		s_mp_exch(&qtmp, q);
	if (r)
		s_mp_exch(&rtmp, r);
CLEANUP:
	mp_clear(&rtmp);
	mp_clear(&qtmp);
	return res;
} /* end mp_div() */

/*****************************************************************************/
/* Compute a = |a| + |b|                                                  */
static mp_err
s_mp_add(mp_int * a, mp_int * b)        /* magnitude addition      */
{
	mp_word w = 0;
	mp_digit * pa;
	mp_digit * pb;
	mp_size ix;
	mp_size used = USED(b);
	mp_err res;

	/* Make sure a has enough precision for the output value */
	if ((used > USED(a)) && (res = s_mp_pad(a, used)) != MP_OKAY)
		return res;
	/*
		Add up all digits up to the precision of b.  If b had initially
		the same precision as a, or greater, we took care of it by the
		padding step above, so there is no problem.  If b had initially
		less precision, we'll have to make sure the carry out is duly
		propagated upward among the higher-order digits of the sum.
	*/
	pa = DIGITS(a);
	pb = DIGITS(b);
	for (ix = 0; ix < used; ++ix)
	{
		w += *pa + *pb++;
		*pa++ = ACCUM(w);
		w = CARRYOUT(w);
	}
	/* If we run out of 'b' digits before we're actually done, make
		sure the carries get propagated upward...
	*/
	used = USED(a);
	while (w && ix < used)
	{
		w += *pa;
		*pa++ = ACCUM(w);
		w = CARRYOUT(w);
		++ix;
	}
	/* If there's an overall carry out, increase precision and include
		it.  We could have done this initially, but why touch the memory
		allocator unless we're sure we have to?
	*/
	if (w)
	{
		if ((res = s_mp_pad(a, used + 1)) != MP_OKAY)
			return res;
		DIGIT(a, ix) = w;  /* pa may not be valid after s_mp_pad() call */
	}
	return MP_OKAY;
} /* end s_mp_add() */

/*****************************************************************************/
/*
  mp_add(a, b, c)

  Compute c = a + b.  All parameters may be identical.
*/
static mp_err
mp_add(mp_int * a, mp_int * b, mp_int * c)
{
	mp_err res;
	int cmp;

	ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);
	if (SIGN(a) == SIGN(b))
	{ /* same sign:  add values, keep sign */
		/* Commutativity of addition lets us do this in either order,
			so we avoid having to use a temporary even if the result
			is supposed to replace the output
		*/
		if (c == b)
		{
			if ((res = s_mp_add(c, a)) != MP_OKAY)
				return res;
		}
		else
		{
			if (c != a && (res = mp_copy(a, c)) != MP_OKAY)
				return res;
			if ((res = s_mp_add(c, b)) != MP_OKAY)
				return res;
		}
	}
	else if ((cmp = s_mp_cmp(a, b)) > 0)
	{  /* different sign: a > b   */
		/* If the output is going to be clobbered, we will use a temporary
			variable; otherwise, we'll do it without touching the memory
			allocator at all, if possible
		*/
		if (c == b)
		{
			mp_int  tmp;
			if ((res = mp_init_copy(&tmp, a)) != MP_OKAY)
				return res;
			if ((res = s_mp_sub(&tmp, b)) != MP_OKAY)
			{
				mp_clear(&tmp);
				return res;
			}
			s_mp_exch(&tmp, c);
			mp_clear(&tmp);
		}
		else
		{
			if (c != a && (res = mp_copy(a, c)) != MP_OKAY)
				return res;
			if ((res = s_mp_sub(c, b)) != MP_OKAY)
				return res;
		}
	}
	else if (cmp == 0)
	{             /* different sign, a == b   */
		mp_zero(c);
		return MP_OKAY;
	}
	else
	{     /* different sign: a < b    */
		/* See above... */
		if (c == a)
		{
			mp_int  tmp;
			if ((res = mp_init_copy(&tmp, b)) != MP_OKAY)
				return res;
			if ((res = s_mp_sub(&tmp, a)) != MP_OKAY)
			{
				mp_clear(&tmp);
				return res;
			}
			s_mp_exch(&tmp, c);
			mp_clear(&tmp);
		}
		else
		{
			if (c != b && (res = mp_copy(b, c)) != MP_OKAY)
				return res;
			if ((res = s_mp_sub(c, a)) != MP_OKAY)
				return res;
		}
	}
	if (USED(c) == 1 && DIGIT(c, 0) == 0)
		SIGN(c) = MP_ZPOS;
	return MP_OKAY;
} /* end mp_add() */

/*****************************************************************************/
/*
  mp_mod(a, m, c)

  Compute c = a (mod m).  Result will always be 0 <= c < m.
*/
static mp_err
mp_mod(mp_int * a, mp_int * m, mp_int * c)
{
	mp_err res;
	int mag;

	ARGCHK(a != NULL && m != NULL && c != NULL, MP_BADARG);
	if (SIGN(m) == MP_NEG)
		return MP_RANGE;
	/*
		If |a| > m, we need to divide to get the remainder and take the
		absolute value.

		If |a| < m, we don't need to do any division, just copy and adjust
		the sign (if a is negative).

		If |a| == m, we can simply set the result to zero.

		This order is intended to minimize the average path length of the
		comparison chain on common workloads -- the most frequent cases are
		that |a| != m, so we do those first.
	*/
	if ((mag = s_mp_cmp(a, m)) > 0)
	{
		if ((res = mp_div(a, m, NULL, c)) != MP_OKAY)
			return res;
		if (SIGN(c) == MP_NEG)
		{
			if ((res = mp_add(c, m, c)) != MP_OKAY)
				return res;
		}
	}
	else if (mag < 0)
	{
		if ((res = mp_copy(a, c)) != MP_OKAY)
			return res;
		if (mp_cmp_z(a) < 0)
		{
			if ((res = mp_add(c, m, c)) != MP_OKAY)
				return res;
		}
	}
	else
	{
		mp_zero(c);
	}
	return MP_OKAY;
} /* end mp_mod() */

/*****************************************************************************/
/* Compute a = |a| * |b|                                                  */
static mp_err
s_mp_mul(mp_int * a, mp_int * b)
{
	mp_word w;
	mp_word k = 0;
	mp_int tmp;
	mp_err res;
	mp_size ix;
	mp_size jx;
	mp_size ua = USED(a);
	mp_size ub = USED(b);
	mp_digit * pa;
	mp_digit * pb;
	mp_digit * pt;
	mp_digit * pbt;

	if ((res = mp_init_size(&tmp, ua + ub)) != MP_OKAY)
		return res;
	/* This has the effect of left-padding with zeroes... */
	USED(&tmp) = ua + ub;
	/* We're going to need the base value each iteration */
	pbt = DIGITS(&tmp);
	/* Outer loop:  Digits of b */
	pb = DIGITS(b);
	for (ix = 0; ix < ub; ++ix, ++pb)
	{
		if (*pb == 0)
			continue;
		/* Inner product:  Digits of a */
		pa = DIGITS(a);
		for (jx = 0; jx < ua; ++jx, ++pa)
		{
			pt = pbt + ix + jx;
			w = *pb * *pa + k + *pt;
			*pt = ACCUM(w);
			k = CARRYOUT(w);
		}
		pbt[ix + jx] = k;
		k = 0;
	}
	s_mp_clamp(&tmp);
	s_mp_exch(&tmp, a);
	mp_clear(&tmp);
	return MP_OKAY;
} /* end s_mp_mul() */

/*****************************************************************************/
/*
  mp_sub(a, b, c)

  Compute c = a - b.  All parameters may be identical.
*/
static mp_err
mp_sub(mp_int * a, mp_int * b, mp_int * c)
{
	mp_err res;
	int cmp;

	ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);
	if (SIGN(a) != SIGN(b))
	{
		if (c == a)
		{
			if ((res = s_mp_add(c, b)) != MP_OKAY)
				return res;
		}
		else
		{
			if (c != b && ((res = mp_copy(b, c)) != MP_OKAY))
				return res;
			if ((res = s_mp_add(c, a)) != MP_OKAY)
				return res;
			SIGN(c) = SIGN(a);
		}
	}
	else if ((cmp = s_mp_cmp(a, b)) > 0)
	{ /* Same sign, a > b */
		if (c == b)
		{
			mp_int  tmp;
			if ((res = mp_init_copy(&tmp, a)) != MP_OKAY)
				return res;
			if ((res = s_mp_sub(&tmp, b)) != MP_OKAY)
			{
				mp_clear(&tmp);
				return res;
			}
			s_mp_exch(&tmp, c);
			mp_clear(&tmp);
		}
		else
		{
			if (c != a && ((res = mp_copy(a, c)) != MP_OKAY))
				return res;
			if ((res = s_mp_sub(c, b)) != MP_OKAY)
				return res;
		}
	}
	else if (cmp == 0)
	{  /* Same sign, equal magnitude */
		mp_zero(c);
		return MP_OKAY;
	}
	else
	{               /* Same sign, b > a */
		if (c == a)
		{
			mp_int  tmp;
			if ((res = mp_init_copy(&tmp, b)) != MP_OKAY)
				return res;
			if ((res = s_mp_sub(&tmp, a)) != MP_OKAY)
			{
				mp_clear(&tmp);
				return res;
			}
			s_mp_exch(&tmp, c);
			mp_clear(&tmp);
		}
		else
		{
			if (c != b && ((res = mp_copy(b, c)) != MP_OKAY))
				return res;
			if ((res = s_mp_sub(c, a)) != MP_OKAY)
				return res;
		}
		SIGN(c) = !SIGN(b);
	}
	if (USED(c) == 1 && DIGIT(c, 0) == 0)
		SIGN(c) = MP_ZPOS;
	return MP_OKAY;
} /* end mp_sub() */

/*****************************************************************************/
static int
mp_cmp(mp_int * a, mp_int * b)
{
	ARGCHK(a != NULL && b != NULL, MP_EQ);
	if (SIGN(a) == SIGN(b))
	{
		int  mag;
		if ((mag = s_mp_cmp(a, b)) == MP_EQ)
			return MP_EQ;
		if (SIGN(a) == MP_ZPOS)
			return mag;
		else
			return -mag;
	}
	else if (SIGN(a) == MP_ZPOS)
	{
		return MP_GT;
	}
	else
	{
		return MP_LT;
	}
} /* end mp_cmp() */

/*****************************************************************************/
/*
  Compute Barrett reduction, x (mod m), given a precomputed value for
  mu = b^2k / m, where b = RADIX and k = #digits(m).  This should be
  faster than straight division, when many reductions by the same
  value of m are required (such as in modular exponentiation).  This
  can nearly halve the time required to do modular exponentiation,
  as compared to using the full integer divide to reduce.

  This algorithm was derived from the _Handbook of Applied
  Cryptography_ by Menezes, Oorschot and VanStone, Ch. 14,
  pp. 603-604.
*/
static mp_err
s_mp_reduce(mp_int * x, mp_int * m, mp_int * mu)
{
	mp_int q;
	mp_err res;
	mp_size um = USED(m);

	if ((res = mp_init_copy(&q, x)) != MP_OKAY)
		return res;
	s_mp_rshd(&q, um - 1);       /* q1 = x / b^(k-1)  */
	s_mp_mul(&q, mu);            /* q2 = q1 * mu      */
	s_mp_rshd(&q, um + 1);       /* q3 = q2 / b^(k+1) */
	/* x = x mod b^(k+1), quick (no division) */
	s_mp_mod_2d(x, DIGIT_BIT * (um + 1));
	/* q = q * m mod b^(k+1), quick (no division) */
	s_mp_mul(&q, m);
	s_mp_mod_2d(&q, DIGIT_BIT * (um + 1));
	/* x = x - q */
	if ((res = mp_sub(x, &q, x)) != MP_OKAY)
		goto CLEANUP;
	/* If x < 0, add b^(k+1) to it */
	if (mp_cmp_z(x) < 0)
	{
		mp_set(&q, 1);
		if ((res = s_mp_lshd(&q, um + 1)) != MP_OKAY)
			goto CLEANUP;
		if ((res = mp_add(x, &q, x)) != MP_OKAY)
			goto CLEANUP;
	}
	/* Back off if it's too big */
	while (mp_cmp(x, m) >= 0)
	{
		if ((res = s_mp_sub(x, m)) != MP_OKAY)
			break;
	}
CLEANUP:
	mp_clear(&q);
	return res;
} /* end s_mp_reduce() */

/*****************************************************************************/
/*
  Computes the square of a, in place.  This can be done more
  efficiently than a general multiplication, because many of the
  computation steps are redundant when squaring.  The inner product
  step is a bit more complicated, but we save a fair number of
  iterations of the multiplication loop.
*/
static mp_err
s_mp_sqr(mp_int * a)
{
	mp_word w;
	mp_word k = 0;
	mp_int tmp;
	mp_err res;
	mp_size ix;
	mp_size jx;
	mp_size kx;
	mp_size used = USED(a);
	mp_digit * pa1;
	mp_digit * pa2;
	mp_digit * pt;
	mp_digit * pbt;

	if ((res = mp_init_size(&tmp, 2 * used)) != MP_OKAY)
		return res;

	/* Left-pad with zeroes */
	USED(&tmp) = 2 * used;

	/* We need the base value each time through the loop */
	pbt = DIGITS(&tmp);

	pa1 = DIGITS(a);
	for (ix = 0; ix < used; ++ix, ++pa1)
	{
		if (*pa1 == 0)
			continue;
		w = DIGIT(&tmp, ix + ix) + (*pa1 * *pa1);
		pbt[ix + ix] = ACCUM(w);
		k = CARRYOUT(w);
		/*
			The inner product is computed as:
			(C, S) = t[i,j] + 2 a[i] a[j] + C
			This can overflow what can be represented in an mp_word, and
			since C arithmetic does not provide any way to check for
			overflow, we have to check explicitly for overflow conditions
			before they happen.
		*/
		for (jx = ix + 1, pa2 = DIGITS(a) + jx; jx < used; ++jx, ++pa2)
		{
			mp_word  u = 0, v;
			/* Store this in a temporary to avoid indirections later */
			pt = pbt + ix + jx;
			/* Compute the multiplicative step */
			w = *pa1 * *pa2;
			/* If w is more than half MP_WORD_MAX, the doubling will
				overflow, and we need to record a carry out into the next
				word */
			u = (w >> (MP_WORD_BIT - 1)) & 1;
			/* Double what we've got, overflow will be ignored as defined
				for C arithmetic (we've already noted if it is to occur) */
			w *= 2;
			/* Compute the additive step */
			v = *pt + k;
			/* If we do not already have an overflow carry, check to see
				if the addition will cause one, and set the carry out if so
			*/
			u |= ((MP_WORD_MAX - v) < w);
			/* Add in the rest, again ignoring overflow */
			w += v;
			/* Set the i,j digit of the output */
			*pt = ACCUM(w);
			/* Save carry information for the next iteration of the loop.
				This is why k must be an mp_word, instead of an mp_digit */
			k = CARRYOUT(w) | (u << DIGIT_BIT);
		} /* for(jx ...) */
		/* Set the last digit in the cycle and reset the carry */
		k = DIGIT(&tmp, ix + jx) + k;
		pbt[ix + jx] = ACCUM(k);
		k = CARRYOUT(k);
		/* If we are carrying out, propagate the carry to the next digit
			in the output.  This may cascade, so we have to be somewhat
			circumspect -- but we will have enough precision in the output
			that we won't overflow
		*/
		kx = 1;
		while (k)
		{
			k = pbt[ix + jx + kx] + 1;
			pbt[ix + jx + kx] = ACCUM(k);
			k = CARRYOUT(k);
			++kx;
		}
	} /* for(ix ...) */
	s_mp_clamp(&tmp);
	s_mp_exch(&tmp, a);
	mp_clear(&tmp);
	return MP_OKAY;
} /* end s_mp_sqr() */

/*****************************************************************************/
/*
  mp_exptmod(a, b, m, c)

  Compute c = (a ** b) mod m.  Uses a standard square-and-multiply
  method with modular reductions at each step. (This is basically the
  same code as mp_expt(), except for the addition of the reductions)

  The modular reductions are done using Barrett's algorithm (see
  s_mp_reduce() below for details)
*/
static mp_err
mp_exptmod(mp_int * a, mp_int * b, mp_int * m, mp_int * c)
{
	mp_int s;
	mp_int x;
	mp_int mu;
	mp_err res;
	mp_digit d;
	mp_digit * db = DIGITS(b);
	mp_size ub = USED(b);
	int dig;
	int bit;

	ARGCHK(a != NULL && b != NULL && c != NULL, MP_BADARG);
	if (mp_cmp_z(b) < 0 || mp_cmp_z(m) <= 0)
		return MP_RANGE;
	if ((res = mp_init(&s)) != MP_OKAY)
		return res;
	if ((res = mp_init_copy(&x, a)) != MP_OKAY)
		goto X;
	if ((res = mp_mod(&x, m, &x)) != MP_OKAY ||
		(res = mp_init(&mu)) != MP_OKAY)
		goto MU;
	mp_set(&s, 1);
	/* mu = b^2k / m */
	s_mp_add_d(&mu, 1);
	s_mp_lshd(&mu, 2 * USED(m));
	if ((res = mp_div(&mu, m, &mu, NULL)) != MP_OKAY)
		goto CLEANUP;
	/* Loop over digits of b in ascending order, except highest order */
	for (dig = 0; dig < (ub - 1); dig++)
	{
		d = *db++;
		/* Loop over the bits of the lower-order digits */
		for (bit = 0; bit < DIGIT_BIT; bit++)
		{
			if (d & 1)
			{
				if ((res = s_mp_mul(&s, &x)) != MP_OKAY)
					goto CLEANUP;
				if ((res = s_mp_reduce(&s, m, &mu)) != MP_OKAY)
					goto CLEANUP;
			}
			d >>= 1;
			if ((res = s_mp_sqr(&x)) != MP_OKAY)
				goto CLEANUP;
			if ((res = s_mp_reduce(&x, m, &mu)) != MP_OKAY)
				goto CLEANUP;
		}
	}
	/* Now do the last digit... */
	d = *db;
	while (d)
	{
		if (d & 1)
		{
			if ((res = s_mp_mul(&s, &x)) != MP_OKAY)
				goto CLEANUP;
			if ((res = s_mp_reduce(&s, m, &mu)) != MP_OKAY)
				goto CLEANUP;
		}
		d >>= 1;
		if ((res = s_mp_sqr(&x)) != MP_OKAY)
			goto CLEANUP;
		if ((res = s_mp_reduce(&x, m, &mu)) != MP_OKAY)
			goto CLEANUP;
	}
	s_mp_exch(&s, c);
CLEANUP:
	mp_clear(&mu);
MU:
	mp_clear(&x);
X:
	mp_clear(&s);
	return res;
} /* end mp_exptmod() */

/*****************************************************************************/
static int
mp_unsigned_bin_size(mp_int * mp)
{
	mp_digit topdig;
	int count;

	ARGCHK(mp != NULL, 0);
	/* Special case for the value zero */
	if (USED(mp) == 1 && DIGIT(mp, 0) == 0)
		return 1;
	count = (USED(mp) - 1) * sizeof(mp_digit);
	topdig = DIGIT(mp, USED(mp) - 1);
	while (topdig != 0)
	{
		++count;
		topdig >>= CHAR_BIT;
	}
	return count;
} /* end mp_unsigned_bin_size() */

/*****************************************************************************/
static mp_err
mp_to_unsigned_bin(mp_int * mp, unsigned char * str)
{
	mp_digit * dp;
	mp_digit * end;
	mp_digit d;
	unsigned char * spos;

	ARGCHK(mp != NULL && str != NULL, MP_BADARG);
	dp = DIGITS(mp);
	end = dp + USED(mp) - 1;
	spos = str;
	/* Special case for zero, quick test */
	if ((dp == end) && (*dp == 0))
	{
		*str = '\0';
		return MP_OKAY;
	}
	/* Generate digits in reverse order */
	while (dp < end)
	{
		int ix;
		d = *dp;
		for (ix = 0; ix < sizeof(mp_digit); ++ix)
		{
			*spos = d & UCHAR_MAX;
			d >>= CHAR_BIT;
			++spos;
		}
		++dp;
	}
	/* Now handle last digit specially, high order zeroes are not written */
	d = *end;
	while (d != 0)
	{
		*spos = d & UCHAR_MAX;
		d >>= CHAR_BIT;
		++spos;
	}
	/* Reverse everything to get digits in the correct order */
	while (--spos > str)
	{
		unsigned char t = *str;
		*str = *spos;
		*spos = t;
		++str;
	}
	return MP_OKAY;
} /* end mp_to_unsigned_bin() */

/*****************************************************************************/
static void
ssl_reverse_it(char * p, int len)
{
	int i;
	int j;
	char temp;

	i = 0;
	j = len - 1;
	while (i < j)
	{
		temp = p[i];
		p[i] = p[j];
		p[j] = temp;
		i++;
		j--;
	}
}

#define PAR_MAX 1024

/*****************************************************************************/
static int
ssl_mod_exp(char * out, int out_len, char * in, int in_len,
	char * mod, int mod_len, char * exp, int exp_len)
{
	mp_int lout;
	mp_int lin;
	mp_int lmod;
	mp_int lexp;
	char * data;
	char * jin;
	char * jmod;
	char * jexp;
	char * llout;
	int sout;

	if ((out_len > PAR_MAX) || (in_len > PAR_MAX) ||
		(mod_len > PAR_MAX) || (exp_len > PAR_MAX))
	{
		printf("ssl_mod_exp: too big\n");
		return 1;
	}
	data = (char *) l_malloc(PAR_MAX * 4, 1);
	if (data == NULL)
	{
		printf("ssl_mod_exp: l_malloc failed\n");
		return 1;
	}
	jin = data;
	jmod = jin + PAR_MAX;
	jexp = jmod + PAR_MAX;
	llout = jexp + PAR_MAX;
	mp_init(&lout);
	mp_init(&lin);
	mp_init(&lmod);
	mp_init(&lexp);
	memcpy(jin, in, in_len);
	ssl_reverse_it(jin, in_len);
	memcpy(jmod, mod, mod_len);
	ssl_reverse_it(jmod, mod_len);
	memcpy(jexp, exp, exp_len);
	ssl_reverse_it(jexp, exp_len);
	mp_read_unsigned_bin(&lin, (uint8 *) jin, in_len);
	mp_read_unsigned_bin(&lmod, (uint8 *) jmod, mod_len);
	mp_read_unsigned_bin(&lexp, (uint8 *) jexp, exp_len);
	mp_exptmod(&lin, &lexp, &lmod, &lout);
	sout = mp_unsigned_bin_size(&lout);
	mp_to_unsigned_bin(&lout, (uint8 *) llout);
	ssl_reverse_it(llout, sout);
	memcpy(out, llout, out_len);
	mp_clear(&lout);
	mp_clear(&lin);
	mp_clear(&lmod);
	mp_clear(&lexp);
	/* clear data before free */
	memset(data, 0, PAR_MAX * 4);
	xfree(data);
	return 0;
}

/*****************************************************************************/
void
ssl_rsa_encrypt(uint8 * out, uint8 * in, int len, uint32 modulus_size,
	uint8 * modulus, uint8 * exponent)
{
	ssl_mod_exp((char *) out, modulus_size, (char *) in, len,
		(char *) modulus, modulus_size, (char *) exponent,
		SEC_EXPONENT_SIZE);
}

/*****************************************************************************/
/* returns newly allocated SSL_CERT or NULL */
SSL_CERT *
ssl_cert_read(uint8 * data, uint32 len)
{
	char * p;

	p = (char *) l_malloc(len, 1);
	memcpy(p, data, len);
	return p;
}

/*****************************************************************************/
void
ssl_cert_free(SSL_CERT * cert)
{
	l_free(cert);
}

/*****************************************************************************/
/* returns newly allocated SSL_PUBLIC_KEY or NULL */
SSL_PUBLIC_KEY *
ssl_cert_get_public_key(SSL_CERT * cert, uint32 * key_len)
{
	return 0;
}

/*****************************************************************************/
/* returns boolean */
RD_BOOL
ssl_cert_verify(SSL_CERT * server_cert, SSL_CERT * cacert)
{
	return True;
}

/*****************************************************************************/
int
ssl_cert_print_fp(FILE * fp, SSL_CERT * cert)
{
	return 0;
}

/*****************************************************************************/
void
ssl_public_key_free(SSL_PUBLIC_KEY * public_key)
{
	l_free(public_key);
}

/*****************************************************************************/
/* returns error */
int
ssl_public_key_get_exp_mod(SSL_PUBLIC_KEY * public_key, uint8 * exponent, uint32 max_exp_len,
	uint8 * modulus, uint32 max_mod_len)
{
	return 0;
}
