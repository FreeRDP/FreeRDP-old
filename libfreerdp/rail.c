/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Protocol services - Remote Applications Integrated Locally (RAIL)

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2009

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <freerdp/rdpset.h>
#include "frdp.h"
#include "rdp.h"
#include "secure.h"
#include "mem.h"
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

	application_name = xstrdup_out_unistr(rdp,
			rdp->app->application_name, &application_name_len);
	working_directory = xstrdup_out_unistr(rdp,
			rdp->app->working_directory, &working_directory_len);
	arguments = xstrdup_out_unistr(rdp,
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
