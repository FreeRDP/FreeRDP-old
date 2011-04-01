/*
   FreeRDP: A Remote Desktop Protocol client.
   FreeRDP Unit Tests

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
#include "CUnit/Basic.h"

#define add_test_suite(name) \
	CU_pSuite pSuite; \
	pSuite = CU_add_suite(#name, init_##name##_suite, clean_##name##_suite); \
	if (pSuite == NULL) { \
		CU_cleanup_registry(); return CU_get_error(); \
	}

#define add_test_function(name) \
	if (CU_add_test(pSuite, #name, test_##name) == NULL) { \
		CU_cleanup_registry(); return CU_get_error(); \
	}

void dump_data(unsigned char * p, int len, int width, char* name);
