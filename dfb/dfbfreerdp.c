/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <pthread.h>
#include <freerdp/freerdp.h>
#include <freerdp/chanman.h>
#include "dfbfreerdp.h"
#include "dfb_win.h"
#include "dfb_keyboard.h"

#define MAX_PLUGIN_DATA 20

static int
set_default_params(rdpSet * settings)
{
	memset(settings, 0, sizeof(rdpSet));
	gethostname(settings->hostname, sizeof(settings->hostname) - 1);
	settings->width = 1024;
	settings->height = 768;
	strcpy(settings->server, "127.0.0.1");
	strcpy(settings->username, "guest");
	settings->tcp_port_rdp = 3389;
	settings->encryption = 1;
	settings->server_depth = 32;
	settings->bitmap_cache = 1;
	settings->bitmap_compression = 1;
	settings->desktop_save = 0;
	settings->performanceflags = PERF_DISABLE_FULLWINDOWDRAG | PERF_DISABLE_MENUANIMATIONS | PERF_DISABLE_WALLPAPER;
	settings->off_screen_bitmaps = 1;
	settings->triblt = 0;
	settings->new_cursors = 1;
	settings->rdp_version = 5;
	return 0;
}

static int
process_params(rdpSet * settings, rdpChanMan * chan_man, int argc, char ** argv, int * pindex)
{
	char * p;
	struct passwd * pw;
	RD_PLUGIN_DATA plugin_data[MAX_PLUGIN_DATA + 1];
	int index;
	int i, j;

	set_default_params(settings);
	pw = getpwuid(getuid());
	if (pw != 0)
	{
		if (pw->pw_name != 0)
		{
			strncpy(settings->username, pw->pw_name, sizeof(settings->username) - 1);
		}
	}
	printf("process_params\n");
	if (argc < *pindex + 1)
	{
		if (*pindex == 1)
			printf("no parameters specified\n");
		return 1;
	}
	while (*pindex < argc)
	{
		if (strcmp("-a", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing server depth\n");
				return 1;
			}
			settings->server_depth = atoi(argv[*pindex]);
		}
		else if (strcmp("-u", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing username\n");
				return 1;
			}
			strncpy(settings->username, argv[*pindex], sizeof(settings->username) - 1);
			settings->username[sizeof(settings->username) - 1] = 0;
		}
		else if (strcmp("-p", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing password\n");
				return 1;
			}
			strncpy(settings->password, argv[*pindex], sizeof(settings->password) - 1);
			settings->password[sizeof(settings->password) - 1] = 0;
			settings->autologin = 1;
		}
		else if (strcmp("-d", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing domain\n");
				return 1;
			}
			strncpy(settings->domain, argv[*pindex], sizeof(settings->domain) - 1);
			settings->domain[sizeof(settings->domain) - 1] = 0;
		}
		else if (strcmp("-s", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing shell\n");
				return 1;
			}
			strncpy(settings->shell, argv[*pindex], sizeof(settings->shell) - 1);
			settings->shell[sizeof(settings->shell) - 1] = 0;
		}
		else if (strcmp("-c", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing directory\n");
				return 1;
			}
			strncpy(settings->directory, argv[*pindex], sizeof(settings->directory) - 1);
			settings->directory[sizeof(settings->directory) - 1] = 0;
		}
		else if (strcmp("-g", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing width\n");
				return 1;
			}
			settings->width = strtol(argv[*pindex], &p, 10);
			if (*p == 'x')
			{
				settings->height = strtol(p + 1, &p, 10);
			}
			if ((settings->width < 16) || (settings->height < 16) ||
				(settings->width > 4096) || (settings->height > 4096))
			{
				printf("invalid dimensions\n");
				return 1;
			}
		}
		else if (strcmp("-t", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing port number\n");
				return 1;
			}
			settings->tcp_port_rdp = atoi(argv[*pindex]);
		}
		else if (strcmp("-n", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing hostname\n");
				return 1;
			}
			strncpy(settings->hostname, argv[*pindex], sizeof(settings->hostname) - 1);
			settings->directory[sizeof(settings->hostname) - 1] = 0;
		}
		else if (strcmp("-o", argv[*pindex]) == 0)
		{
			settings->console_audio = 1;
		}
		else if (strcmp("-0", argv[*pindex]) == 0)
		{
			settings->console_session = 1;
		}
		else if (strcmp("-z", argv[*pindex]) == 0)
		{
			settings->bulk_compression = 1;
		}
		else if (strcmp("-x", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing performance flag\n");
				return 1;
			}
			if (strncmp("m", argv[*pindex], 1) == 0) /* modem */
			{
				settings->performanceflags = PERF_DISABLE_WALLPAPER |
					PERF_DISABLE_FULLWINDOWDRAG | PERF_DISABLE_MENUANIMATIONS |
					PERF_DISABLE_THEMING;
			}
			else if (strncmp("b", argv[*pindex], 1) == 0) /* broadband */
			{
				settings->performanceflags = PERF_DISABLE_WALLPAPER;
			}
			else if (strncmp("l", argv[*pindex], 1) == 0) /* lan */
			{
				settings->performanceflags = PERF_FLAG_NONE;
			}
			else
			{
				settings->performanceflags = strtol(argv[*pindex], 0, 16);
			}
		}
		else if (strcmp("-plugin", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing plugin name\n");
				return 1;
			}
			index = *pindex;
			memset(plugin_data, 0, sizeof(plugin_data));
			if (*pindex < argc - 1 && strcmp("-data", argv[*pindex + 1]) == 0)
			{
				*pindex = *pindex + 2;
				i = 0;
				while (*pindex < argc && strcmp("--", argv[*pindex]) != 0 && i < MAX_PLUGIN_DATA)
				{
					plugin_data[i].size = sizeof(RD_PLUGIN_DATA);
					for (j = 0, p = argv[*pindex]; j < 4 && p != NULL; j++)
					{
						plugin_data[i].data[j] = p;
						p = strchr(plugin_data[i].data[j], ':');
						if (p != NULL)
							*p++ = 0;
					}
					*pindex = *pindex + 1;
					i++;
				}
			}
			freerdp_chanman_load_plugin(chan_man, settings, argv[index], plugin_data);
		}
		else
		{
			strncpy(settings->server, argv[*pindex], sizeof(settings->server) - 1);
			settings->server[sizeof(settings->server) - 1] = 0;
			p = strchr(settings->server, ':');
			if (p)
			{
				*p = 0;
				settings->tcp_port_rdp = atoi(p + 1);
			}
			/* server is the last argument for the current session. arguments
			   followed will be parsed for the next session. */
			*pindex = *pindex + 1;
			return 0;
		}
		*pindex = *pindex + 1;
	}
	printf("missing server name\n");
	return 1;
}

