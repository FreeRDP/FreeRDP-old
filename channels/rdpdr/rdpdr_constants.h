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

#ifndef __RDPDR_CONSTANTS_H
#define __RDPDR_CONSTANTS_H

/* RDPDR_HEADER.Component */
#define RDPDR_CTYP_CORE 0x4472 // "sD" "Ds" ???
#define RDPDR_CTYP_PRN  0x5052 // "RP" "PR" (PR)inting

/* RDPDR_HEADER.PacketId */
#define PAKID_CORE_SERVER_ANNOUNCE      0x496E // "nI" "nI" ???
#define PAKID_CORE_CLIENTID_CONFIRM     0x4343 // "CC" "CC" (C)lientID (C)onfirm
#define PAKID_CORE_CLIENT_NAME          0x434E // "NC" "CN" (C)lient (N)ame
#define PAKID_CORE_DEVICELIST_ANNOUNCE  0x4441 // "AD" "DA" (D)evice (A)nnounce
#define PAKID_CORE_DEVICE_REPLY         0x6472 // "rd" "dr" (d)evice (r)eply
#define PAKID_CORE_DEVICE_IOREQUEST     0x4952 // "RI" "IR" (I)O (R)equest
#define PAKID_CORE_DEVICE_IOCOMPLETION  0x4943 // "CI" "IC" (I)O (C)ompletion
#define PAKID_CORE_SERVER_CAPABILITY    0x5350 // "PS" "SP" (S)erver (C)apability
#define PAKID_CORE_CLIENT_CAPABILITY    0x4350 // "PC" "CP" (C)lient (C)apability
#define PAKID_CORE_DEVICELIST_REMOVE    0x444D // "MD" "DM" (D)evice list (R)emove
#define PAKID_PRN_CACHE_DATA            0x5043 // "CP" "PC" (P)rinter (C)ache data
#define PAKID_CORE_USER_LOGGEDON        0x554C // "LU" "UL" (U)ser (L)ogged on
#define PAKID_PRN_USING_XPS             0x5543 // "CU" "UC" (U)sing (?)XPS

/* CAPABILITY_HEADER.CapabilityType */
#define CAP_GENERAL_TYPE     0x0001
#define CAP_PRINTER_TYPE     0x0002
#define CAP_PORT_TYPE        0x0003
#define CAP_DRIVE_TYPE       0x0004
#define CAP_SMARTCARD_TYPE   0x0005

/* CAPABILITY_HEADER.Version */
#define GENERAL_CAPABILITY_VERSION_01   0x00000001
#define GENERAL_CAPABILITY_VERSION_02   0x00000002
#define PRINT_CAPABILITY_VERSION_01     0x00000001
#define PORT_CAPABILITY_VERSION_01      0x00000001
#define DRIVE_CAPABILITY_VERSION_01     0x00000001
#define DRIVE_CAPABILITY_VERSION_02     0x00000002
#define SMARTCARD_CAPABILITY_VERSION_01 0x00000001

/* DEVICE_ANNOUNCE.DeviceType */
#define RDPDR_DTYP_SERIAL               0x00000001
#define RDPDR_DTYP_PARALLEL             0x00000002
#define RDPDR_DTYP_PRINT                0x00000004
#define RDPDR_DTYP_FILESYSTEM           0x00000008
#define RDPDR_DTYP_SMARTCARD            0x00000020

/* DR_DEVICE_IOREQUEST.MajorFunction */
#define IRP_MJ_CREATE                   0x00000000
#define IRP_MJ_CLOSE                    0x00000002
#define IRP_MJ_READ                     0x00000003
#define IRP_MJ_WRITE                    0x00000004
#define IRP_MJ_DEVICE_CONTROL           0x0000000E
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0000000A
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0000000B
#define IRP_MJ_QUERY_INFORMATION        0x00000005
#define IRP_MJ_SET_INFORMATION          0x00000006
#define IRP_MJ_DIRECTORY_CONTROL        0x0000000C
#define IRP_MJ_LOCK_CONTROL             0x00000011

