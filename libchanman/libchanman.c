/*
   Copyright (c) 2009-2010 Jay Sorg
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

   MS compatible plugin interface
   reference:
   http://msdn.microsoft.com/en-us/library/aa383580(VS.85).aspx

   Notes on threads:
   Many virtual channel plugins are built using threads.
   Non main threads may call MyVirtualChannelOpen,
   MyVirtualChannelClose, or MyVirtualChannelWrite.
   Since the plugin's VirtualChannelEntry function is called
   from the main thread, MyVirtualChannelInit has to be called
   from the main thread.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <semaphore.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include "freerdp.h"
#include "libchanman.h"
#include "vchan.h"

/* The current channel manager reference passes from VirtualChannelEntry to
   VirtualChannelInit for the pInitHandle. */
static rdpChanMan * g_init_chan_man;

/* The list of all channel managers. */
typedef struct rdp_chan_man_list rdpChanManList;
struct rdp_chan_man_list
{
	rdpChanMan * chan_man;
	rdpChanManList * next;
};

static rdpChanManList * g_chan_man_list;

/* To generate unique sequence for all open handles */
static int g_open_handle_sequence;

/* For locking the global resources */
static pthread_mutex_t * g_mutex_init;
static pthread_mutex_t * g_mutex_list;

/* The channel manager stuff */
struct lib_data
{
	void * han; /* returned from dlopen */
	PVIRTUALCHANNELENTRY entry; /* the one and only exported function */
	PCHANNEL_INIT_EVENT_FN init_event_proc;
};

struct chan_data
{
	char name[CHANNEL_NAME_LEN + 1];
	int open_handle;
	int options;
	int flags; /* 0 nothing 1 init 2 open */
	PCHANNEL_OPEN_EVENT_FN open_event_proc;
};

struct rdp_chan_man
{
	/* Only the main thread alters these arrays, before any
	   library thread is allowed in(post_connect is called)
	   so no need to use mutex locking
	   After post_connect, each library thread can only access it's
	   own array items
	   ie, no two threads can access index 0, ... */
	struct lib_data libs[CHANNEL_MAX_COUNT];
	int num_libs;
	struct chan_data chans[CHANNEL_MAX_COUNT];
	int num_chans;

	/* control for entry into MyVirtualChannelInit */
	int can_call_init;
	rdpSet * settings;

	/* true once chan_man_post_connect is called */
	int is_connected;

	/* used for locating the chan_man for a given instance */
	rdpInst * inst;

	/* used for sync write */
	sem_t * sem;
	int pipe_fd[2];

	void * sync_data;
	uint32 sync_data_length;
	void * sync_user_data;
	int sync_index;
};

/* returns the chan_man for the open handle passed in */
static rdpChanMan *
chan_man_find_by_open_handle(int open_handle, int * pindex)
{
	rdpChanManList * list;
	rdpChanMan * chan_man;
	int lindex;

	pthread_mutex_lock(g_mutex_list);
	for (list = g_chan_man_list; list; list = list->next)
	{
		chan_man = list->chan_man;
		for (lindex = 0; lindex < chan_man->num_chans; lindex++)
		{
			if (chan_man->chans[lindex].open_handle == open_handle)
			{
				pthread_mutex_unlock(g_mutex_list);
				*pindex = lindex;
				return chan_man;
			}
		}
	}
	pthread_mutex_unlock(g_mutex_list);
	return NULL;
}

/* returns the chan_man for the rdp instance passed in */
static rdpChanMan *
chan_man_find_by_rdp_inst(rdpInst * inst)
{
	rdpChanManList * list;
	rdpChanMan * chan_man;

	pthread_mutex_lock(g_mutex_list);
	for (list = g_chan_man_list; list; list = list->next)
	{
		chan_man = list->chan_man;
		if (chan_man->inst == inst)
		{
			pthread_mutex_unlock(g_mutex_list);
			return chan_man;
		}
	}
	pthread_mutex_unlock(g_mutex_list);
	return NULL;
}

/* returns struct chan_data for the channel name passed in */
static struct chan_data *
chan_man_find_chan_data_by_name(rdpChanMan * chan_man, const char * chan_name,
	int * pindex)
{
	int lindex;
	struct chan_data * lchan_data;

	for (lindex = 0; lindex < chan_man->num_chans; lindex++)
	{
		lchan_data = chan_man->chans + lindex;
		if (strcmp(chan_name, lchan_data->name) == 0)
		{
			if (pindex != 0)
			{
				*pindex = lindex;
			}
			return lchan_data;
		}
	}
	return 0;
}

