/*
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI

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
#include <semaphore.h>
#include <freerdp/utils/semaphore.h>
#include <freerdp/freerdp.h>
#include <freerdp/chanman.h>
#include <freerdp/utils/memory.h>
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
	settings->mouse_motion = 1;
	settings->off_screen_bitmaps = 1;
	settings->triblt = 0;
	settings->software_gdi = 1;
	settings->new_cursors = 1;
	settings->rdp_version = 5;
	settings->rdp_security = 1;
#ifndef DISABLE_TLS
	settings->tls_security = 1;
	settings->nla_security = 1;
#endif
	return 0;
}

static int
out_args(void)
{
	char help[] =
		"\n"
		"FreeRDP - A Free Remote Desktop Protocol Client\n"
		"See www.freerdp.com for more information\n"
		"\n"
		"Usage: dfbfreerdp [options] server:port\n"
		"\t-a: color depth (8, 15, 16, 24 or 32)\n"
		"\t-u: username\n"
		"\t-p: password\n"
		"\t-d: domain\n"
		"\t-k: keyboard layout ID\n"
		"\t--kbd-list: list all keyboard layout IDs\n"
		"\t-s: shell\n"
		"\t-c: directory\n"
		"\t-g: geometry, using format WxH or X%, default is 1024x768\n"
		"\t-t: alternative port number, default is 3389\n"
		"\t-n: hostname\n"
		"\t-o: console audio\n"
		"\t-0: console session\n"
		"\t-f: fullscreen mode\n"
		"\t-D: hide window decorations\n"
		"\t-z: enable bulk compression\n"
		"\t--gdi: GDI rendering (sw or hw, for software or hardware)\n"
		"\t-x: performance flags (m, b or l for modem, broadband or lan)\n"
		"\t-X: embed into another window with a given XID.\n"
#ifndef DISABLE_TLS
		"\t--no-rdp: disable Standard RDP encryption\n"
		"\t--no-tls: disable TLS encryption\n"
		"\t--no-nla: disable network level authentication\n"
		"\t--sec: force protocol security (rdp, tls or nla)\n"
#endif
		"\t--plugin: load a virtual channel plugin\n"
		"\t--no-osb: disable off screen bitmaps, default on\n"
		"\t--rfx: ask for RemoteFX session\n"
#ifdef HAVE_XV
		"\t--xv-port: choose XVideo adaptor port number.\n"
#endif
		"\t--version: Print out the version and exit\n"
		"\t-h: show this help\n";
	printf("%s\n", help);
	return 0;
}

static int
process_params(rdpSet * settings, rdpChanMan * chan_man, int argc, char ** argv, int * pindex)
{
	int index;
	int i, j;
	char * p;
	struct passwd * pw;
	int num_extensions;
	RD_PLUGIN_DATA plugin_data[MAX_PLUGIN_DATA + 1];

	num_extensions = 0;
	set_default_params(settings);

	p = getlogin();
	i = sizeof(settings->username) - 1;
	if (p != 0)
	{
		strncpy(settings->username, p, i);
	}
	else
	{
		pw = getpwuid(getuid());
		if (pw != 0)
		{
			if (pw->pw_name != 0)
			{
				strncpy(settings->username, pw->pw_name, i);
			}
		}
	}

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
		else if (strcmp("--gdi", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing GDI rendering\n");
				return 1;
			}
			if (strncmp("sw", argv[*pindex], 1) == 0) /* Software */
			{
				settings->software_gdi = 1;
			}
			else if (strncmp("hw", argv[*pindex], 1) == 0) /* Hardware */
			{
				settings->software_gdi = 0;
			}
			else
			{
				printf("unknown GDI rendering\n");
				return 1;
			}
		}
		else if (strcmp("--no-osb", argv[*pindex]) == 0)
		{
			settings->off_screen_bitmaps = 0;
		}
		else if (strcmp("--rfx", argv[*pindex]) == 0)
		{
			settings->rfx_flags = 1;
			settings->ui_decode_flags = 1;
			settings->use_frame_ack = 0;
			settings->server_depth = 32;
			settings->performanceflags = PERF_FLAG_NONE;
		}
		else if ((strcmp("-h", argv[*pindex]) == 0) || strcmp("--help", argv[*pindex]) == 0)
		{
			out_args();
			return 1;
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
#ifndef DISABLE_TLS
		else if (strcmp("--no-rdp", argv[*pindex]) == 0)
		{
			settings->rdp_security = 0;
		}
		else if (strcmp("--no-tls", argv[*pindex]) == 0)
		{
			settings->tls_security = 0;
		}
		else if (strcmp("--no-nla", argv[*pindex]) == 0)
		{
			settings->nla_security = 0;
		}
		else if (strcmp("--sec", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing protocol security\n");
				return 1;
			}
			if (strncmp("rdp", argv[*pindex], 1) == 0) /* Standard RDP */
			{
				settings->rdp_security = 1;
				settings->tls_security = 0;
				settings->nla_security = 0;
			}
			else if (strncmp("tls", argv[*pindex], 1) == 0) /* TLS */
			{
				settings->rdp_security = 0;
				settings->tls_security = 1;
				settings->nla_security = 0;
			}
			else if (strncmp("nla", argv[*pindex], 1) == 0) /* NLA */
			{
				settings->rdp_security = 0;
				settings->tls_security = 0;
				settings->nla_security = 1;
			}
			else
			{
				printf("unknown protocol security\n");
				return 1;
			}
		}
#endif
		else if (strcmp("--plugin", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing plugin name\n");
				return 1;
			}
			index = *pindex;
			memset(plugin_data, 0, sizeof(plugin_data));
			if (*pindex < argc - 1 && strcmp("--data", argv[*pindex + 1]) == 0)
			{
				*pindex = *pindex + 2;
				i = 0;
				while (*pindex < argc && strcmp("--", argv[*pindex]) != 0 && i < MAX_PLUGIN_DATA)
				{
					plugin_data[i].size = sizeof(RD_PLUGIN_DATA);
					for (j = 0, p = argv[*pindex]; j < 4 && p != NULL; j++)
					{
						plugin_data[i].data[j] = p;
						if (j < 3)
						{
							p = strchr(plugin_data[i].data[j], ':');
							if (p != NULL)
								*p++ = 0;
						}
					}
					*pindex = *pindex + 1;
					i++;
				}
			}
			freerdp_chanman_load_plugin(chan_man, settings, argv[index], plugin_data);
		}
		else if (strcmp("--ext", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing extension name\n");
				return 1;
			}
			if (num_extensions >= sizeof(settings->extensions) / sizeof(struct rdp_ext_set))
			{
				printf("maximum extensions reached\n");
				return 1;
			}
			index = *pindex;
			snprintf(settings->extensions[num_extensions].name,
				sizeof(settings->extensions[num_extensions].name),
				"%s", argv[index]);
			settings->extensions[num_extensions].data = NULL;
			if (*pindex < argc - 1 && strcmp("--data", argv[*pindex + 1]) == 0)
			{
				*pindex = *pindex + 2;
				settings->extensions[num_extensions].data = argv[*pindex];
				i = 0;
				while (*pindex < argc && strcmp("--", argv[*pindex]) != 0)
				{
					*pindex = *pindex + 1;
					i++;
				}
			}
			num_extensions++;
		}
		else if ((strcmp("-h", argv[*pindex]) == 0) || strcmp("--help", argv[*pindex]) == 0)
		{
			out_args();
			return 1;
		}
		else if (strcmp("--version", argv[*pindex]) == 0)
		{
			printf("This is FreeRDP version %s\n", PACKAGE_VERSION);
			return 1;
		}
		else if (argv[*pindex][0] != '-')
		{
			settings->server[sizeof(settings->server) - 1] = 0;
			if (argv[*pindex][0] == '[' && (p = strchr(argv[*pindex], ']'))
				&& (p[1] == 0 || (p[1] == ':' && !strchr(p + 2, ':'))))
			{
				/* Either "[...]" or "[...]:..." with at most one : after the brackets */
				strncpy(settings->server, argv[*pindex] + 1, sizeof(settings->server) - 1);
				if ((p = strchr(settings->server, ']')))
				{
					*p = 0;
					if (p[1] == ':')
						settings->tcp_port_rdp = atoi(p + 2);
				}
			}
			else
			{
				/* Port number is cut off and used if exactly one : in the string */
				strncpy(settings->server, argv[*pindex], sizeof(settings->server) - 1);
				if ((p = strchr(settings->server, ':')) && !strchr(p + 1, ':'))
				{
					*p = 0;
					settings->tcp_port_rdp = atoi(p + 1);
				}
			}
			/* server is the last argument for the current session. arguments
			   followed will be parsed for the next session. */
			*pindex = *pindex + 1;

			return 0;
		}
		else
		{
			printf("invalid option: %s\n", argv[*pindex]);
			return 1;
		}
		*pindex = *pindex + 1;
	}
	printf("missing server name\n");
	return 1;
}