/* DR_DEVICE_IOREQUEST.MinorFunction */
#define IRP_MN_QUERY_DIRECTORY          0x00000001
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY  0x00000002

/* DR_CREATE_REQ.CreateDisposition */
#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005

/* DR_CREATE_RSP.Information */
/* DR_DRIVE_CREATE_RSP.DeviceCreateResponse */
#define FILE_SUPERSEDED                 0x00000000
#define FILE_OPENED                     0x00000001
#define FILE_OVERWRITTEN                0x00000003

/* DR_CORE_CLIENT_ANNOUNCE_RSP.VersionMinor */
#define RDPDR_MINOR_RDP_VERSION_5_0     0x0002
#define RDPDR_MINOR_RDP_VERSION_5_1     0x0005
#define RDPDR_MINOR_RDP_VERSION_5_2     0x000A
#define RDPDR_MINOR_RDP_VERSION_6_X     0x000C

/* DR_CORE_CLIENT_NAME_REQ.UnicodeFlag */
#define RDPDR_CLIENT_NAME_UNICODE       0x00000001
#define RDPDR_CLIENT_NAME_ASCII         0x00000000

/* GENERAL_CAPS_SET.ioCode1 */
#define RDPDR_IRP_MJ_CREATE                   0x00000001
#define RDPDR_IRP_MJ_CLEANUP                  0x00000002
#define RDPDR_IRP_MJ_CLOSE                    0x00000004
#define RDPDR_IRP_MJ_READ                     0x00000008
#define RDPDR_IRP_MJ_WRITE                    0x00000010
#define RDPDR_IRP_MJ_FLUSH_BUFFERS            0x00000020
#define RDPDR_IRP_MJ_SHUTDOWN                 0x00000040
#define RDPDR_IRP_MJ_DEVICE_CONTROL           0x00000080
#define RDPDR_IRP_MJ_QUERY_VOLUME_INFORMATION 0x00000100
#define RDPDR_IRP_MJ_SET_VOLUME_INFORMATION   0x00000200
#define RDPDR_IRP_MJ_QUERY_INFORMATION        0x00000400
#define RDPDR_IRP_MJ_SET_INFORMATION          0x00000800
#define RDPDR_IRP_MJ_DIRECTORY_CONTROL        0x00001000
#define RDPDR_IRP_MJ_LOCK_CONTROL             0x00002000
#define RDPDR_IRP_MJ_QUERY_SECURITY           0x00004000
#define RDPDR_IRP_MJ_SET_SECURITY             0x00008000

/* GENERAL_CAPS_SET.extendedPDU */
#define RDPDR_DEVICE_REMOVE_PDUS        0x00000001
#define RDPDR_CLIENT_DISPLAY_NAME_PDU   0x00000002
#define RDPDR_USER_LOGGEDON_PDU         0x00000004

/* GENERAL_CAPS_SET.extraFlags1 */
#define ENABLE_ASYNCIO                  0x00000001

/* DR_DRIVE_LOCK_REQ.Operation */
#define RDP_LOWIO_OP_SHAREDLOCK         0x00000002
#define RDP_LOWIO_OP_EXCLUSIVELOCK      0x00000003
#define RDP_LOWIO_OP_UNLOCK             0x00000004
#define RDP_LOWIO_OP_UNLOCK_MULTIPLE    0x00000005

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

#define RDPDR_MAX_DEVICES               0x10

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

/* Smartcard constants */
#define SCARD_LOCK_TCP		0
#define SCARD_LOCK_SEC		1
#define SCARD_LOCK_CHANNEL	2
#define SCARD_LOCK_RDPDR	3
#define SCARD_LOCK_LAST		4

#endif /* __CONSTANTS_RDPDR_H */

