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

#ifndef __SCARD_QUEUE_H
#define __SCARD_QUEUE_H

struct scard_queue_node
{
	void *elem;
	struct scard_queue_node *next;
};

typedef struct scard_queue
{
	struct scard_queue_node *head;
} ScardQueue;

ScardQueue *
scard_queue_new();
void
scard_queue_free(ScardQueue * queue);
int
scard_queue_empty(ScardQueue * queue);
void
scard_queue_push(ScardQueue * queue, void * elem);
void
scard_queue_pop(ScardQueue * queue);
void *
scard_queue_first(ScardQueue * queue);
void
scard_queue_remove(ScardQueue * queue, void * elem);
void *
scard_queue_next(ScardQueue * queue, void * elem);
int
scard_queue_size(ScardQueue * queue);

#endif
