/*
   FreeRDP: A Remote Desktop Protocol client.
   Channels

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
*/

/* leave this __MEM1_H, __MEM_H has a conflict */
#ifndef __MEM1_H
#define __MEM1_H

void *
xmalloc(int size);
void *
xrealloc(void * oldmem, int size);
void
xfree(void * mem);
char *
xstrdup(const char * s);

#endif
