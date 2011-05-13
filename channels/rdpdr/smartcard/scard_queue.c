/*
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Smart Card Device Service

   Copyright 2011 O.S. Systems Software Ltda.
   Copyright 2011 Eduardo Fiss Beloni <beloni@ossystems.com.br>

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

#include <stdlib.h>

#include "rdpdr_types.h"
#include "scard_queue.h"

ScardQueue *
scard_queue_new()
{
	return calloc(1, sizeof(ScardQueue));
}

void
scard_queue_free(ScardQueue * queue)
{
	struct scard_queue_node *walker = queue->head;
	struct scard_queue_node *node = NULL;

	while (walker)
	{
		node = walker->next;
		free(walker);
		walker = node;
	}

	free(queue);
	queue = NULL;
}

int
scard_queue_empty(ScardQueue * queue)
{
	return !(queue && queue->head);
}

void
scard_queue_push(ScardQueue * queue, void * elem)
{
	struct scard_queue_node *walker = NULL;
	struct scard_queue_node *node = NULL;

	if (!queue)
		return;

	for (walker = queue->head; walker; walker = walker->next)
		node = walker;

	walker = calloc(1, sizeof(struct scard_queue_node));
	walker->elem = elem;

	if (!node) /* the first element */
		queue->head = walker;
	else
		node->next = walker;
}

void
scard_queue_pop(ScardQueue * queue)
{
	if (scard_queue_empty(queue))
		return;

	/* void *taken = queue->head->elem; */
	struct scard_queue_node *dying = queue->head;

	queue->head = queue->head->next;
	free(dying);
}

void *
scard_queue_first(ScardQueue * queue)
{
	return !scard_queue_empty(queue) ? queue->head->elem : NULL;
}

void
scard_queue_remove(ScardQueue * queue, void * elem)
{
	struct scard_queue_node *walker = NULL;
	struct scard_queue_node *prev = NULL;
	_Bool found = 0;

	if (scard_queue_empty(queue))
		return;

	for (walker = queue->head; walker; walker = walker->next)
	{
		if (walker->elem == elem)
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

		free(walker);
	}
}

void *
scard_queue_next(ScardQueue * queue, void * elem)
{
	struct scard_queue_node *walker = NULL;

	if (scard_queue_empty(queue))
		return NULL;

	for (walker = queue->head; walker; walker = walker->next)
		if (walker->elem == elem)
			return walker->next ? walker->next->elem : NULL;

	return NULL;
}

int
scard_queue_size(ScardQueue * queue)
{
	struct scard_queue_node *walker = NULL;
	int size = 0;

	if (scard_queue_empty(queue))
		return 0;

	for (walker = queue->head; walker; walker = walker->next)
		size++;

	return size;
}
