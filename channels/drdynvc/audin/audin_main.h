/*
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Device Manager

   Copyright 2010-2011 Vic Lee

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

#ifndef __AUDIN_MAIN_H
#define __AUDIN_MAIN_H

#include <freerdp/types_ui.h>

typedef int (*wave_in_receive_func) (char * wave_data, int size, void * user_data);

void *
wave_in_new(void);
void
wave_in_free(void * device_data);
int
wave_in_format_supported(void * device_data, char * snd_format, int size);
int
wave_in_set_format(void * device_data, uint32 FramesPerPacket, char * snd_format, int size);
int
wave_in_open(void * device_data, wave_in_receive_func receive_func, void * user_data);
int
wave_in_close(void * device_data);

#endif
