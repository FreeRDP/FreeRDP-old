/*
   FreeRDP: A Remote Desktop Protocol client.
   Main

   Copyright (C) Jay Sorg 2009-2011

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
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <freerdp/freerdp.h>
#include <freerdp/chanman.h>
#include <freerdp/kbd.h>
#include <freerdp/errinfo.h>
#include <freerdp/utils/semaphore.h>
#include "xf_types.h"
#include "xf_win.h"
#include "xf_keyboard.h"
#include "xf_event.h"
#include "xf_video.h"
#include "xf_decode.h"

#define MAX_PLUGIN_DATA 20

static sem_t g_sem;
static volatile int g_thread_count = 0;

/* RDP disconnect reason */
static uint32 g_disconnect_reason;

/* generic ui error found before starting RDP communication */
static uint8 g_error_code = 0;

static int
set_default_params(xfInfo * xfi)
{
	rdpSet * settings;

	settings = xfi->settings;
	memset(settings, 0, sizeof(rdpSet));
	gethostname(settings->hostname, sizeof(settings->hostname) - 1);
	settings->width = 1024;
	settings->height = 768;
	strcpy(settings->server, "127.0.0.1");
	strcpy(settings->username, "guest");
	settings->tcp_port_rdp = 3389;
	settings->encryption = 1;
	settings->server_depth = 16;
	settings->bitmap_cache = 1;
	settings->bitmap_compression = 1;
	settings->desktop_save = 0;
	settings->performanceflags =
		PERF_DISABLE_WALLPAPER | PERF_DISABLE_FULLWINDOWDRAG | PERF_DISABLE_MENUANIMATIONS;
	settings->off_screen_bitmaps = 1;
	settings->polygon_ellipse_orders = 1;
	settings->triblt = 0;
	settings->software_gdi = 0;
	settings->new_cursors = 1;
	settings->rdp_version = 5;
	settings->rdp_security = 1;
#ifndef DISABLE_TLS
	settings->tls_security = 1;
	settings->nla_security = 1;
#endif
	xfi->fullscreen = xfi->fs_toggle = 0;
	xfi->decoration = 1;
	xfi->grab_keyboard = 1;
#ifdef HAVE_XV
	xfi->xv_port = -1;
#endif
	return 0;
}

