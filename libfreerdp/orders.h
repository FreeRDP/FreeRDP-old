/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP order processing

   Copyright (C) Jay Sorg 2009-2011

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

#ifndef __ORDERS_H
#define __ORDERS_H

#include <stddef.h>
#include <freerdp/types_ui.h>
#include "stream.h"

struct rdp_orders
{
	struct rdp_rdp *rdp;
	void *order_state;
	void *buffer;
	size_t buffer_size;
};
typedef struct rdp_orders rdpOrders;

void process_orders(rdpOrders * orders, STREAM s, uint16 num_orders);
void reset_order_state(rdpOrders * orders);
rdpOrders *orders_new(struct rdp_rdp *rdp);
void orders_free(rdpOrders * orders);

#endif
