/*
   FreeRDP: A Remote Desktop Protocol client.

   Copyright (c) 2009-2011 Jay Sorg
   Copyright (c) 2010-2011 Vic Lee

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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <freerdp/freerdp.h>
#include <freerdp/chanman.h>
#include "wf_types.h"
#include "wf_event.h"
#include "wf_win.h"

LPCTSTR g_wnd_class_name = L"wfreerdp";
HINSTANCE g_hInstance;
HCURSOR g_default_cursor;
volatile int g_thread_count = 0;
HANDLE g_done_event;
HWND g_focus_hWnd;

static int
create_console(void)
{
	if (!AllocConsole())
	{
		return 1;
	}
	freopen("CONOUT$", "w", stdout);
	printf("Debug console created.\n");
	return 0;
}

static int
set_default_params(wfInfo * wfi)
{
	rdpSet * settings;

	settings = wfi->settings;
	memset(settings, 0, sizeof(rdpSet));
	gethostname(settings->hostname, sizeof(settings->hostname) - 1);
	settings->width = 800;
	settings->height = 600;
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
	settings->triblt = 0;
	settings->new_cursors = 1;
	settings->rdp_version = 5;
	settings->rdp_security = 1;
#ifndef DISABLE_TLS
	settings->tls_security = 1;
	settings->nla_security = 1;
#endif
	wfi->fullscreen = wfi->fs_toggle = 0;
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
		"\t-s: shell\n"
		"\t-c: directory\n"
		"\t-g: geometry, using format WxH or X%, default is 800x600\n"
		"\t-t: alternative port number, default is 3389\n"
		"\t-n: hostname\n"
		"\t-o: console audio\n"
		"\t-0: console session\n"
		"\t-f: fullscreen mode\n"
		"\t-z: enable bulk compression\n"
		"\t-x: performance flags (m, b or l for modem, broadband or lan)\n"
#ifndef DISABLE_TLS
		"\t--no-rdp: disable Standard RDP encryption\n"
		"\t--no-tls: disable TLS encryption\n"
		"\t--no-nla: disable network level authentication\n"
		"\t--sec: force protocol security (rdp, tls or nla)\n"
#endif
		"\t--no-osb: disable off screen bitmaps, default on\n"
		"\t--no-console: don't open a console window for debug output\n"
		"\t--debug-log: write debug output to freerdp.log\n"
		"\t--version: Print out the version and exit\n"
		"\t-h: show this help\n";
	printf("%s\n", help);
	return 0;
}

static int
process_params(wfInfo * wfi, int argc, char ** argv, int * pindex)
{
	rdpSet * settings;
	char * p;
	int i;
	char show_console = 1;

	/* Early scanning of options for stdout/console handling */
	for(i = 1; i < argc; i++)
		if (strcmp("--no-console", argv[i]) == 0)
		{
			show_console = 0;
		}
		else if (strcmp("--debug-log", argv[i]) == 0)
		{
			freopen("freerdp.log", "w", stdout);
			show_console = 0;
		}
	if (show_console && !GetConsoleWindow())
	{
		create_console();
		printf("(this console window can be redirected to freerdp.log with --debug-log\n"
				"or hidden with --no-debug)\n");
	}

	set_default_params(wfi);
	settings = wfi->settings;

	DEBUG("process_params\n");
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
			if (*p == '%')
			{
				wfi->percentscreen = settings->width;
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
		else if (strcmp("--no-osb", argv[*pindex]) == 0)
		{
			settings->off_screen_bitmaps = 0;
		}
		else if (strcmp("-f", argv[*pindex]) == 0)
		{
			wfi->fullscreen = wfi->fs_toggle = 1;
			DEBUG("full screen option\n");
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
			/* TODO: Handle --data ... -- */
			freerdp_chanman_load_plugin(wfi->chan_man, settings, argv[*pindex], NULL);
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
		else if ((strcmp("--no-console", argv[*pindex]) == 0) ||
				(strcmp("--debug-log", argv[*pindex]) == 0))
		{
			/* Skip options that already has been processed */
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
run_wfreerdp(wfInfo * wfi)
{
	rdpInst * inst;
	void * read_fds[32];
	void * write_fds[32];
	int read_count;
	int write_count;
	int index;
	HANDLE fds[64];
	int fds_count;
	int gmcode;
	int alldone;
	MSG msg;

	DEBUG("run_wfreerdp:\n");
	/* create an instance of the library */
	wfi->inst = inst = freerdp_new(wfi->settings);
	if (inst == NULL)
	{
		printf("run_wfreerdp: freerdp_new failed\n");
		return 1;
	}
	SET_WFI(inst, wfi);

	if ((inst->version != FREERDP_INTERFACE_VERSION) ||
	    (inst->size != sizeof(rdpInst)))
	{
		printf("run_wfreerdp: freerdp_new size, version / size do not "
		       "match expecting v %d s %d got v %d s %d\n",
		       FREERDP_INTERFACE_VERSION, sizeof(rdpInst),
		       inst->version, inst->size);
		return 1;
	}

	inst->settings->keyboard_layout = (int)GetKeyboardLayout(0) & 0x0000FFFF;
	printf("keyboard_layout: 0x%X\n", inst->settings->keyboard_layout);

	if (wf_pre_connect(wfi) != 0)
	{
		printf("run_wfreerdp: wf_pre_connect failed\n");
		return 1;
	}
	if (freerdp_chanman_pre_connect(wfi->chan_man, inst) != 0)
	{
		printf("run_wfreerdp: freerdp_chanman_pre_connect failed\n");
		return 1;
	}
	/* call connect */
	if (inst->rdp_connect(inst) != 0)
	{
		printf("run_wfreerdp: inst->rdp_connect failed\n");
		return 1;
	}
	if (freerdp_chanman_post_connect(wfi->chan_man, inst) != 0)
	{
		printf("run_wfreerdp: freerdp_chanman_post_connect failed\n");
		return 1;
	}
	if (wf_post_connect(wfi) != 0)
	{
		printf("run_wfreerdp: wf_post_connect failed\n");
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
			printf("run_wfreerdp: inst->rdp_get_fds failed\n");
			break;
		}
		/* get channel fds */
		if (freerdp_chanman_get_fds(wfi->chan_man, inst, read_fds, &read_count, write_fds, &write_count) != 0)
		{
			printf("run_wfreerdp: freerdp_chanman_get_fds failed\n");
			break;
		}
		fds_count = 0;
		/* setup read fds */
		for (index = 0; index < read_count; index++)
		{
			fds[fds_count++] = read_fds[index];
		}
		/* setup write fds */
		for (index = 0; index < write_count; index++)
		{
			fds[fds_count++] = write_fds[index];
		}
		/* exit if nothing to do */
		if (fds_count == 0)
		{
			printf("run_wfreerdp: fds_count is zero\n");
			break;
		}
		/* do the wait */
		if (MsgWaitForMultipleObjects(fds_count, fds, FALSE, INFINITE, QS_ALLINPUT) == WAIT_FAILED)
		{
			printf("run_wfreerdp: WaitForMultipleObjects failed\n");
			break;
		}
		/* check the libfreerdp fds */
		if (inst->rdp_check_fds(inst) != 0)
		{
			printf("run_wfreerdp: inst->rdp_check_fds failed\n");
			break;
		}
		/* check channel fds */
		if (freerdp_chanman_check_fds(wfi->chan_man, inst) != 0)
		{
			printf("run_wfreerdp: freerdp_chanman_check_fds failed\n");
			break;
		}
		alldone = FALSE;
		while (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
		{
			gmcode = GetMessage(&msg, 0, 0, 0);
			if (gmcode == 0 || gmcode == -1)
			{
				alldone = TRUE;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (alldone)
		{
			break;
		}
		wf_update_window(wfi);
	}
	/* cleanup */
	wf_uninit(wfi);
	freerdp_chanman_free(wfi->chan_man);
	freerdp_free(inst);
	free(wfi->settings);
	free(wfi);
	return 0;
}

static DWORD WINAPI
thread_func(LPVOID lpParam)
{
	wfInfo * wfi;

	wfi = (wfInfo *) lpParam;
	run_wfreerdp(wfi);
	g_thread_count--;
	DEBUG("thread terminated - count now %d\n", g_thread_count);
	if (g_thread_count < 1)
	{
		SetEvent(g_done_event);
	}
	return NULL;
}

static DWORD WINAPI
kbd_thread_func(LPVOID lpParam)
{
	HHOOK hook_handle;
	MSG msg;
	BOOL bRet;

	DEBUG("keyboard thread started\n");

	hook_handle = SetWindowsHookEx(WH_KEYBOARD_LL, wf_ll_kbd_proc, g_hInstance, 0);
	if (hook_handle)
	{
		while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
		{
			if (bRet == -1)
			{
				printf("keyboard thread error getting message\n");
				break;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		DEBUG("keyboard thread ended\n");
		UnhookWindowsHookEx(hook_handle);
	}
	else
		printf("failed to install keyboard hook\n");

	return NULL;
}

INT WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wnd_cls;
	WSADATA wsa_data;
	wfInfo * wfi;
	int rv;
	int index = 1;

	if (WSAStartup(0x101, &wsa_data) != 0)
	{
		return 1;
	}
	g_done_event = CreateEvent(0, 1, 0, 0);
#if defined(WITH_DEBUG) || defined(_DEBUG)
	create_console();
#endif
	if (!freerdp_global_init())
	{
		printf("Error initializing freerdp\n");
		return 1;
	}
	freerdp_chanman_init();
	g_default_cursor = LoadCursor(NULL, IDC_ARROW);

	wnd_cls.cbSize        = sizeof(WNDCLASSEX);
	wnd_cls.style         = CS_HREDRAW | CS_VREDRAW;
	wnd_cls.lpfnWndProc   = wf_event_proc;
	wnd_cls.cbClsExtra    = 0;
	wnd_cls.cbWndExtra    = 0;
	wnd_cls.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wnd_cls.hCursor       = g_default_cursor;
	wnd_cls.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wnd_cls.lpszMenuName  = NULL;
	wnd_cls.lpszClassName = g_wnd_class_name;
	wnd_cls.hInstance     = hInstance;
	wnd_cls.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&wnd_cls);

	g_hInstance = hInstance;

	if (!CreateThread(NULL, 0, kbd_thread_func, NULL, 0, NULL))
		printf("error creating keyboard handler thread");

	while (1)
	{
		wfi = (wfInfo *) malloc(sizeof(wfInfo));
		memset(wfi, 0, sizeof(wfInfo));
		wfi->settings = (rdpSet *) malloc(sizeof(rdpSet));
		wfi->chan_man = freerdp_chanman_new();
		rv = process_params(wfi, __argc, __argv, &index);
		if (rv)
		{
			freerdp_chanman_free(wfi->chan_man);
			free(wfi->settings);
			free(wfi);
			break;
		}
		if (CreateThread(NULL, 0, thread_func, wfi, 0, NULL) != 0)
		{
			g_thread_count++;
		}
	}

	if (g_thread_count > 0)
		WaitForSingleObject(g_done_event, INFINITE);
	else
		MessageBox(GetConsoleWindow(),
			L"Failed to start wfreerdp.\n\nPlease check the debug output.",
			L"FreeRDP Error", MB_ICONSTOP);

	freerdp_chanman_uninit();
	freerdp_global_finish();
	WSACleanup();
	return 0;
}