/* returns struct rdp_chan for the channel id passed in */
static struct rdp_chan *
chan_man_find_rdp_chan_by_id(rdpChanMan * chan_man, rdpSet * settings,
	int chan_id, int * pindex)
{
	int lindex;
	int lcount;
	struct rdp_chan * lrdp_chan;

	lcount = settings->num_channels;
	for (lindex = 0; lindex < lcount; lindex++)
	{
		lrdp_chan = settings->channels + lindex;
		if (lrdp_chan->chan_id == chan_id)
		{
			if (pindex != 0)
			{
				*pindex = lindex;
			}
			return lrdp_chan;
		}
	}
	return 0;
}

/* returns struct rdp_chan for the channel name passed in */
static struct rdp_chan *
chan_man_find_rdp_chan_by_name(rdpChanMan * chan_man, rdpSet * settings,
	const char * chan_name, int * pindex)
{
	int lindex;
	int lcount;
	struct rdp_chan * lrdp_chan;

	lcount = settings->num_channels;
	for (lindex = 0; lindex < lcount; lindex++)
	{
		lrdp_chan = settings->channels + lindex;
		if (strcmp(chan_name, lrdp_chan->name) == 0)
		{
			if (pindex != 0)
			{
				*pindex = lindex;
			}
			return lrdp_chan;
		}
	}
	return 0;
}

/* must be called by same thread that calls chan_man_load_plugin
   according to MS docs
   only called from main thread */
static uint32
MyVirtualChannelInit(void ** ppInitHandle, PCHANNEL_DEF pChannel,
	int channelCount, uint32 versionRequested,
	PCHANNEL_INIT_EVENT_FN pChannelInitEventProc)
{
	rdpChanMan * chan_man;
	int index;
	struct lib_data * llib;
	struct chan_data * lchan;
	struct rdp_chan * lrdp_chan;
	PCHANNEL_DEF lchan_def;

	chan_man = g_init_chan_man;
	*ppInitHandle = chan_man;

	printf("MyVirtualChannelInit:\n");
	if (!chan_man->can_call_init)
	{
		printf("MyVirtualChannelInit: error not in entry\n");
		return CHANNEL_RC_NOT_IN_VIRTUALCHANNELENTRY;
	}
	if (ppInitHandle == 0)
	{
		printf("MyVirtualChannelInit: error bad pphan\n");
		return CHANNEL_RC_BAD_INIT_HANDLE;
	}
	if (chan_man->num_chans + channelCount >= CHANNEL_MAX_COUNT)
	{
		printf("MyVirtualChannelInit: error too many channels\n");
		return CHANNEL_RC_TOO_MANY_CHANNELS;
	}
	if (pChannel == 0)
	{
		printf("MyVirtualChannelInit: error bad pchan\n");
		return CHANNEL_RC_BAD_CHANNEL;
	}
	if (chan_man->is_connected)
	{
		printf("MyVirtualChannelInit: error already connected\n");
		return CHANNEL_RC_ALREADY_CONNECTED;
	}
	if (versionRequested != VIRTUAL_CHANNEL_VERSION_WIN2000)
	{
		printf("MyVirtualChannelInit: warning version\n");
	}
	for (index = 0; index < channelCount; index++)
	{
		lchan_def = pChannel + index;
		if (chan_man_find_chan_data_by_name(chan_man, lchan_def->name, 0) != 0)
		{
			printf("MyVirtualChannelInit: error channel already used\n");
			return CHANNEL_RC_BAD_CHANNEL;
		}
	}
	llib = chan_man->libs + chan_man->num_libs;
	llib->init_event_proc = pChannelInitEventProc;
	chan_man->num_libs++;
	for (index = 0; index < channelCount; index++)
	{
		lchan_def = pChannel + index;
		lchan = chan_man->chans + chan_man->num_chans;

		pthread_mutex_lock(g_mutex_list);
		lchan->open_handle = g_open_handle_sequence++;
		pthread_mutex_unlock(g_mutex_list);

		lchan->flags = 1; /* init */
		strncpy(lchan->name, lchan_def->name, CHANNEL_NAME_LEN);
		lchan->options = lchan_def->options;
		if (chan_man->settings->num_channels < 16)
		{
			lrdp_chan = chan_man->settings->channels + chan_man->settings->num_channels;
			strncpy(lrdp_chan->name, lchan_def->name, 7);
			lrdp_chan->flags = lchan_def->options;
			chan_man->settings->num_channels++;
		}
		else
		{
			printf("MyVirtualChannelInit: warning more than 16 channels\n");
		}
		chan_man->num_chans++;
	}
	return CHANNEL_RC_OK;
}

