/*
   FreeRDP: A Remote Desktop Protocol client.
   ASN.1 Encoding and Decoding

   Copyright 2009 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#include "frdp.h"
#include "iso.h"
#include "mcs.h"
#include "chan.h"
#include "secure.h"
#include "rdp.h"
#include "mem.h"
#include "asn1.h"

/* Parse an ASN.1 BER header */
RD_BOOL
ber_parse_header(rdpMcs * mcs, STREAM s, int tagval, int *length)
{
	int tag, len;

	if (tagval > 0xff)
	{
		in_uint16_be(s, tag);
	}
	else
	{
		in_uint8(s, tag);
	}

	if (tag != tagval)
	{
		ui_error(mcs->sec->rdp->inst, "expected tag %d, got %d\n", tagval, tag);
		return False;
	}

	in_uint8(s, len);

	if (len & 0x80)
	{
		len &= ~0x80;
		*length = 0;
		while (len--)
			next_be(s, *length);
	}
	else
		*length = len;

	return s_check(s);
}

/* Output an ASN.1 BER header */
void
ber_out_header(STREAM s, int tagval, int length)
{
	if (tagval > 0xff)
	{
		out_uint16_be(s, tagval);
	}
	else
	{
		out_uint8(s, tagval);
	}

	if (length >= 0x80)
	{
		out_uint8(s, 0x82);
		out_uint16_be(s, length);
	}
	else
		out_uint8(s, length);
}

/* Output an ASN.1 BER integer */
void
ber_out_integer(STREAM s, int value)
{
	ber_out_header(s, BER_TAG_INTEGER, 2);
	out_uint16_be(s, value);
}
