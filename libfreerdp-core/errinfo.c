/*
   FreeRDP: A Remote Desktop Protocol client.
   Error information

   Copyright 2010 O.S. Systems Software Ltda.
   Copyright 2010 Eduardo Fiss Beloni <beloni@ossystems.com.br>

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

#include <freerdp/errinfo.h>
#include <stdio.h>
#include <string.h>

/* Returns the message that matches given reason based on the Error Info PDU set */

char *
freerdp_str_disconnect_reason(unsigned int reason, char *msgbuf, size_t buflen)
{
	char *pmsg;

	switch (reason)
	{
		case 0:
			pmsg = "Success";
			break;

		case ERRINFO_RPC_INITIATED_DISCONNECT:
			pmsg = "Disconnect by server";
			break;

		case ERRINFO_RPC_INITIATED_LOGOFF:
			pmsg = "Logoff by server";
			break;

		case ERRINFO_IDLE_TIMEOUT:
			pmsg = "Server idle timeout reached";
			break;

		case ERRINFO_LOGON_TIMEOUT:
			pmsg = "Server logon timeout reached";
			break;

		case ERRINFO_DISCONNECTED_BY_OTHERCONNECTION:
			pmsg = "The session has been replaced";
			break;

		case ERRINFO_OUT_OF_MEMORY:
			pmsg = "The server ran out of memory";
			break;

		case ERRINFO_SERVER_DENIED_CONNECTION:
		case ERRINFO_SERVER_DENIED_CONNECTION_FIPS:
			pmsg = "The server denied the connection";
			break;

		case ERRINFO_SERVER_INSUFFICIENT_PRIVILEGES:
			pmsg = "User cannot connect to the server due to insufficient privileges";
			break;

		case ERRINFO_SERVER_FRESH_CREDENTIALS_REQUIRED:
			pmsg = "Server doesn't accept user credentials";
			break;

		case ERRINFO_RPC_INITIATED_DISCONNECT_BYUSER:
			pmsg = "Disconnected by an administrative tool in the server";
			break;

		case ERRINFO_LICENSE_INTERNAL:
			pmsg = "Internal licensing error";
			break;

		case ERRINFO_LICENSE_NO_LICENSE_SERVER:
			pmsg = "No license server available";
			break;

		case ERRINFO_LICENSE_NO_LICENSE:
			pmsg = "No valid license available";
			break;

		case ERRINFO_LICENSE_BAD_CLIENT_MSG:
			pmsg = "Invalid licensing message";
			break;

		case ERRINFO_LICENSE_HWID_DOESNT_MATCH_LICENSE:
			pmsg = "Hardware id doesn't match software license";
			break;

		case ERRINFO_LICENSE_BAD_CLIENT_LICENSE:
			pmsg = "Client license error";
			break;

		case ERRINFO_LICENSE_CANT_FINISH_PROTOCOL:
			pmsg = "Network error during licensing protocol";
			break;

		case ERRINFO_LICENSE_CLIENT_ENDED_PROTOCOL:
			pmsg = "Licensing protocol was not completed";
			break;

		case ERRINFO_LICENSE_BAD_CLIENT_ENCRYPTION:
			pmsg = "Incorrect client license enryption";
			break;

		case ERRINFO_LICENSE_CANT_UPGRADE_LICENSE:
			pmsg = "Can't upgrade license";
			break;

		case ERRINFO_LICENSE_NO_REMOTE_CONNECTIONS:
			pmsg = "The server is not licensed to accept remote connections";
			break;

		default:
			/* RDP specific error reason can't be handled individually yet */
			if (reason >= 0x10c9 && reason <= 0x1193)
				snprintf(msgbuf, buflen, "RDP protocol error reason 0x%x", reason);
			else
				snprintf(msgbuf, buflen, "Unknown error reason 0x%x", reason);

			return msgbuf;
	}

	strncpy(msgbuf, pmsg, buflen);

	return msgbuf;
}