/* can be called from any thread
   thread safe because no 2 threads can have the same channel name registered */
static uint32
MyVirtualChannelOpen(void * pInitHandle, uint32 * pOpenHandle,
	char * pChannelName, PCHANNEL_OPEN_EVENT_FN pChannelOpenEventProc)
{
	rdpChanMan * chan_man;
	int index;
	struct chan_data * lchan;

	printf("MyVirtualChannelOpen:\n");
	chan_man = (rdpChanMan *) pInitHandle;
	if (pOpenHandle == 0)
	{
		printf("MyVirtualChannelOpen: error bad chanhan\n");
		return CHANNEL_RC_BAD_CHANNEL_HANDLE;
	}
	if (pChannelOpenEventProc == 0)
	{
		printf("MyVirtualChannelOpen: error bad proc\n");
		return CHANNEL_RC_BAD_PROC;
	}
	if (!chan_man->is_connected)
	{
		printf("MyVirtualChannelOpen: error not connected\n");
		return CHANNEL_RC_NOT_CONNECTED;
	}
	lchan = chan_man_find_chan_data_by_name(chan_man, pChannelName, &index);
	if (lchan == 0)
	{
		printf("MyVirtualChannelOpen: error chan name\n");
		return CHANNEL_RC_UNKNOWN_CHANNEL_NAME;
	}
	if (lchan->flags == 2)
	{
		printf("MyVirtualChannelOpen: error chan already open\n");
		return CHANNEL_RC_ALREADY_OPEN;
	}

	lchan->flags = 2; /* open */
	lchan->open_event_proc = pChannelOpenEventProc;
	*pOpenHandle = lchan->open_handle;
	return CHANNEL_RC_OK;
}

/* can be called from any thread
   thread safe because no 2 threads can have the same openHandle */
static uint32
MyVirtualChannelClose(uint32 openHandle)
{
	rdpChanMan * chan_man;
	struct chan_data * lchan;
	int index;

	chan_man = chan_man_find_by_open_handle(openHandle, &index);
	if ((chan_man == NULL) || (index < 0) || (index >= CHANNEL_MAX_COUNT))
	{
		printf("MyVirtualChannelClose: error bad chanhan\n");
		return CHANNEL_RC_BAD_CHANNEL_HANDLE;
	}
	if (!chan_man->is_connected)
	{
		printf("MyVirtualChannelClose: error not connected\n");
		return CHANNEL_RC_NOT_CONNECTED;
	}
	lchan = chan_man->chans + index;
	if (lchan->flags != 2)
	{
		printf("MyVirtualChannelClose: error not open\n");
		return CHANNEL_RC_NOT_OPEN;
	}
	lchan->flags = 0;
	return CHANNEL_RC_OK;
}

static int
chan_man_is_ev_set(rdpChanMan * chan_man)
{
	fd_set rfds;
	int num_set;
	struct timeval time;

	FD_ZERO(&rfds);
	FD_SET(chan_man->pipe_fd[0], &rfds);
	memset(&time, 0, sizeof(time));
	num_set = select(chan_man->pipe_fd[0] + 1, &rfds, 0, 0, &time);
	return (num_set == 1);
}

static void
chan_man_set_ev(rdpChanMan * chan_man)
{
	int len;

	if (chan_man_is_ev_set(chan_man))
	{
		return;
	}
	len = write(chan_man->pipe_fd[1], "sig", 4);
	if (len != 4)
	{
		printf("chan_man_set_ev: error\n");
	}
}

static void
chan_man_clear_ev(rdpChanMan * chan_man)
{
	int len;

	while (chan_man_is_ev_set(chan_man))
	{
		len = read(chan_man->pipe_fd[0], &len, 4);
		if (len != 4)
		{
			printf("chan_man_clear_ev: error\n");
		}
	}
}