static void *
graphics_update_thread(void * arg)
{
	rdpInst *inst = (rdpInst*) arg;
	dfbInfo *dfbi = GET_DFBI(inst);

	int interval = 400;
	clock_t prev = clock();
	clock_t curr = clock();

	int clocksPerMS = CLOCKS_PER_SEC * 1000;

	while (1)
	{
		usleep(interval);

		if (dfbi->hwnd->dirty)
		{
			curr = clock();

			if ((curr - prev) / clocksPerMS > 2 * interval)
			{				
				prev = curr;
				inst->ui_begin_update(inst);
			}
		}
	}

	pthread_detach(pthread_self());
	return NULL;
}

static int
run_dfbfreerdp(rdpSet * settings, rdpChanMan * chan_man)
{
	rdpInst * inst;
	pthread_t thread;
	void * dfb_info;
	void * read_fds[32];
	void * write_fds[32];
	int read_count;
	int write_count;
	int index;
	int sck;
	int max_sck;
	fd_set rfds;
	fd_set wfds;

	printf("run_dfbfreerdp:\n");
	/* create an instance of the library */
	inst = freerdp_new(settings);
	if (inst == NULL)
	{
		printf("run_dfbfreerdp: freerdp_new failed\n");
		return 1;
	}
	if ((inst->version != FREERDP_INTERFACE_VERSION) ||
	    (inst->size != sizeof(rdpInst)))
	{
		printf("run_dfbfreerdp: freerdp_new size, version / size do not "
		       "match expecting v %d s %d got v %d s %d\n",
		       FREERDP_INTERFACE_VERSION, (int)sizeof(rdpInst),
		       inst->version, inst->size);
		return 1;
	}
	if (dfb_pre_connect(inst) != 0)
	{
		printf("run_dfbfreerdp: dfb_pre_connect failed\n");
		return 1;
	}
	if (freerdp_chanman_pre_connect(chan_man, inst) != 0)
	{
		printf("run_dfbfreerdp: freerdp_chanman_pre_connect failed\n");
		return 1;
	}
	/* call connect */
	printf("keyboard_layout: %X\n", inst->settings->keyboard_layout);
	if (inst->rdp_connect(inst) != 0)
	{
		printf("run_dfbfreerdp: inst->rdp_connect failed\n");
		return 1;
	}
	if (freerdp_chanman_post_connect(chan_man, inst) != 0)
	{
		printf("run_dfbfreerdp: freerdp_chanman_post_connect failed\n");
		return 1;
	}
	if (dfb_post_connect(inst) != 0)
	{
		printf("run_dfbfreerdp: dfb_post_connect failed\n");
		return 1;
	}
	pthread_create(&thread, 0, graphics_update_thread, (void*) inst);
	
	/* program main loop */
	while (1)
	{
		read_count = 0;
		write_count = 0;
		/* get libfreerdp fds */
		if (inst->rdp_get_fds(inst, read_fds, &read_count, write_fds, &write_count) != 0)
		{
			printf("run_dfbfreerdp: inst->rdp_get_fds failed\n");
			break;
		}
		/* get DirectFB fds */
		if (dfb_get_fds(inst, read_fds, &read_count, write_fds, &write_count) != 0)
		{
			printf("run_dfbfreerdp: dfb_get_fds failed\n");
			break;
		}
		/* get channel fds */
		if (freerdp_chanman_get_fds(chan_man, inst, read_fds, &read_count, write_fds, &write_count) != 0)
		{
			printf("run_dfbfreerdp: freerdp_chanman_get_fds failed\n");
			break;
		}
		max_sck = 0;
		/* setup read fds */
		FD_ZERO(&rfds);
		for (index = 0; index < read_count; index++)
		{
			sck = (int)(long) (read_fds[index]);
			if (sck > max_sck)
				max_sck = sck;
			FD_SET(sck, &rfds);
		}
		/* setup write fds */
		FD_ZERO(&wfds);
		for (index = 0; index < write_count; index++)
		{
			sck = (int)(long) (write_fds[index]);
			if (sck > max_sck)
				max_sck = sck;
			FD_SET(sck, &wfds);
		}
		/* exit if nothing to do */
		if (max_sck == 0)
		{
			printf("run_dfbfreerdp: max_sck is zero\n");
			break;
		}
		/* do the wait */
		if (select(max_sck + 1, &rfds, &wfds, NULL, NULL) == -1)
		{
			/* these are not really errors */
			if (!((errno == EAGAIN) ||
				(errno == EWOULDBLOCK) ||
				(errno == EINPROGRESS) ||
				(errno == EINTR))) /* signal occurred */
			{
				printf("run_dfbfreerdp: select failed\n");
				break;
			}
		}
		/* check the libfreerdp fds */
		if (inst->rdp_check_fds(inst) != 0)
		{
			printf("run_dfbfreerdp: inst->rdp_check_fds failed\n");
			break;
		}
		/* check DirectFB fds */
		if (dfb_check_fds(inst) != 0)
		{
			printf("run_dfbfreerdp: dfb_check_fds failed\n");
			break;
		}
		/* check channel fds */
		if (freerdp_chanman_check_fds(chan_man, inst) != 0)
		{
			printf("run_dfbfreerdp: freerdp_chanman_check_fds failed\n");
			break;
		}
	}
	/* cleanup */
	dfb_info = inst->param1;
	inst->rdp_disconnect(inst);
	freerdp_free(inst);
	dfb_uninit(dfb_info);
	return 0;
}

