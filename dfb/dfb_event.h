/*
   FreeRDP: A Remote Desktop Protocol client.
   DirectFB UI Event Handling

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

#ifndef __DFB_EVENT_H
#define __DFB_EVENT_H

#include <directfb.h>
#include <freerdp/freerdp.h>

int
dfb_process_event(rdpInst * inst, DFBEvent * event);

#endif /* __DFB_EVENT_H */
