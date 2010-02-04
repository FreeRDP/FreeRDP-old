/*
   Copyright (c) 2009-2010 Jay Sorg

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/time.h>
#include "types_ui.h"
#include "vchan.h"
#include "chan_stream.h"

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

struct wait_obj
{
	int sock;
	struct sockaddr_un sa;
};

struct data_in_item
{
	struct data_in_item * next;
	char * data;
	int data_size;
};

static CHANNEL_ENTRY_POINTS g_ep;
static void * g_han;
static CHANNEL_DEF g_channel_def[2];
static uint32 g_open_handle[2];
static char * g_data_in[2];
static int g_data_in_size[2];
static int g_data_in_read[2];
static struct wait_obj g_term_event;
static struct wait_obj g_data_in_event;
static struct data_in_item * volatile g_list_head;
static struct data_in_item * volatile g_list_tail;
/* for locking the linked list */
static pthread_mutex_t * g_mutex;
static volatile int g_thread_status;

/* get time in milliseconds */
static uint32
get_mstime(void)
{
	struct timeval tp;

	gettimeofday(&tp, 0);
	return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

static int
init_wait_obj(struct wait_obj * obj, const char * name)
{
	int pid;
	int size;

	pid = getpid();
	obj->sock = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (obj->sock < 0)
	{
		LLOGLN(0, ("init_wait_obj: socket failed"));
		return 1;
	}
	obj->sa.sun_family = AF_UNIX;
	size = sizeof(obj->sa.sun_path) - 1;
	snprintf(obj->sa.sun_path, size, "/tmp/%s%8.8x", name, pid);
	obj->sa.sun_path[size] = 0;
	size = sizeof(obj->sa);
	if (bind(obj->sock, (struct sockaddr*)(&(obj->sa)), size) < 0)
	{
		LLOGLN(0, ("init_wait_obj: bind failed"));
		close(obj->sock);
		obj->sock = -1;
		unlink(obj->sa.sun_path);
		return 1;
	}
	return 0;
}

static int
deinit_wait_obj(struct wait_obj * obj)
{
	if (obj->sock != -1)
	{
		close(obj->sock);
		obj->sock = -1;
		unlink(obj->sa.sun_path);
	}
	return 0;
}

static int
is_wait_obj_set(struct wait_obj * obj)
{
	fd_set rfds;
	int num_set;
	struct timeval time;

	FD_ZERO(&rfds);
	FD_SET(obj->sock, &rfds);
	memset(&time, 0, sizeof(time));
	num_set = select(obj->sock + 1, &rfds, 0, 0, &time);
	return (num_set == 1);
}

static int
set_wait_obj(struct wait_obj * obj)
{
	int len;

	if (is_wait_obj_set(obj))
	{
		return 0;
	}
	len = sendto(obj->sock, "sig", 4, 0, (struct sockaddr*)(&(obj->sa)),
		sizeof(obj->sa));
	if (len != 4)
	{
		LLOGLN(0, ("set_wait_obj: error"));
		return 1;
	}
	return 0;
}

static int
clear_wait_obj(struct wait_obj * obj)
{
	int len;

	while (is_wait_obj_set(obj))
	{
		len = recvfrom(obj->sock, &len, 4, 0, 0, 0);
		if (len != 4)
		{
			LLOGLN(0, ("chan_man_clear_ev: error"));
			return 1;
		}
	}
	return 0;
}

static int
wait(int timeout, int numr, int * listr)
{
	int max;
	int rv;
	int index;
	int sock;
	struct timeval time;
	struct timeval * ptime;
	fd_set fds;

	ptime = 0;
	if (timeout >= 0)
	{
		time.tv_sec = timeout / 1000;
		time.tv_usec = (timeout * 1000) % 1000000;
		ptime = &time;
	}
	max = 0;
	FD_ZERO(&fds);
	for (index = 0; index < numr; index++)
	{
		sock = listr[index];
		FD_SET(sock, &fds);
		if (sock > max)
		{
			max = sock;
		}
	}
	rv = select(max + 1, &fds, 0, 0, ptime);
	return rv;
}

/* called by main thread
   add item to linked list and inform worker thread that there is data */
static void
signal_data_in(void)
{
	struct data_in_item * item;

	item = (struct data_in_item *) malloc(sizeof(struct data_in_item));
	item->next = 0;
	item->data = g_data_in[0];
	g_data_in[0] = 0;
	item->data_size = g_data_in_size[0];
	g_data_in_size[0] = 0;
	pthread_mutex_lock(g_mutex);
	if (g_list_tail == 0)
	{
		g_list_head = item;
		g_list_tail = item;
	}
	else
	{
		g_list_tail->next = item;
		g_list_tail = item;
	}
	pthread_mutex_unlock(g_mutex);
	set_wait_obj(&g_data_in_event);
}

static int
thread_process_message(char * data, int data_size)
{

	return 0;
}

/* process the linked list of data that has come in */
static int
thread_process_data(void)
{
	char * data;
	int data_size;
	struct data_in_item * item;

	while (1)
	{
		pthread_mutex_lock(g_mutex);

		if (g_list_head == 0)
		{
			pthread_mutex_unlock(g_mutex);
			break;
		}

		data = g_list_head->data;
		data_size = g_list_head->data_size;
		item = g_list_head;
		g_list_head = g_list_head->next;

		if (g_list_head == 0)
		{
			g_list_tail = 0;
		}

		pthread_mutex_unlock(g_mutex);
		if (data != 0)
		{
			thread_process_message(data, data_size);
			free(data);
		}
		if (item != 0)
		{
			free(item);
		}
	}
	return 0;
}

static void *
thread_func(void * arg)
{
	int listr[2];
	int numr;

	g_thread_status = 1;
	LLOGLN(10, ("thread_func: in"));

	while (1)
	{
		listr[0] = g_term_event.sock;
		listr[1] = g_data_in_event.sock;
		numr = 2;
		wait(-1, numr, listr);

		if (is_wait_obj_set(&g_term_event))
		{
			break;
		}
		if (is_wait_obj_set(&g_data_in_event))
		{
			clear_wait_obj(&g_data_in_event);
			/* process data in */
			thread_process_data();
		}
	}

	LLOGLN(10, ("thread_func: out"));
	g_thread_status = -1;
	return 0;
}

static void
OpenEventProcessReceived(uint32 openHandle, void * pData, uint32 dataLength,
	uint32 totalLength, uint32 dataFlags)
{
	int index;
	index = (openHandle == g_open_handle[0]) ? 0 : 1;

	LLOGLN(10, ("OpenEventProcessReceived: receive openHandle %d dataLength %d "
		"totalLength %d dataFlags %d",
		openHandle, dataLength, totalLength, dataFlags));

	if (dataFlags & CHANNEL_FLAG_FIRST)
	{
		g_data_in_read[index] = 0;
		if (g_data_in[index] != 0)
		{
			free(g_data_in[index]);
		}
		g_data_in[index] = (char *) malloc(totalLength);
		g_data_in_size[index] = totalLength;
	}

	memcpy(g_data_in[index] + g_data_in_read[index], pData, dataLength);
	g_data_in_read[index] += dataLength;

	if (dataFlags & CHANNEL_FLAG_LAST)
	{
		if (g_data_in_read[index] != g_data_in_size[index])
		{
			LLOGLN(0, ("OpenEventProcessReceived: read error"));
		}
		if (index == 0)
		{
			signal_data_in();
		}
	}
}

static void
OpenEvent(uint32 openHandle, uint32 event, void * pData, uint32 dataLength,
	uint32 totalLength, uint32 dataFlags)
{
	LLOGLN(10, ("OpenEvent: event %d", event));
	switch (event)
	{
		case CHANNEL_EVENT_DATA_RECEIVED:
			OpenEventProcessReceived(openHandle, pData, dataLength,
				totalLength, dataFlags);
			break;
		case CHANNEL_EVENT_WRITE_COMPLETE:
			free(pData);
			break;
	}
}

static void
InitEventProcessConnected(void * pInitHandle, void * pData, uint32 dataLength)
{
	uint32 error;
	pthread_t thread;

	if (pInitHandle != g_han)
	{
		LLOGLN(0, ("InitEventProcessConnected: error no match"));
	}
	error = g_ep.pVirtualChannelOpen(g_han, &(g_open_handle[0]),
		g_channel_def[0].name, OpenEvent);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("InitEventProcessConnected: Open failed"));
	}
	error = g_ep.pVirtualChannelOpen(g_han, &(g_open_handle[1]),
		g_channel_def[1].name, OpenEvent);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("InitEventProcessConnected: Open failed"));
	}
	pthread_create(&thread, 0, thread_func, 0);
	pthread_detach(thread);
}

