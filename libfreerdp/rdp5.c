/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - RDP5 short form PDU processing
   Copyright (C) Matthew Chapman 1999-2008
   Copyright (C) Erik Forsberg 2003-2008

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
#include "rdp.h"
#include "orders.h"
#include "mem.h"

void
rdp5_process(rdpRdp * rdp, STREAM s)
{
	uint16 length, count, x, y;
	uint8 type, ctype;
	uint8 *next;

	uint32 roff, rlen;
	struct stream *ns = &(rdp->mppc_dict.ns);
	struct stream *ts;

	ui_begin_update(rdp->inst);
	while (s->p < s->end)
	{
		in_uint8(s, type);
		if (type & RDP5_COMPRESSED)
		{
			in_uint8(s, ctype);
			in_uint16_le(s, length);
			type ^= RDP5_COMPRESSED;
		}
		else
		{
			ctype = 0;
			in_uint16_le(s, length);
		}
		rdp->next_packet = next = s->p + length;

		if (ctype & RDP_MPPC_COMPRESSED)
		{
			if (mppc_expand(rdp, s->p, length, ctype, &roff, &rlen) == -1)
				ui_error(rdp->inst, "error while decompressing packet\n");

			/* allocate memory and copy the uncompressed data into the temporary stream */
			ns->data = (uint8 *) xrealloc(ns->data, rlen);

			memcpy((ns->data), (unsigned char *) (rdp->mppc_dict.hist + roff), rlen);

			ns->size = rlen;
			ns->end = ns->data + ns->size;
			ns->p = ns->data;
			ns->rdp_hdr = ns->p;

			ts = ns;
		}
		else
			ts = s;

		switch (type)
		{
			case 0:	/* update orders */
				in_uint16_le(ts, count);
				process_orders(rdp->orders, ts, count);
				break;
			case 1:	/* update bitmap */
				in_uint8s(ts, 2);	/* part length */
				process_bitmap_updates(rdp, ts);
				break;
			case 2:	/* update palette */
				in_uint8s(ts, 2);	/* uint16 = 2 */
				process_palette(rdp, ts);
				break;
			case 3:	/* update synchronize */
				break;
			case 5:	/* null pointer */
				ui_set_null_cursor(rdp->inst);
				break;
			case 6:	/* default pointer */
				break;
			case 8:	/* pointer position */
				in_uint16_le(ts, x);
				in_uint16_le(ts, y);
				ui_move_pointer(rdp->inst, x, y);
				break;
			case 9:	/* color pointer */
				process_color_pointer_pdu(rdp, ts);
				break;
			case 10:	/* cached pointer */
				process_cached_pointer_pdu(rdp, ts);
				break;
			case 11:
				process_new_pointer_pdu(rdp, ts);
				break;
			default:
				ui_unimpl(rdp->inst, "RDP5 opcode %d\n", type);
		}

		s->p = next;
	}
	ui_end_update(rdp->inst);
}
