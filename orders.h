/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   RDP order processing
   Copyright (C) Jay Sorg 2009

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

#ifndef __ORDERS_H
#define __ORDERS_H

struct rdp_orders
{
	struct rdp_rdp *rdp;
	void *order_state;
};
typedef struct rdp_orders rdpOrders;

void process_orders(rdpOrders * orders, STREAM s, uint16 num_orders);
void reset_order_state(rdpOrders * orders);
rdpOrders *orders_setup(struct rdp_rdp *rdp);
void orders_cleanup(rdpOrders * orders);

#endif
