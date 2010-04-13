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

#include "frdp.h"
#include "rdp.h"
#include "rdpset.h"
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

        /* Still lacking proper packet initialization */
	s = NULL;

        rdp_out_rail_pdu_header(s, RDP_RAIL_ORDER_EXEC, 12);

        out_uint16_le(s,
                RAIL_EXEC_FLAG_EXPAND_WORKINGDIRECTORY |
                RAIL_EXEC_FLAG_EXPAND_ARGUMENTS); // flags
        out_uint16_le(s, 2 * strlen(rdp->app->application_name)); // ExeOrFileLength
        out_uint16_le(s, 2 * strlen(rdp->app->working_directory)); // WorkingDirLength
        out_uint16_le(s, 2 * strlen(rdp->app->arguments)); // ArgumentsLength
        rdp_out_unistr(rdp, s, rdp->app->application_name); // ExeOrFile
        rdp_out_unistr(rdp, s, rdp->app->working_directory); // WorkingDir
        rdp_out_unistr(rdp, s, rdp->app->arguments); // Arguments

        s_mark_end(s);
	sec_send(rdp->sec, s, rdp->settings->encryption ? SEC_ENCRYPT : 0);
}

