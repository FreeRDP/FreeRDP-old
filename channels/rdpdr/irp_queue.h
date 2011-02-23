/*
   FreeRDP: A Remote Desktop Protocol client.
   Device Redirection - Pending Request Packet Queue Processing

   Copyright 2010 O.S. Systems Software Ltda.
   Copyright 2010 Eduardo Fiss Beloni <beloni@ossystems.com.br>

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
