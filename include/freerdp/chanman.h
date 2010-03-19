/*
   Copyright (c) 2009-2010 Jay Sorg
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

#ifndef __FREERDP_CHANMAN_H
#define __FREERDP_CHANMAN_H

#include <freerdp/freerdp.h>

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

#ifdef _WIN32
#define CHR TCHAR
#else
#define CHR char
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
	const CHR * filename);
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

#ifdef __cplusplus
}
#endif

#endif
