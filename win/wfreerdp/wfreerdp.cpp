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
#include "wf_event.h"
#include "wf_win.h"

LPCTSTR g_wnd_class_name = L"wfreerdp";
HINSTANCE g_hInstance;
HCURSOR g_default_cursor;

struct thread_data
{
	rdpSet * settings;
	rdpChanMan * chan_man;
	HWND hwnd;
};

static int
set_default_params(rdpSet * settings)
{
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
		"\t-g: geometry, using format WxH, default is 1024x768\n"
		"\t-t: alternative port number, default is 3389\n"
		"\t-n: hostname\n"
		"\t-o: console audio\n"
		"\t-0: console session\n"
		"\t-z: enable bulk compression\n"
		"\t-x: performance flags (m, b or l for modem, broadband or lan)\n"
#ifndef DISABLE_TLS
		"\t--no-tls: disable TLS encryption\n"
#endif
		"\t--no-osb: disable off screen bitmaps, default on\n"
		"\t--version: Print out the version and exit\n"
		"\t-h: show this help\n";
	printf("%s\n", help);
	return 0;
}

static int
process_params(rdpSet * settings, rdpChanMan * chan_man, int argc, LPWSTR * argv, int * pindex)
{
	WCHAR * p;

	set_default_params(settings);

	printf("process_params\n");
	if (argc < *pindex + 1)
	{
		if (*pindex == 1)
			printf("no parameters specified\n");
		return 1;
	}
	while (*pindex < argc)
	{
		if (wcscmp(L"-a", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing server depth\n");
				return 1;
			}
			settings->server_depth = _wtoi(argv[*pindex]);
		}
		else if (wcscmp(L"-u", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing username\n");
				return 1;
			}
			WideCharToMultiByte(CP_ACP, 0, argv[*pindex], -1, settings->username, sizeof(settings->username) - 1, NULL, NULL);
			settings->username[sizeof(settings->username) - 1] = 0;
		}
		else if (wcscmp(L"-p", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing password\n");
				return 1;
			}
			WideCharToMultiByte(CP_ACP, 0, argv[*pindex], -1, settings->password, sizeof(settings->password) - 1, NULL, NULL);
			settings->password[sizeof(settings->password) - 1] = 0;
			settings->autologin = 1;
		}
		else if (wcscmp(L"-d", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing domain\n");
				return 1;
			}
			WideCharToMultiByte(CP_ACP, 0, argv[*pindex], -1, settings->domain, sizeof(settings->domain) - 1, NULL, NULL);
			settings->domain[sizeof(settings->domain) - 1] = 0;
		}
		else if (wcscmp(L"-s", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing shell\n");
				return 1;
			}
			WideCharToMultiByte(CP_ACP, 0, argv[*pindex], -1, settings->shell, sizeof(settings->shell) - 1, NULL, NULL);
			settings->shell[sizeof(settings->shell) - 1] = 0;
		}
		else if (wcscmp(L"-c", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing directory\n");
				return 1;
			}
			WideCharToMultiByte(CP_ACP, 0, argv[*pindex], -1, settings->directory, sizeof(settings->directory) - 1, NULL, NULL);
			settings->directory[sizeof(settings->directory) - 1] = 0;
		}
		else if (wcscmp(L"-g", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing width\n");
				return 1;
			}
			settings->width = wcstol(argv[*pindex], &p, 10);
			if (*p == L'x')
			{
				settings->height = wcstol(p + 1, &p, 10);
			}
			if ((settings->width < 16) || (settings->height < 16) ||
				(settings->width > 4096) || (settings->height > 4096))
			{
				printf("invalid dimensions\n");
				return 1;
			}
		}
		else if (wcscmp(L"-t", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing port number\n");
				return 1;
			}
			settings->tcp_port_rdp = _wtoi(argv[*pindex]);
		}
		else if (wcscmp(L"-n", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing hostname\n");
				return 1;
			}
			WideCharToMultiByte(CP_ACP, 0, argv[*pindex], -1, settings->hostname, sizeof(settings->hostname) - 1, NULL, NULL);
			settings->hostname[sizeof(settings->hostname) - 1] = 0;
		}
		else if (wcscmp(L"-o", argv[*pindex]) == 0)
		{
			settings->console_audio = 1;
		}
		else if (wcscmp(L"-0", argv[*pindex]) == 0)
		{
			settings->console_session = 1;
		}
		else if (wcscmp(L"-z", argv[*pindex]) == 0)
		{
			settings->bulk_compression = 1;
		}
		else if (wcscmp(L"--no-osb", argv[*pindex]) == 0)
		{
			settings->off_screen_bitmaps = 0;
		}
		else if (wcscmp(L"-x", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing performance flag\n");
				return 1;
			}
			if (wcsncmp(L"m", argv[*pindex], 1) == 0) /* modem */
			{
				settings->performanceflags = PERF_DISABLE_WALLPAPER |
					PERF_DISABLE_FULLWINDOWDRAG | PERF_DISABLE_MENUANIMATIONS |
					PERF_DISABLE_THEMING;
			}
			else if (wcsncmp(L"b", argv[*pindex], 1) == 0) /* broadband */
			{
				settings->performanceflags = PERF_DISABLE_WALLPAPER;
			}
			else if (wcsncmp(L"l", argv[*pindex], 1) == 0) /* lan */
			{
				settings->performanceflags = PERF_FLAG_NONE;
			}
			else
			{
				settings->performanceflags = wcstol(argv[*pindex], 0, 16);
			}
		}
#ifndef DISABLE_TLS
		else if (wcscmp(L"--no-rdp", argv[*pindex]) == 0)
		{
			settings->rdp_security = 0;
		}
		else if (wcscmp(L"--no-tls", argv[*pindex]) == 0)
		{
			settings->tls_security = 0;
		}
		else if (wcscmp(L"--no-nla", argv[*pindex]) == 0)
		{
			settings->nla_security = 0;
		}
		else if (wcscmp(L"--sec", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing protocol security\n");
				return 1;
			}
			if (wcsncmp(L"rdp", argv[*pindex], 1) == 0) /* Standard RDP */
			{
				settings->rdp_security = 1;
				settings->tls_security = 0;
				settings->nla_security = 0;
			}
			else if (wcsncmp(L"tls", argv[*pindex], 1) == 0) /* TLS */
			{
				settings->rdp_security = 0;
				settings->tls_security = 1;
				settings->nla_security = 0;
			}
			else if (wcsncmp(L"nla", argv[*pindex], 1) == 0) /* NLA */
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
		else if (wcscmp(L"-plugin", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				printf("missing plugin name\n");
				return 1;
			}
			/* TODO: Handle --data ... -- */
			freerdp_chanman_load_plugin(chan_man, settings, argv[*pindex], NULL);
		}
		else if ((wcscmp(L"-h", argv[*pindex]) == 0) || wcscmp(L"--help", argv[*pindex]) == 0)
		{
			out_args();
			return 1;
		}
		else if (wcscmp(L"--version", argv[*pindex]) == 0)
		{
			printf("This is FreeRDP version %s\n", PACKAGE_VERSION);
			return 1;
		}
		else if (argv[*pindex][0] != L'-')
		{
			char *cp;
			char s[sizeof(settings->server)];
			WideCharToMultiByte(CP_ACP, 0, argv[*pindex], -1, s, sizeof(s) - 1, NULL, NULL);
			settings->server[sizeof(settings->server) - 1] = 0;
			if (s[0] == '[' && (cp = strchr(s, ']'))
				&& (cp[1] == 0 || (cp[1] == ':' && !strchr(cp + 2, ':'))))
			{
				/* Either "[...]" or "[...]:..." with at most one : after the brackets */
				strncpy(settings->server, s + 1, sizeof(settings->server) - 1);
				if ((cp = strchr(settings->server, ']')))
				{
					*cp = 0;
					if (cp[1] == ':')
						settings->tcp_port_rdp = atoi(cp + 2);
				}
			}
			else
			{
				/* Port number is cut off and used if exactly one : in the string */
				strncpy(settings->server, s, sizeof(settings->server) - 1);
				if ((cp = strchr(settings->server, ':')) && !strchr(cp + 1, ':'))
				{
					*cp = 0;
					settings->tcp_port_rdp = atoi(cp + 1);
				}
			}
			/* server is the last argument for the current session. arguments
			   followed will be parsed for the next session. */
			*pindex = *pindex + 1;
			return 0;
		}
		else
		{
			wprintf(L"invalid option: %s\n", argv[*pindex]);
			return 1;
		}
		*pindex = *pindex + 1;
	}
	printf("missing server name\n");
	return 1;
}

static DWORD WINAPI
run_wfreerdp(LPVOID lpParam)
{
	struct thread_data * data;
	rdpInst * inst;
	void * read_fds[32];
	void * write_fds[32];
	int read_count;
	int write_count;
	int index;
	HANDLE fds[64];
	int fds_count;

	printf("run_wfreerdp:\n");
	/* create an instance of the library */
	data = (struct thread_data *) lpParam;
	inst = freerdp_new(data->settings);
	if (inst == NULL)
	{
		printf("run_wfreerdp: freerdp_new failed\n");
		return 1;
	}
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
	printf("keyboard_layout: 0x%08X\n", inst->settings->keyboard_layout);

	if (wf_pre_connect(inst, data->hwnd) != 0)
	{
		printf("run_wfreerdp: wf_pre_connect failed\n");
		return 1;
	}
	if (freerdp_chanman_pre_connect(data->chan_man, inst) != 0)
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
	if (freerdp_chanman_post_connect(data->chan_man, inst) != 0)
	{
		printf("run_wfreerdp: freerdp_chanman_post_connect failed\n");
		return 1;
	}
	if (wf_post_connect(inst) != 0)
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
		if (freerdp_chanman_get_fds(data->chan_man, inst, read_fds, &read_count, write_fds, &write_count) != 0)
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
		if (WaitForMultipleObjects(fds_count, fds, FALSE, INFINITE) == WAIT_FAILED)
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
		if (freerdp_chanman_check_fds(data->chan_man, inst) != 0)
		{
			printf("run_wfreerdp: freerdp_chanman_check_fds failed\n");
			break;
		}
		wf_update_window(inst);
	}
	/* cleanup */
	freerdp_chanman_free(data->chan_man);
	free(data->settings);
	free(data);
	wf_uninit(inst);
	freerdp_free(inst);
	return 0;
}

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

