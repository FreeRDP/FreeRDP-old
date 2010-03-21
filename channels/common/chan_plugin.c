/*
   Copyright (c) 2010 Vic Lee

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
#include <freerdp/types_ui.h>
#include <freerdp/vchan.h>
#include "chan_plugin.h"

/* The list of all plugin instances. */
typedef struct rdp_chan_plugin_list rdpChanPluginList;
struct rdp_chan_plugin_list
{
	rdpChanPlugin * chan_plugin;
	rdpChanPluginList * next;
};

static rdpChanPluginList * g_chan_plugin_list = NULL;

/* For locking the global resources */
static pthread_mutex_t * g_mutex = NULL;

void
chan_plugin_init(rdpChanPlugin * chan_plugin)
{
	rdpChanPluginList * list;

	/* The channel manager will guarantee only one thread can call
	   VirtualChannelInit at a time. So this should be safe. */
	if (g_mutex == NULL)
	{
		g_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(g_mutex, 0);
	}

	chan_plugin->init_handle = NULL;
	memset(chan_plugin->open_handle, 0, sizeof(chan_plugin->open_handle));
	chan_plugin->num_open_handles = 0;

	/* Add it to the global list */
	list = (rdpChanPluginList *) malloc(sizeof(rdpChanPluginList));
	list->chan_plugin = chan_plugin;

	pthread_mutex_lock(g_mutex);
	list->next = g_chan_plugin_list;
	g_chan_plugin_list = list;
	pthread_mutex_unlock(g_mutex);
}

void
chan_plugin_uninit(rdpChanPlugin * chan_plugin)
{
	rdpChanPluginList * list;
	rdpChanPluginList * prev;

	/* Remove from global list */
	pthread_mutex_lock(g_mutex);
	for (prev = NULL, list = g_chan_plugin_list; list; prev = list, list = list->next)
	{
		if (list->chan_plugin == chan_plugin)
		{
			break;
		}
	}
	if (list)
	{
		if (prev)
		{
			prev->next = list->next;
		}
		else
		{
			g_chan_plugin_list = list->next;
		}
		free(list);
	}
	pthread_mutex_unlock(g_mutex);
}

int
chan_plugin_register_open_handle(rdpChanPlugin * chan_plugin,
	int open_handle)
{
	if (chan_plugin->num_open_handles >= CHANNEL_MAX_COUNT)
	{
		printf("chan_plugin_register_open_handle: too many handles\n");
		return 1;
	}
	chan_plugin->open_handle[chan_plugin->num_open_handles++] = open_handle;
	return 0;
}

int
chan_plugin_unregister_open_handle(rdpChanPlugin * chan_plugin,
	int open_handle)
{
	int lindex;

	for (lindex = 0; lindex < chan_plugin->num_open_handles; lindex++)
	{
		if (chan_plugin->open_handle[lindex] == open_handle)
		{
			chan_plugin->open_handle[lindex] = chan_plugin->open_handle[chan_plugin->num_open_handles - 1];
			chan_plugin->num_open_handles--;
			return 0;
		}
	}
	printf("chan_plugin_unregister_open_handle: open_handle not found\n");
	return 1;
}

rdpChanPlugin *
chan_plugin_find_by_init_handle(void * init_handle)
{
	rdpChanPluginList * list;
	rdpChanPlugin * chan_plugin;

	pthread_mutex_lock(g_mutex);
	for (list = g_chan_plugin_list; list; list = list->next)
	{
		chan_plugin = list->chan_plugin;
		if (chan_plugin->init_handle == init_handle)
		{
			pthread_mutex_unlock(g_mutex);
			return chan_plugin;
		}
	}
	pthread_mutex_unlock(g_mutex);
	return NULL;
}

rdpChanPlugin *
chan_plugin_find_by_open_handle(int open_handle)
{
	rdpChanPluginList * list;
	rdpChanPlugin * chan_plugin;
	int lindex;

	pthread_mutex_lock(g_mutex);
	for (list = g_chan_plugin_list; list; list = list->next)
	{
		chan_plugin = list->chan_plugin;
		for (lindex = 0; lindex < chan_plugin->num_open_handles; lindex++)
		{
			if (chan_plugin->open_handle[lindex] == open_handle)
			{
				pthread_mutex_unlock(g_mutex);
				return chan_plugin;
			}
		}
	}
	pthread_mutex_unlock(g_mutex);
	return NULL;
}

