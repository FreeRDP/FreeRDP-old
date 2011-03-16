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

#include <freerdp/utils.h>
#include <freerdp/rdpset.h>
#include "frdp.h"
#include "rdp.h"
#include "secure.h"
#include "rail.h"

void
rdp_out_rail_pdu_header(STREAM s, uint16 orderType, uint16 orderLength)
{
	out_uint16_le(s, orderType); // orderType
	out_uint16_le(s, orderLength); // orderLength
}

void
rdp_send_client_execute_pdu(rdpRdp * rdp)
{
	STREAM s;
	size_t application_name_len, working_directory_len, arguments_len;
	char * application_name, * working_directory, * arguments;

	/* Still lacking proper packet initialization */
	s = NULL;
	rdp_out_rail_pdu_header(s, RDP_RAIL_ORDER_EXEC, 12);

	application_name = freerdp_uniconv_out(rdp->uniconv,
			rdp->app->application_name, &application_name_len);
	working_directory = freerdp_uniconv_out(rdp->uniconv,
			rdp->app->working_directory, &working_directory_len);
	arguments = freerdp_uniconv_out(rdp->uniconv,
			rdp->app->arguments, &arguments_len);

	out_uint16_le(s,
			RAIL_EXEC_FLAG_EXPAND_WORKINGDIRECTORY |
			RAIL_EXEC_FLAG_EXPAND_ARGUMENTS); // flags
	out_uint16_le(s, application_name_len); // ExeOrFileLength
	out_uint16_le(s, working_directory_len); // WorkingDirLength
	out_uint16_le(s, arguments_len); // ArgumentsLength
	out_uint8a(s, application_name, application_name_len + 2); // ExeOrFile
	out_uint8a(s, working_directory, working_directory_len + 2); // WorkingDir
	out_uint8a(s, arguments, arguments_len + 2); // Arguments

	xfree(application_name);
	xfree(working_directory);
	xfree(arguments);

	s_mark_end(s);
	sec_send(rdp->sec, s, rdp->settings->encryption ? SEC_ENCRYPT : 0);
}