INT WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wnd_cls;
	WSADATA wsa_data;
	struct thread_data * data;
	int rv;
	MSG msg;
	LPWSTR * argv;
	int argc;
	int index = 1;

	if (WSAStartup(0x101, &wsa_data) != 0)
	{
		return 1;
	}
	create_console();
	/*if (!freerdp_global_init())
	{
		printf("Error initializing freerdp\n");
		return 1;
	}*/
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

	argv = CommandLineToArgvW(GetCommandLine(), &argc);

	while (1)
	{
		data = (struct thread_data *) malloc(sizeof(struct thread_data));
		data->settings = (rdpSet *) malloc(sizeof(rdpSet));
		data->chan_man = freerdp_chanman_new();
		data->hwnd = NULL;

		rv = process_params(data->settings, data->chan_man, argc, argv, &index);
		if (rv == 0)
		{
			data->hwnd = CreateWindowEx(0, g_wnd_class_name, L"wfreerdp",
				WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT, NULL,
				NULL, g_hInstance, NULL);

			CreateThread(NULL, 0, run_wfreerdp, data, 0, NULL);
		}
		else
		{
			freerdp_chanman_free(data->chan_man);
			free(data->settings);
			free(data);
			break;
		}
	}

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	freerdp_chanman_uninit();
	//freerdp_global_finish();
	WSACleanup();
	return 0;
}