static int g_thread_count = 0;

struct thread_data
{
	rdpInst * inst;
	rdpSet * settings;
	rdpChanMan * chan_man;
};

static void *
thread_func(void * arg)
{
	struct thread_data * data;
	data = (struct thread_data *) arg;
	
	run_dfbfreerdp(data->settings, data->chan_man);
	
	free(data->settings);
	freerdp_chanman_free(data->chan_man);
	free(data);

	pthread_detach(pthread_self());
	g_thread_count--;

	return NULL;
}

int
main(int argc, char ** argv)
{
	struct thread_data * data;
	int rv;
	pthread_t thread;
	int index = 1;

	setlocale(LC_CTYPE, "");
	freerdp_chanman_init();
	dfb_init(&argc, &argv);
	dfb_kb_init();

	while (1)
	{
		data = (struct thread_data *) malloc(sizeof(struct thread_data));
		data->settings = (rdpSet *) malloc(sizeof(rdpSet));
		data->chan_man = freerdp_chanman_new();
		rv = process_params(data->settings, data->chan_man, argc, argv, &index);
		if (rv == 0)
		{
			g_thread_count++;
			printf("starting thread %d to %s:%d\n", g_thread_count,
				data->settings->server, data->settings->tcp_port_rdp);
			pthread_create(&thread, 0, thread_func, data);
		}
		else
		{
			free(data->settings);
			freerdp_chanman_free(data->chan_man);
			free(data);
			break;
		}
	}

	while (g_thread_count > 0)
	{
		sleep(1);
	}

	freerdp_chanman_uninit();
	return 0;
}

