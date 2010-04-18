/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   RAIL (Remote Application Integrated Locally) constants

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

#ifndef __CONSTANTS_RDPDR_H
#define __CONSTANTS_RDPDR_H

/* NT status codes for RDPDR */
#define RD_STATUS_SUCCESS                  0x00000000
#define RD_STATUS_NOT_IMPLEMENTED          0x00000001
#define RD_STATUS_PENDING                  0x00000103

#define RD_STATUS_NO_MORE_FILES            0x80000006
#define RD_STATUS_DEVICE_PAPER_EMPTY       0x8000000e
#define RD_STATUS_DEVICE_POWERED_OFF       0x8000000f
#define RD_STATUS_DEVICE_OFF_LINE          0x80000010
#define RD_STATUS_DEVICE_BUSY              0x80000011

#define RD_STATUS_INVALID_HANDLE           0xc0000008
#define RD_STATUS_INVALID_PARAMETER        0xc000000d
#define RD_STATUS_NO_SUCH_FILE             0xc000000f
#define RD_STATUS_INVALID_DEVICE_REQUEST   0xc0000010
#define RD_STATUS_ACCESS_DENIED            0xc0000022
#define RD_STATUS_OBJECT_NAME_COLLISION    0xc0000035
#define RD_STATUS_DISK_FULL                0xc000007f
#define RD_STATUS_FILE_IS_A_DIRECTORY      0xc00000ba
#define RD_STATUS_NOT_SUPPORTED            0xc00000bb
#define RD_STATUS_TIMEOUT                  0xc0000102
#define RD_STATUS_NOTIFY_ENUM_DIR          0xc000010c
#define RD_STATUS_CANCELLED                0xc0000120

					       // <le> <be> <meaning>
#define RDPDR_COMPONENT_TYPE_CORE	0x4472 // "sD" "Ds" ???
#define RDPDR_COMPONENT_TYPE_PRINTING	0x5052 // "RP" "PR" (PR)inting

#define PAKID_CORE_SERVER_ANNOUNCE	0x496E // "nI" "nI" ???
#define PAKID_CORE_CLIENTID_CONFIRM	0x4343 // "CC" "CC" (C)lientID (C)onfirm
#define PAKID_CORE_CLIENT_NAME		0x434E // "NC" "CN" (C)lient (N)ame
#define PAKID_CORE_DEVICELIST_ANNOUNCE	0x4441 // "AD" "DA" (D)evice (A)nnounce
#define PAKID_CORE_DEVICE_REPLY		0x6472 // "rd" "dr" (d)evice (r)eply
#define PAKID_CORE_DEVICE_IOREQUEST	0x4952 // "RI" "IR" (I)O (R)equest
#define PAKID_CORE_DEVICE_IOCOMPLETION	0x4943 // "CI" "IC" (I)O (C)ompletion
#define PAKID_CORE_SERVER_CAPABILITY	0x5350 // "PS" "SP" (S)erver (C)apability
#define PAKID_CORE_CLIENT_CAPABILITY	0x4350 // "PC" "CP" (C)lient (C)apability
#define PAKID_CORE_DEVICELIST_REMOVE	0x444D // "MD" "DM" (D)evice list (R)emove
#define PAKID_PRN_CACHE_DATA		0x5043 // "CP" "PC" (P)rinter (C)ache data
#define PAKID_CORE_USER_LOGGEDON	0x554C // "LU" "UL" (U)ser (L)ogged on
#define PAKID_PRN_USING_XPS		0x5543 // "CU" "UC" (U)sing (?)XPS

#define RDPDR_MAX_DEVICES               0x10

#define DEVICE_TYPE_SERIAL              0x00000001
#define DEVICE_TYPE_PARALLEL            0x00000002
#define DEVICE_TYPE_PRINTER             0x00000004
#define DEVICE_TYPE_DISK                0x00000008
#define DEVICE_TYPE_SMARTCARD		0x00000020

#define IRP_MJ_CREATE			0x00000000
#define IRP_MJ_CLOSE			0x00000002
#define IRP_MJ_READ			0x00000003
#define IRP_MJ_WRITE			0x00000004
#define	IRP_MJ_QUERY_INFORMATION	0x00000005
#define IRP_MJ_SET_INFORMATION		0x00000006
#define IRP_MJ_QUERY_VOLUME_INFORMATION	0x0000000A
#define IRP_MJ_DIRECTORY_CONTROL	0x0000000C
#define IRP_MJ_DEVICE_CONTROL		0x0000000E
#define IRP_MJ_LOCK_CONTROL             0x00000011

#define IRP_MN_QUERY_DIRECTORY          0x00000001
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY  0x00000002

#define FILE_DIRECTORY_FILE             0x00000001
#define FILE_NON_DIRECTORY_FILE         0x00000040
#define FILE_COMPLETE_IF_OPLOCKED       0x00000100
#define FILE_DELETE_ON_CLOSE            0x00001000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY  0x00800000

#define RDPDR_PRINTER_ANNOUNCE_FLAG_ASCII		0x00000001
#define RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER	0x00000002
#define RDPDR_PRINTER_ANNOUNCE_FLAG_NETWORKPRINTER	0x00000004
#define RDPDR_PRINTER_ANNOUNCE_FLAG_TSPRINTER		0x00000008
#define RDPDR_PRINTER_ANNOUNCE_FLAG_XPSFORMAT		0x00000010

/* Device redirection capability set types */
#define DR_CAPSET_TYPE_GENERAL			0x0001
#define DR_CAPSET_TYPE_PRINTER			0x0002
#define DR_CAPSET_TYPE_PORT			0x0003
#define DR_CAPSET_TYPE_DRIVE			0x0004
#define DR_CAPSET_TYPE_SMARTCARD		0x0005

#define DR_CAPSET_LEN_GENERAL			0x0028
#define DR_CAPSET_LEN_PRINTER			0x0008
#define DR_CAPSET_LEN_PORT			0x0008
#define DR_CAPSET_LEN_DRIVE			0x0008
#define DR_CAPSET_LEN_SMARTCARD			0x0008

/* Device redirection general capability set */
#define DR_GENERAL_CAPABILITY_VERSION_01	0x00000001
#define DR_GENERAL_CAPABILITY_VERSION_02	0x00000002

#define DR_MINOR_RDP_VERSION_5_0		0x0002
#define DR_MINOR_RDP_VERSION_5_1		0x0005
#define DR_MINOR_RDP_VERSION_5_2		0x000A
#define DR_MINOR_RDP_VERSION_6_X		0x000C

#define DR_DEVICE_REMOVE_PDUS			0x00000001
#define	DR_CLIENT_DISPLAY_NAME_PDU		0x00000002
#define DR_USER_LOGGEDON_PDU			0x00000004
#define DR_ENABLE_ASYNCIO			0x00000000

#endif /* __CONSTANTS_RDPDR_H */