/* can be called from any thread */
static uint32
MyVirtualChannelWrite(uint32 openHandle, void * pData, uint32 dataLength,
	void * pUserData)
{
	rdpChanMan * chan_man;
	struct chan_data * lchan;
	int index;

	chan_man = chan_man_find_by_open_handle(openHandle, &index);
	if ((chan_man == NULL) || (index < 0) || (index >= CHANNEL_MAX_COUNT))
	{
		printf("MyVirtualChannelWrite: error bad chanhan\n");
		return CHANNEL_RC_BAD_CHANNEL_HANDLE;
	}
	if (!chan_man->is_connected)
	{
		printf("MyVirtualChannelWrite: error not connected\n");
		return CHANNEL_RC_NOT_CONNECTED;
	}
	if (pData == 0)
	{
		printf("MyVirtualChannelWrite: error bad pData\n");
		return CHANNEL_RC_NULL_DATA;
	}
	if (dataLength == 0)
	{
		printf("MyVirtualChannelWrite: error bad dataLength\n");
		return CHANNEL_RC_ZERO_LENGTH;
	}
	lchan = chan_man->chans + index;
	if (lchan->flags != 2)
	{
		printf("MyVirtualChannelWrite: error not open\n");
		return CHANNEL_RC_NOT_OPEN;
	}
	sem_wait(chan_man->sem); /* lock chan_man->sync* vars */
	chan_man->sync_data = pData;
	chan_man->sync_data_length = dataLength;
	chan_man->sync_user_data = pUserData;
	chan_man->sync_index = index;
	/* set the event */
	chan_man_set_ev(chan_man);
	return CHANNEL_RC_OK;
}

/* this is called shortly after the application starts and
   before any other function in the file
   called only from main thread */
int
chan_man_init(void)
{
	g_init_chan_man = NULL;
	g_chan_man_list = NULL;
	g_open_handle_sequence = 1;
	g_mutex_init = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(g_mutex_init, 0);
	g_mutex_list = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(g_mutex_list, 0);

	return 0;
}

int
chan_man_uninit(void)
{
	while (g_chan_man_list)
	{
		chan_man_free(g_chan_man_list->chan_man);
	}

	pthread_mutex_destroy(g_mutex_init);
	free(g_mutex_init);
	pthread_mutex_destroy(g_mutex_list);
	free(g_mutex_list);

	return 0;
}

rdpChanMan *
chan_man_new(void)
{
	rdpChanMan * chan_man;
	rdpChanManList * list;

	chan_man = (rdpChanMan *) malloc(sizeof(rdpChanMan));
	memset(chan_man, 0, sizeof(rdpChanMan));

	chan_man->sem = (sem_t *) malloc(sizeof(sem_t));
	memset(chan_man->sem, 0, sizeof(sem_t));
	sem_init(chan_man->sem, 0, 1); /* start at 1 */
	chan_man->pipe_fd[0] = -1;
	chan_man->pipe_fd[1] = -1;
	if (pipe(chan_man->pipe_fd) < 0)
	{
		printf("chan_man_init: pipe failed\n");
	}

	/* Add it to the global list */
	list = (rdpChanManList *) malloc(sizeof(rdpChanManList));
	list->chan_man = chan_man;

	pthread_mutex_lock(g_mutex_list);
	list->next = g_chan_man_list;
	g_chan_man_list = list;
	pthread_mutex_unlock(g_mutex_list);

	return chan_man;
}

void
chan_man_free(rdpChanMan * chan_man)
{
	rdpChanManList * list;
	rdpChanManList * prev;
	int index;
	struct lib_data * llib;

	/* tell all libraries we are shutting down */
	for (index = 0; index < chan_man->num_libs; index++)
	{
		llib = chan_man->libs + index;
		if (llib->init_event_proc != 0)
		{
			llib->init_event_proc(chan_man, CHANNEL_EVENT_TERMINATED,
				0, 0);
		}
	}
	sem_destroy(chan_man->sem);
	free(chan_man->sem);
	if (chan_man->pipe_fd[0] != -1)
	{
		close(chan_man->pipe_fd[0]);
		chan_man->pipe_fd[0] = -1;
	}
	if (chan_man->pipe_fd[1] != -1)
	{
		close(chan_man->pipe_fd[1]);
		chan_man->pipe_fd[1] = -1;
	}

	/* Remove from global list */
	pthread_mutex_lock(g_mutex_list);
	for (prev = NULL, list = g_chan_man_list; list; prev = list, list = list->next)
	{
		if (list->chan_man == chan_man)
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
			g_chan_man_list = list->next;
		}
		free(list);
	}
	pthread_mutex_unlock(g_mutex_list);

	free(chan_man);
}

