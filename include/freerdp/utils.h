/*
   FreeRDP: A Remote Desktop Protocol client.
   Utility Library

   Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __LIBFREERDPUTILS_H
#define __LIBFREERDPUTILS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

/* semaphore utils */

void freerdp_sem_create(void * sem_struct, int iv);
void freerdp_sem_signal(void * sem_struct);
void freerdp_sem_wait(void * sem_struct);

/* memory utils */

void* xmalloc(size_t size);
void* xrealloc(void * oldmem, size_t size);
void xfree(void * mem);
char* xstrdup(const char * s);

/* datablob utils */

struct _DATABLOB
{
	void* data;
	int length;
};
typedef struct _DATABLOB DATABLOB;

void datablob_alloc(DATABLOB *datablob, int length);
void datablob_free(DATABLOB *datablob);

/* unicode utils */

#define DEFAULT_CODEPAGE	"UTF-8"
#define WINDOWS_CODEPAGE	"UTF-16LE"

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#ifndef ICONV_CONST
#define ICONV_CONST ""
#endif

struct _UNICONV
{
	int iconv;
#ifdef HAVE_ICONV
	iconv_t* in_iconv_h;	/* non-thread-safe converter to DEFAULT_CODEPAGE from WINDOWS_CODEPAGE */
	iconv_t* out_iconv_h;	/* non-thread-safe converter to WINDOWS_CODEPAGE from DEFAULT_CODEPAGE */
#endif
};
typedef struct _UNICONV UNICONV;

UNICONV* freerdp_uniconv_new();
void freerdp_uniconv_free(UNICONV *uniconv);
char* freerdp_uniconv_in(UNICONV *uniconv, unsigned char* pin, size_t in_len);
char* freerdp_uniconv_out(UNICONV *uniconv, char *str, size_t *pout_len);

/* stream macros */

#define GET_UINT8(_p1, _offset) *(((uint8 *) _p1) + _offset)
#define GET_UINT16(_p1, _offset) ( \
	(uint16) (*(((uint8 *) _p1) + _offset)) + \
	((uint16) (*(((uint8 *) _p1) + _offset + 1)) << 8))
#define GET_UINT32(_p1, _offset) ( \
	(uint32) (*(((uint8 *) _p1) + _offset)) + \
	((uint32) (*(((uint8 *) _p1) + _offset + 1)) << 8) + \
	((uint32) (*(((uint8 *) _p1) + _offset + 2)) << 16) + \
	((uint32) (*(((uint8 *) _p1) + _offset + 3)) << 24))
#define GET_UINT64(_p1, _offset) ( \
	(uint64) (*(((uint8 *) _p1) + _offset)) + \
	((uint64) (*(((uint8 *) _p1) + _offset + 1)) << 8) + \
	((uint64) (*(((uint8 *) _p1) + _offset + 2)) << 16) + \
	((uint64) (*(((uint8 *) _p1) + _offset + 3)) << 24) + \
	((uint64) (*(((uint8 *) _p1) + _offset + 4)) << 32) + \
	((uint64) (*(((uint8 *) _p1) + _offset + 5)) << 40) + \
	((uint64) (*(((uint8 *) _p1) + _offset + 6)) << 48) + \
	((uint64) (*(((uint8 *) _p1) + _offset + 7)) << 56))

#define SET_UINT8(_p1, _offset, _value) *(((uint8 *) _p1) + _offset) = (uint8) (_value)
#define SET_UINT16(_p1, _offset, _value) \
	*(((uint8 *) _p1) + _offset) = (uint8) (((uint16) (_value)) & 0xff), \
	*(((uint8 *) _p1) + _offset + 1) = (uint8) ((((uint16) (_value)) >> 8) & 0xff)
#define SET_UINT32(_p1, _offset, _value) \
	*(((uint8 *) _p1) + _offset) = (uint8) (((uint32) (_value)) & 0xff), \
	*(((uint8 *) _p1) + _offset + 1) = (uint8) ((((uint32) (_value)) >> 8) & 0xff), \
	*(((uint8 *) _p1) + _offset + 2) = (uint8) ((((uint32) (_value)) >> 16) & 0xff), \
	*(((uint8 *) _p1) + _offset + 3) = (uint8) ((((uint32) (_value)) >> 24) & 0xff)
#define SET_UINT64(_p1, _offset, _value) \
	*(((uint8 *) _p1) + _offset) = (uint8) (((uint64) (_value)) & 0xff), \
	*(((uint8 *) _p1) + _offset + 1) = (uint8) ((((uint64) (_value)) >> 8) & 0xff), \
	*(((uint8 *) _p1) + _offset + 2) = (uint8) ((((uint64) (_value)) >> 16) & 0xff), \
	*(((uint8 *) _p1) + _offset + 3) = (uint8) ((((uint64) (_value)) >> 24) & 0xff), \
	*(((uint8 *) _p1) + _offset + 4) = (uint8) ((((uint64) (_value)) >> 32) & 0xff), \
	*(((uint8 *) _p1) + _offset + 5) = (uint8) ((((uint64) (_value)) >> 40) & 0xff), \
	*(((uint8 *) _p1) + _offset + 6) = (uint8) ((((uint64) (_value)) >> 48) & 0xff), \
	*(((uint8 *) _p1) + _offset + 7) = (uint8) ((((uint64) (_value)) >> 56) & 0xff)

/* wait_obj */

struct wait_obj *
wait_obj_new(const char * name);
int
wait_obj_free(struct wait_obj * obj);
int
wait_obj_is_set(struct wait_obj * obj);
int
wait_obj_set(struct wait_obj * obj);
int
wait_obj_clear(struct wait_obj * obj);
int
wait_obj_select(struct wait_obj ** listobj, int numobj, int * listr, int numr,
	int timeout);

/* channel plugin base class */

typedef struct rdp_chan_plugin rdpChanPlugin;
struct rdp_chan_plugin
{
	void * init_handle;
	int open_handle[30];
	int num_open_handles;
};

void
chan_plugin_init(rdpChanPlugin * chan_plugin);
void
chan_plugin_uninit(rdpChanPlugin * chan_plugin);
int
chan_plugin_register_open_handle(rdpChanPlugin * chan_plugin,
	int open_handle);
int
chan_plugin_unregister_open_handle(rdpChanPlugin * chan_plugin,
	int open_handle);
rdpChanPlugin *
chan_plugin_find_by_init_handle(void * init_handle);
rdpChanPlugin *
chan_plugin_find_by_open_handle(int open_handle);

#endif /* __LIBFREERDPUTILS_H */
