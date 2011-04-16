/*
   FreeRDP: A Remote Desktop Protocol client.
   Virtual Channel Manager

   Copyright 2009-2011 Jay Sorg
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

#ifndef __FREERDP_CHANMAN_H
#define __FREERDP_CHANMAN_H

#include "freerdp.h"

#if defined _WIN32 || defined __CYGWIN__
  #ifdef FREERDP_CHANMAN_EXPORTS
    #ifdef __GNUC__
      #define FREERDP_CHANMAN_API __attribute__((dllexport))
    #else
      #define FREERDP_CHANMAN_API __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define FREERDP_CHANMAN_API __attribute__((dllimport))
    #else
      #define FREERDP_CHANMAN_API __declspec(dllimport)
    #endif
  #endif
#else
  #if __GNUC__ >= 4
    #define FREERDP_CHANMAN_API   __attribute__ ((visibility("default")))
  #else
    #define FREERDP_CHANMAN_API
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rdp_chan_man rdpChanMan;

FREERDP_CHANMAN_API int
freerdp_chanman_init(void);
FREERDP_CHANMAN_API int
freerdp_chanman_uninit(void);
FREERDP_CHANMAN_API rdpChanMan *
freerdp_chanman_new(void);
FREERDP_CHANMAN_API void
freerdp_chanman_free(rdpChanMan * chan_man);
FREERDP_CHANMAN_API int
freerdp_chanman_load_plugin(rdpChanMan * chan_man, rdpSet * settings,
	const char * filename, void * data);
FREERDP_CHANMAN_API int
freerdp_chanman_pre_connect(rdpChanMan * chan_man, rdpInst * inst);
FREERDP_CHANMAN_API int
freerdp_chanman_post_connect(rdpChanMan * chan_man, rdpInst * inst);
FREERDP_CHANMAN_API int
freerdp_chanman_data(rdpInst * inst, int chan_id, char * data, int data_size,
	int flags, int total_size);
FREERDP_CHANMAN_API int
freerdp_chanman_get_fds(rdpChanMan * chan_man, rdpInst * inst, void ** read_fds,
	int * read_count, void ** write_fds, int * write_count);
FREERDP_CHANMAN_API int
freerdp_chanman_check_fds(rdpChanMan * chan_man, rdpInst * inst);
FREERDP_CHANMAN_API RD_EVENT *
freerdp_chanman_pop_event(rdpChanMan * chan_man);
FREERDP_CHANMAN_API void
freerdp_chanman_free_event(rdpChanMan * chan_man, RD_EVENT * event);
FREERDP_CHANMAN_API void
freerdp_chanman_close(rdpChanMan * chan_man, rdpInst * inst);

#ifdef __cplusplus
}
#endif

#endif
