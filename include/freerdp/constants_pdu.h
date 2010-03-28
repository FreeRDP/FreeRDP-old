/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   RDP PDU constants

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

#ifndef __CONSTANTS_PDU_H
#define __CONSTANTS_PDU_H

/* ISO PDU codes */
enum ISO_PDU_CODE
{
	ISO_PDU_CR = 0xE0,	/* Connection Request */
	ISO_PDU_CC = 0xD0,	/* Connection Confirm */
	ISO_PDU_DR = 0x80,	/* Disconnect Request */
	ISO_PDU_DT = 0xF0,	/* Data */
	ISO_PDU_ER = 0x70	/* Error */
};

/* MCS PDU codes */
enum MCS_PDU_TYPE
{
	MCS_EDRQ = 1,		/* Erect Domain Request */
	MCS_DPUM = 8,		/* Disconnect Provider Ultimatum */
	MCS_AURQ = 10,		/* Attach User Request */
	MCS_AUCF = 11,		/* Attach User Confirm */
	MCS_CJRQ = 14,		/* Channel Join Request */
	MCS_CJCF = 15,		/* Channel Join Confirm */
	MCS_SDRQ = 25,		/* Send Data Request */
	MCS_SDIN = 26		/* Send Data Indication */
};

/* RDP PDU codes */
enum RDP_PDU_TYPE
{
	RDP_PDU_DEMAND_ACTIVE = 1,
	RDP_PDU_CONFIRM_ACTIVE = 3,
	RDP_PDU_DEACTIVATE = 6,
	RDP_PDU_DATA = 7
};

enum RDP_DATA_PDU_TYPE
{
	RDP_DATA_PDU_UPDATE = 2,
	RDP_DATA_PDU_CONTROL = 20,
	RDP_DATA_PDU_POINTER = 27,
	RDP_DATA_PDU_INPUT = 28,
	RDP_DATA_PDU_SYNCHRONIZE = 31,
	RDP_DATA_PDU_BELL = 34,
	RDP_DATA_PDU_CLIENT_WINDOW_STATUS = 35,
	RDP_DATA_PDU_LOGON = 38,
	RDP_DATA_PDU_FONT2 = 39,
	RDP_DATA_PDU_KEYBOARD_INDICATORS = 41,
	RDP_DATA_PDU_DISCONNECT = 47
};

enum RDP_CONTROL_PDU_TYPE
{
	RDP_CTL_REQUEST_CONTROL = 1,
	RDP_CTL_GRANT_CONTROL = 2,
	RDP_CTL_DETACH = 3,
	RDP_CTL_COOPERATE = 4
};

enum RDP_UPDATE_PDU_TYPE
{
	RDP_UPDATE_ORDERS = 0,
	RDP_UPDATE_BITMAP = 1,
	RDP_UPDATE_PALETTE = 2,
	RDP_UPDATE_SYNCHRONIZE = 3
};

enum RDP_POINTER_PDU_TYPE
{
	RDP_PTRMSGTYPE_SYSTEM = 1,
	RDP_PTRMSGTYPE_POSITION = 3,
	RDP_PTRMSGTYPE_COLOR = 6,
	RDP_PTRMSGTYPE_CACHED = 7,
	RDP_PTRMSGTYPE_POINTER = 8
};

enum RDP_SYSTEM_POINTER_TYPE
{
	RDP_SYSPTR_NULL = 0,
	RDP_SYSPTR_DEFAULT = 0x7F00
};

enum REDIR_FLAGS
{
	LB_TARGET_NET_ADDRESS = 0x00000001,
	LB_LOAD_BALANCE_INFO = 0x00000002,
	LB_USERNAME = 0x00000004,
	LB_DOMAIN = 0x00000008,
	LB_PASSWORD = 0x00000010,
	LB_DONTSTOREUSERNAME = 0x00000020,
	LB_SMARTCARD_LOGON = 0x00000040,
	LB_NOREDIRECT = 0x00000080,
	LB_TARGET_FQDN = 0x00000100,
	LB_TARGET_NETBIOS_NAME = 0x00000200,
	LB_TARGET_NET_ADDRESSES = 0x00000800
};

/* RDP5 disconnect PDU */
#define exDiscReasonNoInfo				0x0000
#define exDiscReasonAPIInitiatedDisconnect		0x0001
#define exDiscReasonAPIInitiatedLogoff			0x0002
#define exDiscReasonServerIdleTimeout			0x0003
#define exDiscReasonServerLogonTimeout			0x0004
#define exDiscReasonReplacedByOtherConnection		0x0005
#define exDiscReasonOutOfMemory				0x0006
#define exDiscReasonServerDeniedConnection		0x0007
#define exDiscReasonServerDeniedConnectionFips		0x0008
#define exDiscReasonLicenseInternal			0x0100
#define exDiscReasonLicenseNoLicenseServer		0x0101
#define exDiscReasonLicenseNoLicense			0x0102
#define exDiscReasonLicenseErrClientMsg			0x0103
#define exDiscReasonLicenseHwidDoesntMatchLicense	0x0104
#define exDiscReasonLicenseErrClientLicense		0x0105
#define exDiscReasonLicenseCantFinishProtocol		0x0106
#define exDiscReasonLicenseClientEndedProtocol		0x0107
#define exDiscReasonLicenseErrClientEncryption		0x0108
#define exDiscReasonLicenseCantUpgradeLicense		0x0109
#define exDiscReasonLicenseNoRemoteConnections		0x010a

#define PDU_FLAG_FIRST		0x01
#define PDU_FLAG_LAST		0x02

#endif /* __CONSTANTS_PDU_H */
