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
*/

#ifndef __WAIT_OBJ_H
#define __WAIT_OBJ_H

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

#endif
