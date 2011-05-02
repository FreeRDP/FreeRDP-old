/*
   FreeRDP: A Remote Desktop Protocol client.
   GDI Window Routines Unit Tests

   Copyright 2010 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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
#include <freerdp/freerdp.h>
#include "gdi.h"
#include "gdi_window.h"
#include "test_gdi_window.h"

GDI *gdi;
rdpInst *inst;

int test_width = 1024;
int test_height = 768;
int test_depth = 32;

int init_gdi_window_suite(void)
{
	inst = (rdpInst*) malloc(sizeof(rdpInst));
	memset(inst, 0, sizeof(rdpInst));

	inst->settings = (rdpSet*) malloc(sizeof(rdpSet));
	memset(inst->settings, 0, sizeof(rdpSet));

	inst->settings->width = test_width;
	inst->settings->height = test_height;
	inst->settings->server_depth = test_depth;

	return 0;
}

int clean_gdi_window_suite(void)
{
	free(inst->settings);
	free(inst);

	return 0;
}

int add_gdi_window_suite(void)
{
	add_test_suite(gdi_window);

	add_test_function(gdi_init);

	return 0;
}

void test_gdi_init(void)
{
	CU_ASSERT(gdi_init(inst, CLRCONV_ALPHA) == 0);

	gdi = GET_GDI(inst);
	CU_ASSERT(gdi != NULL);

	CU_ASSERT(gdi->width == test_width);
	CU_ASSERT(gdi->height == test_height);
	CU_ASSERT(gdi->srcBpp == test_depth);
}
