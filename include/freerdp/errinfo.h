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

#ifndef __ERRINFO_H
#define __ERRINFO_H


/* These error codes can be found in
   [MS-RDPBCGR] 2.2.5.1.1 Set Error Info PDU Data */

/* protocol-independent codes */
#define ERRINFO_RPC_INITIATED_DISCONNECT            0x00000001
#define ERRINFO_RPC_INITIATED_LOGOFF                0x00000002
#define ERRINFO_IDLE_TIMEOUT                        0x00000003
#define ERRINFO_LOGON_TIMEOUT                       0x00000004
#define ERRINFO_DISCONNECTED_BY_OTHERCONNECTION     0x00000005
#define ERRINFO_OUT_OF_MEMORY                       0x00000006
#define ERRINFO_SERVER_DENIED_CONNECTION            0x00000007
#define ERRINFO_SERVER_DENIED_CONNECTION_FIPS       0x00000008
#define ERRINFO_SERVER_INSUFFICIENT_PRIVILEGES      0x00000009
#define ERRINFO_SERVER_FRESH_CREDENTIALS_REQUIRED   0x0000000A
#define ERRINFO_RPC_INITIATED_DISCONNECT_BYUSER     0x0000000B

/* protocol-independent licensing codes */
#define ERRINFO_LICENSE_INTERNAL                    0x00000100
#define ERRINFO_LICENSE_NO_LICENSE_SERVER           0x00000101
#define ERRINFO_LICENSE_NO_LICENSE                  0x00000102
#define ERRINFO_LICENSE_BAD_CLIENT_MSG              0x00000103
#define ERRINFO_LICENSE_HWID_DOESNT_MATCH_LICENSE   0x00000104
#define ERRINFO_LICENSE_BAD_CLIENT_LICENSE          0x00000105
#define ERRINFO_LICENSE_CANT_FINISH_PROTOCOL        0x00000106
#define ERRINFO_LICENSE_CLIENT_ENDED_PROTOCOL       0x00000107
#define ERRINFO_LICENSE_BAD_CLIENT_ENCRYPTION       0x00000108
#define ERRINFO_LICENSE_CANT_UPGRADE_LICENSE        0x00000109
#define ERRINFO_LICENSE_NO_REMOTE_CONNECTIONS       0x0000010A

/* TODO: define RDP specific codes */


#include <stddef.h>

#define ERRINFO_BUFFER_SIZE 80

/* Thread safe disconnect reason to string description */
char *
freerdp_str_disconnect_reason(unsigned int reason, char *msgbuf, size_t buflen);

#endif