/* this is called when processing the command line parameters
   called only from main thread */
int
chan_man_load_plugin(rdpChanMan * chan_man, rdpSet * settings,
	const char * filename)
{
	struct lib_data * lib;
	CHANNEL_ENTRY_POINTS ep;
	int ok;

	printf("chan_man_load_plugin: filename %s\n", filename);
	if (chan_man->num_libs + 1 >= CHANNEL_MAX_COUNT)
	{
		printf("chan_man_load_plugin: too many channels\n");
		return 1;
	}
	lib = chan_man->libs + chan_man->num_libs;
	lib->han = dlopen(filename, RTLD_LOCAL | RTLD_LAZY);
	if (lib->han == 0)
	{
		printf("chan_man_load_plugin: failed to load library\n");
		return 1;
	}
	lib->entry = (PVIRTUALCHANNELENTRY)
		dlsym(lib->han, CHANNEL_EXPORT_FUNC_NAME);
	if (lib->entry == 0)
	{
		printf("chan_man_load_plugin: failed to find export function\n");
		dlclose(lib->han);
		return 1;
	}
	ep.cbSize = sizeof(ep);
	ep.protocolVersion = VIRTUAL_CHANNEL_VERSION_WIN2000;
	ep.pVirtualChannelInit = MyVirtualChannelInit;
	ep.pVirtualChannelOpen = MyVirtualChannelOpen;
	ep.pVirtualChannelClose = MyVirtualChannelClose;
	ep.pVirtualChannelWrite = MyVirtualChannelWrite;

	/* enable MyVirtualChannelInit */
	chan_man->can_call_init = 1;
	chan_man->settings = settings;

	pthread_mutex_lock(g_mutex_init);
	g_init_chan_man = chan_man;
	ok = lib->entry(&ep);
	g_init_chan_man = NULL;
	pthread_mutex_unlock(g_mutex_init);

	/* disable MyVirtualChannelInit */
	chan_man->settings = 0;
	chan_man->can_call_init = 0;
	if (!ok)
	{
		printf("chan_man_load_plugin: export function call failed\n");
		dlclose(lib->han);
		return 1;
	}
	return 0;
}

/* go through and inform all the libraries that we are initialized
   called only from main thread */
int
chan_man_pre_connect(rdpChanMan * chan_man, rdpInst * inst)
{
	int index;
	struct lib_data * llib;
	CHANNEL_DEF lchannel_def;
	void * dummy;

	printf("chan_man_pre_connect:\n");
	chan_man->inst = inst;

	/* If rdpsnd is registered but not rdpdr, it's necessary to register a fake
	   rdpdr channel to make sound work. This is a workaround for Window 7 and
	   Windows 2008 */
	if (chan_man_find_chan_data_by_name(chan_man, "rdpsnd", 0) != 0 &&
		chan_man_find_chan_data_by_name(chan_man, "rdpdr", 0) == 0)
	{
		lchannel_def.options = CHANNEL_OPTION_INITIALIZED |
			CHANNEL_OPTION_ENCRYPT_RDP;
		strcpy(lchannel_def.name, "rdpdr");
		chan_man->can_call_init = 1;
		chan_man->settings = inst->settings;
		pthread_mutex_lock(g_mutex_init);
		g_init_chan_man = chan_man;
		MyVirtualChannelInit(&dummy, &lchannel_def, 1,
			VIRTUAL_CHANNEL_VERSION_WIN2000, 0);
		g_init_chan_man = NULL;
		pthread_mutex_unlock(g_mutex_init);
		chan_man->can_call_init = 0;
		chan_man->settings = 0;
		printf("chan_man_pre_connect: registered fake rdpdr for rdpsnd.\n");
	}

	for (index = 0; index < chan_man->num_libs; index++)
	{
		llib = chan_man->libs + index;
		if (llib->init_event_proc != 0)
		{
			llib->init_event_proc(chan_man, CHANNEL_EVENT_INITIALIZED,
				0, 0);
		}
	}
	return 0;
}

