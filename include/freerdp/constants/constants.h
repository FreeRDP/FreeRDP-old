/*
   FreeRDP: A Remote Desktop Protocol client.
   Miscellaneous protocol constants

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

#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#include <freerdp/constants/ui.h>
#include <freerdp/constants/core.h>
#include <freerdp/constants/crypto.h>
#include <freerdp/constants/license.h>
#include <freerdp/constants/pdu.h>
#include <freerdp/constants/rail.h>
#include <freerdp/constants/window.h>
#include <freerdp/constants/capabilities.h>

/* MCSPDU choice connect-initial, Application tag 101 */
#define MCS_CONNECT_INITIAL	0x7f65
/* MCSPDU choice connect-response, Application tag 102 */
#define MCS_CONNECT_RESPONSE	0x7f66
#define MCS_TAG_DOMAIN_PARAMS	0x30
#define MCS_GLOBAL_CHANNEL	1003
#define MCS_USERCHANNEL_BASE    1001

#define BER_TAG_BOOLEAN		1
#define BER_TAG_INTEGER		2
#define BER_TAG_OCTET_STRING	4
#define BER_TAG_RESULT		10

#define MIX_TRANSPARENT	0
#define MIX_OPAQUE	1

#define TEXT2_VERTICAL		0x04
#define TEXT2_IMPLICIT_X	0x20

/* RDP bitmap cache (version 2) constants */
#define BMPCACHE2_C0_CELLS	0x78
#define BMPCACHE2_C1_CELLS	0x78
#define BMPCACHE2_C2_CELLS	0x150
#define BMPCACHE2_NUM_PSTCELLS	0x9f6

/* User Data Header Types */
#define CS_CORE         0xC001
#define CS_SECURITY     0xC002
#define CS_NET          0xC003
#define CS_CLUSTER      0xC004
#define CS_MONITOR      0xC005
#define SC_CORE         0x0C01
#define SC_SECURITY     0x0C02
#define SC_NET          0x0C03

#define BMPCACHE2_FLAG_PERSIST	((uint32)1<<31)

#define RDP_SOURCE		"MSTSC"

#endif // __CONSTANTS_H
