/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Device Redirection - Pending Request Packet Queue Processing

   Copyright (C) Eduardo Fiss Beloni <beloni@ossystems.com.br> 2010

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

#ifndef __IRP_QUEUE_H
#define __IRP_QUEUE_H

#include "rdpdr_types.h"

struct irp_queue_node
{
	IRP *irp;
	struct irp_queue_node *next;
};

struct irp_queue
{
	struct irp_queue_node *head;
};
typedef struct irp_queue IRPQueue;

IRPQueue *
irp_queue_new();
void
irp_queue_free(IRPQueue * queue);
int
irp_queue_empty(IRPQueue * queue);
void
irp_queue_push(IRPQueue * queue, IRP * irp);
void
irp_queue_pop(IRPQueue * queue);
IRP *
irp_queue_first(IRPQueue * queue);
void
irp_queue_remove(IRPQueue * queue, IRP * irp);
IRP *
irp_queue_next(IRPQueue * queue, IRP * irp);
int
irp_queue_size(IRPQueue * queue);

#endif