static int
out_args(void)
{
	char help[] =
		"\n"
		"FreeRDP - A Free Remote Desktop Protocol Client\n"
		"See http://freerdp.sourceforge.net for more information\n"
		"\n"
		"Usage: xfreerdp [options] server:port\n"
		"\t-a: color depth (8, 15, 16, 24 or 32)\n"
		"\t-u: username\n"
		"\t-p: password\n"
		"\t-d: domain\n"
		"\t-k: keyboard layout ID\n"
		"\t-K: do not interfere with window manager bindings\n"
		"\t--kbd-list: list all keyboard layout IDs\n"
		"\t-s: shell\n"
		"\t-c: directory\n"
		"\t-g: geometry, using format WxH or X%, default is 1024x768\n"
		"\t-t: alternative port number, default is 3389\n"
		"\t-T: set window title\n"
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

/* Returns "true" on errors or other reasons to not continue normal operation */
static int
process_params(xfInfo * xfi, int argc, char ** argv, int * pindex)
{
	rdpSet * settings;
	rdpKeyboardLayout * layouts;
	char * p;
	RD_PLUGIN_DATA plugin_data[MAX_PLUGIN_DATA + 1];
	int index;
	int i, j;
	struct passwd * pw;
	int num_extensions;
	int rv;

	set_default_params(xfi);
	settings = xfi->settings;
	num_extensions = 0;
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
				exit(XF_EXIT_WRONG_PARAM);
			}
			settings->server_depth = atoi(argv[*pindex]);
		}
		else if (strcmp("-u", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing username\n");
				exit(XF_EXIT_WRONG_PARAM);
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
				exit(XF_EXIT_WRONG_PARAM);
			}
			strncpy(settings->password, argv[*pindex], sizeof(settings->password) - 1);
			settings->password[sizeof(settings->password) - 1] = 0;
			settings->autologin = 1;

			/*
			 * Overwrite original password which could be revealed by a simple "ps aux" command.
			 * This approach won't hide the password length, but it is better than nothing.
			 */

			memset(argv[*pindex], '*', strlen(argv[*pindex]));
		}
		else if (strcmp("-d", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing domain\n");
				exit(XF_EXIT_WRONG_PARAM);
			}
			strncpy(settings->domain, argv[*pindex], sizeof(settings->domain) - 1);
			settings->domain[sizeof(settings->domain) - 1] = 0;
		}
		else if (strcmp("-k", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing keyboard layout ID\n");
				exit(XF_EXIT_WRONG_PARAM);
			}
			sscanf(argv[*pindex], "%X", &(xfi->keyboard_layout_id));
			DEBUG_X11("keyboard layout ID: %X", xfi->keyboard_layout_id);
		}
		else if (strcmp("-K", argv[*pindex]) == 0)
		{
			xfi->grab_keyboard = 0;
		}
		else if (strcmp("--kbd-list", argv[*pindex]) == 0)
		{
			layouts = freerdp_kbd_get_layouts(RDP_KEYBOARD_LAYOUT_TYPE_STANDARD);
			printf("\nKeyboard Layouts\n");
			for (i = 0; layouts[i].code; i++)
				printf("0x%08X\t%s\n", layouts[i].code, layouts[i].name);
			free(layouts);

			layouts = freerdp_kbd_get_layouts(RDP_KEYBOARD_LAYOUT_TYPE_VARIANT);
			printf("\nKeyboard Layout Variants\n");
			for (i = 0; layouts[i].code; i++)
				printf("0x%08X\t%s\n", layouts[i].code, layouts[i].name);
			free(layouts);

			layouts = freerdp_kbd_get_layouts(RDP_KEYBOARD_LAYOUT_TYPE_IME);
			printf("\nKeyboard Input Method Editors (IMEs)\n");
			for (i = 0; layouts[i].code; i++)
				printf("0x%08X\t%s\n", layouts[i].code, layouts[i].name);
			free(layouts);

			return 1;
		}
		else if (strcmp("-s", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing shell\n");
				exit(XF_EXIT_WRONG_PARAM);
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
				exit(XF_EXIT_WRONG_PARAM);
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
				exit(XF_EXIT_WRONG_PARAM);
			}
			settings->width = strtol(argv[*pindex], &p, 10);
			if (*p == 'x')
			{
				settings->height = strtol(p + 1, &p, 10);
			}
			if (*p == '%')
			{
				xfi->percentscreen = settings->width;
			}
		}
		else if (strcmp("-t", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing port number\n");
				exit(XF_EXIT_WRONG_PARAM);
			}
			settings->tcp_port_rdp = atoi(argv[*pindex]);
		}
		else if (strcmp("-T", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing window title\n");
				exit(XF_EXIT_WRONG_PARAM);
			}
			strncpy(xfi->window_title, argv[*pindex], sizeof(xfi->window_title) - 1);
			xfi->window_title[sizeof(xfi->window_title) - 1] = 0;
		}
		else if (strcmp("-n", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing hostname\n");
				exit(XF_EXIT_WRONG_PARAM);
			}
			strncpy(settings->hostname, argv[*pindex], sizeof(settings->hostname) - 1);
			settings->hostname[sizeof(settings->hostname) - 1] = 0;
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
			xfi->codec = XF_CODEC_REMOTEFX;
		}
