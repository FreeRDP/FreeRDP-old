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
#include <stdlib.h>

#include "rdpdr_types.h"
#include "irp_queue.h"

IRPQueue *
irp_queue_new()
{
	return calloc(1, sizeof(IRPQueue));
}

void
irp_queue_free(IRPQueue * queue)
{
	struct irp_queue_node *walker = queue->head;
	struct irp_queue_node *node = NULL;

	while (walker)
	{
		node = walker->next;
		free(walker->irp);
		free(walker);
		walker = node;
	}

	free(queue);
	queue = NULL;
}

int
irp_queue_empty(IRPQueue * queue)
{
	return !(queue && queue->head);
}

void
irp_queue_push(IRPQueue * queue, IRP * irp)
{
	struct irp_queue_node *walker = NULL;
	struct irp_queue_node *node = NULL;

	if (!queue)
		return;

	for (walker = queue->head; walker; walker = walker->next)
		node = walker;

	walker = calloc(1, sizeof(struct irp_queue_node));
	walker->irp = calloc(1, sizeof(IRP));
	*walker->irp = *irp;

	if (!node) /* the first element */
		queue->head = walker;
	else
		node->next = walker;
}

void
irp_queue_pop(IRPQueue * queue)
{
	if (irp_queue_empty(queue))
		return;

	/* IRP *taken = queue->head->irp; */
	struct irp_queue_node *dying = queue->head;

	queue->head = queue->head->next;
	free(dying->irp);
	free(dying);
}

IRP *
irp_queue_first(IRPQueue * queue)
{
	return !irp_queue_empty(queue) ? queue->head->irp : NULL;
}

void
irp_queue_remove(IRPQueue * queue, IRP * irp)
{
	struct irp_queue_node *walker = NULL;
	struct irp_queue_node *prev = NULL;
	int found = 0;

	if (irp_queue_empty(queue))
		return;

	for (walker = queue->head; walker; walker = walker->next)
	{
		if (walker->irp->completionID == irp->completionID)
		{
			found = 1;
			break;
		}
		prev = walker;
	}

	if (found)
	{
		if (prev)
			prev->next = walker->next;
		else
			queue->head = walker->next;

		free(walker->irp);
		free(walker);
	}
}

IRP *
irp_queue_next(IRPQueue * queue, IRP * irp)
{
	struct irp_queue_node *walker = NULL;
	struct irp_queue_node *prev = NULL;

	if (irp_queue_empty(queue))
		return;

	for (walker = queue->head; walker; walker = walker->next)
		if (walker->irp == irp)
			return walker->next ? walker->next->irp : NULL;

	return NULL;
}

int
irp_queue_size(IRPQueue * queue)
{
	struct irp_queue_node *walker = NULL;
	int size = 0;

	if (irp_queue_empty(queue))
		return 0;

	for (walker = queue->head; walker; walker = walker->next)
		size++;

	return size;
}
