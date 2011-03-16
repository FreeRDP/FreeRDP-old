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

#endif /* __LIBFREERDPUTILS_H */