/* go through and inform all the libraries that we are connected
   this will tell the libraries that its ok to call MyVirtualChannelOpen
   called only from main thread */
int
chan_man_post_connect(rdpChanMan * chan_man, rdpInst * inst)
{
	int index;
	int server_name_len;
	struct lib_data * llib;
	char * server_name;

	chan_man->is_connected = 1;
	server_name = inst->settings->server;
	server_name_len = strlen(server_name);
	printf("chan_man_post_connect: server name [%s] chan_man->num_libs [%d]\n",
		server_name, chan_man->num_libs);
	for (index = 0; index < chan_man->num_libs; index++)
	{
		llib = chan_man->libs + index;
		if (llib->init_event_proc != 0)
		{
			llib->init_event_proc(chan_man, CHANNEL_EVENT_CONNECTED,
				server_name, server_name_len);
		}
	}
	return 0;
}

/* data comming from the server to the client
   called only from main thread */
int
chan_man_data(rdpInst * inst, int chan_id, char * data, int data_size,
	int flags, int total_size)
{
	rdpChanMan * chan_man;
	struct rdp_chan * lrdp_chan;
	struct chan_data * lchan_data;
	int index;

	chan_man = chan_man_find_by_rdp_inst(inst);
	if (chan_man == 0)
	{
		printf("chan_man_data: could not find channel manager\n");
		return 1;
	}

	//printf("chan_man_data:\n");
	lrdp_chan = chan_man_find_rdp_chan_by_id(chan_man, inst->settings,
		chan_id, &index);
	if (lrdp_chan == 0)
	{
		printf("chan_man_data: could not find channel id\n");
		return 1;
	}
	lchan_data = chan_man_find_chan_data_by_name(chan_man, lrdp_chan->name,
		&index);
	if (lchan_data == 0)
	{
		printf("chan_man_data: could not find channel name\n");
		return 1;
	}
	if (lchan_data->open_event_proc != 0)
	{
		lchan_data->open_event_proc(lchan_data->open_handle,
			CHANNEL_EVENT_DATA_RECEIVED,
			data, data_size, total_size, flags);
	}
	return 0;
}

/* called only from main thread */
static void
chan_man_process_sync(rdpChanMan * chan_man, rdpInst * inst)
{
	void * ldata;
	uint32 ldata_len;
	void * luser_data;
	int lindex;
	struct chan_data * lchan_data;
	struct rdp_chan * lrdp_chan;

	ldata = chan_man->sync_data;
	ldata_len = chan_man->sync_data_length;
	luser_data = chan_man->sync_user_data;
	lindex = chan_man->sync_index;
	sem_post(chan_man->sem); /* release chan_man->sync* vars */
	lchan_data = chan_man->chans + lindex;
	lrdp_chan = chan_man_find_rdp_chan_by_name(chan_man, inst->settings,
		lchan_data->name, &lindex);
	if (lrdp_chan != 0)
	{
		inst->rdp_channel_data(inst, lrdp_chan->chan_id, ldata, ldata_len);
	}
	if (lchan_data->open_event_proc != 0)
	{
		lchan_data->open_event_proc(lchan_data->open_handle,
			CHANNEL_EVENT_WRITE_COMPLETE,
			luser_data, sizeof(void *), sizeof(void *), 0);
	}
}

/* called only from main thread */
int
chan_man_get_fds(rdpChanMan * chan_man, rdpInst * inst, void ** read_fds,
	int * read_count, void ** write_fds, int * write_count)
{
	if (chan_man->pipe_fd[0] == -1)
	{
		return 0;
	}
	read_fds[*read_count] = (void *) chan_man->pipe_fd[0];
	(*read_count)++;
	return 0;
}

/* called only from main thread */
int
chan_man_check_fds(rdpChanMan * chan_man, rdpInst * inst)
{
	if (chan_man->pipe_fd[0] == -1)
	{
		return 0;
	}
	if (chan_man_is_ev_set(chan_man))
	{
		//printf("chan_man_check_fds: 1\n");
		chan_man_clear_ev(chan_man);
		chan_man_process_sync(chan_man, inst);
	}
	return 0;
}