static void
InitEventProcessTerminated(void)
{
	int index;

	set_wait_obj(&g_term_event);
	index = 0;
	while ((g_thread_status > 0) && (index < 100))
	{
		index++;
		usleep(250 * 1000);
	}
	deinit_wait_obj(&g_term_event);
	deinit_wait_obj(&g_data_in_event);
}

static void
InitEvent(void * pInitHandle, uint32 event, void * pData, uint32 dataLength)
{
	LLOGLN(10, ("InitEvent: event %d", event));
	switch (event)
	{
		case CHANNEL_EVENT_CONNECTED:
			InitEventProcessConnected(pInitHandle, pData, dataLength);
			break;
		case CHANNEL_EVENT_DISCONNECTED:
			break;
		case CHANNEL_EVENT_TERMINATED:
			InitEventProcessTerminated();
			break;
	}
}

int
VirtualChannelEntry(PCHANNEL_ENTRY_POINTS pEntryPoints)
{
	LLOGLN(10, ("VirtualChannelEntry:"));
	g_data_in_size[0] = 0;
	g_data_in_size[1] = 0;
	g_data_in[0] = 0;
	g_data_in[1] = 0;
	g_ep = *pEntryPoints;

	memset(&(g_channel_def[0]), 0, sizeof(g_channel_def));
	g_channel_def[0].options = CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP;
	strcpy(g_channel_def[0].name, "rdpdr");

	g_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(g_mutex, 0);
	g_list_head = 0;
	g_list_tail = 0;

	init_wait_obj(&g_term_event, "freerdprdpsndterm");
	init_wait_obj(&g_data_in_event, "freerdprdpsnddatain");

	g_thread_status = 0;

	g_ep.pVirtualChannelInit(&g_han, g_channel_def, 2,
		VIRTUAL_CHANNEL_VERSION_WIN2000, InitEvent);

	return 1;
}