#ifdef HAVE_XV
		else if (strcmp("--xv-port", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			xfi->xv_port = atoi(argv[*pindex]);
		}
#endif
		else if (strcmp("-f", argv[*pindex]) == 0)
		{
			xfi->fullscreen = xfi->fs_toggle = 1;
			printf("full screen option\n");
		}
		else if (strcmp("-D", argv[*pindex]) == 0)
		{
			xfi->decoration = 0;
		}
		else if (strcmp("-x", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing performance flag\n");
				exit(XF_EXIT_WRONG_PARAM);
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
		} else if (strcmp("-X", argv[*pindex]) == 0) {
			*pindex = *pindex + 1;

			if (*pindex == argc) {
				printf("missing XID\n");
				exit(XF_EXIT_WRONG_PARAM);
			}

			xfi->embed = strtoul(argv[*pindex], NULL, 16);
			if (!xfi->embed) {
				printf("bad XID\n");
				exit(XF_EXIT_WRONG_PARAM);
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
				exit(XF_EXIT_WRONG_PARAM);
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
				exit(XF_EXIT_WRONG_PARAM);
			}
		}
#endif
		else if (strcmp("--plugin", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing plugin name\n");
				exit(XF_EXIT_WRONG_PARAM);
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
			rv = freerdp_chanman_load_plugin(xfi->chan_man, settings, argv[index], plugin_data);
			if (rv)
				exit(XF_EXIT_WRONG_PARAM);
		}
		else if (strcmp("--ext", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing extension name\n");
				exit(XF_EXIT_WRONG_PARAM);
			}
			if (num_extensions >= sizeof(settings->extensions) / sizeof(struct rdp_ext_set))
			{
				printf("maximum extensions reached\n");
				exit(XF_EXIT_WRONG_PARAM);
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
			exit(XF_EXIT_WRONG_PARAM);
		}
		*pindex = *pindex + 1;
	}
	printf("missing server name\n");
	exit(XF_EXIT_WRONG_PARAM);
}

static int
run_xfreerdp(xfInfo * xfi)
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
	RD_EVENT * event;

	/* create an instance of the library */
	inst = freerdp_new(xfi->settings);
	if (inst == NULL)
	{
		printf("run_xfreerdp: freerdp_new failed\n");
		return XF_EXIT_MEMORY;
	}
	if ((inst->version != FREERDP_INTERFACE_VERSION) ||
	    (inst->size != sizeof(rdpInst)))
	{
		printf("run_xfreerdp: freerdp_new size, version / size do not "
		       "match expecting v %d s %d got v %d s %d\n",
		       FREERDP_INTERFACE_VERSION, (int)sizeof(rdpInst),
		       inst->version, inst->size);
		return XF_EXIT_PROTOCOL;
	}
	xfi->inst = inst;
	SET_XFI(inst, xfi);
	if (xf_pre_connect(xfi) != 0)
	{
		printf("run_xfreerdp: xf_pre_connect failed\n");
		return XF_EXIT_CONN_FAILED;
	}
	if (freerdp_chanman_pre_connect(xfi->chan_man, inst) != 0)
	{
		printf("run_xfreerdp: freerdp_chanman_pre_connect failed\n");
		return XF_EXIT_CONN_FAILED;
	}

	xf_kb_init(xfi->display, xfi->keyboard_layout_id);
	xf_kb_inst_init(xfi);
	printf("keyboard_layout: 0x%X\n", inst->settings->keyboard_layout);

	/* call connect */
	if (inst->rdp_connect(inst) != 0)
	{
		printf("run_xfreerdp: inst->rdp_connect failed\n");
		return XF_EXIT_CONN_FAILED;
	}
	if (freerdp_chanman_post_connect(xfi->chan_man, inst) != 0)
	{
		printf("run_xfreerdp: freerdp_chanman_post_connect failed\n");
		return XF_EXIT_CONN_FAILED;
	}
	if (xf_post_connect(xfi) != 0)
	{
		printf("run_xfreerdp: xf_post_connect failed\n");
		return XF_EXIT_CONN_FAILED;
	}
	xf_video_init(xfi);
	xf_decode_init(xfi);

	/* program main loop */
	while (1)
	{
		read_count = 0;
		write_count = 0;
		/* get libfreerdp fds */
		if (inst->rdp_get_fds(inst, read_fds, &read_count, write_fds, &write_count) != 0)
		{
			printf("run_xfreerdp: inst->rdp_get_fds failed\n");
			break;
		}
		/* get x fds */
		if (xf_get_fds(xfi, read_fds, &read_count, write_fds, &write_count) != 0)
		{
			printf("run_xfreerdp: xf_get_fds failed\n");
			break;
		}
		/* get channel fds */
		if (freerdp_chanman_get_fds(xfi->chan_man, inst, read_fds, &read_count, write_fds, &write_count) != 0)
		{
			printf("run_xfreerdp: freerdp_chanman_get_fds failed\n");
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
			printf("run_xfreerdp: max_sck is zero\n");
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
				printf("run_xfreerdp: select failed\n");
				break;
			}
		}
		/* check the libfreerdp fds */
		if (inst->rdp_check_fds(inst) != 0)
		{
			printf("run_xfreerdp: inst->rdp_check_fds failed\n");
			break;
		}
		/* check x fds */
		if (xf_check_fds(xfi) != 0)
		{
			/* xfreerdp is usually terminated by this failing because the X windows has been closed */
			DEBUG_X11("xf_check_fds failed");
			break;
		}
		/* check channel fds */
		if (freerdp_chanman_check_fds(xfi->chan_man, inst) != 0)
		{
			printf("run_xfreerdp: freerdp_chanman_check_fds failed\n");
			break;
		}
		/* check channel event */
		event = freerdp_chanman_pop_event(xfi->chan_man);
		if (event)
		{
			switch (event->event_type)
			{
				case RD_EVENT_TYPE_VIDEO_FRAME:
					xf_video_process_frame(xfi, (RD_VIDEO_FRAME_EVENT *) event);
					break;
				case RD_EVENT_TYPE_REDRAW:
					xf_handle_redraw_event(xfi, (RD_REDRAW_EVENT *) event);
					break;
				default:
					printf("run_xfreerdp: unknown event type %d\n", event->event_type);
					break;
			}
			freerdp_chanman_free_event(xfi->chan_man, event);
		}
	}

	g_disconnect_reason = inst->disc_reason;

	/* cleanup */
	xf_decode_uninit(xfi);
	xf_video_uninit(xfi);
	freerdp_chanman_close(xfi->chan_man, inst);
	inst->rdp_disconnect(inst);
	freerdp_free(inst);
	xf_uninit(xfi);
	return 0;
}

