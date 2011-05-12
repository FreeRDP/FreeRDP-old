/*
   FreeRDP: A Remote Desktop Protocol client.
   Device Redirection

   Copyright 2010 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __RDPDR_MAIN_H
#define __RDPDR_MAIN_H

#include <pthread.h>
#include <freerdp/utils.h>

#include "rdpdr_types.h"

struct data_in_item
{
	struct data_in_item * next;
	char * data;
	int data_size;
};

typedef struct rdpdr_plugin rdpdrPlugin;
struct rdpdr_plugin
{
	rdpChanPlugin chan_plugin;

	CHANNEL_ENTRY_POINTS ep;
	CHANNEL_DEF channel_def;
	uint32 open_handle;
	char * data_in;
	int data_in_size;
	int data_in_read;
	struct wait_obj * term_event;
	struct wait_obj * data_in_event;
	struct data_in_item * list_head;
	struct data_in_item * list_tail;
	/* for locking the linked list */
	pthread_mutex_t * mutex;
	int thread_status;

	uint16 versionMinor;
	uint16 clientID;
	DEVMAN* devman;

	/* Async IO stuff */
	IRPQueue * queue;
	fd_set readfds, writefds;
	int nfds;
	struct timeval tv;
	uint32 select_timeout;
};

#endif /* __RDPDR_MAIN_H */
