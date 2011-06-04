/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP Licensing Constants

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

#ifndef __CONSTANTS_LICENSE_H
#define __CONSTANTS_LICENSE_H

/* RDP licensing constants */
#define LICENSE_TOKEN_SIZE     10
#define LICENSE_HWID_SIZE      20
#define LICENSE_SIGNATURE_SIZE 16

#define LICENSE_REQUEST             0x01
#define LICENSE_PLATFORM_CHALLENGE  0x02
#define NEW_LICENSE                 0x03
#define UPGRADE_LICENSE             0x04
#define LICENSE_INFO                0x12
#define NEW_LICENSE_REQUEST         0x13
#define PLATFORM_CHALLENGE_RESPONSE 0x15
#define LICENSE_ERROR_ALERT         0xff

#define LICENSE_TAG_USER    0x000f
#define LICENSE_TAG_HOST    0x0010

#endif /* __CONSTANTS_LICENSE_H */