static int
run_dfbfreerdp(rdpSet * settings, rdpChanMan * chan_man)
{
	rdpInst * inst;
	void * read_fds[32];
	void * write_fds[32];
	int read_count;
	int write_count;
	int index;
	int sck;
	int max_sck;
	fd_set rfds;
	fd_set wfds;

	memset(read_fds, 0, sizeof(read_fds));
	memset(write_fds, 0, sizeof(write_fds));

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
		if (dfb_check_fds(inst, &rfds) != 0)
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
	inst->rdp_disconnect(inst);
	dfb_uninit(inst);
	freerdp_free(inst);
	return 0;
}

static sem_t g_sem;
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
        if (g_thread_count < 1)
        {
                freerdp_sem_signal(&g_sem);
        }

	return NULL;
}

int
main(int argc, char ** argv)
{
	struct thread_data * data;
	int rv;
	pthread_t thread;
	int index = 1;

	char *home = getenv("HOME");
	if (home) {
		static char resourcefile[512];
		strncat(resourcefile, home, strlen(home));
		resourcefile[512-1] = (char)0;
		strcat(resourcefile, "/.directfbrc");
		resourcefile[512-1] = (char)0;

		char *display = getenv("DISPLAY");

#if   defined(__unix) || defined(__linux)
		char *graphics = "fbdev";
#elif defined(__APPLE__)
		char *graphics = "opengl";
#else
		char *graphics = "gdi";
#endif
		if (display) graphics = "x11";

		static char buffer[128];
		strcat(buffer, "system=");
		strcat(buffer, graphics);
		strcat(buffer, "\ndepth=32\nmode=1024x768\nautoflip-window\nforce-windowed\n");

		FILE *fp;
		fp = fopen(resourcefile, "wx"); /* "x" assures no overwrite of an existing resource file */
		if (fp != NULL)
		{
			fputs((char *)(&buffer), fp);
			fclose(fp);
			printf("INFO: created default DirectFB resource file: %s\n", resourcefile);
		}
	} else {
		printf("WARNING: HOME variable not set, unable to create a default DirectFB ~/.directfbrc resource file\n");
	}

	setlocale(LC_CTYPE, "");

	if (!freerdp_global_init())
	{
		printf("Error initializing freerdp\n");
		return 1;
	}
	freerdp_chanman_init();

	dfb_init(&argc, &argv);
	dfb_kb_init();

	freerdp_sem_create(&g_sem, 0);

	while (1)
	{
		data = (struct thread_data *) xmalloc(sizeof(struct thread_data));
		data->settings = (rdpSet *) xmalloc(sizeof(rdpSet));
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
                DEBUG("main thread, waiting for all threads to exit");
                freerdp_sem_wait(&g_sem);
                DEBUG("main thread, all threads did exit");
	}

	freerdp_chanman_uninit();
	freerdp_global_finish();

	return 0;
}