/* maches an error info PDU set to the XF_EXIT_CODE */
static uint8
exit_code_from_disconnect_reason(uint32 reason)
{
	if (reason == 0)
		return XF_EXIT_SUCCESS;

	/* License error set */
	else if (reason >= ERRINFO_LICENSE_INTERNAL &&
		reason <= ERRINFO_LICENSE_NO_REMOTE_CONNECTIONS)
		reason -= ERRINFO_LICENSE_INTERNAL + XF_EXIT_LICENSE_INTERNAL;

	/* RDP protocol error set */
	else if (reason >= 0x10c9 && reason <= 0x1193)
		reason = XF_EXIT_RDP;

	/* There's no need to test protocol-independent codes: they match */
	else if (!(reason <= ERRINFO_RPC_INITIATED_DISCONNECT_BYUSER))
		reason = XF_EXIT_UNKNOWN;

	return reason;
}

static void *
thread_func(void * arg)
{
	xfInfo * xfi;

	xfi = (xfInfo *) arg;
	g_error_code = run_xfreerdp(xfi);
	free(xfi->settings);
	freerdp_chanman_free(xfi->chan_man);
	free(xfi);

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
	int rv;
	xfInfo * xfi;
	pthread_t thread;
	int index = 1;
	char reason_msg[ERRINFO_BUFFER_SIZE];

	setlocale(LC_CTYPE, "");
	if (argc == 1)
	{
		out_args();
		return 0;
	}
	if (!freerdp_global_init())
	{
		printf("Error initializing freerdp\n");
		return 1;
	}
	freerdp_chanman_init();

	freerdp_sem_create(&g_sem, 0);

	while (1)
	{
		xfi = (xfInfo *) malloc(sizeof(xfInfo));
		memset(xfi, 0, sizeof(xfInfo));
		xfi->settings = (rdpSet *) malloc(sizeof(rdpSet));
		xfi->chan_man = freerdp_chanman_new();
		xfi->clrconv = (HCLRCONV) malloc(sizeof(CLRCONV));
		memset(xfi->clrconv, 0, sizeof(CLRCONV));

		xfi->clrconv->alpha = 1;
		xfi->clrconv->palette = NULL;

		rv = process_params(xfi, argc, argv, &index);
		if (rv)
		{
			free(xfi->settings);
			freerdp_chanman_free(xfi->chan_man);
			free(xfi);
			break;
		}

		DEBUG_X11("starting thread %d to %s:%d", g_thread_count,
			xfi->settings->server, xfi->settings->tcp_port_rdp);
		if (pthread_create(&thread, 0, thread_func, xfi) == 0)
		{
			g_thread_count++;
		}
	}

	if (g_thread_count > 0)
	{
		DEBUG_X11("main thread, waiting for all threads to exit");
		freerdp_sem_wait(&g_sem);
		DEBUG_X11("main thread, all threads did exit");
	}

	freerdp_chanman_uninit();
	freerdp_global_finish();

	if (g_error_code)
		return g_error_code;
	else if (g_disconnect_reason)
	{
		printf("disconnect: %s\n",
			freerdp_str_disconnect_reason(g_disconnect_reason, reason_msg, ERRINFO_BUFFER_SIZE));
	}

	return exit_code_from_disconnect_reason(g_disconnect_reason);
}
