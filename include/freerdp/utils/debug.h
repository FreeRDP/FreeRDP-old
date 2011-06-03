/*
   FreeRDP: A Remote Desktop Protocol client.
   Debug Macros

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

#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WITH_DEBUG_ASSERT
#include <assert.h>
#define ASSERT(a)	assert(a)
#else
#define ASSERT(a)	do { } while (0)
#endif

#define DEBUG_NULL(fmt, ...) do { } while (0)
#define DEBUG_PRINT(str, fmt, ...) printf(str fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#define DEBUG_CLASS(class, fmt, ...) DEBUG_PRINT("DBG_##class %s (%d): ", fmt, ...)

#ifdef WITH_DEBUG
#define DEBUG(fmt, ...)	printf("DBG %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG(fmt, ...) DEBUG_NULL(fmt, ...)
#endif

#endif /* __DEBUG_H */
