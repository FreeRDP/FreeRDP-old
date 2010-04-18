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

#ifndef __RDPDR_H
#define __RDPDR_H

#include <freerdp/types_ui.h>
#include <freerdp/vchan.h>
#include "rdpdr_constants.h"
#include "irp.h"
#include "devman.h"
#include "types.h"
#include "chan_stream.h"
#include "chan_plugin.h"

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

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
	CHANNEL_DEF channel_def[2];
	uint32 open_handle[2];
	char * data_in[2];
	int data_in_size[2];
	int data_in_read[2];
	struct wait_obj * term_event;
	struct wait_obj * data_in_event;
	struct data_in_item * volatile list_head;
	struct data_in_item * volatile list_tail;
	/* for locking the linked list */
	pthread_mutex_t * mutex;
	volatile int thread_status;

	uint16 versionMinor;
	uint16 clientID;
	DEVMAN* devman;
};

#endif /* __RDPDR_H */
