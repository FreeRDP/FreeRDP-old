/*
   FreeRDP: A Remote Desktop Protocol client.
   Remote Applications Integrated Locally (RAIL)

   Copyright 2009 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __RAIL_H
#define	__RAIL_H

#include "rdp.h"

struct rdp_app
{
	char application_name[260];
	char working_directory[260];
	char arguments[260];
};
typedef struct rdp_app rdpApp;

void
rdp_send_client_execute_pdu(rdpRdp * rdp);

#endif	// __RAIL_H
