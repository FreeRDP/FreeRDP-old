/*
   FreeRDP: A Remote Desktop Protocol client.
   usleep implementation

   Copyright 2011 Petr Stetiar <ynezz@true.cz>

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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#endif

#include <time.h>

#include <freerdp/types/base.h>
#include <freerdp/utils/usleep.h>

void freerdp_usleep(uint32 delay)
{
#ifdef WIN32
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = delay;
	select(0, NULL, NULL, NULL, &tv);
#else
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = delay * 1000;
	nanosleep(&ts, NULL);
#endif
}
