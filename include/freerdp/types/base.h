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

   these are the types needed for the ui interface
   self contained file, requires no other includes

*/

#ifndef __TYPES_BASE_H
#define __TYPES_BASE_H

#ifndef True
#define True  (1)
#define False (0)
#endif

typedef unsigned char uint8;
typedef signed char sint8;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned int uint32;
typedef signed int sint32;
#ifdef _WIN32
typedef unsigned __int64 uint64;
typedef signed __int64 sint64;
#else
typedef unsigned long long uint64;
typedef signed long long sint64;
#endif

#endif

