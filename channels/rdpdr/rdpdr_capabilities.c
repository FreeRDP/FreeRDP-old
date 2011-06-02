/*
   FreeRDP: A Remote Desktop Protocol client.
   Device Redirection Capabilities

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "rdpdr_types.h"
#include "rdpdr_constants.h"
#include <freerdp/utils/stream.h>
#include <freerdp/utils/chan_plugin.h>
#include <freerdp/utils/unicode.h>
#include <freerdp/utils/wait_obj.h>

#include "rdpdr_capabilities.h"

/* Output device redirection capability set header */
void
rdpdr_out_capset_header(char* data, int size, uint16 capabilityType, uint16 capabilityLength, uint32 version)
{
	SET_UINT16(data, 0, capabilityType); /* capabilityType */
	SET_UINT16(data, 2, capabilityLength); /* capabilityLength */
	SET_UINT32(data, 4, version); /* version */
}

/* Output device direction general capability set */
int
rdpdr_out_general_capset(char* data, int size)
{
	SET_UINT32(data, 8, 0); /* osType, ignored on receipt */
	SET_UINT32(data, 12, 0); /* osVersion, unused and must be set to zero */
	SET_UINT16(data, 16, 1); /* protocolMajorVersion, must be set to 1 */
	SET_UINT16(data, 18, RDPDR_MINOR_RDP_VERSION_5_2); /* protocolMinorVersion */
	SET_UINT32(data, 20, 0x0000FFFF); /* ioCode1 */
	SET_UINT32(data, 24, 0); /* ioCode2, must be set to zero, reserved for future use */
	SET_UINT32(data, 28, RDPDR_DEVICE_REMOVE_PDUS | RDPDR_CLIENT_DISPLAY_NAME_PDU | RDPDR_USER_LOGGEDON_PDU); /* extendedPDU */
	SET_UINT32(data, 32, ENABLE_ASYNCIO); /* extraFlags1 */
	SET_UINT32(data, 36, 0); /* extraFlags2, must be set to zero, reserved for future use */
	SET_UINT32(data, 40, 0); /* SpecialTypeDeviceCap, number of special devices to be redirected before logon */

	rdpdr_out_capset_header(data, size,
		CAP_GENERAL_TYPE, 44, GENERAL_CAPABILITY_VERSION_02);

	return 44;
}

/* Process device direction general capability set */
int
rdpdr_process_general_capset(char* data, int size)
{
	uint16 capabilityLength;
	uint32 version;

	uint16 protocolMinorVersion;
	uint32 ioCode1;
	uint32 extendedPDU;
	uint32 extraFlags1;

	capabilityLength = GET_UINT16(data, 0); /* capabilityLength */
	version = GET_UINT32(data, 2); /* version */

	/* osType, ignored on receipt (4 bytes) */
	/* osVersion, unused and must be set to zero (4 bytes) */
	/* protocolMajorVersion, must be set to 1 (2 bytes) */
	protocolMinorVersion = GET_UINT16(data, 16); /* protocolMinorVersion */
	ioCode1 = GET_UINT32(data, 18); /* ioCode1 */
	/* ioCode2, must be set to zero, reserved for future use (4 bytes) */
	extendedPDU = GET_UINT32(data, 26); /* extendedPDU */
	extraFlags1 = GET_UINT32(data, 30); /* extraFlags1 */
	/* extraFlags2, must be set to zero, reserved for future use (4 bytes) */

	/*
	 * SpecialTypeDeviceCap (4 bytes):
	 * present when GENERAL_CAPABILITY_VERSION_02 is used
	 */

	if (version == GENERAL_CAPABILITY_VERSION_02)
	{
		uint32 specialTypeDeviceCap;
		specialTypeDeviceCap = GET_UINT32(data, 34);
	}

	return (int)capabilityLength;
}

/* Output printer direction capability set */
int
rdpdr_out_printer_capset(char* data, int size)
{
	rdpdr_out_capset_header(data, size,
		CAP_PRINTER_TYPE, 8, PRINT_CAPABILITY_VERSION_01);

	return 8;
}

/* Process printer direction capability set */
int
rdpdr_process_printer_capset(char* data, int size)
{
	uint16 capabilityLength;
	uint32 version;

	capabilityLength = GET_UINT16(data, 0); /* capabilityLength */
	version = GET_UINT32(data, 2); /* version */

	return (int)capabilityLength;
}

/* Output port redirection capability set */
int
rdpdr_out_port_capset(char* data, int size)
{
	rdpdr_out_capset_header(data, size,
		CAP_PORT_TYPE, 8, PORT_CAPABILITY_VERSION_01);

	return 8;
}

/* Process port redirection capability set */
int
rdpdr_process_port_capset(char* data, int size)
{
	uint16 capabilityLength;
	uint32 version;

	capabilityLength = GET_UINT16(data, 0); /* capabilityLength */
	version = GET_UINT32(data, 2); /* version */

	return (int)capabilityLength;
}

/* Output drive redirection capability set */
int
rdpdr_out_drive_capset(char* data, int size)
{
	rdpdr_out_capset_header(data, size,
		CAP_DRIVE_TYPE, 8, DRIVE_CAPABILITY_VERSION_02);

	return 8;
}

/* Process drive redirection capability set */
int
rdpdr_process_drive_capset(char* data, int size)
{
	uint16 capabilityLength;
	uint32 version;

	capabilityLength = GET_UINT16(data, 0); /* capabilityLength */
	version = GET_UINT32(data, 2); /* version */

	return (int)capabilityLength;
}

/* Output smart card redirection capability set */
int
rdpdr_out_smartcard_capset(char* data, int size)
{
	rdpdr_out_capset_header(data, size,
		CAP_SMARTCARD_TYPE, 8, SMARTCARD_CAPABILITY_VERSION_01);

	return 8;
}

/* Process smartcard redirection capability set */
int
rdpdr_process_smartcard_capset(char* data, int size)
{
	uint16 capabilityLength;
	uint32 version;

	capabilityLength = GET_UINT16(data, 0); /* capabilityLength */
	version = GET_UINT32(data, 2); /* version */

	return (int)capabilityLength;
}

void
rdpdr_process_capabilities(char* data, int size)
{
	int i;
	int offset;
	uint16 numCapabilities;
	uint16 capabilityType;

	numCapabilities = GET_UINT16(data, 0); /* numCapabilities */
	/* pad (2 bytes) */
	offset = 4;

	for(i = 0; i < numCapabilities; i++)
	{
		capabilityType = GET_UINT16(data, offset);

		switch (capabilityType)
		{
			case CAP_GENERAL_TYPE:
				offset += rdpdr_process_general_capset(&data[offset], size - offset);
				break;

			case CAP_PRINTER_TYPE:
				offset += rdpdr_process_printer_capset(&data[offset], size - offset);
				break;

			case CAP_PORT_TYPE:
				offset += rdpdr_process_port_capset(&data[offset], size - offset);
				break;

			case CAP_DRIVE_TYPE:
				offset += rdpdr_process_drive_capset(&data[offset], size - offset);
				break;

			case CAP_SMARTCARD_TYPE:
				offset += rdpdr_process_smartcard_capset(&data[offset], size - offset);
				break;

			default:
				//fprintf(stderr, "unimpl: Device redirection capability set type %d\n", capabilityType);
				break;
		}
	}
}
