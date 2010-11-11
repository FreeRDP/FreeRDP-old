/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Device Redirection

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

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

#ifndef __RDPDR_MAIN_H
#define __RDPDR_MAIN_H

#include "wait_obj.h"

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
