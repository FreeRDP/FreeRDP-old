/*
   FreeRDP: A Remote Desktop Protocol client.

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
#include <netdb.h>
#include <unistd.h>
#include <freerdp/utils.h>

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

struct wait_obj
{
	int pipe_fd[2];
};

struct wait_obj *
wait_obj_new(const char * name)
{
	struct wait_obj * obj;

	obj = (struct wait_obj *) malloc(sizeof(struct wait_obj));

	obj->pipe_fd[0] = -1;
	obj->pipe_fd[1] = -1;
	if (pipe(obj->pipe_fd) < 0)
	{
		LLOGLN(0, ("init_wait_obj: pipe failed"));
		free(obj);
		return NULL;
	}
	return obj;
}

int
wait_obj_free(struct wait_obj * obj)
{
	if (obj)
	{
		if (obj->pipe_fd[0] != -1)
		{
			close(obj->pipe_fd[0]);
			obj->pipe_fd[0] = -1;
		}
		if (obj->pipe_fd[1] != -1)
		{
			close(obj->pipe_fd[1]);
			obj->pipe_fd[1] = -1;
		}
		free(obj);
	}
	return 0;
}

int
wait_obj_is_set(struct wait_obj * obj)
{
	fd_set rfds;
	int num_set;
	struct timeval time;

	FD_ZERO(&rfds);
	FD_SET(obj->pipe_fd[0], &rfds);
	memset(&time, 0, sizeof(time));
	num_set = select(obj->pipe_fd[0] + 1, &rfds, 0, 0, &time);
	return (num_set == 1);
}

int
wait_obj_set(struct wait_obj * obj)
{
	int len;

	if (wait_obj_is_set(obj))
	{
		return 0;
	}
	len = write(obj->pipe_fd[1], "sig", 4);
	if (len != 4)
	{
		LLOGLN(0, ("set_wait_obj: error"));
		return 1;
	}
	return 0;
}

int
wait_obj_clear(struct wait_obj * obj)
{
	int len;

	while (wait_obj_is_set(obj))
	{
		len = read(obj->pipe_fd[0], &len, 4);
		if (len != 4)
		{
			LLOGLN(0, ("chan_man_clear_ev: error"));
			return 1;
		}
	}
	return 0;
}

int
wait_obj_select(struct wait_obj ** listobj, int numobj, int * listr, int numr,
	int timeout)
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
	if (listobj)
	{
		for (index = 0; index < numobj; index++)
		{
			sock = listobj[index]->pipe_fd[0];
			FD_SET(sock, &fds);
			if (sock > max)
			{
				max = sock;
			}
		}
	}
	if (listr)
	{
		for (index = 0; index < numr; index++)
		{
			sock = listr[index];
			FD_SET(sock, &fds);
			if (sock > max)
			{
				max = sock;
			}
		}
	}
	rv = select(max + 1, &fds, 0, 0, ptime);
	return rv;
}

