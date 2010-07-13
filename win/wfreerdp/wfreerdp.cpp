/*
   Copyright (c) 2009 Jay Sorg
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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
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
	strcpy(settings->hostname, "test");
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
	settings->rdp5_performanceflags =
		RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG | RDP5_NO_MENUANIMATIONS;
	settings->off_screen_bitmaps = 1;
	settings->triblt = 0;
	settings->new_cursors = 1;
	settings->rdp_version = 5;
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
		return 1;
	}
	while (*pindex < argc)
	{
		if (wcscmp(L"-a", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				return 1;
			}
			settings->server_depth = _wtoi(argv[*pindex]);
		}
		else if (wcscmp(L"-u", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				return 1;
			}
			WideCharToMultiByte(CP_ACP, 0, argv[*pindex], -1, settings->username, 255, NULL, NULL);
			settings->username[255] = 0;
		}
		else if (wcscmp(L"-p", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				return 1;
			}
			WideCharToMultiByte(CP_ACP, 0, argv[*pindex], -1, settings->password, 63, NULL, NULL);
			settings->password[63] = 0;
			settings->autologin = 1;
		}
		else if (wcscmp(L"-g", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
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
				printf("invalid parameter\n");
				return 1;
			}
		}
		else if (wcscmp(L"-t", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				return 1;
			}
			settings->tcp_port_rdp = _wtoi(argv[*pindex]);
		}
		else if (wcscmp(L"-z", argv[*pindex]) == 0)
		{
			settings->bulk_compression = 1;
		}
		else if (wcscmp(L"-x", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				return 1;
			}
			if (wcsncmp(L"m", argv[*pindex], 1) == 0) /* modem */
			{
				settings->rdp5_performanceflags = RDP5_NO_WALLPAPER |
					RDP5_NO_FULLWINDOWDRAG |  RDP5_NO_MENUANIMATIONS |
					RDP5_NO_THEMING;
			}
			else if (wcsncmp(L"b", argv[*pindex], 1) == 0) /* broadband */
			{
				settings->rdp5_performanceflags = RDP5_NO_WALLPAPER;
			}
			else if (wcsncmp(L"l", argv[*pindex], 1) == 0) /* lan */
			{
				settings->rdp5_performanceflags = RDP5_DISABLE_NOTHING;
			}
			else
			{
				settings->rdp5_performanceflags = wcstol(argv[*pindex], 0, 16);
			}
		}
		else if (wcscmp(L"-plugin", argv[*pindex]) == 0)
		{
			*pindex = *pindex + 1;
			if (*pindex == argc)
			{
				return 1;
			}
			freerdp_chanman_load_plugin(chan_man, settings, argv[*pindex]);
		}
		else
		{
			WideCharToMultiByte(CP_ACP, 0, argv[*pindex], -1, settings->server, 63, NULL, NULL);
			settings->server[63] = 0;
			/* server is the last argument for the current session. arguments
			   followed will be parsed for the next session. */
			*pindex = *pindex + 1;
			return 0;
		}
		*pindex = *pindex + 1;
	}
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
	WSACleanup();
	return 0;
}
